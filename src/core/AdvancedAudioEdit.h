#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace triumph::advanced
{
using AudioChannels = std::vector<std::vector<float>>;

inline double pitchRatioFromSemitones (double semitones) noexcept
{
    return std::pow (2.0, std::clamp (semitones, -24.0, 24.0) / 12.0);
}

inline double targetAmplitudeFromDb (double decibels) noexcept
{
    return std::pow (10.0, decibels / 20.0);
}

inline float normalisationGain (float peak, double targetDb = -1.0,
                                float maximumGain = 16.0f) noexcept
{
    if (! std::isfinite (peak) || peak <= 1.0e-9f)
        return 1.0f;
    const auto target = static_cast<float> (targetAmplitudeFromDb (targetDb));
    return std::clamp (target / peak, 0.0f, std::max (0.0f, maximumGain));
}

inline std::size_t reversedPosition (std::size_t position,
                                     std::size_t length) noexcept
{
    if (length == 0)
        return 0;
    return length - 1 - std::min (position, length - 1);
}

inline AudioChannels pitchShiftOla (const AudioChannels& input,
                                    double semitones)
{
    if (input.empty())
        return {};

    auto sampleCount = input.front().size();
    for (const auto& channel : input)
        sampleCount = std::min (sampleCount, channel.size());
    if (sampleCount == 0)
        return AudioChannels (input.size());
    if (std::abs (semitones) < 0.0001)
    {
        auto copy = input;
        for (auto& channel : copy)
            channel.resize (sampleCount);
        return copy;
    }

    std::size_t windowSize = 2048;
    while (windowSize > sampleCount && windowSize > 64)
        windowSize /= 2;
    windowSize = std::min (windowSize, sampleCount);
    const auto analysisHop = std::max<std::size_t> (1, windowSize / 4);
    const auto ratio = pitchRatioFromSemitones (semitones);
    const auto synthesisHop = std::max<std::size_t> (
        1, static_cast<std::size_t> (std::llround (analysisHop * ratio)));
    const auto frameCount = sampleCount <= windowSize
        ? std::size_t { 1 }
        : (sampleCount - windowSize + analysisHop - 1) / analysisHop + 1;
    const auto stretchedCount = (frameCount - 1) * synthesisHop + windowSize;

    AudioChannels stretched (input.size(),
                              std::vector<float> (stretchedCount, 0.0f));
    std::vector<float> weights (stretchedCount, 0.0f);
    constexpr double twoPi = 6.28318530717958647692;

    for (std::size_t frame = 0; frame < frameCount; ++frame)
    {
        const auto inputStart = std::min (frame * analysisHop,
                                          sampleCount - windowSize);
        const auto outputStart = frame * synthesisHop;
        for (std::size_t sample = 0; sample < windowSize; ++sample)
        {
            const auto window = windowSize > 1
                ? static_cast<float> (0.5 - 0.5 * std::cos (
                    twoPi * static_cast<double> (sample)
                    / static_cast<double> (windowSize - 1)))
                : 1.0f;
            weights[outputStart + sample] += window;
            for (std::size_t channel = 0; channel < input.size(); ++channel)
                stretched[channel][outputStart + sample]
                    += input[channel][inputStart + sample] * window;
        }
    }

    for (std::size_t sample = 0; sample < stretchedCount; ++sample)
        if (weights[sample] > 1.0e-6f)
            for (auto& channel : stretched)
                channel[sample] /= weights[sample];

    AudioChannels result (input.size(), std::vector<float> (sampleCount, 0.0f));
    if (sampleCount == 1 || stretchedCount == 1)
    {
        for (std::size_t channel = 0; channel < input.size(); ++channel)
            result[channel][0] = stretched[channel][0];
        return result;
    }

    const auto scale = static_cast<double> (stretchedCount - 1)
                       / static_cast<double> (sampleCount - 1);
    for (std::size_t sample = 0; sample < sampleCount; ++sample)
    {
        const auto position = static_cast<double> (sample) * scale;
        const auto index = std::min (
            static_cast<std::size_t> (position), stretchedCount - 1);
        const auto next = std::min (index + 1, stretchedCount - 1);
        const auto fraction = static_cast<float> (
            position - static_cast<double> (index));
        for (std::size_t channel = 0; channel < input.size(); ++channel)
            result[channel][sample] = stretched[channel][index]
                + (stretched[channel][next] - stretched[channel][index]) * fraction;
    }
    return result;
}
}
