#include "MixerComponent.h"

#include "StudioLookAndFeel.h"
#include "core/MixerMath.h"

#include <algorithm>
#include <cmath>

namespace triumph
{
namespace
{
juce::String channelName (const ProjectModel& project,
                          const juce::String& id)
{
    if (id == "master")
        return "Master";
    const auto track = project.getTrackState (id);
    if (track.id.isNotEmpty())
        return track.name;
    const auto channel = project.getMixerChannelState (id);
    return channel.id.isNotEmpty() ? channel.name : "Missing Route";
}
}

class MixerComponent::SpectrumDisplay final : public juce::Component
{
public:
    void setLevels (std::array<float, 128> newLevels)
    {
        levels = std::move (newLevels);
        repaint();
    }

    void paint (juce::Graphics& graphics) override
    {
        auto area = getLocalBounds().toFloat().reduced (1.0f);
        graphics.setColour (StudioColours::surface);
        graphics.fillRoundedRectangle (area, 5.0f);
        graphics.setColour (StudioColours::border);
        graphics.drawRoundedRectangle (area, 5.0f, 1.0f);

        juce::Path spectrum;
        spectrum.startNewSubPath (area.getX(), area.getBottom());
        for (std::size_t index = 0; index < levels.size(); ++index)
        {
            const auto x = area.getX() + area.getWidth()
                * static_cast<float> (index) / static_cast<float> (levels.size() - 1);
            const auto y = area.getBottom() - area.getHeight() * levels[index];
            spectrum.lineTo (x, y);
        }
        spectrum.lineTo (area.getRight(), area.getBottom());
        spectrum.closeSubPath();
        graphics.setColour (StudioColours::primary.withAlpha (0.18f));
        graphics.fillPath (spectrum);
        graphics.setColour (StudioColours::primaryBright);
        graphics.strokePath (spectrum, juce::PathStrokeType (1.3f));

        graphics.setColour (StudioColours::textMuted);
        graphics.setFont (9.0f);
        graphics.drawText ("20", area.toNearestInt().removeFromLeft (28).reduced (4, 2),
                           juce::Justification::bottomLeft, false);
        graphics.drawText ("20k", area.toNearestInt().removeFromRight (32).reduced (4, 2),
                           juce::Justification::bottomRight, false);
    }

private:
    std::array<float, 128> levels {};
};

class MixerComponent::ChannelStrip final : public juce::Component
{
public:
    class SendRow final : public juce::Component
    {
    public:
        SendRow (ProjectModel& model, juce::String owner,
                 MixerSendState state)
            : project (model), ownerId (std::move (owner)), sendId (state.id)
        {
            destinationLabel.setColour (juce::Label::textColourId,
                                        StudioColours::textMuted);
            destinationLabel.getProperties().set ("fontSize", 9.0f);
            addAndMakeVisible (destinationLabel);

            gainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
            gainSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            gainSlider.setRange (0.0, 2.0, 0.01);
            gainSlider.onDragStart = [this]
            {
                project.beginUndoTransaction ("Adjust Send Level");
            };
            gainSlider.onValueChange = [this]
            {
                project.setMixerSendGain (ownerId, sendId,
                                          static_cast<float> (gainSlider.getValue()));
            };
            addAndMakeVisible (gainSlider);

            for (auto* button : { &enabledButton, &preButton, &sidechainButton,
                                  &removeButton })
                addAndMakeVisible (*button);
            enabledButton.setClickingTogglesState (true);
            preButton.setClickingTogglesState (true);
            sidechainButton.setClickingTogglesState (true);
            enabledButton.onClick = [this]
            {
                project.setMixerSendEnabled (ownerId, sendId,
                                              enabledButton.getToggleState());
            };
            preButton.onClick = [this]
            {
                project.setMixerSendPreFader (ownerId, sendId,
                                              preButton.getToggleState());
            };
            sidechainButton.onClick = [this]
            {
                project.setMixerSendSidechain (ownerId, sendId,
                                               sidechainButton.getToggleState());
            };
            removeButton.onClick = [this]
            {
                project.removeMixerSend (ownerId, sendId);
            };
            refresh (state);
        }

