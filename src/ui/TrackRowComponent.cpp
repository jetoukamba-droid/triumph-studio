#include "TrackRowComponent.h"

#include "core/TimelineMath.h"
#include "core/TempoMap.h"
#include "core/TimeStretch.h"
#include "StudioLookAndFeel.h"

#include <cmath>
#include <utility>

namespace triumph
{
TrackRowComponent::TrackRowComponent (ProjectModel& projectModel,
                                      AudioEngine& sharedEngine,
                                      juce::String stableTrackId)
    : project (projectModel),
      engine (sharedEngine),
      trackId (std::move (stableTrackId))
{
    nameLabel.getProperties().set ("fontSize", 14.0f);
    nameLabel.getProperties().set ("bold", true);
    nameLabel.setEditable (true, true, false);
    nameLabel.onTextChange = [this]
    {
        project.setTrackName (trackId, nameLabel.getText());
    };
    addAndMakeVisible (nameLabel);

    infoLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);
    infoLabel.getProperties().set ("fontSize", 11.0f);
    addAndMakeVisible (infoLabel);

    muteButton.setClickingTogglesState (true);
    muteButton.setColour (juce::TextButton::buttonOnColourId, StudioColours::red);
    muteButton.onClick = [this]
    {
        project.setTrackMuted (trackId, muteButton.getToggleState());
    };
    addAndMakeVisible (muteButton);

    soloButton.setClickingTogglesState (true);
    soloButton.setColour (juce::TextButton::buttonOnColourId, StudioColours::orange);
    soloButton.onClick = [this]
    {
        project.setTrackSolo (trackId, soloButton.getToggleState());
    };
    addAndMakeVisible (soloButton);

    recordButton.setClickingTogglesState (true);
    recordButton.setColour (juce::TextButton::buttonOnColourId, StudioColours::red);
    recordButton.onClick = [this]
    {
        project.setTrackRecordArmed (trackId, recordButton.getToggleState());
    };
    addAndMakeVisible (recordButton);

    instrumentSelector.addItem ("Triumph Piano", 1);
    instrumentSelector.addItem ("Triumph Synth", 2);
    instrumentSelector.addItem ("Triumph Drums", 3);
    instrumentSelector.addItem ("Triumph Sampler", 4);
    instrumentSelector.setTooltip (
        "Change this track's built-in instrument without deleting the track");
    instrumentSelector.onChange = [this]
    {
        if (instrumentSelector.getSelectedId() <= 0)
            return;
        project.setBuiltInInstrument (trackId, instrumentSelector.getText());
    };
    addAndMakeVisible (instrumentSelector);

    gainLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);
    gainLabel.getProperties().set ("fontSize", 10.0f);
    addAndMakeVisible (gainLabel);

    gainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 46, 20);
    gainSlider.setRange (0.0, 1.5, 0.01);
    gainSlider.setTextValueSuffix ("x");
    gainSlider.onDragStart = [this]
    {
        project.beginUndoTransaction ("Adjust Track Volume");
    };
    gainSlider.onValueChange = [this]
    {
        project.setTrackGain (trackId, static_cast<float> (gainSlider.getValue()));
    };
    addAndMakeVisible (gainSlider);

    panLabel.setColour (juce::Label::textColourId, StudioColours::textMuted);
    panLabel.getProperties().set ("fontSize", 10.0f);
    addAndMakeVisible (panLabel);

    panSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    panSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 46, 20);
    panSlider.setRange (-1.0, 1.0, 0.01);
    panSlider.onDragStart = [this]
    {
        project.beginUndoTransaction ("Adjust Track Pan");
    };
    panSlider.onValueChange = [this]
    {
        project.setTrackPan (trackId, static_cast<float> (panSlider.getValue()));
    };
    addAndMakeVisible (panSlider);

    latencyLabel.setColour (juce::Label::textColourId,
                            StudioColours::textMuted);
    latencyLabel.getProperties().set ("fontSize", 9.0f);
    addAndMakeVisible (latencyLabel);

    latencySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    latencySlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 20);
    latencySlider.setRange (-262143.0, 262143.0, 1.0);
    latencySlider.setTextValueSuffix (" smp");
    latencySlider.setTooltip (
        "Correct missing or inaccurate reported path latency in samples");
    latencySlider.onDragStart = [this]
    {
        project.beginUndoTransaction ("Adjust Manual Latency Offset");
    };
    latencySlider.onValueChange = [this]
    {
        project.setTrackManualLatencyOffset (
            trackId, juce::roundToInt (latencySlider.getValue()),
            ! latencySlider.isMouseButtonDown());
    };
    addAndMakeVisible (latencySlider);

    // Track actions live in the contextual menu, as they do in established
    // arrangement views. Register the row as a listener for its child controls
    // so right-clicking the name, status, or mix controls opens the same menu.
    nameLabel.addMouseListener (this, false);
    infoLabel.addMouseListener (this, false);
    muteButton.addMouseListener (this, false);
    soloButton.addMouseListener (this, false);
    recordButton.addMouseListener (this, false);
    instrumentSelector.addMouseListener (this, false);
    gainLabel.addMouseListener (this, false);
    gainSlider.addMouseListener (this, false);
    panLabel.addMouseListener (this, false);
    panSlider.addMouseListener (this, false);
    latencyLabel.addMouseListener (this, false);
    latencySlider.addMouseListener (this, false);

    refreshFromModel();
    startTimerHz (30);
}

TrackRowComponent::~TrackRowComponent()
{
    stopTimer();
}

const juce::String& TrackRowComponent::getTrackId() const noexcept
{
    return trackId;
}

int TrackRowComponent::getPreferredHeight() const
{
    auto highestLane = 0;
    for (const auto& clip : project.getTrackState (trackId).clips)
        highestLane = juce::jmax (highestLane, clip.takeLaneIndex);
    return userHeight + (hasTakeLanes() ? (highestLane + 1) * 32 : 0);
}

void TrackRowComponent::refreshFromModel()
{
    const auto track = project.getTrackState (trackId);
    nameLabel.setText (track.name, juce::dontSendNotification);
    muteButton.setToggleState (track.muted, juce::dontSendNotification);
    soloButton.setToggleState (track.solo, juce::dontSendNotification);
    recordButton.setToggleState (track.recordArmed, juce::dontSendNotification);
    recordButton.setEnabled (true);
    gainSlider.setValue (track.gain, juce::dontSendNotification);
    panSlider.setValue (track.pan, juce::dontSendNotification);
    latencySlider.setValue (track.manualLatencyOffsetSamples,
                            juce::dontSendNotification);
    populateInstrumentSelector (track);

    if (track.isInstrument)
    {
        int noteCount = 0;
        for (const auto& clip : track.midiClips)
            noteCount += static_cast<int> (clip.notes.size());
        infoLabel.setText ((track.pluginName.isNotEmpty()
                                ? track.pluginName.toUpperCase()
                                : juce::String ("TRIUMPH INSTRUMENT"))
                               + "  /  "
                               + juce::String (noteCount) + " NOTE"
                               + (noteCount == 1 ? "" : "S"),
                           juce::dontSendNotification);
    }
    else if (const auto* audioTrack = engine.findTrack (trackId))
    {
        auto takeCount = 0;
        for (const auto& clip : track.clips)
            if (clip.takeGroupId.isNotEmpty())
                ++takeCount;
        infoLabel.setText (formatDuration (audioTrack->getLengthSeconds())
                               + "  /  " + juce::String (audioTrack->getSourceChannelCount())
                               + " CH  /  "
                               + (takeCount > 1
                                      ? juce::String (takeCount) + " TAKES"
                                      : juce::String (static_cast<int> (track.clips.size()))
                                            + " CLIP" + (track.clips.size() == 1 ? "" : "S")),
                           juce::dontSendNotification);
    }
    else if (track.clips.empty())
        infoLabel.setText (track.recordArmed ? "ARMED  /  READY TO RECORD"
                                             : "EMPTY AUDIO TRACK",
                           juce::dontSendNotification);
    else
        infoLabel.setText ("SOURCE OFFLINE", juce::dontSendNotification);

    repaint();
}

void TrackRowComponent::setTimelineView (double newPixelsPerSecond,
                                         bool shouldSnap,
                                         juce::int64 newGridSamples)
{
    pixelsPerSecond = juce::jlimit (30.0, 320.0, newPixelsPerSecond);
    snapEnabled = shouldSnap;
    gridSamples = juce::jmax (static_cast<juce::int64> (1), newGridSamples);
    repaint();
}

void TrackRowComponent::setSelectedClip (const juce::String& clipId)
{
    if (selectedClipId != clipId)
    {
        selectedClipId = clipId;
        repaint();
    }
}

void TrackRowComponent::setTrackSelected (bool shouldBeSelected)
{
    if (trackSelected != shouldBeSelected)
    {
        trackSelected = shouldBeSelected;
        repaint (0, 0, controlWidth + 2, getHeight());
    }
}

