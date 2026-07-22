#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace triumph::automation_write
{
enum class Mode
{
    read,
    touch,
    latch,
    write
};

struct Point
{
    std::int64_t sample = 0;
    float value = 0.0f;
};

inline std::vector<Point> simplify (const std::vector<Point>& source,
                                    float tolerance)
{
    if (source.size() <= 2)
        return source;
    tolerance = std::max (0.0f, tolerance);
    std::vector<bool> keep (source.size(), false);
    keep.front() = true;
    keep.back() = true;
    struct Range { std::size_t first = 0; std::size_t last = 0; };
    std::vector<Range> stack { { 0, source.size() - 1 } };
    while (! stack.empty())
    {
        const auto range = stack.back();
        stack.pop_back();
        const auto sampleSpan = static_cast<double> (
            source[range.last].sample - source[range.first].sample);
        auto maximumError = 0.0f;
        auto maximumIndex = range.first;
        for (auto index = range.first + 1; index < range.last; ++index)
        {
            const auto amount = sampleSpan <= 0.0 ? 0.0
                : static_cast<double> (
                    source[index].sample - source[range.first].sample)
                    / sampleSpan;
            const auto expected = source[range.first].value
                + static_cast<float> (amount)
                    * (source[range.last].value
                       - source[range.first].value);
            const auto error = std::abs (source[index].value - expected);
            if (error > maximumError)
            {
                maximumError = error;
                maximumIndex = index;
            }
        }
        if (maximumError > tolerance && maximumIndex > range.first)
        {
            keep[maximumIndex] = true;
            stack.push_back ({ range.first, maximumIndex });
            stack.push_back ({ maximumIndex, range.last });
        }
    }
    std::vector<Point> result;
    result.reserve (source.size());
    for (std::size_t index = 0; index < source.size(); ++index)
        if (keep[index])
            result.push_back (source[index]);
    return result;
}

class Pass final
{
public:
    Pass (Mode writeMode, std::int64_t startSample,
          std::int64_t minimumSpacingSamples = 32) noexcept
        : mode (writeMode), start (std::max<std::int64_t> (0, startSample)),
          spacing (std::max<std::int64_t> (1, minimumSpacingSamples))
    {
    }

    void beginGesture (std::int64_t sample, float value)
    {
        touching = true;
        append (sample, value, true);
    }

    void update (std::int64_t sample, float value)
    {
        if (mode == Mode::read || (mode == Mode::touch && ! touching))
            return;
        if (mode == Mode::latch && ! touching && ! latched)
            return;
        append (sample, value, false);
    }

    void endGesture (std::int64_t sample, float value)
    {
        if (! touching)
            return;
        append (sample, value, true);
        touching = false;
        latched = mode == Mode::latch;
    }

    std::vector<Point> finish (std::int64_t endSample,
                               float tolerance = 0.0005f)
    {
        if (mode == Mode::latch && latched && ! points.empty())
            append (endSample, points.back().value, true);
        if (mode == Mode::write && points.empty())
            points.push_back ({ start, 0.0f });
        std::stable_sort (points.begin(), points.end(), [] (const auto& a,
                                                            const auto& b)
        {
            return a.sample < b.sample;
        });
        return simplify (points, tolerance);
    }

private:
    void append (std::int64_t sample, float value, bool force)
    {
        if (mode == Mode::read)
            return;
        sample = std::max (start, sample);
        value = std::clamp (value, 0.0f, 1.0f);
        if (! force && ! points.empty()
            && sample - points.back().sample < spacing)
        {
            points.back() = { sample, value };
            return;
        }
        if (! points.empty() && points.back().sample == sample)
            points.back().value = value;
        else
            points.push_back ({ sample, value });
    }

    Mode mode = Mode::read;
    std::int64_t start = 0;
    std::int64_t spacing = 32;
    bool touching = false;
    bool latched = false;
    std::vector<Point> points;
};
}
