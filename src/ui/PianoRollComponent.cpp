#include "PianoRollComponent.h"

#include "StudioLookAndFeel.h"
#include "core/TempoMap.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace triumph
{
PianoRollComponent::PianoRollComponent (ProjectModel& projectModel,
                                        juce::String instrumentTrackId,
                                        juce::String midiClipId,
                                        AuditionCallback auditionCallback)
    : project (projectModel),
      trackId (std::move (instrumentTrackId)),
      clipId (std::move (midiClipId)),
      onAudition (std::move (auditionCallback))
{
    quantizeButton.setColour (juce::TextButton::buttonColourId, StudioColours::primary);
    quantizeButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    quantizeButton.onClick = [this]
    {
        project.quantizeMidiClip (trackId, clipId, 0.25);
        repaint();
    };
    addAndMakeVisible (quantizeButton);

    expressionTypeBox.addItem ("Pitch", 1);
    expressionTypeBox.addItem ("Pressure", 2);
    expressionTypeBox.addItem ("Timbre", 3);
    expressionTypeBox.setSelectedId (1, juce::dontSendNotification);
    addAndMakeVisible (expressionTypeBox);
    expressionValueSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    expressionValueSlider.setTextBoxStyle (
        juce::Slider::TextBoxRight, false, 48, 20);
    expressionValueSlider.setRange (-1.0, 1.0, 0.001);
    expressionValueSlider.setValue (0.0, juce::dontSendNotification);
    addAndMakeVisible (expressionValueSlider);
    addExpressionButton.onClick = [this]
    {
        const auto clip = getClip();
        const auto* note = findSelectedNote (clip);
        if (note == nullptr)
            return;
        auto expression = note->expression;
        MidiNoteState::ExpressionPoint point;
        point.offsetBeats = juce::jlimit (
            0.0, note->lengthBeats, selectedExpressionOffsetBeats);
        point.value = static_cast<float> (expressionValueSlider.getValue());
        point.type = expressionTypeBox.getSelectedId() == 2
            ? MidiNoteState::ExpressionPoint::Type::pressure
            : expressionTypeBox.getSelectedId() == 3
                ? MidiNoteState::ExpressionPoint::Type::timbre
                : MidiNoteState::ExpressionPoint::Type::pitch;
        expression.push_back (point);
        project.setMidiNoteExpression (
            trackId, clipId, selectedNoteId, std::move (expression));
        refreshSelectionLabel();
    };
    addAndMakeVisible (addExpressionButton);

    for (const auto controller : { 1, 11, 64, 74 })
        controllerBox.addItem ("CC " + juce::String (controller),
                               controller + 1);
    controllerBox.setSelectedId (2, juce::dontSendNotification);
    addAndMakeVisible (controllerBox);
    controllerValueSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    controllerValueSlider.setTextBoxStyle (
        juce::Slider::TextBoxRight, false, 48, 20);
    controllerValueSlider.setRange (0.0, 1.0, 0.001);
    controllerValueSlider.setValue (0.5, juce::dontSendNotification);
    addAndMakeVisible (controllerValueSlider);
    addControllerButton.onClick = [this]
    {
        const auto clip = getClip();
        const auto* note = findSelectedNote (clip);
        MidiControllerEventState event;
        event.beat = juce::jlimit (0.0, clip.lengthBeats, controllerBeat);
        event.channel = note != nullptr ? note->channel : 1;
        event.noteId = note != nullptr ? note->noteId : 0;
        event.controller = juce::jmax (0, controllerBox.getSelectedId() - 1);
        event.value = static_cast<std::uint32_t> (std::llround (
            controllerValueSlider.getValue()
                * static_cast<double> (
                    std::numeric_limits<std::uint32_t>::max())));
        project.addMidiControllerEvent (trackId, clipId, event);
        refreshSelectionLabel();
    };
    addAndMakeVisible (addControllerButton);

    selectionLabel.setColour (juce::Label::textColourId,
                              StudioColours::primaryBright);
    selectionLabel.getProperties().set ("fontSize", 10.0f);
    addAndMakeVisible (selectionLabel);

    instructionLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);
    instructionLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (instructionLabel);
    refreshSelectionLabel();
    setSize (920, 580);
    startTimerHz (15);
}

