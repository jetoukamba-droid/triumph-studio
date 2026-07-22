#pragma once

#include <juce_core/juce_core.h>

namespace triumph
{
struct RecordingJournalEntry
{
    juce::String sessionId;
    juce::String projectId;
    juce::String trackId;
    juce::String tapPoint { "hardware-input" };
    juce::String status { "recording" };
    int passNumber = 1;
    int inputStartChannel = 0;
    juce::File projectFile;
    juce::File audioFile;
    double sampleRate = 0.0;
    int channels = 0;
    juce::int64 timelineStartSamples = 0;
    juce::int64 committedSamples = 0;
    int droppedBlocks = 0;
    juce::int64 droppedSamples = 0;
    juce::int64 createdAtMilliseconds = 0;

    bool isValid() const noexcept
    {
        return sessionId.isNotEmpty() && projectId.isNotEmpty()
               && trackId.isNotEmpty() && audioFile != juce::File()
               && sampleRate > 0.0 && channels > 0;
    }
};

class RecordingJournal final
{
public:
    static juce::Result write (const juce::File& journalFile,
                               const RecordingJournalEntry& entry);
    static juce::Result read (const juce::File& journalFile,
                              RecordingJournalEntry& entry);
    static bool remove (const juce::File& journalFile);
    static juce::Array<juce::File> find (const juce::File& directory);
};
}
