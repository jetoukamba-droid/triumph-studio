#include "AudioEngine.h"

#include "core/TimelineMath.h"
#include "core/BuiltInInstrument.h"
#include "core/MixerMath.h"
#include "core/PluginHostPolicy.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iterator>
#include <limits>
#include <thread>
#include <utility>

namespace triumph
{
namespace
{
std::pair<double, double> preparedBeatAndBpmAtSeconds (
    double seconds, const std::vector<tempo::Point>& points) noexcept
{
    return tempo::beatAndBpmAtSecondsPrepared (seconds, points);
}

juce::String stableParameterIdFor (
    const juce::AudioProcessorParameter& parameter,
    int parameterIndex)
{
    juce::String providedId;
    if (const auto* hosted = dynamic_cast<
            const juce::HostedAudioProcessorParameter*> (&parameter))
        providedId = hosted->getParameterID();
    return plugin::stableParameterId (
        providedId.toStdString(), parameterIndex,
        parameter.getName (256).toStdString());
}

bool negotiateInstrumentBuses (juce::AudioPluginInstance& instance)
{
    instance.disableNonMainBuses();
    auto layout = instance.getBusesLayout();
    if (! layout.outputBuses.isEmpty()
        && layout.getMainOutputChannels() != 2)
    {
        auto stereo = layout;
        stereo.outputBuses.getReference (0) =
            juce::AudioChannelSet::stereo();
        if (instance.checkBusesLayoutSupported (stereo))
            instance.setBusesLayout (stereo);
    }
    return plugin::instrumentBusPolicy (
        true, instance.getTotalNumOutputChannels()).accepted;
}

bool negotiateEffectBuses (juce::AudioPluginInstance& instance)
{
    instance.disableNonMainBuses();
    auto layout = instance.getBusesLayout();
    auto stereo = layout;
    if (! stereo.inputBuses.isEmpty())
        stereo.inputBuses.getReference (0) = juce::AudioChannelSet::stereo();
    if (! stereo.outputBuses.isEmpty())
        stereo.outputBuses.getReference (0) = juce::AudioChannelSet::stereo();
    if (instance.checkBusesLayoutSupported (stereo))
        instance.setBusesLayout (stereo);
    const auto inputs = instance.getTotalNumInputChannels();
    const auto outputs = instance.getTotalNumOutputChannels();
    return inputs >= 1 && inputs <= 2 && outputs >= 1 && outputs <= 2;
}

std::uint64_t sampleRateMilliHz (double sampleRate) noexcept
{
    if (sampleRate <= 0.0)
        return 0;
    return static_cast<std::uint64_t> (sampleRate * 1000.0 + 0.5);
}

float renderNamedBuiltInInstrument (const juce::String& instrumentName,
                                    int noteNumber,
                                    int channel,
                                    float velocity,
                                    double elapsedSeconds,
                                    double remainingSeconds,
                                    juce::int64 absoluteSample,
                                    double sampleRate) noexcept
{
    const auto name = instrumentName.toLowerCase();
    if (name.contains ("drum"))
        return instrument::renderSample (noteNumber, 10, velocity,
                                         elapsedSeconds, remainingSeconds,
                                         absoluteSample, sampleRate);

    const auto base = instrument::renderSample (noteNumber, channel, velocity,
                                                elapsedSeconds,
                                                remainingSeconds,
                                                absoluteSample, sampleRate);
    if (name.contains ("synth"))
    {
        const auto overtone = instrument::renderSample (
            noteNumber + 12, channel, velocity * 0.42f,
            elapsedSeconds, remainingSeconds, absoluteSample, sampleRate);
        return juce::jlimit (-0.85f, 0.85f, base * 0.92f + overtone * 0.58f);
    }
    if (name.contains ("sampler"))
        return base * 0.78f
               + instrument::deterministicNoise (absoluteSample, noteNumber)
                     * velocity * 0.018f;

    return base;
}

class CallbackMeasurement final
{
public:
    CallbackMeasurement (realtime::Telemetry& target,
                         int sampleCount,
                         double sampleRate) noexcept
        : telemetry (target), started (Clock::now())
    {
        if (sampleCount > 0 && sampleRate > 0.0)
            deadlineNanoseconds = static_cast<std::uint64_t> (
                std::llround (static_cast<double> (sampleCount)
                              * 1000000000.0 / sampleRate));
    }

    ~CallbackMeasurement()
    {
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds> (
            Clock::now() - started).count();
        telemetry.recordCallback (
            static_cast<std::uint64_t> (std::max<std::int64_t> (0, elapsed)),
            deadlineNanoseconds, suspectedDropout);
    }

    void markSuspectedDropout() noexcept { suspectedDropout = true; }

private:
    using Clock = std::chrono::steady_clock;
    realtime::Telemetry& telemetry;
    Clock::time_point started;
    std::uint64_t deadlineNanoseconds = 0;
    bool suspectedDropout = false;
};
}

AudioEngine::AudioEngine()
{
    formatManager.registerBasicFormats();
    pluginFormatManager.addFormat (std::make_unique<juce::VST3PluginFormat>());
    readAheadThread.startThread();
}

AudioEngine::~AudioEngine()
{
    pluginHostAlive->store (false);
    for (const auto& slotId : getLoadedInsertPluginSlotIds())
        unloadInsertPlugin (slotId);
    unloadInstrumentPlugin();
    clearTracks();
    renderStates.clear();
    reclaimRetiredResources();
    readAheadThread.stopThread (3000);
}

void AudioEngine::beginRenderStateUpdate()
{
    const juce::ScopedLock lock (controlLock);
    ++renderUpdateDepth;
}

void AudioEngine::endRenderStateUpdate()
{
    const juce::ScopedLock lock (controlLock);
    jassert (renderUpdateDepth > 0);
    if (renderUpdateDepth > 0)
        --renderUpdateDepth;
    if (renderUpdateDepth == 0 && renderStateDirty)
        publishRenderSnapshot();
}

AudioEngine::FileDetails AudioEngine::inspectAudioFile (const juce::File& audioFile)
{
    FileDetails details;

    auto reader = std::unique_ptr<juce::AudioFormatReader> (
        formatManager.createReaderFor (audioFile));

    if (reader != nullptr)
    {
        details.sampleRate = reader->sampleRate;
        details.channels = static_cast<int> (reader->numChannels);
        details.lengthInSamples = reader->lengthInSamples;
    }

    return details;
}

AudioTrack* AudioEngine::addTrack (const juce::String& trackId,
                                   const juce::File& audioFile)
{
    if (trackId.isEmpty())
        return nullptr;

    if (auto* existing = findTrack (trackId))
        return existing;

    auto newTrack = std::make_shared<AudioTrack> (trackId,
                                                  audioFile,
                                                  formatManager,
                                                  thumbnailCache,
                                                  readAheadThread);

    if (! newTrack->isValid())
        return nullptr;

    const juce::ScopedLock lock (controlLock);
    if (preparedBlockSize > 0 && preparedSampleRate > 0.0)
    {
        newTrack->prepareToPlay (preparedBlockSize, preparedSampleRate);
        newTrack->play();
    }
    auto* result = newTrack.get();
    tracks.push_back (std::move (newTrack));
    markRenderStateDirty();
    return result;
}

bool AudioEngine::removeTrack (const juce::String& trackId)
{
    const juce::ScopedLock lock (controlLock);
    const auto match = std::find_if (tracks.begin(), tracks.end(), [&trackId] (const auto& track)
    {
        return track->getId() == trackId;
    });

    if (match == tracks.end())
        return false;

    tracks.erase (match);
    markRenderStateDirty();
    return true;
}

void AudioEngine::clearTracks()
{
    playing.store (false);
    const juce::ScopedLock lock (controlLock);

    tracks.clear();
    midiTracks.clear();
    delayConfiguration.clear();
    graphLatencySamples.store (0);
    markRenderStateDirty();
}

int AudioEngine::getTrackCount() const noexcept
{
    return static_cast<int> (tracks.size());
}

AudioTrack* AudioEngine::getTrack (int index) noexcept
{
    return juce::isPositiveAndBelow (index, getTrackCount()) ? tracks[static_cast<size_t> (index)].get()
                                                              : nullptr;
}

const AudioTrack* AudioEngine::getTrack (int index) const noexcept
{
    return juce::isPositiveAndBelow (index, getTrackCount()) ? tracks[static_cast<size_t> (index)].get()
                                                              : nullptr;
}

AudioTrack* AudioEngine::findTrack (const juce::String& trackId) noexcept
{
    const auto match = std::find_if (tracks.begin(), tracks.end(), [&trackId] (const auto& track)
    {
        return track->getId() == trackId;
    });
    return match != tracks.end() ? match->get() : nullptr;
}

const AudioTrack* AudioEngine::findTrack (const juce::String& trackId) const noexcept
{
    const auto match = std::find_if (tracks.begin(), tracks.end(), [&trackId] (const auto& track)
    {
        return track->getId() == trackId;
    });
    return match != tracks.end() ? match->get() : nullptr;
}

void AudioEngine::configureTrackClips (const juce::String& trackId,
                                       std::vector<AudioTrack::ClipPlayback> clips,
                                       double timelineSampleRate)
{
    const juce::ScopedLock lock (controlLock);
    const auto match = std::find_if (tracks.begin(), tracks.end(), [&trackId] (const auto& track)
    {
        return track->getId() == trackId;
    });

    if (match != tracks.end())
    {
        const auto missingSource = std::find_if (
            clips.begin(), clips.end(), [&] (const auto& clip)
        {
            return ! (*match)->hasSource (clip.sourceId, clip.sourceFile);
        });
        if (missingSource == clips.end())
        {
            // Clip timing/rate/gain is published through AudioTrack's own
            // lock-free schedule exchange. No graph or reader rebuild is
            // required for ordinary arrangement edits.
            (*match)->setClips (std::move (clips), timelineSampleRate);
            return;
        }

        const auto previous = *match;
        const auto primaryFile = clips.empty() ? previous->getSourceFile()
                                                : clips.front().sourceFile;
        auto replacement = std::make_shared<AudioTrack> (
            trackId, primaryFile, formatManager, thumbnailCache,
            readAheadThread);
        if (! replacement->isValid())
            return;
        replacement->setName (previous->getName());
        replacement->setGain (previous->getGain());
        replacement->setPan (previous->getPan());
        replacement->setMuted (previous->isMuted());
        replacement->setSolo (previous->isSolo());
        replacement->setClips (std::move (clips), timelineSampleRate);
        if (preparedBlockSize > 0 && preparedSampleRate > 0.0)
        {
            replacement->prepareToPlay (preparedBlockSize,
                                         preparedSampleRate);
            replacement->play();
        }
        *match = std::move (replacement);
        markRenderStateDirty();
    }
}

void AudioEngine::configureMidiTracks (std::vector<MidiTrackPlayback> newTracks,
                                       std::vector<tempo::Point> newTempoPoints)
{
    const juce::ScopedLock lock (controlLock);
    tempoPoints = tempo::normalised (std::move (newTempoPoints));
    for (auto& track : newTracks)
    {
        for (auto& note : track.notes)
        {
            note.startSeconds = tempo::beatToSeconds (note.startBeat, tempoPoints);
            note.endSeconds = tempo::beatToSeconds (note.startBeat + note.lengthBeats,
                                                    tempoPoints);
            note.performanceChannel = juce::jlimit (1, 16, note.channel);
            for (auto& expression : note.expression)
                expression.seconds = tempo::beatToSeconds (
                    note.startBeat + expression.offsetBeats, tempoPoints);
        }

        struct ActiveMpeNote
        {
            std::uint32_t id = 0;
            double endBeat = 0.0;
        };
        std::vector<MidiNotePlayback*> ordered;
        ordered.reserve (track.notes.size());
        for (auto& note : track.notes)
            ordered.push_back (&note);
        std::stable_sort (ordered.begin(), ordered.end(), [] (const auto* a,
                                                               const auto* b)
        {
            return a->startBeat < b->startBeat;
        });
        midi::MpeZoneAllocator allocator;
        std::vector<ActiveMpeNote> active;
        for (std::size_t index = 0; index < ordered.size(); ++index)
        {
            auto& note = *ordered[index];
            for (auto current = active.begin(); current != active.end();)
                if (current->endBeat <= note.startBeat)
                {
                    allocator.release (current->id);
                    current = active.erase (current);
                }
                else
                    ++current;
            if (note.noteId == 0 && note.expression.empty())
                continue;
            const auto id = note.noteId != 0
                ? note.noteId : static_cast<std::uint32_t> (index + 1);
            if (const auto channel = allocator.allocate (id))
            {
                note.performanceChannel = *channel;
                active.push_back ({ id, note.startBeat + note.lengthBeats });
            }
        }
        for (auto& controller : track.controllers)
            controller.seconds = tempo::beatToSeconds (
                controller.beat, tempoPoints);
    }
    midiTracks = std::move (newTracks);
    markRenderStateDirty();
}

void AudioEngine::configureMusicalTimeline (
    std::vector<tempo::Point> newTempoPoints,
    std::vector<automation::Signature> newSignatures,
    bool shouldEnableMetronome, float clickGain)
{
    const juce::ScopedLock lock (controlLock);
    tempoPoints = tempo::normalised (std::move (newTempoPoints));
    timeSignatures = automation::normalisedSignatures (
        std::move (newSignatures));
    metronomeEnabled.store (shouldEnableMetronome);
    metronomeGain.store (juce::jlimit (0.0f, 1.0f, clickGain));
    markRenderStateDirty();
}

void AudioEngine::configureDelayCompensation (
    std::vector<DelayTrack> newTracks,
    bool lowLatencyMonitoring)
{
    const juce::ScopedLock lock (controlLock);
    lowLatencyMonitoringEnabled.store (lowLatencyMonitoring);
    delayTrackConfiguration = std::move (newTracks);
    rebuildDelayCompensationPlan();
    markRenderStateDirty();
}

void AudioEngine::rebuildDelayCompensationPlan()
{
    std::vector<plugins::Node> nodes;
    nodes.reserve (delayTrackConfiguration.size()
                   + mixerConfiguration.size() + 1);
    for (const auto& track : delayTrackConfiguration)
        nodes.push_back ({ track.id.toStdString(),
                           track.pluginLatencySamples,
                           track.pluginBypassed,
                           track.frozen,
                           track.pluginAvailable,
                           track.manualLatencyOffsetSamples,
                           track.monitored });
    for (const auto& channel : mixerConfiguration)
    {
        const auto exists = std::any_of (
            nodes.begin(), nodes.end(), [&channel] (const auto& node)
        {
            return node.id == channel.id.toStdString();
        });
        if (! exists)
            nodes.push_back ({ channel.id.toStdString(),
                               channel.pluginLatencySamples,
                               false, false, true, 0, false });
    }
    constexpr auto pdcMasterId = "__triumph_master__";
    nodes.push_back ({ pdcMasterId, 0, false, false, true, 0, false });

    std::vector<plugins::Edge> edges;
    if (mixerPlanConfiguration.valid)
    {
        edges.reserve (mixerPlanConfiguration.routes.size());
        for (const auto& route : mixerPlanConfiguration.routes)
            if (route.enabled)
                edges.push_back ({ route.source,
                    route.destination == mixer::masterId
                        ? pdcMasterId : route.destination,
                    route.kind == mixer::RouteKind::sidechain
                        ? plugins::EdgeKind::sidechain
                        : route.kind == mixer::RouteKind::send
                            ? plugins::EdgeKind::send
                            : plugins::EdgeKind::audio,
                    0 });
    }
    else
    {
        edges.reserve (delayTrackConfiguration.size());
        for (const auto& track : delayTrackConfiguration)
            edges.push_back ({ track.id.toStdString(), pdcMasterId,
                               plugins::EdgeKind::audio, 0 });
    }

    plugins::Options options;
    options.lowLatencyMonitoring = lowLatencyMonitoringEnabled.load();
    delayPlanConfiguration = plugins::buildCompensationPlan (
        nodes, edges, options);
    graphLatencySamples.store (delayPlanConfiguration.valid
        ? static_cast<int> (juce::jmin<std::int64_t> (
              delayPlanConfiguration.graphLatencySamples,
              std::numeric_limits<int>::max()))
        : 0);

    std::vector<PreparedDelay> replacement;
    replacement.reserve (delayTrackConfiguration.size());
    for (const auto& track : delayTrackConfiguration)
    {
        auto delay = 0;
        if (! mixerPlanConfiguration.valid && delayPlanConfiguration.valid)
        {
            const auto node = std::find_if (
                delayPlanConfiguration.nodes.begin(),
                delayPlanConfiguration.nodes.end(), [&track] (const auto& item)
            {
                return item.id == track.id.toStdString();
            });
            if (node != delayPlanConfiguration.nodes.end())
                delay = static_cast<int> (juce::jlimit<std::int64_t> (
                    0, 262143, node->delaySamples));
        }
        replacement.push_back ({ track.id, delay });
    }
    delayConfiguration = std::move (replacement);
}

int AudioEngine::getGraphLatencySamples() const noexcept
{
    return graphLatencySamples.load();
}

bool AudioEngine::isLowLatencyMonitoringEnabled() const noexcept
{
    return lowLatencyMonitoringEnabled.load();
}

bool AudioEngine::configureMixer (std::vector<MixerChannel> channels)
{
    std::vector<mixer::Channel> graphChannels;
    std::vector<mixer::Route> graphRoutes;
    graphChannels.reserve (channels.size());
    for (const auto& channel : channels)
    {
        graphChannels.push_back ({ channel.id.toStdString(), channel.kind,
                                   channel.outputDestinationId.toStdString(),
                                   { static_cast<std::size_t> (juce::jlimit (
                                         0, 16, channel.inputChannels)),
                                     static_cast<std::size_t> (juce::jlimit (
                                         1, 16, channel.outputChannels)),
                                     static_cast<std::size_t> (juce::jlimit (
                                         0, 16, channel.sidechainChannels)),
                                     channel.explicitBusLayout } });
        for (const auto& send : channel.sends)
            graphRoutes.push_back ({ send.id.toStdString(),
                                     channel.id.toStdString(),
                                     send.destinationId.toStdString(),
                                     send.sidechain ? mixer::RouteKind::sidechain
                                                    : mixer::RouteKind::send,
                                     send.gain, send.preFader, send.enabled });
    }

    auto plan = mixer::buildPlan (std::move (graphChannels),
                                  std::move (graphRoutes));
    const juce::ScopedLock lock (controlLock);
    if (! plan.valid)
    {
        mixerGraphError = plan.error;
        return false;
    }

    std::vector<MeterPublication> meters;
    meters.reserve (channels.size());
    for (const auto& channel : channels)
    {
        const auto previous = std::find_if (
            meterPublications.begin(), meterPublications.end(), [&channel] (const auto& candidate)
        {
            return candidate.id == channel.id;
        });
        meters.push_back ({ channel.id,
                            previous != meterPublications.end()
                                ? previous->values
                                : std::make_shared<MeterValues>() });
    }
    meterPublications = std::move (meters);
    mixerConfiguration = std::move (channels);
    mixerPlanConfiguration = std::move (plan);
    rebuildDelayCompensationPlan();
    mixerGraphError.clear();
    markRenderStateDirty();
    return true;
}

bool AudioEngine::configureMonitorPaths (std::vector<MonitorPath> paths)
{
    const juce::ScopedLock lock (controlLock);
    std::vector<mixer::MonitorPath> graphPaths;
    graphPaths.reserve (paths.size());
    for (const auto& path : paths)
        graphPaths.push_back ({ path.id.toStdString(), path.role,
                                path.sourceId.toStdString(),
                                static_cast<std::size_t> (juce::jlimit (
                                    1, 16, path.outputChannelCount)),
                                path.gain, path.muted, path.dimmed,
                                static_cast<std::size_t> (juce::jlimit (
                                    0, 15, path.outputStartChannel)) });
    auto plan = mixer::buildMonitorPlan (std::move (graphPaths),
                                         mixerPlanConfiguration);
    if (! plan.valid)
    {
        mixerGraphError = plan.error;
        return false;
    }
    monitorPlanConfiguration = std::move (plan);
    markRenderStateDirty();
    return true;
}

void AudioEngine::configureSync (SyncConfiguration configuration)
{
    configuration.mtcFramesPerSecond = juce::jlimit (
        24, 30, configuration.mtcFramesPerSecond);
    configuration.jitterToleranceSamples = juce::jlimit (
        0, 65536, configuration.jitterToleranceSamples);
    const juce::SpinLock::ScopedLockType lock (syncLock);
    syncConfiguration = configuration;
    syncFollower.setSampleRate (projectTimelineSampleRate.load());
    syncFollower.setSource (configuration.source);
    activeSyncSource.store (configuration.source, std::memory_order_release);
    lastExternalSyncSample.store (-1, std::memory_order_release);
    const auto status = syncFollower.status();
    externalSyncLocked.store (status.locked, std::memory_order_release);
    externalSyncTempoBpm.store (status.tempoBpm,
                                std::memory_order_release);
}

midi::SyncStatus AudioEngine::getSyncStatus() const noexcept
{
    const juce::SpinLock::ScopedLockType lock (syncLock);
    auto status = syncFollower.status();
    if (status.source != midi::SyncSource::internal)
    {
        const auto nowNanoseconds = std::chrono::duration_cast<
            std::chrono::nanoseconds> (
                std::chrono::steady_clock::now().time_since_epoch()).count();
        const auto now = static_cast<juce::int64> (std::llround (
            static_cast<double> (nowNanoseconds)
            * projectTimelineSampleRate.load() / 1000000000.0));
        const auto last = lastExternalSyncSample.load();
        const auto timeout = static_cast<juce::int64> (
            projectTimelineSampleRate.load()
            * (status.source == midi::SyncSource::midiClock ? 0.5 : 1.0));
        status.locked = last >= 0 && now - last <= timeout;
    }
    return status;
}

void AudioEngine::configureAutomation (std::vector<AutomationPlayback> lanes)
{
    const juce::ScopedLock lock (controlLock);
    automationConfiguration = std::move (lanes);
    markRenderStateDirty();
}

juce::String AudioEngine::getMixerGraphError() const
{
    const juce::ScopedLock lock (controlLock);
    return mixerGraphError;
}

std::vector<AudioEngine::MixerMeter> AudioEngine::getMixerMeters() const
{
    std::vector<MixerMeter> result;
    const juce::ScopedLock lock (controlLock);
    result.reserve (meterPublications.size());
    for (const auto& meter : meterPublications)
        result.push_back ({ meter.id,
                            meter.values->left.load (std::memory_order_relaxed),
                            meter.values->right.load (std::memory_order_relaxed),
                            meter.values->sidechainReduction.load (
                                std::memory_order_relaxed) });
    return result;
}

AudioEngine::RealtimeStatus AudioEngine::getRealtimeStatus() const noexcept
{
    return realtimeTelemetry.snapshot();
}

AudioEngine::PluginRuntimeStatus
AudioEngine::getPluginRuntimeStatus() const noexcept
{
    return {
        pluginLastProcessNanoseconds.load (std::memory_order_relaxed),
        pluginMaximumProcessNanoseconds.load (std::memory_order_relaxed),
        pluginDeadlineMisses.load (std::memory_order_relaxed),
        pluginContainedExceptions.load (std::memory_order_relaxed),
        instrumentContainedExceptions.load (std::memory_order_relaxed)
    };
}

int AudioEngine::pullSpectrumSamples (float* destination,
                                      int maximumSamples) noexcept
{
    if (destination == nullptr || maximumSamples <= 0)
        return 0;

    auto available = spectrumFifo.getNumReady();
    const auto discard = juce::jmax (0, available - maximumSamples);
    if (discard > 0)
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        spectrumFifo.prepareToRead (discard, start1, size1, start2, size2);
        spectrumFifo.finishedRead (size1 + size2);
    }
    available = juce::jmin (maximumSamples, spectrumFifo.getNumReady());
    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    spectrumFifo.prepareToRead (available, start1, size1, start2, size2);
    std::copy_n (spectrumRing.data() + start1, size1, destination);
    std::copy_n (spectrumRing.data() + start2, size2, destination + size1);
    spectrumFifo.finishedRead (size1 + size2);
    return size1 + size2;
}

