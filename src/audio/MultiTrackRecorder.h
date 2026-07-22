#pragma once

#include "AudioRecorder.h"
#include "core/RecordingTypes.h"

#include <atomic>
#include <memory>
#include <vector>

namespace triumph
{
class MultiTrackRecorder final
{
public:
    struct Target
    {
        juce::String trackId;
        int firstInputChannel = 0;
        int channelCount = 2;
        RecordingTapPoint tapPoint = RecordingTapPoint::hardwareInput;
        int recordOffsetSamples = 0;
    };

    struct StartContext
    {
        juce::File recordingsDirectory;
        juce::File journalDirectory;
        juce::File projectFile;
        juce::String projectId;
        juce::String sessionId;
        juce::String fileStem;
        double sampleRate = 0.0;
        juce::int64 timelineStartSamples = 0;
        int availableInputChannels = 0;
        int availableOutputChannels = 2;
        bool loopEnabled = false;
    };

    struct CaptureResult
    {
        Target target;
        int passNumber = 1;
        juce::int64 timelineStartSamples = 0;
        AudioRecorder::Result audio;
    };

    MultiTrackRecorder();
    ~MultiTrackRecorder();

    juce::Result start (std::vector<Target> targets,
                        StartContext context);
    void processSpan (const juce::AudioBuffer<float>& hardwareInput,
                      const juce::AudioBuffer<float>& programOutput,
                      int hardwareStartSample,
                      int programStartSample,
                      int numSamples,
                      int passNumber) noexcept;
    juce::Result service();
    std::vector<CaptureResult> stop();
    juce::Result checkpointJournals();

    bool isRecording() const noexcept;
    int getCurrentPass() const noexcept;
    juce::int64 getSamplesRecorded() const noexcept;
    int getDroppedBlockCount() const noexcept;
    juce::int64 getDroppedSampleCount() const noexcept;
    int getPassPreparationStarvationCount() const noexcept;

private:
    struct PassTarget
    {
        Target target;
        juce::File journalFile;
        std::unique_ptr<AudioRecorder> recorder;
    };

    struct Pass
    {
        int number = 1;
        juce::int64 timelineStartSamples = 0;
        std::vector<PassTarget> targets;
        std::atomic<bool> activated { false };
        bool recordingJournalPublished = false;
        bool finalised = false;
    };

    juce::Result preparePass (int passNumber, bool activateImmediately);
    Pass* passForAudioThread (int passNumber) noexcept;
    void finalisePass (Pass& pass);

    juce::TimeSliceThread writerThread { "Triumph multitrack writer service" };
    std::vector<Target> configuredTargets;
    StartContext activeContext;
    std::vector<std::unique_ptr<Pass>> passes;
    std::vector<CaptureResult> completedResults;
    std::atomic<Pass*> activePass { nullptr };
    std::atomic<Pass*> preparedPass { nullptr };
    std::atomic<bool> active { false };
    std::atomic<int> passPreparationStarvations { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiTrackRecorder)
};
}
