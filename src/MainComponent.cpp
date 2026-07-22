#include "MainComponent.h"
#include "core/TempoMap.h"
#include "core/AdvancedAudioEdit.h"
#include "core/ProducerAssistant.h"
#include "core/ResponsiveLayout.h"
#include "core/TimelineMath.h"
#include "core/TimeStretch.h"
#include "core/TransientDetector.h"
#include "core/MonitoringCapabilities.h"
#include "core/AudioOutputHealth.h"
#include "core/RealtimeDiagnosticsReport.h"
#include "core/ReleaseIdentity.h"

#include <algorithm>
#include <cmath>

namespace triumph
{
namespace
{
producer::Request makeProducerRequest (const ProducerSettingsState& settings)
{
    return {
        settings.rootPitchClass,
        settings.scale == "minor" ? producer::Scale::minor
                                   : producer::Scale::major,
        settings.style == "chill" ? producer::Style::chill
            : settings.style == "energetic" ? producer::Style::energetic
                                              : producer::Style::balanced,
        settings.bars,
        settings.variation
    };
}

std::vector<MidiNoteState> toMidiNotes (
    const std::vector<producer::Note>& source)
{
    std::vector<MidiNoteState> result;
    result.reserve (source.size());
    for (const auto& note : source)
        result.push_back ({ {}, note.startBeat, note.lengthBeats,
                            note.noteNumber, note.velocity, note.channel });
    return result;
}

juce::String producerKeyName (int pitchClass)
{
    static const juce::StringArray names {
        "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
    };
    return names[juce::jlimit (0, 11, pitchClass)];
}

void drawTriumphBrandMark (juce::Graphics& graphics,
                           juce::Rectangle<float> area)
{
    const auto size = juce::jmin (area.getWidth(), area.getHeight());
    area = area.withSizeKeepingCentre (size, size);

    graphics.setColour (StudioColours::panelRaised);
    graphics.fillRoundedRectangle (area, size * 0.16f);
    graphics.setColour (StudioColours::primary);
    graphics.drawRoundedRectangle (area.reduced (1.0f), size * 0.14f,
                                   juce::jmax (1.2f, size * 0.055f));

    const auto left = area.getX();
    const auto top = area.getY();
    graphics.setColour (StudioColours::primaryBright);
    graphics.fillRoundedRectangle (left + size * 0.23f, top + size * 0.24f,
                                   size * 0.54f, size * 0.14f, size * 0.05f);
    graphics.fillRoundedRectangle (left + size * 0.43f, top + size * 0.31f,
                                   size * 0.14f, size * 0.45f, size * 0.05f);

    juce::Path wave;
    wave.startNewSubPath (left + size * 0.16f, top + size * 0.63f);
    wave.lineTo (left + size * 0.30f, top + size * 0.63f);
    wave.lineTo (left + size * 0.38f, top + size * 0.50f);
    wave.lineTo (left + size * 0.48f, top + size * 0.79f);
    wave.lineTo (left + size * 0.58f, top + size * 0.56f);
    wave.lineTo (left + size * 0.68f, top + size * 0.63f);
    wave.lineTo (left + size * 0.84f, top + size * 0.63f);
    graphics.setColour (StudioColours::primary);
    graphics.strokePath (wave, juce::PathStrokeType (
        juce::jmax (1.2f, size * 0.065f), juce::PathStrokeType::curved,
        juce::PathStrokeType::rounded));
}
}

class PluginEditorWindow final : public juce::DocumentWindow
{
public:
    PluginEditorWindow (const juce::String& title,
                        juce::AudioProcessorEditor& editor)
        : DocumentWindow (title, StudioColours::panel,
                          juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar (true);
        setContentNonOwned (&editor, true);
        setResizable (true, true);
        centreWithSize (juce::jmax (520, editor.getWidth()),
                        juce::jmax (380, editor.getHeight()));
    }

    ~PluginEditorWindow() override
    {
        clearContentComponent();
    }

    void closeButtonPressed() override
    {
        setVisible (false);
    }
};

class AudioDeviceSettingsPanel final : public juce::Component
{
public:
    AudioDeviceSettingsPanel (juce::AudioDeviceManager& manager,
                              std::function<void()> testOutput)
        : selector (manager, 0, 2, 0, 2, true, false, false, false)
    {
        guidance.setText (
            "Choose the real speaker/headphone output, then use Test Output. "
            "The test bypasses the project mixer so it also diagnoses silent routing.",
            juce::dontSendNotification);
        guidance.setJustificationType (juce::Justification::centredLeft);
        guidance.setColour (juce::Label::textColourId, StudioColours::textMuted);
        guidance.setMinimumHorizontalScale (0.8f);
        addAndMakeVisible (guidance);

        testButton.setColour (juce::TextButton::buttonColourId,
                              StudioColours::primary);
        testButton.setColour (juce::TextButton::textColourOffId,
                              juce::Colours::white);
        testButton.setTooltip (
            "Play a one-second 440 Hz tone directly through the selected output");
        testButton.onClick = std::move (testOutput);
        addAndMakeVisible (testButton);
        addAndMakeVisible (selector);
        setSize (600, 700);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (12);
        guidance.setBounds (area.removeFromTop (48));
        area.removeFromTop (6);
        testButton.setBounds (area.removeFromBottom (42).reduced (120, 3));
        area.removeFromBottom (6);
        selector.setBounds (area);
    }

private:
    juce::Label guidance;
    juce::TextButton testButton { "Test Output - 440 Hz" };
    juce::AudioDeviceSelectorComponent selector;
};

class StudioInspectorPanel final : public juce::Component
{
public:
    explicit StudioInspectorPanel (ProjectModel& model) : project (model) {}

    void paint (juce::Graphics& graphics) override
    {
        auto area = getLocalBounds();
        graphics.setColour (StudioColours::panel);
        graphics.fillRect (area);
        graphics.setColour (StudioColours::border);
        graphics.drawRect (area, 1);
        graphics.setColour (StudioColours::primary);
        graphics.fillRect (area.removeFromTop (2));

        auto header = getLocalBounds().reduced (10, 8).removeFromTop (30);
        drawTab (graphics, header.removeFromLeft (94), "INSPECTOR", true);
        drawTab (graphics, header.removeFromLeft (116), "TRACK SETTINGS", false);

        auto body = getLocalBounds().reduced (12, 48);
        const auto hasTrack = project.getTrackCount() > 0;
        const auto track = hasTrack ? project.getTrackState (project.getTrackId (0))
                                    : TrackState {};

        drawSectionTitle (graphics, body, "Selected Track");
        drawTrackBadge (graphics, body.removeFromTop (48), track, hasTrack);
        body.removeFromTop (8);

        drawField (graphics, body, "Type", hasTrack && track.isInstrument
                                      ? "Instrument" : "Audio");
        drawField (graphics, body, "Instrument",
                   hasTrack && track.pluginName.isNotEmpty()
                       ? track.pluginName : "Triumph Drums");

        drawSectionTitle (graphics, body, "Gain");
        drawKnobRow (graphics, body.removeFromTop (72), "0.0 dB", "C", "Room");
        body.removeFromTop (8);

        drawField (graphics, body, "MIDI Input", "All Inputs");
        drawField (graphics, body, "MIDI Monitor", "In");

        drawSectionTitle (graphics, body, "Track Icon");
        auto icons = body.removeFromTop (34);
        for (int i = 0; i < 7; ++i)
        {
            auto icon = icons.removeFromLeft (30).reduced (2);
            graphics.setColour (i == 2 ? StudioColours::primary.withAlpha (0.22f)
                                        : StudioColours::surface);
            graphics.fillRoundedRectangle (icon.toFloat(), 4.0f);
            graphics.setColour (i == 2 ? StudioColours::primary
                                        : StudioColours::border);
            graphics.drawRoundedRectangle (icon.toFloat(), 4.0f, 1.0f);
        }

        body.removeFromTop (10);
        drawSegmentedRow (graphics, body, "View", "Tracks", "All");
        drawSegmentedRow (graphics, body, "Sends", "Post", "Pre");
        drawSegmentedRow (graphics, body, "Meter", "Peak", "RMS");
    }

private:
    void drawTab (juce::Graphics& graphics, juce::Rectangle<int> area,
                  const juce::String& text, bool active)
    {
        graphics.setColour (active ? StudioColours::panelRaised
                                   : StudioColours::surface);
        graphics.fillRect (area);
        graphics.setColour (active ? StudioColours::primary
                                   : StudioColours::border);
        graphics.drawRect (area, 1);
        graphics.setColour (active ? StudioColours::text
                                   : StudioColours::textMuted);
        graphics.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
        graphics.drawFittedText (text, area.reduced (5, 2),
                                 juce::Justification::centred, 1);
    }

    void drawSectionTitle (juce::Graphics& graphics, juce::Rectangle<int>& body,
                           const juce::String& title)
    {
        auto label = body.removeFromTop (21);
        graphics.setColour (StudioColours::textMuted);
        graphics.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
        graphics.drawText (title.toUpperCase(), label,
                           juce::Justification::centredLeft, false);
    }

    void drawTrackBadge (juce::Graphics& graphics, juce::Rectangle<int> area,
                         const TrackState& track, bool hasTrack)
    {
        graphics.setColour (StudioColours::panelRaised);
        graphics.fillRoundedRectangle (area.toFloat(), 5.0f);
        graphics.setColour (StudioColours::border);
        graphics.drawRoundedRectangle (area.toFloat(), 5.0f, 1.0f);
        auto swatch = area.removeFromLeft (14).reduced (4, 13);
        graphics.setColour (hasTrack ? track.colour : StudioColours::primary);
        graphics.fillRoundedRectangle (swatch.toFloat(), 2.0f);
        graphics.setColour (StudioColours::text);
        graphics.setFont (juce::FontOptions (15.0f).withStyle ("Bold"));
        graphics.drawText (hasTrack ? track.name : "No Track",
                           area.reduced (8, 4),
                           juce::Justification::centredLeft, false);
    }

    void drawField (juce::Graphics& graphics, juce::Rectangle<int>& body,
                    const juce::String& label, const juce::String& value)
    {
        auto row = body.removeFromTop (34);
        graphics.setColour (StudioColours::textMuted);
        graphics.setFont (9.0f);
        graphics.drawText (label, row.removeFromLeft (82),
                           juce::Justification::centredLeft, false);
        graphics.setColour (StudioColours::surface);
        graphics.fillRoundedRectangle (row.toFloat().reduced (0, 3), 4.0f);
        graphics.setColour (StudioColours::border);
        graphics.drawRoundedRectangle (row.toFloat().reduced (0.5f, 3.5f),
                                       4.0f, 1.0f);
        graphics.setColour (StudioColours::text);
        graphics.drawFittedText (value, row.reduced (9, 4),
                                 juce::Justification::centredLeft, 1);
    }

    void drawKnobRow (juce::Graphics& graphics, juce::Rectangle<int> area,
                      const juce::String& first,
                      const juce::String& second,
                      const juce::String& third)
    {
        const juce::String labels[] { first, second, third };
        for (int i = 0; i < 3; ++i)
        {
            auto cell = area.removeFromLeft (area.getWidth() / (3 - i)).reduced (4, 0);
            auto knob = cell.withSizeKeepingCentre (34, 34).translated (0, -8);
            graphics.setColour (StudioColours::surface);
            graphics.fillEllipse (knob.toFloat());
            graphics.setColour (StudioColours::border);
            graphics.drawEllipse (knob.toFloat(), 1.0f);
            graphics.setColour (StudioColours::primary);
            juce::Path arc;
            const auto arcBounds = knob.toFloat().reduced (5.0f);
            arc.addCentredArc (arcBounds.getCentreX(), arcBounds.getCentreY(),
                               arcBounds.getWidth() * 0.5f,
                               arcBounds.getHeight() * 0.5f,
                               0.0f,
                               juce::MathConstants<float>::pi * 1.1f,
                               juce::MathConstants<float>::pi * 1.76f,
                               true);
            graphics.strokePath (arc, juce::PathStrokeType (2.2f));
            graphics.setColour (StudioColours::textMuted);
            graphics.setFont (10.0f);
            graphics.drawFittedText (labels[i],
                                     cell.removeFromBottom (20),
                                     juce::Justification::centred, 1);
        }
    }

    void drawSegmentedRow (juce::Graphics& graphics, juce::Rectangle<int>& body,
                           const juce::String& label,
                           const juce::String& left,
                           const juce::String& right)
    {
        auto row = body.removeFromTop (28);
        graphics.setColour (StudioColours::textMuted);
        graphics.setFont (9.0f);
        graphics.drawText (label, row.removeFromLeft (58),
                           juce::Justification::centredLeft, false);
        auto leftCell = row.removeFromLeft (72).reduced (1, 3);
        auto rightCell = row.removeFromLeft (72).reduced (1, 3);
        graphics.setColour (StudioColours::panelRaised);
        graphics.fillRoundedRectangle (leftCell.toFloat(), 4.0f);
        graphics.setColour (StudioColours::surface);
        graphics.fillRoundedRectangle (rightCell.toFloat(), 4.0f);
        graphics.setColour (StudioColours::border);
        graphics.drawRoundedRectangle (leftCell.toFloat(), 4.0f, 1.0f);
        graphics.drawRoundedRectangle (rightCell.toFloat(), 4.0f, 1.0f);
        graphics.setColour (StudioColours::text);
        graphics.drawFittedText (left, leftCell, juce::Justification::centred, 1);
        graphics.setColour (StudioColours::textMuted);
        graphics.drawFittedText (right, rightCell, juce::Justification::centred, 1);
    }

    ProjectModel& project;
};

class StudioBrowserPanel final : public juce::Component
{
public:
    explicit StudioBrowserPanel (ProjectModel& model) : project (model) {}

    void paint (juce::Graphics& graphics) override
    {
        auto area = getLocalBounds();
        graphics.setColour (StudioColours::panel);
        graphics.fillRect (area);
        graphics.setColour (StudioColours::border);
        graphics.drawRect (area, 1);
        graphics.setColour (StudioColours::primary);
        graphics.fillRect (area.removeFromTop (2));

        auto body = getLocalBounds().reduced (10, 8);
        auto tabs = body.removeFromTop (30);
        drawTab (graphics, tabs.removeFromLeft (84), "BROWSER", true);
        drawTab (graphics, tabs.removeFromLeft (84), "PLUGINS", false);
        body.removeFromTop (8);

        auto search = body.removeFromTop (30);
        graphics.setColour (StudioColours::surface);
        graphics.fillRoundedRectangle (search.toFloat(), 5.0f);
        graphics.setColour (StudioColours::border);
        graphics.drawRoundedRectangle (search.toFloat(), 5.0f, 1.0f);
        graphics.setColour (StudioColours::textMuted);
        graphics.setFont (11.0f);
        graphics.drawText ("Search plugins...", search.reduced (9, 2),
                           juce::Justification::centredLeft, false);
        body.removeFromTop (8);

        drawCategory (graphics, body, "All Plugins", "125", false);
        drawCategory (graphics, body, "Instruments", "66", true);
        drawCategory (graphics, body, "Effects", "59", false);
        drawCategory (graphics, body, "Utilities", "15", false);
        drawCategory (graphics, body, "Favorites", "5", false);
        drawCategory (graphics, body, "Recent", "10", false);
        body.removeFromTop (8);

        drawPlugin (graphics, body, "Vital", "Wavetable Synth", StudioColours::violet);
        drawPlugin (graphics, body, "Surge XT", "Synthesizer", StudioColours::clipBlue);
        drawPlugin (graphics, body, "Dexed", "FM Synth", StudioColours::clipAmber);
        drawPlugin (graphics, body, "Triumph Piano", "Virtual Piano", StudioColours::textMuted);
        drawPlugin (graphics, body, "Triumph Drums", "Drum Sampler", StudioColours::clipGreen);

        graphics.setColour (StudioColours::surface);
        graphics.fillRect (getLocalBounds().removeFromBottom (28));
        graphics.setColour (StudioColours::textMuted);
        graphics.setFont (10.0f);
        graphics.drawText ("Manage Plugins",
                           getLocalBounds().removeFromBottom (28).reduced (8, 0),
                           juce::Justification::centred, false);
    }

private:
    void drawTab (juce::Graphics& graphics, juce::Rectangle<int> area,
                  const juce::String& text, bool active)
    {
        graphics.setColour (active ? StudioColours::panelRaised
                                   : StudioColours::surface);
        graphics.fillRect (area);
        graphics.setColour (active ? StudioColours::primary
                                   : StudioColours::border);
        graphics.drawRect (area, 1);
        graphics.setColour (active ? StudioColours::text
                                   : StudioColours::textMuted);
        graphics.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
        graphics.drawFittedText (text, area.reduced (5, 2),
                                 juce::Justification::centred, 1);
    }

    void drawCategory (juce::Graphics& graphics, juce::Rectangle<int>& body,
                       const juce::String& name, const juce::String& count,
                       bool active)
    {
        auto row = body.removeFromTop (25);
        graphics.setColour (active ? StudioColours::panelRaised
                                   : juce::Colours::transparentBlack);
        graphics.fillRect (row);
        graphics.setColour (active ? StudioColours::text : StudioColours::textMuted);
        graphics.setFont (11.0f);
        graphics.drawText (name, row.reduced (8, 0),
                           juce::Justification::centredLeft, false);
        graphics.drawText (count, row.reduced (8, 0),
                           juce::Justification::centredRight, false);
    }

    void drawPlugin (juce::Graphics& graphics, juce::Rectangle<int>& body,
                     const juce::String& name, const juce::String& type,
                     juce::Colour colour)
    {
        auto row = body.removeFromTop (58).reduced (0, 3);
        auto thumb = row.removeFromLeft (52).reduced (3);
        graphics.setColour (StudioColours::surface);
        graphics.fillRoundedRectangle (row.toFloat(), 5.0f);
        graphics.setColour (StudioColours::border.withAlpha (0.70f));
        graphics.drawRoundedRectangle (row.toFloat(), 5.0f, 1.0f);
        graphics.setColour (colour.withAlpha (0.46f));
        graphics.fillRoundedRectangle (thumb.toFloat(), 4.0f);
        graphics.setColour (colour);
        graphics.drawRoundedRectangle (thumb.toFloat(), 4.0f, 1.0f);
        graphics.setColour (StudioColours::text);
        graphics.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
        graphics.drawText (name, row.reduced (8, 6).removeFromTop (20),
                           juce::Justification::centredLeft, false);
        graphics.setColour (StudioColours::textMuted);
        graphics.setFont (10.0f);
        graphics.drawText (type, row.reduced (8, 22),
                           juce::Justification::centredLeft, false);
    }

    ProjectModel& project;
};

MainComponent::MainComponent()
    : arrangement (project, audioEngine),
      mixer (project, audioEngine),
      tempoAutomation (project, audioEngine)
{
    juce::LookAndFeel::setDefaultLookAndFeel (&studioLookAndFeel);

    brandLabel.getProperties().set ("fontSize", 18.0f);
    brandLabel.getProperties().set ("bold", true);
    brandLabel.setColour (juce::Label::textColourId, StudioColours::text);
    addAndMakeVisible (brandLabel);

    projectLabel.getProperties().set ("fontSize", 13.0f);
    projectLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);
    addAndMakeVisible (projectLabel);

    for (auto* button : { &newButton, &openButton, &recentButton, &saveButton,
                          &saveAsButton, &collectButton, &relinkButton, &importButton,
                          &exportButton,
                          &newTrackButton, &midiTrackButton, &pianoRollButton, &pluginButton,
                          &deviceButton,
                          &undoButton, &redoButton, &rewindButton, &playButton,
                          &pauseButton, &stopButton, &recordButton, &monitorButton,
                          &lowLatencyButton, &mixerButton, &automationButton,
                          &advancedEditButton, &producerButton, &helpButton, &moreButton,
                          &splitButton, &useTakeButton, &detectButton,
                          &warpAllButton, &snapButton })
    {
        addAndMakeVisible (*button);
        button->setMouseClickGrabsKeyboardFocus (false);
    }

    newButton.setButtonText ("New");
    openButton.setButtonText ("Open");
    recentButton.setButtonText ("Recent");
    saveButton.setButtonText ("Save");
    saveAsButton.setButtonText ("Save As");
    collectButton.setButtonText ("Collect");
    relinkButton.setButtonText ("Relink");
    importButton.setButtonText ("+ Import Audio");
    exportButton.setButtonText ("Export Mix");
    newTrackButton.setButtonText ("+ Track");
    midiTrackButton.setButtonText ("+ MIDI");
    pianoRollButton.setButtonText ("Keys");
    pluginButton.setButtonText ("VST3");
    deviceButton.setButtonText ("Device");
    undoButton.setButtonText ("Undo");
    redoButton.setButtonText ("Redo");
    lowLatencyButton.setButtonText ("Low Latency");
    moreButton.setButtonText ("More");

    for (auto* button : { &newButton, &openButton, &recentButton, &saveButton,
                          &saveAsButton, &collectButton, &relinkButton,
                          &importButton, &exportButton, &newTrackButton,
                          &midiTrackButton, &pianoRollButton, &pluginButton,
                          &deviceButton, &undoButton, &redoButton,
                          &lowLatencyButton, &moreButton })
        button->setTextOnly (true);

    importButton.setColour (juce::TextButton::buttonColourId, StudioColours::primary);
    importButton.setColour (juce::TextButton::textColourOffId, StudioColours::surface);
    exportButton.setColour (juce::TextButton::buttonColourId, StudioColours::green);
    exportButton.setColour (juce::TextButton::textColourOffId, StudioColours::surface);
    playButton.setColour (juce::TextButton::buttonColourId, StudioColours::primary);
    playButton.setColour (juce::TextButton::textColourOffId, StudioColours::surface);
    recordButton.setColour (juce::TextButton::buttonColourId,
                            StudioColours::red.withAlpha (0.86f));
    recordButton.setColour (juce::TextButton::buttonOnColourId,
                            StudioColours::red);
    recordButton.setColour (juce::TextButton::textColourOffId, StudioColours::surface);
    relinkButton.setColour (juce::TextButton::buttonColourId,
                            StudioColours::red.withAlpha (0.82f));
    relinkButton.setColour (juce::TextButton::textColourOffId, StudioColours::surface);

    newButton.setTooltip ("New project (Ctrl+N)");
    openButton.setTooltip ("Open project (Ctrl+O)");
    recentButton.setTooltip ("Open a recent project");
    saveButton.setTooltip ("Save project (Ctrl+S)");
    saveAsButton.setTooltip ("Save project as (Ctrl+Shift+S)");
    collectButton.setTooltip ("Collect project media and save a portable copy");
    relinkButton.setTooltip ("Relink missing project media");
    importButton.setTooltip ("Import audio (Ctrl+I)");
    exportButton.setTooltip ("Export mix (Ctrl+Shift+E)");
    newTrackButton.setTooltip ("Add and arm an audio track (Ctrl+T)");
    midiTrackButton.setTooltip ("Add an instrument track (Ctrl+Shift+T)");
    pianoRollButton.setTooltip ("Open the piano-roll editor");
    pluginButton.setTooltip ("Open the instrument and audio-effect rack");
    undoButton.setTooltip ("Undo (Ctrl+Z)");
    redoButton.setTooltip ("Redo (Ctrl+Y or Ctrl+Shift+Z)");
    rewindButton.setTooltip ("Return to project start (W or Home)");
    playButton.setTooltip ("Play or resume (Space)");
    pauseButton.setTooltip ("Pause playback (Space while playing)");
    stopButton.setTooltip ("Stop and return to start (.)");
    recordButton.setTooltip ("Start or stop recording (R)");

    newButton.onClick = [this] { createNewProject(); };
    openButton.onClick = [this] { beginOpenProject(); };
    recentButton.onClick = [this] { showRecentProjectsMenu(); };
    saveButton.onClick = [this] { beginSaveProject (false); };
    saveAsButton.onClick = [this] { beginSaveProject (true); };
    collectButton.onClick = [this] { beginCollectAndSave(); };
    relinkButton.onClick = [this] { beginRelinkMissingMedia(); };
    importButton.onClick = [this] { beginImportAudio(); };
    exportButton.onClick = [this] { handleExportButton(); };
    newTrackButton.onClick = [this] { createAndArmAudioTrack(); };
    midiTrackButton.onClick = [this] { createInstrumentTrack(); };
    pianoRollButton.onClick = [this] { showPianoRoll(); };
    pluginButton.onClick = [this] { showVst3Menu(); };
    deviceButton.onClick = [this] { showAudioDeviceSettings(); };
    deviceButton.setTooltip (
        "Select audio hardware and play a direct output test tone");
    undoButton.onClick = [this] { project.undo(); };
    redoButton.onClick = [this] { project.redo(); };
    rewindButton.onClick = [this] { audioEngine.setPositionSeconds (0.0); };
    playButton.onClick = [this] { startPlayback(); };
    pauseButton.onClick = [this] { audioEngine.pause(); };
    stopButton.onClick = [this]
    {
        if (isAnyRecording())
            stopRecording();
        else
            audioEngine.stopAndRewind();
    };
    recordButton.onClick = [this] { toggleRecording(); };

