#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace triumph::transient
{
struct Settings
{
    int windowSamples = 256;
    int hopSamples = 128;
    int historyFrames = 24;
    float sensitivity = 2.0f;
    std::int64_t minimumSpacingSamples = 2400;
};

struct Detection
{
    std::int64_t sample = 0;
    float strength = 0.0f;
};

inline std::vector<Detection> detect (const float* samples,
                                      std::int64_t sampleCount,
                                      Settings settings = {})
{
    std::vector<Detection> result;
    if (samples == nullptr || sampleCount < 2)
        return result;

    settings.windowSamples = std::max (16, settings.windowSamples);
    settings.hopSamples = std::max (1, settings.hopSamples);
    settings.historyFrames = std::max (4, settings.historyFrames);
    settings.sensitivity = std::max (0.1f, settings.sensitivity);
    settings.minimumSpacingSamples = std::max<std::int64_t> (1,
                                                              settings.minimumSpacingSamples);

    std::vector<float> flux;
    std::vector<std::int64_t> positions;
    double previousEnergy = 0.0;
    for (std::int64_t start = 0; start < sampleCount;
         start += settings.hopSamples)
    {
        const auto end = std::min<std::int64_t> (sampleCount,
                                                 start + settings.windowSamples);
        double energy = 0.0;
        for (auto index = start; index < end; ++index)
            energy += static_cast<double> (samples[index]) * samples[index];
        energy = std::sqrt (energy / static_cast<double> (std::max<std::int64_t> (
            1, end - start)));
        flux.push_back (static_cast<float> (std::max (0.0, energy - previousEnergy)));
        positions.push_back (start);
        previousEnergy = energy;
    }

    for (std::size_t frame = 1; frame + 1 < flux.size(); ++frame)
    {
        const auto historyStart = frame > static_cast<std::size_t> (settings.historyFrames)
                                      ? frame - settings.historyFrames : 0;
        double mean = 0.0;
        for (auto index = historyStart; index < frame; ++index)
            mean += flux[index];
        mean /= static_cast<double> (std::max<std::size_t> (1, frame - historyStart));
        const auto threshold = static_cast<float> (mean) * settings.sensitivity + 0.0001f;
        if (flux[frame] < threshold || flux[frame] < flux[frame - 1]
            || flux[frame] < flux[frame + 1])
            continue;

        const Detection candidate { positions[frame], flux[frame] };
        if (! result.empty()
            && candidate.sample - result.back().sample < settings.minimumSpacingSamples)
        {
            if (candidate.strength > result.back().strength)
                result.back() = candidate;
            continue;
        }
        result.push_back (candidate);
    }
    return result;
}
}
