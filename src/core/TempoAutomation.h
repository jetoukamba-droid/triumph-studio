#pragma once

#include "TempoMap.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace triumph::automation
{
enum class Mode
{
    read,
    touch,
    latch,
    write
};

enum class Curve
{
    hold,
    linear,
    smooth
};

enum class PluginParameterDelivery
{
    sampleOffsetSubBlocks,
    nativeVst3ParameterQueueRamp
};

inline const char* pluginParameterDeliveryName (
    PluginParameterDelivery delivery) noexcept
{
    switch (delivery)
    {
        case PluginParameterDelivery::sampleOffsetSubBlocks:
            return "sample-offset sub-block slicing";
        case PluginParameterDelivery::nativeVst3ParameterQueueRamp:
            return "native VST3 parameter queue/ramp plan";
    }
    return "sample-offset sub-block slicing";
}

struct Point
{
    std::int64_t sample = 0;
    float value = 0.0f;
    Curve curve = Curve::linear;
};

inline std::vector<Point> normalised (std::vector<Point> points)
{
    for (auto& point : points)
        point.sample = std::max<std::int64_t> (0, point.sample);
    std::stable_sort (points.begin(), points.end(), [] (const auto& a, const auto& b)
    {
        return a.sample < b.sample;
    });
    std::vector<Point> result;
    for (const auto& point : points)
    {
        if (! result.empty() && result.back().sample == point.sample)
            result.back() = point;
        else
            result.push_back (point);
    }
    return result;
}

inline float valueAtSample (std::int64_t sample,
                            const std::vector<Point>& points,
                            float fallback = 0.0f) noexcept
{
    if (points.empty())
        return fallback;
    if (sample <= points.front().sample)
        return points.front().value;

    const auto upper = std::upper_bound (
        points.begin(), points.end(), sample, [] (std::int64_t position,
                                                  const auto& point)
    {
        return position < point.sample;
    });
    if (upper == points.end())
        return points.back().value;
    const auto& right = *upper;
    const auto& left = *(upper - 1);
    if (left.curve == Curve::hold || right.sample <= left.sample)
        return left.value;
    auto amount = static_cast<float> (sample - left.sample)
                  / static_cast<float> (right.sample - left.sample);
    amount = std::clamp (amount, 0.0f, 1.0f);
    if (left.curve == Curve::smooth)
        amount = amount * amount * (3.0f - 2.0f * amount);
    return left.value + (right.value - left.value) * amount;
}

inline Curve curveAtSample (std::int64_t sample,
                            const std::vector<Point>& points) noexcept
{
    if (points.empty() || sample < points.front().sample)
        return Curve::hold;

    const auto upper = std::upper_bound (
        points.begin(), points.end(), sample, [] (std::int64_t position,
                                                  const auto& point)
    {
        return position < point.sample;
    });
    if (upper == points.begin())
        return points.front().curve;
    return (upper - 1)->curve;
}

struct NativeParameterQueuePoint
{
    std::uint32_t sampleOffset = 0;
    float value = 0.0f;
    Curve curveToNext = Curve::hold;
};

inline bool isRampCurve (Curve curve) noexcept
{
    return curve == Curve::linear || curve == Curve::smooth;
}

inline std::vector<NativeParameterQueuePoint> nativeParameterQueueForBlock (
    std::int64_t blockStartSample, int blockSize,
    const std::vector<Point>& points, float currentValue,
    float lastDeliveredValue)
{
    std::vector<NativeParameterQueuePoint> queue;
    if (blockSize <= 0 || points.empty())
        return queue;

    const auto blockEndSample = blockStartSample
        + static_cast<std::int64_t> (blockSize);
    const auto clampedValueAt = [&] (std::int64_t sample)
    {
        return std::clamp (valueAtSample (sample, points, currentValue),
                           0.0f, 1.0f);
    };
    const auto append = [&] (std::uint32_t offset, float value, Curve curve)
    {
        value = std::clamp (value, 0.0f, 1.0f);
        if (! queue.empty() && queue.back().sampleOffset == offset)
        {
            queue.back().value = value;
            queue.back().curveToNext = curve;
            return;
        }
        queue.push_back ({ offset, value, curve });
    };

    const auto startValue = clampedValueAt (blockStartSample);
    if (lastDeliveredValue < 0.0f
        || std::abs (startValue - lastDeliveredValue) > 0.000001f)
        append (0, startValue, curveAtSample (blockStartSample, points));

    for (const auto& point : points)
    {
        if (point.sample <= blockStartSample || point.sample >= blockEndSample)
            continue;
        append (static_cast<std::uint32_t> (point.sample - blockStartSample),
                point.value, point.curve);
    }

    if (! queue.empty() && queue.front().sampleOffset > 0)
    {
        const auto firstSample = blockStartSample
            + static_cast<std::int64_t> (queue.front().sampleOffset);
        const auto startCurve = curveAtSample (blockStartSample, points);
        if (isRampCurve (startCurve)
            && std::abs (startValue - clampedValueAt (firstSample))
                   > 0.000001f)
            queue.insert (queue.begin(), { 0, startValue, startCurve });
    }

    const auto finalSample = blockEndSample - 1;
    const auto finalValue = clampedValueAt (finalSample);
    const auto finalCurve = curveAtSample (finalSample, points);
    if (queue.empty() && isRampCurve (curveAtSample (blockStartSample, points))
        && std::abs (startValue - finalValue) > 0.000001f)
        append (0, startValue, curveAtSample (blockStartSample, points));
    if (isRampCurve (finalCurve)
        && (queue.empty()
            || queue.back().sampleOffset
                   != static_cast<std::uint32_t> (blockSize - 1))
        && (queue.empty()
            || std::abs (queue.back().value - finalValue) > 0.000001f))
        append (static_cast<std::uint32_t> (blockSize - 1), finalValue,
                Curve::hold);

    if (! queue.empty())
        queue.back().curveToNext = Curve::hold;
    return queue;
}

inline bool nativeParameterQueueHasRamp (
    const std::vector<NativeParameterQueuePoint>& queue) noexcept
{
    for (std::size_t index = 0; index + 1 < queue.size(); ++index)
        if (isRampCurve (queue[index].curveToNext)
            && std::abs (queue[index].value - queue[index + 1].value)
                   > 0.000001f)
            return true;
    return false;
}

template <typename Visitor>
inline bool visitParameterEventsForBlock (std::int64_t blockStartSample,
                                          int blockSize,
                                          const std::vector<Point>& points,
                                          float currentValue,
                                          float lastDeliveredValue,
                                          Visitor&& visitor)
{
    if (blockSize <= 0 || points.empty())
        return true;

    const auto blockEndSample = blockStartSample
        + static_cast<std::int64_t> (blockSize);
    const auto startValue = std::clamp (
        valueAtSample (blockStartSample, points, currentValue),
        0.0f, 1.0f);
    if (lastDeliveredValue < 0.0f
        || std::abs (startValue - lastDeliveredValue) > 0.000001f)
        if (! visitor (static_cast<std::uint32_t> (0), startValue))
            return false;

    for (const auto& point : points)
    {
        if (point.sample <= blockStartSample || point.sample >= blockEndSample)
            continue;
        const auto offset = static_cast<std::uint32_t> (
            point.sample - blockStartSample);
        if (! visitor (offset, std::clamp (point.value, 0.0f, 1.0f)))
            return false;
    }
    return true;
}

template <typename OffsetProvider, typename Visitor>
inline bool visitParameterAutomationSlices (int blockSize,
                                            std::size_t eventCount,
                                            OffsetProvider&& offsetAt,
                                            Visitor&& visitor)
{
    if (blockSize <= 0)
        return true;

    auto cursor = 0;
    auto nextEvent = static_cast<std::size_t> (0);
    while (cursor < blockSize)
    {
        while (nextEvent < eventCount
               && static_cast<int> (offsetAt (nextEvent)) <= cursor)
            ++nextEvent;

        auto nextOffset = blockSize;
        if (nextEvent < eventCount)
            nextOffset = std::clamp (static_cast<int> (offsetAt (nextEvent)),
                                     0, blockSize);
        nextOffset = std::clamp (nextOffset, cursor + 1, blockSize);

        if (! visitor (cursor, nextOffset - cursor))
            return false;
        cursor = nextOffset;
    }
    return true;
}

inline bool shouldWrite (Mode mode, bool gestureActive,
                         bool latchEngaged) noexcept
{
    switch (mode)
    {
        case Mode::read:  return false;
        case Mode::touch: return gestureActive;
        case Mode::latch: return gestureActive || latchEngaged;
        case Mode::write: return true;
    }
    return false;
}

struct Signature
{
    double beat = 0.0;
    int numerator = 4;
    int denominator = 4;
};

inline std::vector<Signature> normalisedSignatures (std::vector<Signature> points)
{
    if (points.empty())
        points.push_back ({ 0.0, 4, 4 });
    for (auto& point : points)
    {
        point.beat = std::max (0.0, point.beat);
        point.numerator = std::clamp (point.numerator, 1, 32);
        const int supported[] { 1, 2, 4, 8, 16, 32 };
        auto closest = supported[0];
        for (const auto candidate : supported)
            if (std::abs (candidate - point.denominator)
                < std::abs (closest - point.denominator))
                closest = candidate;
        point.denominator = closest;
    }
    std::stable_sort (points.begin(), points.end(), [] (const auto& a, const auto& b)
    {
        return a.beat < b.beat;
    });
    std::vector<Signature> result;
    for (const auto& point : points)
    {
        if (! result.empty() && std::abs (result.back().beat - point.beat) < 1.0e-9)
            result.back() = point;
        else
            result.push_back (point);
    }
    if (result.front().beat > 0.0)
        result.insert (result.begin(), { 0.0, 4, 4 });
    return result;
}

inline Signature signatureAtBeat (double beat,
                                  const std::vector<Signature>& source)
{
    const auto points = normalisedSignatures (source);
    for (std::size_t index = points.size(); index-- > 0;)
        if (beat >= points[index].beat)
            return points[index];
    return points.front();
}

inline double beatUnitLength (const Signature& signature) noexcept
{
    return 4.0 / static_cast<double> (std::max (1, signature.denominator));
}

inline bool isBarAccent (double beat, const Signature& signature) noexcept
{
    const auto barLength = signature.numerator * beatUnitLength (signature);
    if (barLength <= 0.0)
        return false;
    const auto relative = std::max (0.0, beat - signature.beat);
    return std::abs (relative - std::round (relative / barLength) * barLength)
           < 1.0e-7;
}

struct MetronomeEvent
{
    int sampleOffset = 0;
    bool accent = false;
    double beat = 0.0;
};

inline std::vector<MetronomeEvent> metronomeEventsForBlock (
    std::int64_t blockStartSample, int blockSize, double sampleRate,
    const std::vector<tempo::Point>& tempoPoints,
    const std::vector<Signature>& signatures)
{
    std::vector<MetronomeEvent> events;
    if (blockSize <= 0 || sampleRate <= 0.0)
        return events;
    const auto startSeconds = static_cast<double> (blockStartSample) / sampleRate;
    const auto endSeconds = static_cast<double> (blockStartSample + blockSize)
                            / sampleRate;
    const auto startBeat = tempo::secondsToBeat (startSeconds, tempoPoints);
    const auto endBeat = tempo::secondsToBeat (endSeconds, tempoPoints);
    const auto normalSignatures = normalisedSignatures (signatures);

    auto cursor = std::max (0.0, std::floor (startBeat * 8.0) / 8.0);
    for (int guard = 0; guard < blockSize * 2 + 128 && cursor <= endBeat + 1.0; ++guard)
    {
        const auto signature = signatureAtBeat (cursor, normalSignatures);
        const auto unit = beatUnitLength (signature);
        const auto relative = std::max (0.0, cursor - signature.beat);
        const auto tick = signature.beat + std::ceil ((relative - 1.0e-9) / unit) * unit;
        const auto eventSample = tempo::beatToSamples (tick, sampleRate, tempoPoints);
        const auto offset = tempo::sampleOffsetInBlock (eventSample,
                                                       blockStartSample,
                                                       blockSize);
        if (offset >= 0)
            events.push_back ({ offset, isBarAccent (tick, signature), tick });
        cursor = tick + unit;
    }
    std::sort (events.begin(), events.end(), [] (const auto& a, const auto& b)
    {
        return a.sampleOffset < b.sampleOffset;
    });
    events.erase (std::unique (events.begin(), events.end(), [] (const auto& a,
                                                                 const auto& b)
    {
        return a.sampleOffset == b.sampleOffset;
    }), events.end());
    return events;
}
}
