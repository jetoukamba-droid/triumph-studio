#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_graphics/juce_graphics.h>

#include "core/RecordingTypes.h"

#include <functional>
#include <cstdint>
#include <vector>

namespace triumph
{
struct MediaSourceState
{
    juce::String id;
    juce::File file;
    double sampleRate = 0.0;
    int channels = 0;
    juce::int64 lengthInSamples = 0;
    juce::String assetFingerprint;
};

struct ProjectRepairRecordState
{
    juce::String id;
    juce::String severity { "warning" };
    juce::String area;
    juce::String subjectId;
    juce::String message;
};

struct PersistentUndoEntryState
{
    juce::String description;
    juce::String stateXml;
};

struct UndoManifestState
{
    bool hadUndo = false;
    bool hadRedo = false;
    juce::String undoDescription;
    juce::String redoDescription;
    std::vector<PersistentUndoEntryState> undoHistory;
    std::vector<PersistentUndoEntryState> redoHistory;
};

struct WarpMarkerState
{
    juce::String id;
    juce::int64 timelineOffsetSamples = 0;
    juce::int64 sourceOffsetSamples = 0;
};

struct TransientMarkerState
{
    juce::String id;
    juce::int64 timelineOffsetSamples = 0;
    juce::int64 sourceOffsetSamples = 0;
    float strength = 0.0f;
};

struct ClipVariantState
{
    juce::String id;
    juce::String name;
    juce::String sourceId;
    juce::int64 sourceOffsetSamples = 0;
    juce::int64 lengthInSamples = 0;
    juce::int64 fadeInSamples = 0;
    juce::int64 fadeOutSamples = 0;
    float gain = 1.0f;
    double timeStretchRatio = 1.0;
    juce::String timeStretchProvider { "builtin-resample" };
    bool reversed = false;
    std::vector<WarpMarkerState> warpMarkers;
    std::vector<TransientMarkerState> transientMarkers;
};

struct AudioClipState
{
    juce::String id;
    juce::String name;
    juce::String sourceId;
    juce::int64 timelineStartSamples = 0;
    juce::int64 sourceOffsetSamples = 0;
    juce::int64 lengthInSamples = 0;
    juce::int64 fadeInSamples = 0;
    juce::int64 fadeOutSamples = 0;
    float gain = 1.0f;
    juce::Colour colour { juce::Colours::transparentBlack };
    bool muted = false;
    bool locked = false;
    juce::String takeGroupId;
    int takeLaneIndex = 0;
    bool activeTake = true;
    double timeStretchRatio = 1.0;
    juce::String timeStretchProvider { "builtin-resample" };
    bool reversed = false;
    std::vector<WarpMarkerState> warpMarkers;
    std::vector<TransientMarkerState> transientMarkers;
    std::vector<ClipVariantState> variants;
};

struct CompRegionState
{
    juce::String id;
    juce::String takeGroupId;
    int takeLaneIndex = 0;
    juce::int64 timelineStartSamples = 0;
    juce::int64 lengthInSamples = 0;
    juce::int64 crossfadeSamples = 480;
};

struct AudioPlaybackSegmentState
{
    juce::String id;
    juce::String sourceId;
    juce::int64 timelineStartSamples = 0;
    juce::int64 sourceOffsetSamples = 0;
    juce::int64 lengthInSamples = 0;
    float gain = 1.0f;
    juce::int64 fadeInSamples = 0;
    juce::int64 fadeOutSamples = 0;
    double playbackRate = 1.0;
    bool reversed = false;
    juce::int64 fadeAnchorStartSamples = 0;
    juce::int64 fadeAnchorLengthSamples = 0;
};

struct MidiNoteState
{
    juce::String id;
    double startBeat = 0.0;
    double lengthBeats = 1.0;
    int noteNumber = 60;
    float velocity = 0.8f;
    int channel = 1;
    std::uint32_t noteId = 0;
    struct ExpressionPoint
    {
        enum class Type
        {
            pitch,
            pressure,
            timbre
        };
        juce::String id;
        double offsetBeats = 0.0;
        float value = 0.0f;
        Type type = Type::pitch;
    };
    std::vector<ExpressionPoint> expression;
};

struct MidiControllerEventState
{
    juce::String id;
    double beat = 0.0;
    int channel = 1;
    int controller = 1;
    std::uint32_t value = 0;
    std::uint32_t noteId = 0;
};

struct MidiClipState
{
    juce::String id;
    double startBeat = 0.0;
    double lengthBeats = 16.0;
    std::vector<MidiNoteState> notes;
    std::vector<MidiControllerEventState> controllers;
};

enum class TempoCurve
{
    step,
    linear
};

struct TempoPointState
{
    juce::String id;
    double beat = 0.0;
    double bpm = 120.0;
    TempoCurve curve = TempoCurve::step;
};

struct TimeSignaturePointState
{
    juce::String id;
    double beat = 0.0;
    int numerator = 4;
    int denominator = 4;
};

struct ProjectMarkerState
{
    juce::String id;
    double beat = 0.0;
    juce::String name { "Marker" };
    juce::Colour colour { 0xffdf6b34 };
};

enum class AutomationMode
{
    read,
    touch,
    latch,
    write
};

enum class AutomationCurve
{
    hold,
    linear,
    smooth
};

struct AutomationPointState
{
    juce::String id;
    juce::int64 samplePosition = 0;
    float value = 0.0f;
    AutomationCurve curve = AutomationCurve::linear;
};

struct AutomationLaneState
{
    juce::String id;
    juce::String targetId { "master" };
    juce::String parameterId { "gain" };
    AutomationMode mode = AutomationMode::read;
    bool enabled = true;
    std::vector<AutomationPointState> points;
};

enum class MixerChannelKind
{
    returnChannel,
    bus
};

struct MixerSendState
{
    juce::String id;
    juce::String destinationId;
    float gain = 0.0f;
    bool enabled = true;
    bool preFader = false;
    bool sidechain = false;
};

struct MixerBusLayoutState
{
    int inputChannels = 2;
    int outputChannels = 2;
    int sidechainChannels = 0;
    bool explicitLayout = false;
};

enum class PluginSlotRole
{
    instrument,
    insertEffect
};

enum class PluginIsolationMode
{
    workerProcess,
    trustedInProcess
};

struct PluginSlotState
{
    juce::String id;
    juce::String stablePluginId;
    juce::String name;
    juce::String descriptionXml;
    juce::String stateBase64;
    PluginSlotRole role = PluginSlotRole::insertEffect;
    PluginIsolationMode isolation = PluginIsolationMode::workerProcess;
    int order = 0;
    MixerBusLayoutState layout;
    bool bypassed = false;
    bool available = true;
    int latencySamples = 0;
    int restartCount = 0;
};

struct MixerChannelState
{
    juce::String id;
    juce::String name;
    MixerChannelKind kind = MixerChannelKind::bus;
    float gain = 0.85f;
    float pan = 0.0f;
    bool muted = false;
    bool solo = false;
    juce::String outputDestinationId { "master" };
    std::vector<MixerSendState> sends;
    MixerBusLayoutState layout;
    std::vector<PluginSlotState> pluginSlots;
};

struct TrackState
{
    juce::String id;
    juce::String name;
    juce::Colour colour { 0xff2869d8 };
    float gain = 0.85f;
    float pan = 0.0f;
    bool muted = false;
    bool solo = false;
    bool recordArmed = false;
    int inputStartChannel = 0;
    int inputChannelCount = 2;
    RecordingTapPoint recordingTapPoint = RecordingTapPoint::hardwareInput;
    int recordOffsetSamples = 0;
    bool isInstrument = false;
    juce::String pluginName;
    juce::String pluginDescriptionXml;
    juce::String pluginStateBase64;
    bool pluginBypassed = false;
    int pluginLatencySamples = 0;
    bool pluginAvailable = true;
    int manualLatencyOffsetSamples = 0;
    juce::String outputDestinationId { "master" };
    std::vector<MixerSendState> sends;
    MixerBusLayoutState layout;
    std::vector<PluginSlotState> pluginSlots;
    bool frozen = false;
    juce::String freezeSourceId;
    std::vector<AudioClipState> clips;
    std::vector<CompRegionState> compRegions;
    std::vector<MidiClipState> midiClips;
};

struct ProducerSettingsState
{
    int rootPitchClass = 0;
    juce::String scale { "major" };
    juce::String style { "balanced" };
    int bars = 4;
    std::uint32_t variation = 1;
};

struct TrackMixUpdate
{
    juce::String trackId;
    float gain = 0.75f;
    float pan = 0.0f;
};

enum class RecordingMode
{
    normal,
    punch,
    loop
};

struct RecordingSettingsState
{
    RecordingMode mode = RecordingMode::normal;
    int preRollBars = 0;
    juce::int64 punchStartSamples = 0;
    juce::int64 punchEndSamples = 0;
    juce::int64 loopStartSamples = 0;
    juce::int64 loopEndSamples = 0;
    float controlRoomGain = 1.0f;
    bool controlRoomDimmed = false;
    bool controlRoomMuted = false;
};

enum class InputMonitorMode
{
    off,
    whileArmed,
    always
};

enum class SyncSourceState
{
    internal,
    midiClock,
    midiTimeCode,
    link
};

struct SyncSettingsState
{
    SyncSourceState source = SyncSourceState::internal;
    bool sendMidiClock = false;
    bool followExternalStartStop = true;
    int mtcFramesPerSecond = 30;
    int jitterToleranceSamples = 256;
};

enum class MonitorPathRole
{
    controlRoom,
    cue,
    listen,
    talkback
};

struct MonitorPathState
{
    juce::String id;
    juce::String name;
    MonitorPathRole role = MonitorPathRole::controlRoom;
    juce::String sourceId { "master" };
    int outputStartChannel = 0;
    int outputChannelCount = 2;
    float gain = 1.0f;
    bool muted = false;
    bool dimmed = false;
};

struct ProjectState
{
    juce::String projectId;
    juce::String name { "Untitled Project" };
    float masterGain = 0.9f;
    bool masterMuted = false;
    double timelineSampleRate = 48000.0;
    int timeSignatureNumerator = 4;
    int timeSignatureDenominator = 4;
    bool metronomeEnabled = false;
    float metronomeGain = 0.5f;
    bool lowLatencyMonitoring = false;
    InputMonitorMode inputMonitorMode = InputMonitorMode::off;
    int manualRecordOffsetSamples = 0;
    RecordingSettingsState recordingSettings;
    SyncSettingsState syncSettings;
    std::vector<MonitorPathState> monitorPaths;
    std::vector<TempoPointState> tempoPoints { { {}, 0.0, 120.0,
                                                 TempoCurve::step } };
    std::vector<TimeSignaturePointState> timeSignatures {
        { {}, 0.0, 4, 4 }
    };
    std::vector<ProjectMarkerState> markers;
    std::vector<AutomationLaneState> automationLanes;
    std::vector<MediaSourceState> mediaSources;
    std::vector<TrackState> tracks;
    std::vector<MixerChannelState> mixerChannels;
    ProducerSettingsState producerSettings;
    std::vector<ProjectRepairRecordState> repairRecords;
    UndoManifestState undoManifest;
};

class ProjectModel final : private juce::ValueTree::Listener
{
public:
    using ChangeCallback = std::function<void()>;

