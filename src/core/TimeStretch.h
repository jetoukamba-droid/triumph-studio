#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace triumph::stretch
{
inline constexpr const char* builtInProviderId = "builtin-resample";
inline constexpr double minimumRatio = 0.25;
inline constexpr double maximumRatio = 4.0;

inline double clampRatio (double ratio) noexcept
{
    return std::clamp (std::isfinite (ratio) ? ratio : 1.0,
                       minimumRatio,
                       maximumRatio);
}

// Ratio is timeline duration / native duration. The built-in provider is
// deliberately varispeed: speed and pitch remain linked.
inline double playbackRate (double ratio) noexcept
{
    return 1.0 / clampRatio (ratio);
}

inline std::int64_t timelineToNativeSamples (std::int64_t timelineSamples,
                                             double ratio) noexcept
{
    return static_cast<std::int64_t> (std::llround (
        static_cast<double> (timelineSamples) * playbackRate (ratio)));
}

inline std::int64_t nativeToTimelineSamples (std::int64_t nativeSamples,
                                             double ratio) noexcept
{
    return static_cast<std::int64_t> (std::llround (
        static_cast<double> (nativeSamples) * clampRatio (ratio)));
}
}