PianoRollComponent::~PianoRollComponent()
{
    stopAudition();
    stopTimer();
}

MidiClipState PianoRollComponent::getClip() const
{
    const auto track = project.getTrackState (trackId);
    for (const auto& clip : track.midiClips)
        if (clip.id == clipId)
            return clip;
    return {};
}

const MidiNoteState* PianoRollComponent::findSelectedNote (
    const MidiClipState& clip) const
{
    const auto match = std::find_if (
        clip.notes.begin(), clip.notes.end(), [this] (const auto& note)
    {
        return note.id == selectedNoteId;
    });
    return match != clip.notes.end() ? &*match : nullptr;
}

void PianoRollComponent::refreshSelectionLabel()
{
    const auto clip = getClip();
    const auto* note = findSelectedNote (clip);
    selectionLabel.setText (
        note != nullptr
            ? juce::MidiMessage::getMidiNoteName (
                  note->noteNumber, true, true, 3)
                + "  MPE " + juce::String (note->expression.size())
                + "  MIDI2 CC " + juce::String (clip.controllers.size())
            : "Select a note for per-note expression",
        juce::dontSendNotification);
    addExpressionButton.setEnabled (note != nullptr);
}

juce::Rectangle<int> PianoRollComponent::getKeyboardBounds() const
{
    auto editor = getLocalBounds().withTrimmedTop (82).reduced (8);
    return editor.removeFromLeft (64);
}

juce::Rectangle<int> PianoRollComponent::getGridBounds() const
{
    auto editor = getLocalBounds().withTrimmedTop (82).reduced (8);
    editor.removeFromLeft (64);
    return editor;
}

juce::Rectangle<int> PianoRollComponent::getNoteBounds (const MidiNoteState& note) const
{
    const auto grid = getGridBounds();
    const auto clip = getClip();
    const auto beatWidth = grid.getWidth() / juce::jmax (1.0, clip.lengthBeats);
    const auto rowHeight = static_cast<double> (grid.getHeight())
                           / (highestNote - lowestNote + 1);
    return { juce::roundToInt (grid.getX() + note.startBeat * beatWidth),
             juce::roundToInt (grid.getY() + (highestNote - note.noteNumber) * rowHeight),
             juce::jmax (3, juce::roundToInt (note.lengthBeats * beatWidth)),
             juce::jmax (3, juce::roundToInt (rowHeight)) };
}