    ProjectModel();
    ~ProjectModel() override;

    void setChangeCallback (ChangeCallback callback);
    void reset();

    juce::String getProjectId() const;

    juce::String getName() const;
    void setName (juce::String newName, bool undoable = true);

    float getMasterGain() const;
    void setMasterGain (float newGain, bool undoable = true);
    bool getMasterMuted() const;
    void setMasterMuted (bool shouldBeMuted, bool undoable = true);

    double getTimelineSampleRate() const;
    void setTimelineSampleRate (double newSampleRate, bool undoable = false);
    double getTempoBpm() const;
    void setTempoBpm (double bpm, bool undoable = true);
    std::vector<TempoPointState> getTempoPoints() const;
    juce::String addTempoPoint (double beat, double bpm,
                                TempoCurve curve = TempoCurve::step);
    bool updateTempoPoint (const juce::String& pointId, double beat,
                           double bpm, TempoCurve curve);
    bool removeTempoPoint (const juce::String& pointId);
    std::vector<TimeSignaturePointState> getTimeSignatures() const;
    juce::String addTimeSignature (double beat, int numerator, int denominator);
    bool updateTimeSignature (const juce::String& pointId, double beat,
                              int numerator, int denominator);
    bool removeTimeSignature (const juce::String& pointId);
    std::vector<ProjectMarkerState> getMarkers() const;
    juce::String addMarker (double beat, juce::String name = {});
    bool updateMarker (const juce::String& markerId, double beat,
                       juce::String name);
    bool removeMarker (const juce::String& markerId);
    bool getMetronomeEnabled() const;
    void setMetronomeEnabled (bool enabled, bool undoable = true);
    float getMetronomeGain() const;
    void setMetronomeGain (float gain, bool undoable = true);
    std::vector<AutomationLaneState> getAutomationLanes() const;
    AutomationLaneState getAutomationLane (const juce::String& laneId) const;
    juce::String addAutomationLane (juce::String targetId,
                                    juce::String parameterId,
                                    AutomationMode mode = AutomationMode::read);
    bool removeAutomationLane (const juce::String& laneId);
    void setAutomationLaneMode (const juce::String& laneId,
                                AutomationMode mode);
    void setAutomationLaneEnabled (const juce::String& laneId, bool enabled);
    juce::String addAutomationPoint (const juce::String& laneId,
                                     juce::int64 samplePosition, float value,
                                     AutomationCurve curve = AutomationCurve::linear,
                                     bool beginNewTransaction = true);
    bool updateAutomationPoint (const juce::String& laneId,
                                const juce::String& pointId,
                                juce::int64 samplePosition, float value,
                                AutomationCurve curve);
    bool removeAutomationPoint (const juce::String& laneId,
                                const juce::String& pointId);
    bool getLowLatencyMonitoring() const;
    void setLowLatencyMonitoring (bool enabled, bool undoable = true);
    InputMonitorMode getInputMonitorMode() const;
    void setInputMonitorMode (InputMonitorMode mode, bool undoable = true);
    int getManualRecordOffsetSamples() const;
    void setManualRecordOffsetSamples (int samples, bool undoable = true);
    RecordingSettingsState getRecordingSettings() const;
    void setRecordingSettings (const RecordingSettingsState& settings,
                               bool undoable = true);
    SyncSettingsState getSyncSettings() const;
    void setSyncSettings (const SyncSettingsState& settings,
                          bool undoable = true);
    std::vector<MonitorPathState> getMonitorPaths() const;
    void setMonitorPaths (std::vector<MonitorPathState> paths,
                          bool undoable = true);

