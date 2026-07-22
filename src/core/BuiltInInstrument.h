#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace triumph::instrument
{
inline double midiFrequency (int noteNumber) noexcept
{
    return 440.0 * std::pow (2.0, (std::clamp (noteNumber, 0, 127) - 69) / 12.0);
}

inline float deterministicNoise (std::int64_t sampleIndex,
                                 int salt = 0) noexcept
{
    const auto phase = static_cast<double> (sampleIndex + salt * 131)
                       * 12.9898;
    const auto value = std::sin (phase) * 43758.5453123;
    return static_cast<float> ((value - std::floor (value)) * 2.0 - 1.0);
}

inline float renderSample (int noteNumber, int channel, float velocity,
                           double elapsedSeconds, double remainingSeconds,
                           std::int64_t absoluteSample,
                           double sampleRate) noexcept
{
    if (elapsedSeconds < 0.0 || remainingSeconds <= 0.0 || sampleRate <= 0.0)
        return 0.0f;
    constexpr double twoPi = 6.28318530717958647692;
    velocity = std::clamp (velocity, 0.0f, 1.0f);
    const auto release = std::clamp (remainingSeconds / 0.02, 0.0, 1.0);

    if (channel == 10)
    {
        if (noteNumber == 36)
        {
            const auto phase = twoPi * (45.0 * elapsedSeconds
                + 100.0 * (1.0 - std::exp (-25.0 * elapsedSeconds)) / 25.0);
            return static_cast<float> (std::sin (phase)
                * std::exp (-12.0 * elapsedSeconds) * release * velocity * 0.42);
        }
        if (noteNumber == 38)
        {
            const auto noise = deterministicNoise (absoluteSample, noteNumber);
            const auto body = std::sin (twoPi * 185.0 * elapsedSeconds);
            return static_cast<float> ((noise * 0.78 + body * 0.22)
                * std::exp (-22.0 * elapsedSeconds) * release * velocity * 0.30);
        }
        if (noteNumber == 42 || noteNumber == 46)
        {
            const auto noise = deterministicNoise (absoluteSample, noteNumber)
                - deterministicNoise (absoluteSample - 1, noteNumber);
            const auto decay = noteNumber == 46 ? 13.0 : 45.0;
            return static_cast<float> (noise * std::exp (-decay * elapsedSeconds)
                * release * velocity * 0.13);
        }
    }

    const auto attack = std::clamp (elapsedSeconds / 0.005, 0.0, 1.0);
    const auto envelope = std::min (attack, release);
    return static_cast<float> (std::sin (twoPi * midiFrequency (noteNumber)
                                        * elapsedSeconds)
                               * envelope * velocity * 0.14);
}
}
