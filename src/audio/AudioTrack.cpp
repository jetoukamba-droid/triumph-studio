#include "AudioTrack.h"

#include "core/TimelineMath.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace triumph
{
namespace
{
class ReversedAudioReaderSource final : public juce::PositionableAudioSource
{
public:
    explicit ReversedAudioReaderSource (
        std::unique_ptr<juce::AudioFormatReader> sourceReader)
        : reader (std::move (sourceReader))
    {
    }

    void prepareToPlay (int samplesPerBlockExpected, double) override
    {
        scratch.setSize (juce::jmax (1, static_cast<int> (reader->numChannels)),
                         juce::jmax (32768, samplesPerBlockExpected),
                         false, false, true);
    }

    void releaseResources() override
    {
        scratch.setSize (0, 0);
    }

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& info) override
    {
        info.clearActiveBufferRegion();
        if (reader == nullptr || info.buffer == nullptr || info.numSamples <= 0)
            return;

        const auto start = nextPosition.fetch_add (info.numSamples);
        const auto remaining = juce::jmax (
            static_cast<juce::int64> (0), reader->lengthInSamples - start);
        const auto available = juce::jmin (
            remaining, static_cast<juce::int64> (info.numSamples));
        const auto count = static_cast<int> (available);
        if (count <= 0)
            return;
        if (scratch.getNumSamples() < count)
            scratch.setSize (scratch.getNumChannels(), count, false, false, true);
        scratch.clear();
        const auto originalStart = reader->lengthInSamples - start - count;
        reader->read (&scratch, 0, count, originalStart, true, true);
        for (int channel = 0; channel < info.buffer->getNumChannels(); ++channel)
        {
            const auto sourceChannel = juce::jmin (
                channel, scratch.getNumChannels() - 1);
            auto* destination = info.buffer->getWritePointer (
                channel, info.startSample);
            const auto* source = scratch.getReadPointer (sourceChannel);
            for (int sample = 0; sample < count; ++sample)
                destination[sample] = source[count - 1 - sample];
        }
    }

    void setNextReadPosition (juce::int64 position) override
    {
        nextPosition.store (juce::jlimit (static_cast<juce::int64> (0),
                                          getTotalLength(), position));
    }

    juce::int64 getNextReadPosition() const override
    {
        return nextPosition.load();
    }

    juce::int64 getTotalLength() const override
    {
        return reader != nullptr ? reader->lengthInSamples : 0;
    }

    bool isLooping() const override { return false; }

private:
    std::unique_ptr<juce::AudioFormatReader> reader;
    juce::AudioBuffer<float> scratch;
    std::atomic<juce::int64> nextPosition { 0 };
};
}

struct AudioTrack::SourcePlayback
{
    SourcePlayback (juce::String stableSourceId,
                    const juce::File& sourceFile,
                    juce::AudioFormatManager& formats,
                    juce::AudioThumbnailCache& cache,
                    juce::TimeSliceThread& thread,
                    bool shouldBuildThumbnail)
        : sourceId (std::move (stableSourceId)),
          file (sourceFile),
          thumbnail (512, formats, cache),
          varispeed (&transport, false, 2),
          reverseVarispeed (&reverseTransport, false, 2)
    {
        if (auto* reader = formats.createReaderFor (file))
        {
            sampleRate = reader->sampleRate;
            channels = static_cast<int> (reader->numChannels);
            lengthInSamples = reader->lengthInSamples;
            readerSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);
            transport.setSource (readerSource.get(), 32768, &thread, sampleRate);
            if (shouldBuildThumbnail)
                thumbnail.setSource (new juce::FileInputSource (file));
            if (auto* reverseReader = formats.createReaderFor (file))
            {
                reverseReaderSource = std::make_unique<ReversedAudioReaderSource> (
                    std::unique_ptr<juce::AudioFormatReader> (reverseReader));
                reverseTransport.setSource (reverseReaderSource.get(), 32768,
                                            &thread, sampleRate);
            }
        }
    }

    ~SourcePlayback()
    {
        transport.stop();
        transport.setSource (nullptr);
        reverseTransport.stop();
        reverseTransport.setSource (nullptr);
    }

    bool isValid() const noexcept
    {
        return readerSource != nullptr && sampleRate > 0.0 && lengthInSamples > 0;
    }

    juce::String sourceId;
    juce::File file;
    juce::AudioThumbnail thumbnail;
    juce::AudioTransportSource transport;
    juce::ResamplingAudioSource varispeed;
    juce::AudioTransportSource reverseTransport;
    juce::ResamplingAudioSource reverseVarispeed;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    std::unique_ptr<ReversedAudioReaderSource> reverseReaderSource;
    double sampleRate = 0.0;
    int channels = 0;
    juce::int64 lengthInSamples = 0;
    double lastVarispeedRatio = -1.0;
    double lastReverseVarispeedRatio = -1.0;
};

