#include "TempoAutomationComponent.h"

#include "StudioLookAndFeel.h"
#include "core/TempoMap.h"

#include <algorithm>
#include <cmath>

namespace triumph
{
namespace
{
std::vector<tempo::Point> tempoPointsFor (const ProjectModel& project)
{
    std::vector<tempo::Point> result;
    for (const auto& point : project.getTempoPoints())
        result.push_back ({ point.beat, point.bpm,
            point.curve == TempoCurve::linear ? tempo::SegmentCurve::linear
                                               : tempo::SegmentCurve::step });
    return tempo::normalised (std::move (result));
}

juce::String modeName (AutomationMode mode)
{
    switch (mode)
    {
        case AutomationMode::touch: return "TOUCH";
        case AutomationMode::latch: return "LATCH";
        case AutomationMode::write: return "WRITE";
        case AutomationMode::read: break;
    }
    return "READ";
}

bool isPluginParameter (const juce::String& parameterId)
{
    return parameterId.startsWith ("plugin:");
}

bool splitInsertParameter (AudioEngine& engine,
                           const juce::String& qualifiedId,
                           juce::String& slotId,
                           juce::String& stableParameterId)
{
    if (! qualifiedId.startsWith ("plugin:"))
        return false;
    const auto remainder = qualifiedId.substring (7);
    const auto separator = remainder.indexOfChar (':');
    if (separator <= 0)
        return false;
    const auto candidate = remainder.substring (0, separator);
    const auto loaded = engine.getLoadedInsertPluginSlotIds();
    if (std::find (loaded.begin(), loaded.end(), candidate) == loaded.end())
        return false;
    slotId = candidate;
    stableParameterId = remainder.substring (separator + 1);
    return stableParameterId.startsWith ("plugin:");
}

float normalisedAutomationValue (const juce::String& parameterId,
                                 float value)
{
    if (parameterId == "pan")
        return (value + 1.0f) * 0.5f;
    return isPluginParameter (parameterId) ? value : value / 1.5f;
}

float automationValueFromNormalised (const juce::String& parameterId,
                                     float normalised)
{
    if (parameterId == "pan")
        return normalised * 2.0f - 1.0f;
    return isPluginParameter (parameterId) ? normalised : normalised * 1.5f;
}
}

class TempoAutomationComponent::TimelineCanvas final : public juce::Component
{
public:
    TimelineCanvas (ProjectModel& model, AudioEngine& sharedEngine,
                    std::function<juce::String()> laneProvider,
                    std::function<AutomationCurve()> curveProvider)
        : project (model), engine (sharedEngine),
          getLaneId (std::move (laneProvider)),
          getCurve (std::move (curveProvider))
    {
        setMouseCursor (juce::MouseCursor::CrosshairCursor);
    }