void AudioEngine::loadInstrumentPlugin (
    juce::String trackId,
    juce::PluginDescription description,
    juce::String stateBase64,
    std::function<void (bool, juce::String)> completion)
{
    const auto request = pluginLoadRequest.fetch_add (1) + 1;
    const auto alive = pluginHostAlive;
    double sampleRate = 44100.0;
    int blockSize = 512;
    {
        const juce::ScopedLock lock (controlLock);
        if (preparedSampleRate > 0.0)
            sampleRate = preparedSampleRate;
        if (preparedBlockSize > 0)
            blockSize = preparedBlockSize;
    }

    pluginFormatManager.createPluginInstanceAsync (
        description, sampleRate, blockSize,
        [this, alive, request, trackId = std::move (trackId), description,
         stateBase64 = std::move (stateBase64),
         completion = std::move (completion), sampleRate, blockSize]
        (std::unique_ptr<juce::AudioPluginInstance> instance,
         const juce::String& error)
        {
            if (! alive->load())
                return;

            if (request != pluginLoadRequest.load())
            {
                if (completion)
                    completion (false, "Plug-in load superseded by a newer request.");
                return;
            }

            if (instance == nullptr)
            {
                if (completion)
                    completion (false, error.isNotEmpty() ? error
                                                          : "The VST3 instance could not be created.");
                return;
            }

            if (! description.isInstrument)
            {
                if (completion)
                    completion (false, "The selected VST3 is not an instrument plug-in.");
                return;
            }

            if (! negotiateInstrumentBuses (*instance))
            {
                if (completion)
                    completion (false,
                                "The instrument did not expose a supported audio output bus.");
                return;
            }

            if (stateBase64.isNotEmpty())
            {
                juce::MemoryBlock state;
                if (state.fromBase64Encoding (stateBase64))
                    instance->setStateInformation (state.getData(),
                                                   static_cast<int> (state.getSize()));
            }

            prepareInstrumentPlugin (*instance, sampleRate, blockSize);
            const auto reportedLatency = juce::jmax (
                0, instance->getLatencySamples());
            instance->addListener (this);

            {
                const juce::ScopedLock lock (controlLock);
                if (instrumentPlugin != nullptr)
                {
                    instrumentPlugin->removeListener (this);
                    retiredPlugins.push_back (std::move (instrumentPlugin));
                }
                instrumentPlugin = std::move (instance);
                instrumentPluginProcessor.store (
                    instrumentPlugin.get(), std::memory_order_release);
                instrumentPluginTrackId = trackId;
                instrumentPluginName = description.name;
                reportedInstrumentPluginLatencySamples.store (
                    reportedLatency, std::memory_order_release);
                instrumentPluginLatencyChangePending.store (
                    false, std::memory_order_release);
                instrumentPluginLoaded.store (true);
                pluginNeedsAllNotesOff.store (true);
                pluginTailSamplesRemaining.store (0);
                liveMidiCollector.reset (sampleRate);
                markRenderStateDirty();
            }
            if (completion)
                completion (true, description.name);
        });
}

void AudioEngine::loadInsertPlugin (
    InsertPluginLoad request,
    std::function<void (bool, juce::String, int)> completion)
{
    const auto token = insertPluginLoadRequest.fetch_add (1) + 1;
    const auto alive = pluginHostAlive;
    double sampleRate = 44100.0;
    int blockSize = 512;
    {
        const juce::ScopedLock lock (controlLock);
        if (preparedSampleRate > 0.0)
            sampleRate = preparedSampleRate;
        if (preparedBlockSize > 0)
            blockSize = preparedBlockSize;
    }
    pluginFormatManager.createPluginInstanceAsync (
        request.description, sampleRate, blockSize,
        [this, alive, token, request = std::move (request),
         completion = std::move (completion), sampleRate, blockSize]
        (std::unique_ptr<juce::AudioPluginInstance> instance,
         const juce::String& error) mutable
        {
            if (! alive->load())
                return;
            if (token != insertPluginLoadRequest.load())
            {
                if (completion)
                    completion (false,
                                "Insert load superseded by a newer request.", 0);
                return;
            }
            if (instance == nullptr)
            {
                if (completion)
                    completion (false, error.isNotEmpty() ? error
                        : "The VST3 insert instance could not be created.", 0);
                return;
            }
            if (request.description.isInstrument)
            {
                if (completion)
                    completion (false,
                                "An instrument cannot occupy an audio-effect slot.",
                                0);
                return;
            }
            if (! negotiateEffectBuses (*instance))
            {
                if (completion)
                    completion (false,
                                "The insert does not expose a supported mono/stereo main bus.",
                                0);
                return;
            }
            if (request.stateBase64.isNotEmpty())
            {
                juce::MemoryBlock state;
                if (! state.fromBase64Encoding (request.stateBase64))
                {
                    if (completion)
                        completion (false,
                                    "The stored insert state is damaged.", 0);
                    return;
                }
                instance->setStateInformation (
                    state.getData(), static_cast<int> (state.getSize()));
            }
            prepareInstrumentPlugin (*instance, sampleRate, blockSize);
            const auto latency = juce::jmax (0, instance->getLatencySamples());
            instance->addListener (this);
            {
                const juce::ScopedLock lock (controlLock);
                const auto existing = std::find_if (
                    insertPlugins.begin(), insertPlugins.end(),
                    [&request] (const auto& slot)
                {
                    return slot.slotId == request.slotId;
                });
                if (existing != insertPlugins.end())
                {
                    if (existing->instance != nullptr)
                    {
                        existing->instance->removeListener (this);
                        retiredPlugins.push_back (
                            std::move (existing->instance));
                    }
                    insertPlugins.erase (existing);
                }
                insertPlugins.push_back ({ request.slotId, request.ownerId,
                    juce::jlimit (0, 15, request.order),
                    request.descriptionXml, std::move (instance),
                    request.bypassed, latency });
                std::stable_sort (
                    insertPlugins.begin(), insertPlugins.end(),
                    [] (const auto& a, const auto& b)
                {
                    return a.ownerId == b.ownerId
                        ? a.order < b.order : a.ownerId < b.ownerId;
                });
                markRenderStateDirty();
            }
            if (completion)
                completion (true, request.description.name, latency);
        });
}

