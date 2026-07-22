#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

namespace triumph
{
class SampleDelayLine final
{
public:
    void prepare (int channelCount, int delayInSamples, int maximumBlockSize)
    {
        channels = std::max (0, channelCount);
        delay = std::max (0, delayInSamples);
        size = delay > 0
            ? delay + std::max (1, maximumBlockSize) + 1 : 0;
        storage.assign (static_cast<std::size_t> (channels * size), 0.0f);
        writePosition = 0;
    }

    void clear() noexcept
    {
        std::fill (storage.begin(), storage.end(), 0.0f);
        writePosition = 0;
    }

    int getDelaySamples() const noexcept { return delay; }
    bool isPrepared() const noexcept
    {
        return delay == 0 || (channels > 0 && size > delay && ! storage.empty());
    }

    void processAdd (const float* const* source,
                     float* const* destination,
                     int channelCount,
                     int sampleCount) noexcept
    {
        channelCount = std::min ({ channelCount, channels });
        if (source == nullptr || destination == nullptr
            || channelCount <= 0 || sampleCount <= 0)
            return;

        if (delay <= 0 || size <= delay || storage.empty())
        {
            for (int channel = 0; channel < channelCount; ++channel)
                if (source[channel] != nullptr && destination[channel] != nullptr)
                    for (int sample = 0; sample < sampleCount; ++sample)
                        destination[channel][sample] += source[channel][sample];
            return;
        }

        for (int sample = 0; sample < sampleCount; ++sample)
        {
            auto readPosition = writePosition - delay;
            if (readPosition < 0)
                readPosition += size;
            for (int channel = 0; channel < channelCount; ++channel)
            {
                if (source[channel] == nullptr || destination[channel] == nullptr)
                    continue;
                auto* channelStorage = storage.data()
                    + static_cast<std::size_t> (channel * size);
                channelStorage[writePosition] = source[channel][sample];
                destination[channel][sample] += channelStorage[readPosition];
            }
            if (++writePosition >= size)
                writePosition = 0;
        }
    }

private:
    std::vector<float> storage;
    int channels = 0;
    int delay = 0;
    int size = 0;
    int writePosition = 0;
};
}