AudioTrack::AudioTrack (juce::String trackId,
                        const juce::File& source,
                        juce::AudioFormatManager& formatManager,
                        juce::AudioThumbnailCache& thumbnailCache,
                        juce::TimeSliceThread& readAheadThread,
                        bool shouldBuildThumbnails)
    : id (std::move (trackId)),
      sourceFile (source),
      name (source.getFileNameWithoutExtension()),
      formatManager (formatManager),
      thumbnailCache (thumbnailCache),
      readAheadThread (readAheadThread),
      buildThumbnails (shouldBuildThumbnails)
{
    if (auto* initial = addSource ({}, sourceFile))
    {
        sourceChannels = initial->channels;
        lengthSeconds = static_cast<double> (initial->lengthInSamples)
                        / initial->sampleRate;
    }
}

AudioTrack::~AudioTrack() = default;

bool AudioTrack::isValid() const noexcept
{
    return ! sources.empty() && sources.front()->isValid();
}

const juce::String& AudioTrack::getId() const noexcept
{
    return id;
}

const juce::File& AudioTrack::getSourceFile() const noexcept
{
    return sourceFile;
}

const juce::String& AudioTrack::getName() const noexcept
{
    return name;
}

void AudioTrack::setName (juce::String newName)
{
    newName = newName.trim();
    name = newName.isNotEmpty() ? std::move (newName) : sourceFile.getFileNameWithoutExtension();
}

double AudioTrack::getLengthSeconds() const noexcept
{
    return lengthSeconds;
}

double AudioTrack::getPositionSeconds() const
{
    return ! sources.empty() ? sources.front()->transport.getCurrentPosition() : 0.0;
}

int AudioTrack::getSourceChannelCount() const noexcept
{
    return sourceChannels;
}

juce::AudioThumbnail& AudioTrack::getThumbnail() noexcept
{
    jassert (! sources.empty());
    return sources.front()->thumbnail;
}

juce::AudioThumbnail* AudioTrack::getThumbnailForSource (
    const juce::String& sourceId) noexcept
{
    if (auto* source = findSource (sourceId, {}))
        return &source->thumbnail;
    return nullptr;
}

void AudioTrack::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    preparedBlockSize = samplesPerBlockExpected;
    preparedOutputSampleRate = sampleRate;
    for (auto& source : sources)
    {
        source->varispeed.prepareToPlay (samplesPerBlockExpected, sampleRate);
        source->reverseVarispeed.prepareToPlay (samplesPerBlockExpected, sampleRate);
    }
    clipScratch.setSize (2, samplesPerBlockExpected, false, true, true);
}

void AudioTrack::releaseResources()
{
    for (auto& source : sources)
    {
        source->varispeed.releaseResources();
        source->reverseVarispeed.releaseResources();
    }
    clipScratch.setSize (0, 0);
    preparedBlockSize = 0;
    preparedOutputSampleRate = 0.0;
}

void AudioTrack::setClips (std::vector<ClipPlayback> newClips,
                           double projectSampleRate)
{
    for (const auto& clip : newClips)
        if (findSource (clip.sourceId, clip.sourceFile) == nullptr)
            addSource (clip.sourceId, clip.sourceFile);

    auto schedule = std::make_unique<ClipSchedule>();
    schedule->clips = std::move (newClips);
    schedule->timelineSampleRate = juce::jmax (1.0, projectSampleRate);
    clipSchedules.publish (std::move (schedule));
}