    monitorButton.setClickingTogglesState (true);
    monitorButton.setColour (juce::TextButton::buttonOnColourId, StudioColours::primary);
    monitorButton.setTooltip (
        "Monitor the selected audio input through the master output (M)");
    monitorButton.onClick = [this]
    {
        project.setInputMonitorMode (
            monitorButton.getToggleState() ? InputMonitorMode::always
                                           : InputMonitorMode::off);
        audioStatusLabel.setText (monitorButton.getToggleState()
                                      ? "Input monitoring always on - use headphones"
                                      : "Input monitoring off",
                                  juce::dontSendNotification);
    };
    lowLatencyButton.setClickingTogglesState (true);
    lowLatencyButton.setColour (juce::TextButton::buttonOnColourId,
                                StudioColours::orange);
    lowLatencyButton.setTooltip (
        "Protect armed live-monitoring paths from added compensation delay");
    lowLatencyButton.onClick = [this]
    {
        project.setLowLatencyMonitoring (
            lowLatencyButton.getToggleState());
        audioStatusLabel.setText (
            lowLatencyButton.getToggleState()
                ? "Low-latency monitoring enabled - armed paths stay immediate"
                : "Full delay compensation enabled",
            juce::dontSendNotification);
    };
    mixerButton.setClickingTogglesState (true);
    mixerButton.setColour (juce::TextButton::buttonOnColourId,
                           StudioColours::primary);
    mixerButton.setTooltip (
        "Show sends, returns, buses, sidechains, master strip, and spectrum (B)");
    mixerButton.onClick = [this]
    {
        if (mixerButton.getToggleState())
        {
            automationButton.setToggleState (false, juce::dontSendNotification);
            tempoAutomation.setVisible (false);
            advancedEditButton.setToggleState (false, juce::dontSendNotification);
            advancedEdit.setVisible (false);
            producerButton.setToggleState (false, juce::dontSendNotification);
            producerAssistant.setVisible (false);
            helpButton.setToggleState (false, juce::dontSendNotification);
            helpAssistant.setVisible (false);
        }
        mixer.setVisible (mixerButton.getToggleState());
        resized();
    };
    automationButton.setClickingTogglesState (true);
    automationButton.setColour (juce::TextButton::buttonOnColourId,
                                StudioColours::primary);
    automationButton.setTooltip (
        "Edit tempo ramps, signatures, markers, metronome, and automation (A)");
    automationButton.onClick = [this]
    {
        if (automationButton.getToggleState())
        {
            mixerButton.setToggleState (false, juce::dontSendNotification);
            mixer.setVisible (false);
            advancedEditButton.setToggleState (false, juce::dontSendNotification);
            advancedEdit.setVisible (false);
            producerButton.setToggleState (false, juce::dontSendNotification);
            producerAssistant.setVisible (false);
            helpButton.setToggleState (false, juce::dontSendNotification);
            helpAssistant.setVisible (false);
        }
        tempoAutomation.setVisible (automationButton.getToggleState());
        resized();
    };
    advancedEditButton.setClickingTogglesState (true);
    advancedEditButton.setColour (juce::TextButton::buttonOnColourId,
                                  StudioColours::primary);
    advancedEditButton.setTooltip (
        "Reverse, normalize, and pitch-process the selected audio clip");
    advancedEditButton.onClick = [this]
    {
        if (advancedEditButton.getToggleState())
        {
            mixerButton.setToggleState (false, juce::dontSendNotification);
            automationButton.setToggleState (false, juce::dontSendNotification);
            mixer.setVisible (false);
            tempoAutomation.setVisible (false);
            producerButton.setToggleState (false, juce::dontSendNotification);
            producerAssistant.setVisible (false);
            helpButton.setToggleState (false, juce::dontSendNotification);
            helpAssistant.setVisible (false);
        }
        advancedEdit.setVisible (advancedEditButton.getToggleState());
        resized();
    };
    advancedEdit.onToggleReverse = [this] { toggleSelectedClipReverse(); };
    advancedEdit.onNormalize = [this] { beginNormalizeSelectedClip(); };
    advancedEdit.onApplyPitch = [this] (double semitones)
    {
        beginPitchShiftSelectedClip (semitones);
    };
    producerButton.setClickingTogglesState (true);
    producerButton.setColour (juce::TextButton::buttonOnColourId,
                              StudioColours::primary);
    producerButton.setTooltip (
        "Open the local Chord, Drum, Melody, and Mix producer assistants");
    producerButton.onClick = [this]
    {
        if (producerButton.getToggleState())
        {
            mixerButton.setToggleState (false, juce::dontSendNotification);
            automationButton.setToggleState (false, juce::dontSendNotification);
            advancedEditButton.setToggleState (false, juce::dontSendNotification);
            mixer.setVisible (false);
            tempoAutomation.setVisible (false);
            advancedEdit.setVisible (false);
            producerAssistant.setSettings (project.getProducerSettings());
            helpButton.setToggleState (false, juce::dontSendNotification);
            helpAssistant.setVisible (false);
        }
        producerAssistant.setVisible (producerButton.getToggleState());
        resized();
    };
    producerAssistant.onSettingsChanged = [this] (
        const ProducerSettingsState& settings)
    {
        project.setProducerSettings (settings, false);
    };
    producerAssistant.onGenerateChords = [this] (
        const ProducerSettingsState& settings)
    {
        generateProducerTrack ("chords", settings);
    };
    producerAssistant.onGenerateDrums = [this] (
        const ProducerSettingsState& settings)
    {
        generateProducerTrack ("drums", settings);
    };
    producerAssistant.onGenerateMelody = [this] (
        const ProducerSettingsState& settings)
    {
        generateProducerTrack ("melody", settings);
    };
    producerAssistant.onBalanceMix = [this] (
        const ProducerSettingsState& settings)
    {
        applyProducerMix (settings);
    };
    helpButton.setColour (juce::TextButton::buttonOnColourId,
                          StudioColours::primary);
    helpButton.setTooltip ("Help menu - press F1 for Triumph Assistant");
    helpButton.onClick = [this] { showHelpMenu(); };
    moreButton.setTooltip ("More project and editing commands");
    moreButton.onClick = [this] { showMoreMenu(); };
    helpAssistant.onActionRequested = [this] (const juce::String& action)
    {
        openHelpAction (action);
    };
    splitButton.onClick = [this]
    {
        if (! arrangement.splitSelectedClipAt (audioEngine.getPositionSeconds()))
            audioStatusLabel.setText ("Place the playhead inside the selected clip",
                                      juce::dontSendNotification);
    };
    useTakeButton.setTooltip (
        "Use the selected lane for the whole take; Shift-drag a lane to comp a range");
    useTakeButton.onClick = [this]
    {
        if (! arrangement.activateSelectedTake())
            audioStatusLabel.setText ("Select an inactive take lane first",
                                      juce::dontSendNotification);
    };
    detectButton.setTooltip ("Analyze the selected audio clip for transient onsets");
    detectButton.onClick = [this] { detectSelectedClipTransients(); };
    warpAllButton.setTooltip (
        "Convert detected transients into editable warp markers (undoable)");
    warpAllButton.onClick = [this] { convertSelectedClipTransients(); };

    snapButton.setClickingTogglesState (true);
    snapButton.setToggleState (true, juce::dontSendNotification);
    snapButton.setColour (juce::TextButton::buttonOnColourId, StudioColours::primary);
    snapButton.setTooltip ("Toggle snap for clip edits (S)");
    snapButton.onClick = [this]
    {
        arrangement.setSnapEnabled (snapButton.getToggleState());
        audioStatusLabel.setText (snapButton.getToggleState()
                                      ? "Snap: 250 ms grid"
                                      : "Snap disabled",
                                  juce::dontSendNotification);
    };

    timeLabel.setJustificationType (juce::Justification::centred);
    timeLabel.getProperties().set ("fontSize", 17.0f);
    timeLabel.getProperties().set ("bold", true);
    addAndMakeVisible (timeLabel);

    tempoLabel.getProperties().set ("fontSize", 10.0f);
    tempoLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);
    addAndMakeVisible (tempoLabel);

    tempoSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    tempoSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 52, 22);
    tempoSlider.setRange (40.0, 240.0, 0.1);
    tempoSlider.setValue (project.getTempoBpm(), juce::dontSendNotification);
    tempoSlider.setTextValueSuffix (" BPM");
    tempoSlider.onDragStart = [this]
    {
        project.beginUndoTransaction ("Change Project Tempo");
    };
    tempoSlider.onValueChange = [this]
    {
        project.setTempoBpm (tempoSlider.getValue());
    };
    addAndMakeVisible (tempoSlider);

    zoomLabel.getProperties().set ("fontSize", 10.0f);
    zoomLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);
    addAndMakeVisible (zoomLabel);

    zoomSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    zoomSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    zoomSlider.setRange (30.0, 320.0, 1.0);
    zoomSlider.setValue (arrangement.getPixelsPerSecond(), juce::dontSendNotification);
    zoomSlider.setTooltip ("Timeline horizontal zoom");
    zoomSlider.onValueChange = [this]
    {
        arrangement.setPixelsPerSecond (zoomSlider.getValue());
    };
    addAndMakeVisible (zoomSlider);

    masterLabel.getProperties().set ("fontSize", 10.0f);
    masterLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);
    addAndMakeVisible (masterLabel);

    masterSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    masterSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 22);
    masterSlider.setRange (0.0, 1.25, 0.01);
    masterSlider.setValue (project.getMasterGain(), juce::dontSendNotification);
    masterSlider.setTextValueSuffix ("x");
    masterSlider.onDragStart = [this]
    {
        project.beginUndoTransaction ("Adjust Master Volume");
    };
    masterSlider.onValueChange = [this]
    {
        project.setMasterGain (static_cast<float> (masterSlider.getValue()));
    };
    addAndMakeVisible (masterSlider);

    audioStatusLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);
    audioStatusLabel.getProperties().set ("fontSize", 11.0f);
    addAndMakeVisible (audioStatusLabel);
    realtimeStatusLabel.setJustificationType (juce::Justification::centredRight);
    realtimeStatusLabel.setColour (juce::Label::textColourId, StudioColours::green);
    realtimeStatusLabel.getProperties().set ("fontSize", 10.0f);
    addAndMakeVisible (realtimeStatusLabel);
    inspectorPanel = std::make_unique<StudioInspectorPanel> (project);
    browserPanel = std::make_unique<StudioBrowserPanel> (project);
    addAndMakeVisible (*inspectorPanel);
    addAndMakeVisible (arrangement);
    addAndMakeVisible (*browserPanel);
    arrangement.setTrackDeleteRequestCallback ([this] (const juce::String& trackId)
    {
        requestTrackDeletion (trackId);
    });
    addAndMakeVisible (mixer);
    mixerButton.setToggleState (true, juce::dontSendNotification);
    addChildComponent (tempoAutomation);
    addChildComponent (advancedEdit);
    addChildComponent (producerAssistant);
    addChildComponent (helpAssistant);

    project.setChangeCallback ([this] { handleProjectChanged(); });
    synchroniseEngineWithProject();

    setOpaque (true);
    setWantsKeyboardFocus (true);
    setSize (1440, 900);
    deviceManager.addChangeListener (this);
    deviceManager.addMidiInputDeviceCallback ({}, this);
    auto storedDeviceState = workspace.getAudioDeviceState();
    setAudioChannels (2, 2, storedDeviceState.get());
    if (! hasUsableAudioOutput())
        recoverDefaultAudioOutput();
    saveAudioDeviceStateIfUsable();
    updateAudioDeviceStatus();
    lastAutosaveAttemptMilliseconds = juce::Time::getMillisecondCounterHiRes();
    updateProjectToolbarState();
    startTimerHz (30);

    juce::Component::SafePointer<MainComponent> safeThis (this);
    juce::MessageManager::callAsync ([safeThis]
    {
        if (safeThis != nullptr)
            safeThis->offerCrashRecovery();
    });
}

MainComponent::~MainComponent()
{
    appShuttingDown.store (true, std::memory_order_release);
    stopTimer();
    if (transientAnalysisThread.joinable())
        transientAnalysisThread.join();
    advancedEditCancelRequested.store (true);
    if (advancedEditThread.joinable())
        advancedEditThread.join();
    exportCancelRequested.store (true);
    if (exportThread.joinable())
        exportThread.join();
    if (pluginScanThread.joinable())
        pluginScanThread.join();
    pluginEditorWindow.reset();
    if (isAnyRecording())
        stopRecording();
    deviceManager.removeMidiInputDeviceCallback ({}, this);
    audioEngine.clearLiveMidiNotes();
    saveAudioDeviceStateIfUsable();
    deviceManager.removeChangeListener (this);
    shutdownAudio();
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    activeOutputSampleRate.store (juce::jmax (1.0, sampleRate),
                                  std::memory_order_release);
    recordingInputScratch.setSize (2, juce::jmax (1, samplesPerBlockExpected),
                                   false, true, false);
    audioEngine.prepareToPlay (samplesPerBlockExpected, sampleRate);
    audioCallbackPrepared.store (true, std::memory_order_release);
    queueAudioOutputRecovery();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    recording::BlockPlan recordingBlock;
    auto recordingScratchReady = false;
    if (bufferToFill.buffer != nullptr)
    {
        captureInputPeaks (*bufferToFill.buffer,
                           bufferToFill.startSample,
                           bufferToFill.numSamples);

        if (recorder.isRecording()
            && recordingInputScratch.getNumSamples() >= bufferToFill.numSamples)
        {
            recordingInputScratch.clear();
            const auto channels = juce::jmin (
                recordingInputScratch.getNumChannels(),
                bufferToFill.buffer->getNumChannels());
            for (int channel = 0; channel < channels; ++channel)
                recordingInputScratch.copyFrom (
                    channel, 0, *bufferToFill.buffer, channel,
                    bufferToFill.startSample, bufferToFill.numSamples);
            const auto timelineRate = recordingPlan.getTimelineSampleRate();
            const auto blockStart = static_cast<juce::int64> (std::llround (
                audioEngine.getPositionSeconds() * timelineRate));
            recordingBlock = recordingPlan.processBlock (
                blockStart, bufferToFill.numSamples);
            recordingScratchReady = true;
        }
    }
    audioEngine.getNextAudioBlock (bufferToFill);

    if (bufferToFill.buffer != nullptr)
    {
        mixOutputTestTone (*bufferToFill.buffer,
                           bufferToFill.startSample,
                           bufferToFill.numSamples);
        auto peak = 0.0f;
        for (int channel = 0;
             channel < bufferToFill.buffer->getNumChannels(); ++channel)
            peak = juce::jmax (
                peak,
                bufferToFill.buffer->getMagnitude (
                    channel, bufferToFill.startSample,
                    bufferToFill.numSamples));
        auto published = outputPeak.load (std::memory_order_relaxed);
        while (published < peak
               && ! outputPeak.compare_exchange_weak (
                   published, peak, std::memory_order_relaxed))
        {
        }
        outputCallbackCount.fetch_add (1, std::memory_order_relaxed);
    }

    if (recordingScratchReady && bufferToFill.buffer != nullptr)
    {
        for (int index = 0; index < recordingBlock.spanCount; ++index)
        {
            const auto& span = recordingBlock.spans[
                static_cast<std::size_t> (index)];
            recorder.processSpan (
                recordingInputScratch, *bufferToFill.buffer,
                span.frameOffset,
                bufferToFill.startSample + span.frameOffset,
                span.frameCount, span.passNumber);
        }
        if (recordingBlock.loopWrapped)
            audioEngine.setPositionSeconds (
                static_cast<double> (recordingBlock.wrappedTimelinePosition)
                / recordingPlan.getTimelineSampleRate());
        if (recordingBlock.captureFinished)
            recordingStopRequested.store (true, std::memory_order_release);
    }
}

void MainComponent::captureInputPeaks (const juce::AudioBuffer<float>& input,
                                       int startSample,
                                       int numSamples) noexcept
{
    if (numSamples <= 0)
        return;

    const auto publishPeak = [] (std::atomic<float>& destination, float peak)
    {
        auto previous = destination.load (std::memory_order_relaxed);
        while (previous < peak
               && ! destination.compare_exchange_weak (previous, peak,
                                                       std::memory_order_relaxed))
        {
        }
    };

    if (input.getNumChannels() > 0)
        publishPeak (inputPeakLeft,
                     input.getMagnitude (0, startSample, numSamples));
    if (input.getNumChannels() > 1)
        publishPeak (inputPeakRight,
                     input.getMagnitude (1, startSample, numSamples));
}

void MainComponent::mixOutputTestTone (juce::AudioBuffer<float>& output,
                                       int startSample,
                                       int numSamples) noexcept
{
    if (outputTestRequested.exchange (false, std::memory_order_acq_rel))
    {
        const auto sampleRate = activeOutputSampleRate.load (
            std::memory_order_acquire);
        outputTestTotalSamples = juce::jmax (
            1, static_cast<int> (std::llround (sampleRate)));
        outputTestSamplesRendered = 0;
        outputTestPhase = 0.0;
        outputTestActive.store (true, std::memory_order_release);
    }

    if (! outputTestActive.load (std::memory_order_acquire)
        || numSamples <= 0 || output.getNumChannels() <= 0)
        return;

    const auto sampleRate = activeOutputSampleRate.load (
        std::memory_order_acquire);
    const auto fadeSamples = juce::jmax (
        1, static_cast<int> (std::llround (sampleRate * 0.010)));
    const auto count = juce::jmin (
        numSamples, outputTestTotalSamples - outputTestSamplesRendered);
    const auto phaseIncrement = 2.0 * juce::MathConstants<double>::pi
                                * 440.0 / sampleRate;

    for (int sample = 0; sample < count; ++sample)
    {
        const auto envelope = output::testToneEnvelope (
            outputTestSamplesRendered, outputTestTotalSamples, fadeSamples);
        const auto value = static_cast<float> (
            std::sin (outputTestPhase) * 0.18 * envelope);
        for (int channel = 0; channel < output.getNumChannels(); ++channel)
            output.addSample (channel, startSample + sample, value);
        outputTestPhase += phaseIncrement;
        if (outputTestPhase >= 2.0 * juce::MathConstants<double>::pi)
            outputTestPhase -= 2.0 * juce::MathConstants<double>::pi;
        ++outputTestSamplesRendered;
    }

    if (outputTestSamplesRendered >= outputTestTotalSamples)
        outputTestActive.store (false, std::memory_order_release);
}

void MainComponent::releaseResources()
{
    if (audioCallbackPrepared.exchange (false, std::memory_order_acq_rel)
        && ! appShuttingDown.load (std::memory_order_acquire))
    {
        const auto playbackWasRunning = audioEngine.isPlaying();
        audioOutputRecovery.noteUnavailable (playbackWasRunning);
        if (playbackWasRunning)
            audioEngine.pause();
    }
    outputTestRequested.store (false, std::memory_order_release);
    outputTestActive.store (false, std::memory_order_release);
    outputTestTotalSamples = 0;
    outputTestSamplesRendered = 0;
    recordingInputScratch.setSize (0, 0);
    audioEngine.releaseResources();
}

void MainComponent::paint (juce::Graphics& graphics)
{
    graphics.fillAll (StudioColours::background);

    auto bounds = getLocalBounds();
    auto topBar = bounds.removeFromTop (58);
    auto transportBar = bounds.removeFromTop (58);

    graphics.setColour (StudioColours::background);
    graphics.fillRect (topBar);
    graphics.setColour (StudioColours::panelRaised);
    graphics.fillRect (transportBar);
    graphics.setColour (StudioColours::surface.withAlpha (0.82f));
    graphics.fillRect (transportBar.reduced (10, 7));
    graphics.setColour (StudioColours::border);
    graphics.drawRect (transportBar.reduced (10, 7), 1);
    graphics.drawHorizontalLine (topBar.getBottom(), 0.0f, static_cast<float> (getWidth()));
    graphics.drawHorizontalLine (transportBar.getBottom(), 0.0f, static_cast<float> (getWidth()));
    graphics.drawVerticalLine (246, 9.0f, static_cast<float> (transportBar.getBottom() - 9));

    if (! brandLogoBounds.isEmpty())
        drawTriumphBrandMark (graphics, brandLogoBounds.toFloat());

    if (! inputMeterBounds.isEmpty())
    {
        auto meterArea = inputMeterBounds;
        auto labelArea = meterArea.removeFromLeft (22);
        graphics.setColour (StudioColours::textMuted);
        graphics.setFont (9.0f);
        graphics.drawFittedText ("IN", labelArea, juce::Justification::centred, 1);

        const auto now = juce::Time::getMillisecondCounterHiRes();
        const auto drawMeter = [&graphics, now] (juce::Rectangle<int> area,
                                                  float peak,
                                                  double clipHoldUntil)
        {
            graphics.setColour (StudioColours::surface);
            graphics.fillRoundedRectangle (area.toFloat(), 2.0f);
            graphics.setColour (StudioColours::border);
            graphics.drawRoundedRectangle (area.toFloat(), 2.0f, 1.0f);

            const auto decibels = juce::Decibels::gainToDecibels (peak, -60.0f);
            const auto proportion = juce::jlimit (0.0f, 1.0f, (decibels + 60.0f) / 60.0f);
            auto colour = decibels >= -3.0f ? StudioColours::red
                         : decibels >= -12.0f ? StudioColours::orange
                                              : StudioColours::green;
            auto level = area.withWidth (juce::roundToInt (
                static_cast<float> (area.getWidth()) * proportion));
            graphics.setColour (colour);
            graphics.fillRoundedRectangle (level.toFloat(), 2.0f);

            if (clipHoldUntil > now)
            {
                graphics.setColour (StudioColours::red);
                graphics.fillRect (area.removeFromRight (4));
            }
        };

        auto leftMeter = meterArea.removeFromTop (7);
        meterArea.removeFromTop (3);
        auto rightMeter = meterArea.removeFromTop (7);
        drawMeter (leftMeter, displayedInputPeakLeft, leftClipHoldUntilMilliseconds);
        drawMeter (rightMeter, displayedInputPeakRight, rightClipHoldUntilMilliseconds);
    }
}

void MainComponent::resized()
{
    compactToolbarLayout = layout::useCompactToolbars (getWidth());
    recentButton.setVisible (! compactToolbarLayout);
    collectButton.setVisible (! compactToolbarLayout);
    relinkButton.setVisible (! compactToolbarLayout);
    lowLatencyButton.setVisible (! compactToolbarLayout);
    splitButton.setVisible (! compactToolbarLayout);
    useTakeButton.setVisible (! compactToolbarLayout);
    detectButton.setVisible (! compactToolbarLayout);
    warpAllButton.setVisible (! compactToolbarLayout);
    moreButton.setVisible (compactToolbarLayout);

    auto bounds = getLocalBounds();
    auto topBar = bounds.removeFromTop (58).reduced (12, 8);
    auto titleArea = topBar.removeFromLeft (190);
    brandLogoBounds = titleArea.removeFromLeft (32).reduced (2, 3);
    brandLabel.setBounds (titleArea.removeFromTop (25));
    projectLabel.setBounds (titleArea);

    const auto placeTopButton = [&topBar] (StudioIconButton& button, int width)
    {
        button.setBounds (topBar.removeFromRight (width).reduced (2));
    };

    placeTopButton (importButton, 108);
    placeTopButton (exportButton, 88);
    placeTopButton (newTrackButton, 68);
    placeTopButton (midiTrackButton, 66);
    placeTopButton (pianoRollButton, 60);
    placeTopButton (pluginButton, 62);
    if (! compactToolbarLayout)
        placeTopButton (lowLatencyButton, 96);
    placeTopButton (deviceButton, 70);
    if (compactToolbarLayout)
        placeTopButton (moreButton, 58);
    else
    {
        placeTopButton (relinkButton, 68);
        placeTopButton (collectButton, 72);
    }
    placeTopButton (saveAsButton, 76);
    placeTopButton (saveButton, 60);
    if (! compactToolbarLayout)
        placeTopButton (recentButton, 70);
    placeTopButton (openButton, 62);
    placeTopButton (newButton, 58);
    placeTopButton (redoButton, 60);
    placeTopButton (undoButton, 60);

    auto transportArea = bounds.removeFromTop (78);
    auto transport = transportArea.removeFromTop (58).reduced (16, 8);
    constexpr auto transportButtonWidth = 46;
    rewindButton.setBounds (transport.removeFromLeft (transportButtonWidth).reduced (3));
    playButton.setBounds (transport.removeFromLeft (transportButtonWidth).reduced (3));
    pauseButton.setBounds (transport.removeFromLeft (transportButtonWidth).reduced (3));
    stopButton.setBounds (transport.removeFromLeft (transportButtonWidth).reduced (3));
    recordButton.setBounds (transport.removeFromLeft (transportButtonWidth).reduced (3));
    monitorButton.setBounds (transport.removeFromLeft (transportButtonWidth).reduced (3));
    mixerButton.setBounds (transport.removeFromLeft (transportButtonWidth).reduced (3));
    automationButton.setBounds (transport.removeFromLeft (transportButtonWidth).reduced (3));
    advancedEditButton.setBounds (transport.removeFromLeft (transportButtonWidth).reduced (3));
    producerButton.setBounds (transport.removeFromLeft (transportButtonWidth).reduced (3));
    helpButton.setBounds (transport.removeFromLeft (transportButtonWidth).reduced (3));

    if (! compactToolbarLayout)
    {
        splitButton.setBounds (transport.removeFromLeft (transportButtonWidth).reduced (3));
        useTakeButton.setBounds (transport.removeFromLeft (transportButtonWidth).reduced (3));
        detectButton.setBounds (transport.removeFromLeft (transportButtonWidth).reduced (3));
        warpAllButton.setBounds (transport.removeFromLeft (transportButtonWidth).reduced (3));
    }
    snapButton.setBounds (transport.removeFromLeft (transportButtonWidth).reduced (3));
    timeLabel.setBounds (transport.removeFromLeft (88).reduced (4, 0));
    auto tempo = transport.removeFromLeft (92);
    tempoLabel.setBounds (tempo.removeFromLeft (30));
    tempoSlider.setBounds (tempo);

    auto master = transport.removeFromRight (126);
    masterLabel.setBounds (master.removeFromLeft (38));
    masterSlider.setBounds (master);
    auto zoom = transport.removeFromRight (72);
    zoomLabel.setBounds (zoom.removeFromLeft (28));
    zoomSlider.setBounds (zoom);
    inputMeterBounds = transport.removeFromRight (42).reduced (3, 10);
    auto statusArea = transportArea.reduced (16, 0);
    realtimeStatusLabel.setBounds (statusArea.removeFromRight (250));
    audioStatusLabel.setBounds (statusArea);

    if (mixer.isVisible())
    {
        const auto mixerHeight = juce::jlimit (
            178, 220, juce::roundToInt (bounds.getHeight() * 0.32f));
        mixer.setBounds (bounds.removeFromBottom (mixerHeight));
    }
    else if (tempoAutomation.isVisible())
    {
        const auto panelHeight = juce::jlimit (
            220, 300, juce::roundToInt (bounds.getHeight() * 0.42f));
        tempoAutomation.setBounds (bounds.removeFromBottom (panelHeight));
    }
    else if (advancedEdit.isVisible())
    {
        const auto panelHeight = juce::jlimit (
            150, 185, juce::roundToInt (bounds.getHeight() * 0.25f));
        advancedEdit.setBounds (bounds.removeFromBottom (panelHeight));
    }
    else if (producerAssistant.isVisible())
    {
        const auto panelHeight = juce::jlimit (
            160, 200, juce::roundToInt (bounds.getHeight() * 0.25f));
        producerAssistant.setBounds (bounds.removeFromBottom (panelHeight));
    }
    else if (helpAssistant.isVisible())
    {
        const auto panelHeight = juce::jlimit (
            240, 310, juce::roundToInt (bounds.getHeight() * 0.40f));
        helpAssistant.setBounds (bounds.removeFromBottom (panelHeight));
    }

    const auto showSidePanels = getWidth() >= 900;
    if (inspectorPanel != nullptr)
        inspectorPanel->setVisible (showSidePanels);
    if (browserPanel != nullptr)
        browserPanel->setVisible (showSidePanels);

    if (showSidePanels)
    {
        const auto inspectorWidth = juce::jlimit (
            154, 238, juce::roundToInt (static_cast<float> (getWidth()) * 0.18f));
        const auto browserWidth = juce::jlimit (
            150, 232, juce::roundToInt (static_cast<float> (getWidth()) * 0.17f));
        if (inspectorPanel != nullptr)
            inspectorPanel->setBounds (bounds.removeFromLeft (inspectorWidth));
        bounds.removeFromLeft (4);
        if (browserPanel != nullptr)
            browserPanel->setBounds (bounds.removeFromRight (browserWidth));
        bounds.removeFromRight (4);
    }
    else
    {
        if (inspectorPanel != nullptr)
            inspectorPanel->setBounds ({});
        if (browserPanel != nullptr)
            browserPanel->setBounds ({});
    }

    arrangement.setBounds (bounds);
}

bool MainComponent::keyPressed (const juce::KeyPress& key)
{
    const auto modifiers = key.getModifiers();
    const auto keyCode = key.getKeyCode();

    if (keyCode == juce::KeyPress::F1Key)
    {
        setHelpAssistantOpen (! helpAssistant.isVisible());
        return true;
    }

    if (keyCode == juce::KeyPress::homeKey)
    {
        if (! isAnyRecording())
            audioEngine.setPositionSeconds (0.0);
        return true;
    }

    std::uint8_t modifierMask = shortcuts::noModifier;
    if (modifiers.isCommandDown())
        modifierMask |= shortcuts::commandModifier;
    if (modifiers.isShiftDown())
        modifierMask |= shortcuts::shiftModifier;
    if (modifiers.isAltDown())
        modifierMask |= shortcuts::altModifier;

    const auto command = shortcuts::commandFor (keyCode, modifierMask);
    const auto recording = isAnyRecording();
    const auto processing = advancedEditInProgress.load();

    if (processing
        && (command == shortcuts::Command::undo
            || command == shortcuts::Command::redo
            || command == shortcuts::Command::saveProject
            || command == shortcuts::Command::saveProjectAs
            || command == shortcuts::Command::newProject
            || command == shortcuts::Command::openProject
            || command == shortcuts::Command::importAudio
            || command == shortcuts::Command::exportMix
            || command == shortcuts::Command::addAudioTrack
            || command == shortcuts::Command::addInstrumentTrack
            || command == shortcuts::Command::splitClip
            || keyCode == juce::KeyPress::deleteKey
            || keyCode == juce::KeyPress::backspaceKey))
        return true;

    if (recording && command != shortcuts::Command::toggleRecord
        && command != shortcuts::Command::stop
        && command != shortcuts::Command::none)
        return true;

    switch (command)
    {
        case shortcuts::Command::togglePlayback:
            if (! recording)
            {
                if (audioEngine.isPlaying())
                    audioEngine.pause();
                else
                    startPlayback();
            }
            return true;
        case shortcuts::Command::stop:
            if (recording)
                stopRecording();
            else
                audioEngine.stopAndRewind();
            return true;
        case shortcuts::Command::rewind:
            if (! recording)
                audioEngine.setPositionSeconds (0.0);
            return true;
        case shortcuts::Command::toggleRecord:
            if (recordButton.isEnabled() || recording)
                toggleRecording();
            return true;
        case shortcuts::Command::toggleMonitor:
            if (monitorButton.isEnabled())
                monitorButton.triggerClick();
            return true;
        case shortcuts::Command::toggleMixer:
            if (mixerButton.isEnabled())
                mixerButton.triggerClick();
            return true;
        case shortcuts::Command::toggleTempoAutomation:
            if (automationButton.isEnabled())
                automationButton.triggerClick();
            return true;
        case shortcuts::Command::toggleSnap:
            if (snapButton.isEnabled())
                snapButton.triggerClick();
            return true;
        case shortcuts::Command::zoomIn:
            zoomSlider.setValue (zoomSlider.getValue() + 24.0);
            return true;
        case shortcuts::Command::zoomOut:
            zoomSlider.setValue (zoomSlider.getValue() - 24.0);
            return true;
        case shortcuts::Command::newProject:
            if (newButton.isEnabled()) createNewProject();
            return true;
        case shortcuts::Command::openProject:
            if (openButton.isEnabled()) beginOpenProject();
            return true;
        case shortcuts::Command::saveProject:
            if (saveButton.isEnabled()) beginSaveProject (false);
            return true;
        case shortcuts::Command::saveProjectAs:
            if (saveAsButton.isEnabled()) beginSaveProject (true);
            return true;
        case shortcuts::Command::undo:
            if (undoButton.isEnabled()) project.undo();
            return true;
        case shortcuts::Command::redo:
            if (redoButton.isEnabled()) project.redo();
            return true;
        case shortcuts::Command::importAudio:
            if (importButton.isEnabled()) beginImportAudio();
            return true;
        case shortcuts::Command::exportMix:
            if (exportButton.isEnabled()) handleExportButton();
            return true;
        case shortcuts::Command::addAudioTrack:
            if (newTrackButton.isEnabled()) createAndArmAudioTrack();
            return true;
        case shortcuts::Command::addInstrumentTrack:
            if (midiTrackButton.isEnabled()) createInstrumentTrack();
            return true;
        case shortcuts::Command::splitClip:
            arrangement.splitSelectedClipAt (audioEngine.getPositionSeconds());
            return true;
        case shortcuts::Command::none:
            break;
    }

    if (keyCode == juce::KeyPress::deleteKey
        || keyCode == juce::KeyPress::backspaceKey)
        return recording ? true : arrangement.deleteSelectedClip();

    return false;
}