    void paint (juce::Graphics& graphics) override
    {
        auto area = getLocalBounds();
        graphics.setColour (StudioColours::surface);
        graphics.fillRect (area);
        graphics.setColour (StudioColours::border);
        graphics.drawRect (area);

        const auto ruler = area.removeFromTop (27);
        const auto tempoLane = area.removeFromTop (72);
        const auto eventLane = area.removeFromTop (42);
        const auto automationLane = area;
        graphics.setColour (StudioColours::panelRaised);
        graphics.fillRect (ruler);
        graphics.setColour (StudioColours::panel);
        graphics.fillRect (eventLane);
        graphics.setColour (StudioColours::border.withAlpha (0.55f));
        graphics.drawHorizontalLine (ruler.getBottom(), 0.0f,
                                     static_cast<float> (getWidth()));
        graphics.drawHorizontalLine (tempoLane.getBottom(), 0.0f,
                                     static_cast<float> (getWidth()));
        graphics.drawHorizontalLine (eventLane.getBottom(), 0.0f,
                                     static_cast<float> (getWidth()));

        const auto length = juce::jmax (8.0, engine.getProjectLengthSeconds());
        const auto xForSeconds = [this, length] (double seconds)
        {
            return 58.0f + static_cast<float> (juce::jlimit (0.0, length, seconds)
                                               / length)
                             * static_cast<float> (juce::jmax (1, getWidth() - 64));
        };
        graphics.setFont (10.0f);
        for (int second = 0; second <= static_cast<int> (std::ceil (length)); ++second)
        {
            const auto x = xForSeconds (second);
            graphics.setColour (StudioColours::border.withAlpha (
                second % 5 == 0 ? 0.60f : 0.25f));
            graphics.drawVerticalLine (static_cast<int> (x),
                                       static_cast<float> (ruler.getY()),
                                       static_cast<float> (getHeight()));
            if (second % 5 == 0)
            {
                graphics.setColour (StudioColours::textMuted);
                graphics.drawText (juce::String (second) + "s",
                                   static_cast<int> (x) + 3, ruler.getY(),
                                   36, ruler.getHeight(),
                                   juce::Justification::centredLeft, false);
            }
        }

        graphics.setColour (StudioColours::textMuted);
        graphics.drawText ("TEMPO", 8, tempoLane.getY(), 48, tempoLane.getHeight(),
                           juce::Justification::centredLeft, false);
        graphics.drawText ("EVENTS", 8, eventLane.getY(), 48, eventLane.getHeight(),
                           juce::Justification::centredLeft, false);
        graphics.drawText ("AUTO", 8, automationLane.getY(), 48,
                           automationLane.getHeight(),
                           juce::Justification::centredLeft, false);

        const auto points = tempoPointsFor (project);
        juce::Path tempoPath;
        bool first = true;
        for (std::size_t index = 0; index < points.size(); ++index)
        {
            const auto x = xForSeconds (tempo::beatToSeconds (points[index].beat,
                                                               points));
            const auto y = static_cast<float> (tempoLane.getBottom() - 8)
                - static_cast<float> ((points[index].bpm - 20.0) / 380.0)
                    * static_cast<float> (tempoLane.getHeight() - 16);
            if (first) { tempoPath.startNewSubPath (x, y); first = false; }
            else tempoPath.lineTo (x, y);
            graphics.setColour (StudioColours::primary);
            graphics.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);
            graphics.setColour (StudioColours::text);
            graphics.drawText (juce::String (points[index].bpm, 1),
                               static_cast<int> (x) + 5, static_cast<int> (y) - 9,
                               48, 18, juce::Justification::centredLeft, false);
            if (index + 1 < points.size()
                && points[index].curve == tempo::SegmentCurve::step)
            {
                const auto nextX = xForSeconds (tempo::beatToSeconds (
                    points[index + 1].beat, points));
                tempoPath.lineTo (nextX, y);
            }
        }
        graphics.setColour (StudioColours::primary);
        graphics.strokePath (tempoPath, juce::PathStrokeType (1.7f));

        for (const auto& signature : project.getTimeSignatures())
        {
            const auto x = xForSeconds (tempo::beatToSeconds (signature.beat,
                                                               points));
            graphics.setColour (StudioColours::green);
            graphics.drawVerticalLine (static_cast<int> (x),
                                       static_cast<float> (eventLane.getY()),
                                       static_cast<float> (eventLane.getBottom()));
            graphics.drawText (juce::String (signature.numerator) + "/"
                                   + juce::String (signature.denominator),
                               static_cast<int> (x) + 3, eventLane.getY(), 42,
                               eventLane.getHeight() / 2,
                               juce::Justification::centredLeft, false);
        }
        for (const auto& marker : project.getMarkers())
        {
            const auto x = xForSeconds (tempo::beatToSeconds (marker.beat, points));
            graphics.setColour (StudioColours::orange);
            graphics.drawVerticalLine (static_cast<int> (x),
                                       static_cast<float> (eventLane.getCentreY()),
                                       static_cast<float> (getHeight()));
            graphics.drawText (marker.name, static_cast<int> (x) + 3,
                               eventLane.getCentreY(), 90,
                               eventLane.getHeight() / 2,
                               juce::Justification::centredLeft, false);
        }

