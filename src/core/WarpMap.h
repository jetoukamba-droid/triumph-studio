#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace triumph::warp
{
struct Anchor
{
    std::int64_t timeline = 0;
    std::int64_t source = 0;
};

inline std::vector<Anchor> normalise (std::vector<Anchor> anchors)
{
    std::sort (anchors.begin(), anchors.end(), [] (const auto& a, const auto& b)
    {
        return a.timeline < b.timeline;
    });
    anchors.erase (std::unique (anchors.begin(), anchors.end(), [] (const auto& a,
                                                                    const auto& b)
    {
        return a.timeline == b.timeline;
    }), anchors.end());
    return anchors;
}

inline std::int64_t sourceAt (const std::vector<Anchor>& ordered,
                              std::int64_t timeline) noexcept
{
    if (ordered.empty())
        return 0;
    if (timeline <= ordered.front().timeline)
        return ordered.front().source;
    if (timeline >= ordered.back().timeline)
        return ordered.back().source;

    const auto right = std::upper_bound (ordered.begin(), ordered.end(), timeline,
                                         [] (auto value, const auto& anchor)
    {
        return value < anchor.timeline;
    });
    const auto& b = *right;
    const auto& a = *(right - 1);
    const auto proportion = static_cast<double> (timeline - a.timeline)
                            / static_cast<double> (b.timeline - a.timeline);
    return static_cast<std::int64_t> (std::llround (
        a.source + proportion * static_cast<double> (b.source - a.source)));
}

inline std::int64_t timelineAt (const std::vector<Anchor>& ordered,
                                std::int64_t source) noexcept
{
    if (ordered.empty())
        return 0;
    if (source <= ordered.front().source)
        return ordered.front().timeline;
    if (source >= ordered.back().source)
        return ordered.back().timeline;
    for (std::size_t index = 1; index < ordered.size(); ++index)
    {
        const auto& a = ordered[index - 1];
        const auto& b = ordered[index];
        if (source > b.source || b.source <= a.source)
            continue;
        const auto proportion = static_cast<double> (source - a.source)
                                / static_cast<double> (b.source - a.source);
        return static_cast<std::int64_t> (std::llround (
            a.timeline + proportion * static_cast<double> (b.timeline - a.timeline)));
    }
    return ordered.back().timeline;
}

inline double playbackRate (const std::vector<Anchor>& ordered,
                            std::int64_t start,
                            std::int64_t end,
                            double timelineRate,
                            double sourceRate) noexcept
{
    if (end <= start || timelineRate <= 0.0 || sourceRate <= 0.0)
        return 1.0;
    const auto sourceDelta = sourceAt (ordered, end) - sourceAt (ordered, start);
    return std::clamp ((static_cast<double> (sourceDelta) / sourceRate)
                           / (static_cast<double> (end - start) / timelineRate),
                       0.25, 4.0);
}
}
