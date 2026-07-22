#pragma once

#include "OfflineRenderer.h"

#include <atomic>

namespace triumph
{
class StemExporter final
{
public:
    struct Options
    {
        OfflineRenderer::Options render;
        juce::String projectName { "Untitled Project" };
    };

    static juce::Result renderTrackStems (
        const ProjectState& snapshot,
        const juce::File& destinationDirectory,
        const Options& options,
        std::atomic<bool>& cancelRequested,
        std::atomic<float>& progress);

    static juce::String legalStemName (juce::String name);
};
}