        const auto laneId = getLaneId();
        const auto lane = project.getAutomationLane (laneId);
        if (lane.id.isNotEmpty())
        {
            juce::Path path;
            bool pathStarted = false;
            for (const auto& point : lane.points)
            {
                const auto seconds = static_cast<double> (point.samplePosition)
                                     / project.getTimelineSampleRate();
                const auto x = xForSeconds (seconds);
                auto normal = normalisedAutomationValue (
                    lane.parameterId, point.value);
                normal = juce::jlimit (0.0f, 1.0f, normal);
                const auto y = static_cast<float> (automationLane.getBottom() - 8)
                    - normal * static_cast<float> (automationLane.getHeight() - 16);
                if (! pathStarted) { path.startNewSubPath (x, y); pathStarted = true; }
                else path.lineTo (x, y);
                graphics.setColour (StudioColours::primaryBright);
                graphics.fillEllipse (x - 4.0f, y - 4.0f, 8.0f, 8.0f);
            }
            graphics.setColour (StudioColours::primaryBright);
            graphics.strokePath (path, juce::PathStrokeType (1.8f));
            graphics.setColour (StudioColours::textMuted);
            graphics.drawText (lane.parameterId.toUpperCase() + "  "
                                   + modeName (lane.mode),
                               62, automationLane.getY() + 3, 180, 18,
                               juce::Justification::centredLeft, false);
        }

        const auto playheadX = xForSeconds (engine.getPositionSeconds());
        graphics.setColour (StudioColours::playhead);
        graphics.drawVerticalLine (static_cast<int> (playheadX), 0.0f,
                                   static_cast<float> (getHeight()));
    }

    void mouseDoubleClick (const juce::MouseEvent& event) override
    {
        const auto laneId = getLaneId();
        const auto lane = project.getAutomationLane (laneId);
        const auto automationTop = 27 + 72 + 42;
        if (lane.id.isEmpty() || event.y < automationTop)
            return;
        const auto length = juce::jmax (8.0, engine.getProjectLengthSeconds());
        const auto amount = juce::jlimit (0.0, 1.0,
            static_cast<double> (event.x - 58)
                / static_cast<double> (juce::jmax (1, getWidth() - 64)));
        const auto timelineSample = static_cast<juce::int64> (std::llround (
            amount * length * project.getTimelineSampleRate()));
        const auto normal = juce::jlimit (0.0f, 1.0f,
            static_cast<float> (getHeight() - 8 - event.y)
                / static_cast<float> (juce::jmax (1, getHeight() - automationTop - 16)));
        const auto value = automationValueFromNormalised (
            lane.parameterId, normal);
        project.addAutomationPoint (laneId, timelineSample, value, getCurve());
    }

    void mouseDown (const juce::MouseEvent& event) override
    {
        if (! event.mods.isPopupMenu())
            return;
        const auto laneId = getLaneId();
        const auto lane = project.getAutomationLane (laneId);
        if (lane.id.isEmpty())
            return;
        const auto automationTop = 27 + 72 + 42;
        const auto length = juce::jmax (8.0, engine.getProjectLengthSeconds());
        juce::String nearestId;
        auto nearestDistance = 14.0f;
        for (const auto& point : lane.points)
        {
            const auto seconds = static_cast<double> (point.samplePosition)
                                 / project.getTimelineSampleRate();
            const auto x = 58.0f + static_cast<float> (
                juce::jlimit (0.0, length, seconds) / length)
                    * static_cast<float> (juce::jmax (1, getWidth() - 64));
            auto normal = normalisedAutomationValue (
                lane.parameterId, point.value);
            normal = juce::jlimit (0.0f, 1.0f, normal);
            const auto y = static_cast<float> (getHeight() - 8)
                - normal * static_cast<float> (
                    juce::jmax (1, getHeight() - automationTop - 16));
            const auto distance = juce::Point<float> (
                static_cast<float> (event.x), static_cast<float> (event.y))
                    .getDistanceFrom (juce::Point<float> (x, y));
            if (distance < nearestDistance)
            {
                nearestDistance = distance;
                nearestId = point.id;
            }
        }
        if (nearestId.isNotEmpty())
            project.removeAutomationPoint (laneId, nearestId);
    }

private:
    ProjectModel& project;
    AudioEngine& engine;
    std::function<juce::String()> getLaneId;
    std::function<AutomationCurve()> getCurve;
};

