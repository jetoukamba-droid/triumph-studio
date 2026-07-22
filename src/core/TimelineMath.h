#pragma once

#include <algorithm>
#include <cmath>

namespace triumph::timeline
{
inline double clampSeconds (double seconds, double duration) noexcept
{
    return std::clamp (seconds, 0.0, std::max (0.0, duration));
}

inline double secondsToPixels (double seconds, double duration, double width) noexcept
{
    if (duration <= 0.0 || width <= 0.0)
        return 0.0;

    return clampSeconds (seconds, duration) / duration * width;
}

inline double pixelsToSeconds (double pixels, double width, double duration) noexcept
{
    if (width <= 0.0 || duration <= 0.0)
        return 0.0;

    return std::clamp (pixels, 0.0, width) / width * duration;
}

inline long long secondsToSamples (double seconds, double sampleRate) noexcept
{
    if (seconds <= 0.0 || sampleRate <= 0.0)
        return 0;

    return static_cast<long long> (std::llround (seconds * sampleRate));
}

inline double samplesToSeconds (long long samples, double sampleRate) noexcept
{
    return sampleRate > 0.0 ? static_cast<double> (std::max (0LL, samples)) / sampleRate
                            : 0.0;
}

inline double samplesToMilliseconds (long long samples, double sampleRate) noexcept
{
    return samplesToSeconds (samples, sampleRate) * 1000.0;
}

inline long long convertSampleCount (long long samples,
                                     double sourceSampleRate,
                                     double destinationSampleRate) noexcept
{
    if (samples <= 0 || sourceSampleRate <= 0.0 || destinationSampleRate <= 0.0)
        return 0;

    return static_cast<long long> (std::llround (
        static_cast<double> (samples) * destinationSampleRate / sourceSampleRate));
}

inline long long compensateInputLatency (long long transportStartSamples,
                                          long long inputLatencySamples,
                                          double deviceSampleRate,
                                          double timelineSampleRate,
                                          long long manualOffsetSamples = 0) noexcept
{
    transportStartSamples = std::max (0LL, transportStartSamples);
    const auto latency = convertSampleCount (inputLatencySamples,
                                              deviceSampleRate,
                                              timelineSampleRate);
    const auto manualOffset = convertSampleCount (
        std::abs (manualOffsetSamples), deviceSampleRate, timelineSampleRate);
    const auto signedOffset = manualOffsetSamples < 0 ? -manualOffset
                                                      : manualOffset;
    return std::max (0LL, transportStartSamples - latency + signedOffset);
}

inline long long snapSamples (long long samples, long long gridSamples) noexcept
{
    samples = std::max (0LL, samples);

    if (gridSamples <= 0)
        return samples;

    return std::max (0LL, static_cast<long long> (std::llround (
                             static_cast<double> (samples) / gridSamples)) * gridSamples);
}

inline bool rangesOverlap (long long firstStart,
                           long long firstLength,
                           long long secondStart,
                           long long secondLength) noexcept
{
    if (firstLength <= 0 || secondLength <= 0)
        return false;

    return firstStart < secondStart + secondLength
           && secondStart < firstStart + firstLength;
}

inline float equalPowerFadeIn (long long position, long long length) noexcept
{
    if (length <= 0)
        return 1.0f;
    const auto progress = std::clamp (static_cast<double> (position)
                                         / static_cast<double> (length),
                                     0.0, 1.0);
    return static_cast<float> (std::sqrt (progress));
}

inline float equalPowerFadeOut (long long position, long long length) noexcept
{
    if (length <= 0)
        return 1.0f;
    const auto progress = std::clamp (static_cast<double> (position)
                                         / static_cast<double> (length),
                                     0.0, 1.0);
    return static_cast<float> (std::sqrt (1.0 - progress));
}

struct StereoBalance
{
    float left = 1.0f;
    float right = 1.0f;
};

inline StereoBalance balanceForPan (float pan) noexcept
{
    pan = std::clamp (pan, -1.0f, 1.0f);

    if (pan < 0.0f)
        return { 1.0f, 1.0f + pan };

    return { 1.0f - pan, 1.0f };
}
}
