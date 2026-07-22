#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace triumph::output
{
enum class Availability
{
    noDevice,
    noActiveChannels,
    invalidFormat,
    ready
};

struct DeviceSnapshot
{
    bool devicePresent = false;
    int activeOutputChannels = 0;
    double sampleRate = 0.0;
    int bufferSizeSamples = 0;
};

inline Availability assess (const DeviceSnapshot& snapshot) noexcept
{
    if (! snapshot.devicePresent)
        return Availability::noDevice;
    if (snapshot.activeOutputChannels <= 0)
        return Availability::noActiveChannels;
    if (! std::isfinite (snapshot.sampleRate) || snapshot.sampleRate <= 0.0
        || snapshot.bufferSizeSamples <= 0)
        return Availability::invalidFormat;
    return Availability::ready;
}

inline float testToneEnvelope (int sampleIndex,
                               int totalSamples,
                               int fadeSamples) noexcept
{
    if (sampleIndex < 0 || sampleIndex >= totalSamples || totalSamples <= 0)
        return 0.0f;
    fadeSamples = std::clamp (fadeSamples, 1, std::max (1, totalSamples / 2));
    if (sampleIndex < fadeSamples)
        return static_cast<float> (sampleIndex) / static_cast<float> (fadeSamples);
    const auto samplesFromEnd = totalSamples - 1 - sampleIndex;
    if (samplesFromEnd < fadeSamples)
        return static_cast<float> (samplesFromEnd)
               / static_cast<float> (fadeSamples);
    return 1.0f;
}

inline bool callbacksStalled (bool outputExpected,
                              std::uint64_t currentCount,
                              std::uint64_t previousCount,
                              int stagnantTimerTicks,
                              int warningTicks) noexcept
{
    return outputExpected && currentCount == previousCount
           && stagnantTimerTicks >= std::max (1, warningTicks);
}
}