        void refresh (const MixerSendState& state)
        {
            destinationLabel.setText (
                (state.sidechain ? "SC  " : "SEND  ")
                    + channelName (project, state.destinationId),
                juce::dontSendNotification);
            gainSlider.setValue (state.gain, juce::dontSendNotification);
            enabledButton.setToggleState (state.enabled,
                                          juce::dontSendNotification);
            preButton.setToggleState (state.preFader,
                                      juce::dontSendNotification);
            sidechainButton.setToggleState (state.sidechain,
                                            juce::dontSendNotification);
        }

        const juce::String& getSendId() const noexcept { return sendId; }

        void paint (juce::Graphics& graphics) override
        {
            graphics.setColour (StudioColours::surface);
            graphics.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
            graphics.setColour (StudioColours::border.withAlpha (0.80f));
            graphics.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f),
                                           4.0f, 1.0f);
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced (3, 1);
            auto top = area.removeFromTop (18);
            removeButton.setBounds (top.removeFromRight (20));
            sidechainButton.setBounds (top.removeFromRight (28));
            preButton.setBounds (top.removeFromRight (30));
            enabledButton.setBounds (top.removeFromRight (28));
            destinationLabel.setBounds (top);
            gainSlider.setBounds (area.reduced (0, 1));
        }

