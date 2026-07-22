#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace triumph::plugin
{
enum class TailAction
{
    noteInput,
    transportStop,
    bypass,
    removal,
    freeze,
    offlineRender,
    fault
};

struct TailPolicy
{
    bool renderTail = false;
    double seconds = 0.0;
    double fadeMilliseconds = 0.0;
};

constexpr double minimumNaturalTailSeconds = 1.0;
constexpr double maximumNaturalTailSeconds = 30.0;
constexpr double safeTransitionFadeMilliseconds = 10.0;

inline TailPolicy tailPolicy (TailAction action,
                              double reportedSeconds) noexcept
{
    const auto naturalSeconds = std::clamp (
        std::isfinite (reportedSeconds) ? reportedSeconds : 0.0,
        minimumNaturalTailSeconds, maximumNaturalTailSeconds);
    switch (action)
    {
        case TailAction::noteInput:
        case TailAction::transportStop:
        case TailAction::freeze:
        case TailAction::offlineRender:
            return { true, naturalSeconds, 0.0 };
        case TailAction::bypass:
        case TailAction::removal:
            return { false, 0.0, safeTransitionFadeMilliseconds };
        case TailAction::fault:
            return { false, 0.0, 0.0 };
    }
    return {};
}

inline std::int64_t tailSamples (TailAction action,
                                 double reportedSeconds,
                                 double sampleRate) noexcept
{
    const auto policy = tailPolicy (action, reportedSeconds);
    if (! policy.renderTail || ! std::isfinite (sampleRate)
        || sampleRate <= 0.0)
        return 0;
    return static_cast<std::int64_t> (std::ceil (
        policy.seconds * sampleRate));
}

struct BusPolicy
{
    bool accepted = false;
    int renderChannels = 0;
};

inline BusPolicy instrumentBusPolicy (bool isInstrument,
                                      int mainOutputChannels) noexcept
{
    if (! isInstrument || mainOutputChannels <= 0)
        return {};
    return { true, std::min (2, mainOutputChannels) };
}

inline std::uint64_t parameterNameHash (std::string_view text) noexcept
{
    auto value = UINT64_C (14695981039346656037);
    for (const auto character : text)
    {
        value ^= static_cast<unsigned char> (character);
        value *= UINT64_C (1099511628211);
    }
    return value;
}

inline std::string stableParameterId (std::string_view providedId,
                                      int parameterIndex,
                                      std::string_view parameterName)
{
    if (! providedId.empty())
        return "plugin:" + std::string (providedId);

    std::ostringstream stream;
    stream << "plugin:legacy:" << std::max (0, parameterIndex) << ':'
           << std::hex << std::setw (16) << std::setfill ('0')
           << parameterNameHash (parameterName);
    return stream.str();
}
}