    int getTrackCount() const;
    juce::String getTrackId (int index) const;
    bool hasTrack (const juce::String& trackId) const;
    TrackState getTrackState (const juce::String& trackId) const;
    AudioClipState getClipState (const juce::String& trackId,
                                 const juce::String& clipId) const;
    juce::int64 getProjectLengthInSamples() const;
    MediaSourceState getMediaSourceState (const juce::String& sourceId) const;
    int getMediaSourceCount() const;
    juce::String getMediaSourceId (int index) const;
    void setMediaSourceFile (const juce::String& sourceId,
                             const juce::File& newFile,
                             bool undoable = false);

    juce::String addAudioTrack (const juce::File& sourceFile,
                                double sourceSampleRate,
                                int sourceChannels,
                                juce::int64 sourceLengthInSamples);
    juce::String addEmptyAudioTrack (juce::String name = "Audio Track");
    juce::String addInstrumentTrack (juce::String name = "Triumph Instrument");
    juce::String addGeneratedMidiTrack (
        juce::String name,
        const std::vector<MidiNoteState>& notes,
        double lengthBeats,
        juce::String undoDescription);
    juce::String addMidiNote (const juce::String& trackId,
                              const juce::String& clipId,
                              double startBeat,
                              double lengthBeats,
                              int noteNumber,
                              float velocity = 0.8f);
    int addMidiNotes (const juce::String& trackId,
                      const juce::String& clipId,
                      const std::vector<MidiNoteState>& notes);
    bool removeMidiNote (const juce::String& trackId,
                         const juce::String& clipId,
                         const juce::String& noteId);
    bool quantizeMidiClip (const juce::String& trackId,
                           const juce::String& clipId,
                           double gridBeats);
    juce::String addMidiControllerEvent (
        const juce::String& trackId, const juce::String& clipId,
        const MidiControllerEventState& event);
    bool setMidiNoteExpression (
        const juce::String& trackId, const juce::String& clipId,
        const juce::String& noteId,
        std::vector<MidiNoteState::ExpressionPoint> expression);
    juce::String getFirstInstrumentTrackId() const;
    ProducerSettingsState getProducerSettings() const;
    void setProducerSettings (const ProducerSettingsState& settings,
                              bool undoable = false);
    int applyTrackMixUpdates (const std::vector<TrackMixUpdate>& updates,
                              juce::String undoDescription = "Producer Mix Balance");
    juce::String addRecordedClip (const juce::String& trackId,
                                  const juce::File& sourceFile,
                                  double sourceSampleRate,
                                  int sourceChannels,
                                  juce::int64 sourceLengthInSamples,
                                  juce::int64 timelineStartSamples);
    bool removeTrack (const juce::String& trackId);