void TrackRowComponent::setClipSelectionCallback (ClipSelectionCallback callback)
{
    onClipSelected = std::move (callback);
}

void TrackRowComponent::setTrackDeleteRequestCallback (
    TrackDeleteRequestCallback callback)
{
    onTrackDeleteRequested = std::move (callback);
}

void TrackRowComponent::setTrackHeightChangeCallback (
    TrackHeightChangeCallback callback)
{
    onTrackHeightChanged = std::move (callback);
}

void TrackRowComponent::setUserHeight (int newHeight)
{
    userHeight = juce::jlimit (preferredHeight, 220, newHeight);
}

void TrackRowComponent::paint (juce::Graphics& graphics)
{
    const auto track = project.getTrackState (trackId);
    const auto bounds = getLocalBounds().toFloat();
    graphics.setColour (trackSelected ? StudioColours::panelRaised
                                      : StudioColours::panel);
    graphics.fillRect (bounds);

    if (trackSelected)
    {
        graphics.setColour (StudioColours::selection.withAlpha (0.42f));
        graphics.fillRect (
            bounds.withWidth (static_cast<float> (controlWidth)).reduced (2.0f));
    }

    graphics.setColour (track.colour);
    graphics.fillRect (bounds.withWidth (5.0f).reduced (1.0f));
    graphics.setColour (StudioColours::border);
    graphics.drawRect (bounds.reduced (0.5f), 1.0f);

    if (track.recordArmed)
    {
        graphics.setColour (StudioColours::red.withAlpha (0.75f));
        graphics.drawRoundedRectangle (bounds.reduced (2.0f), 5.0f, 2.0f);
    }
    graphics.drawVerticalLine (controlWidth, 0.0f, static_cast<float> (getHeight()));

    // A compact track-type badge mirrors the visual hierarchy of professional
    // arrangement views without consuming a permanent action-button slot.
    const juce::Rectangle<float> typeBadge { 13.0f, 12.0f, 25.0f, 25.0f };
    graphics.setColour (track.colour.withAlpha (0.22f));
    graphics.fillRoundedRectangle (typeBadge, 5.0f);
    graphics.setColour (track.colour.brighter (0.34f));
    graphics.drawRoundedRectangle (typeBadge, 5.0f, 1.0f);
    if (track.isInstrument)
    {
        for (int key = 0; key < 5; ++key)
        {
            const auto keyArea = juce::Rectangle<float> (
                typeBadge.getX() + 4.0f + key * 3.4f,
                typeBadge.getY() + 7.0f,
                3.0f,
                11.0f);
            graphics.setColour ((key == 1 || key == 3)
                                    ? StudioColours::primaryBright
                                    : StudioColours::text);
            graphics.fillRoundedRectangle (keyArea, 0.8f);
        }
    }
    else
    {
        juce::Path waveform;
        waveform.startNewSubPath (typeBadge.getX() + 4.0f,
                                  typeBadge.getCentreY());
        waveform.lineTo (typeBadge.getX() + 7.0f,
                         typeBadge.getCentreY() - 5.0f);
        waveform.lineTo (typeBadge.getX() + 10.0f,
                         typeBadge.getCentreY() + 5.0f);
        waveform.lineTo (typeBadge.getX() + 13.0f,
                         typeBadge.getCentreY() - 7.0f);
        waveform.lineTo (typeBadge.getX() + 16.0f,
                         typeBadge.getCentreY() + 4.0f);
        waveform.lineTo (typeBadge.getRight() - 4.0f,
                         typeBadge.getCentreY());
        graphics.setColour (StudioColours::primaryBright);
        graphics.strokePath (waveform, juce::PathStrokeType (1.7f));
    }

    auto meterArea = juce::Rectangle<float> (
        static_cast<float> (controlWidth - 44),
        static_cast<float> (getHeight() - 19),
        34.0f,
        4.0f);
    graphics.setColour (StudioColours::surface);
    graphics.fillRoundedRectangle (meterArea, 1.5f);
    graphics.setColour (StudioColours::green.withAlpha (track.muted ? 0.25f : 0.85f));
    graphics.fillRoundedRectangle (meterArea.withWidth (
        juce::jlimit (5.0f, meterArea.getWidth(),
                      meterArea.getWidth() * juce::jlimit (0.12f, 1.0f, track.gain))),
        1.5f);
    const auto knob = juce::Rectangle<float> (
        static_cast<float> (controlWidth - 39),
        static_cast<float> (getHeight() - 41),
        18.0f,
        18.0f);
    graphics.setColour (StudioColours::surface);
    graphics.fillEllipse (knob);
    graphics.setColour (StudioColours::primary);
    graphics.drawEllipse (knob, 1.0f);
    graphics.drawLine (knob.getCentreX(), knob.getCentreY(),
                       knob.getCentreX() + 5.0f, knob.getCentreY() - 5.0f, 1.3f);

    const auto timelineArea = getTimelineArea();
    graphics.setColour (StudioColours::workspace);
    graphics.fillRect (timelineArea);
    graphics.setColour (StudioColours::border.withAlpha (0.26f));
    for (int y = timelineArea.getY() + 22; y < timelineArea.getBottom(); y += 22)
        graphics.drawHorizontalLine (y,
                                     static_cast<float> (timelineArea.getX()),
                                     static_cast<float> (timelineArea.getRight()));

    const auto timelineRate = project.getTimelineSampleRate();
    auto visibleGridSamples = gridSamples;
    while (timeline::samplesToSeconds (visibleGridSamples, timelineRate)
               * pixelsPerSecond < 24.0)
        visibleGridSamples *= 2;

    const auto gridSeconds = timeline::samplesToSeconds (visibleGridSamples, timelineRate);
    if (gridSeconds > 0.0)
    {
        graphics.setColour (StudioColours::border.withAlpha (0.42f));
        for (double seconds = gridSeconds;
             timelineArea.getX() + seconds * pixelsPerSecond < timelineArea.getRight();
             seconds += gridSeconds)
        {
            const auto x = juce::roundToInt (timelineArea.getX()
                                             + seconds * pixelsPerSecond);
            graphics.drawVerticalLine (x,
                                       static_cast<float> (timelineArea.getY()),
                                       static_cast<float> (timelineArea.getBottom()));
        }
    }

    auto* audioTrack = engine.findTrack (trackId);

    if (hasTakeLanes())
    {
        auto compLane = getCompLaneArea();
        graphics.setColour (StudioColours::surface.withAlpha (0.88f));
        graphics.fillRect (compLane);
        graphics.setColour (StudioColours::textMuted);
        graphics.setFont (10.0f);
        graphics.drawText ("COMP", compLane.removeFromLeft (42).reduced (6, 2),
                           juce::Justification::centredLeft, false);
        for (const auto& region : track.compRegions)
        {
            const auto x = getTimelineArea().getX()
                + timeline::samplesToSeconds (region.timelineStartSamples, timelineRate)
                    * pixelsPerSecond;
            const auto width = timeline::samplesToSeconds (
                region.lengthInSamples, timelineRate) * pixelsPerSecond;
            juce::Rectangle<float> regionArea {
                static_cast<float> (x), static_cast<float> (getCompLaneArea().getY()),
                static_cast<float> (juce::jmax (4.0, width)),
                static_cast<float> (getCompLaneArea().getHeight())
            };
            graphics.setColour (track.colour.withAlpha (0.42f));
            graphics.fillRoundedRectangle (regionArea, 4.0f);
            graphics.setColour (StudioColours::primary);
            graphics.drawRoundedRectangle (regionArea.reduced (0.5f), 4.0f, 1.5f);
            if (regionArea.getWidth() > 54.0f)
            {
                graphics.setColour (StudioColours::text);
                graphics.drawText ("TAKE " + juce::String (region.takeLaneIndex + 1),
                                   regionArea.toNearestInt().reduced (6, 2),
                                   juce::Justification::centredLeft, false);
            }
        }
    }

    for (const auto& modelClip : track.clips)
    {
        const auto previewActive = dragOriginClip.id.isNotEmpty()
                                   && modelClip.id == dragOriginClip.id
                                   && dragMode != DragMode::none
                                   && dragMode != DragMode::seek
                                   && dragMode != DragMode::compRange;
        const auto& clip = previewActive ? dragPreviewClip : modelClip;
        const auto clipArea = getClipBounds (clip);
        if (! clipArea.intersects (timelineArea))
            continue;

        const auto selected = clip.id == selectedClipId;
        const auto clipColour = clip.colour.isTransparent() ? track.colour
                                                             : clip.colour;
        const auto takeAlpha = (clip.activeTake ? 1.0f : 0.48f)
                               * (clip.muted ? 0.40f : 1.0f);
        const auto eventHeaderHeight = juce::jlimit (15, 22,
                                                     clipArea.getHeight() / 4);
        auto eventHeader = clipArea;
        eventHeader.setHeight (eventHeaderHeight);
        auto eventBody = clipArea.withTrimmedTop (eventHeaderHeight);

        graphics.setColour (clipColour.withAlpha (
            (selected ? 0.58f : 0.42f) * takeAlpha));
        graphics.fillRoundedRectangle (clipArea.toFloat(), 5.0f);
        graphics.setColour (clipColour.darker (0.34f).withAlpha (
            (selected ? 0.98f : 0.90f) * takeAlpha));
        graphics.fillRoundedRectangle (eventHeader.toFloat(), 5.0f);
        graphics.fillRect (eventHeader.withTrimmedTop (
            juce::jmax (0, eventHeader.getHeight() - 5)));

        if (audioTrack != nullptr && eventBody.getWidth() > 12
            && eventBody.getHeight() > 6)
        {
            const auto source = project.getMediaSourceState (clip.sourceId);
            const auto sourceStart = timeline::samplesToSeconds (
                clip.sourceOffsetSamples, source.sampleRate);
            const auto sourceEnd = sourceStart + timeline::samplesToSeconds (
                clip.lengthInSamples, timelineRate);
            graphics.setColour (clipColour.withAlpha (takeAlpha));
            if (auto* thumbnail = audioTrack->getThumbnailForSource (clip.sourceId))
                thumbnail->drawChannels (graphics,
                                         eventBody.reduced (5, 4),
                                         sourceStart,
                                         sourceEnd,
                                         0.90f * takeAlpha);
        }
        else if (audioTrack == nullptr)
        {
            graphics.setColour (StudioColours::textMuted);
            graphics.setFont (12.0f);
            graphics.drawFittedText ("Media offline",
                                     eventBody.reduced (10),
                                     juce::Justification::centredLeft,
                                     1);
        }

        graphics.setColour (selected ? StudioColours::primaryBright
                                     : clipColour.brighter (0.08f).withAlpha (0.92f));
        graphics.drawRoundedRectangle (clipArea.toFloat().reduced (0.5f),
                                       5.0f,
                                       selected ? 2.5f : 1.0f);

        auto eventTitleArea = eventHeader.reduced (7, 1);
        if (clip.locked && clipArea.getWidth() > 74)
        {
            auto badge = eventTitleArea.removeFromRight (34);
            graphics.setColour (StudioColours::orange);
            graphics.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
            graphics.drawText ("LOCK", badge,
                               juce::Justification::centredRight, false);
        }

        if (clip.takeGroupId.isNotEmpty() && clipArea.getWidth() > 58)
        {
            graphics.setColour (clip.activeTake ? StudioColours::text
                                                 : StudioColours::textMuted);
            graphics.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
            graphics.drawText ("TAKE " + juce::String (clip.takeLaneIndex + 1)
                                   + (clip.activeTake ? "  COMP" : ""),
                               eventTitleArea,
                               juce::Justification::centredLeft, false);
        }
        else if (clipArea.getWidth() > 42)
        {
            graphics.setColour (clip.muted ? StudioColours::textMuted
                                            : StudioColours::text);
            graphics.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
            graphics.drawFittedText (clipDisplayName (clip),
                                     eventTitleArea,
                                     juce::Justification::centredLeft, 1);
        }

        if (clip.muted && clipArea.getWidth() > 58)
        {
            graphics.setColour (StudioColours::textMuted);
            graphics.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
            graphics.drawText ("MUTED", eventBody.reduced (7, 3),
                               juce::Justification::centred, false);
        }

        if (std::abs (clip.timeStretchRatio - 1.0) > 0.001
            && clipArea.getWidth() > 82)
        {
            graphics.setColour (StudioColours::primary);
            graphics.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
            graphics.drawText ("RATE "
                                   + juce::String (1.0 / clip.timeStretchRatio, 2)
                                   + "x",
                               clipArea.reduced (7, 3),
                               juce::Justification::bottomRight, false);
        }
        if (clip.reversed && clipArea.getWidth() > 58)
        {
            graphics.setColour (StudioColours::orange);
            graphics.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
            graphics.drawText ("REV", clipArea.reduced (7, 3),
                               juce::Justification::bottomLeft, false);
        }

        const auto fadeInX = clipArea.getX() + juce::roundToInt (
            timeline::samplesToSeconds (clip.fadeInSamples, timelineRate)
                * pixelsPerSecond);
        const auto fadeOutX = clipArea.getRight() - juce::roundToInt (
            timeline::samplesToSeconds (clip.fadeOutSamples, timelineRate)
                * pixelsPerSecond);
        if (clip.fadeInSamples > 0)
        {
            juce::Path fadeCurve;
            fadeCurve.startNewSubPath (static_cast<float> (clipArea.getX()),
                                       static_cast<float> (clipArea.getBottom() - 3));
            fadeCurve.cubicTo (static_cast<float> (fadeInX - 6),
                               static_cast<float> (clipArea.getBottom() - 3),
                               static_cast<float> (fadeInX - 6),
                               static_cast<float> (clipArea.getY() + 4),
                               static_cast<float> (fadeInX),
                               static_cast<float> (clipArea.getY() + 4));
            graphics.setColour (StudioColours::text.withAlpha (0.90f));
            graphics.strokePath (fadeCurve, juce::PathStrokeType (1.8f));
        }
        if (clip.fadeOutSamples > 0)
        {
            juce::Path fadeCurve;
            fadeCurve.startNewSubPath (static_cast<float> (fadeOutX),
                                       static_cast<float> (clipArea.getY() + 4));
            fadeCurve.cubicTo (static_cast<float> (fadeOutX + 6),
                               static_cast<float> (clipArea.getY() + 4),
                               static_cast<float> (fadeOutX + 6),
                               static_cast<float> (clipArea.getBottom() - 3),
                               static_cast<float> (clipArea.getRight()),
                               static_cast<float> (clipArea.getBottom() - 3));
            graphics.setColour (StudioColours::text.withAlpha (0.90f));
            graphics.strokePath (fadeCurve, juce::PathStrokeType (1.8f));
        }

        for (const auto& marker : clip.transientMarkers)
        {
            const auto markerX = clipArea.getX() + juce::roundToInt (
                timeline::samplesToSeconds (marker.timelineOffsetSamples,
                                            timelineRate) * pixelsPerSecond);
            graphics.setColour (StudioColours::green.withAlpha (
                0.35f + 0.55f * juce::jlimit (0.0f, 1.0f, marker.strength)));
            graphics.drawVerticalLine (markerX,
                                       static_cast<float> (clipArea.getCentreY()),
                                       static_cast<float> (clipArea.getBottom() - 3));
        }

        for (const auto& marker : clip.warpMarkers)
        {
            const auto markerX = clipArea.getX() + juce::roundToInt (
                timeline::samplesToSeconds (marker.timelineOffsetSamples,
                                            timelineRate) * pixelsPerSecond);
            graphics.setColour (StudioColours::primary);
            graphics.drawVerticalLine (markerX,
                                       static_cast<float> (clipArea.getY() + 2),
                                       static_cast<float> (clipArea.getBottom() - 2));
            juce::Path triangle;
            triangle.addTriangle (static_cast<float> (markerX - 4),
                                  static_cast<float> (clipArea.getY() + 2),
                                  static_cast<float> (markerX + 4),
                                  static_cast<float> (clipArea.getY() + 2),
                                  static_cast<float> (markerX),
                                  static_cast<float> (clipArea.getY() + 8));
            graphics.fillPath (triangle);
        }

        if (selected && clipArea.getWidth() >= 18)
        {
            // Cubase-style event edges: the visible lower corner grips are the
            // same hit zones used by the trim gestures. Alt-dragging the right
            // edge continues to engage varispeed stretching.
            graphics.setColour (StudioColours::primaryBright);
            graphics.fillRect (clipArea.getX(), clipArea.getY() + 4,
                               3, clipArea.getHeight() - 8);
            graphics.fillRect (clipArea.getRight() - 3,
                               clipArea.getY() + 4,
                               3,
                               clipArea.getHeight() - 8);
            graphics.fillRoundedRectangle (
                juce::Rectangle<float> (static_cast<float> (clipArea.getX() + 1),
                                        static_cast<float> (clipArea.getBottom() - 9),
                                        8.0f, 8.0f), 2.0f);
            graphics.fillRoundedRectangle (
                juce::Rectangle<float> (static_cast<float> (clipArea.getRight() - 9),
                                        static_cast<float> (clipArea.getBottom() - 9),
                                        8.0f, 8.0f), 2.0f);
            graphics.setColour (StudioColours::text);
            graphics.fillEllipse (static_cast<float> (fadeInX - 4),
                                  static_cast<float> (clipArea.getY() + 1),
                                  8.0f, 8.0f);
            graphics.fillEllipse (static_cast<float> (fadeOutX - 4),
                                  static_cast<float> (clipArea.getY() + 1),
                                  8.0f, 8.0f);
        }
    }

    if (dragMode == DragMode::compRange && dragOriginClip.id.isNotEmpty())
    {
        const auto start = juce::jmin (compRangeAnchor, compRangeCurrent);
        const auto end = juce::jmax (compRangeAnchor, compRangeCurrent);
        const auto clipArea = getClipBounds (dragOriginClip);
        const auto x = getTimelineArea().getX()
            + timeline::samplesToSeconds (start, timelineRate) * pixelsPerSecond;
        const auto width = timeline::samplesToSeconds (end - start, timelineRate)
            * pixelsPerSecond;
        juce::Rectangle<float> rangeArea {
            static_cast<float> (x), static_cast<float> (clipArea.getY()),
            static_cast<float> (juce::jmax (2.0, width)),
            static_cast<float> (clipArea.getHeight())
        };
        graphics.setColour (StudioColours::primary.withAlpha (0.30f));
        graphics.fillRect (rangeArea);
        graphics.setColour (StudioColours::primary);
        graphics.drawRect (rangeArea, 2.0f);
    }

    if (track.isInstrument)
    {
        std::vector<tempo::Point> tempoPoints;
        for (const auto& point : project.getTempoPoints())
            tempoPoints.push_back ({ point.beat, point.bpm,
                point.curve == TempoCurve::linear ? tempo::SegmentCurve::linear
                                                   : tempo::SegmentCurve::step });
        for (const auto& clip : track.midiClips)
        {
            const auto clipStart = tempo::beatToSeconds (clip.startBeat, tempoPoints);
            const auto clipEnd = tempo::beatToSeconds (clip.startBeat + clip.lengthBeats,
                                                       tempoPoints);
            juce::Rectangle<int> clipArea {
                juce::roundToInt (timelineArea.getX() + clipStart * pixelsPerSecond),
                timelineArea.getY(),
                juce::jmax (4, juce::roundToInt ((clipEnd - clipStart) * pixelsPerSecond)),
                timelineArea.getHeight()
            };
            auto partHeader = clipArea;
            partHeader.setHeight (juce::jlimit (15, 22, clipArea.getHeight() / 4));
            const auto noteBody = clipArea.withTrimmedTop (partHeader.getHeight());

            graphics.setColour (track.colour.withAlpha (0.44f));
            graphics.fillRoundedRectangle (clipArea.toFloat(), 5.0f);
            graphics.setColour (track.colour.darker (0.34f).withAlpha (0.92f));
            graphics.fillRoundedRectangle (partHeader.toFloat(), 5.0f);
            graphics.fillRect (partHeader.withTrimmedTop (
                juce::jmax (0, partHeader.getHeight() - 5)));
            if (clipArea.getWidth() > 45)
            {
                graphics.setColour (StudioColours::text);
                graphics.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
                graphics.drawFittedText (
                    track.name + "  /  MIDI PART",
                    partHeader.reduced (7, 1),
                    juce::Justification::centredLeft, 1);
            }
            for (const auto& note : clip.notes)
            {
                const auto noteStart = tempo::beatToSeconds (
                    clip.startBeat + note.startBeat, tempoPoints);
                const auto noteEnd = tempo::beatToSeconds (
                    clip.startBeat + note.startBeat + note.lengthBeats, tempoPoints);
                const auto y = juce::jmap (note.noteNumber, 36, 84,
                                           noteBody.getBottom() - 7,
                                           noteBody.getY() + 4);
                juce::Rectangle<int> noteArea {
                    juce::roundToInt (timelineArea.getX() + noteStart * pixelsPerSecond),
                    y,
                    juce::jmax (2, juce::roundToInt ((noteEnd - noteStart)
                                                     * pixelsPerSecond)),
                    4
                };
                graphics.setColour (track.colour.withAlpha (
                    juce::jmap (note.velocity, 0.0f, 1.0f, 0.5f, 1.0f)));
                graphics.fillRoundedRectangle (noteArea.toFloat(), 2.0f);
            }
            graphics.setColour (track.colour.brighter (0.08f).withAlpha (0.92f));
            graphics.drawRoundedRectangle (clipArea.toFloat().reduced (0.5f),
                                           5.0f, 1.0f);
        }
    }

    const auto projectDuration = engine.getProjectLengthSeconds();
    if (projectDuration > 0.0)
    {
        const auto playheadX = timelineArea.getX()
                               + engine.getPositionSeconds() * pixelsPerSecond;
        if (playheadX >= timelineArea.getX() && playheadX <= timelineArea.getRight())
        {
            graphics.setColour (StudioColours::playhead);
            graphics.drawVerticalLine (juce::roundToInt (playheadX),
                                       static_cast<float> (timelineArea.getY() - 5),
                                       static_cast<float> (timelineArea.getBottom() + 5));
        }
    }
}

