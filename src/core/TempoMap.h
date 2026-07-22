#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace triumph::tempo
{
enum class SegmentCurve
{
    step,
    linear
};

struct Point
{
    double beat = 0.0;
    double bpm = 120.0;
    SegmentCurve curve = SegmentCurve::step;
};

inline std::vector<Point> normalised (std::vector<Point> points)
{
    if (points.empty())
        points.push_back ({ 0.0, 120.0, SegmentCurve::step });

    for (auto& point : points)
    {
        point.beat = std::max (0.0, point.beat);
        point.bpm = std::clamp (point.bpm, 20.0, 400.0);
    }

    std::stable_sort (points.begin(), points.end(), [] (const auto& a, const auto& b)
    {
        return a.beat < b.beat;
    });

    std::vector<Point> result;
    for (const auto& point : points)
    {
        if (! result.empty() && std::abs (result.back().beat - point.beat) < 1.0e-9)
            result.back() = point;
        else
            result.push_back (point);
    }

    if (result.front().beat > 0.0)
        result.insert (result.begin(), { 0.0, result.front().bpm,
                                        SegmentCurve::step });

    return result;
}

inline double segmentSeconds (double beatLength, double startBpm,
                              double endBpm, SegmentCurve curve) noexcept
{
    beatLength = std::max (0.0, beatLength);
    startBpm = std::clamp (startBpm, 20.0, 400.0);
    endBpm = std::clamp (endBpm, 20.0, 400.0);
    if (curve == SegmentCurve::step || beatLength <= 0.0
        || std::abs (endBpm - startBpm) < 1.0e-9)
        return beatLength * 60.0 / startBpm;

    const auto slope = (endBpm - startBpm) / beatLength;
    return 60.0 * std::log (endBpm / startBpm) / slope;
}

inline double secondsIntoSegmentToBeat (double seconds, double segmentBeats,
                                        double startBpm, double endBpm,
                                        SegmentCurve curve) noexcept
{
    seconds = std::max (0.0, seconds);
    if (curve == SegmentCurve::step || segmentBeats <= 0.0
        || std::abs (endBpm - startBpm) < 1.0e-9)
        return seconds * startBpm / 60.0;

    const auto slope = (endBpm - startBpm) / segmentBeats;
    return startBpm * (std::exp (seconds * slope / 60.0) - 1.0) / slope;
}

inline double beatToSecondsPrepared (double beat,
                                     const std::vector<Point>& points) noexcept
{
    beat = std::max (0.0, beat);
    if (points.empty())
        return beat * 0.5;
    double seconds = 0.0;

    for (size_t index = 0; index < points.size(); ++index)
    {
        const auto segmentStart = points[index].beat;
        const auto segmentEnd = index + 1 < points.size() ? points[index + 1].beat : beat;
        if (beat <= segmentStart)
            break;

        const auto beatsInSegment = std::max (0.0, std::min (beat, segmentEnd)
                                                    - segmentStart);
        const auto endBpm = index + 1 < points.size() ? points[index + 1].bpm
                                                       : points[index].bpm;
        const auto totalBeats = std::max (0.0, segmentEnd - segmentStart);
        auto partialEndBpm = points[index].bpm;
        if (points[index].curve == SegmentCurve::linear && totalBeats > 0.0)
            partialEndBpm += (endBpm - points[index].bpm)
                             * (beatsInSegment / totalBeats);
        seconds += segmentSeconds (beatsInSegment, points[index].bpm,
                                   partialEndBpm, points[index].curve);
        if (beat <= segmentEnd)
            break;
    }

    return seconds;
}

inline double beatToSeconds (double beat, const std::vector<Point>& sourcePoints)
{
    const auto points = normalised (sourcePoints);
    return beatToSecondsPrepared (beat, points);
}

inline double secondsToBeatPrepared (double seconds,
                                     const std::vector<Point>& points) noexcept
{
    seconds = std::max (0.0, seconds);
    if (points.empty())
        return seconds * 2.0;
    double elapsed = 0.0;

    for (size_t index = 0; index < points.size(); ++index)
    {
        if (index + 1 < points.size())
        {
            const auto segmentBeats = points[index + 1].beat - points[index].beat;
            const auto duration = segmentSeconds (segmentBeats, points[index].bpm,
                                                  points[index + 1].bpm,
                                                  points[index].curve);
            if (seconds <= elapsed + duration)
                return points[index].beat + secondsIntoSegmentToBeat (
                    seconds - elapsed, segmentBeats, points[index].bpm,
                    points[index + 1].bpm, points[index].curve);
            elapsed += duration;
        }
        else
        {
            return points[index].beat
                   + (seconds - elapsed) * points[index].bpm / 60.0;
        }
    }

    return 0.0;
}

inline double secondsToBeat (double seconds, const std::vector<Point>& sourcePoints)
{
    const auto points = normalised (sourcePoints);
    return secondsToBeatPrepared (seconds, points);
}

inline double bpmAtBeatPrepared (double beat,
                                 const std::vector<Point>& points) noexcept
{
    beat = std::max (0.0, beat);
    for (std::size_t index = points.size(); index-- > 0;)
    {
        if (beat < points[index].beat)
            continue;
        if (index + 1 < points.size()
            && points[index].curve == SegmentCurve::linear)
        {
            const auto length = points[index + 1].beat - points[index].beat;
            if (length > 0.0)
            {
                const auto amount = std::clamp ((beat - points[index].beat) / length,
                                                0.0, 1.0);
                return points[index].bpm
                       + (points[index + 1].bpm - points[index].bpm) * amount;
            }
        }
        return points[index].bpm;
    }
    return 120.0;
}

inline double bpmAtBeat (double beat, const std::vector<Point>& sourcePoints)
{
    const auto points = normalised (sourcePoints);
    return bpmAtBeatPrepared (beat, points);
}

inline std::pair<double, double> beatAndBpmAtSecondsPrepared (
    double seconds, const std::vector<Point>& points) noexcept
{
    const auto beat = secondsToBeatPrepared (seconds, points);
    return { beat, bpmAtBeatPrepared (beat, points) };
}

inline std::pair<double, double> beatAndBpmAtSeconds (
    double seconds, const std::vector<Point>& points)
{
    const auto prepared = normalised (points);
    return beatAndBpmAtSecondsPrepared (seconds, prepared);
}

inline long long beatToSamplesPrepared (double beat, double sampleRate,
                                        const std::vector<Point>& points) noexcept
{
    if (sampleRate <= 0.0)
        return 0;
    return static_cast<long long> (std::llround (
        beatToSecondsPrepared (beat, points) * sampleRate));
}

inline long long beatToSamples (double beat,
                                double sampleRate,
                                const std::vector<Point>& points)
{
    const auto prepared = normalised (points);
    return beatToSamplesPrepared (beat, sampleRate, prepared);
}

inline int sampleOffsetInBlock (long long eventSample,
                                long long blockStartSample,
                                int blockSize) noexcept
{
    if (blockSize <= 0 || eventSample < blockStartSample
        || eventSample >= blockStartSample + blockSize)
        return -1;
    return static_cast<int> (eventSample - blockStartSample);
}

inline double quantizeBeat (double beat, double gridBeats) noexcept
{
    beat = std::max (0.0, beat);
    if (gridBeats <= 0.0)
        return beat;
    return std::max (0.0, std::round (beat / gridBeats) * gridBeats);
}
}
