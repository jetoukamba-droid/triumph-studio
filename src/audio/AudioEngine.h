#pragma once

#include <JuceHeader.h>

#include "AudioTrack.h"
#include "core/TempoMap.h"
#include "core/TempoAutomation.h"
#include "core/ParameterSmoother.h"
#include "core/PluginGraph.h"
#include "core/PluginRack.h"
#include "core/SampleDelayLine.h"
#include "core/MixerGraph.h"
#include "core/MidiPerformance.h"
#include "core/RealtimeEngineState.h"
#include "core/FixedSampleClock.h"

#include <atomic>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace triumph
{
class AudioEngine final : private juce::AudioProcessorListener
{
public:
    struct FileDetails
    {
        double sampleRate = 0.0;
        int channels = 0;
        juce::int64 lengthInSamples = 0;

        bool isValid() const noexcept
        {
            return sampleRate > 0.0 && channels > 0 && lengthInSamples > 0;
        }
    };

    struct MidiNotePlayback
    {
        double startBeat = 0.0;
        double lengthBeats = 1.0;
        double startSeconds = 0.0;
        double endSeconds = 0.0;
        int noteNumber = 60;
        float velocity = 0.8f;
        int channel = 1;
        std::uint32_t noteId = 0;
        int performanceChannel = 1;
        struct Expression
        {
            enum class Type
            {
                pitch,
                pressure,
                timbre
            };
            double offsetBeats = 0.0;
            double seconds = 0.0;
            float value = 0.0f;
            Type type = Type::pitch;
        };
        std::vector<Expression> expression;
    };

    struct MidiControllerPlayback
    {
        double beat = 0.0;
        double seconds = 0.0;
        int channel = 1;
        int controller = 1;
        std::uint32_t value = 0;
        std::uint32_t noteId = 0;
    };

    struct MidiTrackPlayback
    {
        juce::String id;
        juce::String instrumentName;
        float gain = 0.75f;
        float pan = 0.0f;
        bool muted = false;
        bool solo = false;
        int manualLatencyOffsetSamples = 0;
        std::vector<MidiNotePlayback> notes;
        std::vector<MidiControllerPlayback> controllers;
    };

    struct DelayTrack
    {
        juce::String id;
        int pluginLatencySamples = 0;
        int manualLatencyOffsetSamples = 0;
        bool pluginBypassed = false;
        bool frozen = false;
        bool pluginAvailable = true;
        bool monitored = false;
    };

    struct MixerSend
    {
        juce::String id;
        juce::String destinationId;
        float gain = 0.0f;
        bool enabled = true;
        bool preFader = false;
        bool sidechain = false;
    };

    struct MixerChannel
    {
        juce::String id;
        mixer::ChannelKind kind = mixer::ChannelKind::track;
        float gain = 0.85f;
        float pan = 0.0f;
        bool muted = false;
        bool solo = false;
        juce::String outputDestinationId { mixer::masterId };
        std::vector<MixerSend> sends;
        int inputChannels = 2;
        int outputChannels = 2;
        int sidechainChannels = 0;
        bool explicitBusLayout = false;
        int pluginLatencySamples = 0;
    };

    struct MonitorPath
    {
        juce::String id;
        mixer::MonitorRole role = mixer::MonitorRole::controlRoom;
        juce::String sourceId { mixer::masterId };
        int outputStartChannel = 0;
        int outputChannelCount = 2;
        float gain = 1.0f;
        bool muted = false;
        bool dimmed = false;
    };

    struct SyncConfiguration
    {
        midi::SyncSource source = midi::SyncSource::internal;
        bool sendMidiClock = false;
        bool followExternalStartStop = true;
        int mtcFramesPerSecond = 30;
        int jitterToleranceSamples = 256;
    };

    struct MixerMeter
    {
        juce::String id;
        float left = 0.0f;
        float right = 0.0f;
        float sidechainReduction = 0.0f;
    };

    struct AutomationPlayback
    {
        juce::String targetId;
        juce::String parameterId;
        bool enabled = true;
        std::vector<automation::Point> points;
    };

    struct PluginParameterDescriptor
    {
        juce::String stableId;
        juce::String name;
        juce::String label;
        float defaultValue = 0.0f;
        float currentValue = 0.0f;
        int stepCount = 0;
        bool automatable = false;
        bool discrete = false;
        bool orientationInverted = false;
    };

    struct InsertPluginLoad
    {
        juce::String slotId;
        juce::String ownerId;
        int order = 0;
        juce::PluginDescription description;
        juce::String descriptionXml;
        juce::String stateBase64;
        bool bypassed = false;
    };

    AudioEngine();
    ~AudioEngine();

    FileDetails inspectAudioFile (const juce::File& audioFile);
    AudioTrack* addTrack (const juce::String& trackId, const juce::File& audioFile);
    bool removeTrack (const juce::String& trackId);
    void clearTracks();

    int getTrackCount() const noexcept;
    AudioTrack* getTrack (int index) noexcept;
    const AudioTrack* getTrack (int index) const noexcept;
    AudioTrack* findTrack (const juce::String& trackId) noexcept;
    const AudioTrack* findTrack (const juce::String& trackId) const noexcept;
    void configureTrackClips (const juce::String& trackId,
                              std::vector<AudioTrack::ClipPlayback> clips,
                              double timelineSampleRate);
    void configureMidiTracks (std::vector<MidiTrackPlayback> newTracks,
                              std::vector<tempo::Point> newTempoPoints);
    void configureMusicalTimeline (
        std::vector<tempo::Point> newTempoPoints,
        std::vector<automation::Signature> newSignatures,
        bool metronomeEnabled, float metronomeGain);
    void configureDelayCompensation (std::vector<DelayTrack> newTracks,
                                     bool lowLatencyMonitoring);
    bool configureMixer (std::vector<MixerChannel> channels);
    bool configureMonitorPaths (std::vector<MonitorPath> paths);
    void configureSync (SyncConfiguration configuration);
    midi::SyncStatus getSyncStatus() const noexcept;
    void configureAutomation (std::vector<AutomationPlayback> lanes);
    void setPlaybackLoop (juce::int64 startSamples,
                          juce::int64 endSamples,
                          bool enabled) noexcept;
    juce::String getMixerGraphError() const;
    std::vector<MixerMeter> getMixerMeters() const;
    using RealtimeStatus = realtime::TelemetrySnapshot;
    RealtimeStatus getRealtimeStatus() const noexcept;
    struct PluginRuntimeStatus
    {
        std::uint64_t lastProcessNanoseconds = 0;
        std::uint64_t maximumProcessNanoseconds = 0;
        std::uint64_t deadlineMisses = 0;
        std::uint64_t containedExceptions = 0;
        std::uint64_t instrumentContainedExceptions = 0;
    };
    PluginRuntimeStatus getPluginRuntimeStatus() const noexcept;
    void beginRenderStateUpdate();
    void endRenderStateUpdate();
    int pullSpectrumSamples (float* destination, int maximumSamples) noexcept;
    int getGraphLatencySamples() const noexcept;
    bool isLowLatencyMonitoringEnabled() const noexcept;
    void loadInstrumentPlugin (juce::String trackId,
                               juce::PluginDescription description,
                               juce::String stateBase64,
                               std::function<void (bool, juce::String)> completion);
    void createOfflineInstrumentInstance (
        juce::PluginDescription description,
        juce::String stateBase64,
        double sampleRate,
        int blockSize,
        std::function<void (std::unique_ptr<juce::AudioPluginInstance>,
                            juce::String)> completion);
    void unloadInstrumentPlugin();
    void loadInsertPlugin (
        InsertPluginLoad request,
        std::function<void (bool, juce::String, int)> completion);
    void unloadInsertPlugin (const juce::String& slotId);
    std::vector<juce::String> getLoadedInsertPluginSlotIds() const;
    juce::String getInsertPluginDescriptionXml (
        const juce::String& slotId) const;
    void setInsertPluginBypassed (const juce::String& slotId,
                                  bool shouldBeBypassed);
    juce::String captureInsertPluginStateBase64 (
        const juce::String& slotId);
    std::vector<PluginParameterDescriptor> getInsertPluginParameters (
        const juce::String& slotId);
    struct InsertPluginParameterInventory
    {
        juce::String slotId;
        std::vector<PluginParameterDescriptor> parameters;
    };
    std::vector<InsertPluginParameterInventory>
        getInsertPluginParameterInventory();
    bool setInsertPluginParameterValue (const juce::String& slotId,
                                        const juce::String& stableId,
                                        float normalisedValue);
    juce::AudioProcessorEditor* getInsertPluginEditor (
        const juce::String& slotId);
    struct InsertPluginLatencyChange
    {
        juce::String slotId;
        int latencySamples = 0;
    };
    std::vector<InsertPluginLatencyChange>
        consumeInsertPluginLatencyChanges();
    std::vector<juce::String> consumeContainedInsertPluginSlots();
    bool hasInstrumentPlugin() const noexcept;
    juce::String getInstrumentPluginTrackId() const;
    juce::String getInstrumentPluginName() const;
    juce::String captureInstrumentPluginStateBase64();
    juce::Result applyInstrumentPluginStateBase64 (
        const juce::String& stateBase64);
    std::vector<PluginParameterDescriptor> getInstrumentPluginParameters();
    bool setInstrumentPluginParameterValue (const juce::String& stableId,
                                            float normalisedValue);
    int getInstrumentPluginLatencySamples() const;
    bool consumeInstrumentPluginLatencyChange (int& latencySamples) noexcept;
    void setInstrumentPluginBypassed (bool shouldBeBypassed) noexcept;
    juce::AudioProcessorEditor* getInstrumentPluginEditor();
    void setProjectTimeline (double timelineSampleRate,
                             juce::int64 lengthInTimelineSamples) noexcept;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate);
    void releaseResources();
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill);

    void play();
    void pause();
    void stopAndRewind();
    void setPositionSeconds (double seconds);
    void setRecordingTransportActive (bool active) noexcept;
    void setInputMonitoring (bool enabled) noexcept;
    bool isInputMonitoring() const noexcept;
    void setControlRoomState (float gain, bool dimmed, bool muted) noexcept;
    void setInputChannelCount (int channels) noexcept;
    void setLiveMidiNote (int channel, int noteNumber, float velocity) noexcept;
    void clearLiveMidiNotes() noexcept;
    void pushLiveMidiMessage (const juce::MidiMessage& message);

    bool isPlaying() const;
    double getPositionSeconds() const;
    double getProjectLengthSeconds() const;

    void setMasterGain (float newGain) noexcept;
    float getMasterGain() const noexcept;
    void setMasterMuted (bool shouldBeMuted) noexcept;
    bool isMasterMuted() const noexcept;

