#include "AudioRecorder.h"

#include <array>
#include <cmath>
#include <thread>

namespace triumph
{
AudioRecorder::AudioRecorder (juce::TimeSliceThread* sharedWriterThread)
    : writerThread (sharedWriterThread != nullptr
                        ? sharedWriterThread : &ownedWriterThread),
      ownsWriterThread (sharedWriterThread == nullptr)
{
    if (ownsWriterThread)
        writerThread->startThread();
}

AudioRecorder::~AudioRecorder()
{
    stop();
    if (ownsWriterThread)
        writerThread->stopThread (5000);
}

juce::Result AudioRecorder::start (const juce::File& destination,
                                   double sampleRate,
                                   int channels,
                                   JournalContext journal)
{
    stop();
    stateMachine.reset();

    if (sampleRate <= 0.0 || channels <= 0 || channels > 2)
    {
        stateMachine.abort();
        return juce::Result::fail ("The selected audio input configuration is invalid.");
    }

    if (destination.getParentDirectory().createDirectory().failed())
    {
        stateMachine.abort();
        return juce::Result::fail ("The Recordings folder could not be created.");
    }

    destination.deleteFile();
    std::unique_ptr<juce::OutputStream> stream (destination.createOutputStream());
    if (stream == nullptr)
    {
        stateMachine.abort();
        return juce::Result::fail ("The recording file could not be opened for writing.");
    }

    juce::WavAudioFormat wav;
    const auto options = juce::AudioFormatWriterOptions {}
                             .withSampleRate (sampleRate)
                             .withNumChannels (channels)
                             .withBitsPerSample (24);
    auto writer = wav.createWriterFor (stream, options);
    if (writer == nullptr)
    {
        stateMachine.abort();
        return juce::Result::fail ("A 24-bit WAV writer could not be created.");
    }

    auto backgroundWriter = std::make_unique<juce::AudioFormatWriter::ThreadedWriter> (
        writer.release(), *writerThread, 65536);
    // WavAudioFormatWriter::flush rewrites the RIFF/data lengths. Doing this on
    // the background writer thread once per second leaves a readable checkpoint
    // if the process terminates before normal finalization.
    backgroundWriter->setFlushInterval (juce::jmax (
        1, static_cast<int> (std::llround (sampleRate))));

    if (! stateMachine.arm())
    {
        stateMachine.abort();
        return juce::Result::fail ("The recording state could not be armed.");
    }

    if (journal.isEnabled())
    {
        journal.entry.audioFile = destination;
        journal.entry.sampleRate = sampleRate;
        journal.entry.channels = channels;
        if (journal.entry.status.isEmpty())
            journal.entry.status = "recording";
        journal.entry.committedSamples = 0;
        journal.entry.droppedBlocks = 0;
        journal.entry.droppedSamples = 0;
        if (journal.entry.createdAtMilliseconds <= 0)
            journal.entry.createdAtMilliseconds =
                juce::Time::currentTimeMillis();
        const auto journalResult = RecordingJournal::write (
            journal.journalFile, journal.entry);
        if (journalResult.failed())
        {
            stateMachine.abort();
            backgroundWriter.reset();
            destination.deleteFile();
            return journalResult;
        }
    }

    {
        const juce::ScopedLock lock (writerLock);
        threadedWriter = std::move (backgroundWriter);
        activeFile = destination;
        activeSampleRate = sampleRate;
        activeChannels = channels;
        samplesRecorded.store (0);
        droppedBlocks.store (0);
        droppedSamples.store (0);
        activeJournal = std::move (journal);
        lastJournalCheckpointSamples = 0;
        activeWriter.store (threadedWriter.get(), std::memory_order_release);
    }

    if (! stateMachine.beginCapture())
    {
        stop();
        stateMachine.abort();
        return juce::Result::fail ("The recording state could not enter capture.");
    }

    return juce::Result::ok();
}

void AudioRecorder::processInput (const juce::AudioBuffer<float>& input,
                                  int startSample,
                                  int numSamples,
                                  int firstInputChannel) noexcept
{
    if (numSamples <= 0)
        return;

    writerReaders.fetch_add (1, std::memory_order_seq_cst);
    auto* writer = activeWriter.load (std::memory_order_seq_cst);

    if (writer == nullptr)
    {
        writerReaders.fetch_sub (1, std::memory_order_seq_cst);
        return;
    }

    if (firstInputChannel < 0
        || input.getNumChannels() < firstInputChannel + activeChannels)
    {
        droppedBlocks.fetch_add (1);
        droppedSamples.fetch_add (numSamples);
        writerReaders.fetch_sub (1, std::memory_order_seq_cst);
        return;
    }

    std::array<const float*, 2> channels {};
    for (int channel = 0; channel < activeChannels; ++channel)
        channels[static_cast<size_t> (channel)] = input.getReadPointer (
            firstInputChannel + channel, startSample);

    if (writer->write (channels.data(), numSamples))
        samplesRecorded.fetch_add (numSamples);
    else
    {
        droppedBlocks.fetch_add (1);
        droppedSamples.fetch_add (numSamples);
    }
    writerReaders.fetch_sub (1, std::memory_order_seq_cst);
}

AudioRecorder::Result AudioRecorder::stop()
{
    Result result;

    if (activeWriter.load (std::memory_order_acquire) != nullptr)
        stateMachine.beginFinalize();

    JournalContext journal;
    {
        const juce::ScopedLock lock (writerLock);
        activeWriter.store (nullptr, std::memory_order_seq_cst);
        while (writerReaders.load (std::memory_order_seq_cst) != 0)
            std::this_thread::yield();
        result.file = activeFile;
        result.sampleRate = activeSampleRate;
        result.channels = activeChannels;
        result.samples = samplesRecorded.load();
        result.droppedBlocks = droppedBlocks.load();
        result.droppedSamples = droppedSamples.load();
        threadedWriter.reset();
        journal = activeJournal;
        activeJournal = {};
        lastJournalCheckpointSamples = 0;
    }
    if (stateMachine.get() == recording::State::finalizing)
        stateMachine.finishFinalize (result.isValid());
    if (journal.isEnabled())
    {
        if (result.isValid())
            RecordingJournal::remove (journal.journalFile);
        else
        {
            journal.entry.status = "aborted";
            journal.entry.committedSamples = result.samples;
            journal.entry.droppedBlocks = result.droppedBlocks;
            journal.entry.droppedSamples = result.droppedSamples;
            RecordingJournal::write (journal.journalFile, journal.entry);
        }
    }
    activeFile = juce::File {};
    activeSampleRate = 0.0;
    activeChannels = 0;
    samplesRecorded.store (0);
    droppedBlocks.store (0);
    droppedSamples.store (0);
    return result;
}

juce::Result AudioRecorder::checkpointJournal()
{
    JournalContext journal;
    {
        const juce::ScopedLock lock (writerLock);
        if (! activeJournal.isEnabled() || threadedWriter == nullptr)
            return juce::Result::ok();
        const auto captured = samplesRecorded.load (std::memory_order_relaxed);
        const auto interval = juce::jmax (
            static_cast<juce::int64> (1),
            static_cast<juce::int64> (std::llround (activeSampleRate)));
        if (captured - lastJournalCheckpointSamples < interval)
            return juce::Result::ok();
        lastJournalCheckpointSamples = captured;
        activeJournal.entry.committedSamples = captured;
        activeJournal.entry.droppedBlocks = droppedBlocks.load (
            std::memory_order_relaxed);
        activeJournal.entry.droppedSamples = droppedSamples.load (
            std::memory_order_relaxed);
        journal = activeJournal;
    }
    return RecordingJournal::write (journal.journalFile, journal.entry);
}

juce::Result AudioRecorder::setJournalStatus (const juce::String& status)
{
    JournalContext journal;
    {
        const juce::ScopedLock lock (writerLock);
        if (! activeJournal.isEnabled() || threadedWriter == nullptr)
            return juce::Result::ok();
        activeJournal.entry.status = status;
        journal = activeJournal;
    }
    return RecordingJournal::write (journal.journalFile, journal.entry);
}

bool AudioRecorder::isRecording() const noexcept
{
    return stateMachine.isCapturing()
           && activeWriter.load (std::memory_order_acquire) != nullptr;
}

recording::State AudioRecorder::getState() const noexcept
{
    return stateMachine.get();
}

juce::int64 AudioRecorder::getSamplesRecorded() const noexcept
{
    return samplesRecorded.load();
}

int AudioRecorder::getDroppedBlockCount() const noexcept
{
    return droppedBlocks.load();
}

juce::int64 AudioRecorder::getDroppedSampleCount() const noexcept
{
    return droppedSamples.load();
}
}
