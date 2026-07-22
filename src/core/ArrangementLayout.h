#pragma once

#include <algorithm>

namespace triumph::arrangement
{
inline constexpr int rowContentInsetX = 8;
inline constexpr int rowVerticalInset = 8;
inline constexpr int trackHeaderWidth = 166;
inline constexpr int eventDisplayInset = 8;
inline constexpr int preferredTrackHeight = 64;
inline constexpr int maximumTrackHeight = 220;

inline constexpr int eventDisplayOriginX() noexcept
{
    return trackHeaderWidth + eventDisplayInset;
}

inline constexpr int rulerOriginX() noexcept
{
    return rowContentInsetX + eventDisplayOriginX();
}

inline int clampTrackHeight (int requestedHeight) noexcept
{
    return std::clamp (requestedHeight, preferredTrackHeight,
                       maximumTrackHeight);
}

inline constexpr bool rulerUsesPanelLabel() noexcept
{
    return false;
}

inline constexpr bool newInstrumentTrackCreatesClip() noexcept
{
    return false;
}
}