void AudioEngine::unloadInsertPlugin (const juce::String& slotId)
{
    insertPluginLoadRequest.fetch_add (1);
    const juce::ScopedLock lock (controlLock);
    const auto slot = std::find_if (
        insertPlugins.begin(), insertPlugins.end(), [&slotId] (const auto& item)
    {
        return item.slotId == slotId;
    });
    if (slot == insertPlugins.end())
        return;
    if (slot->instance != nullptr)
    {
        slot->instance->removeListener (this);
        retiredPlugins.push_back (std::move (slot->instance));
    }
    insertPlugins.erase (slot);
    requestRuntimeReset();
    markRenderStateDirty();
}

std::vector<juce::String> AudioEngine::getLoadedInsertPluginSlotIds() const
{
    const juce::ScopedLock lock (controlLock);
    std::vector<juce::String> result;
    result.reserve (insertPlugins.size());
    for (const auto& slot : insertPlugins)
        result.push_back (slot.slotId);
    return result;
}

juce::String AudioEngine::getInsertPluginDescriptionXml (
    const juce::String& slotId) const
{
    const juce::ScopedLock lock (controlLock);
    const auto slot = std::find_if (
        insertPlugins.begin(), insertPlugins.end(), [&slotId] (const auto& item)
    {
        return item.slotId == slotId;
    });
    return slot != insertPlugins.end() ? slot->descriptionXml : juce::String();
}

void AudioEngine::setInsertPluginBypassed (
    const juce::String& slotId, bool shouldBeBypassed)
{
    const juce::ScopedLock lock (controlLock);
    const auto slot = std::find_if (
        insertPlugins.begin(), insertPlugins.end(), [&slotId] (const auto& item)
    {
        return item.slotId == slotId;
    });
    if (slot != insertPlugins.end() && slot->bypassed != shouldBeBypassed)
    {
        slot->bypassed = shouldBeBypassed;
        markRenderStateDirty();
    }
}

juce::String AudioEngine::captureInsertPluginStateBase64 (
    const juce::String& slotId)
{
    beginPluginControl();
    juce::String encoded;
    {
        const juce::ScopedLock lock (controlLock);
        const auto slot = std::find_if (
            insertPlugins.begin(), insertPlugins.end(),
            [&slotId] (const auto& item)
        {
            return item.slotId == slotId;
        });
        if (slot != insertPlugins.end() && slot->instance != nullptr)
        {
            juce::MemoryBlock state;
            slot->instance->getStateInformation (state);
            encoded = state.toBase64Encoding();
        }
    }
    endPluginControl();
    return encoded;
}

std::vector<AudioEngine::PluginParameterDescriptor>
AudioEngine::getInsertPluginParameters (const juce::String& slotId)
{
    std::vector<PluginParameterDescriptor> result;
    beginPluginControl();
    {
        const juce::ScopedLock lock (controlLock);
        const auto slot = std::find_if (
            insertPlugins.begin(), insertPlugins.end(),
            [&slotId] (const auto& item)
        {
            return item.slotId == slotId;
        });
        if (slot != insertPlugins.end() && slot->instance != nullptr)
        {
            const auto parameters = slot->instance->getParameters();
            result.reserve (static_cast<std::size_t> (parameters.size()));
            for (int index = 0; index < parameters.size(); ++index)
            {
                const auto* parameter = parameters[index];
                if (parameter == nullptr)
                    continue;
                result.push_back ({
                    stableParameterIdFor (*parameter, index),
                    parameter->getName (256), parameter->getLabel(),
                    parameter->getDefaultValue(), parameter->getValue(),
                    parameter->getNumSteps(), parameter->isAutomatable(),
                    parameter->isDiscrete(),
                    parameter->isOrientationInverted() });
            }
        }
    }
    endPluginControl();
    return result;
}

std::vector<AudioEngine::InsertPluginParameterInventory>
AudioEngine::getInsertPluginParameterInventory()
{
    std::vector<InsertPluginParameterInventory> result;
    beginPluginControl();
    {
        const juce::ScopedLock lock (controlLock);
        result.reserve (insertPlugins.size());
        for (const auto& slot : insertPlugins)
        {
            if (slot.instance == nullptr)
                continue;
            InsertPluginParameterInventory inventory;
            inventory.slotId = slot.slotId;
            const auto parameters = slot.instance->getParameters();
            inventory.parameters.reserve (
                static_cast<std::size_t> (parameters.size()));
            for (int index = 0; index < parameters.size(); ++index)
            {
                const auto* parameter = parameters[index];
                if (parameter == nullptr)
                    continue;
                inventory.parameters.push_back ({
                    stableParameterIdFor (*parameter, index),
                    parameter->getName (256), parameter->getLabel(),
                    parameter->getDefaultValue(), parameter->getValue(),
                    parameter->getNumSteps(), parameter->isAutomatable(),
                    parameter->isDiscrete(),
                    parameter->isOrientationInverted() });
            }
            result.push_back (std::move (inventory));
        }
    }
    endPluginControl();
    return result;
}

bool AudioEngine::setInsertPluginParameterValue (
    const juce::String& slotId, const juce::String& stableId,
    float normalisedValue)
{
    auto changed = false;
    beginPluginControl();
    {
        const juce::ScopedLock lock (controlLock);
        const auto slot = std::find_if (
            insertPlugins.begin(), insertPlugins.end(),
            [&slotId] (const auto& item)
        {
            return item.slotId == slotId;
        });
        if (slot != insertPlugins.end() && slot->instance != nullptr)
        {
            const auto parameters = slot->instance->getParameters();
            for (int index = 0; index < parameters.size(); ++index)
            {
                auto* parameter = parameters[index];
                if (parameter == nullptr
                    || stableParameterIdFor (*parameter, index) != stableId)
                    continue;
                parameter->beginChangeGesture();
                parameter->setValueNotifyingHost (
                    juce::jlimit (0.0f, 1.0f, normalisedValue));
                parameter->endChangeGesture();
                changed = true;
                break;
            }
        }
    }
    endPluginControl();
    return changed;
}

juce::AudioProcessorEditor* AudioEngine::getInsertPluginEditor (
    const juce::String& slotId)
{
    juce::AudioProcessorEditor* editor = nullptr;
    beginPluginControl();
    {
        const juce::ScopedLock lock (controlLock);
        const auto slot = std::find_if (
            insertPlugins.begin(), insertPlugins.end(),
            [&slotId] (const auto& item)
        {
            return item.slotId == slotId;
        });
        if (slot != insertPlugins.end() && slot->instance != nullptr)
            editor = slot->instance->createEditorIfNeeded();
    }
    endPluginControl();
    return editor;
}

std::vector<AudioEngine::InsertPluginLatencyChange>
AudioEngine::consumeInsertPluginLatencyChanges()
{
    std::vector<std::pair<juce::AudioProcessor*, int>> notices;
    notices.reserve (insertLatencyNotices.size());
    for (auto& notice : insertLatencyNotices)
    {
        if (notice.state.load (std::memory_order_acquire) != 2)
            continue;
        notices.emplace_back (
            notice.processor.load (std::memory_order_relaxed),
            notice.samples.load (std::memory_order_relaxed));
        notice.processor.store (nullptr, std::memory_order_relaxed);
        notice.state.store (0, std::memory_order_release);
    }
    std::vector<InsertPluginLatencyChange> result;
    if (notices.empty())
        return result;
    const juce::ScopedLock lock (controlLock);
    for (const auto& [processor, latency] : notices)
    {
        const auto slot = std::find_if (
            insertPlugins.begin(), insertPlugins.end(),
            [processor] (const auto& item)
        {
            return item.instance.get() == processor;
        });
        if (slot == insertPlugins.end())
            continue;
        slot->latencySamples = juce::jlimit (0, 262143, latency);
        result.push_back ({ slot->slotId, slot->latencySamples });
    }
    return result;
}

std::vector<juce::String>
AudioEngine::consumeContainedInsertPluginSlots()
{
    std::vector<juce::AudioProcessor*> processors;
    processors.reserve (insertFaultNotices.size());
    for (auto& notice : insertFaultNotices)
    {
        if (notice.state.load (std::memory_order_acquire) != 2)
            continue;
        processors.push_back (
            notice.processor.load (std::memory_order_relaxed));
        notice.processor.store (nullptr, std::memory_order_relaxed);
        notice.state.store (0, std::memory_order_release);
    }
    std::vector<juce::String> result;
    if (processors.empty())
        return result;
    const juce::ScopedLock lock (controlLock);
    for (auto* processor : processors)
    {
        const auto slot = std::find_if (
            insertPlugins.begin(), insertPlugins.end(),
            [processor] (const auto& item)
        {
            return item.instance.get() == processor;
        });
        if (slot != insertPlugins.end())
            result.push_back (slot->slotId);
    }
    return result;
}

void AudioEngine::createOfflineInstrumentInstance (
    juce::PluginDescription description,
    juce::String stateBase64,
    double sampleRate,
    int blockSize,
    std::function<void (std::unique_ptr<juce::AudioPluginInstance>,
                        juce::String)> completion)
{
    const auto alive = pluginHostAlive;
    pluginFormatManager.createPluginInstanceAsync (
        description, juce::jmax (1.0, sampleRate), juce::jmax (16, blockSize),
        [alive, description, stateBase64 = std::move (stateBase64),
         completion = std::move (completion)]
        (std::unique_ptr<juce::AudioPluginInstance> instance,
         const juce::String& error) mutable
        {
            if (! alive->load())
                return;
            if (instance == nullptr)
            {
                if (completion)
                    completion (nullptr, error.isNotEmpty()
                        ? error : "The VST3 freeze instance could not be created.");
                return;
            }
            if (! description.isInstrument)
            {
                if (completion)
                    completion (nullptr, "The selected VST3 is not an instrument.");
                return;
            }
            if (stateBase64.isNotEmpty())
            {
                juce::MemoryBlock state;
                if (! state.fromBase64Encoding (stateBase64))
                {
                    if (completion)
                        completion (nullptr, "The stored VST3 state is damaged.");
                    return;
                }
                instance->setStateInformation (state.getData(),
                                               static_cast<int> (state.getSize()));
            }
            if (completion)
                completion (std::move (instance), {});
        });
}

void AudioEngine::unloadInstrumentPlugin()
{
    pluginLoadRequest.fetch_add (1);
    {
        const juce::ScopedLock lock (controlLock);
        instrumentPluginLoaded.store (false);
        instrumentPluginProcessor.store (nullptr, std::memory_order_release);
        if (instrumentPlugin != nullptr)
        {
            instrumentPlugin->removeListener (this);
            retiredPlugins.push_back (std::move (instrumentPlugin));
        }
        instrumentPluginTrackId.clear();
        instrumentPluginName.clear();
        instrumentPluginBypassed.store (false, std::memory_order_release);
        instrumentPluginOutputMix.store (1.0f, std::memory_order_release);
        reportedInstrumentPluginLatencySamples.store (0,
                                                       std::memory_order_release);
        instrumentPluginLatencyChangePending.store (false,
                                                     std::memory_order_release);
        pluginTailSamplesRemaining.store (0);
        requestRuntimeReset();
        markRenderStateDirty();
    }
}

bool AudioEngine::hasInstrumentPlugin() const noexcept
{
    return instrumentPluginLoaded.load();
}

juce::String AudioEngine::getInstrumentPluginTrackId() const
{
    const juce::ScopedLock lock (controlLock);
    return instrumentPluginTrackId;
}

juce::String AudioEngine::getInstrumentPluginName() const
{
    const juce::ScopedLock lock (controlLock);
    return instrumentPluginName;
}

juce::String AudioEngine::captureInstrumentPluginStateBase64()
{
    beginPluginControl();
    const juce::ScopedLock lock (controlLock);
    if (instrumentPlugin == nullptr)
    {
        endPluginControl();
        return {};
    }
    juce::MemoryBlock state;
    instrumentPlugin->getStateInformation (state);
    const auto encoded = state.toBase64Encoding();
    endPluginControl();
    return encoded;
}

juce::Result AudioEngine::applyInstrumentPluginStateBase64 (
    const juce::String& stateBase64)
{
    juce::MemoryBlock state;
    if (stateBase64.isEmpty() || ! state.fromBase64Encoding (stateBase64))
        return juce::Result::fail ("The plug-in state payload is damaged.");

    beginPluginControl();
    const juce::ScopedLock lock (controlLock);
    if (instrumentPlugin == nullptr)
    {
        endPluginControl();
        return juce::Result::fail ("No live instrument plug-in is loaded.");
    }
    instrumentPlugin->setStateInformation (
        state.getData(), static_cast<int> (state.getSize()));
    reportedInstrumentPluginLatencySamples.store (
        juce::jmax (0, instrumentPlugin->getLatencySamples()),
        std::memory_order_release);
    instrumentPluginLatencyChangePending.store (
        true, std::memory_order_release);
    pluginNeedsAllNotesOff.store (true, std::memory_order_release);
    requestRuntimeReset();
    endPluginControl();
    return juce::Result::ok();
}

std::vector<AudioEngine::PluginParameterDescriptor>
AudioEngine::getInstrumentPluginParameters()
{
    std::vector<PluginParameterDescriptor> result;
    beginPluginControl();
    const juce::ScopedLock lock (controlLock);
    if (instrumentPlugin != nullptr)
    {
        const auto parameters = instrumentPlugin->getParameters();
        result.reserve (static_cast<std::size_t> (parameters.size()));
        for (int index = 0; index < parameters.size(); ++index)
        {
            const auto* parameter = parameters[index];
            if (parameter == nullptr)
                continue;
            result.push_back ({
                stableParameterIdFor (*parameter, index),
                parameter->getName (256), parameter->getLabel(),
                parameter->getDefaultValue(), parameter->getValue(),
                parameter->getNumSteps(), parameter->isAutomatable(),
                parameter->isDiscrete(),
                parameter->isOrientationInverted() });
        }
    }
    endPluginControl();
    return result;
}