    void setTrackName (const juce::String& trackId, juce::String newName);
    void setTrackGain (const juce::String& trackId, float newGain);
    void setTrackPan (const juce::String& trackId, float newPan);
    void setTrackMuted (const juce::String& trackId, bool shouldBeMuted);
    void setTrackSolo (const juce::String& trackId, bool shouldBeSolo);
    void setTrackRecordArmed (const juce::String& trackId, bool shouldBeArmed);
    void setTrackInputRoute (const juce::String& trackId,
                             int firstInputChannel,
                             int channelCount);
    void setTrackRecordingTapPoint (const juce::String& trackId,
                                    RecordingTapPoint tapPoint);
    void setTrackRecordOffset (const juce::String& trackId,
                               int offsetSamples);
    void setInstrumentPlugin (const juce::String& trackId,
                              juce::String pluginName,
                              juce::String descriptionXml,
                              juce::String stateBase64 = {});
    void setBuiltInInstrument (const juce::String& trackId,
                               juce::String instrumentName);
    void setInstrumentPluginState (const juce::String& trackId,
                                   juce::String stateBase64);
    void setInstrumentPluginBypassed (const juce::String& trackId,
                                      bool shouldBeBypassed);
    void setInstrumentPluginHealth (const juce::String& trackId,
                                    int latencySamples,
                                    bool isAvailable);
    std::vector<PluginSlotState> getPluginSlots (
        const juce::String& ownerId) const;
    juce::String addPluginSlot (const juce::String& ownerId,
                                PluginSlotState slot);
    bool updatePluginSlot (const juce::String& ownerId,
                           const PluginSlotState& slot);
    bool removePluginSlot (const juce::String& ownerId,
                           const juce::String& slotId);
    void setTrackManualLatencyOffset (const juce::String& trackId,
                                      int offsetSamples,
                                      bool beginNewTransaction = true);
    void setTrackOutputDestination (const juce::String& trackId,
                                    juce::String destinationId);
    bool freezeInstrumentTrack (const juce::String& trackId,
                                const juce::File& renderedFile,
                                double sampleRate,
                                int channels,
                                juce::int64 lengthInSamples);
    bool unfreezeInstrumentTrack (const juce::String& trackId);
    void clearInstrumentPlugin (const juce::String& trackId);
    juce::String getArmedTrackId() const;
    std::vector<juce::String> getArmedTrackIds() const;

