#include "ArrangementComponent.h"

#include "core/TimelineMath.h"
#include "StudioLookAndFeel.h"

#include <cmath>
#include <utility>

namespace triumph
{
namespace
{
constexpr int rowContentInsetX = 8;

int timelineLeftOffset() noexcept
{
    return rowContentInsetX + TrackRowComponent::controlWidth
           + TrackRowComponent::timelineInset;
}
}

ArrangementComponent::ArrangementComponent (ProjectModel& projectModel,
                                            AudioEngine& sharedEngine)
    : project (projectModel), engine (sharedEngine)
{
    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (true, true);
    viewport.setScrollBarThickness (10);
    addAndMakeVisible (viewport);

    emptyState.setText ("Import audio, create an audio track, or add a MIDI instrument",
                        juce::dontSendNotification);
    emptyState.setJustificationType (juce::Justification::centred);
    emptyState.setColour (juce::Label::textColourId, StudioColours::textMuted);
    emptyState.getProperties().set ("fontSize", 16.0f);
    addAndMakeVisible (emptyState);
    rebuildRows();
}

ArrangementComponent::~ArrangementComponent()
{
    viewport.setViewedComponent (nullptr, false);
}

void ArrangementComponent::refreshFromModel()
{
    bool structureMatches = rows.size() == project.getTrackCount();

    if (structureMatches)
        for (int index = 0; index < rows.size(); ++index)
            if (rows[index]->getTrackId() != project.getTrackId (index))
                structureMatches = false;

    if (! structureMatches)
    {
        rebuildRows();
        return;
    }

    if (selectedClipId.isNotEmpty()
        && project.getClipState (selectedTrackId, selectedClipId).id.isEmpty())
    {
        selectedTrackId.clear();
        selectedClipId.clear();
    }

    for (auto* row : rows)
    {
        row->setTimelineView (pixelsPerSecond, snapEnabled, getGridSamples());
        row->setSelectedClip (row->getTrackId() == selectedTrackId ? selectedClipId
                                                                   : juce::String());
        row->setTrackSelected (row->getTrackId() == selectedTrackId);
        row->refreshFromModel();
    }

    emptyState.setVisible (rows.isEmpty());
    updateContentBounds();
    repaint();
}

void ArrangementComponent::setSnapEnabled (bool shouldSnap)
{
    if (snapEnabled == shouldSnap)
        return;

    snapEnabled = shouldSnap;
    refreshFromModel();
}

bool ArrangementComponent::isSnapEnabled() const noexcept
{
    return snapEnabled;
}

void ArrangementComponent::setPixelsPerSecond (double newPixelsPerSecond)
{
    newPixelsPerSecond = juce::jlimit (30.0, 320.0, newPixelsPerSecond);
    if (std::abs (pixelsPerSecond - newPixelsPerSecond) < 0.01)
        return;

    pixelsPerSecond = newPixelsPerSecond;
    refreshFromModel();
}

double ArrangementComponent::getPixelsPerSecond() const noexcept
{
    return pixelsPerSecond;
}

bool ArrangementComponent::splitSelectedClipAt (double timelineSeconds)
{
    if (! hasSelectedClip())
        return false;

    auto splitSample = static_cast<juce::int64> (timeline::secondsToSamples (
        timelineSeconds, project.getTimelineSampleRate()));
    if (snapEnabled)
        splitSample = static_cast<juce::int64> (
            timeline::snapSamples (splitSample, getGridSamples()));

    const auto newClipId = project.splitClip (selectedTrackId,
                                              selectedClipId,
                                              splitSample);
    if (newClipId.isEmpty())
        return false;

    selectedClipId = newClipId;
    refreshFromModel();
    return true;
}

bool ArrangementComponent::deleteSelectedClip()
{
    if (! hasSelectedClip())
        return false;

    const auto deleted = project.removeClip (selectedTrackId, selectedClipId);
    if (deleted)
    {
        selectedTrackId.clear();
        selectedClipId.clear();
        refreshFromModel();
    }
    return deleted;
}

bool ArrangementComponent::activateSelectedTake()
{
    return hasSelectedClip()
               && project.selectActiveTake (selectedTrackId, selectedClipId);
}

bool ArrangementComponent::hasSelectedClip() const noexcept
{
    return selectedTrackId.isNotEmpty() && selectedClipId.isNotEmpty();
}

void ArrangementComponent::setTrackDeleteRequestCallback (
    TrackDeleteRequestCallback callback)
{
    onTrackDeleteRequested = std::move (callback);
}

void ArrangementComponent::paint (juce::Graphics& graphics)
{
    graphics.fillAll (StudioColours::workspace);
    graphics.setColour (StudioColours::border.withAlpha (0.38f));
    graphics.drawRect (getLocalBounds(), 1);
    paintTimelineRuler (graphics);
}

void ArrangementComponent::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (rulerHeight);
    viewport.setBounds (bounds);
    emptyState.setBounds (bounds.reduced (40));
    updateContentBounds();
}

