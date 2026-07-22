#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace triumph::plugin_runtime
{
inline constexpr std::uint32_t protocolMagic = 0x54525048u; // TRPH
inline constexpr std::uint32_t protocolVersion = 1;

enum class WorkerState : std::uint32_t
{
    stopped,
    starting,
    running,
    stalled,
    crashed,
    restarting,
    disabled
};

enum class Fault : std::uint32_t
{
    none,
    launchFailed,
    protocolMismatch,
    deadlineMiss,
    heartbeatTimeout,
    workerExited,
    invalidAudio,
    invalidEvent
};

struct Configuration
{
    std::string slotId;
    std::string stablePluginId;
    double sampleRate = 48000.0;
    std::uint32_t maximumBlockSize = 2048;
    std::uint32_t inputChannels = 2;
    std::uint32_t outputChannels = 2;
    std::uint32_t sidechainChannels = 0;
    std::uint32_t maximumRestartAttempts = 3;
    std::uint64_t heartbeatTimeoutNanoseconds = 500000000;

    bool isValid() const noexcept
    {
        return ! slotId.empty() && ! stablePluginId.empty()
            && sampleRate >= 8000.0 && sampleRate <= 768000.0
            && maximumBlockSize > 0 && maximumBlockSize <= 8192
            && inputChannels <= 16 && outputChannels > 0
            && outputChannels <= 16 && sidechainChannels <= 16
            && heartbeatTimeoutNanoseconds > 0;
    }
};

struct ParameterEvent
{
    std::uint32_t parameterIndex = 0;
    std::uint32_t sampleOffset = 0;
    float normalisedValue = 0.0f;
};

struct MidiEvent
{
    std::uint32_t sampleOffset = 0;
    std::uint8_t size = 0;
    std::array<std::uint8_t, 16> bytes {};
};

struct BlockResult
{
    std::uint32_t sequence = 0;
    std::uint32_t samples = 0;
    std::uint32_t channels = 0;
    std::uint32_t reportedLatencySamples = 0;
    std::uint64_t processNanoseconds = 0;
    Fault fault = Fault::none;
};

template <std::size_t MaximumChannels = 16,
          std::size_t MaximumBlockSize = 2048,
          std::size_t MaximumParameterEvents = 512,
          std::size_t MaximumMidiEvents = 1024>
class SharedBlockBridge final
{
public:
    static constexpr std::size_t slotCount = 3;

    struct alignas (64) Slot
    {
        std::atomic<std::uint32_t> requestedSequence { 0 };
        std::atomic<std::uint32_t> completedSequence { 0 };
        std::atomic<std::uint32_t> workerClaim { 0 };
        std::uint32_t samples = 0;
        std::uint32_t inputChannels = 0;
        std::uint32_t outputChannels = 0;
        std::uint32_t parameterEventCount = 0;
        std::uint32_t midiEventCount = 0;
        std::uint32_t reportedLatencySamples = 0;
        std::uint64_t processNanoseconds = 0;
        std::uint32_t fault = static_cast<std::uint32_t> (Fault::none);
        std::array<float, MaximumChannels * MaximumBlockSize> input {};
        std::array<float, MaximumChannels * MaximumBlockSize> output {};
        std::array<ParameterEvent, MaximumParameterEvents> parameterEvents {};
        std::array<MidiEvent, MaximumMidiEvents> midiEvents {};
    };

    SharedBlockBridge() = default;
    SharedBlockBridge (const SharedBlockBridge&) = delete;
    SharedBlockBridge& operator= (const SharedBlockBridge&) = delete;

    std::uint32_t submit (const float* const* inputChannels,
                          std::uint32_t inputChannelCount,
                          std::uint32_t outputChannelCount,
                          std::uint32_t samples,
                          const ParameterEvent* parameters,
                          std::uint32_t parameterCount,
                          const MidiEvent* midi,
                          std::uint32_t midiCount) noexcept
    {
        if (samples == 0 || samples > MaximumBlockSize
            || inputChannelCount > MaximumChannels
            || outputChannelCount > MaximumChannels
            || parameterCount > MaximumParameterEvents
            || midiCount > MaximumMidiEvents)
            return 0;

        const auto sequence = nextSequence.fetch_add (1,
            std::memory_order_relaxed) + 1;
        auto& slot = slots[sequence % slotCount];
        if (slot.requestedSequence.load (std::memory_order_acquire)
            != slot.completedSequence.load (std::memory_order_acquire))
            return 0;

        slot.samples = samples;
        slot.inputChannels = inputChannelCount;
        slot.outputChannels = outputChannelCount;
        slot.parameterEventCount = parameterCount;
        slot.midiEventCount = midiCount;
        slot.reportedLatencySamples = 0;
        slot.processNanoseconds = 0;
        slot.fault = static_cast<std::uint32_t> (Fault::none);

        for (std::uint32_t channel = 0; channel < inputChannelCount; ++channel)
        {
            const auto* source = inputChannels != nullptr
                                     ? inputChannels[channel] : nullptr;
            auto* destination = slot.input.data()
                + static_cast<std::size_t> (channel) * MaximumBlockSize;
            if (source != nullptr)
                std::copy_n (source, samples, destination);
            else
                std::fill_n (destination, samples, 0.0f);
        }
        if (parameters != nullptr)
            std::copy_n (parameters, parameterCount,
                         slot.parameterEvents.begin());
        if (midi != nullptr)
            std::copy_n (midi, midiCount, slot.midiEvents.begin());

        slot.workerClaim.store (0, std::memory_order_relaxed);
        slot.requestedSequence.store (sequence, std::memory_order_release);
        return sequence;
    }

    Slot* claimNextForWorker() noexcept
    {
        Slot* selected = nullptr;
        auto selectedSequence = std::uint32_t { 0 };
        for (auto& slot : slots)
        {
            const auto requested = slot.requestedSequence.load (
                std::memory_order_acquire);
            if (requested == 0
                || requested == slot.completedSequence.load (
                    std::memory_order_acquire)
                || requested <= selectedSequence)
                continue;
            auto expected = std::uint32_t { 0 };
            if (slot.workerClaim.compare_exchange_strong (
                    expected, requested, std::memory_order_acq_rel))
            {
                selected = &slot;
                selectedSequence = requested;
            }
        }
        return selected;
    }

    void completeFromWorker (Slot& slot, Fault fault,
                             std::uint32_t latencySamples,
                             std::uint64_t elapsedNanoseconds) noexcept
    {
        const auto sequence = slot.workerClaim.load (
            std::memory_order_acquire);
        slot.fault = static_cast<std::uint32_t> (fault);
        slot.reportedLatencySamples = latencySamples;
        slot.processNanoseconds = elapsedNanoseconds;
        slot.completedSequence.store (sequence, std::memory_order_release);
    }

    bool consumeLatest (float* const* outputChannels,
                        std::uint32_t outputChannelCount,
                        std::uint32_t samples,
                        BlockResult& result) noexcept
    {
        Slot* selected = nullptr;
        auto selectedSequence = lastConsumedSequence.load (
            std::memory_order_relaxed);
        for (auto& slot : slots)
        {
            const auto completed = slot.completedSequence.load (
                std::memory_order_acquire);
            if (completed > selectedSequence)
            {
                selected = &slot;
                selectedSequence = completed;
            }
        }
        if (selected == nullptr || selected->samples != samples)
            return false;

        const auto channels = std::min<std::uint32_t> (
            { outputChannelCount, selected->outputChannels,
              static_cast<std::uint32_t> (MaximumChannels) });
        for (std::uint32_t channel = 0; channel < channels; ++channel)
        {
            auto* destination = outputChannels != nullptr
                                    ? outputChannels[channel] : nullptr;
            if (destination == nullptr)
                continue;
            const auto* source = selected->output.data()
                + static_cast<std::size_t> (channel) * MaximumBlockSize;
            std::copy_n (source, samples, destination);
        }
        result = { selectedSequence, samples, channels,
                   selected->reportedLatencySamples,
                   selected->processNanoseconds,
                   static_cast<Fault> (selected->fault) };
        lastConsumedSequence.store (selectedSequence,
                                    std::memory_order_release);
        return true;
    }

    std::array<Slot, slotCount>& sharedSlots() noexcept { return slots; }
    const std::array<Slot, slotCount>& sharedSlots() const noexcept
    {
        return slots;
    }

private:
    std::array<Slot, slotCount> slots {};
    std::atomic<std::uint32_t> nextSequence { 0 };
    std::atomic<std::uint32_t> lastConsumedSequence { 0 };
};

class Watchdog final
{
public:
    explicit Watchdog (Configuration configuration)
        : config (std::move (configuration))
    {
    }

    bool beginLaunch (std::uint64_t nowNanoseconds) noexcept
    {
        if (! config.isValid() || state == WorkerState::disabled
            || nowNanoseconds < restartAllowedAt)
            return false;
        state = restartAttempts == 0 ? WorkerState::starting
                                     : WorkerState::restarting;
        lastHeartbeat = nowNanoseconds;
        lastCompletion = nowNanoseconds;
        fault = Fault::none;
        return true;
    }

    void markRunning (std::uint64_t nowNanoseconds) noexcept
    {
        state = WorkerState::running;
        lastHeartbeat = nowNanoseconds;
        lastCompletion = nowNanoseconds;
    }

    void heartbeat (std::uint64_t nowNanoseconds) noexcept
    {
        if (state == WorkerState::running)
            lastHeartbeat = nowNanoseconds;
    }

    void blockCompleted (std::uint64_t nowNanoseconds) noexcept
    {
        if (state == WorkerState::running)
            lastCompletion = nowNanoseconds;
    }

    bool poll (std::uint64_t nowNanoseconds,
               bool processStillRunning) noexcept
    {
        if (state != WorkerState::running
            && state != WorkerState::starting
            && state != WorkerState::restarting)
            return false;
        if (! processStillRunning)
        {
            fail (Fault::workerExited, nowNanoseconds);
            return true;
        }
        if (nowNanoseconds > lastHeartbeat
            && nowNanoseconds - lastHeartbeat
                   > config.heartbeatTimeoutNanoseconds)
        {
            fail (Fault::heartbeatTimeout, nowNanoseconds);
            return true;
        }
        return false;
    }

    void launchFailed (std::uint64_t nowNanoseconds) noexcept
    {
        fail (Fault::launchFailed, nowNanoseconds);
    }

    void disable() noexcept { state = WorkerState::disabled; }
    void resetRecoveryBudget() noexcept
    {
        restartAttempts = 0;
        restartAllowedAt = 0;
        if (state == WorkerState::disabled)
            state = WorkerState::stopped;
    }

    WorkerState getState() const noexcept { return state; }
    Fault getFault() const noexcept { return fault; }
    std::uint32_t getRestartAttempts() const noexcept
    {
        return restartAttempts;
    }
    std::uint64_t getRestartAllowedAt() const noexcept
    {
        return restartAllowedAt;
    }

private:
    void fail (Fault newFault, std::uint64_t nowNanoseconds) noexcept
    {
        fault = newFault;
        ++restartAttempts;
        if (restartAttempts > config.maximumRestartAttempts)
        {
            state = WorkerState::disabled;
            return;
        }
        state = newFault == Fault::heartbeatTimeout
                    ? WorkerState::stalled : WorkerState::crashed;
        const auto shift = std::min<std::uint32_t> (restartAttempts - 1, 5);
        restartAllowedAt = nowNanoseconds
            + (static_cast<std::uint64_t> (1) << shift) * 100000000ULL;
    }

    Configuration config;
    WorkerState state = WorkerState::stopped;
    Fault fault = Fault::none;
    std::uint32_t restartAttempts = 0;
    std::uint64_t lastHeartbeat = 0;
    std::uint64_t lastCompletion = 0;
    std::uint64_t restartAllowedAt = 0;
};
}
