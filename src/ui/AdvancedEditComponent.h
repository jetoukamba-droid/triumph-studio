#pragma once

#include <JuceHeader.h>

#include "model/ProjectModel.h"

#include <functional>

namespace triumph
{
class AdvancedEditComponent final : public juce::Component
{
public:
    AdvancedEditComponent();

    void setSelection (juce::String trackName,
                       const AudioClipState& clip,
                       bool processing,
                       float progress);

    std::function<void()> onToggleReverse;
    std::function<void()> onNormalize;
    std::function<void (double)> onApplyPitch;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Label titleLabel { {}, "ADVANCED AUDIO EDIT" };
    juce::Label selectionLabel { {}, "Select an audio clip" };
    juce::TextButton reverseButton { "Reverse" };
    juce::TextButton normalizeButton { "Normalize -1 dB" };
    juce::Label pitchLabel { {}, "PITCH" };
    juce::Slider pitchSlider;
    juce::TextButton applyPitchButton { "Apply Pitch" };
    juce::Label processLabel { {}, "Ready" };
    juce::Label existingLabel {
        {}, "Existing non-destructive tools: take lanes, swipe comping, stretch, transients, and warp markers" };
    juce::Label safetyLabel {
        {}, "Pitch creates project-owned 24-bit processed audio. Original media remains untouched; Undo restores it." };
};
}
