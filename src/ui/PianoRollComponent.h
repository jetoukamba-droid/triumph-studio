#pragma once

#include <JuceHeader.h>

#include "model/ProjectModel.h"

#include <functional>

namespace triumph
{
class PianoRollComponent final : public juce::Component,
                                 private juce::Timer
{
public:
    using AuditionCallback = std::function<void (int noteNumber, float velocity)>;

    PianoRollComponent (ProjectModel& projectModel,
                        juce::String instrumentTrackId,
                        juce::String midiClipId,
                        AuditionCallback auditionCallback = {});
    ~PianoRollComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::Rectangle<int> getKeyboardBounds() const;
    juce::Rectangle<int> getGridBounds() const;
    juce::Rectangle<int> getNoteBounds (const MidiNoteState&) const;
    MidiClipState getClip() const;
    const MidiNoteState* findSelectedNote (const MidiClipState& clip) const;
    void refreshSelectionLabel();
    int noteAtY (float y) const;
    double beatAtX (float x) const;
    void startAudition (int noteNumber);
    void stopAudition();

    ProjectModel& project;
    const juce::String trackId;
    const juce::String clipId;
    AuditionCallback onAudition;
    juce::TextButton quantizeButton { "Quantize 1/16" };
    juce::ComboBox expressionTypeBox;
    juce::Slider expressionValueSlider;
    juce::TextButton addExpressionButton { "+ MPE Point" };
    juce::ComboBox controllerBox;
    juce::Slider controllerValueSlider;
    juce::TextButton addControllerButton { "+ MIDI2 CC" };
    juce::Label selectionLabel;
    juce::Label instructionLabel {
        {}, "Click piano keys to audition / Double-click grid to add / Right-click note to delete" };
    juce::String selectedNoteId;
    int auditionedNote = -1;
    bool auditionGestureActive = false;
    double selectedExpressionOffsetBeats = 0.0;
    double controllerBeat = 0.0;
    static constexpr int lowestNote = 36;
    static constexpr int highestNote = 84;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollComponent)
};
}