TempoAutomationComponent::TempoAutomationComponent (
    ProjectModel& projectModel, AudioEngine& sharedEngine)
    : project (projectModel), engine (sharedEngine)
{
    canvas = std::make_unique<TimelineCanvas> (
        project, engine,
        [this] { return selectedLaneId(); },
        [this] { return selectedCurve(); });
    addAndMakeVisible (*canvas);
    juce::Component* components[] {
        &titleLabel, &addTempoButton, &addSignatureButton,
        &addMarkerButton, &metronomeButton, &metronomeSlider,
        &targetBox, &parameterBox, &modeBox, &curveBox, &laneBox,
        &tempoValueSlider, &automationValueSlider, &numeratorBox, &denominatorBox,
        &addLaneButton, &addPointButton, &removeLaneButton
    };
    for (auto* component : components)
        addAndMakeVisible (*component);

    titleLabel.setColour (juce::Label::textColourId, StudioColours::text);
    titleLabel.getProperties().set ("bold", true);
    metronomeButton.setClickingTogglesState (true);
    metronomeButton.setColour (juce::TextButton::buttonOnColourId,
                               StudioColours::orange);
    metronomeButton.onClick = [this]
    {
        project.setMetronomeEnabled (metronomeButton.getToggleState());
    };
    metronomeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    metronomeSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 38, 20);
    metronomeSlider.setRange (0.0, 1.0, 0.01);
    metronomeSlider.onDragStart = [this]
    {
        project.beginUndoTransaction ("Adjust Metronome Level");
    };
    metronomeSlider.onValueChange = [this]
    {
        project.setMetronomeGain (static_cast<float> (metronomeSlider.getValue()));
    };
    parameterBox.addItem ("Gain", 1);
    parameterBox.addItem ("Pan", 2);
    parameterBox.setSelectedId (1, juce::dontSendNotification);
    modeBox.addItem ("READ", 1);
    modeBox.addItem ("TOUCH", 2);
    modeBox.addItem ("LATCH", 3);
    modeBox.addItem ("WRITE", 4);
    modeBox.setSelectedId (1, juce::dontSendNotification);
    curveBox.addItem ("Hold", 1);
    curveBox.addItem ("Linear", 2);
    curveBox.addItem ("Smooth", 3);
    curveBox.setSelectedId (2, juce::dontSendNotification);
    tempoValueSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    tempoValueSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 52, 20);
    tempoValueSlider.setRange (20.0, 400.0, 0.1);
    tempoValueSlider.setTextValueSuffix (" BPM");
    automationValueSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    automationValueSlider.setTextBoxStyle (
        juce::Slider::TextBoxRight, false, 42, 20);
    automationValueSlider.setRange (0.0, 1.5, 0.001);
    automationValueSlider.onDragStart = [this]
    {
        automationGestureActive = true;
        const auto lane = project.getAutomationLane (selectedLaneId());
        if (lane.mode == AutomationMode::latch)
            latchEngaged = true;
        lastAutomationWriteSample = -1;
        project.beginUndoTransaction ("Write Automation");
    };
    automationValueSlider.onDragEnd = [this]
    {
        writeAutomationValue (true);
        automationGestureActive = false;
    };
    automationValueSlider.onValueChange = [this]
    {
        const auto lane = project.getAutomationLane (selectedLaneId());
        if (lane.id.isEmpty())
            return;
        setCurrentParameterValue (
            lane.targetId, lane.parameterId,
            static_cast<float> (automationValueSlider.getValue()));
        writeAutomationValue (true);
    };
    for (int value = 1; value <= 16; ++value)
        numeratorBox.addItem (juce::String (value), value);
    numeratorBox.setSelectedId (4, juce::dontSendNotification);
    for (const auto value : { 1, 2, 4, 8, 16, 32 })
        denominatorBox.addItem (juce::String (value), value);
    denominatorBox.setSelectedId (4, juce::dontSendNotification);

    addTempoButton.onClick = [this] { addTempoAtPlayhead(); };
    addSignatureButton.onClick = [this] { addSignatureAtPlayhead(); };
    addMarkerButton.onClick = [this] { addMarkerAtPlayhead(); };
    addLaneButton.onClick = [this] { addLane(); };
    addPointButton.onClick = [this] { addPointAtPlayhead(); };
    removeLaneButton.onClick = [this]
    {
        if (selectedLaneId().isNotEmpty())
            project.removeAutomationLane (selectedLaneId());
    };
    modeBox.onChange = [this]
    {
        const auto laneId = selectedLaneId();
        if (laneId.isNotEmpty())
        {
            if (modeBox.getSelectedId() == 4 && engine.isPlaying())
                project.beginUndoTransaction ("Write Automation");
            project.setAutomationLaneMode (laneId,
                modeBox.getSelectedId() == 2 ? AutomationMode::touch
                : modeBox.getSelectedId() == 3 ? AutomationMode::latch
                : modeBox.getSelectedId() == 4 ? AutomationMode::write
                                               : AutomationMode::read);
        }
    };
    laneBox.onChange = [this]
    {
        const auto lane = project.getAutomationLane (selectedLaneId());
        if (lane.id.isNotEmpty())
            modeBox.setSelectedId (lane.mode == AutomationMode::touch ? 2
                : lane.mode == AutomationMode::latch ? 3
                : lane.mode == AutomationMode::write ? 4 : 1,
                juce::dontSendNotification);
        latchEngaged = false;
        lastAutomationWriteSample = -1;
        refreshWriteControl();
        canvas->repaint();
    };
    refreshFromModel();
    startTimerHz (20);
}

