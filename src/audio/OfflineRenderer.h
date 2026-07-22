#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

#include "model/ProjectModel.h"

#include <atomic>

namespace triumph
{
class OfflineRenderer final
{
public:
    struct Options
    {
        double sampleRate = 48000.0;
        int bitsPerSample = 24;
        double tailSeconds = 2.0;
        int blockSize = 512;
        juce::int64 timelineStartSamples = 0;
        juce::int64 timelineEndSamples = -1;
        juce::String isolatedTrackId;
        float progressStart = 0.0f;
        float progressSpan = 1.0f;
    };

    static juce::Result renderStereoWav (const ProjectState& snapshot,
                                         const juce::File& destination,
                                         const Options& options,
                                         std::atomic<bool>& cancelRequested,
                                         std::atomic<float>& progress);
};
}