    private:
        ProjectModel& project;
        juce::String ownerId;
        juce::String sendId;
        juce::Label destinationLabel;
        juce::Slider gainSlider;
        juce::TextButton enabledButton { "ON" };
        juce::TextButton preButton { "PRE" };
        juce::TextButton sidechainButton { "SC" };
        juce::TextButton removeButton { "X" };
    };

    ChannelStrip (ProjectModel& model, AudioEngine& sharedEngine,
                  juce::String stableId, bool trackChannel)
        : project (model), engine (sharedEngine), id (std::move (stableId)),
          isTrack (trackChannel)
    {
        nameLabel.setEditable (true, true, false);
        nameLabel.getProperties().set ("bold", true);
        nameLabel.getProperties().set ("fontSize", 12.0f);
        nameLabel.onTextChange = [this]
        {
            if (isTrack)
                project.setTrackName (id, nameLabel.getText());
            else
                project.setMixerChannelName (id, nameLabel.getText());
        };
        addAndMakeVisible (nameLabel);
        typeLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);
        typeLabel.getProperties().set ("fontSize", 9.0f);
        addAndMakeVisible (typeLabel);

        for (auto* button : { &muteButton, &soloButton, &addSendButton,
                              &deleteButton })
            addAndMakeVisible (*button);
        muteButton.setClickingTogglesState (true);
        soloButton.setClickingTogglesState (true);
        muteButton.setColour (juce::TextButton::buttonOnColourId, StudioColours::red);
        soloButton.setColour (juce::TextButton::buttonOnColourId, StudioColours::orange);
        muteButton.onClick = [this]
        {
            if (isTrack)
                project.setTrackMuted (id, muteButton.getToggleState());
            else
                project.setMixerChannelMuted (id, muteButton.getToggleState());
        };
        soloButton.onClick = [this]
        {
            if (isTrack)
                project.setTrackSolo (id, soloButton.getToggleState());
            else
                project.setMixerChannelSolo (id, soloButton.getToggleState());
        };
        deleteButton.onClick = [this]
        {
            if (! isTrack)
                project.removeMixerChannel (id);
        };
        addSendButton.onClick = [this] { showAddSendMenu(); };

        outputBox.onChange = [this]
        {
            const auto selected = outputDestinations[
                juce::jmax (0, outputBox.getSelectedId() - 1)];
            if (isTrack)
                project.setTrackOutputDestination (id, selected);
            else
                project.setMixerChannelOutputDestination (id, selected);
        };
        addAndMakeVisible (outputBox);

        gainSlider.setSliderStyle (juce::Slider::LinearVertical);
        gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 54, 18);
        gainSlider.setRange (0.0, 1.5, 0.01);
        gainSlider.onDragStart = [this]
        {
            project.beginUndoTransaction ("Adjust Mixer Fader");
        };
        gainSlider.onValueChange = [this]
        {
            if (isTrack)
                project.setTrackGain (id, static_cast<float> (gainSlider.getValue()));
            else
                project.setMixerChannelGain (id,
                                             static_cast<float> (gainSlider.getValue()));
        };
        addAndMakeVisible (gainSlider);

        panSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        panSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 44, 16);
        panSlider.setRange (-1.0, 1.0, 0.01);
        panSlider.onDragStart = [this]
        {
            project.beginUndoTransaction ("Adjust Mixer Pan");
        };
        panSlider.onValueChange = [this]
        {
            if (isTrack)
                project.setTrackPan (id, static_cast<float> (panSlider.getValue()));
            else
                project.setMixerChannelPan (id,
                                            static_cast<float> (panSlider.getValue()));
        };
        addAndMakeVisible (panSlider);
        refreshValues();
    }

    void refreshValues()
    {
        const auto channels = project.getMixerChannels();
        outputDestinations.clear();
        outputBox.clear (juce::dontSendNotification);
        outputDestinations.add ("master");
        outputBox.addItem ("OUT  Master", 1);
        for (const auto& channel : channels)
        {
            if (channel.id == id)
                continue;
            outputDestinations.add (channel.id);
            outputBox.addItem ("OUT  " + channel.name,
                               outputDestinations.size());
        }

        juce::String outputId;
        std::vector<MixerSendState> sends;
        if (isTrack)
        {
            const auto state = project.getTrackState (id);
            nameLabel.setText (state.name, juce::dontSendNotification);
            typeLabel.setText (state.isInstrument ? "INSTRUMENT" : "AUDIO",
                               juce::dontSendNotification);
            gainSlider.setValue (state.gain, juce::dontSendNotification);
            panSlider.setValue (state.pan, juce::dontSendNotification);
            muteButton.setToggleState (state.muted, juce::dontSendNotification);
            soloButton.setToggleState (state.solo, juce::dontSendNotification);
            outputId = state.outputDestinationId;
            sends = state.sends;
        }
        else
        {
            const auto state = project.getMixerChannelState (id);
            nameLabel.setText (state.name, juce::dontSendNotification);
            typeLabel.setText (state.kind == MixerChannelKind::returnChannel
                                   ? "RETURN" : "BUS", juce::dontSendNotification);
            gainSlider.setValue (state.gain, juce::dontSendNotification);
            panSlider.setValue (state.pan, juce::dontSendNotification);
            muteButton.setToggleState (state.muted, juce::dontSendNotification);
            soloButton.setToggleState (state.solo, juce::dontSendNotification);
            outputId = state.outputDestinationId;
            sends = state.sends;
        }
        const auto outputIndex = outputDestinations.indexOf (outputId);
        outputBox.setSelectedId (outputIndex >= 0 ? outputIndex + 1 : 1,
                                 juce::dontSendNotification);
        deleteButton.setVisible (! isTrack);

        for (auto* row : sendRows)
        {
            const auto send = std::find_if (sends.begin(), sends.end(),
                                            [row] (const auto& candidate)
            {
                return candidate.id == row->getSendId();
            });
            if (send != sends.end())
                row->refresh (*send);
        }
    }

    void rebuildSendRows()
    {
        sendRows.clear();
        const auto sends = isTrack ? project.getTrackState (id).sends
                                   : project.getMixerChannelState (id).sends;
        for (const auto& send : sends)
        {
            auto* row = sendRows.add (new SendRow (project, id, send));
            addAndMakeVisible (row);
        }
        resized();
    }

    void setMeter (float left, float right, float reduction)
    {
        meterLeft = left;
        meterRight = right;
        sidechainReduction = reduction;
        repaint();
    }

    const juce::String& getChannelId() const noexcept { return id; }

    void paint (juce::Graphics& graphics) override
    {
        auto area = getLocalBounds().toFloat().reduced (1.0f);
        graphics.setColour (StudioColours::panel);
        graphics.fillRoundedRectangle (area, 6.0f);
        graphics.setColour (StudioColours::surface.withAlpha (0.62f));
        graphics.fillRoundedRectangle (area.reduced (4.0f), 5.0f);
        graphics.setColour (StudioColours::border);
        graphics.drawRoundedRectangle (area, 6.0f, 1.0f);

        auto meter = getLocalBounds().removeFromRight (14).reduced (2, 8);
        const auto drawMeter = [&graphics] (juce::Rectangle<int> bounds, float peak)
        {
            graphics.setColour (StudioColours::surface);
            graphics.fillRect (bounds);
            graphics.setColour (StudioColours::border);
            graphics.drawRect (bounds, 1);
            const auto db = juce::Decibels::gainToDecibels (peak, -60.0f);
            const auto proportion = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
            auto level = bounds.removeFromBottom (juce::roundToInt (
                bounds.getHeight() * proportion));
            graphics.setColour (db >= -3.0f ? StudioColours::red
                         : db >= -12.0f ? StudioColours::orange
                                         : StudioColours::green);
            graphics.fillRect (level);
        };
        auto left = meter.removeFromLeft (4);
        meter.removeFromLeft (2);
        drawMeter (left, meterLeft);
        drawMeter (meter.removeFromLeft (4), meterRight);
        if (sidechainReduction > 0.01f)
        {
            graphics.setColour (StudioColours::orange.withAlpha (0.8f));
            graphics.fillRect (getWidth() - 13, 5,
                               juce::roundToInt (sidechainReduction * 7.0f), 3);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (5, 4);
        area.removeFromRight (8);
        auto header = area.removeFromTop (32);
        typeLabel.setBounds (header.removeFromBottom (12));
        nameLabel.setBounds (header);
        auto buttons = area.removeFromTop (22);
        muteButton.setBounds (buttons.removeFromLeft (24).reduced (1));
        soloButton.setBounds (buttons.removeFromLeft (24).reduced (1));
        deleteButton.setBounds (buttons.removeFromRight (20).reduced (1));

        const auto compactStrip = getWidth() < 120;
        outputBox.setVisible (! compactStrip);
        addSendButton.setVisible (! compactStrip);
        for (auto* row : sendRows)
            row->setVisible (! compactStrip);

        if (! compactStrip)
            outputBox.setBounds (area.removeFromTop (24).reduced (0, 2));
        else
            outputBox.setBounds ({});

        auto panArea = area.removeFromTop (46);
        panSlider.setBounds (panArea.withSizeKeepingCentre (
            juce::jmin (48, panArea.getWidth()), panArea.getHeight()).reduced (2));
        gainSlider.setBounds (area.removeFromTop (
            juce::jmax (56, area.getHeight() - (compactStrip ? 0 : 24))).reduced (4, 0));

        if (! compactStrip)
            addSendButton.setBounds (area.removeFromTop (22).reduced (0, 1));
        else
            addSendButton.setBounds ({});
        for (auto* row : sendRows)
            if (! compactStrip)
                row->setBounds (area.removeFromTop (43).reduced (0, 1));
            else
                row->setBounds ({});
    }

private:
    void showAddSendMenu()
    {
        const auto channels = project.getMixerChannels();
        juce::PopupMenu menu;
        menu.addSectionHeader ("Audible Send");
        for (int index = 0; index < static_cast<int> (channels.size()); ++index)
            if (channels[static_cast<std::size_t> (index)].id != id)
                menu.addItem (100 + index,
                              channels[static_cast<std::size_t> (index)].name);
        menu.addSeparator();
        menu.addSectionHeader ("Sidechain");
        for (int index = 0; index < static_cast<int> (channels.size()); ++index)
            if (channels[static_cast<std::size_t> (index)].id != id)
                menu.addItem (1000 + index,
                              channels[static_cast<std::size_t> (index)].name);
        menu.showMenuAsync (
            juce::PopupMenu::Options {}.withTargetComponent (&addSendButton),
            [safe = juce::Component::SafePointer<ChannelStrip> (this), channels]
            (int result)
        {
            if (safe == nullptr || result == 0)
                return;
            const auto sidechain = result >= 1000;
            const auto index = result - (sidechain ? 1000 : 100);
            if (juce::isPositiveAndBelow (index,
                                          static_cast<int> (channels.size())))
                safe->project.addMixerSend (
                    safe->id, channels[static_cast<std::size_t> (index)].id,
                    sidechain);
        });
    }

    ProjectModel& project;
    AudioEngine& engine;
    juce::String id;
    bool isTrack = true;
    juce::Label nameLabel;
    juce::Label typeLabel;
    juce::TextButton muteButton { "M" };
    juce::TextButton soloButton { "S" };
    juce::TextButton addSendButton { "+ SEND" };
    juce::TextButton deleteButton { "X" };
    juce::ComboBox outputBox;
    juce::StringArray outputDestinations;
    juce::Slider gainSlider;
    juce::Slider panSlider;
    juce::OwnedArray<SendRow> sendRows;
    float meterLeft = 0.0f;
    float meterRight = 0.0f;
    float sidechainReduction = 0.0f;
};