TempoAutomationComponent::~TempoAutomationComponent() { stopTimer(); }

void TempoAutomationComponent::refreshFromModel()
{
    tempoValueSlider.setValue (project.getTempoBpm(), juce::dontSendNotification);
    metronomeButton.setToggleState (project.getMetronomeEnabled(),
                                    juce::dontSendNotification);
    metronomeSlider.setValue (project.getMetronomeGain(), juce::dontSendNotification);
    rebuildTargets();
    rebuildLanes();
    refreshWriteControl();
    canvas->repaint();
}

void TempoAutomationComponent::paint (juce::Graphics& graphics)
{
    graphics.fillAll (StudioColours::panel);
    graphics.setColour (StudioColours::border);
    graphics.drawHorizontalLine (0, 0.0f, static_cast<float> (getWidth()));
}

void TempoAutomationComponent::resized()
{
    auto bounds = getLocalBounds().reduced (10, 7);
    auto musicalHeader = bounds.removeFromTop (30);
    titleLabel.setBounds (musicalHeader.removeFromLeft (150));
    addTempoButton.setBounds (musicalHeader.removeFromLeft (76).reduced (2));
    tempoValueSlider.setBounds (musicalHeader.removeFromLeft (118).reduced (2));
    addSignatureButton.setBounds (musicalHeader.removeFromLeft (90).reduced (2));
    numeratorBox.setBounds (musicalHeader.removeFromLeft (46).reduced (2));
    denominatorBox.setBounds (musicalHeader.removeFromLeft (46).reduced (2));
    addMarkerButton.setBounds (musicalHeader.removeFromLeft (76).reduced (2));
    metronomeButton.setBounds (musicalHeader.removeFromLeft (90).reduced (2));
    metronomeSlider.setBounds (musicalHeader.removeFromLeft (92).reduced (2));
    auto automationHeader = bounds.removeFromTop (30);
    targetBox.setBounds (automationHeader.removeFromLeft (128).reduced (2));
    parameterBox.setBounds (automationHeader.removeFromLeft (72).reduced (2));
    modeBox.setBounds (automationHeader.removeFromLeft (76).reduced (2));
    curveBox.setBounds (automationHeader.removeFromLeft (76).reduced (2));
    addLaneButton.setBounds (automationHeader.removeFromLeft (70).reduced (2));
    laneBox.setBounds (automationHeader.removeFromLeft (190).reduced (2));
    addPointButton.setBounds (automationHeader.removeFromLeft (70).reduced (2));
    automationValueSlider.setBounds (
        automationHeader.removeFromLeft (140).reduced (2));
    removeLaneButton.setBounds (automationHeader.removeFromLeft (92).reduced (2));
    bounds.removeFromTop (5);
    canvas->setBounds (bounds);
}