void MainComponent::requestClose (std::function<void()> closeApproved)
{
    if (advancedEditInProgress.load())
    {
        showInformation ("Audio processing",
                         "Wait for the selected clip's audio processing to finish before closing the project.");
        return;
    }
    if (isAnyRecording())
        stopRecording();
    captureInstrumentPluginState();
    confirmSaveOrDiscard (std::move (closeApproved));
}

void MainComponent::timerCallback()
{
    if (audioOutputRecoveryQueued.exchange (false,
                                            std::memory_order_acq_rel))
        serviceAudioOutputRecovery();

    const auto recording = isAnyRecording();
    const auto advancedProcessing = advancedEditInProgress.load();
    const auto projectEditingEnabled = ! recording && ! advancedProcessing;
    const auto exporting = exportInProgress.load();
    const auto now = juce::Time::getMillisecondCounterHiRes();
    if (recordingStopRequested.exchange (false, std::memory_order_acq_rel)
        && recorder.isRecording())
    {
        stopRecording();
        return;
    }
    if (recorder.isRecording())
    {
        const auto serviceResult = recorder.service();
        if (serviceResult.failed() && ! recordingJournalWarningShown)
        {
            recordingJournalWarningShown = true;
            audioStatusLabel.setText (
                "RECORDING - next loop pass could not be prepared",
                juce::dontSendNotification);
        }
    }
    if (recorder.isRecording() && ! recordingJournalWarningShown)
    {
        const auto journalResult = recorder.checkpointJournals();
        if (journalResult.failed())
        {
            recordingJournalWarningShown = true;
            audioStatusLabel.setText (
                "RECORDING - recovery journal update failed",
                juce::dontSendNotification);
        }
    }
    const auto incomingLeft = inputPeakLeft.exchange (0.0f, std::memory_order_relaxed);
    const auto incomingRight = inputPeakRight.exchange (0.0f, std::memory_order_relaxed);
    displayedInputPeakLeft = juce::jmax (incomingLeft, displayedInputPeakLeft * 0.82f);
    displayedInputPeakRight = juce::jmax (incomingRight, displayedInputPeakRight * 0.82f);
    if (incomingLeft >= 0.985f)
        leftClipHoldUntilMilliseconds = now + 1500.0;
    if (incomingRight >= 0.985f)
        rightClipHoldUntilMilliseconds = now + 1500.0;
    repaint (inputMeterBounds.expanded (2));

    const auto callbackCount = outputCallbackCount.load (
        std::memory_order_relaxed);
    const auto callbackAdvanced = callbackCount != previousOutputCallbackCount;
    const auto testRequested = outputTestRequested.load (
        std::memory_order_acquire);
    const auto testActive = outputTestActive.load (
        std::memory_order_acquire);
    const auto outputExpected = audioEngine.isPlaying()
                                || testRequested || testActive;
    if (outputExpected && ! callbackAdvanced)
        ++stagnantOutputTimerTicks;
    else
        stagnantOutputTimerTicks = 0;

    const auto renderedPeak = outputPeak.exchange (
        0.0f, std::memory_order_relaxed);
    const auto recoveryStatus = audioOutputRecovery.snapshot();
    if (recoveryStatus.state == device::RecoveryState::interrupted
        && ! hasUsableAudioOutput())
        queueAudioOutputRecovery();
    const auto callbackStalled = output::callbacksStalled (
        outputExpected, callbackCount, previousOutputCallbackCount,
        stagnantOutputTimerTicks, 30);
    if (! callbackStalled)
        callbackStallEventActive = false;
    if (testActive)
    {
        audioStatusLabel.setColour (juce::Label::textColourId,
                                    StudioColours::primary);
        audioStatusLabel.setText (
            "Playing direct 440 Hz output test",
            juce::dontSendNotification);
    }
    else if (recoveryStatus.state == device::RecoveryState::failed)
    {
        audioStatusLabel.setColour (juce::Label::textColourId,
                                    StudioColours::red);
        audioStatusLabel.setText (
            "Audio output recovery failed - open Device and select an output",
            juce::dontSendNotification);
    }
    else if (recoveryStatus.state == device::RecoveryState::interrupted
             || recoveryStatus.state == device::RecoveryState::reopening)
    {
        audioStatusLabel.setColour (juce::Label::textColourId,
                                    StudioColours::orange);
        audioStatusLabel.setText (
            recoveryStatus.state == device::RecoveryState::reopening
                ? "Reopening the default audio output"
                : "Audio output interrupted - transport position preserved",
            juce::dontSendNotification);
    }
    else if (outputTestStatusPending && ! testRequested)
    {
        outputTestStatusPending = false;
        audioStatusLabel.setColour (juce::Label::textColourId,
                                    StudioColours::orange);
        audioStatusLabel.setText (
            "Output test finished - if you heard nothing, choose another output device",
            juce::dontSendNotification);
    }
    else if (! isAnyRecording() && ! exportInProgress.load()
             && callbackStalled)
    {
        if (! callbackStallEventActive)
        {
            callbackStallEventActive = true;
            ++callbackStallEvents;
        }
        outputWarningVisible = true;
        audioStatusLabel.setColour (juce::Label::textColourId,
                                    StudioColours::red);
        audioStatusLabel.setText (
            "Audio output callback stalled - open Device and press Test Output",
            juce::dontSendNotification);
    }
    else if (! isAnyRecording() && ! exportInProgress.load()
             && audioEngine.isPlaying() && callbackAdvanced)
    {
        if (renderedPeak <= 0.000001f)
            ++silentPlaybackTimerTicks;
        else
            silentPlaybackTimerTicks = 0;

        if (silentPlaybackTimerTicks >= 150)
        {
            if (! sustainedSilenceEventActive)
            {
                sustainedSilenceEventActive = true;
                ++sustainedSilenceEvents;
            }
            outputWarningVisible = true;
            audioStatusLabel.setColour (juce::Label::textColourId,
                                        StudioColours::red);
            audioStatusLabel.setText (
                "Playback is producing silence - check mute/routing or run Device > Test Output",
                juce::dontSendNotification);
        }
        else if (renderedPeak > 0.000001f && outputWarningVisible)
        {
            sustainedSilenceEventActive = false;
            outputWarningVisible = false;
            updateAudioDeviceStatus();
        }
        else if (renderedPeak > 0.000001f)
        {
            sustainedSilenceEventActive = false;
        }
    }
    else if (! audioEngine.isPlaying())
    {
        silentPlaybackTimerTicks = 0;
        sustainedSilenceEventActive = false;
    }
    previousOutputCallbackCount = callbackCount;

    const auto realtime = audioEngine.getRealtimeStatus();
    const auto pluginRuntime = audioEngine.getPluginRuntimeStatus();
    auto realtimeLoad = 0.0;
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        const auto rate = device->getCurrentSampleRate();
        const auto samples = device->getCurrentBufferSizeSamples();
        if (rate > 0.0 && samples > 0)
        {
            const auto deadline = static_cast<double> (samples)
                                  * 1000000000.0 / rate;
            realtimeLoad = deadline > 0.0
                               ? 100.0 * realtime.averageCallbackNanoseconds()
                                     / deadline
                               : 0.0;
        }
    }
    auto realtimeText =
        "RT " + juce::String (juce::jlimit (0.0, 999.0, realtimeLoad), 1)
            + "%  /  XRUN " + juce::String (realtime.suspectedDropouts);
    if (realtime.missingRenderStateBlocks > 0)
        realtimeText += "  /  STATE "
            + juce::String (realtime.missingRenderStateBlocks);
    else if (realtime.oversizedAudioBlocks > 0)
        realtimeText += "  /  BUF "
            + juce::String (realtime.oversizedAudioBlocks);
    else if (realtime.readAheadStarvationBlocks > 0)
        realtimeText += "  /  STREAM "
            + juce::String (realtime.readAheadStarvationBlocks);
    else if (realtime.parameterQueueOverflows > 0)
        realtimeText += "  /  QUEUE "
            + juce::String (realtime.parameterQueueOverflows);
    else if (realtime.variableBlockSizeChanges > 0)
        realtimeText += "  /  VAR "
            + juce::String (realtime.variableBlockSizeChanges);
    if (audioEngine.hasInstrumentPlugin())
        realtimeText += "  /  PLG " + juce::String (
            static_cast<double> (pluginRuntime.lastProcessNanoseconds)
                / 1000000.0, 2) + "ms";
    if (recoveryStatus.interruptions > 0)
        realtimeText += "  /  DEV "
            + juce::String (recoveryStatus.recoveries) + "/"
            + juce::String (recoveryStatus.interruptions);
    const auto syncStatus = audioEngine.getSyncStatus();
    if (syncStatus.source != midi::SyncSource::internal)
        realtimeText += "  /  SYNC "
            + juce::String (syncStatus.locked ? "LOCK" : "WAIT");
    realtimeStatusLabel.setText (realtimeText, juce::dontSendNotification);
    realtimeStatusLabel.setColour (
        juce::Label::textColourId,
        realtime.suspectedDropouts > 0
                || realtime.missingRenderStateBlocks > 0
                || realtime.oversizedAudioBlocks > 0
                || realtime.readAheadStarvationBlocks > 0
                || realtime.parameterQueueOverflows > 0
                || pluginRuntime.containedExceptions > 0
                || recoveryStatus.state == device::RecoveryState::failed
                || callbackStallEventActive
            ? StudioColours::red : StudioColours::green);
    realtimeStatusLabel.setTooltip (
        "Audio callbacks: " + juce::String (realtime.callbackCount)
        + "\nMaximum callback: "
        + juce::String (static_cast<double> (
              realtime.maximumCallbackNanoseconds) / 1000000.0, 3) + " ms"
        + "\nDeadline misses: " + juce::String (realtime.deadlineMisses)
        + "\nMissing render-state blocks: "
        + juce::String (realtime.missingRenderStateBlocks)
        + "\nOversized audio blocks: "
        + juce::String (realtime.oversizedAudioBlocks)
        + "\nRead-ahead/source starvation blocks: "
        + juce::String (realtime.readAheadStarvationBlocks)
        + "\nRender snapshot swaps: " + juce::String (realtime.snapshotSwaps)
        + "\nParameter queue overflows: "
        + juce::String (realtime.parameterQueueOverflows)
        + "\nActive render generation: "
        + juce::String (realtime.renderGeneration)
        + "\nDevice restarts: "
        + juce::String (realtime.deviceRestartCount)
        + "\nVariable block/rate changes: "
        + juce::String (realtime.variableBlockSizeChanges)
        + "\nRender graph builds: "
        + juce::String (realtime.graphBuildCount)
        + "\nAverage graph build: "
        + juce::String (realtime.averageGraphBuildNanoseconds()
                        / 1000000.0, 3) + " ms"
        + "\nMaximum graph build: "
        + juce::String (static_cast<double> (
              realtime.maximumGraphBuildNanoseconds) / 1000000.0, 3)
        + " ms"
        + "\nRender retire backlog high watermark: "
        + juce::String (realtime.renderRetireBacklogHighWatermark)
        + "\nPlug-in maximum process: "
        + juce::String (static_cast<double> (
              pluginRuntime.maximumProcessNanoseconds) / 1000000.0, 3) + " ms"
        + "\nPlug-in deadline misses: "
        + juce::String (pluginRuntime.deadlineMisses)
        + "\nContained plug-in exceptions: "
        + juce::String (pluginRuntime.containedExceptions)
        + "\nAudio-device interruptions: "
        + juce::String (recoveryStatus.interruptions)
        + "\nAudio-device recovery attempts: "
        + juce::String (recoveryStatus.attempts)
        + "\nAudio-device recoveries: "
        + juce::String (recoveryStatus.recoveries)
        + "\nAudio-device recovery failures: "
        + juce::String (recoveryStatus.failures)
        + "\nCallback-stall warnings: "
        + juce::String (callbackStallEvents)
        + "\nSustained-silence warnings: "
        + juce::String (sustainedSilenceEvents)
        + "\nExternal sync: "
        + (syncStatus.source == midi::SyncSource::midiClock ? "MIDI Clock"
            : syncStatus.source == midi::SyncSource::midiTimeCode ? "MTC"
            : syncStatus.source == midi::SyncSource::link ? "Link"
                                                          : "Internal")
        + " - " + (syncStatus.locked ? "locked" : "waiting")
        + "\nExternal tempo: " + juce::String (syncStatus.tempoBpm, 2)
        + " BPM\nMTC position: "
        + juce::String (syncStatus.timecodeSeconds, 3) + " s");

    if (pluginRuntime.instrumentContainedExceptions
            > lastContainedPluginExceptionCount)
    {
        lastContainedPluginExceptionCount =
            pluginRuntime.instrumentContainedExceptions;
        const auto trackId = audioEngine.getInstrumentPluginTrackId();
        const auto track = project.getTrackState (trackId);
        if (track.id.isNotEmpty())
            project.setInstrumentPluginHealth (
                trackId, track.pluginLatencySamples, false);
        audioStatusLabel.setText (
            "VST3 processing exception contained; plug-in muted and marked unavailable",
            juce::dontSendNotification);
    }
    for (const auto& failedSlotId
         : audioEngine.consumeContainedInsertPluginSlots())
    {
        auto markUnavailable = [this, &failedSlotId] (
            const juce::String& ownerId)
        {
            for (auto slot : project.getPluginSlots (ownerId))
                if (slot.id == failedSlotId)
                {
                    slot.available = false;
                    ++slot.restartCount;
                    return project.updatePluginSlot (ownerId, slot);
                }
            return false;
        };
        auto updated = false;
        for (int index = 0; index < project.getTrackCount() && ! updated;
             ++index)
            updated = markUnavailable (project.getTrackId (index));
        for (const auto& channel : project.getMixerChannels())
            if (! updated)
                updated = markUnavailable (channel.id);
        audioStatusLabel.setText (
            "Insert exception contained; slot bypassed and marked unavailable",
            juce::dontSendNotification);
    }

    int changedPluginLatency = 0;
    if (audioEngine.consumeInstrumentPluginLatencyChange (
            changedPluginLatency))
    {
        const auto trackId = audioEngine.getInstrumentPluginTrackId();
        const auto track = project.getTrackState (trackId);
        if (track.id.isNotEmpty()
            && track.pluginLatencySamples != changedPluginLatency)
        {
            project.setInstrumentPluginHealth (
                trackId, changedPluginLatency, true);
            audioStatusLabel.setText (
                "VST3 latency changed to "
                    + juce::String (changedPluginLatency)
                    + " samples; delay compensation rebuilt",
                juce::dontSendNotification);
        }
    }
    for (const auto& change : audioEngine.consumeInsertPluginLatencyChanges())
    {
        auto updated = false;
        for (int index = 0; index < project.getTrackCount() && ! updated;
             ++index)
        {
            const auto ownerId = project.getTrackId (index);
            for (auto slot : project.getPluginSlots (ownerId))
                if (slot.id == change.slotId
                    && slot.latencySamples != change.latencySamples)
                {
                    slot.latencySamples = change.latencySamples;
                    updated = project.updatePluginSlot (ownerId, slot);
                    break;
                }
        }
        for (const auto& channel : project.getMixerChannels())
        {
            if (updated)
                break;
            for (auto slot : project.getPluginSlots (channel.id))
                if (slot.id == change.slotId
                    && slot.latencySamples != change.latencySamples)
                {
                    slot.latencySamples = change.latencySamples;
                    updated = project.updatePluginSlot (channel.id, slot);
                    break;
                }
        }
        if (updated)
            audioStatusLabel.setText (
                "Insert latency changed to "
                    + juce::String (change.latencySamples)
                    + " samples; route PDC rebuilt",
                juce::dontSendNotification);
    }

    updateTransportDisplay();
    undoButton.setEnabled (projectEditingEnabled && project.canUndo());
    redoButton.setEnabled (projectEditingEnabled && project.canRedo());
    undoButton.setTooltip (project.canUndo()
                               ? juce::String ("Undo ") + project.getUndoDescription()
                                     + " (Ctrl+Z)"
                               : juce::String ("Nothing to undo (Ctrl+Z)"));
    redoButton.setTooltip (project.canRedo()
                               ? juce::String ("Redo ") + project.getRedoDescription()
                                     + " (Ctrl+Y or Ctrl+Shift+Z)"
                               : juce::String ("Nothing to redo (Ctrl+Y or Ctrl+Shift+Z)"));
    splitButton.setEnabled (projectEditingEnabled && arrangement.hasSelectedClip());
    useTakeButton.setEnabled (projectEditingEnabled && arrangement.hasSelectedClip());
    detectButton.setEnabled (projectEditingEnabled && arrangement.hasSelectedClip());
    const auto selectedClip = arrangement.hasSelectedClip()
        ? project.getClipState (arrangement.getSelectedTrackId(),
                                arrangement.getSelectedClipId())
        : AudioClipState {};
    const auto selectedTrack = arrangement.hasSelectedClip()
        ? project.getTrackState (arrangement.getSelectedTrackId())
        : TrackState {};
    advancedEdit.setSelection (selectedTrack.name, selectedClip,
                               advancedProcessing,
                               advancedEditProgress.load());
    producerAssistant.setProjectTrackCount (project.getTrackCount());
    helpAssistant.setContextSummary (
        recording ? "Recording active"
                  : juce::String (project.getTrackCount()) + " tracks - "
                        + (arrangement.hasSelectedClip() ? "clip selected"
                                                         : "no clip selected"));
    warpAllButton.setEnabled (projectEditingEnabled
                              && ! selectedClip.transientMarkers.empty());
    splitButton.setTooltip (arrangement.hasSelectedClip()
                                ? "Split selected clip at playhead (Ctrl+E)"
                                : "Select a clip to split it");
    recordButton.setButtonText (recording ? "Stop Recording" : "Record");
    recordButton.setTooltip (recording ? "Stop recording safely (R)"
                                       : "Start recording (R)");
    recordButton.setToggleState (recording, juce::dontSendNotification);
    newButton.setEnabled (projectEditingEnabled);
    openButton.setEnabled (projectEditingEnabled);
    recentButton.setEnabled (projectEditingEnabled
                             && workspace.getRecentProjectCount() > 0);
    saveButton.setEnabled (projectEditingEnabled);
    saveAsButton.setEnabled (projectEditingEnabled);
    collectButton.setEnabled (projectEditingEnabled
                              && project.getMediaSourceCount() > 0);
    relinkButton.setEnabled (projectEditingEnabled
                             && workspace.countMissingMedia (project) > 0);
    importButton.setEnabled (projectEditingEnabled);
    recordButton.setEnabled (projectEditingEnabled);
    deviceButton.setEnabled (projectEditingEnabled);
    newTrackButton.setEnabled (projectEditingEnabled);
    midiTrackButton.setEnabled (projectEditingEnabled);
    pianoRollButton.setEnabled (projectEditingEnabled);
    pluginButton.setEnabled (projectEditingEnabled
                             && ! pluginScanInProgress.load());
    lowLatencyButton.setEnabled (projectEditingEnabled);
    mixerButton.setEnabled (projectEditingEnabled);
    automationButton.setEnabled (projectEditingEnabled);
    advancedEditButton.setEnabled (projectEditingEnabled && ! exporting);
    producerButton.setEnabled (projectEditingEnabled && ! exporting);
    mixer.setEnabled (projectEditingEnabled);
    tempoAutomation.setEnabled (projectEditingEnabled);
    advancedEdit.setEnabled (! recording);
    producerAssistant.setEnabled (projectEditingEnabled && ! exporting);
    const auto cancellingExport = exporting && exportCancelRequested.load();
    exportButton.setButtonText (exporting
                                    ? (cancellingExport ? "Cancelling" : "Cancel Export")
                                    : "Export Mix");
    exportButton.setColour (juce::TextButton::buttonColourId,
                            exporting ? StudioColours::orange : StudioColours::green);
    exportButton.setEnabled (projectEditingEnabled
                             && ((exporting && ! cancellingExport)
                                 || (! exporting
                                     && project.getProjectLengthInSamples() > 0)));
    pauseButton.setEnabled (! recording);
    rewindButton.setEnabled (! recording);
    playButton.setEnabled (! recording);
    arrangement.setEnabled (projectEditingEnabled);

    if (recording)
    {
        if (midiCapture.isRecording())
        {
            audioStatusLabel.setText (
                "MIDI RECORDING  " + juce::String (midiCapture.getNoteOnCount())
                    + " note" + (midiCapture.getNoteOnCount() == 1 ? "" : "s"),
                juce::dontSendNotification);
        }
        else
        {
            auto* device = deviceManager.getCurrentAudioDevice();
            const auto rate = device != nullptr ? device->getCurrentSampleRate() : 0.0;
            const auto seconds = rate > 0.0
                                     ? static_cast<double> (recorder.getSamplesRecorded()) / rate
                                     : 0.0;
            const auto state = recordingPlan.getState();
            const auto prefix = state == recording::State::preRoll
                                    ? juce::String ("PRE-ROLL  ")
                                : state == recording::State::punch
                                    ? juce::String ("PUNCH RECORDING  ")
                                : state == recording::State::loopPass
                                    ? "LOOP PASS "
                                          + juce::String (recorder.getCurrentPass())
                                          + "  "
                                    : juce::String ("RECORDING  ");
            audioStatusLabel.setText (prefix + formatTime (seconds),
                                      juce::dontSendNotification);
            if (recorder.getDroppedBlockCount() > 0)
                audioStatusLabel.setText ("RECORDING - DISK DROP DETECTED",
                                          juce::dontSendNotification);
            if (recorder.getPassPreparationStarvationCount() > 0)
                audioStatusLabel.setText (
                    "RECORDING - LOOP PASS PREPARATION MISSED",
                    juce::dontSendNotification);
        }
    }
    else if (exportInProgress.load())
    {
        audioStatusLabel.setText (cancellingExport
                                      ? "Cancelling mix export..."
                                      : "Exporting mix "
                                            + juce::String (juce::roundToInt (
                                                exportProgress.load() * 100.0f))
                                            + "%",
                                  juce::dontSendNotification);
    }

    if (dirty && ! recording
        && now - lastAutosaveAttemptMilliseconds >= autosaveIntervalMilliseconds)
    {
        lastAutosaveAttemptMilliseconds = now;
        performAutosave();
    }
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source != &deviceManager)
        return;

    if (isAnyRecording())
    {
        stopRecording();
        audioOutputRecovery.cancelResume();
        showInformation ("Recording stopped",
                         "The device configuration changed. The captured take was closed safely.");
    }

    if (! hasUsableAudioOutput())
    {
        const auto playbackWasRunning = audioEngine.isPlaying();
        audioOutputRecovery.noteUnavailable (playbackWasRunning);
        if (playbackWasRunning)
            audioEngine.pause();
        updateAudioDeviceStatus();
        queueAudioOutputRecovery();
        return;
    }

    saveAudioDeviceStateIfUsable();
    outputWarningVisible = false;
    stagnantOutputTimerTicks = 0;
    silentPlaybackTimerTicks = 0;
    updateAudioDeviceStatus();
    queueAudioOutputRecovery();
}

void MainComponent::handleIncomingMidiMessage (juce::MidiInput*,
                                                const juce::MidiMessage& message)
{
    const auto nowSeconds = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    audioEngine.pushLiveMidiMessage (message);

    if (message.isNoteOn())
    {
        audioEngine.setLiveMidiNote (message.getChannel(),
                                     message.getNoteNumber(),
                                     message.getFloatVelocity());
        midiCapture.processNoteOn (message.getChannel(),
                                   message.getNoteNumber(),
                                   message.getFloatVelocity(),
                                   nowSeconds);
    }
    else if (message.isNoteOff())
    {
        audioEngine.setLiveMidiNote (message.getChannel(),
                                     message.getNoteNumber(), 0.0f);
        midiCapture.processNoteOff (message.getChannel(),
                                    message.getNoteNumber(),
                                    nowSeconds);
    }
    else if (message.isAllNotesOff())
    {
        audioEngine.clearLiveMidiNotes();
        for (int note = 0; note < 128; ++note)
            midiCapture.processNoteOff (message.getChannel(), note, nowSeconds);
    }
}

void MainComponent::beginImportAudio()
{
    if (isAnyRecording())
        stopRecording();
    fileChooser = std::make_unique<juce::FileChooser> (
        "Import audio into Triumph Studio",
        juce::File::getSpecialLocation (juce::File::userMusicDirectory),
        "*.wav;*.wave;*.aif;*.aiff;*.flac;*.mp3;*.ogg");

    const auto chooserFlags = juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::canSelectMultipleItems;

    fileChooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& chooser)
    {
        for (const auto& file : chooser.getResults())
            addAudioFile (file);
    });
}

void MainComponent::handleExportButton()
{
    if (exportInProgress.load())
    {
        exportCancelRequested.store (true);
        audioStatusLabel.setText ("Cancelling mix export...",
                                  juce::dontSendNotification);
        return;
    }
    showExportPresetMenu();
}

void MainComponent::showExportPresetMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("Professional WAV delivery");
    menu.addItem (1, "Master - 48 kHz / 24-bit / 2 s tail");
    menu.addItem (2, "Distribution - 44.1 kHz / 24-bit / 2 s tail");
    menu.addItem (3, "CD - 44.1 kHz / 16-bit / 2 s tail");
    menu.addItem (4, "Hi-Res - 96 kHz / 24-bit / 2 s tail");
    menu.addSeparator();
    menu.addItem (5, "Master (no tail) - 48 kHz / 24-bit");
    menu.addSeparator();
    menu.addItem (6, "Selected clip range - 48 kHz / 24-bit",
                  arrangement.hasSelectedClip());
    menu.addItem (7, "Track stems - 48 kHz / 24-bit / 2 s tail");
    menu.showMenuAsync (
        juce::PopupMenu::Options {}.withTargetComponent (&exportButton),
        [safe = juce::Component::SafePointer<MainComponent> (this)] (int result)
    {
        if (safe == nullptr || result == 0)
            return;
        if (result == 7)
        {
            safe->beginExportStems();
            return;
        }
        OfflineRenderer::Options options;
        if (result == 2 || result == 3)
            options.sampleRate = 44100.0;
        else if (result == 4)
            options.sampleRate = 96000.0;
        if (result == 3)
            options.bitsPerSample = 16;
        if (result == 5)
            options.tailSeconds = 0.0;
        if (result == 6)
        {
            const auto clip = safe->project.getClipState (
                safe->arrangement.getSelectedTrackId(),
                safe->arrangement.getSelectedClipId());
            if (clip.id.isEmpty())
                return;
            options.timelineStartSamples = clip.timelineStartSamples;
            options.timelineEndSamples = clip.timelineStartSamples
                                         + clip.lengthInSamples;
            options.tailSeconds = 0.0;
        }
        safe->beginExportMix (options);
    });
}

