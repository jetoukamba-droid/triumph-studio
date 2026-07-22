#pragma once

#include <JuceHeader.h>

#include "core/RealtimeEngineState.h"

#include <atomic>
#include <memory>
#include <vector>

namespace triumph
{
class AudioTrack final
{
public:
    struct ClipPlayback
    {
        juce::String id;
        juce::String sourceId;
        juce::File sourceFile;
        juce::int64 timelineStartSamples = 0;
        juce::int64 sourceOffsetSamples = 0;
        juce::int64 lengthInTimelineSamples = 0;
        float gain = 1.0f;
        juce::int64 fadeInTimelineSamples = 0;
        juce::int64 fadeOutTimelineSamples = 0;
        double playbackRate = 1.0;
        bool reversed = false;
        juce::int64 fadeAnchorTimelineStartSamples = 0;
        juce::int64 fadeAnchorTimelineLengthSamples = 0;
    };

    struct RenderReport
    {
        int clipIntersections = 0;
        int renderedClipIntersections = 0;

        bool hadStarvation() const noexcept
        {
            return clipIntersections > renderedClipIntersections;
        }
    };

    AudioTrack (juce::String trackId,
                const juce::File& source,
                juce::AudioFormatManager& formatManager,
                juce::AudioThumbnailCache& thumbnailCache,
                juce::TimeSliceThread& readAheadThread,
                bool shouldBuildThumbnails = true);

    ~AudioTrack();

    bool isValid() const noexcept;
    const juce::String& getId() const noexcept;

    const juce::File& getSourceFile() const noexcept;
    const juce::String& getName() const noexcept;
    void setName (juce::String newName);

    double getLengthSeconds() const noexcept;
    double getPositionSeconds() const;
    int getSourceChannelCount() const noexcept;

    juce::AudioThumbnail& getThumbnail() noexcept;
    juce::AudioThumbnail* getThumbnailForSource (const juce::String& sourceId) noexcept;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate);
    void releaseResources();
    void setClips (std::vector<ClipPlayback> newClips, double projectSampleRate);
    bool hasSource (const juce::String& sourceId,
                    const juce::File& sourceFile) const noexcept;
    RenderReport renderTimelineBlock (juce::AudioBuffer<float>& destination,
                                      int numSamples,
                                      double timelineStartSeconds,
                                      double outputSampleRate,
                                      bool shouldRender,
                                      bool applyTrackControls = true);

    void play();
    void pause();
    void stopAndRewind();
    void setPositionSeconds (double seconds);
    bool isPlaying() const;

    void setGain (float newGain) noexcept;
    float getGain() const noexcept;

    void setPan (float newPan) noexcept;
    float getPan() const noexcept;

    void setMuted (bool shouldBeMuted) noexcept;
    bool isMuted() const noexcept;

    void setSolo (bool shouldBeSolo) noexcept;
    bool isSolo() const noexcept;

private:
    struct ClipSchedule
    {
        std::vector<ClipPlayback> clips;
        double timelineSampleRate = 48000.0;
    };

    struct SourcePlayback;
    SourcePlayback* findSource (const juce::String& sourceId,
                                const juce::File& sourceFile) noexcept;
    SourcePlayback* addSource (juce::String sourceId, const juce::File& sourceFile);

    const juce::String id;
    juce::File sourceFile;
    juce::String name;
    juce::AudioFormatManager& formatManager;
    juce::AudioThumbnailCache& thumbnailCache;
    juce::TimeSliceThread& readAheadThread;
    const bool buildThumbnails;
    std::vector<std::unique_ptr<SourcePlayback>> sources;
    juce::AudioBuffer<float> clipScratch;
    realtime::SnapshotExchange<ClipSchedule> clipSchedules;

    double lengthSeconds = 0.0;
    int sourceChannels = 0;
    int preparedBlockSize = 0;
    double preparedOutputSampleRate = 0.0;
    bool running = false;
    std::atomic<float> gain { 0.85f };
    std::atomic<float> pan { 0.0f };
    std::atomic<bool> muted { false };
    std::atomic<bool> solo { false };
    float previousLeftGain = 0.85f;
    float previousRightGain = 0.85f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioTrack)
};
}
