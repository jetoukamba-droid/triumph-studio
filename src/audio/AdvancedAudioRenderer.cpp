#include "AdvancedAudioRenderer.h"

#include "core/AdvancedAudioEdit.h"
#include "core/TimeStretch.h"
#include "core/TimelineMath.h"
#include "core/WarpMap.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

namespace triumph
{
namespace
{
struct ClipContext
{
    AudioClipState clip;
    MediaSourceState source;
    std::vector<warp::Anchor> anchors;
};

bool findClipContext (const ProjectState& snapshot,
                      const juce::String& trackId,
                      const juce::String& clipId,
                      ClipContext& context)
{
    const auto track = std::find_if (snapshot.tracks.begin(), snapshot.tracks.end(),
                                     [&trackId] (const auto& candidate)
    {
        return candidate.id == trackId;
    });
    if (track == snapshot.tracks.end())
        return false;
    const auto clip = std::find_if (track->clips.begin(), track->clips.end(),
                                    [&clipId] (const auto& candidate)
    {
        return candidate.id == clipId;
    });
    if (clip == track->clips.end())
        return false;
    const auto source = std::find_if (
        snapshot.mediaSources.begin(), snapshot.mediaSources.end(),
        [&clip] (const auto& candidate)
    {
        return candidate.id == clip->sourceId;
    });
    if (source == snapshot.mediaSources.end() || ! source->file.existsAsFile()
        || source->sampleRate <= 0.0 || source->lengthInSamples <= 0)
        return false;

    context.clip = *clip;
    context.source = *source;
    const auto nativeTimelineLength = stretch::timelineToNativeSamples (
        clip->lengthInSamples, clip->timeStretchRatio);
    const auto sourceSpan = static_cast<juce::int64> (
        timeline::convertSampleCount (nativeTimelineLength,
                                      snapshot.timelineSampleRate,
                                      source->sampleRate));
    context.anchors = {
        { 0, clip->sourceOffsetSamples },
        { clip->lengthInSamples,
          juce::jlimit (static_cast<juce::int64> (0), source->lengthInSamples,
                        clip->sourceOffsetSamples + sourceSpan) }
    };
    for (const auto& marker : clip->warpMarkers)
        if (marker.timelineOffsetSamples > 0
            && marker.timelineOffsetSamples < clip->lengthInSamples)
            context.anchors.push_back ({ marker.timelineOffsetSamples,
                                         marker.sourceOffsetSamples });
    context.anchors = warp::normalise (std::move (context.anchors));
    return context.anchors.size() >= 2;
}

double sourcePositionAt (const std::vector<warp::Anchor>& anchors,
                         double timelineOffset)
{
    if (anchors.empty())
        return 0.0;
    timelineOffset = std::clamp (
        timelineOffset, static_cast<double> (anchors.front().timeline),
        static_cast<double> (anchors.back().timeline));
    const auto upper = std::upper_bound (
        anchors.begin(), anchors.end(), timelineOffset,
        [] (double value, const warp::Anchor& anchor)
    {
        return value < static_cast<double> (anchor.timeline);
    });
    if (upper == anchors.begin())
        return static_cast<double> (anchors.front().source);
    if (upper == anchors.end())
        return static_cast<double> (anchors.back().source);
    const auto& right = *upper;
    const auto& left = *(upper - 1);
    const auto span = static_cast<double> (
        right.timeline - left.timeline);
    const auto amount = span > 0.0
        ? (timelineOffset - static_cast<double> (left.timeline)) / span
        : 0.0;
    return static_cast<double> (left.source)
        + amount * static_cast<double> (right.source - left.source);
}

juce::Result createReader (const ClipContext& context,
                           juce::AudioFormatManager& formats,
                           std::unique_ptr<juce::AudioFormatReader>& reader)
{
    reader.reset (formats.createReaderFor (context.source.file));
    return reader != nullptr
        ? juce::Result::ok()
        : juce::Result::fail ("The selected clip's media could not be decoded.");
}

juce::Result renderClipToChannels (
    const ClipContext& context,
    juce::AudioFormatReader& reader, std::atomic<bool>& cancelRequested,
    std::atomic<float>& progress, advanced::AudioChannels& result)
{
    if (context.clip.lengthInSamples <= 0
        || context.clip.lengthInSamples > std::numeric_limits<int>::max())
        return juce::Result::fail (
            "This clip is too long for the built-in pitch renderer.");
    const auto sampleCount = static_cast<int> (context.clip.lengthInSamples);
    const auto channelCount = juce::jlimit (
        1, 2, static_cast<int> (reader.numChannels));
    result.assign (static_cast<std::size_t> (channelCount),
                   std::vector<float> (static_cast<std::size_t> (sampleCount), 0.0f));
    constexpr int outputBlockSize = 16384;
    juce::AudioBuffer<float> sourceBlock;

    for (int outputStart = 0; outputStart < sampleCount;
         outputStart += outputBlockSize)
    {
        if (cancelRequested.load())
            return juce::Result::fail ("Advanced audio edit cancelled.");
        const auto amount = juce::jmin (outputBlockSize, sampleCount - outputStart);
        const auto logicalStart = context.clip.reversed
            ? static_cast<double> (sampleCount - outputStart - 1)
            : static_cast<double> (outputStart);
        const auto logicalEnd = context.clip.reversed
            ? static_cast<double> (sampleCount - outputStart - amount)
            : static_cast<double> (outputStart + amount - 1);
        const auto sourceAtStart = sourcePositionAt (context.anchors, logicalStart);
        const auto sourceAtEnd = sourcePositionAt (context.anchors, logicalEnd);
        const auto firstSource = juce::jmax (static_cast<juce::int64> (0),
            static_cast<juce::int64> (std::floor (
                juce::jmin (sourceAtStart, sourceAtEnd))) - 1);
        const auto lastSource = juce::jmin (reader.lengthInSamples - 1,
            static_cast<juce::int64> (std::ceil (
                juce::jmax (sourceAtStart, sourceAtEnd))) + 1);
        const auto sourceCount = static_cast<int> (
            juce::jmax (static_cast<juce::int64> (2),
                        lastSource - firstSource + 1));
        sourceBlock.setSize (channelCount, sourceCount, false, false, true);
        sourceBlock.clear();
        reader.read (&sourceBlock, 0, sourceCount, firstSource, true, true);

        for (int sample = 0; sample < amount; ++sample)
        {
            const auto timelinePosition = context.clip.reversed
                ? static_cast<double> (sampleCount - 1 - (outputStart + sample))
                : static_cast<double> (outputStart + sample);
            const auto sourcePosition = juce::jlimit (
                0.0, static_cast<double> (sourceCount - 1),
                sourcePositionAt (context.anchors, timelinePosition)
                    - static_cast<double> (firstSource));
            const auto index = juce::jlimit (
                0, sourceCount - 2, static_cast<int> (std::floor (sourcePosition)));
            const auto fraction = static_cast<float> (
                sourcePosition - static_cast<double> (index));
            for (int channel = 0; channel < channelCount; ++channel)
            {
                const auto left = sourceBlock.getSample (channel, index);
                const auto right = sourceBlock.getSample (channel, index + 1);
                result[static_cast<std::size_t> (channel)]
                      [static_cast<std::size_t> (outputStart + sample)]
                    = left + (right - left) * fraction;
            }
        }
        progress.store (0.35f * static_cast<float> (outputStart + amount)
                        / static_cast<float> (sampleCount));
    }
    return juce::Result::ok();
}
}

juce::Result AdvancedAudioRenderer::analyseClipPeak (
    const ProjectState& snapshot, const juce::String& trackId,
    const juce::String& clipId, std::atomic<bool>& cancelRequested,
    std::atomic<float>& progress, float& peak)
{
    progress.store (0.0f);
    peak = 0.0f;
    ClipContext context;
    if (! findClipContext (snapshot, trackId, clipId, context))
        return juce::Result::fail ("The selected audio clip is unavailable.");
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader;
    const auto readerResult = createReader (context, formats, reader);
    if (readerResult.failed())
        return readerResult;

    const auto firstSource = juce::jmax (static_cast<juce::int64> (0),
        static_cast<juce::int64> (context.anchors.front().source));
    const auto lastSource = juce::jmin (reader->lengthInSamples,
        static_cast<juce::int64> (context.anchors.back().source));
    const auto length = lastSource - firstSource;
    if (length <= 0)
        return juce::Result::fail ("The selected clip contains no readable audio.");
    constexpr int blockSize = 65536;
    const auto channels = juce::jlimit (1, 2, static_cast<int> (reader->numChannels));
    juce::AudioBuffer<float> block (channels, blockSize);
    for (juce::int64 position = 0; position < length; position += blockSize)
    {
        if (cancelRequested.load())
            return juce::Result::fail ("Advanced audio edit cancelled.");
        const auto amount = static_cast<int> (juce::jmin (
            static_cast<juce::int64> (blockSize), length - position));
        block.clear();
        reader->read (&block, 0, amount, firstSource + position, true, true);
        for (int channel = 0; channel < channels; ++channel)
            for (int sample = 0; sample < amount; ++sample)
                peak = juce::jmax (peak, std::abs (block.getSample (channel, sample)));
        progress.store (static_cast<float> (position + amount)
                        / static_cast<float> (length));
    }
    progress.store (1.0f);
    return juce::Result::ok();
}

juce::Result AdvancedAudioRenderer::renderPitchShiftedClip (
    const ProjectState& snapshot, const juce::String& trackId,
    const juce::String& clipId, double semitones,
    const juce::File& destination, std::atomic<bool>& cancelRequested,
    std::atomic<float>& progress, OutputInfo& outputInfo)
{
    progress.store (0.0f);
    if (std::abs (semitones) < 0.01 || std::abs (semitones) > 24.0)
        return juce::Result::fail (
            "Choose a pitch shift between -24 and +24 semitones.");
    ClipContext context;
    if (! findClipContext (snapshot, trackId, clipId, context))
        return juce::Result::fail ("The selected audio clip is unavailable.");
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader;
    const auto readerResult = createReader (context, formats, reader);
    if (readerResult.failed())
        return readerResult;

    advanced::AudioChannels input;
    const auto renderResult = renderClipToChannels (
        context, *reader, cancelRequested, progress, input);
    if (renderResult.failed())
        return renderResult;
    if (cancelRequested.load())
        return juce::Result::fail ("Advanced audio edit cancelled.");
    progress.store (0.4f);
    auto shifted = advanced::pitchShiftOla (input, semitones);
    progress.store (0.8f);

    const auto partial = destination.getSiblingFile (
        destination.getFileName() + ".partial");
    partial.deleteFile();
    std::unique_ptr<juce::OutputStream> stream (partial.createOutputStream());
    if (stream == nullptr)
        return juce::Result::fail ("The processed audio file could not be created.");
    juce::WavAudioFormat wav;
    auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions {}
        .withSampleRate (snapshot.timelineSampleRate)
        .withNumChannels (static_cast<int> (shifted.size()))
        .withBitsPerSample (24));
    if (writer == nullptr)
        return juce::Result::fail ("The processed WAV writer could not be created.");