void MainComponent::beginExportMix (OfflineRenderer::Options options)
{
    if (isAnyRecording())
        stopRecording();
    if (exportInProgress.load())
        return;
    captureInstrumentPluginState();

    auto directory = currentProjectFile != juce::File()
                         ? currentProjectFile.getParentDirectory()
                         : juce::File::getSpecialLocation (
                               juce::File::userMusicDirectory);
    auto suggested = directory.getChildFile (currentProjectName + " Mix.wav");
    fileChooser = std::make_unique<juce::FileChooser> (
        "Export a deterministic stereo mix", suggested, "*.wav");
    fileChooser->launchAsync (
        juce::FileBrowserComponent::saveMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::warnAboutOverwriting,
        [this, options] (const juce::FileChooser& chooser)
    {
        auto destination = chooser.getResult();
        if (destination == juce::File())
            return;
        if (! destination.hasFileExtension ("wav"))
            destination = destination.withFileExtension ("wav");

        if (exportThread.joinable())
            exportThread.join();
        exportCancelRequested.store (false);
        exportProgress.store (0.0f);
        exportInProgress.store (true);
        const auto snapshot = project.createSnapshot();
        auto safe = juce::Component::SafePointer<MainComponent> (this);
        exportThread = std::thread ([safe, snapshot, destination, options,
                                     cancel = &exportCancelRequested,
                                     progress = &exportProgress]
        {
            const auto result = OfflineRenderer::renderStereoWav (
                snapshot, destination, options, *cancel, *progress);
            juce::MessageManager::callAsync ([safe, result, destination, options]
            {
                if (safe == nullptr)
                    return;
                safe->exportInProgress.store (false);
                if (result.failed())
                {
                    if (result.getErrorMessage().containsIgnoreCase ("cancel"))
                        safe->audioStatusLabel.setText ("Mix export cancelled",
                                                        juce::dontSendNotification);
                    else
                    {
                        safe->audioStatusLabel.setText ("Mix export failed",
                                                        juce::dontSendNotification);
                        safe->showError ("Mix export failed", result.getErrorMessage());
                    }
                }
                else
                {
                    safe->audioStatusLabel.setText ("Mix export complete",
                                                    juce::dontSendNotification);
                    safe->showInformation (
                        "Mix export complete",
                        "Created a "
                            + juce::String (options.sampleRate / 1000.0, 1)
                            + " kHz, " + juce::String (options.bitsPerSample)
                            + "-bit stereo WAV with deterministic dither"
                            + (options.tailSeconds > 0.0
                                   ? " and a " + juce::String (options.tailSeconds, 1)
                                         + " s tail:\n\n"
                                   : ":\n\n")
                            + destination.getFullPathName());
                }
            });
        });
    });
}

void MainComponent::beginExportStems()
{
    if (isAnyRecording())
        stopRecording();
    if (exportInProgress.load())
        return;
    captureInstrumentPluginState();

    const auto startingDirectory = currentProjectFile != juce::File()
        ? currentProjectFile.getParentDirectory()
        : juce::File::getSpecialLocation (juce::File::userMusicDirectory);
    fileChooser = std::make_unique<juce::FileChooser> (
        "Choose a parent folder for the track stems", startingDirectory);
    fileChooser->launchAsync (
        juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectDirectories,
        [this] (const juce::FileChooser& chooser)
    {
        const auto parent = chooser.getResult();
        if (parent == juce::File())
            return;

        const auto folderBase = StemExporter::legalStemName (currentProjectName)
                                + " Stems";
        const auto destination = parent.getNonexistentChildFile (
            folderBase, juce::String {}, false);
        if (exportThread.joinable())
            exportThread.join();
        exportCancelRequested.store (false);
        exportProgress.store (0.0f);
        exportInProgress.store (true);
        const auto snapshot = project.createSnapshot();
        StemExporter::Options options;
        options.projectName = currentProjectName;
        auto safe = juce::Component::SafePointer<MainComponent> (this);
        exportThread = std::thread ([safe, snapshot, destination, options,
                                     cancel = &exportCancelRequested,
                                     progress = &exportProgress]
        {
            const auto result = StemExporter::renderTrackStems (
                snapshot, destination, options, *cancel, *progress);
            juce::MessageManager::callAsync ([safe, result, destination]
            {
                if (safe == nullptr)
                    return;
                safe->exportInProgress.store (false);
                if (result.failed())
                {
                    if (result.getErrorMessage().containsIgnoreCase ("cancel"))
                        safe->audioStatusLabel.setText (
                            "Stem export cancelled", juce::dontSendNotification);
                    else
                    {
                        safe->audioStatusLabel.setText (
                            "Stem export failed", juce::dontSendNotification);
                        safe->showError ("Stem export failed",
                                         result.getErrorMessage());
                    }
                }
                else
                {
                    safe->audioStatusLabel.setText (
                        "Stem export complete", juce::dontSendNotification);
                    safe->showInformation (
                        "Stem export complete",
                        "Created an atomic track-stem delivery with a "
                        "deterministic manifest:\n\n"
                            + destination.getFullPathName());
                }
            });
        });
    });
}

void MainComponent::beginOpenProject()
{
    if (isAnyRecording())
        stopRecording();
    confirmSaveOrDiscard ([this] { showOpenProjectChooser(); });
}

void MainComponent::showOpenProjectChooser()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Open a Triumph Studio project",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.triumph");

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();
        if (file.existsAsFile())
            loadProjectFromFile (file);
    });
}

void MainComponent::beginSaveProject (bool forceChooseFile, SaveCompletion completion)
{
    if (isAnyRecording())
        stopRecording();
    captureInstrumentPluginState();
    if (currentProjectFile != juce::File() && ! forceChooseFile)
    {
        const auto saved = saveProjectToFile (currentProjectFile);
        if (completion)
            completion (saved);
        return;
    }

    auto suggestedFile = currentProjectFile;
    if (suggestedFile == juce::File())
        suggestedFile = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                            .getChildFile (currentProjectName + ".triumph");

    fileChooser = std::make_unique<juce::FileChooser> (
        forceChooseFile ? "Save Triumph Studio project as" : "Save Triumph Studio project",
        suggestedFile,
        "*.triumph");

    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode
                                  | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this, completion] (const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        bool saved = false;

        if (file != juce::File())
        {
            if (! file.hasFileExtension ("triumph"))
                file = file.withFileExtension ("triumph");

            saved = saveProjectToFile (file);
        }

        if (completion)
            completion (saved);
    });
}

void MainComponent::createNewProject()
{
    if (isAnyRecording())
        stopRecording();
    confirmSaveOrDiscard ([this] { resetProject(); });
}

void MainComponent::confirmSaveOrDiscard (std::function<void()> continuation)
{
    if (! dirty)
    {
        continuation();
        return;
    }

    juce::AlertWindow::showYesNoCancelBox (
        juce::AlertWindow::QuestionIcon,
        "Save changes to " + currentProjectName + "?",
        "Your project has unsaved changes.",
        "Save",
        "Discard",
        "Cancel",
        this,
        juce::ModalCallbackFunction::create ([this, continuation] (int result)
        {
            if (result == 1)
            {
                beginSaveProject (false, [continuation] (bool saved)
                {
                    if (saved)
                        continuation();
                });
            }
            else if (result == 2)
            {
                workspace.deleteRecoveryFile (project);
                continuation();
            }
        }));
}

void MainComponent::resetProject()
{
    audioEngine.stopAndRewind();
    workspace.deleteRecoveryFile (project);
    replacingProject = true;
    project.reset();
    replacingProject = false;
    synchroniseEngineWithProject();
    arrangement.refreshFromModel();
    masterSlider.setValue (project.getMasterGain(), juce::dontSendNotification);
    currentProjectFile = juce::File {};
    setProjectName (project.getName());
    setDirty (false);
    audioStatusLabel.setText ("New project ready", juce::dontSendNotification);
    updateProjectToolbarState();
}

void MainComponent::loadProjectFromFile (const juce::File& file, bool isRecovery)
{
    ProjectState state;
    const auto result = ProjectDocument::load (file, state);

    if (result.failed())
    {
        showError ("Project could not be opened", result.getErrorMessage());
        return;
    }

    audioEngine.stopAndRewind();
    replacingProject = true;
    project.replaceWithSnapshot (state);
    replacingProject = false;
    synchroniseEngineWithProject();
    arrangement.refreshFromModel();

    juce::StringArray missingFiles;
    for (const auto& source : state.mediaSources)
        if (! source.file.existsAsFile())
            missingFiles.addIfNotAlreadyThere (source.file.getFullPathName());

    masterSlider.setValue (project.getMasterGain(), juce::dontSendNotification);
    currentProjectFile = isRecovery ? juce::File() : file;
    setProjectName (project.getName().isNotEmpty() ? project.getName()
                                                   : file.getFileNameWithoutExtension());

    if (isRecovery)
    {
        setDirty (true);
        audioStatusLabel.setText ("Recovered autosave - save the project to keep it",
                                  juce::dontSendNotification);
    }
    else
    {
        setDirty (false);
        workspace.addRecentProject (file);
        audioStatusLabel.setText ("Project loaded", juce::dontSendNotification);
    }

    updateProjectToolbarState();

    if (! missingFiles.isEmpty())
        showError ("Some audio files are missing",
                   "Use Relink to search for these files:\n\n"
                       + missingFiles.joinIntoString ("\n"));

    offerRecordingRecovery();
}

bool MainComponent::saveProjectToFile (const juce::File& file)
{
    const auto previousName = project.getName();
    const auto previousDisplayName = currentProjectName;
    const auto targetAlreadyExists = file.existsAsFile();
    const auto nameFromFile = file.getFileNameWithoutExtension();
    const auto savedName = nameFromFile.isNotEmpty() ? nameFromFile : currentProjectName;
    replacingProject = true;
    project.setName (savedName, false);
    replacingProject = false;

    auto backupResult = juce::Result::ok();
    if (targetAlreadyExists)
        backupResult = workspace.createRollingBackup (file, project);

    const auto result = ProjectDocument::save (file, project);
    if (result.failed())
    {
        replacingProject = true;
        project.setName (previousName, false);
        replacingProject = false;
        setProjectName (previousDisplayName);
        showError ("Project could not be saved", result.getErrorMessage());
        return false;
    }

    setProjectName (savedName);
    currentProjectFile = file;
    workspace.addRecentProject (file);
    workspace.deleteRecoveryFile (project);
    setDirty (false);
    audioStatusLabel.setText ("Project saved", juce::dontSendNotification);

    if (! targetAlreadyExists)
        backupResult = workspace.createRollingBackup (file, project);

    if (backupResult.failed())
        showError ("Project saved, but backup failed", backupResult.getErrorMessage());

    updateProjectToolbarState();
    return true;
}

void MainComponent::beginCollectAndSave()
{
    if (isAnyRecording())
        stopRecording();

    if (currentProjectFile == juce::File())
    {
        beginSaveProject (true, [this] (bool saved)
        {
            if (saved)
                beginCollectAndSave();
        });
        return;
    }

    audioEngine.pause();
    int copiedFiles = 0;
    const auto result = workspace.collectMedia (project, currentProjectFile, copiedFiles);

    if (result.failed())
    {
        showError ("Media could not be collected", result.getErrorMessage());
        return;
    }

    if (! saveProjectToFile (currentProjectFile))
        return;

    showInformation ("Collect and Save complete",
                     juce::String (copiedFiles) + " media file"
                         + (copiedFiles == 1 ? " was" : "s were")
                         + " copied into the project Media folder.");
}

void MainComponent::beginRelinkMissingMedia()
{
    if (isAnyRecording())
        stopRecording();

    if (workspace.countMissingMedia (project) == 0)
    {
        showInformation ("No missing media", "Every project media source is currently available.");
        return;
    }

    fileChooser = std::make_unique<juce::FileChooser> (
        "Choose a folder to search for missing media",
        currentProjectFile != juce::File() ? currentProjectFile.getParentDirectory()
                                           : juce::File::getSpecialLocation (
                                                 juce::File::userMusicDirectory));

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectDirectories,
                              [this] (const juce::FileChooser& chooser)
    {
        const auto directory = chooser.getResult();
        if (! directory.isDirectory())
            return;

        int relinkedCount = 0;
        juce::StringArray unresolved;
        const auto result = workspace.relinkMissingMedia (project,
                                                          directory,
                                                          relinkedCount,
                                                          unresolved);

        if (result.failed())
        {
            showError ("Media relink failed", result.getErrorMessage());
            return;
        }

        synchroniseEngineWithProject();
        arrangement.refreshFromModel();
        updateProjectToolbarState();

        auto message = juce::String (relinkedCount) + " media file"
                       + (relinkedCount == 1 ? " was" : "s were") + " relinked.";
        if (! unresolved.isEmpty())
            message += "\n\nStill missing:\n" + unresolved.joinIntoString ("\n");

        showInformation ("Media relink complete", message);
    });
}

void MainComponent::showRecentProjectsMenu (juce::Component* target)
{
    if (isAnyRecording())
        stopRecording();

    juce::PopupMenu menu;

    for (int index = 0; index < workspace.getRecentProjectCount(); ++index)
    {
        const auto file = workspace.getRecentProject (index);
        menu.addItem (index + 1,
                      file.getFileNameWithoutExtension() + "  -  "
                          + file.getParentDirectory().getFullPathName(),
                      file.existsAsFile());
    }

    if (workspace.getRecentProjectCount() == 0)
        menu.addItem (1, "No recent projects", false);

    menu.addSeparator();
    constexpr int clearRecentId = 1000;
    menu.addItem (clearRecentId,
                  "Clear Recent Projects",
                  workspace.getRecentProjectCount() > 0);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (
                            target != nullptr ? target : &recentButton),
                        [this] (int result)
    {
        if (result == 1000)
        {
            workspace.clearRecentProjects();
            updateProjectToolbarState();
            return;
        }

        if (result <= 0 || result > workspace.getRecentProjectCount())
            return;

        const auto file = workspace.getRecentProject (result - 1);
        confirmSaveOrDiscard ([this, file]
        {
            if (file.existsAsFile())
                loadProjectFromFile (file);
            else
                showError ("Recent project is unavailable", file.getFullPathName());
        });
    });
}

void MainComponent::createAndArmAudioTrack()
{
    const auto trackNumber = project.getTrackCount() + 1;
    const auto trackId = project.addEmptyAudioTrack ("Audio " + juce::String (trackNumber));
    if (trackId.isNotEmpty())
    {
        project.setTrackRecordArmed (trackId, true);
        audioStatusLabel.setText ("Audio " + juce::String (trackNumber)
                                      + " armed for recording",
                                  juce::dontSendNotification);
    }
}

void MainComponent::createInstrumentTrack()
{
    const auto number = project.getTrackCount() + 1;
    const auto trackId = project.addInstrumentTrack (
        "Triumph Instrument " + juce::String (number));
    if (trackId.isNotEmpty())
    {
        project.setTrackRecordArmed (trackId, true);
        showPianoRoll (trackId);
    }
}

void MainComponent::requestTrackDeletion (const juce::String& trackId)
{
    if (! project.hasTrack (trackId))
        return;

    if (isAnyRecording())
    {
        showInformation ("Recording in progress",
                         "Stop recording before deleting a track.");
        return;
    }

    if (advancedEditInProgress.load())
    {
        showInformation ("Audio processing",
                         "Wait for audio processing to finish before deleting a track.");
        return;
    }

    const auto trackName = project.getTrackState (trackId).name;
    const auto safe = juce::Component::SafePointer<MainComponent> (this);
    juce::AlertWindow::showOkCancelBox (
        juce::AlertWindow::WarningIcon,
        "Delete track?",
        "Delete \"" + trackName
            + "\" and all clips on it?\n\nThe source media files will remain on disk, and Undo can restore the track.",
        "Delete Track", "Cancel", this,
        juce::ModalCallbackFunction::create ([safe, trackId, trackName] (int result)
        {
            if (safe == nullptr || result == 0 || ! safe->project.hasTrack (trackId))
                return;

            if (safe->project.removeTrack (trackId))
                safe->audioStatusLabel.setText (
                    "Deleted " + trackName + " - use Undo to restore",
                    juce::dontSendNotification);
        }));
}

void MainComponent::showPianoRoll (const juce::String& requestedTrackId)
{
    const auto trackId = requestedTrackId.isNotEmpty()
                             ? requestedTrackId
                             : project.getFirstInstrumentTrackId();
    if (trackId.isEmpty())
    {
        showInformation ("No instrument track",
                         "Create a MIDI instrument track first.");
        return;
    }

    const auto track = project.getTrackState (trackId);
    if (track.midiClips.empty())
        return;

    const auto safe = juce::Component::SafePointer<MainComponent> (this);
    auto editor = std::make_unique<PianoRollComponent> (
        project, trackId, track.midiClips.front().id,
        [safe] (int noteNumber, float velocity)
        {
            if (safe == nullptr)
                return;

            const auto message = velocity > 0.0f
                ? juce::MidiMessage::noteOn (1, noteNumber, velocity)
                : juce::MidiMessage::noteOff (1, noteNumber);
            safe->audioEngine.pushLiveMidiMessage (message);
            safe->audioEngine.setLiveMidiNote (1, noteNumber, velocity);
        });
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (editor.release());
    options.dialogTitle = track.name + " - Piano Roll";
    options.dialogBackgroundColour = StudioColours::background;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.componentToCentreAround = this;
    options.launchAsync();
}

void MainComponent::showVst3Menu()
{
    enum class InsertMenuActionKind
    {
        load,
        open,
        toggleBypass,
        automate,
        remove
    };
    struct InsertMenuAction
    {
        InsertMenuActionKind kind = InsertMenuActionKind::load;
        juce::String ownerId;
        PluginSlotState slot;
        AudioEngine::PluginParameterDescriptor parameter;
    };
    std::vector<InsertMenuAction> allInsertActions;
    const auto addInsertAction = [&allInsertActions] (
        InsertMenuAction action)
    {
        const auto id = 60000 + static_cast<int> (allInsertActions.size());
        allInsertActions.push_back (std::move (action));
        return id;
    };
    const auto insertParameterInventory =
        audioEngine.getInsertPluginParameterInventory();
    const auto parametersForInsert = [&insertParameterInventory] (
        const juce::String& slotId)
    {
        const auto inventory = std::find_if (
            insertParameterInventory.begin(), insertParameterInventory.end(),
            [&slotId] (const auto& candidate)
        {
            return candidate.slotId == slotId;
        });
        return inventory != insertParameterInventory.end()
            ? inventory->parameters
            : std::vector<AudioEngine::PluginParameterDescriptor> {};
    };
    juce::PopupMenu menu;
    menu.addItem (1, "Load VST3 Instrument...");
    const auto insertOwnerId = preferredInsertOwnerId();
    const auto insertOwnerTrack = project.getTrackState (insertOwnerId);
    const auto insertOwnerChannel = project.getMixerChannelState (
        insertOwnerId);
    const auto insertOwnerName = insertOwnerTrack.id.isNotEmpty()
        ? insertOwnerTrack.name
        : insertOwnerChannel.id.isNotEmpty() ? insertOwnerChannel.name
                                             : juce::String();
    menu.addItem (12, insertOwnerName.isNotEmpty()
                          ? "Load VST3 Audio Effect on " + insertOwnerName + "..."
                          : "Load VST3 Audio Effect...",
                  insertOwnerId.isNotEmpty()
                      && ! pluginScanInProgress.load());
    menu.addItem (9, "Scan Standard VST3 Folders...",
                  ! pluginScanInProgress.load());
    const auto registryEntries = PluginScanService::loadRegistry (
        workspace.getPluginRegistryFile());
    const auto registrySummary = PluginScanService::summarizeRegistry (
        registryEntries);
    juce::PopupMenu registryMenu;
    auto blockedEntryCount = 0;
    for (std::size_t index = 0; index < registryEntries.size(); ++index)
    {
        const auto& entry = registryEntries[index];
        const auto blocked = entry.status == "blocked";
        if (blocked)
            ++blockedEntryCount;
        auto displayName = entry.displayName.isNotEmpty()
                               ? entry.displayName
                               : juce::File (entry.path).getFileName();
        if (entry.version.isNotEmpty())
            displayName += " " + entry.version;
        auto label = juce::String (! entry.fileAvailable ? "Missing: "
                                   : blocked ? "Blocked: " : "Validated: ")
                     + displayName;
        juce::PopupMenu entryMenu;
        entryMenu.addItem (1000 + static_cast<int> (index),
                           blocked ? "Retry as Instrument"
                                   : "Load as Instrument",
                           entry.fileAvailable && ! pluginScanInProgress.load());
        entryMenu.addItem (2000 + static_cast<int> (index),
                           insertOwnerName.isNotEmpty()
                               ? "Load as Audio Effect on " + insertOwnerName
                               : "Load as Audio Effect",
                           entry.fileAvailable && insertOwnerId.isNotEmpty()
                               && ! pluginScanInProgress.load());
        entryMenu.addSeparator();
        entryMenu.addItem (3000 + static_cast<int> (index),
                           "Forget This Registry Record");
        if (entry.error.isNotEmpty())
            entryMenu.addItem (
                3500 + static_cast<int> (index),
                "Last Error: "
                    + entry.error.upToFirstOccurrenceOf (
                        "\n", false, false).substring (0, 72),
                false);
        registryMenu.addSubMenu (label, entryMenu);
    }
    if (! registryEntries.empty())
        registryMenu.addSeparator();
    registryMenu.addItem (13, "Copy Plug-in Registry Report",
                          ! registryEntries.empty());
    registryMenu.addItem (8, "Forget blocked scan records",
                          blockedEntryCount > 0);
    menu.addSubMenu (
        "Plug-in Registry ("
            + juce::String (registrySummary.validated) + " ok, "
            + juce::String (registrySummary.blocked) + " blocked, "
            + juce::String (registrySummary.missing) + " missing)",
        registryMenu, ! registryEntries.empty());
    menu.addSeparator();
    const auto loaded = audioEngine.hasInstrumentPlugin();
    const auto name = loaded ? audioEngine.getInstrumentPluginName() : juce::String();
    const auto liveTrackId = loaded ? audioEngine.getInstrumentPluginTrackId()
                                    : juce::String();
    std::vector<TrackState> frozenTracks;
    TrackState pluginTrack;
    for (int index = 0; index < project.getTrackCount(); ++index)
    {
        const auto candidate = project.getTrackState (project.getTrackId (index));
        if (candidate.frozen)
            frozenTracks.push_back (candidate);
        else if (candidate.pluginDescriptionXml.isNotEmpty()
                 && (pluginTrack.id.isEmpty() || candidate.id == liveTrackId))
            pluginTrack = candidate;
    }
    menu.addItem (2, loaded ? "Open " + name : "No VST3 instrument loaded", loaded);
    menu.addItem (4, "Freeze Instrument to Audio", loaded && ! exportInProgress.load());
    if (frozenTracks.size() <= 1)
        menu.addItem (5, "Unfreeze Instrument", ! frozenTracks.empty()
                                                 && ! exportInProgress.load());
    else
    {
        juce::PopupMenu frozenMenu;
        for (std::size_t index = 0; index < frozenTracks.size(); ++index)
            frozenMenu.addItem (100 + static_cast<int> (index),
                                frozenTracks[index].name);
        menu.addSubMenu ("Unfreeze Instrument", frozenMenu,
                         ! exportInProgress.load());
    }
    menu.addItem (6, pluginTrack.pluginBypassed ? "Enable Instrument Plug-in"
                                               : "Bypass Instrument Plug-in",
                  pluginTrack.id.isNotEmpty() && ! pluginTrack.frozen);
    menu.addItem (7, "Retry Unavailable Plug-in",
                  pluginTrack.id.isNotEmpty() && ! pluginTrack.pluginAvailable
                      && ! pluginTrack.frozen);
    menu.addItem (10, "Save Instrument Preset...", loaded);
    menu.addItem (11, "Load Instrument Preset...", loaded);
    const auto pluginParameters = loaded
        ? audioEngine.getInstrumentPluginParameters()
        : std::vector<AudioEngine::PluginParameterDescriptor> {};
    juce::PopupMenu parameterMenu;
    auto automatableParameterCount = 0;
    for (std::size_t index = 0; index < pluginParameters.size(); ++index)
    {
        const auto& parameter = pluginParameters[index];
        if (! parameter.automatable)
            continue;
        ++automatableParameterCount;
        auto label = parameter.name.isNotEmpty()
                         ? parameter.name : parameter.stableId;
        if (parameter.label.isNotEmpty())
            label += " (" + parameter.label + ")";
        parameterMenu.addItem (20000 + static_cast<int> (index), label);
    }
    menu.addSubMenu (
        "Automate Plug-in Parameter ("
            + juce::String (automatableParameterCount) + ")",
        parameterMenu, loaded && automatableParameterCount > 0);
    menu.addItem (3, "Remove VST3 Instrument", pluginTrack.id.isNotEmpty());

    std::vector<PluginSlotState> insertSlots;
    if (insertOwnerId.isNotEmpty())
        for (const auto& slot : project.getPluginSlots (insertOwnerId))
            if (slot.role == PluginSlotRole::insertEffect)
                insertSlots.push_back (slot);
    std::vector<std::vector<AudioEngine::PluginParameterDescriptor>>
        insertParameters;
    insertParameters.reserve (insertSlots.size());
    juce::PopupMenu insertMenu;
    for (std::size_t slotIndex = 0; slotIndex < insertSlots.size(); ++slotIndex)
    {
        const auto& slot = insertSlots[slotIndex];
        auto parameters = parametersForInsert (slot.id);
        juce::PopupMenu slotMenu;
        slotMenu.addItem (40000 + static_cast<int> (slotIndex),
                          "Open Editor", slot.available);
        slotMenu.addItem (41000 + static_cast<int> (slotIndex),
                          slot.bypassed ? "Enable" : "Bypass");
        juce::PopupMenu insertAutomationMenu;
        auto addedParameters = 0;
        for (std::size_t parameterIndex = 0;
             parameterIndex < parameters.size() && parameterIndex < 1000;
             ++parameterIndex)
        {
            const auto& parameter = parameters[parameterIndex];
            if (! parameter.automatable)
                continue;
            insertAutomationMenu.addItem (
                50000 + static_cast<int> (slotIndex) * 1000
                    + static_cast<int> (parameterIndex),
                parameter.name.isNotEmpty() ? parameter.name
                                            : parameter.stableId);
            ++addedParameters;
        }
        slotMenu.addSubMenu ("Automate Parameter ("
                                 + juce::String (addedParameters) + ")",
                             insertAutomationMenu,
                             slot.available && addedParameters > 0);
        slotMenu.addSeparator();
        slotMenu.addItem (42000 + static_cast<int> (slotIndex), "Remove");
        insertMenu.addSubMenu (
            juce::String (slotIndex + 1) + ". " + slot.name
                + (slot.bypassed ? " [bypassed]" : juce::String()),
            slotMenu);
        insertParameters.push_back (std::move (parameters));
    }
    menu.addSubMenu (
        "Audio Effects (" + juce::String (insertSlots.size()) + ")",
        insertMenu, ! insertSlots.empty());

    std::vector<std::pair<juce::String, juce::String>> insertOwners;
    for (int index = 0; index < project.getTrackCount(); ++index)
    {
        const auto track = project.getTrackState (project.getTrackId (index));
        insertOwners.emplace_back (track.id, track.name);
    }
    for (const auto& channel : project.getMixerChannels())
        insertOwners.emplace_back (channel.id, channel.name);
    juce::PopupMenu allChannelsMenu;
    for (const auto& [ownerId, ownerName] : insertOwners)
    {
        juce::PopupMenu ownerMenu;
        ownerMenu.addItem (addInsertAction ({ InsertMenuActionKind::load,
                                              ownerId, {}, {} }),
                           "Load Audio Effect...");
        auto effectCount = 0;
        for (const auto& slot : project.getPluginSlots (ownerId))
        {
            if (slot.role != PluginSlotRole::insertEffect)
                continue;
            ++effectCount;
            juce::PopupMenu effectMenu;
            effectMenu.addItem (
                addInsertAction ({ InsertMenuActionKind::open, ownerId,
                                   slot, {} }),
                "Open Editor", slot.available);
            effectMenu.addItem (
                addInsertAction ({ InsertMenuActionKind::toggleBypass,
                                   ownerId, slot, {} }),
                slot.bypassed ? "Enable" : "Bypass");
            juce::PopupMenu automationMenu;
            auto automatable = 0;
            for (const auto& parameter : parametersForInsert (slot.id))
            {
                if (! parameter.automatable)
                    continue;
                ++automatable;
                automationMenu.addItem (
                    addInsertAction ({ InsertMenuActionKind::automate,
                                       ownerId, slot, parameter }),
                    parameter.name.isNotEmpty() ? parameter.name
                                                : parameter.stableId);
            }
            effectMenu.addSubMenu (
                "Automate Parameter (" + juce::String (automatable) + ")",
                automationMenu, slot.available && automatable > 0);
            effectMenu.addSeparator();
            effectMenu.addItem (
                addInsertAction ({ InsertMenuActionKind::remove, ownerId,
                                   slot, {} }),
                "Remove");
            ownerMenu.addSubMenu (
                juce::String (effectCount) + ". " + slot.name,
                effectMenu);
        }
        allChannelsMenu.addSubMenu (
            ownerName + " (" + juce::String (effectCount) + ")",
            ownerMenu);
    }
    menu.addSubMenu ("Audio Effects by Channel", allChannelsMenu,
                     ! insertOwners.empty());
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&pluginButton),
                        [this, frozenTracks, pluginTrack,
                        registryEntries, pluginParameters, insertOwnerId,
                        insertSlots, insertParameters,
                        allInsertActions] (int result)
    {
        if (result == 1)
            chooseVst3Instrument();
        else if (result == 2)
            showInstrumentPluginEditor();
        else if (result == 3)
        {
            ++pluginLoadGeneration;
            pluginLoadInProgress = false;
            const auto trackId = pluginTrack.id;
            pluginEditorWindow.reset();
            audioEngine.unloadInstrumentPlugin();
            loadedPluginDescriptionXml.clear();
            failedPluginDescriptionXml.clear();
            project.clearInstrumentPlugin (trackId);
        }
        else if (result == 4)
            beginFreezeInstrumentTrack (audioEngine.getInstrumentPluginTrackId());
        else if (result == 5)
        {
            if (! frozenTracks.empty())
                unfreezeInstrumentTrack (frozenTracks.front().id);
        }
        else if (result == 6)
            project.setInstrumentPluginBypassed (
                pluginTrack.id, ! pluginTrack.pluginBypassed);
        else if (result == 7)
        {
            failedPluginDescriptionXml.clear();
            project.setInstrumentPluginHealth (
                pluginTrack.id, pluginTrack.pluginLatencySamples, true);
            synchroniseInstrumentPluginWithProject();
        }
        else if (result == 8)
        {
            const auto clearResult = PluginScanService::clearBlockedEntries (
                workspace.getPluginRegistryFile());
            if (clearResult.failed())
                showError ("Plug-in registry could not be updated",
                           clearResult.getErrorMessage());
            else
                audioStatusLabel.setText (
                    "Blocked plug-in scan records forgotten",
                    juce::dontSendNotification);
        }
        else if (result == 9)
            beginStandardVst3Scan();
        else if (result == 10)
            saveInstrumentPluginPreset();
        else if (result == 11)
            loadInstrumentPluginPreset();
        else if (result == 12)
            chooseVst3Effect (insertOwnerId);
        else if (result == 13)
        {
            juce::SystemClipboard::copyTextToClipboard (
                PluginScanService::buildRegistryReport (registryEntries));
            audioStatusLabel.setText ("Plug-in registry report copied",
                                      juce::dontSendNotification);
        }
        else if (result >= 40000 && result < 41000)
        {
            const auto index = result - 40000;
            if (juce::isPositiveAndBelow (
                    index, static_cast<int> (insertSlots.size())))
                showInsertPluginEditor (
                    insertSlots[static_cast<std::size_t> (index)].id,
                    insertSlots[static_cast<std::size_t> (index)].name);
        }
        else if (result >= 41000 && result < 42000)
        {
            const auto index = result - 41000;
            if (juce::isPositiveAndBelow (
                    index, static_cast<int> (insertSlots.size())))
            {
                auto slot = insertSlots[static_cast<std::size_t> (index)];
                slot.bypassed = ! slot.bypassed;
                project.updatePluginSlot (insertOwnerId, slot);
            }
        }
        else if (result >= 42000 && result < 43000)
        {
            const auto index = result - 42000;
            if (juce::isPositiveAndBelow (
                    index, static_cast<int> (insertSlots.size())))
            {
                const auto& slot = insertSlots[
                    static_cast<std::size_t> (index)];
                pluginEditorWindow.reset();
                audioEngine.unloadInsertPlugin (slot.id);
                project.removePluginSlot (insertOwnerId, slot.id);
            }
        }
        else if (result >= 60000
                 && result - 60000
                        < static_cast<int> (allInsertActions.size()))
        {
            const auto& action = allInsertActions[
                static_cast<std::size_t> (result - 60000)];
            if (action.kind == InsertMenuActionKind::load)
                chooseVst3Effect (action.ownerId);
            else if (action.kind == InsertMenuActionKind::open)
                showInsertPluginEditor (action.slot.id, action.slot.name);
            else if (action.kind == InsertMenuActionKind::toggleBypass)
            {
                auto slot = action.slot;
                slot.bypassed = ! slot.bypassed;
                project.updatePluginSlot (action.ownerId, slot);
            }
            else if (action.kind == InsertMenuActionKind::remove)
            {
                pluginEditorWindow.reset();
                audioEngine.unloadInsertPlugin (action.slot.id);
                project.removePluginSlot (action.ownerId, action.slot.id);
            }
            else if (action.kind == InsertMenuActionKind::automate)
            {
                const auto qualifiedParameter = "plugin:" + action.slot.id
                    + ":" + action.parameter.stableId;
                const auto laneId = project.addAutomationLane (
                    action.ownerId, qualifiedParameter,
                    AutomationMode::read);
                const auto sample = static_cast<juce::int64> (std::llround (
                    audioEngine.getPositionSeconds()
                        * project.getTimelineSampleRate()));
                project.addAutomationPoint (
                    laneId, sample, action.parameter.currentValue,
                    AutomationCurve::linear);
                if (! automationButton.getToggleState())
                    automationButton.triggerClick();
                audioStatusLabel.setText (
                    "Automation lane ready for " + action.slot.name + " - "
                        + action.parameter.name,
                    juce::dontSendNotification);
            }
        }
        else if (result >= 50000 && result < 60000)
        {
            const auto packed = result - 50000;
            const auto slotIndex = packed / 1000;
            const auto parameterIndex = packed % 1000;
            if (! juce::isPositiveAndBelow (
                    slotIndex, static_cast<int> (insertSlots.size()))
                || ! juce::isPositiveAndBelow (
                    parameterIndex,
                    static_cast<int> (insertParameters[
                        static_cast<std::size_t> (slotIndex)].size())))
                return;
            const auto& slot = insertSlots[
                static_cast<std::size_t> (slotIndex)];
            const auto& parameter = insertParameters[
                static_cast<std::size_t> (slotIndex)][
                    static_cast<std::size_t> (parameterIndex)];
            const auto qualifiedParameter = "plugin:" + slot.id + ":"
                + parameter.stableId;
            const auto laneId = project.addAutomationLane (
                insertOwnerId, qualifiedParameter, AutomationMode::read);
            const auto sample = static_cast<juce::int64> (std::llround (
                audioEngine.getPositionSeconds()
                    * project.getTimelineSampleRate()));
            project.addAutomationPoint (
                laneId, sample, parameter.currentValue,
                AutomationCurve::linear);
            if (! automationButton.getToggleState())
                automationButton.triggerClick();
            audioStatusLabel.setText (
                "Automation lane ready for " + slot.name + " - "
                    + parameter.name,
                juce::dontSendNotification);
        }
        else if (result >= 20000
                 && result - 20000 < static_cast<int> (
                        pluginParameters.size()))
        {
            const auto& parameter = pluginParameters[
                static_cast<std::size_t> (result - 20000)];
            if (! parameter.automatable || pluginTrack.id.isEmpty())
                return;
            const auto laneId = project.addAutomationLane (
                pluginTrack.id, parameter.stableId, AutomationMode::read);
            const auto sample = static_cast<juce::int64> (std::llround (
                audioEngine.getPositionSeconds()
                    * project.getTimelineSampleRate()));
            project.addAutomationPoint (
                laneId, sample, parameter.currentValue,
                AutomationCurve::linear);
            if (! automationButton.getToggleState())
                automationButton.triggerClick();
            audioStatusLabel.setText (
                "Automation lane ready for " + parameter.name,
                juce::dontSendNotification);
        }
        else if (result >= 3000
                 && result - 3000 < static_cast<int> (
                        registryEntries.size()))
        {
            const auto pluginFile = juce::File (
                registryEntries[static_cast<std::size_t> (
                    result - 3000)].path);
            const auto forgetResult = PluginScanService::forgetRegistryEntry (
                workspace.getPluginRegistryFile(), pluginFile);
            if (forgetResult.failed())
                showError ("Plug-in registry could not be updated",
                           forgetResult.getErrorMessage());
            else
                audioStatusLabel.setText (
                    "Plug-in registry record forgotten: "
                        + pluginFile.getFileName(),
                    juce::dontSendNotification);
        }
        else if (result >= 2000
                 && result - 2000 < static_cast<int> (
                        registryEntries.size()))
        {
            const auto pluginFile = juce::File (
                registryEntries[static_cast<std::size_t> (
                    result - 2000)].path);
            if (! pluginFile.exists())
                showError ("Registered VST3 is missing",
                           pluginFile.getFullPathName());
            else
                loadVst3Effect (pluginFile, insertOwnerId);
        }
        else if (result >= 1000
                 && result - 1000 < static_cast<int> (
                        registryEntries.size()))
        {
            const auto pluginFile = juce::File (
                registryEntries[static_cast<std::size_t> (
                    result - 1000)].path);
            if (! pluginFile.exists())
                showError ("Registered VST3 is missing",
                           pluginFile.getFullPathName());
            else
                loadVst3Instrument (pluginFile);
        }
        else if (result >= 100
                 && result - 100 < static_cast<int> (frozenTracks.size()))
            unfreezeInstrumentTrack (
                frozenTracks[static_cast<std::size_t> (result - 100)].id);
    });
}