private:
    class HostedPlayHead final : public juce::AudioPlayHead
    {
    public:
        void update (juce::int64 samples, double seconds, double beat,
                     double bpm, bool isPlaying) noexcept
        {
            samplePosition = samples;
            timeSeconds = seconds;
            ppqPosition = beat;
            tempoBpm = bpm;
            playingNow = isPlaying;
        }

        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo result;
            result.setTimeInSamples (samplePosition);
            result.setTimeInSeconds (timeSeconds);
            result.setPpqPosition (ppqPosition);
            result.setPpqPositionOfLastBarStart (
                std::floor (ppqPosition / 4.0) * 4.0);
            result.setBpm (tempoBpm);
            result.setIsPlaying (playingNow);
            result.setIsRecording (false);
            result.setIsLooping (false);
            return result;
        }

    private:
        juce::int64 samplePosition = 0;
        double timeSeconds = 0.0;
        double ppqPosition = 0.0;
        double tempoBpm = 120.0;
        bool playingNow = false;
    };

    void prepareInstrumentPlugin (juce::AudioPluginInstance& plugin,
                                  double sampleRate,
                                  int blockSize);
    struct MeterValues
    {
        std::atomic<float> left { 0.0f };
        std::atomic<float> right { 0.0f };
        std::atomic<float> sidechainReduction { 0.0f };
    };

    struct MeterPublication
    {
        juce::String id;
        std::shared_ptr<MeterValues> values;
    };

    struct PreparedDelay
    {
        juce::String id;
        int delaySamples = 0;
    };

    struct TrackDelayState
    {
        juce::String id;
        SampleDelayLine line;
        int delaySamples = 0;
        juce::int64 tailSamplesRemaining = 0;
    };

    struct MixerRuntime
    {
        MixerChannel settings;
        juce::AudioBuffer<float> preFader;
        juce::AudioBuffer<float> postFader;
        juce::AudioBuffer<float> sidechain;
        float previousLeftGain = 0.0f;
        float previousRightGain = 0.0f;
        float peakLeft = 0.0f;
        float peakRight = 0.0f;
        float sidechainReduction = 0.0f;
        std::vector<automation::Point> gainAutomation;
        std::vector<automation::Point> panAutomation;
        std::shared_ptr<MeterValues> meter;
    };
    struct MixerRuntimeRoute
    {
        std::size_t sourceIndex = 0;
        std::size_t destinationIndex = 0;
        bool destinationIsMaster = false;
        mixer::RouteKind kind = mixer::RouteKind::send;
        float gain = 1.0f;
        bool preFader = false;
        bool enabled = true;
        int delaySamples = 0;
        SampleDelayLine delayLine;
        juce::AudioBuffer<float> gainBuffer;
        juce::int64 tailSamplesRemaining = 0;
    };

    struct PluginAutomationRuntime
    {
        juce::AudioProcessorParameter* parameter = nullptr;
        std::vector<automation::Point> points;
        float lastValue = -1.0f;
    };

    struct PluginParameterEvent
    {
        juce::AudioProcessorParameter* parameter = nullptr;
        std::uint32_t sampleOffset = 0;
        float value = 0.0f;
    };

    struct InsertPluginRuntime
    {
        juce::String slotId;
        juce::String ownerId;
        int order = 0;
        juce::AudioPluginInstance* instance = nullptr;
        bool bypassed = false;
        std::vector<PluginAutomationRuntime> automation;
        juce::AudioBuffer<float> dryBuffer;
    };

    struct RenderSnapshot
    {
        std::uint64_t generation = 0;
        int blockSize = 0;
        double sampleRate = 0.0;
        std::vector<std::shared_ptr<AudioTrack>> tracks;
        std::vector<MidiTrackPlayback> midiTracks;
        std::vector<tempo::Point> tempoPoints;
        std::vector<automation::Signature> timeSignatures;
        std::vector<automation::Point> masterGainAutomation;
        std::vector<automation::Point> masterPanAutomation;
        std::vector<TrackDelayState> trackDelayStates;
        std::vector<MixerRuntime> mixerRuntime;
        std::vector<MixerRuntimeRoute> mixerRuntimeRoutes;
        mixer::Plan mixerPlan;
        mixer::MonitorPlan monitorPlan;
        juce::AudioBuffer<float> scratchBuffer;
        juce::AudioBuffer<float> inputMonitorBuffer;
        juce::AudioBuffer<float> playbackMixBuffer;
        juce::AudioPluginInstance* instrumentPlugin = nullptr;
        juce::String instrumentPluginTrackId;
        juce::AudioBuffer<float> pluginAudioBuffer;
        juce::MidiBuffer pluginMidiBuffer;
        juce::MidiBuffer pluginProcessMidiBuffer;
        juce::MidiBuffer insertMidiBuffer;
        juce::MidiBuffer insertProcessMidiBuffer;
        std::vector<PluginAutomationRuntime> pluginAutomation;
        std::vector<InsertPluginRuntime> insertPlugins;
        std::array<PluginParameterEvent, 256> pluginParameterEvents {};
        ParameterSmoother pluginOutputMixSmoother;
        ParameterSmoother masterGainSmoother;
        ParameterSmoother controlRoomGainSmoother;
        float masterGainTarget = 0.9f;
        bool masterMutedTarget = false;
        double metronomePhase = 0.0;
        double metronomeFrequency = 1200.0;
        float metronomeEnvelope = 0.0f;
        std::uint64_t appliedRuntimeReset = 0;
    };

    void applySmoothedMasterGain (RenderSnapshot& state,
                                  juce::AudioBuffer<float>& buffer,
                                  int startSample,
                                  int numSamples,
                                  juce::int64 blockStartSample = 0,
                                  double timelineSamplesPerDeviceSample = 1.0) noexcept;
    void addSmoothedInputMonitor (RenderSnapshot& state,
                                  juce::AudioBuffer<float>& buffer,
                                  int startSample,
                                  int numSamples) noexcept;
    void renderMetronome (RenderSnapshot& state,
                          juce::AudioBuffer<float>& master,
                          int numSamples,
                          double blockStartSeconds) noexcept;
    std::unique_ptr<RenderSnapshot> buildRenderSnapshot();
    void markRenderStateDirty();
    void publishRenderSnapshot();
    void reclaimRetiredResources();
    void requestRuntimeReset() noexcept;
    void rebuildDelayCompensationPlan();
    void drainParameterCommands (RenderSnapshot& state) noexcept;
    void beginPluginControl() noexcept;
    void endPluginControl() noexcept;
    void audioProcessorParameterChanged (juce::AudioProcessor*, int,
                                         float) override;
    void audioProcessorChanged (
        juce::AudioProcessor*,
        const juce::AudioProcessorListener::ChangeDetails&) override;
    std::size_t collectPluginParameterEvents (
        std::vector<PluginAutomationRuntime>& lanes,
        juce::int64 blockStartSample,
        int numSamples,
        std::array<PluginParameterEvent, 256>& events) noexcept;
    void applyPluginParameterEvents (
        std::vector<PluginAutomationRuntime>& lanes,
        const std::array<PluginParameterEvent, 256>& events,
        std::size_t count,
        std::size_t& nextEvent,
        std::uint32_t sampleOffset) noexcept;
    static void copyMidiRange (const juce::MidiBuffer& source,
                               juce::MidiBuffer& destination,
                               int startSample,
                               int numSamples);
    void processPluginBlockWithAutomation (
        juce::AudioPluginInstance& plugin,
        juce::AudioBuffer<float>& audio,
        juce::MidiBuffer& midi,
        juce::MidiBuffer& processMidi,
        std::vector<PluginAutomationRuntime>& automation,
        std::array<PluginParameterEvent, 256>& events,
        int numSamples,
        juce::int64 blockStartSample);

    TrackDelayState* findTrackDelayState (RenderSnapshot& state,
                                         const juce::String& trackId) noexcept;
    void mixTrackWithCompensation (RenderSnapshot& state,
                                   juce::AudioBuffer<float>& destination,
                                   int destinationStartSample,
                                   const juce::AudioBuffer<float>& source,
                                   const juce::String& trackId,
                                   int numSamples) noexcept;
    void clearTrackDelayBuffers (RenderSnapshot& state) noexcept;
    bool hasDelayTailPending (const RenderSnapshot& state) const noexcept;
    MixerRuntime* findMixerRuntime (RenderSnapshot& state,
                                    const juce::String& channelId) noexcept;
    void clearMixerBuffers (RenderSnapshot& state) noexcept;
    void mixSourceIntoMixer (RenderSnapshot& state,
                             const juce::String& trackId,
                             const juce::AudioBuffer<float>& source,
                             int numSamples) noexcept;
    void processMixerGraph (RenderSnapshot& state,
                            juce::AudioBuffer<float>& master,
                            int numSamples,
                            juce::int64 blockStartSample,
                            double timelineSamplesPerDeviceSample,
                            bool pluginsReady) noexcept;
    void processInsertPlugins (RenderSnapshot& state,
                               const juce::String& ownerId,
                               juce::AudioBuffer<float>& buffer,
                               juce::MidiBuffer& midi,
                               int numSamples,
                               juce::int64 blockStartSample) noexcept;
    void pushSpectrumSamples (const juce::AudioBuffer<float>& buffer,
                              int startSample,
                              int numSamples) noexcept;

    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache { 64 };
    juce::TimeSliceThread readAheadThread { "Triumph Studio audio read-ahead" };
    mutable juce::CriticalSection controlLock;
    std::vector<std::shared_ptr<AudioTrack>> tracks;
    std::vector<MidiTrackPlayback> midiTracks;
    std::vector<tempo::Point> tempoPoints { { 0.0, 120.0 } };
    std::vector<automation::Signature> timeSignatures { { 0.0, 4, 4 } };
    std::vector<PreparedDelay> delayConfiguration;
    std::vector<DelayTrack> delayTrackConfiguration;
    std::vector<MixerChannel> mixerConfiguration;
    std::vector<AutomationPlayback> automationConfiguration;
    std::vector<MeterPublication> meterPublications;
    mixer::Plan mixerPlanConfiguration;
    mixer::MonitorPlan monitorPlanConfiguration;
    plugins::Plan delayPlanConfiguration;
    juce::String mixerGraphError;
    juce::AudioPluginFormatManager pluginFormatManager;
    struct HostedInsertPlugin
    {
        juce::String slotId;
        juce::String ownerId;
        int order = 0;
        juce::String descriptionXml;
        std::unique_ptr<juce::AudioPluginInstance> instance;
        bool bypassed = false;
        int latencySamples = 0;
    };
    std::unique_ptr<juce::AudioPluginInstance> instrumentPlugin;
    std::vector<HostedInsertPlugin> insertPlugins;
    std::vector<std::unique_ptr<juce::AudioPluginInstance>> retiredPlugins;
    juce::String instrumentPluginTrackId;
    juce::String instrumentPluginName;
    juce::MidiMessageCollector liveMidiCollector;
    HostedPlayHead instrumentPlayHead;
    realtime::SnapshotExchange<RenderSnapshot> renderStates;
    realtime::ParameterQueue<256> parameterCommands;
    realtime::Telemetry realtimeTelemetry;
    mutable juce::SpinLock syncLock;
    mutable midi::SyncFollower syncFollower;
    SyncConfiguration syncConfiguration;
    std::atomic<midi::SyncSource> activeSyncSource {
        midi::SyncSource::internal
    };
    std::atomic<bool> externalSyncLocked { true };
    std::atomic<double> externalSyncTempoBpm { 120.0 };
    std::atomic<juce::int64> lastExternalSyncSample { -1 };

    int preparedBlockSize = 0;
    double preparedSampleRate = 0.0;
    int renderUpdateDepth = 0;
    bool renderStateDirty = true;
    std::uint64_t nextRenderGeneration = 1;
    std::uint64_t lastAudioRenderGeneration = 0;
    std::atomic<float> masterGain { 0.9f };
    std::atomic<bool> masterMuted { false };
    std::atomic<bool> metronomeEnabled { false };
    std::atomic<float> metronomeGain { 0.5f };
    std::atomic<bool> playing { false };
    std::atomic<bool> recordingTransportActive { false };
    std::atomic<bool> inputMonitoring { false };
    std::atomic<float> controlRoomGain { 1.0f };
    std::atomic<bool> controlRoomDimmed { false };
    std::atomic<bool> controlRoomMuted { false };
    std::atomic<int> inputChannelCount { 0 };
    std::array<std::atomic<float>, 16 * 128> liveMidiNotes {};
    std::array<double, 128> liveMidiPhases {};
    std::atomic<int> liveMidiNoteCount { 0 };
    std::atomic<bool> instrumentPluginLoaded { false };
    std::atomic<juce::AudioProcessor*> instrumentPluginProcessor { nullptr };
    std::atomic<bool> instrumentPluginBypassed { false };
    std::atomic<float> instrumentPluginOutputMix { 1.0f };
    std::atomic<int> reportedInstrumentPluginLatencySamples { 0 };
    std::atomic<bool> instrumentPluginLatencyChangePending { false };
    std::atomic<bool> liveMidiPending { false };
    std::atomic<bool> pluginNeedsAllNotesOff { false };
    std::atomic<bool> pluginControlBusy { false };
    std::atomic<std::uint32_t> pluginAudioUsers { 0 };
    std::shared_ptr<std::atomic<bool>> pluginHostAlive {
        std::make_shared<std::atomic<bool>> (true)
    };
    std::atomic<std::uint64_t> pluginLoadRequest { 0 };
    std::atomic<std::uint64_t> insertPluginLoadRequest { 0 };
    struct InsertLatencyNotice
    {
        std::atomic<int> state { 0 };
        std::atomic<juce::AudioProcessor*> processor { nullptr };
        std::atomic<int> samples { 0 };
    };
    std::array<InsertLatencyNotice, 32> insertLatencyNotices {};
    std::array<InsertLatencyNotice, 32> insertFaultNotices {};
    std::atomic<juce::int64> pluginTailSamplesRemaining { 0 };
    std::atomic<juce::int64> insertPluginTailSamplesRemaining { 0 };
    std::atomic<std::uint64_t> pluginLastProcessNanoseconds { 0 };
    std::atomic<std::uint64_t> pluginMaximumProcessNanoseconds { 0 };
    std::atomic<std::uint64_t> pluginDeadlineMisses { 0 };
    std::atomic<std::uint64_t> pluginContainedExceptions { 0 };
    std::atomic<std::uint64_t> instrumentContainedExceptions { 0 };
    std::atomic<int> graphLatencySamples { 0 };
    std::atomic<bool> lowLatencyMonitoringEnabled { false };
    std::atomic<transport::ClockUnits> timelinePositionUnits { 0 };
    std::atomic<transport::ClockUnits> projectLengthUnits { 0 };
    std::atomic<transport::ClockUnits> playbackLoopStartUnits { 0 };
    std::atomic<transport::ClockUnits> playbackLoopEndUnits { 0 };
    std::atomic<bool> playbackLoopEnabled { false };
    std::atomic<double> projectTimelineSampleRate { 48000.0 };
    std::atomic<std::uint64_t> runtimeResetGeneration { 1 };
    static constexpr int spectrumRingSize = 16384;
    std::array<float, spectrumRingSize> spectrumRing {};
    juce::AbstractFifo spectrumFifo { spectrumRingSize };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEngine)
};
}
