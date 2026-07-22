#pragma once

#include <JuceHeader.h>

#include "model/ProjectModel.h"

#include <atomic>
#include <memory>

namespace triumph
{
class PluginFreezeRenderer final
{
public:
    static juce::Result render (
        std::unique_ptr<juce::AudioPluginInstance> plugin,
        const TrackState& track,
        const std::vector<TempoPointState>& tempoPoints,
        const std::vector<AutomationLaneState>& automationLanes,
        double sampleRate,
        const juce::File& destination,
        std::atomic<bool>& cancelRequested,
        std::atomic<float>& progress);
};
}