void MainComponent::saveInstrumentPluginPreset()
{
    const auto trackId = audioEngine.getInstrumentPluginTrackId();
    const auto track = project.getTrackState (trackId);
    const auto xml = juce::parseXML (track.pluginDescriptionXml);
    juce::PluginDescription description;
    if (xml == nullptr || ! description.loadFromXml (*xml))
    {
        showError ("Preset could not be saved",
                   "The live plug-in identity is unavailable.");
        return;
    }

    const auto initial = juce::File::getSpecialLocation (
        juce::File::userDocumentsDirectory).getChildFile (
            description.name + ".triumphpreset");
    fileChooser = std::make_unique<juce::FileChooser> (
        "Save Triumph instrument preset", initial, "*.triumphpreset");
    fileChooser->launchAsync (
        juce::FileBrowserComponent::saveMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::warnAboutOverwriting,
        [this, stableId = description.createIdentifierString(),
         pluginName = description.name] (const juce::FileChooser& chooser)
        {
            auto destination = chooser.getResult();
            if (destination == juce::File())
                return;
            if (! destination.hasFileExtension ("triumphpreset"))
                destination = destination.withFileExtension (
                    ".triumphpreset");
            const auto saveResult = PluginPresetStore::save (
                destination, stableId, pluginName,
                audioEngine.captureInstrumentPluginStateBase64());
            if (saveResult.failed())
                showError ("Preset could not be saved",
                           saveResult.getErrorMessage());
            else
                audioStatusLabel.setText (
                    "Instrument preset saved: " + destination.getFileName(),
                    juce::dontSendNotification);
        });
}

void MainComponent::loadInstrumentPluginPreset()
{
    const auto trackId = audioEngine.getInstrumentPluginTrackId();
    const auto track = project.getTrackState (trackId);
    const auto xml = juce::parseXML (track.pluginDescriptionXml);
    juce::PluginDescription description;
    if (xml == nullptr || ! description.loadFromXml (*xml))
    {
        showError ("Preset could not be loaded",
                   "The live plug-in identity is unavailable.");
        return;
    }

    fileChooser = std::make_unique<juce::FileChooser> (
        "Load Triumph instrument preset",
        juce::File::getSpecialLocation (
            juce::File::userDocumentsDirectory),
        "*.triumphpreset");
    fileChooser->launchAsync (
        juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles,
        [this, trackId,
         stableId = description.createIdentifierString()]
        (const juce::FileChooser& chooser)
        {
            const auto source = chooser.getResult();
            if (source == juce::File())
                return;
            PluginPresetStore::Preset preset;
            const auto loadResult = PluginPresetStore::load (
                source, stableId, preset);
            if (loadResult.failed())
            {
                showError ("Preset could not be loaded",
                           loadResult.getErrorMessage());
                return;
            }
            const auto applyResult =
                audioEngine.applyInstrumentPluginStateBase64 (
                    preset.stateBase64);
            if (applyResult.failed())
            {
                showError ("Preset could not be loaded",
                           applyResult.getErrorMessage());
                return;
            }
            project.setInstrumentPluginState (trackId, preset.stateBase64);
            audioStatusLabel.setText (
                "Instrument preset loaded: " + source.getFileName(),
                juce::dontSendNotification);
        });
}

juce::File MainComponent::requirePluginScannerHelper()
{
    const auto installResult = PluginScanService::ensureHelperBesideApplication();
    const auto helper = PluginScanService::helperExecutable();
    if (helper.existsAsFile())
        return helper;

    audioStatusLabel.setText ("VST3 scanner helper is missing",
                              juce::dontSendNotification);
    const auto message =
        juce::String ("Triumph cannot safely validate VST3 plug-ins until the "
                      "crash-isolated scanner is available.\n\n")
        + (installResult.failed()
               ? "Automatic scanner repair failed: "
                     + installResult.getErrorMessage() + "\n\n"
               : juce::String {})
        + PluginScanService::helperSearchDiagnostic()
        + "\n\nRebuild the project or copy TriumphPluginScanner.exe beside "
          "Triumph Studio.exe.";
    showError ("VST3 scanner missing",
               message);
    return {};
}

void MainComponent::beginStandardVst3Scan()
{
    if (pluginScanInProgress.exchange (true))
    {
        audioStatusLabel.setText ("A plug-in scan is already running",
                                  juce::dontSendNotification);
        return;
    }

    if (pluginScanThread.joinable())
        pluginScanThread.join();

    const auto helper = requirePluginScannerHelper();
    if (helper.getFullPathName().isEmpty())
    {
        pluginScanInProgress.store (false);
        return;
    }

    const auto directories = PluginScanService::defaultVst3Directories();
    if (directories.empty())
    {
        pluginScanInProgress.store (false);
        showInformation ("No standard VST3 folders",
                         "No installed standard VST3 folder was found.");
        return;
    }

    audioStatusLabel.setText ("Discovering installed VST3 plug-ins...",
                              juce::dontSendNotification);
    const auto registryFile = workspace.getPluginRegistryFile();
    juce::Component::SafePointer<MainComponent> safeThis (this);
    pluginScanThread = std::thread (
        [safeThis, directories, helper, registryFile]
        {
            const auto candidates =
                PluginScanService::discoverVst3Candidates (directories);
            const auto registry = PluginScanService::loadRegistry (
                registryFile);
            auto scanned = 0;
            auto skipped = 0;
            auto validated = 0;
            auto blocked = 0;
            auto registryFailures = 0;

            for (std::size_t index = 0; index < candidates.size(); ++index)
            {
                const auto& candidate = candidates[index];
                const auto* existing = PluginScanService::findEntry (
                    registry, candidate);
                if (! PluginScanService::shouldScan (
                        candidate, existing, false))
                {
                    ++skipped;
                    continue;
                }

                ++scanned;
                auto result = PluginScanService::scanVst3 (
                    candidate, helper, 30000);
                if (result.succeeded())
                    ++validated;
                else
                    ++blocked;
                if (PluginScanService::updateRegistry (
                        registryFile, candidate, result).failed())
                    ++registryFailures;

                const auto progress = "Scanning VST3 "
                    + juce::String (static_cast<int> (index + 1)) + "/"
                    + juce::String (static_cast<int> (candidates.size()))
                    + ": " + candidate.getFileName();
                juce::MessageManager::callAsync (
                    [safeThis, progress]
                    {
                        if (safeThis != nullptr)
                            safeThis->audioStatusLabel.setText (
                                progress, juce::dontSendNotification);
                    });
            }

            const auto summary = candidates.empty()
                ? juce::String ("No VST3 plug-ins were found")
                : "VST3 discovery complete: "
                    + juce::String (validated) + " validated, "
                    + juce::String (blocked) + " blocked, "
                    + juce::String (skipped) + " unchanged, "
                    + juce::String (scanned) + " scanned"
                    + (registryFailures > 0
                           ? ", " + juce::String (registryFailures)
                                 + " registry write failures"
                           : juce::String());
            juce::MessageManager::callAsync (
                [safeThis, summary]
                {
                    if (safeThis == nullptr)
                        return;
                    safeThis->pluginScanInProgress.store (false);
                    safeThis->audioStatusLabel.setText (
                        summary, juce::dontSendNotification);
                });
        });
}

void MainComponent::chooseVst3Instrument()
{
    const auto commonFiles = juce::SystemStats::getEnvironmentVariable (
        "CommonProgramFiles", "C:\\Program Files\\Common Files");
    auto initial = juce::File (commonFiles).getChildFile ("VST3");
    if (! initial.isDirectory())
        initial = juce::File::getSpecialLocation (juce::File::globalApplicationsDirectory);

    fileChooser = std::make_unique<juce::FileChooser> (
        "Load a VST3 instrument", initial, "*.vst3");
    fileChooser->launchAsync (
        juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::canSelectDirectories,
        [this] (const juce::FileChooser& chooser)
        {
            const auto selected = chooser.getResult();
            if (selected != juce::File())
                loadVst3Instrument (selected);
        });
}

void MainComponent::chooseVst3Effect (juce::String ownerId)
{
    if (ownerId.isEmpty())
    {
        showInformation ("Audio Effect",
                         "Select or arm a track before adding an audio effect.");
        return;
    }
    const auto commonFiles = juce::SystemStats::getEnvironmentVariable (
        "CommonProgramFiles", "C:\\Program Files\\Common Files");
    auto initial = juce::File (commonFiles).getChildFile ("VST3");
    if (! initial.isDirectory())
        initial = juce::File::getSpecialLocation (
            juce::File::globalApplicationsDirectory);
    fileChooser = std::make_unique<juce::FileChooser> (
        "Load a VST3 audio effect", initial, "*.vst3");
    fileChooser->launchAsync (
        juce::FileBrowserComponent::openMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::canSelectDirectories,
        [this, ownerId = std::move (ownerId)]
        (const juce::FileChooser& chooser)
        {
            const auto selected = chooser.getResult();
            if (selected != juce::File())
                loadVst3Effect (selected, ownerId);
        });
}

void MainComponent::loadVst3Instrument (const juce::File& pluginFile)
{
    if (pluginScanInProgress.exchange (true))
    {
        audioStatusLabel.setText ("A plug-in scan is already running",
                                  juce::dontSendNotification);
        return;
    }

    if (pluginScanThread.joinable())
        pluginScanThread.join();

    const auto helper = requirePluginScannerHelper();
    if (helper.getFullPathName().isEmpty())
    {
        pluginScanInProgress.store (false);
        return;
    }

    audioStatusLabel.setText ("Validating " + pluginFile.getFileName() + "...",
                              juce::dontSendNotification);
    juce::Component::SafePointer<MainComponent> safeThis (this);
    const auto registryFile = workspace.getPluginRegistryFile();
    pluginScanThread = std::thread (
        [safeThis, pluginFile, helper, registryFile]
        {
            auto result = PluginScanService::scanVst3 (
                pluginFile, helper, 30000);
            const auto registryResult = PluginScanService::updateRegistry (
                registryFile, pluginFile, result);
            if (registryResult.failed())
            {
                const auto registryError = "Plug-in registry update failed: "
                                           + registryResult.getErrorMessage();
                result.error = result.error.isNotEmpty()
                                   ? result.error + "\n\n" + registryError
                                   : registryError;
            }

            juce::MessageManager::callAsync (
                [safeThis, result = std::move (result)]
                {
                    if (safeThis == nullptr)
                        return;
                    safeThis->pluginScanInProgress.store (false);
                    safeThis->completeVst3InstrumentScan (result);
                });
        });
}

void MainComponent::loadVst3Effect (const juce::File& pluginFile,
                                    juce::String ownerId)
{
    if (pluginScanInProgress.exchange (true))
    {
        audioStatusLabel.setText ("A plug-in scan is already running",
                                  juce::dontSendNotification);
        return;
    }
    if (pluginScanThread.joinable())
        pluginScanThread.join();

    const auto helper = requirePluginScannerHelper();
    if (helper.getFullPathName().isEmpty())
    {
        pluginScanInProgress.store (false);
        return;
    }

    audioStatusLabel.setText (
        "Validating audio effect " + pluginFile.getFileName() + "...",
        juce::dontSendNotification);
    juce::Component::SafePointer<MainComponent> safeThis (this);
    const auto registryFile = workspace.getPluginRegistryFile();
    pluginScanThread = std::thread (
        [safeThis, pluginFile, helper, registryFile,
         ownerId = std::move (ownerId)]
        {
            auto result = PluginScanService::scanVst3 (
                pluginFile, helper, 30000);
            const auto registryResult = PluginScanService::updateRegistry (
                registryFile, pluginFile, result);
            if (registryResult.failed())
                result.error = result.error.isNotEmpty()
                    ? result.error + "\n\nPlug-in registry update failed: "
                        + registryResult.getErrorMessage()
                    : "Plug-in registry update failed: "
                        + registryResult.getErrorMessage();
            juce::MessageManager::callAsync (
                [safeThis, ownerId, result = std::move (result)]
                {
                    if (safeThis == nullptr)
                        return;
                    safeThis->pluginScanInProgress.store (false);
                    safeThis->completeVst3EffectScan (result, ownerId);
                });
        });
}

void MainComponent::completeVst3InstrumentScan (
    const PluginScanService::Result& scanResult)
{
    const auto& descriptions = scanResult.descriptions;
    const auto match = std::find_if (descriptions.begin(), descriptions.end(),
                                     [] (const auto& description)
    {
        return description.isInstrument;
    });
    if (match == descriptions.end())
    {
        showError ("VST3 instrument could not be loaded",
                   scanResult.error.isNotEmpty()
                       ? scanResult.error
                       : "The selected VST3 contains no instrument type.");
        return;
    }

    auto targetTrackId = project.getArmedTrackId();
    if (targetTrackId.isEmpty()
        || ! project.getTrackState (targetTrackId).isInstrument)
        targetTrackId = project.getFirstInstrumentTrackId();
    if (targetTrackId.isEmpty())
    {
        targetTrackId = project.addInstrumentTrack (match->name);
        project.setTrackRecordArmed (targetTrackId, true);
    }

    const auto description = *match;
    const auto xml = description.createXml();
    if (xml == nullptr)
    {
        showError ("VST3 instrument could not be loaded",
                   "The plug-in description could not be serialized.");
        return;
    }
    const auto descriptionXml = xml->toString();
    pluginEditorWindow.reset();
    pluginLoadInProgress = true;
    const auto generation = ++pluginLoadGeneration;
    juce::Component::SafePointer<MainComponent> safeThis (this);
    audioEngine.loadInstrumentPlugin (
        targetTrackId, description, {},
        [safeThis, targetTrackId, descriptionXml, name = description.name,
         generation]
        (bool succeeded, juce::String message)
        {
            if (safeThis == nullptr)
                return;
            if (generation != safeThis->pluginLoadGeneration)
                return;
            safeThis->pluginLoadInProgress = false;
            if (! succeeded)
            {
                safeThis->showError ("VST3 instrument could not be loaded", message);
                return;
            }
            safeThis->loadedPluginDescriptionXml = descriptionXml;
            safeThis->failedPluginDescriptionXml.clear();
            safeThis->project.setInstrumentPlugin (
                targetTrackId, name, descriptionXml,
                safeThis->audioEngine.captureInstrumentPluginStateBase64());
            safeThis->project.setInstrumentPluginHealth (
                targetTrackId,
                safeThis->audioEngine.getInstrumentPluginLatencySamples(), true);
            safeThis->audioStatusLabel.setText (
                name + " loaded on the instrument track",
                juce::dontSendNotification);
            safeThis->showInstrumentPluginEditor();
        });
}

void MainComponent::completeVst3EffectScan (
    const PluginScanService::Result& scanResult,
    const juce::String& ownerId)
{
    const auto match = std::find_if (
        scanResult.descriptions.begin(), scanResult.descriptions.end(),
        [] (const auto& description)
    {
        return ! description.isInstrument
            && description.numInputChannels > 0
            && description.numOutputChannels > 0;
    });
    if (match == scanResult.descriptions.end())
    {
        showError ("VST3 audio effect could not be loaded",
                   scanResult.error.isNotEmpty()
                       ? scanResult.error
                       : "The selected VST3 contains no supported audio effect.");
        return;
    }
    if (project.getTrackState (ownerId).id.isEmpty()
        && project.getMixerChannelState (ownerId).id.isEmpty())
    {
        showError ("VST3 audio effect could not be loaded",
                   "The destination track or mixer channel no longer exists.");
        return;
    }
    const auto xml = match->createXml();
    if (xml == nullptr)
    {
        showError ("VST3 audio effect could not be loaded",
                   "The plug-in description could not be serialized.");
        return;
    }
    PluginSlotState slot;
    slot.name = match->name;
    slot.descriptionXml = xml->toString();
    slot.role = PluginSlotRole::insertEffect;
    slot.isolation = PluginIsolationMode::trustedInProcess;
    slot.layout.inputChannels = juce::jlimit (
        1, 2, match->numInputChannels);
    slot.layout.outputChannels = juce::jlimit (
        1, 2, match->numOutputChannels);
    slot.layout.explicitLayout = true;
    for (const auto& existing : project.getPluginSlots (ownerId))
        slot.order = juce::jmax (slot.order, existing.order + 1);
    slot.order = juce::jlimit (0, 15, slot.order);
    const auto slotId = project.addPluginSlot (ownerId, slot);
    if (slotId.isEmpty())
    {
        showError ("VST3 audio effect could not be loaded",
                   "The channel has no available insert slot.");
        return;
    }
    audioStatusLabel.setText (
        match->name + " added to the contained multi-insert rack",
        juce::dontSendNotification);
    synchroniseInsertPluginsWithProject();
}

void MainComponent::showInstrumentPluginEditor()
{
    if (pluginEditorWindow != nullptr)
    {
        pluginEditorWindow->setVisible (true);
        pluginEditorWindow->toFront (true);
        return;
    }

    auto* editor = audioEngine.getInstrumentPluginEditor();
    if (editor == nullptr)
    {
        showInformation ("Plug-in editor unavailable",
                         "The loaded instrument does not provide a custom editor.");
        return;
    }
    pluginEditorWindow = std::make_unique<PluginEditorWindow> (
        audioEngine.getInstrumentPluginName(), *editor);
    pluginEditorWindow->setVisible (true);
    setDirty (true);
}

void MainComponent::showInsertPluginEditor (
    const juce::String& slotId, const juce::String& name)
{
    pluginEditorWindow.reset();
    auto* editor = audioEngine.getInsertPluginEditor (slotId);
    if (editor == nullptr)
    {
        showError ("Audio Effect Editor",
                   "The effect is not loaded or does not provide an editor.");
        return;
    }
    pluginEditorWindow = std::make_unique<PluginEditorWindow> (
        name + " - Audio Effect", *editor);
    pluginEditorWindow->setVisible (true);
}