void TempoAutomationComponent::timerCallback()
{
    const auto transportPlaying = engine.isPlaying();
    if (transportPlaying && ! transportWasPlaying)
    {
        const auto lane = project.getAutomationLane (selectedLaneId());
        if (lane.mode == AutomationMode::write)
            project.beginUndoTransaction ("Write Automation");
    }
    if (! transportPlaying && transportWasPlaying)
    {
        latchEngaged = false;
        lastAutomationWriteSample = -1;
    }
    if (transportPlaying)
        writeAutomationValue();
    transportWasPlaying = transportPlaying;
    rebuildTargets();
    rebuildLanes();
    canvas->repaint();
}

void TempoAutomationComponent::rebuildTargets()
{
    juce::String signature ("master;");
    for (int index = 0; index < project.getTrackCount(); ++index)
    {
        const auto track = project.getTrackState (project.getTrackId (index));
        signature += track.id + ":" + track.name + ";";
    }
    for (const auto& channel : project.getMixerChannels())
        signature += channel.id + ":" + channel.name + ";";
    if (signature == targetSignature)
        return;
    const auto selected = selectedTargetId();
    targetBox.clear (juce::dontSendNotification);
    targetBox.addItem ("Master", 1);
    int item = 2;
    for (int index = 0; index < project.getTrackCount(); ++index)
    {
        const auto track = project.getTrackState (project.getTrackId (index));
        targetBox.addItem (track.name, item++);
        targetBox.getProperties().set (
            juce::Identifier ("target_" + juce::String (item - 1)), track.id);
    }
    for (const auto& channel : project.getMixerChannels())
    {
        targetBox.addItem (channel.name, item++);
        targetBox.getProperties().set (
            juce::Identifier ("target_" + juce::String (item - 1)), channel.id);
    }
    targetBox.setSelectedId (1, juce::dontSendNotification);
    for (int id = 2; id < item; ++id)
        if (targetBox.getProperties()[juce::Identifier (
                "target_" + juce::String (id))].toString()
            == selected)
            targetBox.setSelectedId (id, juce::dontSendNotification);
    targetSignature = signature;
}

