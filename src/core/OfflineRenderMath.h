#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace triumph::offline
{
inline std::int64_t renderLengthSamples (std::int64_t projectSamples,
                                         double timelineSampleRate,
                                         double outputSampleRate,
                                         double tailSeconds) noexcept
{
    if (projectSamples <= 0 || timelineSampleRate <= 0.0
        || outputSampleRate <= 0.0)
        return 0;

    const auto material = static_cast<std::int64_t> (std::llround (
        static_cast<double> (projectSamples) * outputSampleRate
        / timelineSampleRate));
    const auto tail = static_cast<std::int64_t> (std::llround (
        std::max (0.0, tailSeconds) * outputSampleRate));
    return std::max<std::int64_t> (0, material)
           + std::max<std::int64_t> (0, tail);
}

class DeterministicDither
{
public:
    explicit DeterministicDither (std::uint32_t seed = 0x54524955u) : state (seed) {}

    float nextTpdf (int bitsPerSample) noexcept
    {
        if (bitsPerSample != 16 && bitsPerSample != 24)
            return 0.0f;
        const auto scale = std::ldexp (1.0f, -bitsPerSample + 1);
        return (uniform() - uniform()) * scale;
    }

    float next24BitTpdf() noexcept
    {
        return nextTpdf (24);
    }

private:
    float uniform() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float> (state & 0x00ffffffu)
               / static_cast<float> (0x01000000u);
    }

    std::uint32_t state;
};

inline float linearSample (float a, float b, double fraction) noexcept
{
    return a + (b - a) * static_cast<float> (std::clamp (fraction, 0.0, 1.0));
}
}
