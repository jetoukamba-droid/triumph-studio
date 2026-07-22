#pragma once

#include <JuceHeader.h>

#include "model/ProjectModel.h"

#include <functional>

namespace triumph
{
class ProducerAssistantComponent final : public juce::Component
{
public:
    ProducerAssistantComponent();

    ProducerSettingsState getSettings() const;
    void setSettings (const ProducerSettingsState& settings);
    void setStatus (juce::String status, bool isWarning = false);
    void setProjectTrackCount (int trackCount);

    std::function<void (const ProducerSettingsState&)> onGenerateChords;
    std::function<void (const ProducerSettingsState&)> onGenerateDrums;
    std::function<void (const ProducerSettingsState&)> onGenerateMelody;
    std::function<void (const ProducerSettingsState&)> onBalanceMix;
    std::function<void (const ProducerSettingsState&)> onSettingsChanged;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void notifySettingsChanged();

    juce::Label titleLabel { {}, "AI PRODUCER" };
    juce::Label localLabel { {}, "LOCAL - PROJECT-AWARE" };
    juce::Label keyLabel { {}, "KEY" };
    juce::ComboBox keyBox;
    juce::Label scaleLabel { {}, "SCALE" };
    juce::ComboBox scaleBox;
    juce::Label styleLabel { {}, "STYLE" };
    juce::ComboBox styleBox;
    juce::Label barsLabel { {}, "LENGTH" };
    juce::ComboBox barsBox;
    juce::TextButton variationButton { "New Variation" };
    juce::Label variationLabel { {}, "Variation 1" };
    juce::TextButton chordsButton { "Create Chords" };
    juce::TextButton drumsButton { "Create Drums" };
    juce::TextButton melodyButton { "Create Melody" };
    juce::TextButton mixButton { "Balance Mix" };
    juce::Label statusLabel { {}, "Choose musical settings, then create a new MIDI track." };
    juce::Label safetyLabel {
        {}, "Runs entirely on this computer. Generated MIDI and mix changes are editable, saved, and undoable." };
    std::uint32_t variation = 1;
    int projectTrackCount = 0;
};
}
