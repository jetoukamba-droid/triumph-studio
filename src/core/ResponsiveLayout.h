#pragma once

namespace triumph::layout
{
inline constexpr int compactToolbarBreakpoint = 1500;
inline constexpr int minimumWindowWidth = 1280;
inline constexpr int minimumWindowHeight = 720;

inline bool useCompactToolbars (int windowWidth) noexcept
{
    return windowWidth < compactToolbarBreakpoint;
}

inline int requiredTopToolbarWidth (bool compact) noexcept
{
    return compact ? 1188 : 1488;
}

inline int requiredTransportToolbarWidth (bool compact) noexcept
{
    return compact ? 1104 : 1332;
}

inline bool fitsTargetWidth (int windowWidth) noexcept
{
    const auto compact = useCompactToolbars (windowWidth);
    return requiredTopToolbarWidth (compact) <= windowWidth - 24
        && requiredTransportToolbarWidth (compact) <= windowWidth - 32;
}
}