void TrackRowComponent::resized()
{
    auto controls = getLocalBounds().removeFromLeft (controlWidth).reduced (8, 6);
    auto top = controls.removeFromTop (27);
    top.removeFromLeft (32); // painted audio/instrument type badge
    nameLabel.setBounds (top.removeFromLeft (juce::jmax (42, top.getWidth() - 75)));
    muteButton.setBounds (top.removeFromLeft (25).reduced (2));
    soloButton.setBounds (top.removeFromLeft (25).reduced (2));
    recordButton.setBounds (top.removeFromLeft (25).reduced (2));
    infoLabel.setBounds (controls.removeFromTop (16));
    const auto track = project.getTrackState (trackId);
    instrumentSelector.setVisible (track.isInstrument && getHeight() >= 82);
    if (instrumentSelector.isVisible())
        instrumentSelector.setBounds (controls.removeFromTop (22).reduced (0, 2));

    gainLabel.setVisible (false);
    gainSlider.setVisible (false);
    panLabel.setVisible (false);
    panSlider.setVisible (false);
    latencyLabel.setVisible (false);
    latencySlider.setVisible (false);

    gainLabel.setBounds ({});
    gainSlider.setBounds ({});
    panLabel.setBounds ({});
    panSlider.setBounds ({});
    latencyLabel.setBounds ({});
    latencySlider.setBounds ({});
}

