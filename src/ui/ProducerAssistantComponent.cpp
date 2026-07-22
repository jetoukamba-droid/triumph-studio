#include "ProducerAssistantComponent.h"

#include "StudioLookAndFeel.h"

namespace triumph
{
ProducerAssistantComponent::ProducerAssistantComponent()
{
    juce::Component* components[] = {
        &titleLabel, &localLabel, &keyLabel, &keyBox, &scaleLabel, &scaleBox,
        &styleLabel, &styleBox, &barsLabel, &barsBox, &variationButton,
        &variationLabel, &chordsButton, &drumsButton, &melodyButton, &mixButton,
        &statusLabel, &safetyLabel
    };
    for (auto* component : components)
        addAndMakeVisible (*component);

    titleLabel.getProperties().set ("fontSize", 15.0f);
    titleLabel.getProperties().set ("bold", true);
    titleLabel.setColour (juce::Label::textColourId, StudioColours::text);
    localLabel.getProperties().set ("bold", true);
    localLabel.setColour (juce::Label::textColourId, StudioColours::green);
    for (auto* label : { &keyLabel, &scaleLabel, &styleLabel, &barsLabel })
        label->setColour (juce::Label::textColourId, StudioColours::textMuted);
    variationLabel.setColour (juce::Label::textColourId, StudioColours::primary);
    statusLabel.setColour (juce::Label::textColourId, StudioColours::primary);
    safetyLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);

    const juce::StringArray keys {
        "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
    };
    keyBox.addItemList (keys, 1);
    scaleBox.addItem ("Major", 1);
    scaleBox.addItem ("Minor", 2);
    styleBox.addItem ("Balanced", 1);
    styleBox.addItem ("Chill", 2);
    styleBox.addItem ("Energetic", 3);
    barsBox.addItem ("4 bars", 1);
    barsBox.addItem ("8 bars", 2);
    barsBox.addItem ("16 bars", 3);

    keyBox.onChange = [this] { notifySettingsChanged(); };
    scaleBox.onChange = [this] { notifySettingsChanged(); };
    styleBox.onChange = [this] { notifySettingsChanged(); };
    barsBox.onChange = [this] { notifySettingsChanged(); };
    variationButton.onClick = [this]
    {
        ++variation;
        if (variation == 0) variation = 1;
        variationLabel.setText ("Variation " + juce::String (
            static_cast<juce::int64> (variation)), juce::dontSendNotification);
        notifySettingsChanged();
    };

    chordsButton.onClick = [this]
    {
        if (onGenerateChords) onGenerateChords (getSettings());
    };
    drumsButton.onClick = [this]
    {
        if (onGenerateDrums) onGenerateDrums (getSettings());
    };
    melodyButton.onClick = [this]
    {
        if (onGenerateMelody) onGenerateMelody (getSettings());
    };
    mixButton.onClick = [this]
    {
        if (onBalanceMix) onBalanceMix (getSettings());
    };
    mixButton.setColour (juce::TextButton::buttonColourId, StudioColours::green);
    mixButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);

    setSettings (ProducerSettingsState {});
}

ProducerSettingsState ProducerAssistantComponent::getSettings() const
{
    ProducerSettingsState settings;
    settings.rootPitchClass = juce::jlimit (0, 11, keyBox.getSelectedId() - 1);
    settings.scale = scaleBox.getSelectedId() == 2 ? "minor" : "major";
    settings.style = styleBox.getSelectedId() == 2 ? "chill"
        : styleBox.getSelectedId() == 3 ? "energetic" : "balanced";
    settings.bars = barsBox.getSelectedId() == 2 ? 8
        : barsBox.getSelectedId() == 3 ? 16 : 4;
    settings.variation = variation;
    return settings;
}

void ProducerAssistantComponent::setSettings (
    const ProducerSettingsState& settings)
{
    keyBox.setSelectedId (juce::jlimit (1, 12,
                                        settings.rootPitchClass + 1),
                          juce::dontSendNotification);
    scaleBox.setSelectedId (settings.scale == "minor" ? 2 : 1,
                            juce::dontSendNotification);
    styleBox.setSelectedId (settings.style == "chill" ? 2
                                : settings.style == "energetic" ? 3 : 1,
                            juce::dontSendNotification);
    barsBox.setSelectedId (settings.bars >= 16 ? 3
                               : settings.bars >= 8 ? 2 : 1,
                           juce::dontSendNotification);
    variation = settings.variation == 0 ? 1 : settings.variation;
    variationLabel.setText ("Variation " + juce::String (
        static_cast<juce::int64> (variation)), juce::dontSendNotification);
}

void ProducerAssistantComponent::setStatus (juce::String status,
                                            bool isWarning)
{
    statusLabel.setText (status, juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId,
                           isWarning ? StudioColours::orange
                                     : StudioColours::primary);
}

void ProducerAssistantComponent::setProjectTrackCount (int trackCount)
{
    projectTrackCount = juce::jmax (0, trackCount);
    mixButton.setEnabled (projectTrackCount > 0);
    mixButton.setTooltip (projectTrackCount > 0
        ? "Apply a local role-aware gain and pan starting balance to every track"
        : "Add at least one track before balancing the mix");
}

void ProducerAssistantComponent::notifySettingsChanged()
{
    if (onSettingsChanged) onSettingsChanged (getSettings());
}

void ProducerAssistantComponent::paint (juce::Graphics& graphics)
{
    graphics.fillAll (StudioColours::panel);
    graphics.setColour (StudioColours::border);
    graphics.drawHorizontalLine (0, 0.0f, static_cast<float> (getWidth()));
}

void ProducerAssistantComponent::resized()
{
    auto bounds = getLocalBounds().reduced (14, 10);
    auto header = bounds.removeFromTop (28);
    titleLabel.setBounds (header.removeFromLeft (130));
    localLabel.setBounds (header.removeFromLeft (210));
    statusLabel.setBounds (header);

    bounds.removeFromTop (7);
    auto settings = bounds.removeFromTop (36);
    keyLabel.setBounds (settings.removeFromLeft (34));
    keyBox.setBounds (settings.removeFromLeft (74).reduced (2));
    scaleLabel.setBounds (settings.removeFromLeft (50));
    scaleBox.setBounds (settings.removeFromLeft (104).reduced (2));
    styleLabel.setBounds (settings.removeFromLeft (48));
    styleBox.setBounds (settings.removeFromLeft (118).reduced (2));
    barsLabel.setBounds (settings.removeFromLeft (58));
    barsBox.setBounds (settings.removeFromLeft (100).reduced (2));
    variationButton.setBounds (settings.removeFromLeft (124).reduced (2));
    variationLabel.setBounds (settings.removeFromLeft (110).reduced (4, 0));

    bounds.removeFromTop (7);
    auto actions = bounds.removeFromTop (42);
    chordsButton.setBounds (actions.removeFromLeft (140).reduced (3));
    drumsButton.setBounds (actions.removeFromLeft (140).reduced (3));
    melodyButton.setBounds (actions.removeFromLeft (140).reduced (3));
    mixButton.setBounds (actions.removeFromLeft (130).reduced (3));

    bounds.removeFromTop (7);
    safetyLabel.setBounds (bounds.removeFromTop (26));
}
}
