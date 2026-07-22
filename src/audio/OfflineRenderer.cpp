#include "OfflineRenderer.h"

#include "core/OfflineRenderMath.h"
#include "core/MixerGraph.h"
#include "core/MixerMath.h"
#include "core/TempoMap.h"
#include "core/TempoAutomation.h"
#include "core/TimelineMath.h"
#include "core/BuiltInInstrument.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace triumph
{
namespace
{
struct SourceReader
{
    juce::String id;
    std::unique_ptr<juce::AudioFormatReader> reader;
};

struct RenderTrack
{
    TrackState state;
    std::vector<AudioPlaybackSegmentState> segments;
};

struct RenderMixerChannel
{
    RenderMixerChannel (const juce::String& channelId,
                        mixer::ChannelKind channelKind,
                        float channelGain,
                        float channelPan,
                        bool channelMuted,
                        bool channelSolo)
        : id (channelId),
          kind (channelKind),
          gain (channelGain),
          pan (channelPan),
          muted (channelMuted),
          solo (channelSolo)
    {
    }

    juce::String id;
    mixer::ChannelKind kind = mixer::ChannelKind::track;
    float gain = 0.85f;
    float pan = 0.0f;
    bool muted = false;
    bool solo = false;
    juce::AudioBuffer<float> preFader;
    juce::AudioBuffer<float> postFader;
    juce::AudioBuffer<float> sidechain;
    std::vector<automation::Point> gainAutomation;
    std::vector<automation::Point> panAutomation;
};

SourceReader* findReader (std::vector<SourceReader>& readers,
                          const juce::String& id)
{
    const auto found = std::find_if (readers.begin(), readers.end(),
                                     [&id] (const auto& candidate)
    {
        return candidate.id == id;
    });
    return found != readers.end() ? &*found : nullptr;
}

RenderMixerChannel* findMixerChannel (
    std::vector<RenderMixerChannel>& channels,
    const juce::String& id)
{
    const auto found = std::find_if (channels.begin(), channels.end(),
                                     [&id] (const auto& candidate)
    {
        return candidate.id == id;
    });
    return found != channels.end() ? &*found : nullptr;
}
}

juce::Result OfflineRenderer::renderStereoWav (
    const ProjectState& snapshot, const juce::File& destination,
    const Options& options, std::atomic<bool>& cancelRequested,
    std::atomic<float>& progress)
{
    progress.store (juce::jlimit (0.0f, 1.0f, options.progressStart));
    if (options.sampleRate <= 0.0 || options.blockSize <= 0
        || (options.bitsPerSample != 16 && options.bitsPerSample != 24))
        return juce::Result::fail ("The export settings are invalid.");

    const auto trackIsIncluded = [&options] (const TrackState& track)
    {
        return options.isolatedTrackId.isEmpty()
               || track.id == options.isolatedTrackId;
    };
    if (options.isolatedTrackId.isNotEmpty()
        && std::none_of (snapshot.tracks.begin(), snapshot.tracks.end(),
                         trackIsIncluded))
        return juce::Result::fail ("The requested export track no longer exists.");

    for (const auto& track : snapshot.tracks)
        if (track.isInstrument
            && ! track.frozen
            && trackIsIncluded (track)
            && (track.pluginDescriptionXml.isNotEmpty()
                || track.pluginName.isNotEmpty()))
            return juce::Result::fail (
                "This export includes an external VST3 instrument. Triumph Studio "
                "blocks incomplete delivery until that instrument is frozen to audio.");

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::vector<SourceReader> readers;
    for (const auto& source : snapshot.mediaSources)
    {
        auto reader = std::unique_ptr<juce::AudioFormatReader> (
            formats.createReaderFor (source.file));
        if (reader == nullptr)
            return juce::Result::fail ("Missing or unreadable source: "
                                       + source.file.getFullPathName());
        readers.push_back ({ source.id, std::move (reader) });
    }

    ProjectModel model;
    model.replaceWithSnapshot (snapshot);
    std::vector<RenderTrack> renderTracks;
    renderTracks.reserve (snapshot.tracks.size());
    for (const auto& track : snapshot.tracks)
        if (trackIsIncluded (track))
            renderTracks.push_back ({ track, model.resolveAudioPlayback (track.id) });

    std::vector<mixer::Channel> graphChannels;
    std::vector<mixer::Route> graphRoutes;
    std::vector<RenderMixerChannel> mixerChannels;
    const auto addRoutes = [&graphRoutes] (const juce::String& sourceId,
                                           const std::vector<MixerSendState>& sends)
    {
        for (const auto& send : sends)
            graphRoutes.push_back ({ send.id.toStdString(), sourceId.toStdString(),
                                     send.destinationId.toStdString(),
                                     send.sidechain ? mixer::RouteKind::sidechain
                                                    : mixer::RouteKind::send,
                                     send.gain, send.preFader, send.enabled });
    };
    for (const auto& track : snapshot.tracks)
    {
        graphChannels.push_back ({ track.id.toStdString(), mixer::ChannelKind::track,
                                   track.outputDestinationId.toStdString() });
        addRoutes (track.id, track.sends);
        mixerChannels.push_back ({ track.id, mixer::ChannelKind::track,
                                   track.gain, track.pan, track.muted, track.solo });
    }
    for (const auto& channel : snapshot.mixerChannels)
    {
        const auto kind = channel.kind == MixerChannelKind::returnChannel
                              ? mixer::ChannelKind::returnChannel
                              : mixer::ChannelKind::bus;
        graphChannels.push_back ({ channel.id.toStdString(), kind,
                                   channel.outputDestinationId.toStdString() });
        addRoutes (channel.id, channel.sends);
        mixerChannels.push_back ({ channel.id, kind, channel.gain, channel.pan,
                                   channel.muted, channel.solo });
    }
    const auto mixerPlan = mixer::buildPlan (std::move (graphChannels),
                                             std::move (graphRoutes));
    if (! mixerPlan.valid)
        return juce::Result::fail ("The mixer routing graph is invalid: "
                                   + juce::String (mixerPlan.error));
    const auto timelineRate = juce::jmax (1.0, snapshot.timelineSampleRate);
    const auto projectSamples = model.getProjectLengthInSamples();
    const auto rangeStart = juce::jlimit<juce::int64> (
        0, projectSamples, options.timelineStartSamples);
    const auto requestedEnd = options.timelineEndSamples > 0
                                  ? options.timelineEndSamples
                                  : projectSamples;
    const auto rangeEnd = juce::jlimit<juce::int64> (
        rangeStart, projectSamples, requestedEnd);
    if (rangeEnd <= rangeStart)
        return juce::Result::fail ("The export range contains no project material.");
    const auto totalSamples = static_cast<juce::int64> (
        offline::renderLengthSamples (rangeEnd - rangeStart, timelineRate,
                                      options.sampleRate, options.tailSeconds));
    if (totalSamples <= 0)
        return juce::Result::fail ("The project has no material to export.");

    const auto partial = destination.getSiblingFile (
        destination.getFileName() + ".partial");
    partial.deleteFile();
    std::unique_ptr<juce::OutputStream> stream (partial.createOutputStream());
    if (stream == nullptr)
        return juce::Result::fail ("The export file could not be opened.");
    juce::WavAudioFormat wav;
    auto writerOptions = juce::AudioFormatWriterOptions {}
        .withSampleRate (options.sampleRate)
        .withNumChannels (2)
        .withBitsPerSample (options.bitsPerSample);
    auto writer = wav.createWriterFor (stream, writerOptions);
    if (writer == nullptr)
        return juce::Result::fail ("The 24-bit WAV writer could not be created.");

    std::vector<tempo::Point> tempoPoints;
    for (const auto& point : snapshot.tempoPoints)
        tempoPoints.push_back ({ point.beat, point.bpm,
            point.curve == TempoCurve::linear ? tempo::SegmentCurve::linear
                                               : tempo::SegmentCurve::step });
    tempoPoints = tempo::normalised (std::move (tempoPoints));
    for (auto& channel : mixerChannels)
    {
        channel.preFader.setSize (2, options.blockSize);
        channel.postFader.setSize (2, options.blockSize);
        channel.sidechain.setSize (2, options.blockSize);
    }
    std::vector<automation::Point> masterGainAutomation;
    std::vector<automation::Point> masterPanAutomation;
    for (const auto& lane : snapshot.automationLanes)
    {
        if (! lane.enabled || lane.points.empty())
            continue;
        std::vector<automation::Point> points;
        for (const auto& point : lane.points)
            points.push_back ({ point.samplePosition, point.value,
                point.curve == AutomationCurve::hold ? automation::Curve::hold
                : point.curve == AutomationCurve::smooth ? automation::Curve::smooth
                                                         : automation::Curve::linear });
        points = automation::normalised (std::move (points));
        if (lane.targetId == mixer::masterId && lane.parameterId == "gain")
            masterGainAutomation = std::move (points);
        else if (lane.targetId == mixer::masterId && lane.parameterId == "pan")
            masterPanAutomation = std::move (points);
        else if (auto* channel = findMixerChannel (mixerChannels, lane.targetId))
        {
            if (lane.parameterId == "pan")
                channel->panAutomation = std::move (points);
            else if (lane.parameterId == "gain")
                channel->gainAutomation = std::move (points);
        }
    }

    juce::AudioBuffer<float> mix (2, options.blockSize);
    juce::AudioBuffer<float> trackBuffer (2, options.blockSize);
    juce::AudioBuffer<float> sourceBuffer (2, options.blockSize * 5 + 4);
    offline::DeterministicDither dither;

    for (juce::int64 blockStart = 0; blockStart < totalSamples;
         blockStart += options.blockSize)
    {
        if (cancelRequested.load())
        {
            writer.reset();
            partial.deleteFile();
            return juce::Result::fail ("Export cancelled.");
        }
        const auto blockSamples = static_cast<int> (juce::jmin (
            static_cast<juce::int64> (options.blockSize),
            totalSamples - blockStart));
        mix.clear();
        for (auto& channel : mixerChannels)
        {
            channel.preFader.clear();
            channel.postFader.clear();
            channel.sidechain.clear();
        }
        const auto blockStartSeconds = static_cast<double> (rangeStart) / timelineRate
                                       + static_cast<double> (blockStart)
                                             / options.sampleRate;

        for (const auto& renderTrack : renderTracks)
        {
            const auto& track = renderTrack.state;
            trackBuffer.clear();

            for (const auto& segment : renderTrack.segments)
            {
                auto* source = findReader (readers, segment.sourceId);
                if (source == nullptr || source->reader == nullptr)
                    continue;
                const auto segmentStart = timeline::samplesToSeconds (
                    segment.timelineStartSamples, timelineRate);
                const auto segmentLength = timeline::samplesToSeconds (
                    segment.lengthInSamples, timelineRate);
                const auto segmentEnd = segmentStart + segmentLength;
                const auto blockEnd = blockStartSeconds
                    + static_cast<double> (blockSamples) / options.sampleRate;
                const auto intersectionStart = juce::jmax (segmentStart,
                                                            blockStartSeconds);
                const auto intersectionEnd = juce::jmin (segmentEnd, blockEnd);
                if (intersectionEnd <= intersectionStart)
                    continue;
                const auto destinationStart = juce::jlimit (0, blockSamples,
                    juce::roundToInt ((intersectionStart - blockStartSeconds)
                                      * options.sampleRate));
                const auto destinationEnd = juce::jlimit (destinationStart,
                    blockSamples, juce::roundToInt ((intersectionEnd - blockStartSeconds)
                                                     * options.sampleRate));
                const auto amount = destinationEnd - destinationStart;
                const auto direction = segment.reversed ? -1.0 : 1.0;
                const auto segmentSourceSpan = segmentLength
                    * source->reader->sampleRate * segment.playbackRate;
                const auto sourceOrigin = static_cast<double> (
                    segment.sourceOffsetSamples)
                    + (segment.reversed ? juce::jmax (0.0, segmentSourceSpan - 1.0)
                                        : 0.0);
                const auto sourceStart = sourceOrigin
                    + direction * (intersectionStart - segmentStart)
                        * source->reader->sampleRate * segment.playbackRate;
                const auto sourceIncrement = direction
                    * source->reader->sampleRate / options.sampleRate
                    * segment.playbackRate;
                const auto sourceFinish = sourceStart
                    + static_cast<double> (juce::jmax (0, amount - 1))
                        * sourceIncrement;
                const auto firstSourceSample = static_cast<juce::int64> (
                    std::floor (juce::jmin (sourceStart, sourceFinish)));
                const auto lastSourceSample = static_cast<juce::int64> (
                    std::ceil (juce::jmax (sourceStart, sourceFinish))) + 1;
                const auto sourceCount = juce::jmax (
                    2, static_cast<int> (lastSourceSample - firstSourceSample + 1));
                if (sourceBuffer.getNumSamples() < sourceCount)
                    sourceBuffer.setSize (2, sourceCount, false, false, true);
                sourceBuffer.clear();
                source->reader->read (&sourceBuffer, 0, sourceCount,
                                      firstSourceSample, true, true);

                for (int sample = 0; sample < amount; ++sample)
                {
                    const auto position = sourceStart + sample * sourceIncrement
                                          - firstSourceSample;
                    const auto index = juce::jlimit (0, sourceCount - 2,
                                                     static_cast<int> (position));
                    const auto fraction = position - std::floor (position);
                    const auto fadeAnchorStartSamples =
                        segment.fadeAnchorLengthSamples > 0
                            ? segment.fadeAnchorStartSamples
                            : segment.timelineStartSamples;
                    const auto fadeAnchorLengthSamples =
                        segment.fadeAnchorLengthSamples > 0
                            ? segment.fadeAnchorLengthSamples
                            : segment.lengthInSamples;
                    const auto fadeAnchorStart = timeline::samplesToSeconds (
                        fadeAnchorStartSamples, timelineRate);
                    const auto timelinePosition = static_cast<juce::int64> (std::llround (
                        (intersectionStart - fadeAnchorStart
                         + static_cast<double> (sample) / options.sampleRate)
                        * timelineRate));
                    auto fade = 1.0f;
                    if (segment.fadeInSamples > 0
                        && timelinePosition < segment.fadeInSamples)
                        fade *= timeline::equalPowerFadeIn (
                            timelinePosition, segment.fadeInSamples);
                    if (segment.fadeOutSamples > 0
                        && timelinePosition >= fadeAnchorLengthSamples
                                                   - segment.fadeOutSamples)
                        fade *= timeline::equalPowerFadeOut (
                            timelinePosition - (fadeAnchorLengthSamples
                                                - segment.fadeOutSamples),
                            segment.fadeOutSamples);
                    for (int channel = 0; channel < 2; ++channel)
                    {
                        const auto sourceChannel = source->reader->numChannels == 1
                                                       ? 0 : channel;
                        const auto value = offline::linearSample (
                            sourceBuffer.getSample (sourceChannel, index),
                            sourceBuffer.getSample (sourceChannel, index + 1),
                            fraction) * segment.gain * fade;
                        trackBuffer.addSample (channel, destinationStart + sample, value);
                    }
                }
            }

            if (track.isInstrument && ! track.frozen)
                for (const auto& midiClip : track.midiClips)
                    for (const auto& note : midiClip.notes)
                    {
                        const auto noteStart = tempo::beatToSeconds (
                            midiClip.startBeat + note.startBeat, tempoPoints);
                        const auto noteEnd = tempo::beatToSeconds (
                            midiClip.startBeat + note.startBeat + note.lengthBeats,
                            tempoPoints);
                        const auto blockEnd = blockStartSeconds
                            + static_cast<double> (blockSamples) / options.sampleRate;
                        if (noteEnd <= blockStartSeconds || noteStart >= blockEnd)
                            continue;
                        for (int sample = 0; sample < blockSamples; ++sample)
                        {
                            const auto time = blockStartSeconds
                                + static_cast<double> (sample) / options.sampleRate;
                            if (time < noteStart || time >= noteEnd)
                                continue;
                            const auto value = instrument::renderSample (
                                note.noteNumber, note.channel, note.velocity,
                                time - noteStart, noteEnd - time,
                                static_cast<juce::int64> (std::llround (
                                    time * options.sampleRate)), options.sampleRate);
                            trackBuffer.addSample (0, sample, value);
                            trackBuffer.addSample (1, sample, value);
                        }
                    }

            if (auto* channel = findMixerChannel (mixerChannels, track.id))
                for (int audioChannel = 0; audioChannel < 2; ++audioChannel)
                    channel->preFader.addFrom (audioChannel, 0, trackBuffer,
                                               audioChannel, 0, blockSamples);
        }

        const auto anyTrackSolo = std::any_of (
            mixerChannels.begin(), mixerChannels.end(), [] (const auto& channel)
        {
            return channel.kind == mixer::ChannelKind::track && channel.solo;
        });
        const auto anyAuxSolo = std::any_of (
            mixerChannels.begin(), mixerChannels.end(), [] (const auto& channel)
        {
            return channel.kind != mixer::ChannelKind::track && channel.solo;
        });
        for (const auto channelIndex : mixerPlan.processOrder)
        {
            if (channelIndex >= mixerChannels.size())
                continue;
            auto& channel = mixerChannels[channelIndex];
            channel.postFader.makeCopyOf (channel.preFader, true);
            for (int sample = 0; sample < blockSamples; ++sample)
            {
                const auto control = juce::jmax (
                    std::abs (channel.sidechain.getSample (0, sample)),
                    std::abs (channel.sidechain.getSample (1, sample)));
                const auto duckGain = mixer::sidechainDuckGain (control);
                for (int audioChannel = 0; audioChannel < 2; ++audioChannel)
                    channel.postFader.setSample (
                        audioChannel, sample,
                        channel.postFader.getSample (audioChannel, sample) * duckGain);
            }
            const auto soloAllows = channel.kind == mixer::ChannelKind::track
                ? (! anyTrackSolo || channel.solo)
                : (! anyAuxSolo || channel.solo);
            if (channel.muted || ! soloAllows)
                channel.postFader.clear();
            else
            {
                if (! channel.gainAutomation.empty()
                    || ! channel.panAutomation.empty())
                {
                    for (int sample = 0; sample < blockSamples; ++sample)
                    {
                        const auto absoluteSample = rangeStart
                            + static_cast<juce::int64> (std::llround (
                                (blockStart + sample) * timelineRate
                                / options.sampleRate));
                        const auto gain = juce::jlimit (0.0f, 1.5f,
                            automation::valueAtSample (
                                absoluteSample, channel.gainAutomation,
                                channel.gain));
                        const auto pan = juce::jlimit (-1.0f, 1.0f,
                            automation::valueAtSample (
                                absoluteSample, channel.panAutomation,
                                channel.pan));
                        const auto balance = timeline::balanceForPan (pan);
                        channel.postFader.setSample (
                            0, sample, channel.postFader.getSample (0, sample)
                                * gain * balance.left);
                        channel.postFader.setSample (
                            1, sample, channel.postFader.getSample (1, sample)
                                * gain * balance.right);
                    }
                }
                else
                {
                    const auto balance = timeline::balanceForPan (channel.pan);
                    channel.postFader.applyGain (
                        0, 0, blockSamples, channel.gain * balance.left);
                    channel.postFader.applyGain (
                        1, 0, blockSamples, channel.gain * balance.right);
                }
            }

            for (const auto& route : mixerPlan.routes)
            {
                if (! route.enabled || route.source != channel.id.toStdString())
                    continue;
                const auto& routeSource = route.preFader ? channel.preFader
                                                         : channel.postFader;
                if (route.destination == mixer::masterId)
                {
                    if (route.kind != mixer::RouteKind::sidechain)
                        for (int audioChannel = 0; audioChannel < 2; ++audioChannel)
                            mix.addFrom (audioChannel, 0, routeSource,
                                         audioChannel, 0, blockSamples, route.gain);
                    continue;
                }
                auto* destinationChannel = findMixerChannel (
                    mixerChannels, juce::String (route.destination));
                if (destinationChannel == nullptr)
                    continue;
                auto& target = route.kind == mixer::RouteKind::sidechain
                                   ? destinationChannel->sidechain
                                   : destinationChannel->preFader;
                for (int audioChannel = 0; audioChannel < 2; ++audioChannel)
                    target.addFrom (audioChannel, 0, routeSource,
                                    audioChannel, 0, blockSamples, route.gain);
            }
        }

        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < blockSamples; ++sample)
            {
                const auto absoluteSample = rangeStart
                    + static_cast<juce::int64> (std::llround (
                        (blockStart + sample) * timelineRate
                        / options.sampleRate));
                const auto automatedMaster = masterGainAutomation.empty()
                    ? snapshot.masterGain
                    : juce::jlimit (0.0f, 1.25f,
                        automation::valueAtSample (
                            absoluteSample, masterGainAutomation,
                            snapshot.masterGain));
                const auto automatedPan = masterPanAutomation.empty()
                    ? 0.0f : juce::jlimit (-1.0f, 1.0f,
                        automation::valueAtSample (
                            absoluteSample, masterPanAutomation, 0.0f));
                const auto masterBalance = timeline::balanceForPan (automatedPan);
                mix.getWritePointer (channel)[sample]
                    = juce::jlimit (-1.0f, 1.0f,
                        mix.getSample (channel, sample)
                            * (snapshot.masterMuted ? 0.0f : automatedMaster)
                            * (channel == 0 ? masterBalance.left
                                            : masterBalance.right)
                        + dither.nextTpdf (options.bitsPerSample));
            }
        if (! writer->writeFromAudioSampleBuffer (mix, 0, blockSamples))
        {
            writer.reset();
            partial.deleteFile();
            return juce::Result::fail ("The export writer stopped unexpectedly.");
        }
        const auto fraction = static_cast<float> (blockStart + blockSamples)
                              / static_cast<float> (totalSamples);
        progress.store (juce::jlimit (
            0.0f, 1.0f, options.progressStart + fraction * options.progressSpan));
    }

    writer.reset();
    if (! partial.replaceFileIn (destination))
    {
        partial.deleteFile();
        return juce::Result::fail ("The completed export could not be published.");
    }
    progress.store (juce::jlimit (
        0.0f, 1.0f, options.progressStart + options.progressSpan));
    return juce::Result::ok();
}
}