void TrackRowComponent::mouseDown (const juce::MouseEvent& event)
{
    const auto localEvent = event.getEventRelativeTo (this);
    dragOriginX = localEvent.position.x;
    dragOriginY = localEvent.position.y;

    if (! localEvent.mods.isPopupMenu()
        && localEvent.position.y >= static_cast<float> (getHeight() - 7))
    {
        dragMode = DragMode::trackHeight;
        resizeStartHeight = userHeight;
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        return;
    }

    const auto clipId = findClipAt (localEvent.position);

    if (localEvent.position.x < static_cast<float> (controlWidth))
    {
        selectedClipId.clear();
        dragMode = DragMode::none;
        if (onClipSelected)
            onClipSelected (trackId, {});
        if (localEvent.mods.isPopupMenu())
            showTrackContextMenu();
        repaint();
        return;
    }

    if (clipId.isNotEmpty())
    {
        selectedClipId = clipId;
        dragOriginClip = project.getClipState (trackId, clipId);
        dragPreviewClip = dragOriginClip;
        if (onClipSelected)
            onClipSelected (trackId, clipId);

        if (localEvent.mods.isPopupMenu())
        {
            dragMode = DragMode::none;
            showClipContextMenu (clipId);
            repaint();
            return;
        }

        if (dragOriginClip.locked)
        {
            dragMode = DragMode::none;
            repaint();
            return;
        }

        const auto markerAtMouse = findWarpMarkerAt (dragOriginClip,
                                                     localEvent.position.x);
        if (localEvent.mods.isCommandDown())
        {
            draggedWarpMarkerId = markerAtMouse;
            draggedWarpMarkerWasNew = draggedWarpMarkerId.isEmpty();
            if (draggedWarpMarkerWasNew)
            {
                draggedWarpMarkerId = "drag-preview";
                const auto offset = juce::jlimit (
                    static_cast<juce::int64> (1),
                    juce::jmax (static_cast<juce::int64> (1),
                                dragOriginClip.lengthInSamples - 1),
                    snapped (sampleAtX (localEvent.position.x))
                        - dragOriginClip.timelineStartSamples);
                dragPreviewClip.warpMarkers.push_back (
                    { draggedWarpMarkerId, offset, 0 });
            }
            else
                project.beginUndoTransaction ("Move Warp Marker");
            dragMode = DragMode::warpMarker;
        }
        else if (localEvent.mods.isShiftDown() && dragOriginClip.takeGroupId.isNotEmpty())
        {
            dragMode = DragMode::compRange;
            compRangeAnchor = juce::jlimit (
                dragOriginClip.timelineStartSamples,
                dragOriginClip.timelineStartSamples + dragOriginClip.lengthInSamples,
                snapped (sampleAtX (localEvent.position.x)));
            compRangeCurrent = compRangeAnchor;
        }
        else
        {
            dragMode = editModeAt (dragOriginClip, localEvent.position);
            const auto altDown = localEvent.mods.isAltDown()
                || juce::ModifierKeys::getCurrentModifiersRealtime().isAltDown();
            if (altDown && dragMode == DragMode::trimEnd)
                dragMode = DragMode::stretchEnd;
        }

        if (dragMode == DragMode::move)
            project.beginUndoTransaction ("Move Clip");
        else if (dragMode == DragMode::trimStart)
            project.beginUndoTransaction ("Trim Clip Start");
        else if (dragMode == DragMode::trimEnd)
            project.beginUndoTransaction ("Trim Clip End");
        else if (dragMode == DragMode::stretchEnd)
            project.beginUndoTransaction ("Stretch Clip");

        repaint();
        return;
    }

    if (localEvent.mods.isPopupMenu())
    {
        dragMode = DragMode::none;
        if (onClipSelected)
            onClipSelected (trackId, {});
        showTrackContextMenu();
        repaint();
        return;
    }

    selectedClipId.clear();
    dragMode = DragMode::seek;
    if (onClipSelected)
        onClipSelected (trackId, juce::String());
    seekFromMouseX (localEvent.position.x);
}