    std::vector<MixerChannelState> getMixerChannels() const;
    MixerChannelState getMixerChannelState (const juce::String& channelId) const;
    juce::String addMixerChannel (MixerChannelKind kind,
                                  juce::String name = {});
    bool removeMixerChannel (const juce::String& channelId);
    void setMixerChannelName (const juce::String& channelId,
                              juce::String name);
    void setMixerChannelGain (const juce::String& channelId, float gain);
    void setMixerChannelPan (const juce::String& channelId, float pan);
    void setMixerChannelMuted (const juce::String& channelId, bool muted);
    void setMixerChannelSolo (const juce::String& channelId, bool solo);
    void setMixerChannelOutputDestination (const juce::String& channelId,
                                           juce::String destinationId);
    juce::String addMixerSend (const juce::String& ownerId,
                               const juce::String& destinationId,
                               bool sidechain = false);
    bool removeMixerSend (const juce::String& ownerId,
                          const juce::String& sendId);
    void setMixerSendDestination (const juce::String& ownerId,
                                  const juce::String& sendId,
                                  juce::String destinationId);
    void setMixerSendGain (const juce::String& ownerId,
                           const juce::String& sendId,
                           float gain);
    void setMixerSendEnabled (const juce::String& ownerId,
                              const juce::String& sendId,
                              bool enabled);
    void setMixerSendPreFader (const juce::String& ownerId,
                               const juce::String& sendId,
                               bool preFader);
    void setMixerSendSidechain (const juce::String& ownerId,
                                const juce::String& sendId,
                                bool sidechain);

