#pragma once

#include "RecordingStateMachine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace triumph::recording
{
enum class SessionMode : std::uint8_t
{
    normal,
    punch,
    loop
};

struct PlanSettings
{
    SessionMode mode = SessionMode::normal;
    std::int64_t captureStartSamples = 0;
    std::int64_t captureEndSamples = 0;
    std::int64_t preRollSamples = 0;
};

struct CaptureSpan
{
    int frameOffset = 0;
    int frameCount = 0;
    int passNumber = 1;
};

struct BlockPlan
{
    static constexpr int maximumSpans = 8;

    std::array<CaptureSpan, maximumSpans> spans {};
    int spanCount = 0;
    bool captureStarted = false;
    bool captureFinished = false;
    bool loopWrapped = false;
    std::int64_t wrappedTimelinePosition = 0;

    bool add (CaptureSpan span) noexcept
    {
        if (span.frameCount <= 0 || spanCount >= maximumSpans)
            return false;
        spans[static_cast<std::size_t> (spanCount++)] = span;
        return true;
    }
};

class Plan final
{
public:
    bool configure (PlanSettings requested,
                    double timelineSampleRate,
                    double deviceSampleRate,
                    int maximumDeviceBlockSize) noexcept
    {
        reset();
        if (timelineSampleRate <= 0.0 || deviceSampleRate <= 0.0
            || maximumDeviceBlockSize <= 0)
            return false;

        requested.captureStartSamples = std::max<std::int64_t> (
            0, requested.captureStartSamples);
        requested.preRollSamples = std::max<std::int64_t> (
            0, requested.preRollSamples);
        if (requested.mode != SessionMode::normal
            && requested.captureEndSamples <= requested.captureStartSamples)
            return false;

        const auto timelinePerFrame = timelineSampleRate / deviceSampleRate;
        const auto minimumLoopLength = static_cast<std::int64_t> (
            std::ceil (maximumDeviceBlockSize * timelinePerFrame));
        if (requested.mode == SessionMode::loop
            && requested.captureEndSamples - requested.captureStartSamples
                   < minimumLoopLength)
            return false;

        settings = requested;
        timelineRate = timelineSampleRate;
        deviceRate = deviceSampleRate;
        configured = stateMachine.arm();
        if (configured && settings.preRollSamples > 0)
            configured = stateMachine.beginPreRoll();
        return configured;
    }

    void reset() noexcept
    {
        configured = false;
        currentPass = 1;
        stateMachine.reset();
    }

    bool isConfigured() const noexcept { return configured; }

    std::int64_t getTransportStartSample() const noexcept
    {
        return std::max<std::int64_t> (
            0, settings.captureStartSamples - settings.preRollSamples);
    }

    std::int64_t getCaptureStartSample() const noexcept
    {
        return settings.captureStartSamples;
    }

    std::int64_t getCaptureEndSample() const noexcept
    {
        return settings.captureEndSamples;
    }

    SessionMode getMode() const noexcept { return settings.mode; }
    double getTimelineSampleRate() const noexcept { return timelineRate; }
    State getState() const noexcept { return stateMachine.get(); }
    int getCurrentPass() const noexcept { return currentPass; }

    BlockPlan processBlock (std::int64_t blockStartTimelineSample,
                            int deviceFrames) noexcept
    {
        BlockPlan result;
        if (! configured || deviceFrames <= 0
            || stateMachine.get() == State::finalizing
            || stateMachine.get() == State::finalized
            || stateMachine.get() == State::aborted)
            return result;

        const auto ratio = timelineRate / deviceRate;
        const auto blockEndTimelineSample = blockStartTimelineSample
            + static_cast<std::int64_t> (std::llround (deviceFrames * ratio));

        if (! stateMachine.isCapturing()
            && blockEndTimelineSample > settings.captureStartSamples)
        {
            const auto mode = settings.mode == SessionMode::punch
                                  ? CaptureMode::punch
                              : settings.mode == SessionMode::loop
                                  ? CaptureMode::loop
                                  : CaptureMode::normal;
            result.captureStarted = stateMachine.beginCapture (mode);
        }

        if (! stateMachine.isCapturing())
            return result;

        if (settings.mode == SessionMode::normal)
        {
            const auto firstFrame = frameAtBoundary (
                settings.captureStartSamples, blockStartTimelineSample,
                deviceFrames, ratio);
            result.add ({ firstFrame, deviceFrames - firstFrame, 1 });
            return result;
        }

        if (settings.mode == SessionMode::punch)
        {
            const auto firstFrame = frameAtBoundary (
                settings.captureStartSamples, blockStartTimelineSample,
                deviceFrames, ratio);
            const auto endFrame = frameAtBoundary (
                settings.captureEndSamples, blockStartTimelineSample,
                deviceFrames, ratio);
            result.add ({ firstFrame, endFrame - firstFrame, 1 });
            if (blockEndTimelineSample >= settings.captureEndSamples)
            {
                result.captureFinished = true;
                stateMachine.beginFinalize();
            }
            return result;
        }

        auto frameCursor = 0;
        auto timelineCursor = blockStartTimelineSample;
        auto pass = currentPass;
        while (frameCursor < deviceFrames
               && result.spanCount < BlockPlan::maximumSpans)
        {
            if (timelineCursor < settings.captureStartSamples)
            {
                const auto startFrame = frameAtBoundary (
                    settings.captureStartSamples, blockStartTimelineSample,
                    deviceFrames, ratio);
                frameCursor = std::max (frameCursor, startFrame);
                timelineCursor = settings.captureStartSamples;
                if (frameCursor >= deviceFrames)
                    break;
            }

            const auto framesToEnd = frameAtBoundary (
                settings.captureEndSamples, timelineCursor,
                deviceFrames - frameCursor, ratio);
            const auto amount = std::min (deviceFrames - frameCursor,
                                          framesToEnd);
            result.add ({ frameCursor, amount, pass });
            frameCursor += amount;
            timelineCursor += static_cast<std::int64_t> (
                std::llround (amount * ratio));

            if (frameCursor < deviceFrames
                || timelineCursor >= settings.captureEndSamples)
            {
                result.loopWrapped = true;
                ++pass;
                stateMachine.beginNextLoopPass();
                timelineCursor = settings.captureStartSamples;
                if (amount == 0 && frameCursor < deviceFrames)
                {
                    // A rounding boundary must always make forward progress.
                    result.add ({ frameCursor, 1, pass });
                    ++frameCursor;
                    timelineCursor += static_cast<std::int64_t> (
                        std::llround (ratio));
                }
            }
        }
        currentPass = pass;
        result.wrappedTimelinePosition = timelineCursor;
        return result;
    }

    bool beginFinalize() noexcept
    {
        return stateMachine.beginFinalize();
    }

    bool finishFinalize (bool valid) noexcept
    {
        configured = false;
        return stateMachine.finishFinalize (valid);
    }

    void abort() noexcept
    {
        configured = false;
        stateMachine.abort();
    }

private:
    static int frameAtBoundary (std::int64_t boundary,
                                std::int64_t blockStart,
                                int deviceFrames,
                                double timelineSamplesPerFrame) noexcept
    {
        if (boundary <= blockStart)
            return 0;
        const auto offset = static_cast<int> (std::ceil (
            static_cast<double> (boundary - blockStart)
            / timelineSamplesPerFrame));
        return std::clamp (offset, 0, deviceFrames);
    }

    PlanSettings settings;
    double timelineRate = 0.0;
    double deviceRate = 0.0;
    int currentPass = 1;
    bool configured = false;
    StateMachine stateMachine;
};
}
