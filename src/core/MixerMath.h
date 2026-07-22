#pragma once

#include <algorithm>
#include <cmath>

namespace triumph::mixer
{
inline float sidechainDuckGain (float controlLevel,
                                float depth = 6.0f) noexcept
{
    controlLevel = std::max (0.0f, controlLevel);
    depth = std::max (0.0f, depth);
    return 1.0f / (1.0f + controlLevel * depth);
}

inline int logarithmicSpectrumBin (int displayIndex,
                                   int displaySize,
                                   int fftSize) noexcept
{
    if (displaySize <= 1 || fftSize < 4)
        return 1;
    const auto proportion = std::clamp (
        static_cast<double> (displayIndex)
            / static_cast<double> (displaySize - 1),
        0.0, 1.0);
    return std::clamp (
        static_cast<int> (std::lround (
            std::pow (proportion, 2.15) * (fftSize / 2 - 2))) + 1,
        1, fftSize / 2 - 1);
}
}
