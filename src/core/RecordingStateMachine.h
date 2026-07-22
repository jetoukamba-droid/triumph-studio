#pragma once

#include <atomic>
#include <cstdint>

namespace triumph::recording
{
enum class State : std::uint8_t
{
    idle,
    armed,
    preRoll,
    recording,
    punch,
    loopPass,
    finalizing,
    finalized,
    aborted,
    recovered
};

enum class CaptureMode : std::uint8_t
{
    normal,
    punch,
    loop
};

class StateMachine final
{
public:
    State get() const noexcept
    {
        return state.load (std::memory_order_acquire);
    }

    int getLoopPass() const noexcept
    {
        return loopPass.load (std::memory_order_relaxed);
    }

    bool arm() noexcept
    {
        auto current = state.load (std::memory_order_acquire);
        while (current == State::idle || current == State::finalized
               || current == State::aborted || current == State::recovered)
        {
            if (state.compare_exchange_weak (current, State::armed,
                                             std::memory_order_acq_rel))
            {
                loopPass.store (0, std::memory_order_relaxed);
                return true;
            }
        }
        return current == State::armed;
    }

    bool beginPreRoll() noexcept
    {
        auto expected = State::armed;
        return state.compare_exchange_strong (expected, State::preRoll,
                                              std::memory_order_acq_rel);
    }

    bool beginCapture (CaptureMode mode = CaptureMode::normal) noexcept
    {
        auto current = state.load (std::memory_order_acquire);
        if (current != State::armed && current != State::preRoll)
            return false;

        const auto next = mode == CaptureMode::punch ? State::punch
                         : mode == CaptureMode::loop ? State::loopPass
                                                     : State::recording;
        if (! state.compare_exchange_strong (current, next,
                                             std::memory_order_acq_rel))
            return false;
        loopPass.store (mode == CaptureMode::loop ? 1 : 0,
                        std::memory_order_relaxed);
        return true;
    }

    bool beginNextLoopPass() noexcept
    {
        if (get() != State::loopPass)
            return false;
        loopPass.fetch_add (1, std::memory_order_relaxed);
        return true;
    }

    bool beginFinalize() noexcept
    {
        auto current = state.load (std::memory_order_acquire);
        while (current == State::armed || current == State::preRoll
               || current == State::recording || current == State::punch
               || current == State::loopPass)
        {
            if (state.compare_exchange_weak (current, State::finalizing,
                                             std::memory_order_acq_rel))
                return true;
        }
        return current == State::finalizing;
    }

    bool finishFinalize (bool validCapture) noexcept
    {
        auto expected = State::finalizing;
        return state.compare_exchange_strong (
            expected, validCapture ? State::finalized : State::aborted,
            std::memory_order_acq_rel);
    }

    void abort() noexcept
    {
        state.store (State::aborted, std::memory_order_release);
    }

    bool markRecovered() noexcept
    {
        auto current = state.load (std::memory_order_acquire);
        while (current == State::aborted || current == State::finalized)
            if (state.compare_exchange_weak (current, State::recovered,
                                             std::memory_order_acq_rel))
                return true;
        return current == State::recovered;
    }

    void reset() noexcept
    {
        loopPass.store (0, std::memory_order_relaxed);
        state.store (State::idle, std::memory_order_release);
    }

    bool isCapturing() const noexcept
    {
        const auto current = get();
        return current == State::recording || current == State::punch
               || current == State::loopPass;
    }

    bool isActive() const noexcept
    {
        const auto current = get();
        return current == State::armed || current == State::preRoll
               || isCapturing() || current == State::finalizing;
    }

private:
    std::atomic<State> state { State::idle };
    std::atomic<int> loopPass { 0 };
};
}