bool AudioTrack::hasSource (const juce::String& sourceId,
                            const juce::File& requestedFile) const noexcept
{
    for (const auto& source : sources)
    {
        if (sourceId.isNotEmpty() && source->sourceId == sourceId)
            return true;
        if (requestedFile != juce::File() && source->file == requestedFile
            && (sourceId.isEmpty() || source->sourceId.isEmpty()
                || source->sourceId == sourceId))
            return true;
    }
    return false;
}

AudioTrack::SourcePlayback* AudioTrack::findSource (
    const juce::String& sourceId,
    const juce::File& requestedFile) noexcept
{
    for (auto& source : sources)
    {
        if (sourceId.isNotEmpty() && source->sourceId == sourceId)
            return source.get();
        if (requestedFile != juce::File() && source->file == requestedFile
            && (sourceId.isEmpty() || source->sourceId.isEmpty()))
        {
            if (source->sourceId.isEmpty())
                source->sourceId = sourceId;
            return source.get();
        }
    }
    return nullptr;
}

AudioTrack::SourcePlayback* AudioTrack::addSource (
    juce::String sourceId,
    const juce::File& requestedFile)
{
    if (! requestedFile.existsAsFile())
        return nullptr;

    auto source = std::make_unique<SourcePlayback> (
        std::move (sourceId), requestedFile, formatManager,
        thumbnailCache, readAheadThread, buildThumbnails);
    if (! source->isValid())
        return nullptr;

    if (preparedOutputSampleRate > 0.0 && preparedBlockSize > 0)
    {
        source->varispeed.prepareToPlay (preparedBlockSize,
                                         preparedOutputSampleRate);
        source->reverseVarispeed.prepareToPlay (preparedBlockSize,
                                                preparedOutputSampleRate);
    }
    if (running)
    {
        source->transport.start();
        source->reverseTransport.start();
    }
    auto* result = source.get();
    sources.push_back (std::move (source));
    lengthSeconds = juce::jmax (
        lengthSeconds,
        static_cast<double> (result->lengthInSamples) / result->sampleRate);
    return result;
}

