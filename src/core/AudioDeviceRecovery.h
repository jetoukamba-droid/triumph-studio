#pragma once

#include <atomic>
#include <cstdint>

namespace triumph::device
{
enum class RecoveryState : std::uint8_t
{
    stable,
    interrupted,
    reopening,
    recovered,
    failed
};

struct RecoverySnapshot
{
    RecoveryState state = RecoveryState::stable;
    std::uint64_t interruptions = 0;
    std::uint64_t attempts = 0;
    std::uint64_t recoveries = 0;
    std::uint64_t failures = 0;
    bool resumePending = false;
};

// A lock-free event ledger shared by the audio-device lifecycle and the
// message thread. It retains transport intent but never owns or mutates the
// transport itself.
class RecoveryTracker final
{
public:
    void noteUnavailable (bool playbackWasRunning) noexcept
    {
        auto current = state.load (std::memory_order_acquire);
        for (;;)
        {
            if (current == RecoveryState::interrupted
                || current == RecoveryState::reopening
                || current == RecoveryState::failed)
            {
                if (playbackWasRunning)
                    resumePending.store (true, std::memory_order_release);
                return;
            }

            if (state.compare_exchange_weak (
                    current, RecoveryState::interrupted,
                    std::memory_order_acq_rel, std::memory_order_acquire))
            {
                interruptions.fetch_add (1, std::memory_order_relaxed);
                if (playbackWasRunning)
                    resumePending.store (true, std::memory_order_release);
                return;
            }
        }
    }

    bool beginRecovery() noexcept
    {
        auto current = state.load (std::memory_order_acquire);
        while (current == RecoveryState::interrupted
               || current == RecoveryState::failed)
        {
            if (state.compare_exchange_weak (
                    current, RecoveryState::reopening,
                    std::memory_order_acq_rel, std::memory_order_acquire))
            {
                attempts.fetch_add (1, std::memory_order_relaxed);
                return true;
            }
        }
        return false;
    }

    // Returns true exactly once when interrupted playback should resume.
    bool markReady() noexcept
    {
        auto current = state.load (std::memory_order_acquire);
        while (current == RecoveryState::interrupted
               || current == RecoveryState::reopening
               || current == RecoveryState::failed)
        {
            if (state.compare_exchange_weak (
                    current, RecoveryState::recovered,
                    std::memory_order_acq_rel, std::memory_order_acquire))
            {
                recoveries.fetch_add (1, std::memory_order_relaxed);
                return resumePending.exchange (false,
                                               std::memory_order_acq_rel);
            }
        }
        return false;
    }

    void markFailed() noexcept
    {
        auto expected = RecoveryState::reopening;
        if (state.compare_exchange_strong (
                expected, RecoveryState::failed,
                std::memory_order_acq_rel, std::memory_order_acquire))
            failures.fetch_add (1, std::memory_order_relaxed);
    }

    void settleRecovered() noexcept
    {
        auto expected = RecoveryState::recovered;
        state.compare_exchange_strong (
            expected, RecoveryState::stable,
            std::memory_order_acq_rel, std::memory_order_acquire);
    }

    void cancelResume() noexcept
    {
        resumePending.store (false, std::memory_order_release);
    }

    RecoverySnapshot snapshot() const noexcept
    {
        return {
            state.load (std::memory_order_acquire),
            interruptions.load (std::memory_order_relaxed),
            attempts.load (std::memory_order_relaxed),
            recoveries.load (std::memory_order_relaxed),
            failures.load (std::memory_order_relaxed),
            resumePending.load (std::memory_order_acquire)
        };
    }

private:
    std::atomic<RecoveryState> state { RecoveryState::stable };
    std::atomic<std::uint64_t> interruptions { 0 };
    std::atomic<std::uint64_t> attempts { 0 };
    std::atomic<std::uint64_t> recoveries { 0 };
    std::atomic<std::uint64_t> failures { 0 };
    std::atomic<bool> resumePending { false };
};
}
