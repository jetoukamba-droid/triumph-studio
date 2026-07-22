#include "PluginFreezeRenderer.h"

#include "core/PluginHostPolicy.h"

#include "core/TempoMap.h"
#include "core/TempoAutomation.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace triumph
{
namespace
{
struct Event
{
    juce::int64 sample = 0;
    juce::MidiMessage message;
};

struct ParameterAutomation
{
    juce::AudioProcessorParameter* parameter = nullptr;
    std::vector<automation::Point> points;
    float lastValue = -1.0f;
};

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

class FreezePlayHead final : public juce::AudioPlayHead
{
public:
    FreezePlayHead (double rate, const std::vector<tempo::Point>& map)
        : sampleRate (rate), tempoPoints (map)
    {
    }

    void setSamplePosition (juce::int64 position) noexcept
    {
        samplePosition = position;
    }

    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo result;
        const auto seconds = static_cast<double> (samplePosition) / sampleRate;
        const auto beat = tempo::secondsToBeat (seconds, tempoPoints);
        result.setTimeInSamples (samplePosition);
        result.setTimeInSeconds (seconds);
        result.setPpqPosition (beat);
        result.setPpqPositionOfLastBarStart (std::floor (beat / 4.0) * 4.0);
        result.setBpm (tempo::bpmAtBeat (beat, tempoPoints));
        result.setIsPlaying (true);
        result.setIsRecording (false);
        result.setIsLooping (false);
        return result;
    }

private:
    double sampleRate = 48000.0;
    const std::vector<tempo::Point>& tempoPoints;
    juce::int64 samplePosition = 0;
};

class ScopedPlayHead final
{
public:
    ScopedPlayHead (juce::AudioProcessor& processorToUse,
                    juce::AudioPlayHead& playHead)
        : processor (processorToUse)
    {
        processor.setPlayHead (&playHead);
    }

    ~ScopedPlayHead()
    {
        processor.setPlayHead (nullptr);
    }

private:
    juce::AudioProcessor& processor;
};
}