void TempoAutomationComponent::rebuildLanes()
{
    juce::String signature;
    const auto lanes = project.getAutomationLanes();
    for (const auto& lane : lanes)
        signature += lane.id + ":" + lane.targetId + ":" + lane.parameterId
                     + ":" + juce::String (static_cast<int> (lane.mode)) + ";";
    if (signature == laneSignature)
        return;
    const auto selected = selectedLaneId();
    laneBox.clear (juce::dontSendNotification);
    int item = 1;
    for (const auto& lane : lanes)
    {
        auto targetName = juce::String ("Master");
        if (lane.targetId != "master")
        {
            const auto track = project.getTrackState (lane.targetId);
            const auto channel = project.getMixerChannelState (lane.targetId);
            targetName = track.id.isNotEmpty() ? track.name
                       : channel.id.isNotEmpty() ? channel.name : "Missing";
        }
        laneBox.addItem (targetName + " " + lane.parameterId.toUpperCase()
                             + " - " + modeName (lane.mode), item);
        laneBox.getProperties().set (
            juce::Identifier ("lane_" + juce::String (item)), lane.id);
        ++item;
    }
    if (item > 1) laneBox.setSelectedId (1, juce::dontSendNotification);
    for (int id = 1; id < item; ++id)
        if (laneBox.getProperties()[juce::Identifier (
                "lane_" + juce::String (id))].toString()
            == selected)
            laneBox.setSelectedId (id, juce::dontSendNotification);
    laneSignature = signature;
}

double TempoAutomationComponent::playheadBeat() const
{
    return tempo::secondsToBeat (engine.getPositionSeconds(),
                                 tempoPointsFor (project));
}

juce::String TempoAutomationComponent::selectedTargetId() const
{
    const auto id = targetBox.getSelectedId();
    return id <= 1 ? juce::String ("master")
                   : targetBox.getProperties()[juce::Identifier (
                       "target_" + juce::String (id))].toString();
}

juce::String TempoAutomationComponent::selectedLaneId() const
{
    const auto id = laneBox.getSelectedId();
    return id > 0 ? laneBox.getProperties()[juce::Identifier (
                        "lane_" + juce::String (id))].toString()
                  : juce::String();
}

AutomationCurve TempoAutomationComponent::selectedCurve() const
{
    return curveBox.getSelectedId() == 1 ? AutomationCurve::hold
        : curveBox.getSelectedId() == 3 ? AutomationCurve::smooth
                                        : AutomationCurve::linear;
}

float TempoAutomationComponent::currentParameterValue (
    const juce::String& targetId, const juce::String& parameterId)
{
    if (isPluginParameter (parameterId))
    {
        juce::String slotId;
        juce::String stableId;
        if (splitInsertParameter (engine, parameterId, slotId, stableId))
        {
            for (const auto& parameter :
                 engine.getInsertPluginParameters (slotId))
                if (parameter.stableId == stableId)
                    return parameter.currentValue;
            return 0.0f;
        }
        for (const auto& parameter : engine.getInstrumentPluginParameters())
            if (parameter.stableId == parameterId)
                return parameter.currentValue;
        return 0.0f;
    }
    if (targetId == "master")
        return parameterId == "pan" ? 0.0f : project.getMasterGain();
    const auto track = project.getTrackState (targetId);
    if (track.id.isNotEmpty())
        return parameterId == "pan" ? track.pan : track.gain;
    const auto channel = project.getMixerChannelState (targetId);
    return parameterId == "pan" ? channel.pan : channel.gain;
}

void TempoAutomationComponent::setCurrentParameterValue (
    const juce::String& targetId, const juce::String& parameterId, float value)
{
    if (isPluginParameter (parameterId))
    {
        juce::String slotId;
        juce::String stableId;
        if (splitInsertParameter (engine, parameterId, slotId, stableId))
            engine.setInsertPluginParameterValue (slotId, stableId, value);
        else
            engine.setInstrumentPluginParameterValue (parameterId, value);
        return;
    }
    if (targetId == "master")
    {
        if (parameterId == "gain")
            project.setMasterGain (value);
        return;
    }
    const auto track = project.getTrackState (targetId);
    if (track.id.isNotEmpty())
    {
        if (parameterId == "pan") project.setTrackPan (targetId, value);
        else project.setTrackGain (targetId, value);
        return;
    }
    const auto channel = project.getMixerChannelState (targetId);
    if (channel.id.isNotEmpty())
    {
        if (parameterId == "pan") project.setMixerChannelPan (targetId, value);
        else project.setMixerChannelGain (targetId, value);
    }
}

