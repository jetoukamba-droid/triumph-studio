#pragma once

#include <JuceHeader.h>

#include "audio/AudioEngine.h"
#include "audio/AudioRecorder.h"
#include "audio/MultiTrackRecorder.h"
#include "audio/AdvancedAudioRenderer.h"
#include "audio/OfflineRenderer.h"
#include "audio/PluginFreezeRenderer.h"
#include "audio/StemExporter.h"
#include "core/MidiCapture.h"
#include "core/RecordingPlan.h"
#include "core/AudioDeviceRecovery.h"
#include "core/KeyboardShortcuts.h"
#include "model/ProjectModel.h"
#include "project/ProjectDocument.h"
#include "project/ProjectWorkspace.h"
#include "plugin/PluginScanService.h"
#include "plugin/PluginPresetStore.h"
#include "ui/ArrangementComponent.h"
#include "ui/AdvancedEditComponent.h"
#include "ui/MixerComponent.h"
#include "ui/TempoAutomationComponent.h"
#include "ui/PianoRollComponent.h"
#include "ui/ProducerAssistantComponent.h"
#include "ui/HelpAssistantComponent.h"
#include "ui/StudioLookAndFeel.h"
#include "ui/StudioIconButton.h"

#include <functional>
#include <atomic>
#include <cstdint>
#include <thread>

namespace triumph
{
class PluginEditorWindow;

class MainComponent final : public juce::AudioAppComponent,
                            private juce::Timer,
                            private juce::ChangeListener,
                            private juce::MidiInputCallback
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    void requestClose (std::function<void()> closeApproved);

private:
    using SaveCompletion = std::function<void (bool)>;

    static constexpr double autosaveIntervalMilliseconds = 120000.0;

    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void handleIncomingMidiMessage (juce::MidiInput*,
                                    const juce::MidiMessage&) override;

    void beginImportAudio();
    void handleExportButton();
    void showExportPresetMenu();
    void beginExportMix (OfflineRenderer::Options options);
    void beginExportStems();
    void beginOpenProject();
    void showOpenProjectChooser();
    void beginSaveProject (bool forceChooseFile, SaveCompletion completion = {});
    void createNewProject();
    void confirmSaveOrDiscard (std::function<void()> continuation);
    void resetProject();
    void loadProjectFromFile (const juce::File& file, bool isRecovery = false);
    bool saveProjectToFile (const juce::File& file);
    void beginCollectAndSave();
    void beginRelinkMissingMedia();
    void showRecentProjectsMenu (juce::Component* target = nullptr);
    void createAndArmAudioTrack();
    void createInstrumentTrack();
    void requestTrackDeletion (const juce::String& trackId);
    void showPianoRoll (const juce::String& requestedTrackId = {});
    void showVst3Menu();
    void chooseVst3Instrument();
    void chooseVst3Effect (juce::String ownerId);
    juce::File requirePluginScannerHelper();
    void beginStandardVst3Scan();
    void loadVst3Instrument (const juce::File& pluginFile);
    void loadVst3Effect (const juce::File& pluginFile,
                         juce::String ownerId);
    void completeVst3InstrumentScan (const PluginScanService::Result& result);
    void completeVst3EffectScan (const PluginScanService::Result& result,
                                 const juce::String& ownerId);
    void saveInstrumentPluginPreset();
    void loadInstrumentPluginPreset();
    void showInstrumentPluginEditor();
    void showInsertPluginEditor (const juce::String& slotId,
                                 const juce::String& name);
    void beginFreezeInstrumentTrack (const juce::String& trackId);
    void unfreezeInstrumentTrack (const juce::String& trackId);
    void synchroniseInstrumentPluginWithProject();
    void synchroniseInsertPluginsWithProject();
    void captureInstrumentPluginState();
    void captureInsertPluginStates();
    juce::String preferredInsertOwnerId() const;
    void toggleRecording();
    void startRecording();
    void stopRecording();
    void startMidiRecording (const juce::String& trackId);
    void stopMidiRecording();
    bool isAnyRecording() const noexcept;
    void showAudioDeviceSettings();
    void updateAudioDeviceStatus();
    void startPlayback();
    void startOutputTest();
    bool hasUsableAudioOutput();
    bool recoverDefaultAudioOutput();
    void saveAudioDeviceStateIfUsable();
    void queueAudioOutputRecovery();
    void serviceAudioOutputRecovery();
    void mixOutputTestTone (juce::AudioBuffer<float>& output,
                            int startSample,
                            int numSamples) noexcept;
    void detectSelectedClipTransients();
    void convertSelectedClipTransients();
    void toggleSelectedClipReverse();
    void beginNormalizeSelectedClip();
    void beginPitchShiftSelectedClip (double semitones);
    void generateProducerTrack (const juce::String& kind,
                                const ProducerSettingsState& settings);
    void applyProducerMix (const ProducerSettingsState& settings);
    void showHelpMenu();
    void showMoreMenu();
    juce::String buildRealtimeDiagnosticsReportText();
    void copyRealtimeDiagnosticsReport();
    void showRecordAlignmentDialog();
    void showRecordingSetupDialog();
    void setHelpAssistantOpen (bool shouldOpen,
                               juce::String initialQuestion = {});
    void openHelpAction (const juce::String& action);
    void captureInputPeaks (const juce::AudioBuffer<float>& input,
                            int startSample,
                            int numSamples) noexcept;