    const auto sampleCount = static_cast<int> (shifted.front().size());
    constexpr int blockSize = 32768;
    juce::AudioBuffer<float> block (static_cast<int> (shifted.size()), blockSize);
    for (int position = 0; position < sampleCount; position += blockSize)
    {
        if (cancelRequested.load())
        {
            writer.reset();
            partial.deleteFile();
            return juce::Result::fail ("Advanced audio edit cancelled.");
        }
        const auto amount = juce::jmin (blockSize, sampleCount - position);
        block.clear();
        for (int channel = 0; channel < block.getNumChannels(); ++channel)
            block.copyFrom (channel, 0,
                shifted[static_cast<std::size_t> (channel)].data() + position,
                amount);
        if (! writer->writeFromAudioSampleBuffer (block, 0, amount))
        {
            writer.reset();
            partial.deleteFile();
            return juce::Result::fail ("The processed audio file could not be written.");
        }
        progress.store (0.8f + 0.2f * static_cast<float> (position + amount)
                                    / static_cast<float> (sampleCount));
    }
    writer.reset();
    if (! partial.moveFileTo (destination))
    {
        partial.deleteFile();
        return juce::Result::fail ("The processed audio file could not be finalized.");
    }
    outputInfo.sampleRate = snapshot.timelineSampleRate;
    outputInfo.channels = static_cast<int> (shifted.size());
    outputInfo.lengthInSamples = sampleCount;
    progress.store (1.0f);
    return juce::Result::ok();
}
}