void MainComponent::beginFreezeInstrumentTrack (const juce::String& trackId)
{
    if (exportInProgress.load() || trackId.isEmpty())
        return;
    captureInstrumentPluginState();
    const auto track = project.getTrackState (trackId);
    if (! track.isInstrument || track.pluginDescriptionXml.isEmpty())
    {
        showError ("Freeze Instrument", "No live VST3 instrument is available to freeze.");
        return;
    }
    const auto xml = juce::parseXML (track.pluginDescriptionXml);
    juce::PluginDescription description;
    if (xml == nullptr || ! description.loadFromXml (*xml))
    {
        project.setInstrumentPluginHealth (trackId, 0, false);
        showError ("Freeze Instrument", "The stored VST3 description is invalid.");
        return;
    }

    const auto sampleRate = project.getTimelineSampleRate();
    const auto freezeDirectory = currentProjectFile != juce::File()
        ? currentProjectFile.getParentDirectory().getChildFile ("Freeze")
        : juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
              .getChildFile ("Triumph Studio").getChildFile ("Freeze Cache")
              .getChildFile (project.getProjectId());
    if (freezeDirectory.createDirectory().failed())
    {
        showError ("Freeze Instrument", "The freeze cache folder could not be created.");
        return;
    }
    const auto destination = freezeDirectory.getChildFile (
        track.id + "-" + juce::Uuid().toString() + ".wav");
    exportCancelRequested.store (false);
    exportProgress.store (0.0f);
    exportInProgress.store (true);
    audioStatusLabel.setText ("Preparing " + track.name + " for freeze...",
                              juce::dontSendNotification);
    auto safe = juce::Component::SafePointer<MainComponent> (this);
    audioEngine.createOfflineInstrumentInstance (
        description, track.pluginStateBase64, sampleRate, 512,
        [safe, track, sampleRate, destination]
        (std::unique_ptr<juce::AudioPluginInstance> instance,
         juce::String error) mutable
        {
            if (safe == nullptr)
                return;
            if (instance == nullptr)
            {
                safe->exportInProgress.store (false);
                safe->project.setInstrumentPluginHealth (track.id, 0, false);
                safe->showError ("Freeze Instrument", error);
                return;
            }
            safe->project.setInstrumentPluginHealth (
                track.id, instance->getLatencySamples(), true);
            if (safe->exportThread.joinable())
                safe->exportThread.join();
            auto tempoPoints = safe->project.getTempoPoints();
            auto automationLanes = safe->project.getAutomationLanes();
            auto cancel = &safe->exportCancelRequested;
            auto progress = &safe->exportProgress;
            safe->audioStatusLabel.setText ("Freezing " + track.name + "...",
                                            juce::dontSendNotification);
            safe->exportThread = std::thread (
                [safe, plugin = std::move (instance), track, tempoPoints,
                 automationLanes,
                 sampleRate, destination, cancel, progress] () mutable
            {
                const auto result = PluginFreezeRenderer::render (
                    std::move (plugin), track, tempoPoints, automationLanes,
                    sampleRate,
                    destination, *cancel, *progress);
                juce::MessageManager::callAsync (
                    [safe, result, destination, track]
                {
                    if (safe == nullptr)
                        return;
                    safe->exportInProgress.store (false);
                    if (result.failed())
                    {
                        destination.deleteFile();
                        safe->showError ("Freeze Instrument", result.getErrorMessage());
                        return;
                    }
                    const auto currentTrack = safe->project.getTrackState (track.id);
                    if (currentTrack.frozen
                        || currentTrack.pluginDescriptionXml
                               != track.pluginDescriptionXml)
                    {
                        destination.deleteFile();
                        safe->showError (
                            "Freeze Instrument",
                            "The instrument changed while the freeze was rendering. "
                            "No stale audio was committed; start the freeze again.");
                        return;
                    }
                    const auto details = safe->audioEngine.inspectAudioFile (destination);
                    if (! details.isValid()
                        || ! safe->project.freezeInstrumentTrack (
                            track.id, destination, details.sampleRate,
                            details.channels, details.lengthInSamples))
                    {
                        destination.deleteFile();
                        safe->showError ("Freeze Instrument",
                                         "The rendered freeze could not be committed to the project.");
                        return;
                    }
                    safe->audioStatusLabel.setText (
                        track.name + " frozen with latency compensation",
                        juce::dontSendNotification);
                });
            });
        });
}

void MainComponent::unfreezeInstrumentTrack (const juce::String& trackId)
{
    const auto track = project.getTrackState (trackId);
    if (! track.frozen)
        return;
    if (! project.unfreezeInstrumentTrack (trackId))
    {
        showError ("Unfreeze Instrument", "The frozen track could not be restored.");
        return;
    }
    audioStatusLabel.setText (track.name + " restored to live VST3 playback",
                              juce::dontSendNotification);
}

void MainComponent::synchroniseInstrumentPluginWithProject()
{
    TrackState desired;
    for (int index = 0; index < project.getTrackCount(); ++index)
    {
        const auto candidate = project.getTrackState (project.getTrackId (index));
        if (candidate.isInstrument && ! candidate.frozen
            && candidate.pluginAvailable
            && candidate.pluginDescriptionXml.isNotEmpty())
        {
            desired = candidate;
            break;
        }
    }

    if (desired.id.isEmpty())
    {
        if (pluginLoadInProgress)
        {
            ++pluginLoadGeneration;
            pluginLoadInProgress = false;
        }
        if (audioEngine.hasInstrumentPlugin())
        {
            pluginEditorWindow.reset();
            audioEngine.unloadInstrumentPlugin();
        }
        loadedPluginDescriptionXml.clear();
        failedPluginDescriptionXml.clear();
        return;
    }

    audioEngine.setInstrumentPluginBypassed (desired.pluginBypassed);

    if (pluginLoadInProgress
        || (audioEngine.hasInstrumentPlugin()
            && audioEngine.getInstrumentPluginTrackId() == desired.id
            && loadedPluginDescriptionXml == desired.pluginDescriptionXml)
        || failedPluginDescriptionXml == desired.pluginDescriptionXml)
        return;

    const auto xml = juce::parseXML (desired.pluginDescriptionXml);
    juce::PluginDescription description;
    if (xml == nullptr || ! description.loadFromXml (*xml))
    {
        failedPluginDescriptionXml = desired.pluginDescriptionXml;
        project.setInstrumentPluginHealth (desired.id, 0, false);
        audioStatusLabel.setText ("Stored VST3 description is invalid",
                                  juce::dontSendNotification);
        return;
    }

    pluginEditorWindow.reset();
    pluginLoadInProgress = true;
    const auto generation = ++pluginLoadGeneration;
    juce::Component::SafePointer<MainComponent> safeThis (this);
    audioEngine.loadInstrumentPlugin (
        desired.id, description, desired.pluginStateBase64,
        [safeThis, trackId = desired.id,
         descriptionXml = desired.pluginDescriptionXml,
         name = desired.pluginName, generation]
        (bool succeeded, juce::String message)
        {
            if (safeThis == nullptr)
                return;
            if (generation != safeThis->pluginLoadGeneration)
                return;
            safeThis->pluginLoadInProgress = false;
            if (! succeeded)
            {
                safeThis->project.setInstrumentPluginHealth (trackId, 0, false);
                safeThis->failedPluginDescriptionXml = descriptionXml;
                safeThis->audioStatusLabel.setText (
                    "VST3 unavailable: " + message,
                    juce::dontSendNotification);
                return;
            }
            safeThis->loadedPluginDescriptionXml = descriptionXml;
            safeThis->failedPluginDescriptionXml.clear();
            safeThis->project.setInstrumentPluginHealth (
                trackId,
                safeThis->audioEngine.getInstrumentPluginLatencySamples(), true);
            safeThis->audioStatusLabel.setText (name + " restored",
                                                juce::dontSendNotification);
        });
}

void MainComponent::synchroniseInsertPluginsWithProject()
{
    struct DesiredInsert
    {
        juce::String ownerId;
        PluginSlotState slot;
    };
    std::vector<DesiredInsert> desired;
    for (int index = 0; index < project.getTrackCount(); ++index)
    {
        const auto ownerId = project.getTrackId (index);
        for (const auto& slot : project.getPluginSlots (ownerId))
            if (slot.role == PluginSlotRole::insertEffect)
                desired.push_back ({ ownerId, slot });
    }
    for (const auto& channel : project.getMixerChannels())
        for (const auto& slot : project.getPluginSlots (channel.id))
            if (slot.role == PluginSlotRole::insertEffect)
                desired.push_back ({ channel.id, slot });

    const auto loaded = audioEngine.getLoadedInsertPluginSlotIds();
    for (const auto& slotId : loaded)
    {
        const auto wanted = std::find_if (
            desired.begin(), desired.end(), [&slotId] (const auto& item)
        {
            return item.slot.id == slotId && item.slot.available
                && item.slot.isolation
                       == PluginIsolationMode::trustedInProcess;
        });
        if (wanted == desired.end()
            || audioEngine.getInsertPluginDescriptionXml (slotId)
                   != wanted->slot.descriptionXml)
            audioEngine.unloadInsertPlugin (slotId);
        else
            audioEngine.setInsertPluginBypassed (
                slotId, wanted->slot.bypassed);
    }

    if (insertPluginLoadInProgress.isNotEmpty())
        return;

    const auto refreshedLoaded = audioEngine.getLoadedInsertPluginSlotIds();
    const auto next = std::find_if (
        desired.begin(), desired.end(), [&refreshedLoaded] (const auto& item)
    {
        return item.slot.available
            && item.slot.isolation == PluginIsolationMode::trustedInProcess
            && item.slot.descriptionXml.isNotEmpty()
            && std::find (refreshedLoaded.begin(), refreshedLoaded.end(),
                          item.slot.id) == refreshedLoaded.end();
    });
    if (next == desired.end())
        return;

    const auto xml = juce::parseXML (next->slot.descriptionXml);
    juce::PluginDescription description;
    if (xml == nullptr || ! description.loadFromXml (*xml)
        || description.isInstrument)
    {
        auto failed = next->slot;
        failed.available = false;
        ++failed.restartCount;
        project.updatePluginSlot (next->ownerId, failed);
        return;
    }

    insertPluginLoadInProgress = next->slot.id;
    AudioEngine::InsertPluginLoad request;
    request.slotId = next->slot.id;
    request.ownerId = next->ownerId;
    request.order = next->slot.order;
    request.description = description;
    request.descriptionXml = next->slot.descriptionXml;
    request.stateBase64 = next->slot.stateBase64;
    request.bypassed = next->slot.bypassed;
    juce::Component::SafePointer<MainComponent> safeThis (this);
    audioEngine.loadInsertPlugin (
        std::move (request),
        [safeThis, ownerId = next->ownerId, slotId = next->slot.id]
        (bool succeeded, juce::String message, int latency)
        {
            if (safeThis == nullptr)
                return;
            safeThis->insertPluginLoadInProgress.clear();
            auto slots = safeThis->project.getPluginSlots (ownerId);
            const auto current = std::find_if (
                slots.begin(), slots.end(), [&slotId] (const auto& slot)
            {
                return slot.id == slotId;
            });
            if (current != slots.end())
            {
                auto updated = *current;
                updated.available = succeeded;
                updated.latencySamples = succeeded ? latency : 0;
                if (! succeeded)
                    ++updated.restartCount;
                safeThis->project.updatePluginSlot (ownerId, updated);
            }
            safeThis->audioStatusLabel.setText (
                succeeded ? message + " insert restored"
                          : "Insert unavailable: " + message,
                juce::dontSendNotification);
            safeThis->synchroniseInsertPluginsWithProject();
        });
}

void MainComponent::captureInstrumentPluginState()
{
    if (audioEngine.hasInstrumentPlugin())
        project.setInstrumentPluginState (
            audioEngine.getInstrumentPluginTrackId(),
            audioEngine.captureInstrumentPluginStateBase64());
    captureInsertPluginStates();
}

void MainComponent::captureInsertPluginStates()
{
    const auto loaded = audioEngine.getLoadedInsertPluginSlotIds();
    if (loaded.empty())
        return;
    const auto captureOwner = [this, &loaded] (const juce::String& ownerId)
    {
        for (auto slot : project.getPluginSlots (ownerId))
        {
            if (std::find (loaded.begin(), loaded.end(), slot.id)
                    == loaded.end())
                continue;
            const auto state = audioEngine.captureInsertPluginStateBase64 (
                slot.id);
            if (state.isNotEmpty() && state != slot.stateBase64)
            {
                slot.stateBase64 = state;
                project.updatePluginSlot (ownerId, slot);
            }
        }
    };
    for (int index = 0; index < project.getTrackCount(); ++index)
        captureOwner (project.getTrackId (index));
    for (const auto& channel : project.getMixerChannels())
        captureOwner (channel.id);
}

juce::String MainComponent::preferredInsertOwnerId() const
{
    auto ownerId = arrangement.getSelectedTrackId();
    if (ownerId.isNotEmpty() && project.hasTrack (ownerId))
        return ownerId;
    ownerId = project.getArmedTrackId();
    if (ownerId.isNotEmpty())
        return ownerId;
    return project.getTrackCount() > 0 ? project.getTrackId (0)
                                       : juce::String();
}

void MainComponent::toggleRecording()
{
    if (isAnyRecording())
        stopRecording();
    else
        startRecording();
}

void MainComponent::startRecording()
{
    if (currentProjectFile == juce::File())
    {
        beginSaveProject (true, [this] (bool saved)
        {
            if (saved)
                startRecording();
        });
        return;
    }

    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr)
    {
        showError ("Recording is unavailable",
                   "Choose a working audio device before recording.");
        return;
    }

    const auto activeInputCount = device->getActiveInputChannels().countNumberOfSetBits();
    if (activeInputCount <= 0)
    {
        showError ("No audio input is active",
                   "Open Device and enable at least one input channel.");
        return;
    }

    auto armedTrackIds = project.getArmedTrackIds();
    if (armedTrackIds.empty())
    {
        const auto takeNumber = project.getTrackCount() + 1;
        const auto trackId = project.addEmptyAudioTrack (
            "Take " + juce::String (takeNumber));
        project.setTrackRecordArmed (trackId, true);
        armedTrackIds = project.getArmedTrackIds();
    }

    std::vector<juce::String> audioTrackIds;
    std::vector<juce::String> instrumentTrackIds;
    for (const auto& trackId : armedTrackIds)
        if (project.getTrackState (trackId).isInstrument)
            instrumentTrackIds.push_back (trackId);
        else
            audioTrackIds.push_back (trackId);

    if (! instrumentTrackIds.empty() && ! audioTrackIds.empty())
    {
        showError ("Mixed recording targets are not supported yet",
                   "Record audio tracks together, or record one MIDI instrument. "
                   "Do not mix audio and MIDI armed targets in the same pass.");
        return;
    }
    if (! instrumentTrackIds.empty())
    {
        if (instrumentTrackIds.size() != 1)
        {
            showError ("Multiple MIDI targets are not supported yet",
                       "Arm one instrument track for MIDI recording.");
            return;
        }
        recordingTrackId = instrumentTrackIds.front();
        startMidiRecording (recordingTrackId);
        return;
    }
    if (audioTrackIds.empty())
    {
        showError ("No record-ready track",
                   "Arm at least one audio track, then try recording again.");
        return;
    }

    const auto recordingsDirectory = currentProjectFile.getParentDirectory()
                                         .getChildFile ("Recordings");
    const auto timestamp = juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S");
    const auto sampleRate = device->getCurrentSampleRate();
    const auto timelineRate = project.getTimelineSampleRate();
    const auto currentTransport = static_cast<juce::int64> (std::llround (
        audioEngine.getPositionSeconds() * timelineRate));
    const auto settings = project.getRecordingSettings();
    auto mode = recording::SessionMode::normal;
    auto captureStart = currentTransport;
    auto captureEnd = static_cast<juce::int64> (0);
    if (settings.mode == RecordingMode::punch)
    {
        mode = recording::SessionMode::punch;
        captureStart = settings.punchStartSamples;
        captureEnd = settings.punchEndSamples;
    }
    else if (settings.mode == RecordingMode::loop)
    {
        mode = recording::SessionMode::loop;
        captureStart = settings.loopStartSamples;
        captureEnd = settings.loopEndSamples;
    }

    const auto beatsPerBar = 4.0;
    const auto preRollSamples = static_cast<juce::int64> (std::llround (
        settings.preRollBars * beatsPerBar * 60.0
        / juce::jmax (1.0, project.getTempoBpm()) * timelineRate));
    if (! recordingPlan.configure (
            { mode, captureStart, captureEnd, preRollSamples },
            timelineRate, sampleRate,
            juce::jmax (1, device->getCurrentBufferSizeSamples())))
    {
        showError ("Recording range is invalid",
                   "Punch and loop ranges must end after they start. A loop must "
                   "also be at least one audio buffer long.");
        return;
    }

    const auto alignedHardwareStart = static_cast<juce::int64> (
        timeline::compensateInputLatency (
            captureStart, device->getInputLatencyInSamples(), sampleRate,
            timelineRate, project.getManualRecordOffsetSamples()));
    std::vector<MultiTrackRecorder::Target> targets;
    targets.reserve (audioTrackIds.size());
    for (const auto& trackId : audioTrackIds)
    {
        const auto track = project.getTrackState (trackId);
        MultiTrackRecorder::Target target;
        target.trackId = trackId;
        target.firstInputChannel = track.inputStartChannel;
        target.channelCount = track.inputChannelCount;
        target.tapPoint = track.recordingTapPoint;
        target.recordOffsetSamples = track.recordOffsetSamples
            + (track.recordingTapPoint == RecordingTapPoint::hardwareInput
                   ? static_cast<int> (juce::jlimit (
                         static_cast<juce::int64> (-262143),
                         static_cast<juce::int64> (262143),
                         alignedHardwareStart - captureStart))
                   : 0);
        targets.push_back (std::move (target));
    }

    MultiTrackRecorder::StartContext context;
    context.recordingsDirectory = recordingsDirectory;
    context.journalDirectory = workspace.getRecordingJournalDirectory();
    context.projectFile = currentProjectFile;
    context.projectId = project.getProjectId();
    context.sessionId = juce::Uuid().toString();
    context.fileStem = "Take-" + timestamp;
    context.sampleRate = sampleRate;
    context.timelineStartSamples = captureStart;
    context.availableInputChannels = activeInputCount;
    context.availableOutputChannels = juce::jmax (
        1, device->getActiveOutputChannels().countNumberOfSetBits());
    context.loopEnabled = mode == recording::SessionMode::loop;
    recordingJournalWarningShown = false;
    const auto result = recorder.start (std::move (targets), std::move (context));
    if (result.failed())
    {
        recordingPlan.abort();
        showError ("Recording could not start", result.getErrorMessage());
        return;
    }

    recordingTimelineStartSamples = captureStart;
    recordingStopRequested.store (false, std::memory_order_release);
    audioEngine.setRecordingTransportActive (true);
    audioEngine.setPositionSeconds (
        static_cast<double> (recordingPlan.getTransportStartSample())
        / timelineRate);
    audioEngine.play();
    audioStatusLabel.setText (
        settings.preRollBars > 0
            ? "PRE-ROLL - " + juce::String (settings.preRollBars)
                  + " bar" + (settings.preRollBars == 1 ? "" : "s")
            : audioTrackIds.size() > 1
                ? "MULTITRACK RECORDING - "
                      + juce::String (static_cast<int> (audioTrackIds.size()))
                      + " tracks"
                : "RECORDING",
        juce::dontSendNotification);
    updateProjectToolbarState();
}

void MainComponent::offerRecordingRecovery()
{
    for (const auto& journalFile : workspace.findRecordingJournals())
    {
        RecordingJournalEntry entry;
        if (RecordingJournal::read (journalFile, entry).failed()
            || entry.projectId != project.getProjectId())
            continue;

        if (! project.hasTrack (entry.trackId))
        {
            showError ("Recorded take needs repair",
                       "A journaled take belongs to a track that is no longer in "
                       "this project. The audio remains safe at:\n\n"
                           + entry.audioFile.getFullPathName());
            return;
        }

        const auto details = audioEngine.inspectAudioFile (entry.audioFile);
        if (! details.isValid())
        {
            if (entry.status == "prepared" && entry.committedSamples == 0
                && entry.audioFile.getSize() <= 128)
            {
                RecordingJournal::remove (journalFile);
                if (entry.audioFile.existsAsFile())
                    entry.audioFile.deleteFile();
                continue;
            }
            showError ("Recorded take needs repair",
                       "A journal was found, but its WAV checkpoint is not readable. "
                       "Do not delete these files:\n\n"
                           + entry.audioFile.getFullPathName() + "\n"
                           + journalFile.getFullPathName());
            return;
        }

        auto safe = juce::Component::SafePointer<MainComponent> (this);
        juce::AlertWindow::showOkCancelBox (
            juce::AlertWindow::QuestionIcon,
            "Recover interrupted recording?",
            "Triumph Studio found a readable WAV checkpoint from an interrupted "
            "recording. Recover it onto its original track?\n\n"
                + entry.audioFile.getFileName(),
            "Recover", "Keep for later", this,
            juce::ModalCallbackFunction::create (
                [safe, entry, journalFile, details] (int result)
                {
                    if (safe == nullptr || result == 0)
                        return;
                    const auto clipId = safe->project.addRecordedClip (
                        entry.trackId, entry.audioFile, details.sampleRate,
                        details.channels, details.lengthInSamples,
                        entry.timelineStartSamples);
                    if (clipId.isEmpty())
                    {
                        safe->showError (
                            "Recording recovery failed",
                            "The WAV remains safe at:\n"
                                + entry.audioFile.getFullPathName());
                        return;
                    }
                    RecordingJournal::remove (journalFile);
                    safe->setDirty (true);
                    safe->audioStatusLabel.setText (
                        "Interrupted recording recovered - save the project",
                        juce::dontSendNotification);
                }));
        return;
    }
}

void MainComponent::startMidiRecording (const juce::String& trackId)
{
    const auto track = project.getTrackState (trackId);
    if (! track.isInstrument || track.midiClips.empty())
    {
        showError ("No record-ready instrument",
                   "Arm a MIDI instrument track, then try recording again.");
        recordingTrackId.clear();
        return;
    }

    recordingTrackId = trackId;
    midiRecordingTransportStartSeconds = audioEngine.getPositionSeconds();
    audioEngine.setRecordingTransportActive (true);
    audioEngine.play();
    midiCapture.start (juce::Time::getMillisecondCounterHiRes() / 1000.0);
    audioStatusLabel.setText ("MIDI RECORDING - play an enabled MIDI input",
                              juce::dontSendNotification);
    updateProjectToolbarState();
}

void MainComponent::stopMidiRecording()
{
    audioEngine.setRecordingTransportActive (false);
    audioEngine.pause();
    audioEngine.clearLiveMidiNotes();

    const auto captured = midiCapture.stop (
        juce::Time::getMillisecondCounterHiRes() / 1000.0);
    const auto targetTrackId = recordingTrackId;
    recordingTrackId.clear();

    const auto track = project.getTrackState (targetTrackId);
    if (! track.isInstrument || track.midiClips.empty())
    {
        showError ("MIDI take could not be added",
                   "The destination instrument track is no longer available.");
        return;
    }

    std::vector<tempo::Point> tempoPoints;
    for (const auto& point : project.getTempoPoints())
        tempoPoints.push_back ({ point.beat, point.bpm,
            point.curve == TempoCurve::linear ? tempo::SegmentCurve::linear
                                               : tempo::SegmentCurve::step });

    const auto& clip = track.midiClips.front();
    std::vector<MidiNoteState> notes;
    notes.reserve (captured.size());
    for (const auto& source : captured)
    {
        const auto startSeconds = midiRecordingTransportStartSeconds
                                  + source.startSeconds;
        const auto endSeconds = startSeconds + source.lengthSeconds;
        const auto absoluteStartBeat = tempo::secondsToBeat (startSeconds, tempoPoints);
        const auto absoluteEndBeat = tempo::secondsToBeat (endSeconds, tempoPoints);
        notes.push_back ({ {},
                           juce::jmax (0.0, absoluteStartBeat - clip.startBeat),
                           juce::jmax (1.0 / 64.0,
                                       absoluteEndBeat - absoluteStartBeat),
                           source.noteNumber,
                           source.velocity,
                           source.channel });
    }

    const auto added = project.addMidiNotes (targetTrackId, clip.id, notes);
    if (added == 0)
        project.setTrackRecordArmed (targetTrackId, false);
    audioStatusLabel.setText (added > 0
                                  ? "MIDI take captured - " + juce::String (added)
                                        + " note" + (added == 1 ? "" : "s")
                                  : "MIDI recording stopped - no notes captured",
                              juce::dontSendNotification);
    updateProjectToolbarState();
}

bool MainComponent::isAnyRecording() const noexcept
{
    return recorder.isRecording() || midiCapture.isRecording();
}

void MainComponent::stopRecording()
{
    if (midiCapture.isRecording())
    {
        stopMidiRecording();
        return;
    }

    if (! recorder.isRecording())
        return;

    audioEngine.setRecordingTransportActive (false);
    audioEngine.pause();
    recordingPlan.beginFinalize();
    const auto results = recorder.stop();
    recordingTrackId.clear();

    auto addedTakes = 0;
    auto droppedBlocks = 0;
    juce::int64 droppedSamples = 0;
    juce::String lastFileName;
    for (const auto& result : results)
    {
        if (! result.audio.isValid())
            continue;
        const auto clipId = project.addRecordedClip (
            result.target.trackId,
            result.audio.file,
            result.audio.sampleRate,
            result.audio.channels,
            result.audio.samples,
            result.timelineStartSamples);
        if (clipId.isEmpty())
        {
            showError ("Take could not be added",
                       "The WAV file is safe at:\n"
                           + result.audio.file.getFullPathName());
            continue;
        }
        ++addedTakes;
        droppedBlocks += result.audio.droppedBlocks;
        droppedSamples += result.audio.droppedSamples;
        lastFileName = result.audio.file.getFileName();
    }
    recordingPlan.finishFinalize (addedTakes > 0);

    if (addedTakes == 0)
    {
        showError ("Recording was not captured",
                   "No audio samples were written. Check the selected input and try again.");
        updateAudioDeviceStatus();
        return;
    }

    audioStatusLabel.setText ("Recorded " + juce::String (addedTakes)
                                  + " take" + (addedTakes == 1 ? "" : "s")
                                  + (lastFileName.isNotEmpty()
                                         ? " - " + lastFileName : ""),
                              juce::dontSendNotification);
    updateProjectToolbarState();

    if (droppedBlocks > 0)
    {
        const auto lostMilliseconds = timeline::samplesToMilliseconds (
            droppedSamples,
            deviceManager.getCurrentAudioDevice() != nullptr
                ? deviceManager.getCurrentAudioDevice()->getCurrentSampleRate()
                : 48000.0);
        showInformation ("Recording dropout detected",
                         "The take was saved, but "
                             + juce::String (droppedBlocks)
                             + " audio block(s), about "
                             + juce::String (lostMilliseconds, 1)
                             + " ms, were not written. Increase the audio buffer and record the take again.");
    }
}

void MainComponent::showAudioDeviceSettings()
{
    auto safe = juce::Component::SafePointer<MainComponent> (this);
    auto panel = std::make_unique<AudioDeviceSettingsPanel> (
        deviceManager,
        [safe]
        {
            if (safe != nullptr)
                safe->startOutputTest();
        });

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (panel.release());
    options.dialogTitle = "Triumph Studio - Audio Device and Output Test";
    options.dialogBackgroundColour = StudioColours::panel;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.componentToCentreAround = this;
    options.launchAsync();

    audioStatusLabel.setText ("Audio device settings opened",
                              juce::dontSendNotification);
}

bool MainComponent::hasUsableAudioOutput()
{
    auto* device = deviceManager.getCurrentAudioDevice();
    const output::DeviceSnapshot snapshot {
        device != nullptr,
        device != nullptr
            ? device->getActiveOutputChannels().countNumberOfSetBits() : 0,
        device != nullptr ? device->getCurrentSampleRate() : 0.0,
        device != nullptr ? device->getCurrentBufferSizeSamples() : 0
    };
    return output::assess (snapshot) == output::Availability::ready;
}

bool MainComponent::recoverDefaultAudioOutput()
{
    if (hasUsableAudioOutput())
        return true;

    const auto error = deviceManager.initialise (0, 2, nullptr, true);
    if (error.isNotEmpty() || ! hasUsableAudioOutput())
    {
        audioStatusLabel.setColour (juce::Label::textColourId,
                                    StudioColours::red);
        audioStatusLabel.setText (
            error.isNotEmpty()
                ? "Audio output recovery failed - " + error
                : "No active audio output - open Device settings",
            juce::dontSendNotification);
        return false;
    }

    saveAudioDeviceStateIfUsable();
    return true;
}

void MainComponent::saveAudioDeviceStateIfUsable()
{
    if (! hasUsableAudioOutput())
        return;
    if (auto deviceState = deviceManager.createStateXml())
        workspace.setAudioDeviceState (*deviceState);
}

void MainComponent::queueAudioOutputRecovery()
{
    if (! appShuttingDown.load (std::memory_order_acquire))
        audioOutputRecoveryQueued.store (true, std::memory_order_release);
}