    void offerCrashRecovery();
    void offerRecordingRecovery();
    void performAutosave();

    bool addAudioFile (const juce::File& file, bool showFailureMessage = true);
    void setProjectName (juce::String newName);
    void setDirty (bool shouldBeDirty);
    void handleProjectChanged();
    void synchroniseEngineWithProject();
    void updateTransportDisplay();
    void updateProjectToolbarState();
    juce::String formatTime (double seconds) const;
    void showError (const juce::String& title, const juce::String& message);
    void showInformation (const juce::String& title, const juce::String& message);

    StudioLookAndFeel studioLookAndFeel;
    ProjectWorkspace workspace;
    ProjectModel project;
    AudioEngine audioEngine;
    MultiTrackRecorder recorder;
    recording::Plan recordingPlan;
    MidiCapture midiCapture;
    ArrangementComponent arrangement;
    MixerComponent mixer;
    TempoAutomationComponent tempoAutomation;
    AdvancedEditComponent advancedEdit;
    ProducerAssistantComponent producerAssistant;
    HelpAssistantComponent helpAssistant;

    juce::Label brandLabel { {}, "TRIUMPH STUDIO" };
    juce::Label projectLabel { {}, "Untitled Project" };
    StudioIconButton newButton { StudioIcon::newProject, "New Project" };
    StudioIconButton openButton { StudioIcon::open, "Open Project" };
    StudioIconButton recentButton { StudioIcon::recent, "Recent Projects" };
    StudioIconButton saveButton { StudioIcon::save, "Save Project" };
    StudioIconButton saveAsButton { StudioIcon::saveAs, "Save Project As" };
    StudioIconButton collectButton { StudioIcon::collect, "Collect and Save" };
    StudioIconButton relinkButton { StudioIcon::relink, "Relink Missing Media" };
    StudioIconButton importButton { StudioIcon::importAudio, "Import Audio" };
    StudioIconButton exportButton { StudioIcon::exportMix, "Export Mix" };
    StudioIconButton newTrackButton { StudioIcon::addTrack, "Add Audio Track" };
    StudioIconButton midiTrackButton { StudioIcon::addMidi, "Add Instrument Track" };
    StudioIconButton pianoRollButton { StudioIcon::pianoRoll, "Piano Roll" };
    StudioIconButton pluginButton { StudioIcon::plugin, "Plug-ins" };
    StudioIconButton deviceButton { StudioIcon::device, "Audio Device" };
    StudioIconButton undoButton { StudioIcon::undo, "Undo" };
    StudioIconButton redoButton { StudioIcon::redo, "Redo" };