class MixerComponent::MasterStrip final : public juce::Component
{
public:
    explicit MasterStrip (ProjectModel& model) : project (model)
    {
        nameLabel.getProperties().set ("bold", true);
        nameLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (nameLabel);
        muteButton.setClickingTogglesState (true);
        muteButton.setColour (juce::TextButton::buttonOnColourId, StudioColours::red);
        muteButton.onClick = [this]
        {
            project.setMasterMuted (muteButton.getToggleState());
        };
        addAndMakeVisible (muteButton);
        gainSlider.setSliderStyle (juce::Slider::LinearVertical);
        gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 58, 18);
        gainSlider.setRange (0.0, 1.25, 0.01);
        gainSlider.onDragStart = [this]
        {
            project.beginUndoTransaction ("Adjust Master Fader");
        };
        gainSlider.onValueChange = [this]
        {
            project.setMasterGain (static_cast<float> (gainSlider.getValue()));
        };
        addAndMakeVisible (gainSlider);
        refresh();
    }

    void refresh()
    {
        gainSlider.setValue (project.getMasterGain(), juce::dontSendNotification);
        muteButton.setToggleState (project.getMasterMuted(),
                                   juce::dontSendNotification);
    }

    void paint (juce::Graphics& graphics) override
    {
        auto area = getLocalBounds().toFloat().reduced (1.0f);
        graphics.setColour (StudioColours::panelRaised);
        graphics.fillRoundedRectangle (area, 6.0f);
        graphics.setColour (StudioColours::surface.withAlpha (0.54f));
        graphics.fillRoundedRectangle (area.reduced (5.0f), 5.0f);
        graphics.setColour (StudioColours::primary);
        graphics.drawRoundedRectangle (area, 6.0f, 1.5f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (8, 6);
        nameLabel.setBounds (area.removeFromTop (28));
        muteButton.setBounds (area.removeFromTop (28).reduced (16, 2));
        area.removeFromTop (10);
        gainSlider.setBounds (area.reduced (12, 0));
    }

private:
    ProjectModel& project;
    juce::Label nameLabel { {}, "MASTER" };
    juce::TextButton muteButton { "MUTE" };
    juce::Slider gainSlider;
};

