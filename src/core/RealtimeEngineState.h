#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace triumph::realtime
{
struct ParameterCommand
{
    enum class Type : std::uint8_t
    {
        masterGain,
        masterMute
    };

    Type type = Type::masterGain;
    float value = 0.0f;
};

template <std::size_t Capacity>
class ParameterQueue final
{
    static_assert (Capacity >= 2);

public:
    bool push (ParameterCommand command) noexcept
    {
        const auto write = writePosition.load (std::memory_order_relaxed);
        const auto next = increment (write);
        if (next == readPosition.load (std::memory_order_acquire))
            return false;

        commands[write] = command;
        writePosition.store (next, std::memory_order_release);
        return true;
    }

    bool pop (ParameterCommand& command) noexcept
    {
        const auto read = readPosition.load (std::memory_order_relaxed);
        if (read == writePosition.load (std::memory_order_acquire))
            return false;

        command = commands[read];
        readPosition.store (increment (read), std::memory_order_release);
        return true;
    }

    void reset() noexcept
    {
        readPosition.store (0, std::memory_order_relaxed);
        writePosition.store (0, std::memory_order_relaxed);
    }

private:
    static constexpr std::size_t increment (std::size_t position) noexcept
    {
        return (position + 1) % Capacity;
    }

    std::array<ParameterCommand, Capacity> commands {};
    std::atomic<std::size_t> readPosition { 0 };
    std::atomic<std::size_t> writePosition { 0 };
};

struct TelemetrySnapshot
{
    std::uint64_t callbackCount = 0;
    std::uint64_t totalCallbackNanoseconds = 0;
    std::uint64_t maximumCallbackNanoseconds = 0;
    std::uint64_t deadlineMisses = 0;
    std::uint64_t suspectedDropouts = 0;
    std::uint64_t snapshotSwaps = 0;
    std::uint64_t parameterQueueOverflows = 0;
    std::uint64_t renderGeneration = 0;
    std::uint64_t missingRenderStateBlocks = 0;
    std::uint64_t oversizedAudioBlocks = 0;
    std::uint64_t readAheadStarvationBlocks = 0;
    std::uint64_t deviceRestartCount = 0;
    std::uint64_t variableBlockSizeChanges = 0;
    std::uint64_t graphBuildCount = 0;
    std::uint64_t totalGraphBuildNanoseconds = 0;
    std::uint64_t maximumGraphBuildNanoseconds = 0;
    std::uint64_t renderRetireBacklogHighWatermark = 0;

    double averageCallbackNanoseconds() const noexcept
    {
        return callbackCount == 0
                   ? 0.0
                   : static_cast<double> (totalCallbackNanoseconds)
                         / static_cast<double> (callbackCount);
    }

    double averageGraphBuildNanoseconds() const noexcept
    {
        return graphBuildCount == 0
                   ? 0.0
                   : static_cast<double> (totalGraphBuildNanoseconds)
                         / static_cast<double> (graphBuildCount);
    }
};

class Telemetry final
{
public:
    void recordCallback (std::uint64_t elapsedNanoseconds,
                         std::uint64_t deadlineNanoseconds,
                         bool suspectedDropout = false) noexcept
    {
        callbackCount.fetch_add (1, std::memory_order_relaxed);
        totalCallbackNanoseconds.fetch_add (elapsedNanoseconds,
                                            std::memory_order_relaxed);

        auto maximum = maximumCallbackNanoseconds.load (
            std::memory_order_relaxed);
        while (elapsedNanoseconds > maximum
               && ! maximumCallbackNanoseconds.compare_exchange_weak (
                   maximum, elapsedNanoseconds, std::memory_order_relaxed))
        {
        }

        const auto missed = deadlineNanoseconds > 0
                            && elapsedNanoseconds > deadlineNanoseconds;
        if (missed)
            deadlineMisses.fetch_add (1, std::memory_order_relaxed);
        if (missed || suspectedDropout)
            suspectedDropouts.fetch_add (1, std::memory_order_relaxed);
    }

    void recordSnapshotSwap (std::uint64_t generation) noexcept
    {
        snapshotSwaps.fetch_add (1, std::memory_order_relaxed);
        renderGeneration.store (generation, std::memory_order_relaxed);
    }

    void recordParameterQueueOverflow() noexcept
    {
        parameterQueueOverflows.fetch_add (1, std::memory_order_relaxed);
    }

    void recordMissingRenderStateBlock() noexcept
    {
        missingRenderStateBlocks.fetch_add (1, std::memory_order_relaxed);
        suspectedDropouts.fetch_add (1, std::memory_order_relaxed);
    }

    void recordOversizedAudioBlock() noexcept
    {
        oversizedAudioBlocks.fetch_add (1, std::memory_order_relaxed);
        suspectedDropouts.fetch_add (1, std::memory_order_relaxed);
    }

    void recordReadAheadStarvationBlock() noexcept
    {
        readAheadStarvationBlocks.fetch_add (1, std::memory_order_relaxed);
        suspectedDropouts.fetch_add (1, std::memory_order_relaxed);
    }

    void recordDeviceRestart (int blockSize,
                              std::uint64_t sampleRateMilliHz) noexcept
    {
        deviceRestartCount.fetch_add (1, std::memory_order_relaxed);
        const auto previousBlock =
            lastPreparedBlockSize.exchange (blockSize,
                                            std::memory_order_relaxed);
        const auto previousRate =
            lastPreparedSampleRateMilliHz.exchange (
                sampleRateMilliHz, std::memory_order_relaxed);
        if ((previousBlock > 0 && previousBlock != blockSize)
            || (previousRate > 0 && previousRate != sampleRateMilliHz))
            variableBlockSizeChanges.fetch_add (1, std::memory_order_relaxed);
    }

    void recordGraphBuild (std::uint64_t elapsedNanoseconds,
                           std::uint64_t retiredBacklog) noexcept
    {
        graphBuildCount.fetch_add (1, std::memory_order_relaxed);
        totalGraphBuildNanoseconds.fetch_add (elapsedNanoseconds,
                                              std::memory_order_relaxed);

        auto maximum = maximumGraphBuildNanoseconds.load (
            std::memory_order_relaxed);
        while (elapsedNanoseconds > maximum
               && ! maximumGraphBuildNanoseconds.compare_exchange_weak (
                   maximum, elapsedNanoseconds, std::memory_order_relaxed))
        {
        }

        auto backlogMaximum = renderRetireBacklogHighWatermark.load (
            std::memory_order_relaxed);
        while (retiredBacklog > backlogMaximum
               && ! renderRetireBacklogHighWatermark.compare_exchange_weak (
                   backlogMaximum, retiredBacklog,
                   std::memory_order_relaxed))
        {
        }
    }

    TelemetrySnapshot snapshot() const noexcept
    {
        return {
            callbackCount.load (std::memory_order_relaxed),
            totalCallbackNanoseconds.load (std::memory_order_relaxed),
            maximumCallbackNanoseconds.load (std::memory_order_relaxed),
            deadlineMisses.load (std::memory_order_relaxed),
            suspectedDropouts.load (std::memory_order_relaxed),
            snapshotSwaps.load (std::memory_order_relaxed),
            parameterQueueOverflows.load (std::memory_order_relaxed),
            renderGeneration.load (std::memory_order_relaxed),
            missingRenderStateBlocks.load (std::memory_order_relaxed),
            oversizedAudioBlocks.load (std::memory_order_relaxed),
            readAheadStarvationBlocks.load (std::memory_order_relaxed),
            deviceRestartCount.load (std::memory_order_relaxed),
            variableBlockSizeChanges.load (std::memory_order_relaxed),
            graphBuildCount.load (std::memory_order_relaxed),
            totalGraphBuildNanoseconds.load (std::memory_order_relaxed),
            maximumGraphBuildNanoseconds.load (std::memory_order_relaxed),
            renderRetireBacklogHighWatermark.load (
                std::memory_order_relaxed)
        };
    }

private:
    std::atomic<std::uint64_t> callbackCount { 0 };
    std::atomic<std::uint64_t> totalCallbackNanoseconds { 0 };
    std::atomic<std::uint64_t> maximumCallbackNanoseconds { 0 };
    std::atomic<std::uint64_t> deadlineMisses { 0 };
    std::atomic<std::uint64_t> suspectedDropouts { 0 };
    std::atomic<std::uint64_t> snapshotSwaps { 0 };
    std::atomic<std::uint64_t> parameterQueueOverflows { 0 };
    std::atomic<std::uint64_t> renderGeneration { 0 };
    std::atomic<std::uint64_t> missingRenderStateBlocks { 0 };
    std::atomic<std::uint64_t> oversizedAudioBlocks { 0 };
    std::atomic<std::uint64_t> readAheadStarvationBlocks { 0 };
    std::atomic<std::uint64_t> deviceRestartCount { 0 };
    std::atomic<std::uint64_t> variableBlockSizeChanges { 0 };
    std::atomic<std::uint64_t> graphBuildCount { 0 };
    std::atomic<std::uint64_t> totalGraphBuildNanoseconds { 0 };
    std::atomic<std::uint64_t> maximumGraphBuildNanoseconds { 0 };
    std::atomic<std::uint64_t> renderRetireBacklogHighWatermark { 0 };
    std::atomic<int> lastPreparedBlockSize { 0 };
    std::atomic<std::uint64_t> lastPreparedSampleRateMilliHz { 0 };
};

// One control/publisher thread and any number of short-lived real-time readers.
// Publication is a single atomic pointer store. Superseded objects are retained
// until every reader has left, then reclaimed explicitly by the control thread.
// A ReadHandle never owns or destroys the object it exposes.
template <typename Snapshot>
class SnapshotExchange final
{
public:
    class ReadHandle final
    {
    public:
        ReadHandle() = default;
        ReadHandle (const ReadHandle&) = delete;
        ReadHandle& operator= (const ReadHandle&) = delete;

        ReadHandle (ReadHandle&& other) noexcept
            : owner (std::exchange (other.owner, nullptr)),
              value (std::exchange (other.value, nullptr))
        {
        }

        ReadHandle& operator= (ReadHandle&& other) noexcept
        {
            if (this != &other)
            {
                release();
                owner = std::exchange (other.owner, nullptr);
                value = std::exchange (other.value, nullptr);
            }
            return *this;
        }

        ~ReadHandle() { release(); }

        Snapshot* get() const noexcept { return value; }
        Snapshot& operator*() const noexcept { return *value; }
        Snapshot* operator->() const noexcept { return value; }
        explicit operator bool() const noexcept { return value != nullptr; }

    private:
        friend class SnapshotExchange;

        ReadHandle (SnapshotExchange& exchange, Snapshot* snapshot) noexcept
            : owner (&exchange), value (snapshot)
        {
        }

        void release() noexcept
        {
            if (owner != nullptr)
            {
                owner->readerCount.fetch_sub (1, std::memory_order_seq_cst);
                owner = nullptr;
                value = nullptr;
            }
        }

        SnapshotExchange* owner = nullptr;
        Snapshot* value = nullptr;
    };

    ReadHandle acquire() noexcept
    {
        // The increment is sequenced before the pointer read. A publisher that
        // replaces the pointer must therefore observe this reader before it can
        // reclaim the previous object.
        readerCount.fetch_add (1, std::memory_order_seq_cst);
        return ReadHandle (*this, published.load (std::memory_order_seq_cst));
    }

    void publish (std::unique_ptr<Snapshot> next)
    {
        auto previous = std::move (current);
        current = std::move (next);
        published.store (current.get(), std::memory_order_seq_cst);
        if (previous != nullptr)
            retired.push_back (std::move (previous));
        reclaim();
    }

    void clear()
    {
        publish (nullptr);
    }

    void reclaim()
    {
        if (readerCount.load (std::memory_order_seq_cst) == 0)
            retired.clear();
    }

    std::size_t retiredCount() const noexcept { return retired.size(); }
    std::uint32_t readers() const noexcept
    {
        return readerCount.load (std::memory_order_seq_cst);
    }

private:
    std::atomic<Snapshot*> published { nullptr };
    std::atomic<std::uint32_t> readerCount { 0 };
    std::unique_ptr<Snapshot> current;
    std::vector<std::unique_ptr<Snapshot>> retired;
};
}