    StudioIconButton rewindButton { StudioIcon::rewind, "Return to Start" };
    StudioIconButton playButton { StudioIcon::play, "Play" };
    StudioIconButton pauseButton { StudioIcon::pause, "Pause" };
    StudioIconButton stopButton { StudioIcon::stop, "Stop" };
    StudioIconButton recordButton { StudioIcon::record, "Record" };
    StudioIconButton monitorButton { StudioIcon::monitor, "Input Monitor" };
    StudioIconButton lowLatencyButton { StudioIcon::lowLatency, "Low Latency Monitoring" };
    StudioIconButton mixerButton { StudioIcon::mixer, "Mixer" };
    StudioIconButton automationButton { StudioIcon::tempo, "Tempo and Automation" };
    StudioIconButton advancedEditButton { StudioIcon::edit, "Audio Editor" };
    StudioIconButton producerButton { StudioIcon::producer, "Producer Assistant" };
    StudioIconButton helpButton { StudioIcon::help, "Help" };
    StudioIconButton moreButton { StudioIcon::more, "More Commands" };
    StudioIconButton splitButton { StudioIcon::split, "Split Clip" };
    StudioIconButton useTakeButton { StudioIcon::useTake, "Use Selected Take" };
    StudioIconButton detectButton { StudioIcon::detect, "Detect Transients" };
    StudioIconButton warpAllButton { StudioIcon::warp, "Warp All Transients" };
    StudioIconButton snapButton { StudioIcon::snap, "Snap" };
    juce::Label timeLabel { {}, "00:00.000 / 00:00.000" };
    juce::Label tempoLabel { {}, "TEMPO" };
    juce::Slider tempoSlider;
    juce::Label zoomLabel { {}, "ZOOM" };
    juce::Slider zoomSlider;
    juce::Label masterLabel { {}, "MASTER" };
    juce::Slider masterSlider;
    juce::Label audioStatusLabel { {}, "Audio engine starting..." };
    juce::Label realtimeStatusLabel { {}, "RT --  /  XRUN 0" };
    std::unique_ptr<juce::Component> inspectorPanel;
    std::unique_ptr<juce::Component> browserPanel;

    std::atomic<float> inputPeakLeft { 0.0f };
    std::atomic<float> inputPeakRight { 0.0f };
    juce::AudioBuffer<float> recordingInputScratch;
    std::atomic<bool> recordingStopRequested { false };
    float displayedInputPeakLeft = 0.0f;
    float displayedInputPeakRight = 0.0f;
    double leftClipHoldUntilMilliseconds = 0.0;
    double rightClipHoldUntilMilliseconds = 0.0;
    juce::Rectangle<int> brandLogoBounds;
    juce::Rectangle<int> inputMeterBounds;

    std::atomic<bool> outputTestRequested { false };
    std::atomic<bool> outputTestActive { false };
    std::atomic<double> activeOutputSampleRate { 48000.0 };
    std::atomic<std::uint64_t> outputCallbackCount { 0 };
    std::atomic<float> outputPeak { 0.0f };
    int outputTestTotalSamples = 0;
    int outputTestSamplesRendered = 0;
    double outputTestPhase = 0.0;
    std::uint64_t previousOutputCallbackCount = 0;
    int stagnantOutputTimerTicks = 0;
    int silentPlaybackTimerTicks = 0;
    bool outputWarningVisible = false;
    bool outputTestStatusPending = false;
    device::RecoveryTracker audioOutputRecovery;
    std::atomic<bool> audioCallbackPrepared { false };
    std::atomic<bool> audioOutputRecoveryQueued { false };
    std::atomic<bool> appShuttingDown { false };
    std::uint64_t callbackStallEvents = 0;
    std::uint64_t sustainedSilenceEvents = 0;
    bool callbackStallEventActive = false;
    bool sustainedSilenceEventActive = false;

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<PluginEditorWindow> pluginEditorWindow;
    std::thread transientAnalysisThread;
    std::atomic<bool> transientAnalysisInProgress { false };
    std::thread advancedEditThread;
    std::atomic<bool> advancedEditInProgress { false };
    std::atomic<bool> advancedEditCancelRequested { false };
    std::atomic<float> advancedEditProgress { 0.0f };
    std::thread exportThread;
    std::atomic<bool> exportInProgress { false };
    std::atomic<bool> exportCancelRequested { false };
    std::atomic<float> exportProgress { 0.0f };
    std::thread pluginScanThread;
    std::atomic<bool> pluginScanInProgress { false };
    juce::File currentProjectFile;
    juce::String currentProjectName { "Untitled Project" };
    bool dirty = false;
    bool replacingProject = false;
    juce::String recordingTrackId;
    juce::int64 recordingTimelineStartSamples = 0;
    double midiRecordingTransportStartSeconds = 0.0;
    bool recordingJournalWarningShown = false;
    double lastAutosaveAttemptMilliseconds = 0.0;
    bool pluginLoadInProgress = false;
    bool compactToolbarLayout = false;
    int pluginLoadGeneration = 0;
    std::uint64_t lastContainedPluginExceptionCount = 0;
    juce::String loadedPluginDescriptionXml;
    juce::String failedPluginDescriptionXml;
    juce::String insertPluginLoadInProgress;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
}
