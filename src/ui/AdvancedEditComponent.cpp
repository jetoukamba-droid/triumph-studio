#include "AdvancedEditComponent.h"

#include "StudioLookAndFeel.h"

#include <cmath>

namespace triumph
{
AdvancedEditComponent::AdvancedEditComponent()
{
    juce::Component* components[] = {
        &titleLabel, &selectionLabel, &reverseButton, &normalizeButton,
        &pitchLabel, &pitchSlider, &applyPitchButton, &processLabel,
        &existingLabel, &safetyLabel
    };
    for (auto* component : components)
        addAndMakeVisible (*component);

    titleLabel.getProperties().set ("fontSize", 15.0f);
    titleLabel.getProperties().set ("bold", true);
    titleLabel.setColour (juce::Label::textColourId, StudioColours::text);
    selectionLabel.setColour (juce::Label::textColourId, StudioColours::primary);
    selectionLabel.getProperties().set ("bold", true);
    processLabel.setColour (juce::Label::textColourId, StudioColours::green);
    existingLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);
    safetyLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);

    reverseButton.setClickingTogglesState (true);
    reverseButton.setColour (juce::TextButton::buttonOnColourId,
                             StudioColours::primary);
    reverseButton.onClick = [this]
    {
        if (onToggleReverse)
            onToggleReverse();
    };
    normalizeButton.onClick = [this]
    {
        if (onNormalize)
            onNormalize();
    };

    pitchSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    pitchSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 66, 24);
    pitchSlider.setRange (-24.0, 24.0, 1.0);
    pitchSlider.setValue (0.0, juce::dontSendNotification);
    pitchSlider.setTextValueSuffix (" st");
    pitchSlider.setDoubleClickReturnValue (true, 0.0);
    applyPitchButton.onClick = [this]
    {
        if (onApplyPitch)
            onApplyPitch (pitchSlider.getValue());
    };
}

void AdvancedEditComponent::setSelection (juce::String trackName,
                                          const AudioClipState& clip,
                                          bool processing,
                                          float progress)
{
    const auto selected = clip.id.isNotEmpty();
    selectionLabel.setText (
        selected ? trackName + "  -  Clip " + clip.id.substring (0, 8)
                 : juce::String ("Select an audio clip"),
        juce::dontSendNotification);
    reverseButton.setToggleState (selected && clip.reversed,
                                  juce::dontSendNotification);
    reverseButton.setButtonText (selected && clip.reversed
                                     ? "Reverse: ON" : "Reverse");
    reverseButton.setEnabled (selected && ! processing);
    normalizeButton.setEnabled (selected && ! processing);
    pitchSlider.setEnabled (selected && ! processing);
    applyPitchButton.setEnabled (selected && ! processing
                                 && std::abs (pitchSlider.getValue()) >= 0.01);
    processLabel.setText (
        processing ? "PROCESSING  "
                         + juce::String (juce::roundToInt (
                             juce::jlimit (0.0f, 1.0f, progress) * 100.0f)) + "%"
                   : juce::String ("Ready"),
        juce::dontSendNotification);
    processLabel.setColour (juce::Label::textColourId,
                            processing ? StudioColours::orange
                                       : StudioColours::green);
}

void AdvancedEditComponent::paint (juce::Graphics& graphics)
{
    graphics.fillAll (StudioColours::panel);
    graphics.setColour (StudioColours::border);
    graphics.drawHorizontalLine (0, 0.0f, static_cast<float> (getWidth()));
}

void AdvancedEditComponent::resized()
{
    auto bounds = getLocalBounds().reduced (14, 10);
    auto header = bounds.removeFromTop (30);
    titleLabel.setBounds (header.removeFromLeft (190));
    selectionLabel.setBounds (header.removeFromLeft (360));
    processLabel.setBounds (header.removeFromRight (150));

    bounds.removeFromTop (8);
    auto controls = bounds.removeFromTop (42);
    reverseButton.setBounds (controls.removeFromLeft (120).reduced (3));
    normalizeButton.setBounds (controls.removeFromLeft (150).reduced (3));
    pitchLabel.setBounds (controls.removeFromLeft (52).reduced (3));
    pitchSlider.setBounds (controls.removeFromLeft (260).reduced (3));
    applyPitchButton.setBounds (controls.removeFromLeft (120).reduced (3));

    bounds.removeFromTop (8);
    existingLabel.setBounds (bounds.removeFromTop (26));
    safetyLabel.setBounds (bounds.removeFromTop (26));
}
}