void ArrangementComponent::mouseDown (const juce::MouseEvent& event)
{
    if (! getRulerArea().contains (event.position.toInt()))
        return;

    const auto settings = project.getRecordingSettings();
    const auto sample = rulerSampleAtX (event.position.x);
    const auto loopStartX = rulerXForSample (settings.loopStartSamples);
    const auto loopEndX = rulerXForSample (settings.loopEndSamples);
    const auto hasLoop = settings.loopEndSamples > settings.loopStartSamples;

    rulerDragAnchorSamples = sample;
    if (hasLoop && std::abs (event.position.x - loopStartX) <= 8.0f)
        rulerDragMode = RulerDragMode::loopStart;
    else if (hasLoop && std::abs (event.position.x - loopEndX) <= 8.0f)
        rulerDragMode = RulerDragMode::loopEnd;
    else
    {
        rulerDragMode = RulerDragMode::seek;
        engine.setPositionSeconds (timeline::samplesToSeconds (
            sample, project.getTimelineSampleRate()));
    }
    repaint();
}

void ArrangementComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (rulerDragMode == RulerDragMode::none)
        return;

    if (rulerDragMode == RulerDragMode::seek
        && std::abs (event.position.x
                     - rulerXForSample (rulerDragAnchorSamples)) > 6.0f)
        rulerDragMode = RulerDragMode::loopRange;

    updateLoopRangeFromDrag (event.position.x);
}

void ArrangementComponent::mouseDoubleClick (const juce::MouseEvent& event)
{
    if (! getRulerArea().contains (event.position.toInt()))
        return;

    auto settings = project.getRecordingSettings();
    settings.loopStartSamples = 0;
    settings.loopEndSamples = 0;
    project.setRecordingSettings (settings);
    repaint();
}

void ArrangementComponent::rebuildRows()
{
    rows.clear (true);

    for (int index = 0; index < project.getTrackCount(); ++index)
    {
        auto* row = rows.add (new TrackRowComponent (project,
                                                     engine,
                                                     project.getTrackId (index)));
        row->setUserHeight (heightForTrack (project.getTrackId (index)));
        row->setTimelineView (pixelsPerSecond, snapEnabled, getGridSamples());
        row->setClipSelectionCallback ([this] (const juce::String& trackId,
                                               const juce::String& clipId)
        {
            selectClip (trackId, clipId);
        });
        row->setTrackDeleteRequestCallback ([this] (const juce::String& trackId)
        {
            if (onTrackDeleteRequested)
                onTrackDeleteRequested (trackId);
        });
        row->setTrackHeightChangeCallback ([this] (const juce::String& trackId,
                                                   int newHeight)
        {
            setHeightForTrack (trackId, newHeight);
            updateContentBounds();
        });
        row->setSelectedClip (row->getTrackId() == selectedTrackId ? selectedClipId
                                                                   : juce::String());
        row->setTrackSelected (row->getTrackId() == selectedTrackId);
        content.addAndMakeVisible (row);
    }

    emptyState.setVisible (rows.isEmpty());
    updateContentBounds();
}

void ArrangementComponent::updateContentBounds()
{
    auto rowsHeight = 12;
    for (auto* row : rows)
        rowsHeight += row->getPreferredHeight();
    const auto contentHeight = juce::jmax (viewport.getHeight()
                                               - viewport.getScrollBarThickness(),
                                           rowsHeight);
    const auto timelineSeconds = juce::jmax (15.0,
                                             timeline::samplesToSeconds (
                                                 project.getProjectLengthInSamples(),
                                                 project.getTimelineSampleRate()) + 5.0);
    const auto timelineWidth = juce::roundToInt (timelineSeconds * pixelsPerSecond);
    const auto minimumWidth = juce::jmax (760,
                                          getWidth() - viewport.getScrollBarThickness());
    content.setSize (juce::jmax (minimumWidth,
                                 timelineLeftOffset() + 8 + timelineWidth),
                     contentHeight);

    auto rowBounds = content.getLocalBounds().reduced (8, 6);

    for (auto* row : rows)
    {
        row->setBounds (rowBounds.removeFromTop (row->getPreferredHeight() - 6));
        rowBounds.removeFromTop (6);
    }
}