void TempoAutomationComponent::writeAutomationValue (bool force)
{
    if (! engine.isPlaying())
        return;
    const auto lane = project.getAutomationLane (selectedLaneId());
    if (lane.id.isEmpty())
        return;
    const auto mode = lane.mode == AutomationMode::touch ? automation::Mode::touch
        : lane.mode == AutomationMode::latch ? automation::Mode::latch
        : lane.mode == AutomationMode::write ? automation::Mode::write
                                             : automation::Mode::read;
    if (! automation::shouldWrite (mode, automationGestureActive, latchEngaged))
        return;
    const auto sample = static_cast<juce::int64> (std::llround (
        engine.getPositionSeconds() * project.getTimelineSampleRate()));
    const auto minimumSpacing = static_cast<juce::int64> (std::llround (
        project.getTimelineSampleRate() / 40.0));
    if (! force && lastAutomationWriteSample >= 0
        && sample >= lastAutomationWriteSample
        && sample - lastAutomationWriteSample < minimumSpacing)
        return;
    project.addAutomationPoint (
        lane.id, sample, static_cast<float> (automationValueSlider.getValue()),
        selectedCurve(), false);
    lastAutomationWriteSample = sample;
}

void TempoAutomationComponent::refreshWriteControl()
{
    const auto lane = project.getAutomationLane (selectedLaneId());
    const auto pan = lane.parameterId == "pan";
    const auto pluginParameter = isPluginParameter (lane.parameterId);
    automationValueSlider.setRange (pan ? -1.0 : 0.0,
                                    pan || pluginParameter ? 1.0 : 1.5,
                                    0.001);
    automationValueSlider.setTextValueSuffix (
        pan || pluginParameter ? "" : "x");
    automationValueSlider.setValue (
        currentParameterValue (lane.targetId, lane.parameterId),
        juce::dontSendNotification);
    automationValueSlider.setEnabled (lane.id.isNotEmpty());
}

void TempoAutomationComponent::addTempoAtPlayhead()
{
    project.addTempoPoint (playheadBeat(), tempoValueSlider.getValue(),
                           curveBox.getSelectedId() == 1 ? TempoCurve::step
                                                        : TempoCurve::linear);
}

void TempoAutomationComponent::addSignatureAtPlayhead()
{
    project.addTimeSignature (playheadBeat(), numeratorBox.getSelectedId(),
                              denominatorBox.getSelectedId());
}

void TempoAutomationComponent::addMarkerAtPlayhead()
{
    project.addMarker (playheadBeat());
}

void TempoAutomationComponent::addLane()
{
    const auto mode = modeBox.getSelectedId() == 2 ? AutomationMode::touch
        : modeBox.getSelectedId() == 3 ? AutomationMode::latch
        : modeBox.getSelectedId() == 4 ? AutomationMode::write
                                       : AutomationMode::read;
    const auto id = project.addAutomationLane (
        selectedTargetId(), parameterBox.getSelectedId() == 2 ? "pan" : "gain",
        mode);
    laneSignature.clear();
    rebuildLanes();
    const auto lanes = project.getAutomationLanes();
    for (int index = 0; index < static_cast<int> (lanes.size()); ++index)
        if (lanes[static_cast<std::size_t> (index)].id == id)
            laneBox.setSelectedId (index + 1, juce::sendNotificationSync);
}

void TempoAutomationComponent::addPointAtPlayhead()
{
    auto laneId = selectedLaneId();
    if (laneId.isEmpty())
    {
        addLane();
        laneId = selectedLaneId();
    }
    const auto lane = project.getAutomationLane (laneId);
    if (lane.id.isEmpty()) return;
    const auto sample = static_cast<juce::int64> (std::llround (
        engine.getPositionSeconds() * project.getTimelineSampleRate()));
    project.addAutomationPoint (laneId, sample,
        currentParameterValue (lane.targetId, lane.parameterId), selectedCurve());
}
}
