#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace triumph::transport
{
using ClockUnits = std::int64_t;

// Q20 fixed-point project samples. The integer representation is authoritative;
// seconds are derived only for UI, plug-in playheads, and tempo conversion.
inline constexpr ClockUnits unitsPerSample = ClockUnits { 1 } << 20;

inline ClockUnits saturatingRound (long double value) noexcept
{
    const auto maximum = static_cast<long double> (
        std::numeric_limits<ClockUnits>::max());
    const auto minimum = static_cast<long double> (
        std::numeric_limits<ClockUnits>::min());
    if (value >= maximum)
        return std::numeric_limits<ClockUnits>::max();
    if (value <= minimum)
        return std::numeric_limits<ClockUnits>::min();
    return static_cast<ClockUnits> (std::llround (value));
}

inline ClockUnits fromProjectSamples (std::int64_t samples) noexcept
{
    if (samples <= 0)
        return 0;
    return saturatingRound (static_cast<long double> (samples)
                            * static_cast<long double> (unitsPerSample));
}

inline ClockUnits fromSeconds (double seconds,
                               double projectSampleRate) noexcept
{
    if (seconds <= 0.0 || projectSampleRate <= 0.0)
        return 0;
    return saturatingRound (static_cast<long double> (seconds)
                            * static_cast<long double> (projectSampleRate)
                            * static_cast<long double> (unitsPerSample));
}

inline std::int64_t toProjectSamples (ClockUnits units) noexcept
{
    return units <= 0 ? 0 : units / unitsPerSample;
}

inline double toSeconds (ClockUnits units,
                         double projectSampleRate) noexcept
{
    if (units <= 0 || projectSampleRate <= 0.0)
        return 0.0;
    return static_cast<double> (units)
           / (static_cast<double> (unitsPerSample) * projectSampleRate);
}

inline ClockUnits advanceForDeviceFrames (int deviceFrames,
                                          double projectSampleRate,
                                          double deviceSampleRate) noexcept
{
    if (deviceFrames <= 0 || projectSampleRate <= 0.0
        || deviceSampleRate <= 0.0)
        return 0;
    return saturatingRound (static_cast<long double> (deviceFrames)
                            * static_cast<long double> (projectSampleRate)
                            * static_cast<long double> (unitsPerSample)
                            / static_cast<long double> (deviceSampleRate));
}

inline ClockUnits clamp (ClockUnits value, ClockUnits maximum) noexcept
{
    return std::clamp (value, ClockUnits { 0 }, std::max (ClockUnits { 0 }, maximum));
}
}