void MainComponent::serviceAudioOutputRecovery()
{
    if (appShuttingDown.load (std::memory_order_acquire))
        return;

    if (hasUsableAudioOutput())
    {
        if (! audioCallbackPrepared.load (std::memory_order_acquire))
            return;

        const auto shouldResume = audioOutputRecovery.markReady();
        saveAudioDeviceStateIfUsable();
        if (shouldResume && ! isAnyRecording())
        {
            audioEngine.play();
            if (auto* device = deviceManager.getCurrentAudioDevice())
            {
                audioStatusLabel.setColour (juce::Label::textColourId,
                                            StudioColours::green);
                audioStatusLabel.setText (
                    "Audio output recovered - playback resumed through "
                        + device->getName(),
                    juce::dontSendNotification);
            }
        }
        return;
    }

    if (! audioOutputRecovery.beginRecovery())
        return;

    audioStatusLabel.setColour (juce::Label::textColourId,
                                StudioColours::orange);
    audioStatusLabel.setText (
        "Audio device interrupted - reopening the default output",
        juce::dontSendNotification);

    if (! recoverDefaultAudioOutput())
    {
        audioOutputRecovery.markFailed();
        return;
    }

    if (audioCallbackPrepared.load (std::memory_order_acquire))
    {
        const auto shouldResume = audioOutputRecovery.markReady();
        if (shouldResume && ! isAnyRecording())
            audioEngine.play();
        updateAudioDeviceStatus();
    }
}

void MainComponent::startPlayback()
{
    if (! hasUsableAudioOutput() && ! recoverDefaultAudioOutput())
    {
        showError (
            "No working audio output",
            "Triumph Studio could not open an active output channel. Open Device, "
            "choose your real speakers or headphones, then press Test Output.");
        return;
    }

    audioOutputRecovery.markReady();

    outputWarningVisible = false;
    stagnantOutputTimerTicks = 0;
    silentPlaybackTimerTicks = 0;
    outputPeak.store (0.0f, std::memory_order_relaxed);
    audioEngine.play();
    if (const auto* device = deviceManager.getCurrentAudioDevice())
        audioStatusLabel.setText (
            "Playing through " + device->getName(),
            juce::dontSendNotification);
}

void MainComponent::startOutputTest()
{
    if (isAnyRecording())
    {
        showInformation ("Output test unavailable",
                         "Stop recording before testing the output.");
        return;
    }
    if (! hasUsableAudioOutput() && ! recoverDefaultAudioOutput())
    {
        showError ("No working audio output",
                   "Select a device with at least one active output channel.");
        return;
    }

    audioOutputRecovery.markReady();

    outputTestStatusPending = true;
    outputWarningVisible = false;
    outputTestRequested.store (true, std::memory_order_release);
    audioStatusLabel.setColour (juce::Label::textColourId,
                                StudioColours::primary);
    audioStatusLabel.setText (
        "Output test requested - listen for a one-second 440 Hz tone",
        juce::dontSendNotification);
}

void MainComponent::updateAudioDeviceStatus()
{
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        const auto inputChannels =
            device->getActiveInputChannels().countNumberOfSetBits();
        const auto outputChannels =
            device->getActiveOutputChannels().countNumberOfSetBits();
        audioEngine.setInputChannelCount (inputChannels);
        const auto availability = output::assess ({
            true, outputChannels, device->getCurrentSampleRate(),
            device->getCurrentBufferSizeSamples()
        });
        if (availability != output::Availability::ready)
        {
            audioStatusLabel.setColour (juce::Label::textColourId,
                                        StudioColours::red);
            audioStatusLabel.setText (
                outputChannels <= 0
                    ? device->getName()
                          + " has no active output channels - open Device"
                    : device->getName()
                          + " is not running with a valid sample rate/buffer",
                juce::dontSendNotification);
            return;
        }

        const auto latencyMs = 1000.0 * static_cast<double> (
            device->getOutputLatencyInSamples()) / device->getCurrentSampleRate();
        audioStatusLabel.setColour (juce::Label::textColourId,
                                    StudioColours::textMuted);
        audioStatusLabel.setText (
            device->getName() + "  /  OUT " + juce::String (outputChannels)
                + "  /  "
                + juce::String (device->getCurrentSampleRate() / 1000.0, 1)
                + " kHz  /  "
                + juce::String (device->getCurrentBufferSizeSamples())
                + " samples  /  " + juce::String (latencyMs, 1)
                + " ms output latency",
            juce::dontSendNotification);
    }
    else
    {
        audioEngine.setInputChannelCount (0);
        audioStatusLabel.setColour (juce::Label::textColourId,
                                    StudioColours::red);
        audioStatusLabel.setText ("No audio device - open Device settings",
                                  juce::dontSendNotification);
    }
}

void MainComponent::offerCrashRecovery()
{
    const auto recoveryFiles = workspace.findRecoveryFiles();
    if (recoveryFiles.isEmpty())
        return;

    const auto recoveryFile = recoveryFiles.getFirst();
    ProjectState recoveredState;
    const auto loadResult = ProjectDocument::load (recoveryFile, recoveredState);

    if (loadResult.failed())
    {
        recoveryFile.deleteFile();
        offerCrashRecovery();
        return;
    }

    const auto timestamp = recoveryFile.getLastModificationTime().formatted (
        "%Y-%m-%d %H:%M:%S");
    juce::AlertWindow::showYesNoCancelBox (
        juce::AlertWindow::QuestionIcon,
        "Recover an autosaved project?",
        recoveredState.name + " was autosaved at " + timestamp + ".",
        "Recover",
        "Discard",
        "Later",
        this,
        juce::ModalCallbackFunction::create ([this, recoveryFile] (int result)
        {
            if (result == 1)
                loadProjectFromFile (recoveryFile, true);
            else if (result == 2)
            {
                recoveryFile.deleteFile();
                offerCrashRecovery();
            }
        }));
}

void MainComponent::performAutosave()
{
    captureInstrumentPluginState();
    const auto result = workspace.writeRecoveryFile (project);

    if (result.wasOk())
        audioStatusLabel.setText ("Autosaved "
                                      + juce::Time::getCurrentTime().formatted ("%H:%M"),
                                  juce::dontSendNotification);
    else
        audioStatusLabel.setText ("Autosave failed - use Save",
                                  juce::dontSendNotification);
}

bool MainComponent::addAudioFile (const juce::File& file, bool showFailureMessage)
{
    const auto details = audioEngine.inspectAudioFile (file);

    if (details.isValid())
        return project.addAudioTrack (file,
                                      details.sampleRate,
                                      details.channels,
                                      details.lengthInSamples).isNotEmpty();

    if (showFailureMessage)
        showError ("Audio could not be imported",
                   file.getFileName() + " is missing, damaged, or uses an unsupported codec.");

    return false;
}

void MainComponent::setProjectName (juce::String newName)
{
    currentProjectName = newName.trim().isNotEmpty() ? newName.trim() : "Untitled Project";
    projectLabel.setText (currentProjectName + (dirty ? "  -  Unsaved" : ""),
                          juce::dontSendNotification);
}

void MainComponent::setDirty (bool shouldBeDirty)
{
    if (shouldBeDirty && ! dirty)
        lastAutosaveAttemptMilliseconds = juce::Time::getMillisecondCounterHiRes();

    dirty = shouldBeDirty;
    projectLabel.setText (currentProjectName + (dirty ? "  -  Unsaved" : ""),
                          juce::dontSendNotification);
}

void MainComponent::handleProjectChanged()
{
    synchroniseEngineWithProject();
    arrangement.refreshFromModel();
    masterSlider.setValue (project.getMasterGain(), juce::dontSendNotification);
    tempoSlider.setValue (project.getTempoBpm(), juce::dontSendNotification);
    lowLatencyButton.setToggleState (project.getLowLatencyMonitoring(),
                                     juce::dontSendNotification);
    const auto monitorMode = project.getInputMonitorMode();
    monitorButton.setToggleState (monitorMode != InputMonitorMode::off,
                                  juce::dontSendNotification);
    monitorButton.setButtonText (
        monitorMode == InputMonitorMode::whileArmed ? "Mon Arm" : "Monitor");
    monitorButton.setTooltip (
        monitorMode == InputMonitorMode::always
            ? "Input monitoring is always on - use headphones (M)"
            : monitorMode == InputMonitorMode::whileArmed
                ? "Input monitoring follows the armed track (M)"
                : "Input monitoring is off (M)");
    if (inspectorPanel != nullptr)
        inspectorPanel->repaint();
    if (browserPanel != nullptr)
        browserPanel->repaint();
    mixer.refreshFromModel();
    tempoAutomation.refreshFromModel();
    producerAssistant.setSettings (project.getProducerSettings());
    producerAssistant.setProjectTrackCount (project.getTrackCount());
    setProjectName (project.getName());

    if (! replacingProject)
        setDirty (true);

    updateProjectToolbarState();
}

void MainComponent::synchroniseEngineWithProject()
{
    audioEngine.beginRenderStateUpdate();
    std::vector<AudioEngine::MidiTrackPlayback> midiPlayback;
    std::vector<AudioEngine::DelayTrack> delayTracks;
    std::vector<AudioEngine::MixerChannel> mixerChannels;
    std::vector<AudioEngine::AutomationPlayback> automationPlayback;
    std::vector<tempo::Point> tempoPlayback;
    const auto aggregatePluginLatency = [] (
        const auto& slots, int legacyLatency)
    {
        if (slots.empty())
            return juce::jmax (0, legacyLatency);
        auto latency = 0;
        for (const auto& slot : slots)
            if (slot.available)
                latency = juce::jmin (262143,
                    latency + juce::jmax (0, slot.latencySamples));
        return latency;
    };
    for (const auto& point : project.getTempoPoints())
        tempoPlayback.push_back ({ point.beat, point.bpm,
            point.curve == TempoCurve::linear ? tempo::SegmentCurve::linear
                                               : tempo::SegmentCurve::step });
    std::vector<automation::Signature> signatures;
    for (const auto& signature : project.getTimeSignatures())
        signatures.push_back ({ signature.beat, signature.numerator,
                                signature.denominator });
    for (const auto& laneState : project.getAutomationLanes())
    {
        AudioEngine::AutomationPlayback lane;
        lane.targetId = laneState.targetId;
        lane.parameterId = laneState.parameterId;
        lane.enabled = laneState.enabled;
        for (const auto& point : laneState.points)
            lane.points.push_back ({ point.samplePosition, point.value,
                point.curve == AutomationCurve::hold ? automation::Curve::hold
                : point.curve == AutomationCurve::smooth ? automation::Curve::smooth
                                                         : automation::Curve::linear });
        automationPlayback.push_back (std::move (lane));
    }

    for (int index = audioEngine.getTrackCount() - 1; index >= 0; --index)
    {
        const auto* engineTrack = audioEngine.getTrack (index);
        if (engineTrack != nullptr)
        {
            const auto trackId = engineTrack->getId();
            if (! project.hasTrack (trackId))
                audioEngine.removeTrack (trackId);
        }
    }

    for (int index = 0; index < project.getTrackCount(); ++index)
    {
        const auto trackId = project.getTrackId (index);
        const auto trackState = project.getTrackState (trackId);
        AudioEngine::MixerChannel mixerChannel;
        mixerChannel.id = trackState.id;
        mixerChannel.kind = mixer::ChannelKind::track;
        mixerChannel.gain = trackState.gain;
        mixerChannel.pan = trackState.pan;
        mixerChannel.muted = trackState.muted;
        mixerChannel.solo = trackState.solo;
        mixerChannel.outputDestinationId = trackState.outputDestinationId;
        mixerChannel.inputChannels = trackState.layout.inputChannels;
        mixerChannel.outputChannels = trackState.layout.outputChannels;
        mixerChannel.sidechainChannels = trackState.layout.sidechainChannels;
        mixerChannel.explicitBusLayout = trackState.layout.explicitLayout;
        mixerChannel.pluginLatencySamples = aggregatePluginLatency (
            trackState.pluginSlots, trackState.pluginLatencySamples);
        for (const auto& send : trackState.sends)
            mixerChannel.sends.push_back ({ send.id, send.destinationId,
                                            send.gain, send.enabled,
                                            send.preFader, send.sidechain });
        const auto trackPluginLatency = mixerChannel.pluginLatencySamples;
        mixerChannels.push_back (std::move (mixerChannel));
        delayTracks.push_back ({ trackState.id,
                                 trackPluginLatency,
                                 trackState.manualLatencyOffsetSamples,
                                 trackState.pluginBypassed,
                                 trackState.frozen,
                                 trackState.pluginAvailable,
                                 trackState.recordArmed });
        if (trackState.isInstrument && ! trackState.frozen)
        {
            audioEngine.removeTrack (trackId);
            AudioEngine::MidiTrackPlayback midiTrack;
            midiTrack.id = trackState.id;
            midiTrack.instrumentName = trackState.pluginName;
            midiTrack.gain = trackState.gain;
            midiTrack.pan = trackState.pan;
            midiTrack.muted = trackState.muted;
            midiTrack.solo = trackState.solo;
            midiTrack.manualLatencyOffsetSamples =
                trackState.manualLatencyOffsetSamples;
            for (const auto& clip : trackState.midiClips)
            {
                for (const auto& note : clip.notes)
                {
                    AudioEngine::MidiNotePlayback playback;
                    playback.startBeat = clip.startBeat + note.startBeat;
                    playback.lengthBeats = note.lengthBeats;
                    playback.noteNumber = note.noteNumber;
                    playback.velocity = note.velocity;
                    playback.channel = note.channel;
                    playback.noteId = note.noteId;
                    for (const auto& expression : note.expression)
                    {
                        AudioEngine::MidiNotePlayback::Expression point;
                        point.offsetBeats = expression.offsetBeats;
                        point.value = expression.value;
                        point.type = expression.type
                                         == MidiNoteState::ExpressionPoint::Type::pressure
                                     ? AudioEngine::MidiNotePlayback::Expression::Type::pressure
                                     : expression.type
                                               == MidiNoteState::ExpressionPoint::Type::timbre
                                           ? AudioEngine::MidiNotePlayback::Expression::Type::timbre
                                           : AudioEngine::MidiNotePlayback::Expression::Type::pitch;
                        playback.expression.push_back (point);
                    }
                    midiTrack.notes.push_back (std::move (playback));
                }
                for (const auto& controller : clip.controllers)
                    midiTrack.controllers.push_back ({
                        clip.startBeat + controller.beat, 0.0,
                        controller.channel, controller.controller,
                        controller.value, controller.noteId });
            }
            midiPlayback.push_back (std::move (midiTrack));
            continue;
        }
        juce::File desiredSourceFile;
        const auto resolvedPlayback = project.resolveAudioPlayback (trackId);
        if (! resolvedPlayback.empty())
            desiredSourceFile = project.getMediaSourceState (
                resolvedPlayback.front().sourceId).file;

        auto* engineTrack = audioEngine.findTrack (trackId);

        if (engineTrack != nullptr && engineTrack->getSourceFile() != desiredSourceFile)
        {
            audioEngine.removeTrack (trackId);
            engineTrack = nullptr;
        }

        if (engineTrack == nullptr && desiredSourceFile.existsAsFile())
            engineTrack = audioEngine.addTrack (trackId, desiredSourceFile);

        if (engineTrack == nullptr)
            continue;

        engineTrack->setName (trackState.name);
        engineTrack->setGain (trackState.gain);
        engineTrack->setPan (trackState.pan);
        engineTrack->setMuted (trackState.muted);
        engineTrack->setSolo (trackState.solo);

        std::vector<AudioTrack::ClipPlayback> clipPlayback;
        clipPlayback.reserve (resolvedPlayback.size());

        for (const auto& clip : resolvedPlayback)
        {
            const auto source = project.getMediaSourceState (clip.sourceId);
            clipPlayback.push_back ({ clip.id,
                                      clip.sourceId,
                                      source.file,
                                      clip.timelineStartSamples,
                                      clip.sourceOffsetSamples,
                                      clip.lengthInSamples,
                                      clip.gain,
                                      clip.fadeInSamples,
                                      clip.fadeOutSamples,
                                      clip.playbackRate,
                                      clip.reversed,
                                      clip.fadeAnchorStartSamples,
                                      clip.fadeAnchorLengthSamples });
        }

        audioEngine.configureTrackClips (trackId,
                                         std::move (clipPlayback),
                                         project.getTimelineSampleRate());
    }

    audioEngine.configureMidiTracks (std::move (midiPlayback),
                                     tempoPlayback);
    audioEngine.configureMusicalTimeline (
        std::move (tempoPlayback), std::move (signatures),
        project.getMetronomeEnabled(), project.getMetronomeGain());
    for (const auto& channelState : project.getMixerChannels())
    {
        AudioEngine::MixerChannel channel;
        channel.id = channelState.id;
        channel.kind = channelState.kind == MixerChannelKind::returnChannel
                           ? mixer::ChannelKind::returnChannel
                           : mixer::ChannelKind::bus;
        channel.gain = channelState.gain;
        channel.pan = channelState.pan;
        channel.muted = channelState.muted;
        channel.solo = channelState.solo;
        channel.outputDestinationId = channelState.outputDestinationId;
        channel.inputChannels = channelState.layout.inputChannels;
        channel.outputChannels = channelState.layout.outputChannels;
        channel.sidechainChannels = channelState.layout.sidechainChannels;
        channel.explicitBusLayout = channelState.layout.explicitLayout;
        channel.pluginLatencySamples = aggregatePluginLatency (
            channelState.pluginSlots, 0);
        for (const auto& send : channelState.sends)
            channel.sends.push_back ({ send.id, send.destinationId,
                                       send.gain, send.enabled,
                                       send.preFader, send.sidechain });
        mixerChannels.push_back (std::move (channel));
    }
    audioEngine.configureMixer (std::move (mixerChannels));
    audioEngine.configureDelayCompensation (
        std::move (delayTracks), project.getLowLatencyMonitoring());
    std::vector<AudioEngine::MonitorPath> monitorPaths;
    for (const auto& state : project.getMonitorPaths())
    {
        const auto role = state.role == MonitorPathRole::cue
            ? mixer::MonitorRole::cue
            : state.role == MonitorPathRole::listen
                ? mixer::MonitorRole::listen
                : state.role == MonitorPathRole::talkback
                    ? mixer::MonitorRole::talkback
                    : mixer::MonitorRole::controlRoom;
        monitorPaths.push_back ({ state.id, role, state.sourceId,
                                  state.outputStartChannel,
                                  state.outputChannelCount, state.gain,
                                  state.muted, state.dimmed });
    }
    audioEngine.configureMonitorPaths (std::move (monitorPaths));
    audioEngine.setProjectTimeline (project.getTimelineSampleRate(),
                                    project.getProjectLengthInSamples());
    const auto recordingSettings = project.getRecordingSettings();
    audioEngine.setPlaybackLoop (
        recordingSettings.loopStartSamples,
        recordingSettings.loopEndSamples,
        recordingSettings.loopEndSamples > recordingSettings.loopStartSamples);
    const auto projectSync = project.getSyncSettings();
    AudioEngine::SyncConfiguration sync;
    sync.source = projectSync.source == SyncSourceState::midiClock
        ? midi::SyncSource::midiClock
        : projectSync.source == SyncSourceState::midiTimeCode
            ? midi::SyncSource::midiTimeCode
            : projectSync.source == SyncSourceState::link
                ? midi::SyncSource::link : midi::SyncSource::internal;
    sync.sendMidiClock = projectSync.sendMidiClock;
    sync.followExternalStartStop = projectSync.followExternalStartStop;
    sync.mtcFramesPerSecond = projectSync.mtcFramesPerSecond;
    sync.jitterToleranceSamples = projectSync.jitterToleranceSamples;
    audioEngine.configureSync (sync);
    audioEngine.configureAutomation (std::move (automationPlayback));

    audioEngine.setMasterGain (project.getMasterGain());
    audioEngine.setMasterMuted (project.getMasterMuted());
    audioEngine.setControlRoomState (
        recordingSettings.controlRoomGain,
        recordingSettings.controlRoomDimmed,
        recordingSettings.controlRoomMuted);
    const auto monitorMode = project.getInputMonitorMode();
    audioEngine.setInputMonitoring (
        monitorMode == InputMonitorMode::always
        || (monitorMode == InputMonitorMode::whileArmed
            && project.getArmedTrackId().isNotEmpty()));
    audioEngine.endRenderStateUpdate();
    synchroniseInstrumentPluginWithProject();
    synchroniseInsertPluginsWithProject();
}

void MainComponent::updateTransportDisplay()
{
    timeLabel.setText (formatTime (audioEngine.getPositionSeconds())
                           + " / " + formatTime (audioEngine.getProjectLengthSeconds()),
                       juce::dontSendNotification);
    playButton.setToggleState (audioEngine.isPlaying(), juce::dontSendNotification);
}

void MainComponent::detectSelectedClipTransients()
{
    if (! arrangement.hasSelectedClip())
        return;
    const auto trackId = arrangement.getSelectedTrackId();
    const auto clipId = arrangement.getSelectedClipId();
    const auto clip = project.getClipState (trackId, clipId);
    const auto source = project.getMediaSourceState (clip.sourceId);
    if (clip.id.isEmpty() || ! source.file.existsAsFile())
    {
        showError ("Transient Detection", "The selected clip's source media is unavailable.");
        return;
    }

    if (transientAnalysisInProgress.exchange (true))
    {
        audioStatusLabel.setText ("Transient analysis is already running",
                                  juce::dontSendNotification);
        return;
    }
    audioStatusLabel.setText ("Analyzing selected clip transients...",
                              juce::dontSendNotification);
    const auto timelineRate = project.getTimelineSampleRate();
    auto safe = juce::Component::SafePointer<MainComponent> (this);
    if (transientAnalysisThread.joinable())
        transientAnalysisThread.join();
    transientAnalysisThread = std::thread (
        [safe, trackId, clipId, clip, source, timelineRate]
    {
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (
            formats.createReaderFor (source.file));
        if (reader == nullptr)
        {
            juce::MessageManager::callAsync ([safe]
            {
                if (safe != nullptr)
                {
                    safe->transientAnalysisInProgress.store (false);
                    safe->showError ("Transient Detection",
                                     "Triumph Studio could not decode this audio file.");
                }
            });
            return;
        }

        const auto nativeTimelineLength = stretch::timelineToNativeSamples (
            clip.lengthInSamples, clip.timeStretchRatio);
        const auto requested = static_cast<juce::int64> (timeline::convertSampleCount (
            nativeTimelineLength, timelineRate, reader->sampleRate));
        const auto available = juce::jmax (static_cast<juce::int64> (0),
                                           reader->lengthInSamples
                                               - clip.sourceOffsetSamples);
        const auto count = juce::jmin (available, requested);
        std::vector<float> mono (static_cast<std::size_t> (count), 0.0f);
        constexpr int blockSize = 32768;
        juce::AudioBuffer<float> block (juce::jmax (1, juce::jmin (2,
            static_cast<int> (reader->numChannels))), blockSize);
        for (juce::int64 position = 0; position < count; position += blockSize)
        {
            const auto amount = static_cast<int> (juce::jmin (
                static_cast<juce::int64> (blockSize), count - position));
            block.clear();
            if (! reader->read (&block, 0, amount,
                                clip.sourceOffsetSamples + position, true, true))
                break;
            for (int channel = 0; channel < block.getNumChannels(); ++channel)
                for (int sample = 0; sample < amount; ++sample)
                    mono[static_cast<std::size_t> (position + sample)]
                        += block.getSample (channel, sample)
                           / static_cast<float> (block.getNumChannels());
        }

        transient::Settings settings;
        settings.minimumSpacingSamples = static_cast<juce::int64> (
            reader->sampleRate * 0.04);
        auto detections = transient::detect (mono.data(), count, settings);
        float maximum = 0.0f;
        for (const auto& detection : detections)
            maximum = juce::jmax (maximum, detection.strength);
        std::vector<TransientMarkerState> markers;
        markers.reserve (detections.size());
        for (const auto& detection : detections)
            markers.push_back ({ juce::Uuid().toString(), 0,
                                 clip.sourceOffsetSamples + detection.sample,
                                 maximum > 0.0f ? detection.strength / maximum : 0.0f });

        juce::MessageManager::callAsync ([safe, trackId, clipId,
                                          markers = std::move (markers)] () mutable
        {
            if (safe == nullptr)
                return;
            safe->transientAnalysisInProgress.store (false);
            safe->project.setTransientMarkers (trackId, clipId, markers);
            safe->arrangement.refreshFromModel();
            safe->audioStatusLabel.setText (
                juce::String (markers.size()) + " transients detected - Warp All to convert",
                juce::dontSendNotification);
        });
    });
}

void MainComponent::convertSelectedClipTransients()
{
    if (! arrangement.hasSelectedClip())
        return;
    const auto count = project.convertTransientsToWarpMarkers (
        arrangement.getSelectedTrackId(), arrangement.getSelectedClipId());
    arrangement.refreshFromModel();
    audioStatusLabel.setText (count > 0
        ? juce::String (count) + " warp markers created"
        : juce::String ("No new warp markers were needed"),
        juce::dontSendNotification);
}

void MainComponent::toggleSelectedClipReverse()
{
    if (! arrangement.hasSelectedClip() || advancedEditInProgress.load())
        return;
    if (exportInProgress.load())
    {
        audioStatusLabel.setText ("Wait for export to finish before editing audio",
                                  juce::dontSendNotification);
        return;
    }
    const auto trackId = arrangement.getSelectedTrackId();
    const auto clipId = arrangement.getSelectedClipId();
    const auto clip = project.getClipState (trackId, clipId);
    if (clip.id.isEmpty())
        return;
    project.setClipReversed (trackId, clipId, ! clip.reversed);
    arrangement.refreshFromModel();
    audioStatusLabel.setText (clip.reversed
        ? "Selected clip restored to forward playback"
        : "Selected clip reversed non-destructively",
        juce::dontSendNotification);
}

void MainComponent::beginNormalizeSelectedClip()
{
    if (! arrangement.hasSelectedClip())
        return;
    if (exportInProgress.load())
    {
        audioStatusLabel.setText ("Wait for export to finish before editing audio",
                                  juce::dontSendNotification);
        return;
    }
    if (advancedEditInProgress.exchange (true))
    {
        audioStatusLabel.setText ("An advanced audio edit is already running",
                                  juce::dontSendNotification);
        return;
    }

    const auto trackId = arrangement.getSelectedTrackId();
    const auto clipId = arrangement.getSelectedClipId();
    auto snapshot = project.createSnapshot();
    advancedEditCancelRequested.store (false);
    advancedEditProgress.store (0.0f);
    audioStatusLabel.setText ("Analyzing selected clip peak...",
                              juce::dontSendNotification);
    if (advancedEditThread.joinable())
        advancedEditThread.join();
    auto safe = juce::Component::SafePointer<MainComponent> (this);
    advancedEditThread = std::thread (
        [this, safe, snapshot = std::move (snapshot), trackId, clipId] () mutable
    {
        float peak = 0.0f;
        const auto result = AdvancedAudioRenderer::analyseClipPeak (
            snapshot, trackId, clipId, advancedEditCancelRequested,
            advancedEditProgress, peak);
        juce::MessageManager::callAsync ([safe, result, peak, trackId, clipId]
        {
            if (safe == nullptr)
                return;
            safe->advancedEditInProgress.store (false);
            if (result.failed())
            {
                safe->showError ("Normalize Clip", result.getErrorMessage());
                return;
            }
            if (peak <= 1.0e-9f)
            {
                safe->showError ("Normalize Clip",
                                 "The selected clip is silent and cannot be normalized.");
                return;
            }
            const auto gain = advanced::normalisationGain (peak, -1.0, 16.0f);
            if (! safe->project.setClipGain (
                    trackId, clipId, gain, "Normalize Clip to -1 dB"))
            {
                safe->audioStatusLabel.setText (
                    "Selected clip is already normalized to -1 dB",
                    juce::dontSendNotification);
                return;
            }
            safe->arrangement.refreshFromModel();
            const auto reachedTarget = peak * gain
                >= static_cast<float> (advanced::targetAmplitudeFromDb (-1.0))
                       * 0.999f;
            safe->audioStatusLabel.setText (reachedTarget
                ? "Clip normalized non-destructively to -1 dB peak (gain "
                      + juce::String (gain, 3) + "x)"
                : "Clip gain raised to the 16x safety limit; peak remains below -1 dB",
                juce::dontSendNotification);
        });
    });
}

