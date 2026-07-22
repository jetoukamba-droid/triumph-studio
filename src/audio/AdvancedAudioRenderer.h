#pragma once

#include <JuceHeader.h>

#include "model/ProjectModel.h"

#include <atomic>

namespace triumph
{
class AdvancedAudioRenderer final
{
public:
    struct OutputInfo
    {
        double sampleRate = 0.0;
        int channels = 0;
        juce::int64 lengthInSamples = 0;
    };

    static juce::Result analyseClipPeak (
        const ProjectState& snapshot,
        const juce::String& trackId,
        const juce::String& clipId,
        std::atomic<bool>& cancelRequested,
        std::atomic<float>& progress,
        float& peak);

    static juce::Result renderPitchShiftedClip (
        const ProjectState& snapshot,
        const juce::String& trackId,
        const juce::String& clipId,
        double semitones,
        const juce::File& destination,
        std::atomic<bool>& cancelRequested,
        std::atomic<float>& progress,
        OutputInfo& outputInfo);
};
}