bool AudioEngine::setInstrumentPluginParameterValue (
    const juce::String& stableId,
    float normalisedValue)
{
    auto changed = false;
    beginPluginControl();
    const juce::ScopedLock lock (controlLock);
    if (instrumentPlugin != nullptr)
    {
        const auto parameters = instrumentPlugin->getParameters();
        for (int index = 0; index < parameters.size(); ++index)
        {
            auto* parameter = parameters[index];
            if (parameter == nullptr
                || stableParameterIdFor (*parameter, index) != stableId)
                continue;
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (
                juce::jlimit (0.0f, 1.0f, normalisedValue));
            parameter->endChangeGesture();
            changed = true;
            break;
        }
    }
    endPluginControl();
    return changed;
}

int AudioEngine::getInstrumentPluginLatencySamples() const
{
    return reportedInstrumentPluginLatencySamples.load (
        std::memory_order_acquire);
}

bool AudioEngine::consumeInstrumentPluginLatencyChange (
    int& latencySamples) noexcept
{
    if (! instrumentPluginLatencyChangePending.exchange (
            false, std::memory_order_acq_rel))
        return false;
    latencySamples = reportedInstrumentPluginLatencySamples.load (
        std::memory_order_acquire);
    return true;
}

void AudioEngine::setInstrumentPluginBypassed (
    bool shouldBeBypassed) noexcept
{
    const auto wasBypassed = instrumentPluginBypassed.exchange (
        shouldBeBypassed, std::memory_order_acq_rel);
    if (wasBypassed != shouldBeBypassed)
        pluginNeedsAllNotesOff.store (true, std::memory_order_release);
}

juce::AudioProcessorEditor* AudioEngine::getInstrumentPluginEditor()
{
    beginPluginControl();
    const juce::ScopedLock lock (controlLock);
    auto* editor = instrumentPlugin != nullptr
                       ? instrumentPlugin->createEditorAndMakeActive() : nullptr;
    endPluginControl();
    return editor;
}

void AudioEngine::prepareInstrumentPlugin (juce::AudioPluginInstance& plugin,
                                           double sampleRate,
                                           int blockSize)
{
    plugin.setPlayHead (&instrumentPlayHead);
    plugin.setRateAndBufferSizeDetails (sampleRate, blockSize);
    plugin.prepareToPlay (sampleRate, blockSize);
}

void AudioEngine::setProjectTimeline (double timelineSampleRate,
                                      juce::int64 lengthInTimelineSamples) noexcept
{
    const auto oldRate = projectTimelineSampleRate.load();
    const auto oldPositionSeconds = transport::toSeconds (
        timelinePositionUnits.load(), oldRate);
    const auto newRate = juce::jmax (1.0, timelineSampleRate);
    const auto newLength = transport::fromProjectSamples (juce::jmax (
        static_cast<juce::int64> (0), lengthInTimelineSamples));
    projectTimelineSampleRate.store (newRate);
    projectLengthUnits.store (newLength);
    timelinePositionUnits.store (transport::clamp (
        transport::fromSeconds (oldPositionSeconds, newRate), newLength));
}

void AudioEngine::setPlaybackLoop (juce::int64 startSamples,
                                   juce::int64 endSamples,
                                   bool enabled) noexcept
{
    const auto start = juce::jmax (static_cast<juce::int64> (0), startSamples);
    const auto end = juce::jmax (start, endSamples);
    playbackLoopStartUnits.store (transport::fromProjectSamples (start),
                                  std::memory_order_release);
    playbackLoopEndUnits.store (transport::fromProjectSamples (end),
                                std::memory_order_release);
    playbackLoopEnabled.store (enabled && end > start,
                               std::memory_order_release);
}

std::unique_ptr<AudioEngine::RenderSnapshot> AudioEngine::buildRenderSnapshot()
{
    if (preparedBlockSize <= 0 || preparedSampleRate <= 0.0)
        return nullptr;

    auto state = std::make_unique<RenderSnapshot>();
    state->generation = nextRenderGeneration++;
    state->blockSize = preparedBlockSize;
    state->sampleRate = preparedSampleRate;
    state->midiTracks = midiTracks;
    state->tempoPoints = tempoPoints;
    state->timeSignatures = timeSignatures;
    state->mixerPlan = mixerPlanConfiguration;
    state->monitorPlan = monitorPlanConfiguration;
    state->masterGainTarget = masterGain.load();
    state->masterMutedTarget = masterMuted.load();
    state->appliedRuntimeReset = runtimeResetGeneration.load();

    state->tracks = tracks;

    state->trackDelayStates.reserve (delayConfiguration.size());
    for (const auto& delay : delayConfiguration)
    {
        TrackDelayState runtime;
        runtime.id = delay.id;
        runtime.delaySamples = delay.delaySamples;
        runtime.line.prepare (2, runtime.delaySamples, preparedBlockSize);
        state->trackDelayStates.push_back (std::move (runtime));
    }

    state->mixerRuntime.reserve (mixerConfiguration.size());
    for (const auto& channel : mixerConfiguration)
    {
        MixerRuntime runtime;
        runtime.settings = channel;
        const auto channels = juce::jmax (
            2, juce::jmax (channel.inputChannels,
                           juce::jmax (channel.outputChannels,
                                      channel.sidechainChannels)));
        runtime.preFader.setSize (channels, preparedBlockSize,
                                  false, true, true);
        runtime.postFader.setSize (channels, preparedBlockSize,
                                   false, true, true);
        runtime.sidechain.setSize (channels, preparedBlockSize,
                                   false, true, true);
        const auto meter = std::find_if (
            meterPublications.begin(), meterPublications.end(),
            [&channel] (const auto& candidate) { return candidate.id == channel.id; });
        if (meter != meterPublications.end())
            runtime.meter = meter->values;
        state->mixerRuntime.push_back (std::move (runtime));
    }

    state->mixerRuntimeRoutes.reserve (state->mixerPlan.routes.size());
    std::vector<bool> usedDelayEdges (delayPlanConfiguration.edges.size(), false);
    for (const auto& route : state->mixerPlan.routes)
    {
        const auto source = std::find_if (
            state->mixerPlan.channels.begin(), state->mixerPlan.channels.end(),
            [&route] (const auto& channel) { return channel.id == route.source; });
        if (source == state->mixerPlan.channels.end())
            continue;

        MixerRuntimeRoute runtimeRoute;
        runtimeRoute.sourceIndex = static_cast<std::size_t> (
            std::distance (state->mixerPlan.channels.begin(), source));
        runtimeRoute.destinationIsMaster = route.destination == mixer::masterId;
        if (! runtimeRoute.destinationIsMaster)
        {
            const auto destination = std::find_if (
                state->mixerPlan.channels.begin(), state->mixerPlan.channels.end(),
                [&route] (const auto& channel)
                {
                    return channel.id == route.destination;
                });
            if (destination == state->mixerPlan.channels.end())
                continue;
            runtimeRoute.destinationIndex = static_cast<std::size_t> (
                std::distance (state->mixerPlan.channels.begin(), destination));
        }
        runtimeRoute.kind = route.kind;
        runtimeRoute.gain = route.gain;
        runtimeRoute.preFader = route.preFader;
        runtimeRoute.enabled = route.enabled;
        const auto pdcDestination = route.destination == mixer::masterId
            ? std::string { "__triumph_master__" } : route.destination;
        const auto pdcKind = route.kind == mixer::RouteKind::sidechain
            ? plugins::EdgeKind::sidechain
            : route.kind == mixer::RouteKind::send
                ? plugins::EdgeKind::send : plugins::EdgeKind::audio;
        for (std::size_t index = 0;
             index < delayPlanConfiguration.edges.size(); ++index)
        {
            const auto& edge = delayPlanConfiguration.edges[index];
            if (usedDelayEdges[index] || edge.sourceId != route.source
                || edge.destinationId != pdcDestination
                || edge.kind != pdcKind)
                continue;
            runtimeRoute.delaySamples = static_cast<int> (
                juce::jlimit<std::int64_t> (0, 262143,
                                            edge.delaySamples));
            usedDelayEdges[index] = true;
            break;
        }
        runtimeRoute.delayLine.prepare (2, runtimeRoute.delaySamples,
                                        preparedBlockSize);
        runtimeRoute.gainBuffer.setSize (2, preparedBlockSize,
                                         false, true, true);
        state->mixerRuntimeRoutes.push_back (std::move (runtimeRoute));
    }

    for (const auto& configuredLane : automationConfiguration)
    {
        if (! configuredLane.enabled || configuredLane.points.empty())
            continue;
        auto points = automation::normalised (configuredLane.points);
        if (configuredLane.targetId == mixer::masterId
            && configuredLane.parameterId == "gain")
        {
            state->masterGainAutomation = std::move (points);
            continue;
        }
        if (configuredLane.targetId == mixer::masterId
            && configuredLane.parameterId == "pan")
        {
            state->masterPanAutomation = std::move (points);
            continue;
        }
        auto* runtime = findMixerRuntime (*state, configuredLane.targetId);
        if (runtime == nullptr)
            continue;
        if (configuredLane.parameterId == "pan")
            runtime->panAutomation = std::move (points);
        else if (configuredLane.parameterId == "gain")
            runtime->gainAutomation = std::move (points);
    }

    auto monitorOutputChannels = 2;
    for (const auto& path : state->monitorPlan.paths)
        monitorOutputChannels = juce::jmax (
            monitorOutputChannels,
            static_cast<int> (path.outputStartChannel
                              + path.outputChannels));
    state->scratchBuffer.setSize (2, preparedBlockSize, false, true, true);
    state->inputMonitorBuffer.setSize (2, preparedBlockSize, false, true, true);
    state->playbackMixBuffer.setSize (monitorOutputChannels,
                                      preparedBlockSize,
                                      false, true, true);
    state->masterGainSmoother.reset (preparedSampleRate, 20.0,
                                     state->masterGainTarget);
    state->controlRoomGainSmoother.reset (
        preparedSampleRate, 10.0, controlRoomGain.load());
    state->pluginOutputMixSmoother.reset (
        preparedSampleRate, 10.0,
        instrumentPluginOutputMix.load (std::memory_order_acquire));

    state->instrumentPlugin = instrumentPlugin.get();
    state->instrumentPluginTrackId = instrumentPluginTrackId;
    if (state->instrumentPlugin != nullptr)
    {
        const auto channels = juce::jmax (
            2, juce::jmax (state->instrumentPlugin->getTotalNumInputChannels(),
                           state->instrumentPlugin->getTotalNumOutputChannels()));
        state->pluginAudioBuffer.setSize (channels, preparedBlockSize,
                                          false, true, true);
        std::size_t midiEventCapacity = 4096;
        for (const auto& track : state->midiTracks)
            midiEventCapacity += track.notes.size() * 64;
        state->pluginMidiBuffer.ensureSize (midiEventCapacity);
        state->pluginProcessMidiBuffer.ensureSize (midiEventCapacity);

        const auto parameters = state->instrumentPlugin->getParameters();
        for (const auto& lane : automationConfiguration)
        {
            if (! lane.enabled || lane.points.empty()
                || lane.targetId != state->instrumentPluginTrackId
                || ! lane.parameterId.startsWith ("plugin:"))
                continue;
            for (int index = 0; index < parameters.size(); ++index)
            {
                auto* parameter = parameters[index];
                if (parameter == nullptr || ! parameter->isAutomatable()
                    || stableParameterIdFor (*parameter, index)
                           != lane.parameterId)
                    continue;
                state->pluginAutomation.push_back ({
                    parameter, automation::normalised (lane.points), -1.0f });
                break;
            }
        }
    }

    state->insertMidiBuffer.ensureSize (4096);
    state->insertProcessMidiBuffer.ensureSize (4096);
    state->insertPlugins.reserve (insertPlugins.size());
    for (const auto& hosted : insertPlugins)
    {
        if (hosted.instance == nullptr)
            continue;
        InsertPluginRuntime runtime;
        runtime.slotId = hosted.slotId;
        runtime.ownerId = hosted.ownerId;
        runtime.order = hosted.order;
        runtime.instance = hosted.instance.get();
        runtime.bypassed = hosted.bypassed;
        runtime.dryBuffer.setSize (2, preparedBlockSize,
                                   false, true, true);
        const auto parameters = hosted.instance->getParameters();
        for (const auto& lane : automationConfiguration)
        {
            if (! lane.enabled || lane.points.empty())
                continue;
            auto parameterId = lane.parameterId;
            auto matches = lane.targetId == hosted.slotId;
            const auto qualifiedPrefix = "plugin:" + hosted.slotId + ":";
            if (parameterId.startsWith (qualifiedPrefix))
            {
                parameterId = parameterId.substring (
                    qualifiedPrefix.length());
                if (! parameterId.startsWith ("plugin:"))
                    parameterId = "plugin:" + parameterId;
                matches = true;
            }
            if (lane.targetId.startsWith (qualifiedPrefix))
            {
                parameterId = lane.targetId.substring (
                    qualifiedPrefix.length());
                if (! parameterId.startsWith ("plugin:"))
                    parameterId = "plugin:" + parameterId;
                matches = true;
            }
            if (! matches)
                continue;
            for (int index = 0; index < parameters.size(); ++index)
            {
                auto* parameter = parameters[index];
                if (parameter == nullptr || ! parameter->isAutomatable()
                    || stableParameterIdFor (*parameter, index)
                           != parameterId)
                    continue;
                runtime.automation.push_back ({
                    parameter, automation::normalised (lane.points), -1.0f });
                break;
            }
        }
        state->insertPlugins.push_back (std::move (runtime));
    }
    std::stable_sort (
        state->insertPlugins.begin(), state->insertPlugins.end(),
        [] (const auto& a, const auto& b)
    {
        return a.ownerId == b.ownerId ? a.order < b.order
                                      : a.ownerId < b.ownerId;
    });

    return state;
}

void AudioEngine::markRenderStateDirty()
{
    renderStateDirty = true;
    if (renderUpdateDepth == 0)
        publishRenderSnapshot();
}

void AudioEngine::publishRenderSnapshot()
{
    if (renderUpdateDepth > 0 || preparedBlockSize <= 0
        || preparedSampleRate <= 0.0)
        return;

    const auto buildStarted = std::chrono::steady_clock::now();
    auto prepared = buildRenderSnapshot();
    if (prepared == nullptr)
        return;

    renderStates.publish (std::move (prepared));
    const auto elapsed = static_cast<std::uint64_t> (
        std::max<std::int64_t> (
            0,
            std::chrono::duration_cast<std::chrono::nanoseconds> (
                std::chrono::steady_clock::now() - buildStarted).count()));
    realtimeTelemetry.recordGraphBuild (
        elapsed, static_cast<std::uint64_t> (renderStates.retiredCount()));
    renderStateDirty = false;
    reclaimRetiredResources();
}

