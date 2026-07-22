#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include "RecordingJournal.h"
#include "core/RecordingStateMachine.h"

#include <atomic>
#include <cstdint>
#include <memory>

namespace triumph
{
class AudioRecorder final
{
public:
    struct JournalContext
    {
        juce::File journalFile;
        RecordingJournalEntry entry;

        bool isEnabled() const noexcept
        {
            return journalFile != juce::File();
        }
    };

    struct Result
    {
        juce::File file;
        double sampleRate = 0.0;
        int channels = 0;
        juce::int64 samples = 0;
        int droppedBlocks = 0;
        juce::int64 droppedSamples = 0;

        bool isValid() const noexcept
        {
            return file.existsAsFile() && sampleRate > 0.0
                   && channels > 0 && samples > 0;
        }
    };

    explicit AudioRecorder (juce::TimeSliceThread* sharedWriterThread = nullptr);
    ~AudioRecorder();

    juce::Result start (const juce::File& destination,
                        double sampleRate,
                        int channels,
                        JournalContext journal = {});
    void processInput (const juce::AudioBuffer<float>& input,
                       int startSample,
                       int numSamples,
                       int firstInputChannel = 0) noexcept;
    Result stop();
    juce::Result checkpointJournal();
    juce::Result setJournalStatus (const juce::String& status);

    bool isRecording() const noexcept;
    recording::State getState() const noexcept;
    juce::int64 getSamplesRecorded() const noexcept;
    int getDroppedBlockCount() const noexcept;
    juce::int64 getDroppedSampleCount() const noexcept;

private:
    juce::TimeSliceThread ownedWriterThread { "Triumph Studio recording writer" };
    juce::TimeSliceThread* writerThread = nullptr;
    bool ownsWriterThread = false;
    juce::CriticalSection writerLock;
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
    std::atomic<std::uint32_t> writerReaders { 0 };
    std::atomic<juce::int64> samplesRecorded { 0 };
    std::atomic<int> droppedBlocks { 0 };
    std::atomic<juce::int64> droppedSamples { 0 };
    juce::File activeFile;
    double activeSampleRate = 0.0;
    int activeChannels = 0;
    recording::StateMachine stateMachine;
    JournalContext activeJournal;
    juce::int64 lastJournalCheckpointSamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioRecorder)
};
}
