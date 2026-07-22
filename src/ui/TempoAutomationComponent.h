#pragma once

#include <JuceHeader.h>

#include "audio/AudioEngine.h"
#include "model/ProjectModel.h"

#include <memory>
#include <functional>

namespace triumph
{
class TempoAutomationComponent final : public juce::Component,
                                       private juce::Timer
{
public:
    TempoAutomationComponent (ProjectModel& projectModel,
                              AudioEngine& sharedEngine);
    ~TempoAutomationComponent() override;

    void refreshFromModel();
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class TimelineCanvas;

    void timerCallback() override;
    void rebuildTargets();
    void rebuildLanes();
    double playheadBeat() const;
    juce::String selectedTargetId() const;
    juce::String selectedLaneId() const;
    AutomationCurve selectedCurve() const;
    float currentParameterValue (const juce::String& targetId,
                                 const juce::String& parameterId);
    void setCurrentParameterValue (const juce::String& targetId,
                                   const juce::String& parameterId,
                                   float value);
    void writeAutomationValue (bool force = false);
    void refreshWriteControl();
    void addTempoAtPlayhead();
    void addSignatureAtPlayhead();
    void addMarkerAtPlayhead();
    void addLane();
    void addPointAtPlayhead();

    ProjectModel& project;
    AudioEngine& engine;
    juce::Label titleLabel { {}, "TEMPO + AUTOMATION" };
    juce::TextButton addTempoButton { "+ Tempo" };
    juce::TextButton addSignatureButton { "+ Signature" };
    juce::TextButton addMarkerButton { "+ Marker" };
    juce::TextButton metronomeButton { "Metronome" };
    juce::Slider metronomeSlider;
    juce::ComboBox targetBox;
    juce::ComboBox parameterBox;
    juce::ComboBox modeBox;
    juce::ComboBox curveBox;
    juce::ComboBox laneBox;
    juce::Slider tempoValueSlider;
    juce::Slider automationValueSlider;
    juce::ComboBox numeratorBox;
    juce::ComboBox denominatorBox;
    juce::TextButton addLaneButton { "+ Lane" };
    juce::TextButton addPointButton { "+ Point" };
    juce::TextButton removeLaneButton { "Delete Lane" };
    std::unique_ptr<TimelineCanvas> canvas;
    juce::String targetSignature;
    juce::String laneSignature;
    bool automationGestureActive = false;
    bool latchEngaged = false;
    bool transportWasPlaying = false;
    juce::int64 lastAutomationWriteSample = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TempoAutomationComponent)
};
}