void AudioEngine::reclaimRetiredResources()
{
    renderStates.reclaim();
    if (renderStates.readers() != 0 || renderStates.retiredCount() != 0)
        return;

    for (auto& plugin : retiredPlugins)
        if (plugin != nullptr)
            plugin->releaseResources();
    retiredPlugins.clear();
}

void AudioEngine::requestRuntimeReset() noexcept
{
    runtimeResetGeneration.fetch_add (1, std::memory_order_release);
}

void AudioEngine::drainParameterCommands (RenderSnapshot& state) noexcept
{
    realtime::ParameterCommand command;
    while (parameterCommands.pop (command))
    {
        if (command.type == realtime::ParameterCommand::Type::masterGain)
            state.masterGainTarget = juce::jlimit (0.0f, 1.25f, command.value);
        else if (command.type == realtime::ParameterCommand::Type::masterMute)
            state.masterMutedTarget = command.value >= 0.5f;
    }

    // Atomics retain the newest target if the bounded queue ever overflows.
    state.masterGainTarget = masterGain.load (std::memory_order_relaxed);
    state.masterMutedTarget = masterMuted.load (std::memory_order_relaxed);
}

void AudioEngine::beginPluginControl() noexcept
{
    pluginControlBusy.store (true, std::memory_order_release);
    while (pluginAudioUsers.load (std::memory_order_acquire) != 0)
        std::this_thread::yield();
}

void AudioEngine::endPluginControl() noexcept
{
    pluginControlBusy.store (false, std::memory_order_release);
}

void AudioEngine::audioProcessorParameterChanged (
    juce::AudioProcessor*, int, float)
{
}

void AudioEngine::audioProcessorChanged (
    juce::AudioProcessor* processor,
    const juce::AudioProcessorListener::ChangeDetails& details)
{
    if (processor == nullptr || ! details.latencyChanged)
        return;
    const auto latency = juce::jmax (0, processor->getLatencySamples());
    if (processor == instrumentPluginProcessor.load (
                         std::memory_order_acquire))
    {
        reportedInstrumentPluginLatencySamples.store (
            latency, std::memory_order_release);
        instrumentPluginLatencyChangePending.store (
            true, std::memory_order_release);
        return;
    }
    for (auto& notice : insertLatencyNotices)
    {
        auto expected = 0;
        if (! notice.state.compare_exchange_strong (
                expected, 1, std::memory_order_acq_rel))
            continue;
        notice.processor.store (processor, std::memory_order_relaxed);
        notice.samples.store (latency, std::memory_order_relaxed);
        notice.state.store (2, std::memory_order_release);
        return;
    }
}

std::size_t AudioEngine::collectPluginParameterEvents (
    std::vector<PluginAutomationRuntime>& lanes,
    juce::int64 blockStartSample,
    int numSamples,
    std::array<PluginParameterEvent, 256>& events) noexcept
{
    auto count = static_cast<std::size_t> (0);
    auto overflowed = false;

    const auto push = [&] (juce::AudioProcessorParameter* parameter,
                           std::uint32_t sampleOffset,
                           float value) noexcept
    {
        if (parameter == nullptr)
            return;
        if (count >= events.size())
        {
            overflowed = true;
            return;
        }
        events[count++] = {
            parameter,
            static_cast<std::uint32_t> (
                juce::jlimit (0, juce::jmax (0, numSamples - 1),
                              static_cast<int> (sampleOffset))),
            juce::jlimit (0.0f, 1.0f, value)
        };
    };

    if (numSamples <= 0)
        return 0;

    for (auto& lane : lanes)
    {
        if (lane.parameter == nullptr || lane.points.empty())
            continue;
        automation::visitParameterEventsForBlock (
            blockStartSample, numSamples, lane.points,
            lane.parameter->getValue(), lane.lastValue,
            [&] (std::uint32_t sampleOffset, float value) noexcept
        {
            push (lane.parameter, sampleOffset, value);
            return true;
        });
    }

    std::stable_sort (events.begin(), events.begin() + count,
                      [] (const auto& left, const auto& right)
    {
        return left.sampleOffset < right.sampleOffset;
    });

    if (overflowed)
        realtimeTelemetry.recordParameterQueueOverflow();
    return count;
}

void AudioEngine::applyPluginParameterEvents (
    std::vector<PluginAutomationRuntime>& lanes,
    const std::array<PluginParameterEvent, 256>& events,
    std::size_t count,
    std::size_t& nextEvent,
    std::uint32_t sampleOffset) noexcept
{
    while (nextEvent < count && events[nextEvent].sampleOffset == sampleOffset)
    {
        auto* parameter = events[nextEvent].parameter;
        const auto value = events[nextEvent].value;
        if (parameter != nullptr)
        {
            parameter->setValue (value);
            for (auto& lane : lanes)
                if (lane.parameter == parameter)
                    lane.lastValue = value;
        }
        ++nextEvent;
    }
}

void AudioEngine::copyMidiRange (const juce::MidiBuffer& source,
                                 juce::MidiBuffer& destination,
                                 int startSample,
                                 int numSamples)
{
    destination.clear();
    if (numSamples <= 0)
        return;
    const auto endSample = startSample + numSamples;
    for (const auto metadata : source)
    {
        const auto position = metadata.samplePosition;
        if (position < startSample || position >= endSample)
            continue;
        destination.addEvent (metadata.getMessage(),
                              position - startSample);
    }
}

void AudioEngine::processPluginBlockWithAutomation (
    juce::AudioPluginInstance& plugin,
    juce::AudioBuffer<float>& audio,
    juce::MidiBuffer& midi,
    juce::MidiBuffer& processMidi,
    std::vector<PluginAutomationRuntime>& automation,
    std::array<PluginParameterEvent, 256>& events,
    int numSamples,
    juce::int64 blockStartSample)
{
    const auto eventCount = collectPluginParameterEvents (
        automation, blockStartSample, numSamples, events);
    auto nextEvent = static_cast<std::size_t> (0);
    auto cursor = 0;

    while (cursor < numSamples)
    {
        applyPluginParameterEvents (
            automation, events, eventCount, nextEvent,
            static_cast<std::uint32_t> (cursor));

        auto nextOffset = numSamples;
        if (nextEvent < eventCount)
            nextOffset = static_cast<int> (events[nextEvent].sampleOffset);
        nextOffset = juce::jlimit (cursor + 1, numSamples, nextOffset);
        const auto count = nextOffset - cursor;

        juce::AudioBuffer<float> processView (
            audio.getArrayOfWritePointers(), audio.getNumChannels(),
            cursor, count);
        copyMidiRange (midi, processMidi, cursor, count);
        plugin.processBlock (processView, processMidi);
        cursor = nextOffset;
    }
}

void AudioEngine::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    {
        const juce::ScopedLock lock (controlLock);
        preparedBlockSize = juce::jmax (1, samplesPerBlockExpected);
        preparedSampleRate = juce::jmax (1.0, sampleRate);
        realtimeTelemetry.recordDeviceRestart (
            preparedBlockSize, sampleRateMilliHz (preparedSampleRate));
        liveMidiCollector.reset (preparedSampleRate);
        for (auto& track : tracks)
        {
            track->pause();
            track->releaseResources();
            track->prepareToPlay (preparedBlockSize, preparedSampleRate);
            track->play();
        }
        beginPluginControl();
        if (instrumentPlugin != nullptr)
        {
            instrumentPlugin->releaseResources();
            prepareInstrumentPlugin (*instrumentPlugin,
                                     preparedSampleRate, preparedBlockSize);
        }
        for (auto& slot : insertPlugins)
            if (slot.instance != nullptr)
            {
                slot.instance->releaseResources();
                prepareInstrumentPlugin (*slot.instance,
                                         preparedSampleRate,
                                         preparedBlockSize);
            }
        endPluginControl();
        renderStateDirty = true;
        publishRenderSnapshot();
    }
    spectrumFifo.reset();
}

void AudioEngine::releaseResources()
{
    {
        const juce::ScopedLock lock (controlLock);
        renderStates.clear();
        reclaimRetiredResources();
        beginPluginControl();
        if (instrumentPlugin != nullptr)
            instrumentPlugin->releaseResources();
        for (auto& slot : insertPlugins)
            if (slot.instance != nullptr)
                slot.instance->releaseResources();
        for (auto& plugin : retiredPlugins)
            if (plugin != nullptr)
                plugin->releaseResources();
        retiredPlugins.clear();
        endPluginControl();
        for (auto& track : tracks)
        {
            track->pause();
            track->releaseResources();
        }
        preparedBlockSize = 0;
        preparedSampleRate = 0.0;
        renderStateDirty = true;
    }
    spectrumFifo.reset();
    parameterCommands.reset();
}