AudioTrack::RenderReport AudioTrack::renderTimelineBlock (
    juce::AudioBuffer<float>& destination,
    int numSamples,
    double timelineStartSeconds,
    double outputSampleRate,
    bool shouldRender,
    bool applyTrackControls)
{
    RenderReport report;
    destination.clear();

    auto clipSchedule = clipSchedules.acquire();
    if (! clipSchedule)
    {
        previousLeftGain = applyTrackControls ? 0.0f : 1.0f;
        previousRightGain = applyTrackControls ? 0.0f : 1.0f;
        return report;
    }
    const auto& clips = clipSchedule->clips;
    const auto timelineSampleRate = clipSchedule->timelineSampleRate;

    if (! shouldRender || outputSampleRate <= 0.0
        || clipScratch.getNumSamples() < numSamples)
    {
        previousLeftGain = applyTrackControls ? 0.0f : 1.0f;
        previousRightGain = applyTrackControls ? 0.0f : 1.0f;
        return report;
    }

    const auto blockEndSeconds = timelineStartSeconds
                                 + static_cast<double> (numSamples) / outputSampleRate;

    for (const auto& clip : clips)
    {
        const auto clipStartSeconds = timeline::samplesToSeconds (
            clip.timelineStartSamples, timelineSampleRate);
        const auto clipEndSeconds = clipStartSeconds + timeline::samplesToSeconds (
            clip.lengthInTimelineSamples, timelineSampleRate);
        const auto intersectionStart = juce::jmax (timelineStartSeconds, clipStartSeconds);
        const auto intersectionEnd = juce::jmin (blockEndSeconds, clipEndSeconds);

        if (intersectionEnd <= intersectionStart)
            continue;

        const auto destinationStart = juce::jlimit (
            0,
            numSamples,
            juce::roundToInt ((intersectionStart - timelineStartSeconds)
                              * outputSampleRate));
        const auto destinationEnd = juce::jlimit (
            destinationStart,
            numSamples,
            juce::roundToInt ((intersectionEnd - timelineStartSeconds)
                              * outputSampleRate));
        const auto samplesToRender = destinationEnd - destinationStart;

        if (samplesToRender <= 0)
            continue;
        ++report.clipIntersections;

        auto* source = findSource (clip.sourceId, clip.sourceFile);
        if (source == nullptr || ! source->isValid())
            continue;

        auto& activeTransport = clip.reversed ? source->reverseTransport
                                              : source->transport;
        auto& activeVarispeed = clip.reversed ? source->reverseVarispeed
                                              : source->varispeed;
        const auto sourceSpan = clipEndSeconds - clipStartSeconds;
        const auto expectedSourcePosition = clip.reversed
            ? timeline::samplesToSeconds (
                juce::jmax (static_cast<juce::int64> (0),
                    source->lengthInSamples - clip.sourceOffsetSamples
                    - static_cast<juce::int64> (std::llround (
                        sourceSpan * clip.playbackRate * source->sampleRate))),
                source->sampleRate)
                + (intersectionStart - clipStartSeconds) * clip.playbackRate
            : timeline::samplesToSeconds (
                clip.sourceOffsetSamples, source->sampleRate)
                + (intersectionStart - clipStartSeconds) * clip.playbackRate;

        const auto resamplingRatio = juce::jlimit (0.25, 4.0,
                                                   clip.playbackRate);
        auto& lastResamplingRatio = clip.reversed
            ? source->lastReverseVarispeedRatio
            : source->lastVarispeedRatio;
        activeVarispeed.setResamplingRatio (resamplingRatio);
        if (std::abs (lastResamplingRatio - resamplingRatio) > 0.000001)
        {
            activeVarispeed.flushBuffers();
            lastResamplingRatio = resamplingRatio;
        }

        if (std::abs (activeTransport.getCurrentPosition() - expectedSourcePosition)
            > 2.0 / outputSampleRate)
        {
            activeTransport.setPosition (expectedSourcePosition);
            activeVarispeed.flushBuffers();
        }

        clipScratch.clear();
        juce::AudioSourceChannelInfo clipBlock (&clipScratch,
                                                destinationStart,
                                                samplesToRender);
        activeVarispeed.getNextAudioBlock (clipBlock);
        ++report.renderedClipIntersections;

        if (source->channels == 1 && clipScratch.getNumChannels() > 1)
            clipScratch.copyFrom (1,
                                  destinationStart,
                                  clipScratch,
                                  0,
                                  destinationStart,
                                  samplesToRender);

        if (clip.fadeInTimelineSamples > 0 || clip.fadeOutTimelineSamples > 0)
        {
            const auto fadeAnchorStartSamples =
                clip.fadeAnchorTimelineLengthSamples > 0
                    ? clip.fadeAnchorTimelineStartSamples
                    : clip.timelineStartSamples;
            const auto fadeAnchorLengthSamples =
                clip.fadeAnchorTimelineLengthSamples > 0
                    ? clip.fadeAnchorTimelineLengthSamples
                    : clip.lengthInTimelineSamples;
            const auto fadeAnchorStartSeconds = timeline::samplesToSeconds (
                fadeAnchorStartSamples, timelineSampleRate);
            const auto clipOutputLength = juce::jmax (
                1, juce::roundToInt (timeline::samplesToSeconds (
                    fadeAnchorLengthSamples, timelineSampleRate)
                    * outputSampleRate));
            const auto fadeInLength = juce::jmax (
                0, juce::roundToInt (timeline::samplesToSeconds (
                    clip.fadeInTimelineSamples, timelineSampleRate)
                    * outputSampleRate));
            const auto fadeOutLength = juce::jmax (
                0, juce::roundToInt (timeline::samplesToSeconds (
                    clip.fadeOutTimelineSamples, timelineSampleRate)
                    * outputSampleRate));
            const auto clipOutputOffset = juce::jmax (
                0, juce::roundToInt ((intersectionStart - fadeAnchorStartSeconds)
                                     * outputSampleRate));
            for (int sample = 0; sample < samplesToRender; ++sample)
            {
                const auto position = clipOutputOffset + sample;
                auto fadeGain = 1.0f;
                if (fadeInLength > 0 && position < fadeInLength)
                    fadeGain *= timeline::equalPowerFadeIn (position, fadeInLength);
                if (fadeOutLength > 0
                    && position >= clipOutputLength - fadeOutLength)
                    fadeGain *= timeline::equalPowerFadeOut (
                        position - (clipOutputLength - fadeOutLength),
                        fadeOutLength);
                for (int channel = 0; channel < clipScratch.getNumChannels(); ++channel)
                    clipScratch.getWritePointer (channel)[destinationStart + sample]
                        *= fadeGain;
            }
        }

        for (int channel = 0;
             channel < juce::jmin (destination.getNumChannels(),
                                   clipScratch.getNumChannels());
             ++channel)
            destination.addFrom (channel,
                                 destinationStart,
                                 clipScratch,
                                 channel,
                                 destinationStart,
                                 samplesToRender,
                                 juce::jmax (0.0f, clip.gain));
    }

    if (! applyTrackControls)
    {
        previousLeftGain = 1.0f;
        previousRightGain = 1.0f;
        return report;
    }

    const auto trackGain = gain.load();
    const auto balance = timeline::balanceForPan (pan.load());
    const auto leftGain = trackGain * balance.left;
    const auto rightGain = trackGain * balance.right;

    if (destination.getNumChannels() > 0)
        destination.applyGainRamp (0, 0, numSamples, previousLeftGain, leftGain);

    if (destination.getNumChannels() > 1)
        destination.applyGainRamp (1, 0, numSamples, previousRightGain, rightGain);

    for (int channel = 2; channel < destination.getNumChannels(); ++channel)
        destination.applyGain (channel, 0, numSamples, trackGain);

    previousLeftGain = leftGain;
    previousRightGain = rightGain;
    return report;
}