    bool moveClip (const juce::String& trackId,
                   const juce::String& clipId,
                   juce::int64 newTimelineStartSamples);
    bool trimClipStart (const juce::String& trackId,
                        const juce::String& clipId,
                        juce::int64 newTimelineStartSamples);
    bool trimClipEnd (const juce::String& trackId,
                      const juce::String& clipId,
                      juce::int64 newTimelineEndSamples);
    bool stretchClipToEnd (const juce::String& trackId,
                           const juce::String& clipId,
                           juce::int64 newTimelineEndSamples);
    bool setClipReversed (const juce::String& trackId,
                          const juce::String& clipId,
                          bool reversed);
    bool setClipGain (const juce::String& trackId,
                      const juce::String& clipId,
                      float gain,
                      juce::String undoDescription = "Normalize Clip");
    bool setClipFades (const juce::String& trackId,
                       const juce::String& clipId,
                       juce::int64 fadeInSamples,
                       juce::int64 fadeOutSamples,
                       juce::String undoDescription = "Adjust Clip Fades");
    bool createClipCrossfade (const juce::String& trackId,
                              const juce::String& leftClipId);
    bool setClipName (const juce::String& trackId,
                      const juce::String& clipId,
                      juce::String name);
    bool setClipMuted (const juce::String& trackId,
                       const juce::String& clipId,
                       bool muted);
    bool setClipLocked (const juce::String& trackId,
                        const juce::String& clipId,
                        bool locked);
    bool setClipColour (const juce::String& trackId,
                        const juce::String& clipId,
                        juce::Colour colour);
    bool replaceClipWithProcessedAudio (const juce::String& trackId,
                                        const juce::String& clipId,
                                        const juce::File& renderedFile,
                                        double sampleRate,
                                        int channels,
                                        juce::int64 lengthInSamples,
                                        juce::String undoDescription);
    juce::String addWarpMarker (const juce::String& trackId,
                                const juce::String& clipId,
                                juce::int64 timelineSample);
    bool moveWarpMarker (const juce::String& trackId,
                         const juce::String& clipId,
                         const juce::String& markerId,
                         juce::int64 timelineSample);
    bool removeWarpMarker (const juce::String& trackId,
                           const juce::String& clipId,
                           const juce::String& markerId);
    bool setTransientMarkers (const juce::String& trackId,
                              const juce::String& clipId,
                              const std::vector<TransientMarkerState>& markers);
    bool clearTransientMarkers (const juce::String& trackId,
                                const juce::String& clipId);
    int convertTransientsToWarpMarkers (const juce::String& trackId,
                                        const juce::String& clipId);
    juce::String splitClip (const juce::String& trackId,
                            const juce::String& clipId,
                            juce::int64 splitTimelineSample);
    juce::String duplicateClip (const juce::String& trackId,
                                const juce::String& clipId,
                                juce::int64 newTimelineStartSamples = -1);
    juce::String captureClipVariant (const juce::String& trackId,
                                     const juce::String& clipId,
                                     juce::String variantName = {});
    bool restoreClipVariant (const juce::String& trackId,
                             const juce::String& clipId,
                             const juce::String& variantId);
    bool removeClipVariant (const juce::String& trackId,
                            const juce::String& clipId,
                            const juce::String& variantId);
    bool removeClip (const juce::String& trackId,
                     const juce::String& clipId);
    bool selectActiveTake (const juce::String& trackId,
                           const juce::String& clipId);
    bool assignCompRegion (const juce::String& trackId,
                           const juce::String& clipId,
                           juce::int64 timelineStartSamples,
                           juce::int64 timelineEndSamples);
    bool clearCompRegions (const juce::String& trackId,
                           const juce::String& takeGroupId);
    std::vector<AudioPlaybackSegmentState> resolveAudioPlayback (
        const juce::String& trackId) const;