void AudioEngine::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto renderState = renderStates.acquire();
    auto* state = renderState.get();
    CallbackMeasurement measurement (
        realtimeTelemetry, bufferToFill.numSamples,
        state != nullptr ? state->sampleRate : 0.0);

    if (bufferToFill.buffer == nullptr)
        return;

    if (state == nullptr)
    {
        realtimeTelemetry.recordMissingRenderStateBlock();
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    if (lastAudioRenderGeneration != state->generation)
    {
        lastAudioRenderGeneration = state->generation;
        realtimeTelemetry.recordSnapshotSwap (state->generation);
    }

    drainParameterCommands (*state);
    const auto requestedReset = runtimeResetGeneration.load (
        std::memory_order_acquire);
    if (state->appliedRuntimeReset != requestedReset)
    {
        clearTrackDelayBuffers (*state);
        state->metronomeEnvelope = 0.0f;
        state->metronomePhase = 0.0;
        state->appliedRuntimeReset = requestedReset;
    }

    const auto channelCount = bufferToFill.buffer->getNumChannels();
    const auto monitorEnabled = inputMonitoring.load();
    const auto monitorReady = monitorEnabled
                              && channelCount > 0
                              && bufferToFill.numSamples
                                     <= state->inputMonitorBuffer.getNumSamples();

    if (monitorReady)
    {
        state->inputMonitorBuffer.clear();
        const auto captureChannels = juce::jmin (
            channelCount, state->inputMonitorBuffer.getNumChannels());
        for (int channel = 0; channel < captureChannels; ++channel)
        {
            const auto sourceChannel = inputChannelCount.load() == 1 ? 0 : channel;
            state->inputMonitorBuffer.copyFrom (channel,
                                         0,
                                         *bufferToFill.buffer,
                                         sourceChannel,
                                         bufferToFill.startSample,
                                         bufferToFill.numSamples);
        }
    }

    bufferToFill.clearActiveBufferRegion();

    if (bufferToFill.numSamples > state->scratchBuffer.getNumSamples()
        || bufferToFill.numSamples > state->playbackMixBuffer.getNumSamples())
    {
        realtimeTelemetry.recordOversizedAudioBlock();
        return;
    }

    state->playbackMixBuffer.clear();

    const auto transportRunning = playing.load();
    const auto liveNotesActive = liveMidiNoteCount.load() > 0;
    const auto pluginBypassed = instrumentPluginBypassed.load (
        std::memory_order_acquire);
    const auto instrumentTailPending = pluginTailSamplesRemaining.load() > 0;
    const auto insertTailPending =
        insertPluginTailSamplesRemaining.load() > 0;
    const auto pluginTailPending = instrumentTailPending || insertTailPending;
    const auto delayTailPending = hasDelayTailPending (*state);
    const auto pluginWorkPending =
        (instrumentPluginLoaded.load()
         && (liveMidiPending.load() || pluginNeedsAllNotesOff.load()
             || instrumentTailPending))
        || (! state->insertPlugins.empty() && insertTailPending);
    if ((! transportRunning && ! liveNotesActive
            && ! pluginWorkPending && ! delayTailPending)
        || state->sampleRate <= 0.0)
    {
        if (monitorReady)
            addSmoothedInputMonitor (*state, *bufferToFill.buffer,
                                     bufferToFill.startSample,
                                     bufferToFill.numSamples);
        return;
    }

    clearMixerBuffers (*state);

    const auto timelineRate = projectTimelineSampleRate.load();
    const auto blockStartUnits = timelinePositionUnits.load();
    const auto blockStartSeconds = transport::toSeconds (
        blockStartUnits, timelineRate);
    const auto blockStartSample = static_cast<juce::int64> (
        transport::toProjectSamples (blockStartUnits));
    const auto timelineSamplesPerDeviceSample =
        timelineRate / state->sampleRate;

    const auto hasPlugin = state->instrumentPlugin != nullptr
                           && state->pluginAudioBuffer.getNumSamples()
                                  >= bufferToFill.numSamples;
    const auto hasAnyPlugin = hasPlugin || ! state->insertPlugins.empty();
    auto pluginsReady = false;
    if (hasAnyPlugin
        && ! pluginControlBusy.load (std::memory_order_acquire))
    {
        pluginAudioUsers.fetch_add (1, std::memory_order_acq_rel);
        if (! pluginControlBusy.load (std::memory_order_acquire))
            pluginsReady = true;
        else
            pluginAudioUsers.fetch_sub (1, std::memory_order_release);
    }

    for (auto& track : state->tracks)
    {
        const auto report = track->renderTimelineBlock (
            state->scratchBuffer,
            bufferToFill.numSamples,
            blockStartSeconds,
            state->sampleRate,
            transportRunning,
            false);
        if (report.hadStarvation())
            realtimeTelemetry.recordReadAheadStarvationBlock();
        if (pluginsReady)
        {
            state->insertMidiBuffer.clear();
            processInsertPlugins (*state, track->getId(),
                                  state->scratchBuffer,
                                  state->insertMidiBuffer,
                                  bufferToFill.numSamples,
                                  blockStartSample);
        }

        mixSourceIntoMixer (*state, track->getId(), state->scratchBuffer,
                            bufferToFill.numSamples);
    }

    constexpr double twoPi = 6.28318530717958647692;
    const auto blockEndSeconds = blockStartSeconds
        + static_cast<double> (bufferToFill.numSamples) / state->sampleRate;

    state->pluginMidiBuffer.clear();
    const auto pluginReady = hasPlugin && pluginsReady;
    if (pluginReady)
    {
        liveMidiPending.exchange (false);
        liveMidiCollector.removeNextBlockOfMessages (state->pluginMidiBuffer,
                                                     bufferToFill.numSamples);
        if (pluginBypassed)
            state->pluginMidiBuffer.clear();
        if (pluginNeedsAllNotesOff.exchange (false))
            for (int channel = 1; channel <= 16; ++channel)
                state->pluginMidiBuffer.addEvent (
                    juce::MidiMessage::allNotesOff (channel), 0);
    }

    const auto sampleOffsetFor = [&] (double eventSeconds)
    {
        return juce::jlimit (0, bufferToFill.numSamples - 1,
                             static_cast<int> (std::llround (
                                 (eventSeconds - blockStartSeconds)
                                 * state->sampleRate)));
    };

    if (transportRunning)
    {
        for (const auto& track : state->midiTracks)
        {
            const auto routedToPlugin = hasPlugin
                                        && track.id == state->instrumentPluginTrackId;
            if (routedToPlugin)
            {
                if (pluginReady && ! pluginBypassed)
                    for (const auto& note : track.notes)
                    {
                        if (note.startSeconds >= blockStartSeconds
                            && note.startSeconds < blockEndSeconds)
                            state->pluginMidiBuffer.addEvent (
                                juce::MidiMessage::noteOn (
                                                           note.performanceChannel,
                                                           note.noteNumber,
                                                           note.velocity),
                                sampleOffsetFor (note.startSeconds));
                        if (note.endSeconds >= blockStartSeconds
                            && note.endSeconds < blockEndSeconds)
                            state->pluginMidiBuffer.addEvent (
                                juce::MidiMessage::noteOff (
                                                            note.performanceChannel,
                                                            note.noteNumber),
                                sampleOffsetFor (note.endSeconds));
                        for (const auto& expression : note.expression)
                        {
                            if (expression.seconds < blockStartSeconds
                                || expression.seconds >= blockEndSeconds)
                                continue;
                            const auto normalised = juce::jlimit (
                                0.0f, 1.0f, expression.value * 0.5f + 0.5f);
                            auto event = juce::MidiMessage::pitchWheel (
                                note.performanceChannel, 8192);
                            if (expression.type
                                == MidiNotePlayback::Expression::Type::pitch)
                                event = juce::MidiMessage::pitchWheel (
                                    note.performanceChannel,
                                    juce::jlimit (0, 16383, static_cast<int> (
                                        std::lround (normalised * 16383.0f))));
                            else if (expression.type
                                     == MidiNotePlayback::Expression::Type::pressure)
                                event = juce::MidiMessage::channelPressureChange (
                                    note.performanceChannel,
                                    juce::jlimit (0, 127, static_cast<int> (
                                        std::lround (normalised * 127.0f))));
                            else
                                event = juce::MidiMessage::controllerEvent (
                                    note.performanceChannel, 74,
                                    juce::jlimit (0, 127, static_cast<int> (
                                        std::lround (normalised * 127.0f))));
                            state->pluginMidiBuffer.addEvent (
                                event, sampleOffsetFor (expression.seconds));
                        }
                    }
                if (pluginReady && ! pluginBypassed)
                    for (const auto& controller : track.controllers)
                    {
                        if (controller.seconds < blockStartSeconds
                            || controller.seconds >= blockEndSeconds)
                            continue;
                        const auto value = controller.value <= 127
                            ? static_cast<int> (controller.value)
                            : static_cast<int> (std::llround (
                                static_cast<double> (controller.value)
                                * 127.0
                                / static_cast<double> (
                                    std::numeric_limits<std::uint32_t>::max())));
                        state->pluginMidiBuffer.addEvent (
                            juce::MidiMessage::controllerEvent (
                                juce::jlimit (1, 16, controller.channel),
                                juce::jlimit (0, 127, controller.controller),
                                juce::jlimit (0, 127, value)),
                            sampleOffsetFor (controller.seconds));
                    }
                continue;
            }

            state->scratchBuffer.clear();
            for (const auto& note : track.notes)
            {
                const auto noteStart = note.startSeconds;
                const auto noteEnd = note.endSeconds;
                if (noteEnd <= blockStartSeconds || noteStart >= blockEndSeconds)
                    continue;

                for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
                {
                    const auto absoluteTime = blockStartSeconds
                        + static_cast<double> (sample) / state->sampleRate;
                    if (absoluteTime < noteStart || absoluteTime >= noteEnd)
                        continue;

                    const auto absoluteSample = static_cast<juce::int64> (
                        std::llround (absoluteTime * state->sampleRate));
                    const auto value = renderNamedBuiltInInstrument (
                        track.instrumentName,
                        note.noteNumber, note.channel, note.velocity,
                        absoluteTime - noteStart, noteEnd - absoluteTime,
                        absoluteSample, state->sampleRate);
                    state->scratchBuffer.addSample (0, sample, value);
                    if (channelCount > 1)
                        state->scratchBuffer.addSample (1, sample, value);
                }
            }
            mixSourceIntoMixer (*state, track.id, state->scratchBuffer,
                                bufferToFill.numSamples);
        }
    }

    if (! hasPlugin)
    {
        state->scratchBuffer.clear();
        for (int noteNumber = 0; noteNumber < 128; ++noteNumber)
        {
            auto velocity = 0.0f;
            for (int channel = 0; channel < 16; ++channel)
                velocity = juce::jmax (
                    velocity,
                    liveMidiNotes[static_cast<size_t> (channel * 128 + noteNumber)].load());
            auto& phase = liveMidiPhases[static_cast<size_t> (noteNumber)];
            if (velocity <= 0.0f)
            {
                phase = 0.0;
                continue;
            }

            const auto frequency = 440.0 * std::pow (2.0, (noteNumber - 69) / 12.0);
            const auto phaseDelta = twoPi * frequency / state->sampleRate;
            for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
            {
                const auto value = static_cast<float> (std::sin (phase)
                                                        * velocity * 0.12);
                phase += phaseDelta;
                if (phase >= twoPi)
                    phase -= twoPi;
                state->scratchBuffer.addSample (0, sample, value);
                if (channelCount > 1)
                    state->scratchBuffer.addSample (1, sample, value);
            }
        }
        if (! state->midiTracks.empty())
            mixSourceIntoMixer (*state, state->midiTracks.front().id,
                                state->scratchBuffer,
                                bufferToFill.numSamples);
        else
            for (int channel = 0; channel < channelCount; ++channel)
                state->playbackMixBuffer.addFrom (channel, 0,
                                           state->scratchBuffer,
                                           channel, 0,
                                           bufferToFill.numSamples);
    }

    if (pluginReady)
    {
        const auto [beat, internalBpm] = preparedBeatAndBpmAtSeconds (
            blockStartSeconds, state->tempoPoints);
        const auto bpm = activeSyncSource.load (std::memory_order_acquire)
                             == midi::SyncSource::midiClock
                         && externalSyncLocked.load (std::memory_order_acquire)
            ? externalSyncTempoBpm.load (std::memory_order_acquire)
            : internalBpm;
        instrumentPlayHead.update (static_cast<juce::int64> (std::llround (
                                      blockStartSeconds * state->sampleRate)),
                                   blockStartSeconds, beat, bpm,
                                   transportRunning);
        state->pluginAudioBuffer.clear();
        const auto pluginProcessStarted = std::chrono::steady_clock::now();
        auto pluginProcessSucceeded = true;
        try
        {
            processPluginBlockWithAutomation (
                *state->instrumentPlugin, state->pluginAudioBuffer,
                state->pluginMidiBuffer, state->pluginProcessMidiBuffer,
                state->pluginAutomation, state->pluginParameterEvents,
                bufferToFill.numSamples, blockStartSample);
        }
        catch (...)
        {
            pluginProcessSucceeded = false;
            state->pluginAudioBuffer.clear();
            instrumentPluginBypassed.store (true, std::memory_order_release);
            pluginTailSamplesRemaining.store (0, std::memory_order_release);
            pluginContainedExceptions.fetch_add (1,
                                                  std::memory_order_relaxed);
            instrumentContainedExceptions.fetch_add (
                1, std::memory_order_relaxed);
        }
        const auto pluginElapsed = static_cast<std::uint64_t> (
            std::max<std::int64_t> (0,
                std::chrono::duration_cast<std::chrono::nanoseconds> (
                    std::chrono::steady_clock::now()
                        - pluginProcessStarted).count()));
        pluginLastProcessNanoseconds.store (pluginElapsed,
                                            std::memory_order_relaxed);
        auto maximum = pluginMaximumProcessNanoseconds.load (
            std::memory_order_relaxed);
        while (pluginElapsed > maximum
               && ! pluginMaximumProcessNanoseconds.compare_exchange_weak (
                   maximum, pluginElapsed, std::memory_order_relaxed))
        {
        }
        const auto pluginDeadline = static_cast<std::uint64_t> (std::llround (
            static_cast<double> (bufferToFill.numSamples)
                * 1000000000.0 / state->sampleRate));
        if (pluginDeadline > 0 && pluginElapsed > pluginDeadline)
            pluginDeadlineMisses.fetch_add (1, std::memory_order_relaxed);
        if (! pluginBypassed
            && pluginProcessSucceeded
            && state->pluginMidiBuffer.getNumEvents() > 0)
        {
            pluginTailSamplesRemaining.store (plugin::tailSamples (
                plugin::TailAction::noteInput,
                state->instrumentPlugin->getTailLengthSeconds(),
                state->sampleRate));
        }

        const auto pluginTrack = std::find_if (
            state->midiTracks.begin(), state->midiTracks.end(),
            [state] (const auto& track)
        {
            return track.id == state->instrumentPluginTrackId;
        });
        if (pluginTrack != state->midiTracks.end())
        {
            state->scratchBuffer.clear();
            const auto outputChannels =
                state->instrumentPlugin->getTotalNumOutputChannels();
            state->pluginOutputMixSmoother.setTarget (
                pluginBypassed ? 0.0f : 1.0f);
            for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
            {
                const auto outputMix =
                    state->pluginOutputMixSmoother.process();
                const auto left = outputChannels > 0
                                      ? state->pluginAudioBuffer.getSample (0, sample)
                                            * outputMix
                                      : 0.0f;
                const auto right = outputChannels > 1
                                       ? state->pluginAudioBuffer.getSample (1, sample)
                                             * outputMix
                                       : left;
                state->scratchBuffer.addSample (
                    0, sample, left);
                if (channelCount > 1)
                    state->scratchBuffer.addSample (
                        1, sample, right);
            }
            if (pluginsReady)
                processInsertPlugins (*state, pluginTrack->id,
                                      state->scratchBuffer,
                                      state->pluginMidiBuffer,
                                      bufferToFill.numSamples,
                                      blockStartSample);
            mixSourceIntoMixer (*state, pluginTrack->id, state->scratchBuffer,
                                bufferToFill.numSamples);
            instrumentPluginOutputMix.store (
                state->pluginOutputMixSmoother.getCurrent(),
                std::memory_order_release);
        }

        if (! transportRunning && ! liveNotesActive)
            pluginTailSamplesRemaining.store (juce::jmax (
                static_cast<juce::int64> (0),
                pluginTailSamplesRemaining.load() - bufferToFill.numSamples));

    }

    processMixerGraph (*state, state->playbackMixBuffer,
                       bufferToFill.numSamples,
                       blockStartSample, timelineSamplesPerDeviceSample,
                       pluginsReady);
    if (! transportRunning && ! liveNotesActive)
        insertPluginTailSamplesRemaining.store (juce::jmax (
            static_cast<juce::int64> (0),
            insertPluginTailSamplesRemaining.load()
                - bufferToFill.numSamples));
    if (pluginsReady)
        pluginAudioUsers.fetch_sub (1, std::memory_order_release);
    if (transportRunning && metronomeEnabled.load())
        renderMetronome (*state, state->playbackMixBuffer,
                         bufferToFill.numSamples,
                         blockStartSeconds);
    applySmoothedMasterGain (*state, state->playbackMixBuffer,
                             0,
                             bufferToFill.numSamples,
                             blockStartSample,
                             timelineSamplesPerDeviceSample);
    if (state->monitorPlan.paths.empty())
    {
        const auto channels = juce::jmin (
            channelCount, state->playbackMixBuffer.getNumChannels());
        for (int channel = 0; channel < channels; ++channel)
            bufferToFill.buffer->addFrom (
                channel, bufferToFill.startSample,
                state->playbackMixBuffer, channel, 0,
                bufferToFill.numSamples);
    }
    else
    {
        for (const auto& path : state->monitorPlan.paths)
        {
            const juce::AudioBuffer<float>* source =
                &state->playbackMixBuffer;
            if (path.source != mixer::masterId)
            {
                const auto runtime = std::find_if (
                    state->mixerRuntime.begin(), state->mixerRuntime.end(),
                    [&path] (const auto& candidate)
                {
                    return candidate.settings.id.toStdString() == path.source;
                });
                if (runtime == state->mixerRuntime.end())
                    continue;
                source = &runtime->postFader;
            }
            auto pathGain = path.muted ? 0.0f : path.gain;
            if (path.dimmed)
                pathGain *= 0.25f;
            if (path.role == mixer::MonitorRole::controlRoom)
            {
                if (controlRoomMuted.load (std::memory_order_acquire))
                    pathGain = 0.0f;
                else
                    pathGain *= controlRoomGain.load (
                        std::memory_order_acquire);
                if (controlRoomDimmed.load (std::memory_order_acquire))
                    pathGain *= 0.25f;
            }
            const auto availableOutputs = juce::jmax (
                0, channelCount - static_cast<int> (path.outputStartChannel));
            const auto channels = std::min (
                { static_cast<int> (path.outputChannels), availableOutputs,
                  source->getNumChannels() });
            for (int channel = 0; channel < channels; ++channel)
                bufferToFill.buffer->addFrom (
                    static_cast<int> (path.outputStartChannel) + channel,
                    bufferToFill.startSample, *source, channel, 0,
                    bufferToFill.numSamples, pathGain);
        }
    }
    if (monitorReady)
        addSmoothedInputMonitor (*state, *bufferToFill.buffer,
                                 bufferToFill.startSample,
                                 bufferToFill.numSamples);

    for (auto& delay : state->trackDelayStates)
    {
        if (transportRunning || liveNotesActive || pluginTailPending)
            delay.tailSamplesRemaining = delay.delaySamples;
        else
            delay.tailSamplesRemaining = juce::jmax (
                static_cast<juce::int64> (0),
                delay.tailSamplesRemaining - bufferToFill.numSamples);
    }
    for (auto& route : state->mixerRuntimeRoutes)
    {
        if (transportRunning || liveNotesActive || pluginTailPending)
            route.tailSamplesRemaining = route.delaySamples;
        else
            route.tailSamplesRemaining = juce::jmax (
                static_cast<juce::int64> (0),
                route.tailSamplesRemaining - bufferToFill.numSamples);
    }

    if (! transportRunning)
        return;

    const auto nextPosition = blockStartUnits
        + transport::advanceForDeviceFrames (bufferToFill.numSamples,
                                             timelineRate,
                                             state->sampleRate);
    const auto projectEnd = projectLengthUnits.load();
    if (recordingTransportActive.load())
    {
        timelinePositionUnits.store (nextPosition);
        projectLengthUnits.store (juce::jmax (projectEnd, nextPosition));
    }
    else
    {
        const auto loopEnabled = playbackLoopEnabled.load (
            std::memory_order_acquire);
        const auto loopStart = playbackLoopStartUnits.load (
            std::memory_order_acquire);
        const auto loopEnd = playbackLoopEndUnits.load (
            std::memory_order_acquire);
        if (loopEnabled && loopEnd > loopStart && nextPosition >= loopEnd)
        {
            const auto overflow = nextPosition - loopEnd;
            timelinePositionUnits.store (juce::jmin (loopStart + overflow,
                                                     loopEnd));
            pluginNeedsAllNotesOff.store (true);
            requestRuntimeReset();
        }
        else
        {
            timelinePositionUnits.store (juce::jmin (nextPosition, projectEnd));
        }
        if ((! loopEnabled || loopEnd <= loopStart) && nextPosition >= projectEnd)
            playing.store (false);
    }
}