MixerComponent::MixerComponent (ProjectModel& projectModel,
                                AudioEngine& sharedEngine)
    : project (projectModel), engine (sharedEngine)
{
    titleLabel.getProperties().set ("bold", true);
    titleLabel.getProperties().set ("fontSize", 14.0f);
    addAndMakeVisible (titleLabel);
    routingStatusLabel.setColour (juce::Label::textColourId,
                                  StudioColours::textMuted);
    routingStatusLabel.getProperties().set ("fontSize", 10.0f);
    addAndMakeVisible (routingStatusLabel);
    addAndMakeVisible (addReturnButton);
    addAndMakeVisible (addBusButton);
    addReturnButton.onClick = [this]
    {
        project.addMixerChannel (MixerChannelKind::returnChannel);
    };
    addBusButton.onClick = [this]
    {
        project.addMixerChannel (MixerChannelKind::bus);
    };

    spectrumDisplay = std::make_unique<SpectrumDisplay>();
    addAndMakeVisible (*spectrumDisplay);
    viewport.setViewedComponent (&stripContent, false);
    viewport.setScrollBarsShown (true, true);
    addAndMakeVisible (viewport);
    rebuildStrips();
    startTimerHz (20);
}

MixerComponent::~MixerComponent()
{
    stopTimer();
    viewport.setViewedComponent (nullptr, false);
}

