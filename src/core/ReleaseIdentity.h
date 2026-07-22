#pragma once

#include <string_view>

namespace triumph::release
{
inline constexpr char applicationName[] = "Triumph Studio";
inline constexpr char versionLabel[] = "0.22.0-beta.1";
inline constexpr char releaseChannel[] = "Closed Beta 1";
inline constexpr char releaseSummary[] =
    "First closed beta - professional controls and reliability checkpoint";
inline constexpr bool isPrerelease = true;

constexpr bool hasBetaVersion() noexcept
{
    return std::string_view (versionLabel).find ("-beta.")
           != std::string_view::npos;
}

static_assert (isPrerelease);
static_assert (hasBetaVersion());
}