AudioEngine::TrackDelayState* AudioEngine::findTrackDelayState (
    RenderSnapshot& state,
    const juce::String& trackId) noexcept
{
    const auto match = std::find_if (
        state.trackDelayStates.begin(), state.trackDelayStates.end(),
        [&trackId] (const auto& delay) { return delay.id == trackId; });
    return match != state.trackDelayStates.end() ? &*match : nullptr;
}

void AudioEngine::mixTrackWithCompensation (
    RenderSnapshot& renderState,
    juce::AudioBuffer<float>& destination,
    int destinationStartSample,
    const juce::AudioBuffer<float>& source,
    const juce::String& trackId,
    int numSamples) noexcept
{
    const auto channels = juce::jmin (destination.getNumChannels(),
                                      source.getNumChannels());
    auto* state = findTrackDelayState (renderState, trackId);
    if (state == nullptr || state->delaySamples <= 0
        || ! state->line.isPrepared())
    {
        for (int channel = 0; channel < channels; ++channel)
            destination.addFrom (channel, destinationStartSample,
                                 source, channel, 0, numSamples);
        return;
    }

    std::array<const float*, 2> sourceChannels {};
    std::array<float*, 2> destinationChannels {};
    for (int channel = 0; channel < juce::jmin (2, channels); ++channel)
    {
        sourceChannels[static_cast<std::size_t> (channel)] =
            source.getReadPointer (channel);
        destinationChannels[static_cast<std::size_t> (channel)] =
            destination.getWritePointer (channel, destinationStartSample);
    }
    state->line.processAdd (sourceChannels.data(), destinationChannels.data(),
                            juce::jmin (2, channels), numSamples);
}

void AudioEngine::clearTrackDelayBuffers (RenderSnapshot& renderState) noexcept
{
    for (auto& state : renderState.trackDelayStates)
    {
        state.line.clear();
        state.tailSamplesRemaining = 0;
    }
    for (auto& route : renderState.mixerRuntimeRoutes)
    {
        route.delayLine.clear();
        route.tailSamplesRemaining = 0;
    }
}

bool AudioEngine::hasDelayTailPending (const RenderSnapshot& renderState) const noexcept
{
    return std::any_of (renderState.trackDelayStates.begin(),
                        renderState.trackDelayStates.end(),
                        [] (const auto& state)
    {
        return state.tailSamplesRemaining > 0;
    }) || std::any_of (renderState.mixerRuntimeRoutes.begin(),
                       renderState.mixerRuntimeRoutes.end(),
                       [] (const auto& route)
    {
        return route.tailSamplesRemaining > 0;
    });
}

AudioEngine::MixerRuntime* AudioEngine::findMixerRuntime (
    RenderSnapshot& state,
    const juce::String& channelId) noexcept
{
    const auto found = std::find_if (
        state.mixerRuntime.begin(), state.mixerRuntime.end(),
        [&channelId] (const auto& runtime)
    {
        return runtime.settings.id == channelId;
    });
    return found != state.mixerRuntime.end() ? &*found : nullptr;
}

void AudioEngine::clearMixerBuffers (RenderSnapshot& state) noexcept
{
    for (auto& runtime : state.mixerRuntime)
    {
        runtime.preFader.clear();
        runtime.postFader.clear();
        runtime.sidechain.clear();
        runtime.sidechainReduction *= 0.82f;
    }
}

void AudioEngine::mixSourceIntoMixer (
    RenderSnapshot& state,
    const juce::String& trackId,
    const juce::AudioBuffer<float>& source,
    int numSamples) noexcept
{
    if (auto* runtime = findMixerRuntime (state, trackId))
    {
        mixTrackWithCompensation (state, runtime->preFader, 0, source,
                                  trackId, numSamples);
        return;
    }
    mixTrackWithCompensation (state, state.playbackMixBuffer, 0, source,
                              trackId, numSamples);
}

void AudioEngine::processInsertPlugins (
    RenderSnapshot& state, const juce::String& ownerId,
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
    int numSamples, juce::int64 blockStartSample) noexcept
{
    for (auto& slot : state.insertPlugins)
    {
        if (slot.ownerId != ownerId || slot.instance == nullptr
            || slot.bypassed || numSamples <= 0
            || slot.dryBuffer.getNumSamples() < numSamples)
            continue;
        const auto channels = juce::jmin (
            2, juce::jmin (buffer.getNumChannels(),
                           slot.dryBuffer.getNumChannels()));
        for (int channel = 0; channel < channels; ++channel)
            slot.dryBuffer.copyFrom (channel, 0, buffer, channel, 0,
                                     numSamples);
        auto hasInput = midi.getNumEvents() > 0;
        for (int channel = 0; channel < channels && ! hasInput; ++channel)
            hasInput = slot.dryBuffer.getMagnitude (
                channel, 0, numSamples) > 0.0000001f;
        const auto started = std::chrono::steady_clock::now();
        auto succeeded = true;
        try
        {
            processPluginBlockWithAutomation (
                *slot.instance, buffer, midi, state.insertProcessMidiBuffer,
                slot.automation, state.pluginParameterEvents,
                numSamples, blockStartSample);
        }
        catch (...)
        {
            succeeded = false;
            slot.bypassed = true;
            for (int channel = 0; channel < channels; ++channel)
                buffer.copyFrom (channel, 0, slot.dryBuffer, channel, 0,
                                 numSamples);
            pluginContainedExceptions.fetch_add (1,
                                                  std::memory_order_relaxed);
            for (auto& notice : insertFaultNotices)
            {
                auto expected = 0;
                if (! notice.state.compare_exchange_strong (
                        expected, 1, std::memory_order_acq_rel))
                    continue;
                notice.processor.store (slot.instance,
                                        std::memory_order_relaxed);
                notice.samples.store (0, std::memory_order_relaxed);
                notice.state.store (2, std::memory_order_release);
                break;
            }
        }
        const auto elapsed = static_cast<std::uint64_t> (
            std::max<std::int64_t> (0,
                std::chrono::duration_cast<std::chrono::nanoseconds> (
                    std::chrono::steady_clock::now() - started).count()));
        pluginLastProcessNanoseconds.store (elapsed,
                                            std::memory_order_relaxed);
        auto maximum = pluginMaximumProcessNanoseconds.load (
            std::memory_order_relaxed);
        while (elapsed > maximum
               && ! pluginMaximumProcessNanoseconds.compare_exchange_weak (
                   maximum, elapsed, std::memory_order_relaxed))
        {
        }
        const auto deadline = static_cast<std::uint64_t> (std::llround (
            static_cast<double> (numSamples) * 1000000000.0
            / state.sampleRate));
        if (deadline > 0 && elapsed > deadline)
            pluginDeadlineMisses.fetch_add (1, std::memory_order_relaxed);
        if (succeeded && hasInput)
            insertPluginTailSamplesRemaining.store (
                juce::jmax (insertPluginTailSamplesRemaining.load(),
                    plugin::tailSamples (
                        plugin::TailAction::noteInput,
                        slot.instance->getTailLengthSeconds(),
                        state.sampleRate)),
                std::memory_order_release);
    }
}

void AudioEngine::processMixerGraph (RenderSnapshot& state,
                                     juce::AudioBuffer<float>& master,
                                     int numSamples,
                                     juce::int64 blockStartSample,
                                     double timelineSamplesPerDeviceSample,
                                     bool pluginsReady) noexcept
{
    if (! state.mixerPlan.valid || state.mixerRuntime.empty())
        return;

    const auto anyTrackSolo = std::any_of (
        state.mixerRuntime.begin(), state.mixerRuntime.end(), [] (const auto& runtime)
    {
        return runtime.settings.kind == mixer::ChannelKind::track
               && runtime.settings.solo;
    });
    const auto anyAuxSolo = std::any_of (
        state.mixerRuntime.begin(), state.mixerRuntime.end(), [] (const auto& runtime)
    {
        return runtime.settings.kind != mixer::ChannelKind::track
               && runtime.settings.solo;
    });

    for (const auto graphIndex : state.mixerPlan.processOrder)
    {
        if (graphIndex >= state.mixerRuntime.size())
            continue;
        auto* runtime = &state.mixerRuntime[graphIndex];
        if (runtime->preFader.getNumSamples() < numSamples)
            continue;

        runtime->postFader.makeCopyOf (runtime->preFader, true);
        if (pluginsReady)
        {
            state.insertMidiBuffer.clear();
            processInsertPlugins (state, runtime->settings.id,
                                  runtime->postFader,
                                  state.insertMidiBuffer, numSamples,
                                  blockStartSample);
        }
        auto reductionPeak = 0.0f;
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto control = juce::jmax (
                std::abs (runtime->sidechain.getSample (0, sample)),
                std::abs (runtime->sidechain.getSample (1, sample)));
            const auto duckGain = mixer::sidechainDuckGain (control);
            reductionPeak = juce::jmax (reductionPeak, 1.0f - duckGain);
            if (control > 0.00001f)
                for (int channel = 0; channel < 2; ++channel)
                    runtime->postFader.setSample (
                        channel, sample,
                        runtime->postFader.getSample (channel, sample) * duckGain);
        }
        runtime->sidechainReduction = juce::jmax (
            runtime->sidechainReduction, reductionPeak);

        const auto& settings = runtime->settings;
        const auto soloAllows = settings.kind == mixer::ChannelKind::track
            ? (! anyTrackSolo || settings.solo)
            : (! anyAuxSolo || settings.solo);
        if (settings.muted || ! soloAllows)
        {
            runtime->postFader.clear();
            runtime->previousLeftGain = 0.0f;
            runtime->previousRightGain = 0.0f;
        }
        else
        {
            if (! runtime->gainAutomation.empty()
                || ! runtime->panAutomation.empty())
            {
                for (int sample = 0; sample < numSamples; ++sample)
                {
                    const auto absoluteSample = blockStartSample
                        + static_cast<juce::int64> (std::llround (
                            sample * timelineSamplesPerDeviceSample));
                    const auto gain = juce::jlimit (0.0f, 1.5f,
                        automation::valueAtSample (
                            absoluteSample, runtime->gainAutomation,
                            settings.gain));
                    const auto pan = juce::jlimit (-1.0f, 1.0f,
                        automation::valueAtSample (
                            absoluteSample, runtime->panAutomation,
                            settings.pan));
                    const auto balance = timeline::balanceForPan (pan);
                    runtime->postFader.setSample (
                        0, sample, runtime->postFader.getSample (0, sample)
                            * gain * balance.left);
                    runtime->postFader.setSample (
                        1, sample, runtime->postFader.getSample (1, sample)
                            * gain * balance.right);
                    runtime->previousLeftGain = gain * balance.left;
                    runtime->previousRightGain = gain * balance.right;
                }
            }
            else
            {
                const auto balance = timeline::balanceForPan (settings.pan);
                const auto leftGain = settings.gain * balance.left;
                const auto rightGain = settings.gain * balance.right;
                runtime->postFader.applyGainRamp (
                    0, 0, numSamples, runtime->previousLeftGain, leftGain);
                runtime->postFader.applyGainRamp (
                    1, 0, numSamples, runtime->previousRightGain, rightGain);
                runtime->previousLeftGain = leftGain;
                runtime->previousRightGain = rightGain;
            }
        }

        runtime->peakLeft = juce::jmax (
            runtime->peakLeft * 0.82f,
            runtime->postFader.getMagnitude (0, 0, numSamples));
        runtime->peakRight = juce::jmax (
            runtime->peakRight * 0.82f,
            runtime->postFader.getMagnitude (1, 0, numSamples));

        if (runtime->meter != nullptr)
        {
            runtime->meter->left.store (runtime->peakLeft,
                                        std::memory_order_relaxed);
            runtime->meter->right.store (runtime->peakRight,
                                         std::memory_order_relaxed);
            runtime->meter->sidechainReduction.store (
                runtime->sidechainReduction, std::memory_order_relaxed);
        }

        for (auto& route : state.mixerRuntimeRoutes)
        {
            if (! route.enabled || route.sourceIndex != graphIndex)
                continue;
            const auto& source = route.preFader ? runtime->preFader
                                                : runtime->postFader;
            auto addRoute = [&route, &source, numSamples] (
                juce::AudioBuffer<float>& target)
            {
                const auto channels = juce::jmin (
                    2, juce::jmin (source.getNumChannels(),
                                   target.getNumChannels()));
                if (route.delaySamples <= 0
                    || ! route.delayLine.isPrepared())
                {
                    for (int channel = 0; channel < channels; ++channel)
                        target.addFrom (channel, 0, source, channel, 0,
                                        numSamples, route.gain);
                    return;
                }
                std::array<const float*, 2> input {};
                std::array<float*, 2> output {};
                for (int channel = 0; channel < channels; ++channel)
                {
                    input[static_cast<std::size_t> (channel)] =
                        source.getReadPointer (channel);
                    output[static_cast<std::size_t> (channel)] =
                        target.getWritePointer (channel);
                }
                if (route.gain == 1.0f)
                    route.delayLine.processAdd (input.data(), output.data(),
                                                channels, numSamples);
                else
                {
                    // Route gain remains outside the delay line so its stored
                    // history is independent of later fader/send changes.
                    if (route.gainBuffer.getNumSamples() < numSamples)
                        return;
                    for (int channel = 0; channel < channels; ++channel)
                        route.gainBuffer.copyFrom (
                            channel, 0, source.getReadPointer (channel),
                            numSamples, route.gain);
                    const float* scaled[] {
                        route.gainBuffer.getReadPointer (0),
                        route.gainBuffer.getReadPointer (1) };
                    route.delayLine.processAdd (scaled, output.data(),
                                                channels, numSamples);
                }
            };
            if (route.destinationIsMaster)
            {
                if (route.kind != mixer::RouteKind::sidechain)
                    addRoute (master);
                continue;
            }
            if (route.destinationIndex >= state.mixerRuntime.size())
                continue;
            auto* destination = &state.mixerRuntime[route.destinationIndex];
            auto& target = route.kind == mixer::RouteKind::sidechain
                               ? destination->sidechain
                               : destination->preFader;
            addRoute (target);
        }
    }
}