void MixerComponent::refreshFromModel()
{
    const auto signature = makeStructureSignature();
    if (signature != structureSignature)
        rebuildStrips();
    for (auto* strip : strips)
        strip->refreshValues();
    if (masterStrip != nullptr)
        masterStrip->refresh();
}

void MixerComponent::paint (juce::Graphics& graphics)
{
    graphics.setColour (StudioColours::panel);
    graphics.fillAll();
    graphics.setColour (StudioColours::border);
    graphics.drawRect (getLocalBounds(), 1);
    graphics.setColour (StudioColours::primary.withAlpha (0.75f));
    auto accent = getLocalBounds();
    graphics.fillRect (accent.removeFromTop (2));
}

void MixerComponent::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop (34).reduced (8, 4);
    titleLabel.setBounds (header.removeFromLeft (58));
    addReturnButton.setBounds (header.removeFromLeft (74).reduced (2));
    addBusButton.setBounds (header.removeFromLeft (58).reduced (2));
    routingStatusLabel.setBounds (header.removeFromLeft (200).reduced (6, 0));
    if (header.getWidth() > 260)
    {
        spectrumDisplay->setVisible (true);
        spectrumDisplay->setBounds (header.removeFromRight (
            juce::jmin (300, header.getWidth())).reduced (0, 1));
    }
    else
    {
        spectrumDisplay->setVisible (false);
        spectrumDisplay->setBounds ({});
    }
    viewport.setBounds (area.reduced (4, 2));

    auto maximumSendCount = 0;
    for (int index = 0; index < project.getTrackCount(); ++index)
        maximumSendCount = juce::jmax (
            maximumSendCount, static_cast<int> (project.getTrackState (
                project.getTrackId (index)).sends.size()));
    for (const auto& channel : project.getMixerChannels())
        maximumSendCount = juce::jmax (
            maximumSendCount, static_cast<int> (channel.sends.size()));
    const auto stripHeight = juce::jmax (
        viewport.getHeight() - 8, 144 + maximumSendCount * 28);
    const auto width = static_cast<int> (strips.size()) * 78 + 82;
    stripContent.setSize (juce::jmax (viewport.getWidth(), width), stripHeight);
    auto stripArea = stripContent.getLocalBounds().reduced (3, 2);
    for (auto* strip : strips)
        strip->setBounds (stripArea.removeFromLeft (78).reduced (2, 0));
    if (masterStrip != nullptr)
        masterStrip->setBounds (stripArea.removeFromLeft (78).reduced (2, 0));
}