int ArrangementComponent::heightForTrack (const juce::String& trackId) const
{
    for (const auto& entry : trackHeights)
        if (entry.first == trackId)
            return entry.second;
    return TrackRowComponent::preferredHeight;
}

void ArrangementComponent::setHeightForTrack (const juce::String& trackId,
                                              int newHeight)
{
    const auto clampedHeight = juce::jlimit (
        TrackRowComponent::preferredHeight, 220, newHeight);
    for (auto& entry : trackHeights)
        if (entry.first == trackId)
        {
            entry.second = clampedHeight;
            return;
        }
    trackHeights.emplace_back (trackId, clampedHeight);
}

juce::Rectangle<int> ArrangementComponent::getRulerArea() const
{
    return getLocalBounds().removeFromTop (rulerHeight);
}

juce::int64 ArrangementComponent::rulerSampleAtX (float x) const
{
    const auto contentX = viewport.getViewPositionX() + x;
    const auto timelineX = static_cast<double> (timelineLeftOffset());
    const auto seconds = juce::jmax (0.0, (contentX - timelineX) / pixelsPerSecond);
    auto samples = static_cast<juce::int64> (timeline::secondsToSamples (
        seconds, project.getTimelineSampleRate()));
    return snapEnabled ? static_cast<juce::int64> (
        timeline::snapSamples (samples, getGridSamples())) : samples;
}

float ArrangementComponent::rulerXForSample (juce::int64 sample) const
{
    const auto seconds = timeline::samplesToSeconds (
        sample, project.getTimelineSampleRate());
    const auto contentX = static_cast<double> (timelineLeftOffset())
        + seconds * pixelsPerSecond;
    return static_cast<float> (contentX - viewport.getViewPositionX());
}

void ArrangementComponent::updateLoopRangeFromDrag (float x)
{
    auto settings = project.getRecordingSettings();
    const auto sample = rulerSampleAtX (x);

    if (rulerDragMode == RulerDragMode::loopStart)
        settings.loopStartSamples = juce::jlimit (
            static_cast<juce::int64> (0),
            juce::jmax (static_cast<juce::int64> (0),
                        settings.loopEndSamples - getGridSamples()),
            sample);
    else if (rulerDragMode == RulerDragMode::loopEnd)
        settings.loopEndSamples = juce::jmax (
            settings.loopStartSamples + getGridSamples(), sample);
    else if (rulerDragMode == RulerDragMode::loopRange)
    {
        settings.loopStartSamples = juce::jmin (rulerDragAnchorSamples, sample);
        settings.loopEndSamples = juce::jmax (rulerDragAnchorSamples, sample);
        if (settings.loopEndSamples == settings.loopStartSamples)
            settings.loopEndSamples += getGridSamples();
    }
    else
        return;

    project.setRecordingSettings (settings);
    repaint();
}