void AudioTrack::play()
{
    running = true;
    for (auto& source : sources)
    {
        source->transport.start();
        source->reverseTransport.start();
    }
}

void AudioTrack::pause()
{
    running = false;
    for (auto& source : sources)
    {
        source->transport.stop();
        source->reverseTransport.stop();
    }
}

void AudioTrack::stopAndRewind()
{
    running = false;
    for (auto& source : sources)
    {
        source->transport.stop();
        source->transport.setPosition (0.0);
        source->varispeed.flushBuffers();
        source->reverseTransport.stop();
        source->reverseTransport.setPosition (0.0);
        source->reverseVarispeed.flushBuffers();
    }
}

void AudioTrack::setPositionSeconds (double seconds)
{
    for (auto& source : sources)
    {
        source->transport.setPosition (timeline::clampSeconds (
            seconds,
            static_cast<double> (source->lengthInSamples) / source->sampleRate));
        source->varispeed.flushBuffers();
        source->reverseTransport.setPosition (timeline::clampSeconds (
            seconds,
            static_cast<double> (source->lengthInSamples) / source->sampleRate));
        source->reverseVarispeed.flushBuffers();
    }
}

bool AudioTrack::isPlaying() const
{
    return running;
}

void AudioTrack::setGain (float newGain) noexcept
{
    gain.store (juce::jlimit (0.0f, 1.5f, newGain));
}

float AudioTrack::getGain() const noexcept
{
    return gain.load();
}

void AudioTrack::setPan (float newPan) noexcept
{
    pan.store (juce::jlimit (-1.0f, 1.0f, newPan));
}

float AudioTrack::getPan() const noexcept
{
    return pan.load();
}

void AudioTrack::setMuted (bool shouldBeMuted) noexcept
{
    muted.store (shouldBeMuted);
}

bool AudioTrack::isMuted() const noexcept
{
    return muted.load();
}

void AudioTrack::setSolo (bool shouldBeSolo) noexcept
{
    solo.store (shouldBeSolo);
}

bool AudioTrack::isSolo() const noexcept
{
    return solo.load();
}
}