void AudioEngine::pushSpectrumSamples (const juce::AudioBuffer<float>& buffer,
                                       int startSample,
                                       int numSamples) noexcept
{
    const auto amount = juce::jmin (numSamples, spectrumFifo.getFreeSpace());
    if (amount <= 0 || buffer.getNumChannels() <= 0)
        return;
    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    spectrumFifo.prepareToWrite (amount, start1, size1, start2, size2);
    const auto writeBlock = [&] (int destinationStart, int count, int sourceStart)
    {
        for (int sample = 0; sample < count; ++sample)
        {
            const auto sourceIndex = startSample + sourceStart + sample;
            const auto left = buffer.getSample (0, sourceIndex);
            const auto right = buffer.getNumChannels() > 1
                                   ? buffer.getSample (1, sourceIndex) : left;
            spectrumRing[static_cast<std::size_t> (destinationStart + sample)]
                = 0.5f * (left + right);
        }
    };
    writeBlock (start1, size1, 0);
    writeBlock (start2, size2, size1);
    spectrumFifo.finishedWrite (size1 + size2);
}

void AudioEngine::play()
{
    const juce::ScopedLock lock (controlLock);

    const auto length = projectLengthUnits.load();
    const auto oneMillisecond = transport::fromSeconds (
        0.001, projectTimelineSampleRate.load());
    if (! recordingTransportActive.load()
        && timelinePositionUnits.load() >= juce::jmax (
            transport::ClockUnits { 0 }, length - oneMillisecond))
    {
        timelinePositionUnits.store (0);
        requestRuntimeReset();
    }

    playing.store (recordingTransportActive.load()
                   || ((! tracks.empty() || ! midiTracks.empty()
                        || metronomeEnabled.load())
                       && length > 0));
}

void AudioEngine::pause()
{
    playing.store (false);
    pluginNeedsAllNotesOff.store (true);
    requestRuntimeReset();
}

void AudioEngine::stopAndRewind()
{
    playing.store (false);
    pluginNeedsAllNotesOff.store (true);
    timelinePositionUnits.store (0);
    requestRuntimeReset();
}

void AudioEngine::setPositionSeconds (double seconds)
{
    pluginNeedsAllNotesOff.store (true);
    const auto rate = projectTimelineSampleRate.load();
    timelinePositionUnits.store (transport::clamp (
        transport::fromSeconds (seconds, rate), projectLengthUnits.load()));
    requestRuntimeReset();
}

void AudioEngine::setRecordingTransportActive (bool active) noexcept
{
    recordingTransportActive.store (active);
}

void AudioEngine::setInputMonitoring (bool enabled) noexcept
{
    inputMonitoring.store (enabled);
}

bool AudioEngine::isInputMonitoring() const noexcept
{
    return inputMonitoring.load();
}

void AudioEngine::setControlRoomState (float gain, bool dimmed,
                                       bool muted) noexcept
{
    controlRoomGain.store (juce::jlimit (0.0f, 1.5f, gain),
                           std::memory_order_release);
    controlRoomDimmed.store (dimmed, std::memory_order_release);
    controlRoomMuted.store (muted, std::memory_order_release);
}

void AudioEngine::setInputChannelCount (int channels) noexcept
{
    inputChannelCount.store (juce::jlimit (0, 2, channels));
}

void AudioEngine::setLiveMidiNote (int channel, int noteNumber, float velocity) noexcept
{
    if (channel < 1 || channel > 16 || noteNumber < 0 || noteNumber >= 128)
        return;
    velocity = juce::jlimit (0.0f, 1.0f, velocity);
    auto& note = liveMidiNotes[
        static_cast<size_t> ((channel - 1) * 128 + noteNumber)];
    const auto previous = note.exchange (velocity);
    if (previous <= 0.0f && velocity > 0.0f)
        liveMidiNoteCount.fetch_add (1);
    else if (previous > 0.0f && velocity <= 0.0f)
        liveMidiNoteCount.fetch_sub (1);
}

void AudioEngine::clearLiveMidiNotes() noexcept
{
    for (auto& note : liveMidiNotes)
        note.store (0.0f);
    liveMidiNoteCount.store (0);
}

void AudioEngine::pushLiveMidiMessage (const juce::MidiMessage& message)
{
    const auto* bytes = message.getRawData();
    const auto size = message.getRawDataSize();
    const auto statusByte = size > 0 && bytes != nullptr ? bytes[0] : 0;
    const auto syncSource = activeSyncSource.load (std::memory_order_acquire);
    const auto nowNanoseconds = std::chrono::duration_cast<
        std::chrono::nanoseconds> (
            std::chrono::steady_clock::now().time_since_epoch()).count();
    const auto timelineSample = static_cast<juce::int64> (std::llround (
        static_cast<double> (nowNanoseconds)
        * projectTimelineSampleRate.load() / 1000000000.0));
    auto followStartStop = false;
    auto mtcChaseSeconds = -1.0;
    if (syncSource != midi::SyncSource::internal)
    {
        const juce::SpinLock::ScopedLockType lock (syncLock);
        followStartStop = syncConfiguration.followExternalStartStop;
        if (statusByte == 0xf8 && syncSource == midi::SyncSource::midiClock)
        {
            syncFollower.observeMidiClock (timelineSample);
            lastExternalSyncSample.store (timelineSample,
                                          std::memory_order_release);
        }
        else if (statusByte == 0xf1 && size > 1
                 && syncSource == midi::SyncSource::midiTimeCode)
        {
            const auto data = static_cast<std::uint8_t> (bytes[1]);
            if (syncFollower.observeMtcQuarterFrame (
                static_cast<std::uint8_t> ((data >> 4) & 0x07),
                static_cast<std::uint8_t> (data & 0x0f), timelineSample))
                mtcChaseSeconds = syncFollower.status().timecodeSeconds;
            lastExternalSyncSample.store (timelineSample,
                                          std::memory_order_release);
        }
        const auto syncStatus = syncFollower.status();
        externalSyncLocked.store (syncStatus.locked,
                                  std::memory_order_release);
        externalSyncTempoBpm.store (syncStatus.tempoBpm,
                                    std::memory_order_release);
    }

    if (mtcChaseSeconds >= 0.0)
        timelinePositionUnits.store (transport::clamp (
            transport::fromSeconds (
                mtcChaseSeconds, projectTimelineSampleRate.load()),
            projectLengthUnits.load()), std::memory_order_release);

    if (followStartStop)
    {
        if (statusByte == 0xfa || statusByte == 0xfb)
            play();
        else if (statusByte == 0xfc)
            pause();
    }

    if (! instrumentPluginLoaded.load())
        return;
    liveMidiPending.store (true);
    liveMidiCollector.addMessageToQueue (message);
}

bool AudioEngine::isPlaying() const
{
    return playing.load();
}

double AudioEngine::getPositionSeconds() const
{
    return transport::toSeconds (timelinePositionUnits.load(),
                                 projectTimelineSampleRate.load());
}

double AudioEngine::getProjectLengthSeconds() const
{
    return transport::toSeconds (projectLengthUnits.load(),
                                 projectTimelineSampleRate.load());
}

void AudioEngine::setMasterGain (float newGain) noexcept
{
    const auto value = juce::jlimit (0.0f, 1.25f, newGain);
    masterGain.store (value);
    if (! parameterCommands.push (
            { realtime::ParameterCommand::Type::masterGain, value }))
        realtimeTelemetry.recordParameterQueueOverflow();
}

float AudioEngine::getMasterGain() const noexcept
{
    return masterGain.load();
}

void AudioEngine::setMasterMuted (bool shouldBeMuted) noexcept
{
    masterMuted.store (shouldBeMuted);
    if (! parameterCommands.push (
            { realtime::ParameterCommand::Type::masterMute,
              shouldBeMuted ? 1.0f : 0.0f }))
        realtimeTelemetry.recordParameterQueueOverflow();
}

bool AudioEngine::isMasterMuted() const noexcept
{
    return masterMuted.load();
}

void AudioEngine::renderMetronome (RenderSnapshot& state,
                                   juce::AudioBuffer<float>& master,
                                   int numSamples,
                                   double blockStartSeconds) noexcept
{
    if (state.sampleRate <= 0.0 || numSamples <= 0
        || state.timeSignatures.empty())
        return;

    const auto blockEndSeconds = blockStartSeconds
        + static_cast<double> (numSamples) / state.sampleRate;
    const auto startBeat = tempo::secondsToBeatPrepared (
        blockStartSeconds, state.tempoPoints);
    const auto endBeat = tempo::secondsToBeatPrepared (
        blockEndSeconds, state.tempoPoints);
    std::array<int, 64> eventOffsets {};
    std::array<bool, 64> eventAccents {};
    int eventCount = 0;
    const auto signatureFor = [&state] (double beat)
    {
        for (std::size_t index = state.timeSignatures.size(); index-- > 0;)
            if (beat >= state.timeSignatures[index].beat - 1.0e-9)
                return state.timeSignatures[index];
        return state.timeSignatures.front();
    };

    auto cursor = juce::jmax (0.0, startBeat - 4.0);
    for (int guard = 0; guard < 128 && eventCount < 64; ++guard)
    {
        const auto signature = signatureFor (cursor);
        const auto unit = automation::beatUnitLength (signature);
        const auto relative = juce::jmax (0.0, cursor - signature.beat);
        auto tick = signature.beat + std::ceil ((relative - 1.0e-9) / unit) * unit;
        if (tick < startBeat - 1.0e-8)
            tick += std::ceil ((startBeat - tick) / unit) * unit;
        if (tick >= endBeat + 1.0e-8)
            break;
        const auto eventSeconds = tempo::beatToSecondsPrepared (
            tick, state.tempoPoints);
        const auto offset = static_cast<int> (std::llround (
            (eventSeconds - blockStartSeconds) * state.sampleRate));
        if (offset >= 0 && offset < numSamples)
        {
            eventOffsets[static_cast<std::size_t> (eventCount)] = offset;
            eventAccents[static_cast<std::size_t> (eventCount)] =
                automation::isBarAccent (tick, signature);
            ++eventCount;
        }
        cursor = tick + unit * 0.5;
    }

    auto eventIndex = 0;
    constexpr double twoPi = 6.28318530717958647692;
    const auto decay = static_cast<float> (std::exp (
        -1.0 / (state.sampleRate * 0.022)));
    for (int sample = 0; sample < numSamples; ++sample)
    {
        while (eventIndex < eventCount
               && eventOffsets[static_cast<std::size_t> (eventIndex)] == sample)
        {
            const auto accent = eventAccents[static_cast<std::size_t> (eventIndex)];
            state.metronomeFrequency = accent ? 1760.0 : 1120.0;
            state.metronomeEnvelope = accent ? 0.48f : 0.30f;
            state.metronomePhase = 0.0;
            ++eventIndex;
        }
        if (state.metronomeEnvelope < 0.00001f)
            continue;
        const auto click = static_cast<float> (std::sin (state.metronomePhase))
                           * state.metronomeEnvelope * metronomeGain.load();
        state.metronomePhase += twoPi * state.metronomeFrequency / state.sampleRate;
        if (state.metronomePhase >= twoPi)
            state.metronomePhase -= twoPi;
        state.metronomeEnvelope *= decay;
        for (int channel = 0; channel < juce::jmin (2, master.getNumChannels()); ++channel)
            master.addSample (channel, sample, click);
    }
}

void AudioEngine::applySmoothedMasterGain (RenderSnapshot& state,
                                           juce::AudioBuffer<float>& buffer,
                                           int startSample,
                                           int numSamples,
                                           juce::int64 blockStartSample,
                                           double timelineSamplesPerDeviceSample) noexcept
{
    state.masterGainSmoother.setTarget (
        state.masterMutedTarget ? 0.0f : state.masterGainTarget);
    for (int sample = 0; sample < numSamples; ++sample)
    {
        auto gain = state.masterGainSmoother.process();
        if (! state.masterMutedTarget && ! state.masterGainAutomation.empty())
        {
            const auto absoluteSample = blockStartSample
                + static_cast<juce::int64> (std::llround (
                    sample * timelineSamplesPerDeviceSample));
            gain = juce::jlimit (0.0f, 1.25f,
                automation::valueAtSample (absoluteSample,
                                           state.masterGainAutomation,
                                           state.masterGainTarget));
        }
        auto pan = 0.0f;
        if (! state.masterPanAutomation.empty())
        {
            const auto absoluteSample = blockStartSample
                + static_cast<juce::int64> (std::llround (
                    sample * timelineSamplesPerDeviceSample));
            pan = juce::jlimit (-1.0f, 1.0f,
                automation::valueAtSample (absoluteSample,
                                           state.masterPanAutomation, 0.0f));
        }
        const auto balance = timeline::balanceForPan (pan);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample (channel,
                              startSample + sample,
                              buffer.getSample (channel, startSample + sample) * gain
                                  * (channel == 0 ? balance.left : balance.right));
    }
    pushSpectrumSamples (buffer, startSample, numSamples);
}

void AudioEngine::addSmoothedInputMonitor (RenderSnapshot& state,
                                           juce::AudioBuffer<float>& buffer,
                                           int startSample,
                                           int numSamples) noexcept
{
    std::size_t outputStartChannel = 0;
    auto monitorPathGain = 1.0f;
    if (! state.monitorPlan.paths.empty())
    {
        const auto path = std::find_if (
            state.monitorPlan.paths.begin(), state.monitorPlan.paths.end(),
            [] (const auto& candidate)
        {
            return candidate.role == mixer::MonitorRole::controlRoom;
        });
        if (path == state.monitorPlan.paths.end())
            return;
        outputStartChannel = path->outputStartChannel;
        monitorPathGain = path->muted ? 0.0f : path->gain;
        if (path->dimmed)
            monitorPathGain *= 0.25f;
    }
    auto target = controlRoomMuted.load (std::memory_order_acquire)
                      ? 0.0f : controlRoomGain.load (std::memory_order_acquire);
    if (controlRoomDimmed.load (std::memory_order_acquire))
        target *= 0.25f;
    target *= monitorPathGain;
    state.controlRoomGainSmoother.setTarget (target);
    const auto channels = juce::jmin (
        juce::jmax (0, buffer.getNumChannels()
            - static_cast<int> (outputStartChannel)),
        state.inputMonitorBuffer.getNumChannels());
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto gain = state.controlRoomGainSmoother.process();
        for (int channel = 0; channel < channels; ++channel)
            buffer.addSample (
                static_cast<int> (outputStartChannel) + channel,
                startSample + sample,
                state.inputMonitorBuffer.getSample (channel, sample) * gain);
    }
}
}