void TrackRowComponent::mouseDoubleClick (const juce::MouseEvent& event)
{
    const auto clipId = findClipAt (event.getEventRelativeTo (this).position);
    if (clipId.isNotEmpty())
        project.selectActiveTake (trackId, clipId);
}

juce::String TrackRowComponent::clipDisplayName (
    const AudioClipState& clip) const
{
    if (clip.name.trim().isNotEmpty())
        return clip.name.trim();
    const auto source = project.getMediaSourceState (clip.sourceId);
    if (source.file.getFileNameWithoutExtension().isNotEmpty())
        return source.file.getFileNameWithoutExtension();
    return "Audio Clip";
}

void TrackRowComponent::showTrackContextMenu()
{
    const auto track = project.getTrackState (trackId);
    if (track.id.isEmpty())
        return;

    enum MenuId
    {
        trackInformation = 1000,
        renameTrack,
        muteTrack,
        soloTrack,
        armTrack,
        builtInPiano,
        builtInSynth,
        builtInDrums,
        builtInSampler,
        deleteTrack
    };

    juce::PopupMenu menu;
    menu.addSectionHeader (track.name);
    menu.addItem (trackInformation, "Track Information...");
    menu.addItem (renameTrack, "Rename Track...");
    menu.addSeparator();
    menu.addItem (muteTrack, track.muted ? "Unmute Track" : "Mute Track",
                  true, track.muted);
    menu.addItem (soloTrack, track.solo ? "Unsolo Track" : "Solo Track",
                  true, track.solo);
    menu.addItem (armTrack,
                  track.recordArmed ? "Disarm Track" : "Arm Track for Recording",
                  true, track.recordArmed);
    if (track.isInstrument)
    {
        juce::PopupMenu instrumentMenu;
        const auto currentInstrument = track.pluginName.trim().isNotEmpty()
            ? track.pluginName.trim() : juce::String ("Triumph Piano");
        instrumentMenu.addItem (builtInPiano, "Triumph Piano", true,
                                currentInstrument == "Triumph Piano");
        instrumentMenu.addItem (builtInSynth, "Triumph Synth", true,
                                currentInstrument == "Triumph Synth");
        instrumentMenu.addItem (builtInDrums, "Triumph Drums", true,
                                currentInstrument == "Triumph Drums");
        instrumentMenu.addItem (builtInSampler, "Triumph Sampler", true,
                                currentInstrument == "Triumph Sampler");
        menu.addSubMenu ("Change Instrument", instrumentMenu);
    }
    menu.addSeparator();
    menu.addItem (deleteTrack, "Delete Track...");

    const auto safe = juce::Component::SafePointer<TrackRowComponent> (this);
    menu.showMenuAsync (
        juce::PopupMenu::Options().withTargetComponent (this),
        [safe] (int result)
        {
            if (safe == nullptr || result == 0)
                return;
            const auto current = safe->project.getTrackState (safe->trackId);
            if (current.id.isEmpty())
                return;

            switch (result)
            {
                case trackInformation:
                    safe->showTrackInformation();
                    break;
                case renameTrack:
                    safe->showTrackRenameDialog();
                    break;
                case muteTrack:
                    safe->project.setTrackMuted (safe->trackId, ! current.muted);
                    break;
                case soloTrack:
                    safe->project.setTrackSolo (safe->trackId, ! current.solo);
                    break;
                case armTrack:
                    safe->project.setTrackRecordArmed (safe->trackId,
                                                       ! current.recordArmed);
                    break;
                case builtInPiano:
                    safe->project.setBuiltInInstrument (
                        safe->trackId, "Triumph Piano");
                    break;
                case builtInSynth:
                    safe->project.setBuiltInInstrument (
                        safe->trackId, "Triumph Synth");
                    break;
                case builtInDrums:
                    safe->project.setBuiltInInstrument (
                        safe->trackId, "Triumph Drums");
                    break;
                case builtInSampler:
                    safe->project.setBuiltInInstrument (
                        safe->trackId, "Triumph Sampler");
                    break;
                case deleteTrack:
                    if (safe->onTrackDeleteRequested)
                        safe->onTrackDeleteRequested (safe->trackId);
                    break;
                default:
                    break;
            }
            if (safe != nullptr)
                safe->refreshFromModel();
        });
}

void TrackRowComponent::populateInstrumentSelector (const TrackState& track)
{
    instrumentSelector.setVisible (track.isInstrument);
    if (! track.isInstrument)
        return;

    const auto current = track.pluginName.trim().isNotEmpty()
        ? track.pluginName.trim() : juce::String ("Triumph Piano");
    if (current == "Triumph Synth")
        instrumentSelector.setSelectedId (2, juce::dontSendNotification);
    else if (current == "Triumph Drums")
        instrumentSelector.setSelectedId (3, juce::dontSendNotification);
    else if (current == "Triumph Sampler")
        instrumentSelector.setSelectedId (4, juce::dontSendNotification);
    else if (current == "Triumph Piano")
        instrumentSelector.setSelectedId (1, juce::dontSendNotification);
    else
    {
        instrumentSelector.setText (current, juce::dontSendNotification);
        instrumentSelector.setTooltip (
            "External instrument loaded. Use Plug-ins to replace it, or choose a built-in instrument here.");
    }
}

void TrackRowComponent::showTrackRenameDialog()
{
    const auto track = project.getTrackState (trackId);
    if (track.id.isEmpty())
        return;

    auto* dialog = new juce::AlertWindow (
        "Rename Track", "Give this track a clear arrangement name.",
        juce::AlertWindow::NoIcon);
    dialog->addTextEditor ("name", track.name, "Track name");
    dialog->addButton ("Rename", 1,
                       juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("Cancel", 0,
                       juce::KeyPress (juce::KeyPress::escapeKey));
    const auto safe = juce::Component::SafePointer<TrackRowComponent> (this);
    dialog->enterModalState (
        true,
        juce::ModalCallbackFunction::create (
            [safe, dialog] (int result)
            {
                if (safe == nullptr || result == 0)
                    return;
                safe->project.setTrackName (
                    safe->trackId,
                    dialog->getTextEditorContents ("name"));
                safe->refreshFromModel();
            }),
        true);
}

void TrackRowComponent::showTrackInformation()
{
    const auto track = project.getTrackState (trackId);
    if (track.id.isEmpty())
        return;

    auto details = "Type: " + juce::String (track.isInstrument
                                                 ? "Instrument / MIDI"
                                                 : "Audio")
                   + "\nInput: "
                   + (track.isInstrument
                          ? juce::String ("MIDI")
                          : juce::String (track.inputChannelCount == 1 ? "Mono" : "Stereo")
                                + " (channel "
                                + juce::String (track.inputStartChannel + 1) + ")")
                   + "\nOutput: " + track.outputDestinationId.toUpperCase()
                   + "\nVolume: " + juce::String (track.gain, 2) + "x"
                   + "\nPan: " + juce::String (track.pan, 2)
                   + "\nManual latency offset: "
                   + juce::String (track.manualLatencyOffsetSamples) + " samples";

    if (track.isInstrument)
    {
        auto notes = 0;
        for (const auto& clip : track.midiClips)
            notes += static_cast<int> (clip.notes.size());
        details += "\nInstrument: "
                   + (track.pluginName.isNotEmpty() ? track.pluginName
                                                    : juce::String ("Not loaded"))
                   + "\nMIDI parts: "
                   + juce::String (static_cast<int> (track.midiClips.size()))
                   + "\nMIDI notes: " + juce::String (notes);
    }
    else
    {
        details += "\nAudio events: "
                   + juce::String (static_cast<int> (track.clips.size()));
        if (const auto* audioTrack = engine.findTrack (trackId))
            details += "\nTimeline duration: "
                       + formatDuration (audioTrack->getLengthSeconds());
    }

    details += "\nStatus: "
               + juce::String (track.recordArmed ? "Record armed"
                                  : track.muted ? "Muted"
                                  : track.solo ? "Solo"
                                               : "Active");

    juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
                                            track.name,
                                            details,
                                            "OK",
                                            this);
}

