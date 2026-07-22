#include "audio/RecordingJournal.h"
#include "TestSupport.h"

int main()
{
    using namespace triumph;

    auto directory = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-recording-journal-test", juce::String {}, false);
    REQUIRE (directory.createDirectory().wasOk());
    const auto journalFile = directory.getChildFile (
        "session.recording.json");

    RecordingJournalEntry source;
    source.sessionId = "session-1";
    source.projectId = "project-1";
    source.trackId = "track-1";
    source.tapPoint = "hardware-input";
    source.passNumber = 3;
    source.inputStartChannel = 2;
    source.projectFile = directory.getChildFile ("project.triumph");
    source.audioFile = directory.getChildFile ("take.wav");
    source.sampleRate = 48000.0;
    source.channels = 2;
    source.timelineStartSamples = 24000;
    source.committedSamples = 96000;
    source.droppedBlocks = 2;
    source.droppedSamples = 512;
    source.createdAtMilliseconds = 1234567;

    REQUIRE (RecordingJournal::write (journalFile, source).wasOk());
    REQUIRE (journalFile.existsAsFile());
    const auto found = RecordingJournal::find (directory);
    REQUIRE (found.size() == 1);
    REQUIRE (found.getFirst() == journalFile);

    RecordingJournalEntry loaded;
    REQUIRE (RecordingJournal::read (journalFile, loaded).wasOk());
    REQUIRE (loaded.sessionId == source.sessionId);
    REQUIRE (loaded.projectId == source.projectId);
    REQUIRE (loaded.trackId == source.trackId);
    REQUIRE (loaded.audioFile == source.audioFile);
    REQUIRE (loaded.passNumber == 3);
    REQUIRE (loaded.inputStartChannel == 2);
    REQUIRE (loaded.sampleRate == 48000.0);
    REQUIRE (loaded.channels == 2);
    REQUIRE (loaded.timelineStartSamples == 24000);
    REQUIRE (loaded.committedSamples == 96000);
    REQUIRE (loaded.droppedBlocks == 2);
    REQUIRE (loaded.droppedSamples == 512);

    auto legacyValue = juce::JSON::parse (journalFile.loadFileAsString());
    auto* legacyObject = legacyValue.getDynamicObject();
    REQUIRE (legacyObject != nullptr);
    legacyObject->setProperty ("formatVersion", 1);
    legacyObject->removeProperty ("passNumber");
    legacyObject->removeProperty ("inputStartChannel");
    REQUIRE (journalFile.replaceWithText (
        juce::JSON::toString (legacyValue, true)));
    RecordingJournalEntry legacyLoaded;
    REQUIRE (RecordingJournal::read (journalFile, legacyLoaded).wasOk());
    REQUIRE (legacyLoaded.passNumber == 1);
    REQUIRE (legacyLoaded.inputStartChannel == 0);
    REQUIRE (RecordingJournal::remove (journalFile));
    REQUIRE (! journalFile.existsAsFile());
    REQUIRE (directory.deleteRecursively());
    return 0;
}