void MixerComponent::timerCallback()
{
    refreshFromModel();
    const auto meters = engine.getMixerMeters();
    for (auto* strip : strips)
    {
        const auto meter = std::find_if (meters.begin(), meters.end(),
                                         [strip] (const auto& candidate)
        {
            return candidate.id == strip->getChannelId();
        });
        if (meter != meters.end())
            strip->setMeter (meter->left, meter->right,
                             meter->sidechainReduction);
    }
    updateSpectrum();
    const auto error = engine.getMixerGraphError();
    routingStatusLabel.setText (error.isEmpty() ? "Routing aligned"
                                                : "Routing held: " + error,
                                juce::dontSendNotification);
    routingStatusLabel.setColour (juce::Label::textColourId,
                                  error.isEmpty() ? StudioColours::green
                                                  : StudioColours::red);
}

void MixerComponent::rebuildStrips()
{
    strips.clear();
    stripContent.removeAllChildren();
    for (int index = 0; index < project.getTrackCount(); ++index)
    {
        auto* strip = strips.add (new ChannelStrip (
            project, engine, project.getTrackId (index), true));
        stripContent.addAndMakeVisible (strip);
        strip->rebuildSendRows();
    }
    for (const auto& channel : project.getMixerChannels())
    {
        auto* strip = strips.add (new ChannelStrip (
            project, engine, channel.id, false));
        stripContent.addAndMakeVisible (strip);
        strip->rebuildSendRows();
    }
    masterStrip = std::make_unique<MasterStrip> (project);
    stripContent.addAndMakeVisible (*masterStrip);
    structureSignature = makeStructureSignature();
    resized();
}

juce::String MixerComponent::makeStructureSignature() const
{
    juce::String result;
    for (int index = 0; index < project.getTrackCount(); ++index)
    {
        const auto track = project.getTrackState (project.getTrackId (index));
        result << "T:" << track.id << ":";
        for (const auto& send : track.sends)
            result << send.id << ",";
        result << ";";
    }
    for (const auto& channel : project.getMixerChannels())
    {
        result << "C:" << channel.id << ":";
        for (const auto& send : channel.sends)
            result << send.id << ",";
        result << ";";
    }
    return result;
}

void MixerComponent::updateSpectrum()
{
    std::array<float, fftSize> incoming {};
    const auto received = engine.pullSpectrumSamples (
        incoming.data(), static_cast<int> (incoming.size()));
    if (received > 0)
    {
        if (received >= fftSize)
        {
            std::copy_n (incoming.end() - fftSize, fftSize,
                         spectrumInput.begin());
            spectrumInputCount = fftSize;
        }
        else
        {
            const auto retained = juce::jmin (spectrumInputCount,
                                              fftSize - received);
            std::move (spectrumInput.begin() + (spectrumInputCount - retained),
                       spectrumInput.begin() + spectrumInputCount,
                       spectrumInput.begin());
            std::copy_n (incoming.begin(), received,
                         spectrumInput.begin() + retained);
            spectrumInputCount = retained + received;
        }
    }

    fftData.fill (0.0f);
    const auto leadingSilence = fftSize - spectrumInputCount;
    std::copy_n (spectrumInput.begin(), spectrumInputCount,
                 fftData.begin() + leadingSilence);
    window.multiplyWithWindowingTable (fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());
    std::array<float, 128> levels {};
    for (std::size_t index = 0; index < levels.size(); ++index)
    {
        const auto bin = mixer::logarithmicSpectrumBin (
            static_cast<int> (index), static_cast<int> (levels.size()), fftSize);
        const auto db = juce::Decibels::gainToDecibels (
            fftData[static_cast<std::size_t> (bin)] / fftSize, -84.0f);
        levels[index] = juce::jlimit (0.0f, 1.0f, (db + 84.0f) / 84.0f);
    }
    spectrumDisplay->setLevels (levels);
}
}