void TrackRowComponent::showClipContextMenu (const juce::String& clipId)
{
    const auto clip = project.getClipState (trackId, clipId);
    const auto track = project.getTrackState (trackId);
    if (clip.id.isEmpty())
        return;

    enum MenuId
    {
        rename = 1,
        clipInformation,
        duplicate,
        mute,
        lock,
        reverse,
        splitAtPlayhead,
        resetFades,
        crossfadeNext,
        deleteClip,
        useTrackColour = 100,
        blueColour,
        cyanColour,
        greenColour,
        purpleColour,
        orangeColour,
        redColour
    };

    const auto playhead = static_cast<juce::int64> (
        timeline::secondsToSamples (engine.getPositionSeconds(),
                                    project.getTimelineSampleRate()));
    const auto canSplit = ! clip.locked
        && playhead > clip.timelineStartSamples
        && playhead < clip.timelineStartSamples + clip.lengthInSamples;
    const auto clipEnd = clip.timelineStartSamples + clip.lengthInSamples;
    const auto canCrossfade = ! clip.locked && std::any_of (
        track.clips.begin(), track.clips.end(),
        [&clip, clipEnd] (const auto& candidate)
        {
            return candidate.id != clip.id && ! candidate.locked
                && candidate.timelineStartSamples > clip.timelineStartSamples
                && candidate.timelineStartSamples < clipEnd;
        });

    juce::PopupMenu colours;
    colours.addItem (useTrackColour, "Use Track Colour", true,
                     clip.colour.isTransparent());
    colours.addItem (blueColour, "Blue");
    colours.addItem (cyanColour, "Cyan");
    colours.addItem (greenColour, "Green");
    colours.addItem (purpleColour, "Purple");
    colours.addItem (orangeColour, "Orange");
    colours.addItem (redColour, "Red");

    juce::PopupMenu menu;
    menu.addSectionHeader (clipDisplayName (clip));
    menu.addItem (clipInformation, "Clip Information...");
    menu.addItem (rename, "Rename Clip...");
    menu.addItem (duplicate, "Duplicate Clip");
    menu.addSeparator();
    menu.addItem (mute, clip.muted ? "Unmute Clip" : "Mute Clip",
                  true, clip.muted);
    menu.addItem (lock, clip.locked ? "Unlock Clip" : "Lock Clip",
                  true, clip.locked);
    menu.addItem (reverse, "Reverse Audio", ! clip.locked, clip.reversed);
    menu.addItem (splitAtPlayhead, "Split at Playhead", canSplit);
    menu.addItem (crossfadeNext, "Crossfade with Next Overlap", canCrossfade);
    menu.addItem (resetFades, "Reset Fades",
                  ! clip.locked
                      && (clip.fadeInSamples > 0 || clip.fadeOutSamples > 0));
    menu.addSubMenu ("Clip Colour", colours);
    menu.addSeparator();
    menu.addItem (deleteClip, "Delete Clip", ! clip.locked);

    const auto safe = juce::Component::SafePointer<TrackRowComponent> (this);
    menu.showMenuAsync (
        juce::PopupMenu::Options().withTargetComponent (this),
        [safe, clipId] (int result)
        {
            if (safe == nullptr || result == 0)
                return;
            const auto current = safe->project.getClipState (
                safe->trackId, clipId);
            if (current.id.isEmpty())
                return;

            switch (result)
            {
                case clipInformation:
                    safe->showClipInformation (clipId);
                    break;
                case rename:
                    safe->showClipRenameDialog (clipId);
                    break;
                case duplicate:
                {
                    const auto copyId = safe->project.duplicateClip (
                        safe->trackId, clipId);
                    if (copyId.isNotEmpty())
                    {
                        safe->selectedClipId = copyId;
                        if (safe->onClipSelected)
                            safe->onClipSelected (safe->trackId, copyId);
                    }
                    break;
                }
                case mute:
                    safe->project.setClipMuted (safe->trackId, clipId,
                                                ! current.muted);
                    break;
                case lock:
                    safe->project.setClipLocked (safe->trackId, clipId,
                                                 ! current.locked);
                    break;
                case reverse:
                    safe->project.setClipReversed (safe->trackId, clipId,
                                                   ! current.reversed);
                    break;
                case splitAtPlayhead:
                {
                    const auto sample = static_cast<juce::int64> (
                        timeline::secondsToSamples (
                            safe->engine.getPositionSeconds(),
                            safe->project.getTimelineSampleRate()));
                    const auto secondId = safe->project.splitClip (
                        safe->trackId, clipId, sample);
                    if (secondId.isNotEmpty())
                    {
                        safe->selectedClipId = secondId;
                        if (safe->onClipSelected)
                            safe->onClipSelected (safe->trackId, secondId);
                    }
                    break;
                }
                case resetFades:
                    safe->project.setClipFades (safe->trackId, clipId, 0, 0,
                                                "Reset Clip Fades");
                    break;
                case crossfadeNext:
                    safe->project.createClipCrossfade (safe->trackId, clipId);
                    break;
                case deleteClip:
                    if (safe->project.removeClip (safe->trackId, clipId))
                    {
                        safe->selectedClipId.clear();
                        if (safe->onClipSelected)
                            safe->onClipSelected ({}, {});
                    }
                    break;
                case useTrackColour:
                    safe->project.setClipColour (
                        safe->trackId, clipId, juce::Colours::transparentBlack);
                    break;
                case blueColour:
                    safe->project.setClipColour (
                        safe->trackId, clipId, juce::Colour (0xff3b82f6));
                    break;
                case cyanColour:
                    safe->project.setClipColour (
                        safe->trackId, clipId, juce::Colour (0xff22d3ee));
                    break;
                case greenColour:
                    safe->project.setClipColour (
                        safe->trackId, clipId, juce::Colour (0xff22c55e));
                    break;
                case purpleColour:
                    safe->project.setClipColour (
                        safe->trackId, clipId, juce::Colour (0xff8b5cf6));
                    break;
                case orangeColour:
                    safe->project.setClipColour (
                        safe->trackId, clipId, juce::Colour (0xfff59e0b));
                    break;
                case redColour:
                    safe->project.setClipColour (
                        safe->trackId, clipId, juce::Colour (0xffef476f));
                    break;
                default:
                    break;
            }
            if (safe != nullptr)
                safe->refreshFromModel();
        });
}

void TrackRowComponent::showClipInformation (const juce::String& clipId)
{
    const auto clip = project.getClipState (trackId, clipId);
    if (clip.id.isEmpty())
        return;

    const auto timelineRate = project.getTimelineSampleRate();
    const auto source = project.getMediaSourceState (clip.sourceId);
    const auto startSeconds = timeline::samplesToSeconds (
        clip.timelineStartSamples, timelineRate);
    const auto lengthSeconds = timeline::samplesToSeconds (
        clip.lengthInSamples, timelineRate);
    const auto sourceOffsetSeconds = timeline::samplesToSeconds (
        clip.sourceOffsetSamples,
        source.sampleRate > 0.0 ? source.sampleRate : timelineRate);

    auto details = "Track: " + project.getTrackState (trackId).name
                   + "\nTimeline start: " + juce::String (startSeconds, 3) + " s"
                   + "\nEvent length: " + juce::String (lengthSeconds, 3) + " s"
                   + "\nFade in: " + juce::String (timeline::samplesToSeconds (
                         clip.fadeInSamples, timelineRate), 3) + " s"
                   + "\nFade out: " + juce::String (timeline::samplesToSeconds (
                         clip.fadeOutSamples, timelineRate), 3) + " s"
                   + "\nSource offset: " + juce::String (sourceOffsetSeconds, 3) + " s"
                   + "\nPlayback rate: "
                   + juce::String (1.0 / clip.timeStretchRatio, 3) + "x"
                   + "\nGain: " + juce::String (clip.gain, 2) + "x"
                   + "\nState: "
                   + juce::String (clip.locked ? "Locked"
                                      : clip.muted ? "Muted"
                                                   : "Active");
    if (source.file != juce::File())
        details += "\nSource: " + source.file.getFullPathName();

    juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
                                            clipDisplayName (clip),
                                            details,
                                            "OK",
                                            this);
}

void TrackRowComponent::showClipRenameDialog (const juce::String& clipId)
{
    const auto clip = project.getClipState (trackId, clipId);
    if (clip.id.isEmpty())
        return;

    auto* dialog = new juce::AlertWindow (
        "Rename Clip", "Give this clip a clear arrangement name.",
        juce::AlertWindow::NoIcon);
    dialog->addTextEditor ("name", clipDisplayName (clip), "Clip name");
    dialog->addButton ("Rename", 1,
                       juce::KeyPress (juce::KeyPress::returnKey));
    dialog->addButton ("Cancel", 0,
                       juce::KeyPress (juce::KeyPress::escapeKey));
    const auto safe = juce::Component::SafePointer<TrackRowComponent> (this);
    dialog->enterModalState (
        true,
        juce::ModalCallbackFunction::create (
            [safe, dialog, clipId] (int result)
            {
                if (safe == nullptr || result == 0)
                    return;
                safe->project.setClipName (
                    safe->trackId, clipId,
                    dialog->getTextEditorContents ("name"));
                safe->refreshFromModel();
            }),
        true);
}

void TrackRowComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (dragMode == DragMode::trackHeight)
    {
        const auto deltaY = juce::roundToInt (event.position.y - dragOriginY);
        const auto newHeight = juce::jlimit (
            preferredHeight, 220, resizeStartHeight + deltaY);
        if (newHeight != userHeight)
        {
            userHeight = newHeight;
            if (onTrackHeightChanged)
                onTrackHeightChanged (trackId, userHeight);
            resized();
            repaint();
        }
        return;
    }

    if (dragMode == DragMode::seek)
    {
        seekFromMouseX (event.position.x);
        return;
    }

    if (dragOriginClip.id.isEmpty())
        return;

    // On Windows the Alt state can arrive after the initial mouse-down event.
    // Latch varispeed stretching as soon as Alt is observed during a right-edge
    // drag so the gesture remains dependable.
    if (dragMode == DragMode::trimEnd
        && (event.mods.isAltDown()
            || juce::ModifierKeys::getCurrentModifiersRealtime().isAltDown()))
    {
        dragMode = DragMode::stretchEnd;
        project.beginUndoTransaction ("Stretch Clip");
    }

    if (dragMode == DragMode::warpMarker)
    {
        auto minimum = static_cast<juce::int64> (1);
        auto maximum = juce::jmax (minimum, dragOriginClip.lengthInSamples - 1);
        if (! draggedWarpMarkerWasNew)
        {
            auto current = static_cast<juce::int64> (0);
            for (const auto& marker : dragOriginClip.warpMarkers)
                if (marker.id == draggedWarpMarkerId)
                    current = marker.timelineOffsetSamples;
            for (const auto& marker : dragOriginClip.warpMarkers)
            {
                if (marker.id == draggedWarpMarkerId)
                    continue;
                if (marker.timelineOffsetSamples < current)
                    minimum = juce::jmax (minimum,
                                          marker.timelineOffsetSamples + 1);
                else if (marker.timelineOffsetSamples > current)
                    maximum = juce::jmin (maximum,
                                          marker.timelineOffsetSamples - 1);
            }
        }
        const auto offset = juce::jlimit (
            minimum, maximum,
            snapped (sampleAtX (event.position.x))
                - dragOriginClip.timelineStartSamples);
        for (auto& marker : dragPreviewClip.warpMarkers)
            if (marker.id == draggedWarpMarkerId)
                marker.timelineOffsetSamples = offset;
        repaint();
        return;
    }

    if (dragMode == DragMode::compRange)
    {
        const auto clipEnd = dragOriginClip.timelineStartSamples
                             + dragOriginClip.lengthInSamples;
        compRangeCurrent = juce::jlimit (dragOriginClip.timelineStartSamples,
                                         clipEnd,
                                         snapped (sampleAtX (event.position.x)));
        repaint();
        return;
    }

    const auto timelineRate = project.getTimelineSampleRate();
    const auto delta = static_cast<juce::int64> (std::llround (
        static_cast<double> (event.position.x - dragOriginX)
        / pixelsPerSecond * timelineRate));

    dragPreviewClip = dragOriginClip;
    if (dragMode == DragMode::move)
        dragPreviewClip.timelineStartSamples = snapped (
            dragOriginClip.timelineStartSamples + delta);
    else if (dragMode == DragMode::trimStart)
    {
        const auto oldEnd = dragOriginClip.timelineStartSamples
                            + dragOriginClip.lengthInSamples;
        const auto newStart = juce::jlimit (
            static_cast<juce::int64> (0), oldEnd - 1,
            snapped (dragOriginClip.timelineStartSamples + delta));
        const auto trimOffset = newStart - dragOriginClip.timelineStartSamples;
        dragPreviewClip.timelineStartSamples = newStart;
        dragPreviewClip.lengthInSamples = oldEnd - newStart;
        dragPreviewClip.fadeInSamples = juce::jmin (
            dragOriginClip.fadeInSamples, dragPreviewClip.lengthInSamples);
        dragPreviewClip.fadeOutSamples = juce::jmin (
            dragOriginClip.fadeOutSamples, dragPreviewClip.lengthInSamples);
        for (auto& marker : dragPreviewClip.warpMarkers)
            marker.timelineOffsetSamples -= trimOffset;
        for (auto& marker : dragPreviewClip.transientMarkers)
            marker.timelineOffsetSamples -= trimOffset;
        std::erase_if (dragPreviewClip.warpMarkers,
                       [] (const auto& marker)
                       {
                           return marker.timelineOffsetSamples <= 0;
                       });
        std::erase_if (dragPreviewClip.transientMarkers,
                       [] (const auto& marker)
                       {
                           return marker.timelineOffsetSamples <= 0;
                       });
    }
    else if (dragMode == DragMode::trimEnd)
    {
        auto newEnd = juce::jmax (
            dragOriginClip.timelineStartSamples + 1,
            snapped (dragOriginClip.timelineStartSamples
                     + dragOriginClip.lengthInSamples + delta));
        if (! dragOriginClip.warpMarkers.empty())
            newEnd = juce::jmin (
                newEnd, dragOriginClip.timelineStartSamples
                            + dragOriginClip.lengthInSamples);
        dragPreviewClip.lengthInSamples = newEnd
                                          - dragOriginClip.timelineStartSamples;
        dragPreviewClip.fadeInSamples = juce::jmin (
            dragOriginClip.fadeInSamples, dragPreviewClip.lengthInSamples);
        dragPreviewClip.fadeOutSamples = juce::jmin (
            dragOriginClip.fadeOutSamples, dragPreviewClip.lengthInSamples);
        const auto previewLength = dragPreviewClip.lengthInSamples;
        std::erase_if (dragPreviewClip.warpMarkers,
                       [previewLength] (const auto& marker)
                       {
                           return marker.timelineOffsetSamples >= previewLength;
                       });
        std::erase_if (dragPreviewClip.transientMarkers,
                       [previewLength] (const auto& marker)
                       {
                           return marker.timelineOffsetSamples >= previewLength;
                       });
    }
    else if (dragMode == DragMode::stretchEnd)
    {
        const auto nativeLength = juce::jmax (
            static_cast<juce::int64> (1),
            static_cast<juce::int64> (stretch::timelineToNativeSamples (
                dragOriginClip.lengthInSamples,
                dragOriginClip.timeStretchRatio)));
        const auto requestedLength = juce::jmax (
            static_cast<juce::int64> (1),
            snapped (dragOriginClip.timelineStartSamples
                     + dragOriginClip.lengthInSamples + delta)
                - dragOriginClip.timelineStartSamples);
        const auto ratio = stretch::clampRatio (
            static_cast<double> (requestedLength)
            / static_cast<double> (nativeLength));
        const auto newLength = static_cast<juce::int64> (
            stretch::nativeToTimelineSamples (nativeLength, ratio));
        const auto scale = static_cast<double> (newLength)
                           / static_cast<double> (
                               dragOriginClip.lengthInSamples);
        dragPreviewClip.lengthInSamples = newLength;
        dragPreviewClip.timeStretchRatio = ratio;
        for (auto& marker : dragPreviewClip.warpMarkers)
            marker.timelineOffsetSamples = static_cast<juce::int64> (
                std::llround (marker.timelineOffsetSamples * scale));
        for (auto& marker : dragPreviewClip.transientMarkers)
            marker.timelineOffsetSamples = static_cast<juce::int64> (
                std::llround (marker.timelineOffsetSamples * scale));
        dragPreviewClip.fadeInSamples = static_cast<juce::int64> (
            std::llround (dragOriginClip.fadeInSamples * scale));
        dragPreviewClip.fadeOutSamples = static_cast<juce::int64> (
            std::llround (dragOriginClip.fadeOutSamples * scale));
    }
    else if (dragMode == DragMode::fadeIn)
        dragPreviewClip.fadeInSamples = juce::jlimit (
            static_cast<juce::int64> (0), dragOriginClip.lengthInSamples,
            snapped (sampleAtX (event.position.x))
                - dragOriginClip.timelineStartSamples);
    else if (dragMode == DragMode::fadeOut)
        dragPreviewClip.fadeOutSamples = juce::jlimit (
            static_cast<juce::int64> (0), dragOriginClip.lengthInSamples,
            dragOriginClip.timelineStartSamples + dragOriginClip.lengthInSamples
                - snapped (sampleAtX (event.position.x)));
    repaint();
}