void MainComponent::beginPitchShiftSelectedClip (double semitones)
{
    if (! arrangement.hasSelectedClip() || std::abs (semitones) < 0.01)
        return;
    if (exportInProgress.load())
    {
        audioStatusLabel.setText ("Wait for export to finish before editing audio",
                                  juce::dontSendNotification);
        return;
    }
    if (currentProjectFile == juce::File())
    {
        beginSaveProject (true, [this, semitones] (bool saved)
        {
            if (saved)
                beginPitchShiftSelectedClip (semitones);
        });
        return;
    }
    if (advancedEditInProgress.exchange (true))
    {
        audioStatusLabel.setText ("An advanced audio edit is already running",
                                  juce::dontSendNotification);
        return;
    }

    const auto trackId = arrangement.getSelectedTrackId();
    const auto clipId = arrangement.getSelectedClipId();
    const auto track = project.getTrackState (trackId);
    auto processedFolder = currentProjectFile.getParentDirectory().getChildFile (
        "Processed");
    if (! processedFolder.isDirectory() && ! processedFolder.createDirectory())
    {
        advancedEditInProgress.store (false);
        showError ("Pitch Shift", "The project Processed folder could not be created.");
        return;
    }
    const auto signedPitch = juce::String (semitones >= 0.0 ? "plus" : "minus")
        + juce::String (std::abs (juce::roundToInt (semitones)));
    const auto baseName = juce::File::createLegalFileName (
        (track.name.isNotEmpty() ? track.name : "Clip") + "_pitch_" + signedPitch);
    const auto destination = processedFolder.getNonexistentChildFile (
        baseName, ".wav", false);
    auto snapshot = project.createSnapshot();
    advancedEditCancelRequested.store (false);
    advancedEditProgress.store (0.0f);
    audioStatusLabel.setText (
        "Pitch processing selected clip " + juce::String (semitones, 0)
            + " semitones...",
        juce::dontSendNotification);
    if (advancedEditThread.joinable())
        advancedEditThread.join();
    auto safe = juce::Component::SafePointer<MainComponent> (this);
    advancedEditThread = std::thread (
        [this, safe, snapshot = std::move (snapshot), trackId, clipId,
         semitones, destination] () mutable
    {
        AdvancedAudioRenderer::OutputInfo outputInfo;
        const auto result = AdvancedAudioRenderer::renderPitchShiftedClip (
            snapshot, trackId, clipId, semitones, destination,
            advancedEditCancelRequested, advancedEditProgress,
            outputInfo);
        juce::MessageManager::callAsync (
            [safe, result, outputInfo, destination, trackId, clipId, semitones]
        {
            if (safe == nullptr)
                return;
            safe->advancedEditInProgress.store (false);
            if (result.failed())
            {
                safe->showError ("Pitch Shift", result.getErrorMessage());
                return;
            }
            if (! safe->project.replaceClipWithProcessedAudio (
                    trackId, clipId, destination, outputInfo.sampleRate,
                    outputInfo.channels, outputInfo.lengthInSamples,
                    "Pitch Shift Clip"))
            {
                safe->showError (
                    "Pitch Shift",
                    "The clip changed before processed audio could be committed.");
                return;
            }
            safe->arrangement.refreshFromModel();
            safe->audioStatusLabel.setText (
                "Pitch shifted " + juce::String (semitones, 0)
                    + " semitones - original media retained for Undo",
                juce::dontSendNotification);
        });
    });
}

void MainComponent::generateProducerTrack (
    const juce::String& kind, const ProducerSettingsState& settings)
{
    if (isAnyRecording() || advancedEditInProgress.load()
        || exportInProgress.load())
    {
        producerAssistant.setStatus (
            "Finish recording, export, or audio processing before generating MIDI.", true);
        return;
    }

    const auto request = makeProducerRequest (settings);
    std::vector<producer::Note> generated;
    juce::String trackName;
    juce::String undoDescription;
    if (kind == "drums")
    {
        generated = producer::generateDrums (request);
        trackName = "AI Drums " + settings.style;
        undoDescription = "Producer Create Drums";
    }
    else if (kind == "melody")
    {
        generated = producer::generateMelody (request);
        trackName = "AI Melody " + producerKeyName (settings.rootPitchClass)
            + " " + settings.scale;
        undoDescription = "Producer Create Melody";
    }
    else
    {
        generated = producer::generateChords (request);
        trackName = "AI Chords " + producerKeyName (settings.rootPitchClass)
            + " " + settings.scale;
        undoDescription = "Producer Create Chords";
    }

    const auto notes = toMidiNotes (generated);
    if (notes.empty())
    {
        producerAssistant.setStatus ("The local producer created no notes.", true);
        return;
    }

    project.setProducerSettings (settings, false);
    const auto trackId = project.addGeneratedMidiTrack (
        trackName, notes, static_cast<double> (settings.bars * 4),
        undoDescription);
    if (trackId.isEmpty())
    {
        producerAssistant.setStatus ("The generated MIDI track could not be added.", true);
        return;
    }

    arrangement.refreshFromModel();
    producerAssistant.setProjectTrackCount (project.getTrackCount());
    producerAssistant.setStatus (
        trackName + " created - " + juce::String (notes.size())
            + " editable MIDI notes - Undo removes the complete result");
    audioStatusLabel.setText (trackName + " created locally",
                              juce::dontSendNotification);
}

void MainComponent::showHelpMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "Triumph Assistant    F1");
    menu.addItem (2, "Keyboard Shortcuts");
    menu.addSeparator();
    menu.addItem (3, "About Triumph Studio "
                     + juce::String (release::versionLabel));
    auto safe = juce::Component::SafePointer<MainComponent> (this);
    menu.showMenuAsync (
        juce::PopupMenu::Options().withTargetComponent (&helpButton),
        [safe] (int result)
        {
            if (safe == nullptr) return;
            if (result == 1)
                safe->setHelpAssistantOpen (true);
            else if (result == 2)
                safe->setHelpAssistantOpen (
                    true, "What are the keyboard shortcuts?");
            else if (result == 3)
                safe->showInformation (
                    "About Triumph Studio",
                    juce::String (release::applicationName) + " "
                    + release::versionLabel + "\n" + release::releaseChannel
                    + " - Engineering Preview\n"
                    "Transport-preserving restart, output diagnostics, varispeed, effects, PDC, and MIDI\n"
                    "Windows-first C++20 / JUCE 8 DAW foundation");
        });
}

void MainComponent::showMoreMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("Project");
    menu.addItem (1, "Recent Projects", recentButton.isEnabled());
    menu.addItem (2, "Collect and Save", collectButton.isEnabled());
    menu.addItem (3, "Relink Missing Media", relinkButton.isEnabled());
    menu.addItem (4, "Low Latency Monitoring", lowLatencyButton.isEnabled(),
                  lowLatencyButton.getToggleState());
    menu.addSeparator();
    menu.addSectionHeader ("Recording and Monitoring");
    const auto monitorMode = project.getInputMonitorMode();
    menu.addItem (9, "Input Monitor Off", true,
                  monitorMode == InputMonitorMode::off);
    menu.addItem (10, "Input Monitor While Armed", true,
                  monitorMode == InputMonitorMode::whileArmed);
    menu.addItem (11, "Input Monitor Always", true,
                  monitorMode == InputMonitorMode::always);
    menu.addItem (12, "Record Alignment Offset...  "
                       + juce::String (project.getManualRecordOffsetSamples())
                       + " samples");
    const auto recordingSettings = project.getRecordingSettings();
    menu.addItem (13, "Recording Setup and Input Routes...");
    menu.addItem (14, "Control Room Dim", true,
                  recordingSettings.controlRoomDimmed);
    menu.addItem (15, "Control Room Mute", true,
                  recordingSettings.controlRoomMuted);
    menu.addSeparator();
    menu.addSectionHeader ("MIDI and External Sync");
    const auto sync = project.getSyncSettings();
    menu.addItem (16, "Internal Transport", true,
                  sync.source == SyncSourceState::internal);
    menu.addItem (17, "Follow MIDI Clock", true,
                  sync.source == SyncSourceState::midiClock);
    menu.addItem (18, "Follow MIDI Time Code", true,
                  sync.source == SyncSourceState::midiTimeCode);
    menu.addItem (19, "Ableton Link (not connected)", false,
                  sync.source == SyncSourceState::link);
    menu.addItem (20, "Follow External Start/Stop", true,
                  sync.followExternalStartStop);
    menu.addItem (21, "Send MIDI Clock (output not connected)", false,
                  sync.sendMidiClock);
    menu.addSeparator();
    menu.addSectionHeader ("Diagnostics");
    menu.addItem (22, "Copy Realtime Diagnostics");
    menu.addSeparator();
    menu.addSectionHeader ("Selected Clip");
    menu.addItem (5, "Split at Playhead", splitButton.isEnabled());
    menu.addItem (6, "Use Selected Take", useTakeButton.isEnabled());
    menu.addItem (7, "Detect Transients", detectButton.isEnabled());
    menu.addItem (8, "Warp All Transients", warpAllButton.isEnabled());

    auto safe = juce::Component::SafePointer<MainComponent> (this);
    menu.showMenuAsync (
        juce::PopupMenu::Options().withTargetComponent (&moreButton),
        [safe] (int result)
        {
            if (safe == nullptr) return;
            if (result == 1) safe->showRecentProjectsMenu (&safe->moreButton);
            else if (result == 2) safe->beginCollectAndSave();
            else if (result == 3) safe->beginRelinkMissingMedia();
            else if (result == 4) safe->lowLatencyButton.triggerClick();
            else if (result == 5) safe->splitButton.triggerClick();
            else if (result == 6) safe->useTakeButton.triggerClick();
            else if (result == 7) safe->detectButton.triggerClick();
            else if (result == 8) safe->warpAllButton.triggerClick();
            else if (result == 9)
                safe->project.setInputMonitorMode (InputMonitorMode::off);
            else if (result == 10)
                safe->project.setInputMonitorMode (InputMonitorMode::whileArmed);
            else if (result == 11)
                safe->project.setInputMonitorMode (InputMonitorMode::always);
            else if (result == 12)
                safe->showRecordAlignmentDialog();
            else if (result == 13)
                safe->showRecordingSetupDialog();
            else if (result == 14)
            {
                auto settings = safe->project.getRecordingSettings();
                settings.controlRoomDimmed = ! settings.controlRoomDimmed;
                safe->project.setRecordingSettings (settings);
            }
            else if (result == 15)
            {
                auto settings = safe->project.getRecordingSettings();
                settings.controlRoomMuted = ! settings.controlRoomMuted;
                safe->project.setRecordingSettings (settings);
            }
            else if (result >= 16 && result <= 19)
            {
                auto sync = safe->project.getSyncSettings();
                sync.source = result == 17 ? SyncSourceState::midiClock
                    : result == 18 ? SyncSourceState::midiTimeCode
                    : result == 19 ? SyncSourceState::link
                                   : SyncSourceState::internal;
                safe->project.setSyncSettings (sync);
            }
            else if (result == 20)
            {
                auto sync = safe->project.getSyncSettings();
                sync.followExternalStartStop =
                    ! sync.followExternalStartStop;
                safe->project.setSyncSettings (sync);
            }
            else if (result == 22)
                safe->copyRealtimeDiagnosticsReport();
        });
}

juce::String MainComponent::buildRealtimeDiagnosticsReportText()
{
    realtime::DiagnosticsReportInput input;
    input.applicationName = release::applicationName;
    input.versionLabel = release::versionLabel;
    input.releaseChannel = release::releaseChannel;
    input.projectName = currentProjectName.toStdString();
    input.realtime = audioEngine.getRealtimeStatus();

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        input.audioDevice = device->getName().toStdString();
        input.sampleRate = device->getCurrentSampleRate();
        input.blockSize = device->getCurrentBufferSizeSamples();
    }

    const auto pluginRuntime = audioEngine.getPluginRuntimeStatus();
    input.pluginLastProcessNanoseconds =
        pluginRuntime.lastProcessNanoseconds;
    input.pluginMaximumProcessNanoseconds =
        pluginRuntime.maximumProcessNanoseconds;
    input.pluginDeadlineMisses = pluginRuntime.deadlineMisses;
    input.containedPluginExceptions = pluginRuntime.containedExceptions;
    input.instrumentContainedPluginExceptions =
        pluginRuntime.instrumentContainedExceptions;
    input.pluginAutomationDeliveryMode =
        automation::pluginParameterDeliveryName (
            automation::PluginParameterDelivery::sampleOffsetSubBlocks);

    const auto recoveryStatus = audioOutputRecovery.snapshot();
    input.audioDeviceInterruptions = recoveryStatus.interruptions;
    input.audioDeviceRecoveryAttempts = recoveryStatus.attempts;
    input.audioDeviceRecoveries = recoveryStatus.recoveries;
    input.audioDeviceRecoveryFailures = recoveryStatus.failures;
    input.callbackStallWarnings = callbackStallEvents;
    input.sustainedSilenceWarnings = sustainedSilenceEvents;

    const auto syncStatus = audioEngine.getSyncStatus();
    input.syncSource =
        syncStatus.source == midi::SyncSource::midiClock ? "MIDI Clock"
            : syncStatus.source == midi::SyncSource::midiTimeCode ? "MTC"
            : syncStatus.source == midi::SyncSource::link ? "Link"
                                                          : "Internal";
    input.syncLocked = syncStatus.locked;
    input.externalTempoBpm = syncStatus.tempoBpm;
    input.mtcPositionSeconds = syncStatus.timecodeSeconds;

    const auto report = realtime::buildDiagnosticsReport (input);
    return juce::String::fromUTF8 (report.c_str());
}

void MainComponent::copyRealtimeDiagnosticsReport()
{
    juce::SystemClipboard::copyTextToClipboard (
        buildRealtimeDiagnosticsReportText());
    audioStatusLabel.setColour (juce::Label::textColourId,
                                StudioColours::primary);
    audioStatusLabel.setText (
        "Realtime diagnostics copied to clipboard",
        juce::dontSendNotification);
}

void MainComponent::showRecordingSetupDialog()
{
    const auto settings = project.getRecordingSettings();
    const auto timelineRate = project.getTimelineSampleRate();
    std::vector<juce::String> armedAudioTrackIds;
    for (const auto& trackId : project.getArmedTrackIds())
        if (! project.getTrackState (trackId).isInstrument)
            armedAudioTrackIds.push_back (trackId);
    auto deviceSummary = juce::String (
        "No active device: software and hardware monitoring are unavailable.");
    if (const auto* device = deviceManager.getCurrentAudioDevice())
    {
        const auto capabilities = monitoring::assessDevice (
            device->getActiveInputChannels().countNumberOfSetBits(),
            device->getActiveOutputChannels().countNumberOfSetBits());
        deviceSummary = capabilities.softwareControlRoom
            ? "Software Control Room monitoring is available. "
            : "Software Control Room monitoring is unavailable. ";
        deviceSummary += capabilities.hardwareDirectControl
                                 == monitoring::HardwareDirectControl::driverManaged
            ? "Hardware direct monitoring is controlled by the interface driver. "
            : "Hardware direct monitoring is unavailable. ";
        deviceSummary += capabilities.dedicatedCueOutput
            ? "The active outputs can support a future dedicated cue bus."
            : "No dedicated cue-output pair is active.";
    }
    auto* dialog = new juce::AlertWindow (
        "Recording Setup",
        "Configure the complete audio recording session. Each armed audio "
        "track has an independent input, tap, and record offset. Hardware direct-monitor controls "
        "remain managed by the audio-interface driver; Triumph software "
        "monitoring uses the independent Control Room path.\n\n"
            + deviceSummary,
        juce::AlertWindow::NoIcon);
    dialog->addComboBox ("mode", { "Normal", "Punch", "Loop Takes" },
                         "Mode");
    dialog->getComboBoxComponent ("mode")->setSelectedId (
        settings.mode == RecordingMode::punch ? 2
            : settings.mode == RecordingMode::loop ? 3 : 1);
    juce::StringArray preRollChoices;
    for (int bars = 0; bars <= 8; ++bars)
        preRollChoices.add (juce::String (bars) + (bars == 1 ? " bar" : " bars"));
    dialog->addComboBox ("preRoll", preRollChoices, "Pre-roll");
    dialog->getComboBoxComponent ("preRoll")->setSelectedId (
        settings.preRollBars + 1);
    dialog->addTextEditor (
        "rangeStart",
        juce::String (static_cast<double> (
            settings.mode == RecordingMode::loop
                ? settings.loopStartSamples : settings.punchStartSamples)
            / timelineRate, 3),
        "Punch/loop start (seconds)");
    dialog->addTextEditor (
        "rangeEnd",
        juce::String (static_cast<double> (
            settings.mode == RecordingMode::loop
                ? settings.loopEndSamples : settings.punchEndSamples)
            / timelineRate, 3),
        "Punch/loop end (seconds)");
    for (std::size_t index = 0; index < armedAudioTrackIds.size(); ++index)
    {
        const auto track = project.getTrackState (armedAudioTrackIds[index]);
        const auto prefix = "track" + juce::String (
            static_cast<int> (index));
        dialog->addComboBox (
            prefix + "Route", { "Input 1 (mono)", "Input 2 (mono)",
                                 "Inputs 1-2 (stereo)" },
            track.name + " input");
        const auto selectedRoute = track.inputChannelCount == 2 ? 3
            : track.inputStartChannel == 1 ? 2 : 1;
        dialog->getComboBoxComponent (
            prefix + "Route")->setSelectedId (selectedRoute);
        dialog->addComboBox (
            prefix + "Tap",
            { "Hardware Input", "Device Output (post Control Room)" },
            track.name + " tap");
        dialog->getComboBoxComponent (prefix + "Tap")->setSelectedId (
            track.recordingTapPoint == RecordingTapPoint::programOutput
                ? 2 : 1);
        dialog->addTextEditor (
            prefix + "Offset", juce::String (track.recordOffsetSamples),
            track.name + " record offset (samples)");
    }
    dialog->addTextEditor (
        "controlGain", juce::String (settings.controlRoomGain, 2),
        "Control Room gain (0.00-1.50)");
    dialog->addButton ("Apply", 1,
                       juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("Cancel", 0,
                       juce::KeyPress (juce::KeyPress::escapeKey));

    auto safe = juce::Component::SafePointer<MainComponent> (this);
    dialog->enterModalState (
        true,
        juce::ModalCallbackFunction::create (
            [safe, dialog, timelineRate,
             armedAudioTrackIds = std::move (armedAudioTrackIds)] (int result)
            {
                if (safe == nullptr || result == 0)
                    return;
                const auto startSeconds = dialog->getTextEditorContents (
                    "rangeStart").getDoubleValue();
                const auto endSeconds = dialog->getTextEditorContents (
                    "rangeEnd").getDoubleValue();
                if (startSeconds < 0.0 || endSeconds < 0.0
                    || endSeconds < startSeconds)
                {
                    safe->showError (
                        "Invalid recording range",
                        "The end time must be at or after the non-negative start time.");
                    return;
                }
                auto settings = safe->project.getRecordingSettings();
                const auto modeId = dialog->getComboBoxComponent (
                    "mode")->getSelectedId();
                settings.mode = modeId == 2 ? RecordingMode::punch
                              : modeId == 3 ? RecordingMode::loop
                                            : RecordingMode::normal;
                settings.preRollBars = juce::jlimit (
                    0, 8, dialog->getComboBoxComponent (
                        "preRoll")->getSelectedId() - 1);
                const auto startSamples = static_cast<juce::int64> (
                    std::llround (startSeconds * timelineRate));
                const auto endSamples = static_cast<juce::int64> (
                    std::llround (endSeconds * timelineRate));
                if (settings.mode == RecordingMode::punch)
                {
                    settings.punchStartSamples = startSamples;
                    settings.punchEndSamples = endSamples;
                }
                else if (settings.mode == RecordingMode::loop)
                {
                    settings.loopStartSamples = startSamples;
                    settings.loopEndSamples = endSamples;
                }
                settings.controlRoomGain = juce::jlimit (
                    0.0f, 1.5f, static_cast<float> (
                        dialog->getTextEditorContents (
                            "controlGain").getDoubleValue()));
                safe->project.setRecordingSettings (settings);

                for (std::size_t index = 0;
                     index < armedAudioTrackIds.size(); ++index)
                {
                    const auto prefix = "track" + juce::String (
                        static_cast<int> (index));
                    const auto routeId = dialog->getComboBoxComponent (
                        prefix + "Route")->getSelectedId();
                    const auto firstChannel = routeId == 2 ? 1 : 0;
                    const auto channelCount = routeId == 3 ? 2 : 1;
                    const auto tap = dialog->getComboBoxComponent (
                        prefix + "Tap")->getSelectedId() == 2
                        ? RecordingTapPoint::programOutput
                        : RecordingTapPoint::hardwareInput;
                    const auto offset = juce::jlimit (
                        -8192, 8192,
                        dialog->getTextEditorContents (
                            prefix + "Offset").getIntValue());
                    safe->project.setTrackInputRoute (
                        armedAudioTrackIds[index], firstChannel,
                        channelCount);
                    safe->project.setTrackRecordingTapPoint (
                        armedAudioTrackIds[index], tap);
                    safe->project.setTrackRecordOffset (
                        armedAudioTrackIds[index], offset);
                }
                safe->audioStatusLabel.setText (
                    settings.mode == RecordingMode::punch
                        ? "Punch recording configured"
                        : settings.mode == RecordingMode::loop
                            ? "Loop-take recording configured"
                            : "Normal recording configured",
                    juce::dontSendNotification);
            }),
        true);
}

void MainComponent::showRecordAlignmentDialog()
{
    auto* dialog = new juce::AlertWindow (
        "Record Alignment Offset",
        "Enter a signed device-sample correction. Positive values place the "
        "recorded take later; negative values place it earlier. Range: -8192 to 8192.",
        juce::AlertWindow::NoIcon);
    dialog->addTextEditor (
        "samples", juce::String (project.getManualRecordOffsetSamples()),
        "Offset samples");
    dialog->addButton ("Apply", 1, juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    auto safe = juce::Component::SafePointer<MainComponent> (this);
    dialog->enterModalState (
        true,
        juce::ModalCallbackFunction::create (
            [safe, dialog] (int result)
            {
                if (safe == nullptr || result == 0)
                    return;
                const auto text = dialog->getTextEditorContents ("samples").trim();
                if (! text.containsOnly ("+-0123456789"))
                {
                    safe->showError (
                        "Invalid record offset",
                        "Enter a whole number of samples between -8192 and 8192.");
                    return;
                }
                safe->project.setManualRecordOffsetSamples (text.getIntValue());
                safe->audioStatusLabel.setText (
                    "Record offset: "
                        + juce::String (
                            safe->project.getManualRecordOffsetSamples())
                        + " device samples",
                    juce::dontSendNotification);
            }),
        true);
}

void MainComponent::setHelpAssistantOpen (
    bool shouldOpen, juce::String initialQuestion)
{
    if (shouldOpen)
    {
        mixerButton.setToggleState (false, juce::dontSendNotification);
        automationButton.setToggleState (false, juce::dontSendNotification);
        advancedEditButton.setToggleState (false, juce::dontSendNotification);
        producerButton.setToggleState (false, juce::dontSendNotification);
        mixer.setVisible (false);
        tempoAutomation.setVisible (false);
        advancedEdit.setVisible (false);
        producerAssistant.setVisible (false);
    }
    helpButton.setToggleState (shouldOpen, juce::dontSendNotification);
    helpAssistant.setVisible (shouldOpen);
    resized();
    if (shouldOpen)
    {
        if (initialQuestion.isNotEmpty())
            helpAssistant.askQuestion (std::move (initialQuestion));
        else
            helpAssistant.focusQuestion();
    }
}

void MainComponent::openHelpAction (const juce::String& action)
{
    setHelpAssistantOpen (false);

    if (action == "device")
        showAudioDeviceSettings();
    else if (action == "mixer")
        mixerButton.triggerClick();
    else if (action == "tempo")
        automationButton.triggerClick();
    else if (action == "producer")
        producerButton.triggerClick();
    else if (action == "edit")
        advancedEditButton.triggerClick();
    else if (action == "keys")
        showPianoRoll();
    else if (action == "vst3")
        showVst3Menu();
    else if (action == "export")
        handleExportButton();
    else if (action == "save")
        beginSaveProject (false);

    resized();
}

void MainComponent::applyProducerMix (
    const ProducerSettingsState& settings)
{
    if (isAnyRecording() || advancedEditInProgress.load()
        || exportInProgress.load())
    {
        producerAssistant.setStatus (
            "Finish recording, export, or audio processing before balancing the mix.", true);
        return;
    }

    const auto snapshot = project.createSnapshot();
    std::vector<producer::TrackDescriptor> descriptors;
    descriptors.reserve (snapshot.tracks.size());
    for (const auto& track : snapshot.tracks)
        descriptors.push_back ({ track.name.toStdString(), track.isInstrument });
    const auto suggestions = producer::suggestMix (descriptors);
    if (suggestions.empty())
    {
        producerAssistant.setStatus (
            "Add at least one audio or instrument track before balancing the mix.", true);
        return;
    }

    std::vector<TrackMixUpdate> updates;
    updates.reserve (suggestions.size());
    for (const auto& suggestion : suggestions)
        if (suggestion.trackIndex < snapshot.tracks.size())
            updates.push_back ({ snapshot.tracks[suggestion.trackIndex].id,
                                 suggestion.gain, suggestion.pan });
    project.setProducerSettings (settings, false);
    const auto count = project.applyTrackMixUpdates (
        updates, "Producer Balance Mix");
    if (count <= 0)
    {
        producerAssistant.setStatus ("No valid tracks were available to balance.", true);
        return;
    }

    arrangement.refreshFromModel();
    producerAssistant.setStatus (
        "Balanced " + juce::String (count)
            + " tracks for headroom and stereo separation - Undo restores the prior mix");
    audioStatusLabel.setText (
        "Local Producer balanced " + juce::String (count) + " tracks",
        juce::dontSendNotification);
}

void MainComponent::updateProjectToolbarState()
{
    recentButton.setEnabled (workspace.getRecentProjectCount() > 0);
    collectButton.setEnabled (project.getMediaSourceCount() > 0);
    const auto missingCount = workspace.countMissingMedia (project);
    relinkButton.setEnabled (missingCount > 0);
    relinkButton.setButtonText (missingCount > 0
                                    ? juce::String ("Relink ") + juce::String (missingCount)
                                    : "Relink");
    pianoRollButton.setEnabled (project.getFirstInstrumentTrackId().isNotEmpty());
    juce::String pluginStateLabel { "VST3" };
    bool foundFrozen = false;
    for (int index = 0; index < project.getTrackCount(); ++index)
    {
        const auto track = project.getTrackState (project.getTrackId (index));
        if (track.pluginDescriptionXml.isEmpty())
            continue;
        if (track.frozen)
        {
            foundFrozen = true;
            continue;
        }
        pluginStateLabel = track.pluginBypassed ? "VST3 Bypass"
                         : ! track.pluginAvailable ? "VST3 Safe"
                         : audioEngine.hasInstrumentPlugin() ? "VST3 Live" : "VST3";
        foundFrozen = false;
        break;
    }
    if (pluginStateLabel == "VST3" && foundFrozen)
        pluginStateLabel = "VST3 Frozen";
    pluginButton.setButtonText (pluginStateLabel);
    juce::String pluginTooltip { "Load and manage the project instrument plug-in" };
    if (pluginStateLabel != "VST3")
        pluginTooltip = "Project instrument state: "
            + pluginStateLabel.fromFirstOccurrenceOf ("VST3 ", false, false);
    pluginButton.setTooltip (pluginTooltip + " - click to manage plug-ins");
    const auto graphLatency = audioEngine.getGraphLatencySamples();
    lowLatencyButton.setTooltip (
        (project.getLowLatencyMonitoring()
             ? juce::String ("Low-latency monitoring is active")
             : juce::String ("Full delay compensation is active"))
        + " - graph latency " + juce::String (graphLatency) + " samples");
}

juce::String MainComponent::formatTime (double seconds) const
{
    const auto milliseconds = juce::jmax (0, juce::roundToInt (seconds * 1000.0));
    const auto minutes = milliseconds / 60000;
    const auto remainingSeconds = (milliseconds / 1000) % 60;
    const auto remainingMilliseconds = milliseconds % 1000;

    return juce::String (minutes).paddedLeft ('0', 2)
           + ":" + juce::String (remainingSeconds).paddedLeft ('0', 2)
           + "." + juce::String (remainingMilliseconds).paddedLeft ('0', 3);
}

void MainComponent::showError (const juce::String& title, const juce::String& message)
{
    juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                            title,
                                            message,
                                            "OK",
                                            this);
}

void MainComponent::showInformation (const juce::String& title,
                                     const juce::String& message)
{
    juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
                                            title,
                                            message,
                                            "OK",
                                            this);
}
}