    void beginUndoTransaction (const juce::String& description);
    bool canUndo() const;
    bool canRedo() const;
    juce::String getUndoDescription() const;
    juce::String getRedoDescription() const;
    bool undo();
    bool redo();
    void clearUndoHistory();

    ProjectState createSnapshot() const;
    void replaceWithSnapshot (const ProjectState& snapshot);

private:
    static juce::ValueTree createEmptyState();
    static juce::Colour defaultTrackColour (int index);

    juce::ValueTree findTrack (const juce::String& trackId) const;
    juce::ValueTree findMixerChannel (const juce::String& channelId) const;
    juce::ValueTree findMixerOwner (const juce::String& ownerId) const;
    juce::ValueTree findMixerSend (const juce::String& ownerId,
                                   const juce::String& sendId) const;
    juce::ValueTree findTimelineItem (const juce::Identifier& type,
                                      const juce::String& itemId) const;
    juce::ValueTree findAutomationLane (const juce::String& laneId) const;
    juce::ValueTree findMediaSource (const juce::String& sourceId) const;
    juce::ValueTree findClip (const juce::String& trackId,
                              const juce::String& clipId) const;
    juce::ValueTree findMidiClip (const juce::String& trackId,
                                  const juce::String& clipId) const;
    juce::ValueTree findPluginSlot (const juce::String& ownerId,
                                    const juce::String& slotId) const;
    void capturePersistentUndoState (const juce::String& description);
    bool restorePersistentUndoState (const PersistentUndoEntryState& entry);
    void notifyChanged();

    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override;
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override;
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override;
    void valueTreeParentChanged (juce::ValueTree&) override;

    juce::ValueTree state;
    juce::UndoManager undoManager { 10000, 30 };
    std::vector<ProjectRepairRecordState> repairRecords;
    UndoManifestState loadedUndoManifest;
    std::vector<PersistentUndoEntryState> persistentUndoHistory;
    std::vector<PersistentUndoEntryState> persistentRedoHistory;
    ChangeCallback onChanged;
    bool suppressNotifications = false;
    bool restoringPersistentHistory = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProjectModel)
};
}