void PianoRollComponent::paint (juce::Graphics& graphics)
{
    graphics.fillAll (StudioColours::background);
    const auto keyboard = getKeyboardBounds();
    const auto grid = getGridBounds();

    const auto rowHeight = static_cast<double> (grid.getHeight())
                           / (highestNote - lowestNote + 1);
    graphics.setColour (juce::Colours::white.withAlpha (0.94f));
    graphics.fillRect (keyboard);
    for (int note = lowestNote; note <= highestNote; ++note)
    {
        const auto y = juce::roundToInt (grid.getY()
                                         + (highestNote - note) * rowHeight);
        const auto height = juce::jmax (1, juce::roundToInt (rowHeight));
        const auto black = juce::MidiMessage::isMidiNoteBlack (note);
        auto key = juce::Rectangle<int> (keyboard.getX(), y,
                                         keyboard.getWidth(), height);
        if (black)
        {
            key = key.withTrimmedLeft (juce::roundToInt (
                keyboard.getWidth() * 0.34f));
            graphics.setColour (StudioColours::panel.darker (0.72f));
            graphics.fillRect (key);
        }
        else
        {
            graphics.setColour (juce::Colours::white.withAlpha (0.94f));
            graphics.fillRect (key);
            graphics.setColour (StudioColours::border.withAlpha (0.72f));
            graphics.drawHorizontalLine (key.getBottom(),
                                         static_cast<float> (key.getX()),
                                         static_cast<float> (key.getRight()));
        }

        if (note == auditionedNote)
        {
            graphics.setColour (StudioColours::primary.withAlpha (0.82f));
            graphics.fillRect (key);
        }

        if (! black && note % 12 == 0)
        {
            graphics.setColour (juce::Colours::black.withAlpha (0.70f));
            graphics.setFont (9.0f);
            graphics.drawText (juce::MidiMessage::getMidiNoteName (
                                   note, true, true, 3),
                               key.reduced (4, 0),
                               juce::Justification::centredLeft);
        }
    }
    graphics.setColour (StudioColours::border);
    graphics.drawRect (keyboard);

    graphics.setColour (StudioColours::surface);
    graphics.fillRect (grid);

    for (int note = lowestNote; note <= highestNote; ++note)
    {
        const auto y = juce::roundToInt (grid.getY()
                                         + (highestNote - note) * rowHeight);
        const auto isAccidental = juce::MidiMessage::isMidiNoteBlack (note);
        if (isAccidental)
        {
            graphics.setColour (StudioColours::panel.withAlpha (0.62f));
            graphics.fillRect (grid.getX(), y, grid.getWidth(),
                               juce::jmax (1, juce::roundToInt (rowHeight)));
        }
        graphics.setColour (StudioColours::border.withAlpha (0.35f));
        graphics.drawHorizontalLine (y,
                                     static_cast<float> (grid.getX()),
                                     static_cast<float> (grid.getRight()));
    }

    const auto clip = getClip();
    const auto beatWidth = grid.getWidth() / juce::jmax (1.0, clip.lengthBeats);
    for (double beat = 0.0; beat <= clip.lengthBeats; beat += 0.25)
    {
        const auto x = juce::roundToInt (grid.getX() + beat * beatWidth);
        const auto wholeBeat = std::abs (std::round (beat) - beat) < 0.001;
        graphics.setColour (StudioColours::border.withAlpha (wholeBeat ? 0.58f : 0.22f));
        graphics.drawVerticalLine (x,
                                   static_cast<float> (grid.getY()),
                                   static_cast<float> (grid.getBottom()));
    }

    for (const auto& note : clip.notes)
    {
        const auto bounds = getNoteBounds (note).reduced (1);
        graphics.setColour (StudioColours::primary.withAlpha (
            juce::jmap (note.velocity, 0.0f, 1.0f, 0.45f, 0.95f)));
        graphics.fillRoundedRectangle (bounds.toFloat(), 3.0f);
        graphics.setColour (StudioColours::primary.darker (0.3f));
        graphics.drawRoundedRectangle (
            bounds.toFloat(), 3.0f,
            note.id == selectedNoteId ? 2.5f : 1.0f);
    }

    graphics.setColour (StudioColours::border);
    graphics.drawRect (grid);
}

void PianoRollComponent::resized()
{
    auto toolbar = getLocalBounds().removeFromTop (82).reduced (10, 5);
    auto top = toolbar.removeFromTop (32);
    quantizeButton.setBounds (top.removeFromRight (130).reduced (2));
    instructionLabel.setBounds (top);
    auto expression = toolbar.removeFromTop (32);
    selectionLabel.setBounds (expression.removeFromLeft (180));
    expressionTypeBox.setBounds (expression.removeFromLeft (84).reduced (2));
    expressionValueSlider.setBounds (
        expression.removeFromLeft (150).reduced (2));
    addExpressionButton.setBounds (
        expression.removeFromLeft (100).reduced (2));
    controllerBox.setBounds (expression.removeFromLeft (74).reduced (2));
    controllerValueSlider.setBounds (
        expression.removeFromLeft (150).reduced (2));
    addControllerButton.setBounds (
        expression.removeFromLeft (100).reduced (2));
}