juce::Result PluginFreezeRenderer::render (
    std::unique_ptr<juce::AudioPluginInstance> plugin,
    const TrackState& track,
    const std::vector<TempoPointState>& tempoPoints,
    const std::vector<AutomationLaneState>& automationLanes,
    double sampleRate,
    const juce::File& destination,
    std::atomic<bool>& cancelRequested,
    std::atomic<float>& progress)
{
    progress.store (0.0f);
    if (plugin == nullptr || sampleRate <= 0.0 || destination == juce::File())
        return juce::Result::fail ("The instrument freeze request is invalid.");

    constexpr int blockSize = 512;
    plugin->disableNonMainBuses();
    auto layout = plugin->getBusesLayout();
    if (! layout.outputBuses.isEmpty()
        && layout.getMainOutputChannels() != 2)
    {
        auto stereo = layout;
        stereo.outputBuses.getReference (0) = juce::AudioChannelSet::stereo();
        if (plugin->checkBusesLayoutSupported (stereo))
            plugin->setBusesLayout (stereo);
    }
    plugin->setNonRealtime (true);
    plugin->setRateAndBufferSizeDetails (sampleRate, blockSize);
    plugin->prepareToPlay (sampleRate, blockSize);
    if (plugin->getTotalNumOutputChannels() <= 0)
    {
        plugin->releaseResources();
        return juce::Result::fail ("The instrument exposes no audio output for freezing.");
    }

    std::vector<tempo::Point> points;
    for (const auto& point : tempoPoints)
        points.push_back ({ point.beat, point.bpm,
            point.curve == TempoCurve::linear ? tempo::SegmentCurve::linear
                                               : tempo::SegmentCurve::step });
    points = tempo::normalised (std::move (points));

    std::vector<ParameterAutomation> parameterAutomation;
    const auto parameters = plugin->getParameters();
    for (const auto& lane : automationLanes)
    {
        if (! lane.enabled || lane.points.empty()
            || lane.targetId != track.id
            || ! lane.parameterId.startsWith ("plugin:"))
            continue;
        for (int index = 0; index < parameters.size(); ++index)
        {
            auto* parameter = parameters[index];
            if (parameter == nullptr || ! parameter->isAutomatable()
                || stableParameterIdFor (*parameter, index)
                       != lane.parameterId)
                continue;
            std::vector<automation::Point> automationPoints;
            automationPoints.reserve (lane.points.size());
            for (const auto& point : lane.points)
                automationPoints.push_back ({
                    point.samplePosition, point.value,
                    point.curve == AutomationCurve::hold
                        ? automation::Curve::hold
                        : point.curve == AutomationCurve::smooth
                            ? automation::Curve::smooth
                            : automation::Curve::linear });
            parameterAutomation.push_back ({
                parameter, automation::normalised (
                    std::move (automationPoints)), -1.0f });
            break;
        }
    }

    std::vector<Event> events;
    juce::int64 musicalEnd = 0;
    for (const auto& clip : track.midiClips)
    {
        musicalEnd = juce::jmax (musicalEnd, tempo::beatToSamples (
            clip.startBeat + clip.lengthBeats, sampleRate, points));
        for (const auto& note : clip.notes)
        {
            const auto start = tempo::beatToSamples (
                clip.startBeat + note.startBeat, sampleRate, points);
            const auto end = tempo::beatToSamples (
                clip.startBeat + note.startBeat + note.lengthBeats,
                sampleRate, points);
            events.push_back ({ start, juce::MidiMessage::noteOn (
                juce::jlimit (1, 16, note.channel),
                juce::jlimit (0, 127, note.noteNumber),
                juce::jlimit (0.0f, 1.0f, note.velocity)) });
            events.push_back ({ juce::jmax (start + 1, end),
                                juce::MidiMessage::noteOff (
                                    juce::jlimit (1, 16, note.channel),
                                    juce::jlimit (0, 127, note.noteNumber)) });
            musicalEnd = juce::jmax (musicalEnd, end);
        }
    }
    if (events.empty())
    {
        plugin->releaseResources();
        return juce::Result::fail ("The instrument track has no MIDI notes to freeze.");
    }
    std::sort (events.begin(), events.end(), [] (const auto& a, const auto& b)
    {
        if (a.sample != b.sample)
            return a.sample < b.sample;
        return a.message.isNoteOff() && ! b.message.isNoteOff();
    });

    FreezePlayHead playHead (sampleRate, points);
    ScopedPlayHead playHeadAttachment (*plugin, playHead);

    const auto latency = juce::jmax (0, plugin->getLatencySamples());
    const auto tailSamples = static_cast<juce::int64> (
        triumph::plugin::tailSamples (
            triumph::plugin::TailAction::freeze,
            plugin->getTailLengthSeconds(), sampleRate));
    const auto outputSamples = juce::jmax (
        static_cast<juce::int64> (1),
        musicalEnd + juce::jmax (tailSamples,
                                  static_cast<juce::int64> (sampleRate)));
    const auto processSamples = outputSamples + latency;

    const auto partial = destination.getSiblingFile (
        destination.getFileName() + ".partial");
    partial.deleteFile();
    std::unique_ptr<juce::OutputStream> stream (partial.createOutputStream());
    if (stream == nullptr)
    {
        plugin->releaseResources();
        return juce::Result::fail ("The freeze file could not be created.");
    }
    juce::WavAudioFormat wav;
    auto writer = wav.createWriterFor (
        stream, juce::AudioFormatWriterOptions {}
                    .withSampleRate (sampleRate)
                    .withNumChannels (2)
                    .withBitsPerSample (24));
    if (writer == nullptr)
    {
        plugin->releaseResources();
        partial.deleteFile();
        return juce::Result::fail ("The freeze WAV writer could not be created.");
    }

    const auto channels = juce::jmax (
        2, juce::jmax (plugin->getTotalNumInputChannels(),
                       plugin->getTotalNumOutputChannels()));
    juce::AudioBuffer<float> audio (channels, blockSize);
    juce::AudioBuffer<float> stereo (2, blockSize);
    size_t eventIndex = 0;
    juce::int64 processed = 0;
    juce::int64 written = 0;

    while (processed < processSamples)
    {
        if (cancelRequested.load())
        {
            writer.reset();
            plugin->releaseResources();
            partial.deleteFile();
            return juce::Result::fail ("Instrument freeze cancelled.");
        }
        const auto count = static_cast<int> (juce::jmin (
            static_cast<juce::int64> (blockSize), processSamples - processed));
        playHead.setSamplePosition (processed);
        audio.clear();
        juce::MidiBuffer midi;
        while (eventIndex < events.size()
               && events[eventIndex].sample < processed + count)
        {
            if (events[eventIndex].sample >= processed)
                midi.addEvent (events[eventIndex].message,
                               static_cast<int> (events[eventIndex].sample - processed));
            ++eventIndex;
        }
        for (auto& lane : parameterAutomation)
        {
            const auto value = juce::jlimit (
                0.0f, 1.0f, automation::valueAtSample (
                    processed, lane.points, lane.parameter->getValue()));
            if (std::abs (value - lane.lastValue) > 0.000001f)
            {
                lane.parameter->setValue (value);
                lane.lastValue = value;
            }
        }
        plugin->processBlock (audio, midi);

        const auto readableStart = juce::jmax (
            processed, static_cast<juce::int64> (latency));
        const auto readableEnd = juce::jmin (
            processed + static_cast<juce::int64> (count),
            static_cast<juce::int64> (latency) + outputSamples);
        if (readableEnd > readableStart)
        {
            const auto sourceOffset = static_cast<int> (readableStart - processed);
            const auto writeCount = static_cast<int> (readableEnd - readableStart);
            stereo.clear();
            for (int channel = 0; channel < 2; ++channel)
            {
                const auto sourceChannel = juce::jmin (
                    channel, juce::jmax (0, audio.getNumChannels() - 1));
                stereo.copyFrom (channel, 0, audio, sourceChannel,
                                 sourceOffset, writeCount);
            }
            if (! writer->writeFromAudioSampleBuffer (stereo, 0, writeCount))
            {
                writer.reset();
                plugin->releaseResources();
                partial.deleteFile();
                return juce::Result::fail ("The freeze audio could not be written.");
            }
            written += writeCount;
        }
        processed += count;
        progress.store (static_cast<float> (processed)
                        / static_cast<float> (processSamples));
    }

    writer.reset();
    plugin->releaseResources();
    if (written != outputSamples
        || (! destination.deleteFile() && destination.exists()))
    {
        partial.deleteFile();
        return juce::Result::fail ("The freeze file could not be committed safely.");
    }
    if (! partial.moveFileTo (destination))
    {
        partial.deleteFile();
        return juce::Result::fail ("The freeze file could not be committed safely.");
    }
    progress.store (1.0f);
    return juce::Result::ok();
}
}