void TrackRowComponent::mouseUp (const juce::MouseEvent&)
{
    const auto completedMode = dragMode;
    const auto origin = dragOriginClip;
    const auto preview = dragPreviewClip;
    const auto warpMarkerId = draggedWarpMarkerId;
    const auto warpMarkerWasNew = draggedWarpMarkerWasNew;
    const auto rangeStart = juce::jmin (compRangeAnchor, compRangeCurrent);
    const auto rangeEnd = juce::jmax (compRangeAnchor, compRangeCurrent);

    dragMode = DragMode::none;
    dragOriginClip = {};
    dragPreviewClip = {};
    draggedWarpMarkerId.clear();
    draggedWarpMarkerWasNew = false;

    if (completedMode == DragMode::trackHeight)
    {
        repaint();
        return;
    }

    if (origin.id.isEmpty())
        return;

    if (completedMode == DragMode::move)
        project.moveClip (trackId, origin.id, preview.timelineStartSamples);
    else if (completedMode == DragMode::trimStart)
        project.trimClipStart (trackId, origin.id,
                               preview.timelineStartSamples);
    else if (completedMode == DragMode::trimEnd)
        project.trimClipEnd (trackId, origin.id,
                             origin.timelineStartSamples
                                 + preview.lengthInSamples);
    else if (completedMode == DragMode::stretchEnd)
        project.stretchClipToEnd (trackId, origin.id,
                                  origin.timelineStartSamples
                                      + preview.lengthInSamples);
    else if (completedMode == DragMode::fadeIn
             || completedMode == DragMode::fadeOut)
        project.setClipFades (trackId, origin.id,
                              preview.fadeInSamples,
                              preview.fadeOutSamples);
    else if (completedMode == DragMode::warpMarker)
    {
        for (const auto& marker : preview.warpMarkers)
            if (marker.id == warpMarkerId)
            {
                const auto timelineSample = origin.timelineStartSamples
                                            + marker.timelineOffsetSamples;
                if (warpMarkerWasNew)
                    project.addWarpMarker (trackId, origin.id, timelineSample);
                else
                    project.moveWarpMarker (trackId, origin.id, warpMarkerId,
                                            timelineSample);
                break;
            }
    }
    else if (completedMode == DragMode::compRange)
        project.assignCompRegion (trackId, origin.id, rangeStart, rangeEnd);

    repaint();
}

void TrackRowComponent::mouseMove (const juce::MouseEvent& event)
{
    const auto localEvent = event.getEventRelativeTo (this);
    if (! localEvent.mods.isPopupMenu()
        && localEvent.position.y >= static_cast<float> (getHeight() - 7))
    {
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        return;
    }

    if (localEvent.position.x < static_cast<float> (controlWidth))
    {
        setMouseCursor (localEvent.mods.isPopupMenu()
                            ? juce::MouseCursor::PointingHandCursor
                            : juce::MouseCursor::NormalCursor);
        return;
    }

    const auto clipId = findClipAt (localEvent.position);
    if (clipId.isEmpty())
    {
        setMouseCursor (juce::MouseCursor::NormalCursor);
        return;
    }

    const auto clip = project.getClipState (trackId, clipId);
    if (clip.locked)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        return;
    }
    const auto mode = editModeAt (clip, localEvent.position);
    setMouseCursor (localEvent.mods.isCommandDown()
                        || findWarpMarkerAt (clip, localEvent.position.x).isNotEmpty()
                        ? juce::MouseCursor::CrosshairCursor
                        : localEvent.mods.isShiftDown() && clip.takeGroupId.isNotEmpty()
                        ? juce::MouseCursor::CrosshairCursor
                        : mode == DragMode::trimStart || mode == DragMode::trimEnd
                              || mode == DragMode::stretchEnd
                              || mode == DragMode::fadeIn
                              || mode == DragMode::fadeOut
                        ? juce::MouseCursor::LeftRightResizeCursor
                        : juce::MouseCursor::DraggingHandCursor);
}

void TrackRowComponent::mouseExit (const juce::MouseEvent&)
{
    if (dragMode == DragMode::none)
        setMouseCursor (juce::MouseCursor::NormalCursor);
}

void TrackRowComponent::timerCallback()
{
    const auto playhead = engine.getPositionSeconds();
    if (engine.isPlaying()
        || std::abs (playhead - lastPaintedPlayheadSeconds) > 0.000001)
    {
        lastPaintedPlayheadSeconds = playhead;
        repaint (controlWidth, 0, getWidth() - controlWidth, getHeight());
    }
}

void TrackRowComponent::seekFromMouseX (float x)
{
    if (x < getTimelineArea().getX())
        return;

    engine.setPositionSeconds (juce::jmax (
        0.0,
        static_cast<double> (x - getTimelineArea().getX()) / pixelsPerSecond));
}

juce::Rectangle<int> TrackRowComponent::getTimelineArea() const
{
    return getLocalBounds().withTrimmedLeft (controlWidth + timelineInset)
        .reduced (0, 8);
}

juce::Rectangle<int> TrackRowComponent::getCompLaneArea() const
{
    auto area = getTimelineArea();
    area.setHeight (29);
    return area;
}

juce::Rectangle<int> TrackRowComponent::getClipBounds (const AudioClipState& clip) const
{
    const auto timelineRate = project.getTimelineSampleRate();
    const auto x = getTimelineArea().getX()
                   + timeline::samplesToSeconds (clip.timelineStartSamples, timelineRate)
                         * pixelsPerSecond;
    const auto width = timeline::samplesToSeconds (clip.lengthInSamples, timelineRate)
                       * pixelsPerSecond;
    const auto isTake = clip.takeGroupId.isNotEmpty();
    const auto y = getTimelineArea().getY()
                   + (isTake ? 32 + clip.takeLaneIndex * 32 : 0);
    const auto height = isTake ? 29 : getTimelineArea().getHeight();
    return { juce::roundToInt (x),
             y,
             juce::jmax (4, juce::roundToInt (width)),
             height };
}

juce::int64 TrackRowComponent::sampleAtX (float x) const
{
    const auto seconds = juce::jmax (
        0.0, static_cast<double> (x - getTimelineArea().getX()) / pixelsPerSecond);
    return static_cast<juce::int64> (timeline::secondsToSamples (
        seconds, project.getTimelineSampleRate()));
}

bool TrackRowComponent::hasTakeLanes() const
{
    for (const auto& clip : project.getTrackState (trackId).clips)
        if (clip.takeGroupId.isNotEmpty())
            return true;
    return false;
}

juce::String TrackRowComponent::findClipAt (juce::Point<float> position) const
{
    const auto track = project.getTrackState (trackId);
    constexpr int edgeHitWidth = 14;

    for (auto iterator = track.clips.rbegin(); iterator != track.clips.rend(); ++iterator)
        if (getClipBounds (*iterator).expanded (edgeHitWidth, 0)
                .contains (position.toInt()))
            return iterator->id;

    return {};
}

TrackRowComponent::DragMode TrackRowComponent::editModeAt (
    const AudioClipState& clip, juce::Point<float> position) const
{
    const auto bounds = getClipBounds (clip);
    constexpr float trimHandleWidth = 14.0f;
    constexpr float fadeHandleRadius = 10.0f;
    const auto timelineRate = project.getTimelineSampleRate();
    const auto fadeInX = static_cast<float> (bounds.getX())
        + static_cast<float> (timeline::samplesToSeconds (
            clip.fadeInSamples, timelineRate) * pixelsPerSecond);
    const auto fadeOutX = static_cast<float> (bounds.getRight())
        - static_cast<float> (timeline::samplesToSeconds (
            clip.fadeOutSamples, timelineRate) * pixelsPerSecond);

    if (position.y <= static_cast<float> (bounds.getY() + 18))
    {
        if (std::abs (position.x - fadeInX) <= fadeHandleRadius)
            return DragMode::fadeIn;
        if (std::abs (position.x - fadeOutX) <= fadeHandleRadius)
            return DragMode::fadeOut;
    }

    if (std::abs (position.x - static_cast<float> (bounds.getX()))
            <= trimHandleWidth)
        return DragMode::trimStart;

    if (std::abs (position.x - static_cast<float> (bounds.getRight()))
            <= trimHandleWidth)
        return DragMode::trimEnd;

    return DragMode::move;
}

juce::String TrackRowComponent::findWarpMarkerAt (const AudioClipState& clip,
                                                   float x) const
{
    const auto bounds = getClipBounds (clip);
    const auto timelineRate = project.getTimelineSampleRate();
    for (const auto& marker : clip.warpMarkers)
    {
        const auto markerX = bounds.getX() + timeline::samplesToSeconds (
            marker.timelineOffsetSamples, timelineRate) * pixelsPerSecond;
        if (std::abs (x - static_cast<float> (markerX)) <= 7.0f)
            return marker.id;
    }
    return {};
}

juce::int64 TrackRowComponent::snapped (juce::int64 sample) const
{
    sample = juce::jmax (static_cast<juce::int64> (0), sample);
    return snapEnabled ? static_cast<juce::int64> (
                             timeline::snapSamples (sample, gridSamples))
                       : sample;
}

juce::String TrackRowComponent::formatDuration (double seconds) const
{
    const auto totalSeconds = juce::jmax (0, juce::roundToInt (seconds));
    return juce::String (totalSeconds / 60).paddedLeft ('0', 2)
           + ":"
           + juce::String (totalSeconds % 60).paddedLeft ('0', 2);
}
}