int PianoRollComponent::noteAtY (float y) const
{
    const auto grid = getGridBounds();
    const auto proportion = juce::jlimit (
        0.0, 0.999999,
        static_cast<double> (y - grid.getY()) / grid.getHeight());
    return juce::jlimit (lowestNote, highestNote,
                         highestNote - static_cast<int> (
                             proportion * (highestNote - lowestNote + 1)));
}

double PianoRollComponent::beatAtX (float x) const
{
    const auto grid = getGridBounds();
    const auto clip = getClip();
    const auto raw = juce::jlimit (
        0.0, clip.lengthBeats,
        static_cast<double> (x - grid.getX()) / grid.getWidth()
            * clip.lengthBeats);
    return tempo::quantizeBeat (raw, 0.25);
}

void PianoRollComponent::mouseDoubleClick (const juce::MouseEvent& event)
{
    if (! getGridBounds().contains (event.position.toInt()))
        return;
    const auto clip = getClip();
    const auto beat = beatAtX (event.position.x);
    if (beat >= clip.lengthBeats)
        return;
    selectedNoteId = project.addMidiNote (
        trackId, clipId, beat,
        juce::jmin (1.0, clip.lengthBeats - beat),
        noteAtY (event.position.y));
    selectedExpressionOffsetBeats = 0.0;
    controllerBeat = beat;
    refreshSelectionLabel();
    repaint();
}

void PianoRollComponent::mouseDown (const juce::MouseEvent& event)
{
    if (getKeyboardBounds().contains (event.position.toInt()))
    {
        auditionGestureActive = true;
        startAudition (noteAtY (event.position.y));
        return;
    }

    const auto clip = getClip();
    for (auto iterator = clip.notes.rbegin(); iterator != clip.notes.rend(); ++iterator)
    {
        if (getNoteBounds (*iterator).contains (event.position.toInt()))
        {
            if (event.mods.isPopupMenu())
            {
                project.removeMidiNote (trackId, clipId, iterator->id);
                if (selectedNoteId == iterator->id)
                    selectedNoteId.clear();
            }
            else
            {
                selectedNoteId = iterator->id;
                const auto clickBeat = beatAtX (event.position.x);
                selectedExpressionOffsetBeats = juce::jlimit (
                    0.0, iterator->lengthBeats,
                    clickBeat - iterator->startBeat);
                controllerBeat = clickBeat;
            }
            refreshSelectionLabel();
            repaint();
            return;
        }
    }
    if (! event.mods.isPopupMenu()
        && getGridBounds().contains (event.position.toInt()))
    {
        selectedNoteId.clear();
        controllerBeat = beatAtX (event.position.x);
        refreshSelectionLabel();
        repaint();
    }
}

void PianoRollComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (! auditionGestureActive)
        return;

    if (getKeyboardBounds().expanded (24, 0)
            .contains (event.position.toInt()))
        startAudition (noteAtY (event.position.y));
    else
        stopAudition();
}

void PianoRollComponent::mouseUp (const juce::MouseEvent&)
{
    auditionGestureActive = false;
    stopAudition();
}

void PianoRollComponent::mouseExit (const juce::MouseEvent&)
{
    if (! juce::ModifierKeys::getCurrentModifiersRealtime()
             .isAnyMouseButtonDown())
    {
        auditionGestureActive = false;
        stopAudition();
    }
}

void PianoRollComponent::startAudition (int noteNumber)
{
    noteNumber = juce::jlimit (lowestNote, highestNote, noteNumber);
    if (noteNumber == auditionedNote)
        return;

    stopAudition();
    auditionedNote = noteNumber;
    if (onAudition)
        onAudition (auditionedNote, 0.82f);
    repaint (getKeyboardBounds());
}

void PianoRollComponent::stopAudition()
{
    if (auditionedNote < 0)
        return;

    const auto note = auditionedNote;
    auditionedNote = -1;
    if (onAudition)
        onAudition (note, 0.0f);
    repaint (getKeyboardBounds());
}

void PianoRollComponent::timerCallback()
{
    repaint();
}
}