void ArrangementComponent::paintTimelineRuler (juce::Graphics& graphics)
{
    const auto area = getRulerArea();
    graphics.setColour (StudioColours::background);
    graphics.fillRect (area);
    graphics.setColour (StudioColours::border);
    graphics.drawHorizontalLine (area.getBottom() - 1,
                                 static_cast<float> (area.getX()),
                                 static_cast<float> (area.getRight()));

    auto leftHeader = area.withWidth (timelineLeftOffset());
    graphics.setColour (StudioColours::panel);
    graphics.fillRect (leftHeader);
    graphics.setColour (StudioColours::primary.withAlpha (0.85f));
    graphics.fillRect (leftHeader.withHeight (2));
    graphics.setColour (StudioColours::textMuted);
    graphics.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
    graphics.drawText ("TIMELINE", leftHeader.reduced (13, 4),
                       juce::Justification::centredLeft, false);

    const auto ruler = area.withTrimmedLeft (timelineLeftOffset());
    graphics.setColour (StudioColours::panelRaised);
    graphics.fillRect (ruler);
    graphics.setColour (StudioColours::border.withAlpha (0.50f));
    graphics.drawVerticalLine (ruler.getX(), static_cast<float> (ruler.getY()),
                               static_cast<float> (ruler.getBottom()));

    const auto bpm = juce::jmax (1.0, project.getTempoBpm());
    const auto beatSeconds = 60.0 / bpm;
    const auto signatures = project.getTimeSignatures();
    const auto beatsPerBar = signatures.empty()
        ? 4 : juce::jlimit (1, 32, signatures.front().numerator);
    const auto viewStartSeconds = juce::jmax (
        0.0,
        (static_cast<double> (viewport.getViewPositionX())
            - static_cast<double> (timelineLeftOffset()))
            / pixelsPerSecond);
    const auto viewEndSeconds = viewStartSeconds
        + static_cast<double> (juce::jmax (1, getWidth())) / pixelsPerSecond;
    const auto firstBeat = juce::jmax (
        0, static_cast<int> (std::floor (viewStartSeconds / beatSeconds)) - 1);
    const auto lastBeat = static_cast<int> (std::ceil (
        viewEndSeconds / beatSeconds)) + 2;

    for (int beat = firstBeat; beat <= lastBeat; ++beat)
    {
        const auto x = rulerXForSample (static_cast<juce::int64> (
            timeline::secondsToSamples (beat * beatSeconds,
                                        project.getTimelineSampleRate())));
        if (x < static_cast<float> (ruler.getX() - 20)
            || x > static_cast<float> (ruler.getRight() + 20))
            continue;

        const auto isBar = beat % beatsPerBar == 0;
        graphics.setColour (isBar ? StudioColours::textMuted
                                  : StudioColours::border.withAlpha (0.52f));
        graphics.drawVerticalLine (juce::roundToInt (x),
                                   static_cast<float> (ruler.getY()
                                                       + (isBar ? 8 : 22)),
                                   static_cast<float> (ruler.getBottom()));
        if (isBar)
        {
            graphics.setFont (10.0f);
            graphics.setColour (StudioColours::text);
            graphics.drawText (juce::String (beat / beatsPerBar + 1),
                               juce::Rectangle<int> (
                                   juce::roundToInt (x) + 4,
                                   ruler.getY() + 5, 44, 16),
                               juce::Justification::centredLeft, false);
        }
    }

    const auto settings = project.getRecordingSettings();
    if (settings.loopEndSamples > settings.loopStartSamples)
    {
        const auto loopStartX = rulerXForSample (settings.loopStartSamples);
        const auto loopEndX = rulerXForSample (settings.loopEndSamples);
        juce::Rectangle<float> loopArea {
            loopStartX, static_cast<float> (ruler.getY() + 7),
            juce::jmax (3.0f, loopEndX - loopStartX),
            static_cast<float> (ruler.getHeight() - 14)
        };
        graphics.setColour (StudioColours::primary.withAlpha (0.10f));
        graphics.fillRect (loopArea.withTrimmedTop (12.0f));
        graphics.setColour (StudioColours::primary);
        graphics.drawLine (loopArea.getX(), loopArea.getY(),
                           loopArea.getRight(), loopArea.getY(), 2.0f);
        graphics.drawLine (loopArea.getX(), loopArea.getY(),
                           loopArea.getX(), loopArea.getY() + 16.0f, 2.0f);
        graphics.drawLine (loopArea.getRight(), loopArea.getY(),
                           loopArea.getRight(), loopArea.getY() + 16.0f, 2.0f);
        graphics.fillRect (juce::Rectangle<float> (
            loopStartX - 2.0f, static_cast<float> (ruler.getY() + 8),
            4.0f, static_cast<float> (ruler.getHeight() - 10)));
        graphics.fillRect (juce::Rectangle<float> (
            loopEndX - 2.0f, static_cast<float> (ruler.getY() + 8),
            4.0f, static_cast<float> (ruler.getHeight() - 10)));
        graphics.setColour (StudioColours::text);
        graphics.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
        graphics.drawText ("LOOP", loopArea.toNearestInt().reduced (8, 0),
                           juce::Justification::centredTop, false);
    }

    const auto playheadX = static_cast<float> (
        timelineLeftOffset() - viewport.getViewPositionX()
        + engine.getPositionSeconds() * pixelsPerSecond);
    if (playheadX >= static_cast<float> (ruler.getX())
        && playheadX <= static_cast<float> (ruler.getRight()))
    {
        graphics.setColour (StudioColours::playhead);
        graphics.drawVerticalLine (juce::roundToInt (playheadX),
                                   static_cast<float> (ruler.getY()),
                                   static_cast<float> (ruler.getBottom()));
    }
}

void ArrangementComponent::selectClip (const juce::String& trackId,
                                       const juce::String& clipId)
{
    selectedTrackId = trackId;
    selectedClipId = clipId;

    for (auto* row : rows)
    {
        row->setSelectedClip (row->getTrackId() == selectedTrackId ? selectedClipId
                                                                   : juce::String());
        row->setTrackSelected (row->getTrackId() == selectedTrackId);
    }
}

juce::int64 ArrangementComponent::getGridSamples() const
{
    return juce::jmax (static_cast<juce::int64> (1),
                       static_cast<juce::int64> (timeline::secondsToSamples (
                           0.25, project.getTimelineSampleRate())));
}
}
