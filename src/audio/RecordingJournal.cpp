#include "RecordingJournal.h"

namespace triumph
{
juce::Result RecordingJournal::write (const juce::File& journalFile,
                                      const RecordingJournalEntry& entry)
{
    if (! entry.isValid())
        return juce::Result::fail ("The recording journal metadata is incomplete.");
    if (journalFile.getParentDirectory().createDirectory().failed())
        return juce::Result::fail ("The recording journal folder could not be created.");

    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("format", "TriumphRecordingJournal");
    object->setProperty ("formatVersion", 2);
    object->setProperty ("sessionId", entry.sessionId);
    object->setProperty ("projectId", entry.projectId);
    object->setProperty ("trackId", entry.trackId);
    object->setProperty ("tapPoint", entry.tapPoint);
    object->setProperty ("status", entry.status);
    object->setProperty ("passNumber", entry.passNumber);
    object->setProperty ("inputStartChannel", entry.inputStartChannel);
    object->setProperty ("projectFile", entry.projectFile.getFullPathName());
    object->setProperty ("audioFile", entry.audioFile.getFullPathName());
    object->setProperty ("sampleRate", entry.sampleRate);
    object->setProperty ("channels", entry.channels);
    object->setProperty ("timelineStartSamples", entry.timelineStartSamples);
    object->setProperty ("committedSamples", entry.committedSamples);
    object->setProperty ("droppedBlocks", entry.droppedBlocks);
    object->setProperty ("droppedSamples", entry.droppedSamples);
    object->setProperty ("createdAtMilliseconds", entry.createdAtMilliseconds);

    juce::TemporaryFile temporary (journalFile);
    if (! temporary.getFile().replaceWithText (
            juce::JSON::toString (juce::var (object.release()), true)))
        return juce::Result::fail ("The recording journal could not be written.");
    if (! temporary.overwriteTargetFileWithTemporary())
        return juce::Result::fail ("The recording journal could not be replaced safely.");
    return juce::Result::ok();
}

juce::Result RecordingJournal::read (const juce::File& journalFile,
                                     RecordingJournalEntry& entry)
{
    if (! journalFile.existsAsFile())
        return juce::Result::fail ("The recording journal does not exist.");
    const auto value = juce::JSON::parse (journalFile.loadFileAsString());
    const auto* object = value.getDynamicObject();
    if (object == nullptr
        || object->getProperty ("format").toString()
               != "TriumphRecordingJournal"
        || (static_cast<int> (object->getProperty ("formatVersion")) != 1
            && static_cast<int> (object->getProperty ("formatVersion")) != 2))
        return juce::Result::fail ("The recording journal format is invalid.");

    RecordingJournalEntry loaded;
    loaded.sessionId = object->getProperty ("sessionId").toString();
    loaded.projectId = object->getProperty ("projectId").toString();
    loaded.trackId = object->getProperty ("trackId").toString();
    loaded.tapPoint = object->getProperty ("tapPoint").toString();
    loaded.status = object->getProperty ("status").toString();
    loaded.passNumber = juce::jmax (
        1, static_cast<int> (object->getProperty ("passNumber")));
    loaded.inputStartChannel = juce::jmax (
        0, static_cast<int> (object->getProperty ("inputStartChannel")));
    loaded.projectFile = juce::File (
        object->getProperty ("projectFile").toString());
    loaded.audioFile = juce::File (
        object->getProperty ("audioFile").toString());
    loaded.sampleRate = static_cast<double> (object->getProperty ("sampleRate"));
    loaded.channels = static_cast<int> (object->getProperty ("channels"));
    loaded.timelineStartSamples = static_cast<juce::int64> (
        object->getProperty ("timelineStartSamples"));
    loaded.committedSamples = static_cast<juce::int64> (
        object->getProperty ("committedSamples"));
    loaded.droppedBlocks = static_cast<int> (
        object->getProperty ("droppedBlocks"));
    loaded.droppedSamples = static_cast<juce::int64> (
        object->getProperty ("droppedSamples"));
    loaded.createdAtMilliseconds = static_cast<juce::int64> (
        object->getProperty ("createdAtMilliseconds"));
    if (! loaded.isValid())
        return juce::Result::fail ("The recording journal metadata is incomplete.");
    entry = std::move (loaded);
    return juce::Result::ok();
}

bool RecordingJournal::remove (const juce::File& journalFile)
{
    return ! journalFile.existsAsFile() || journalFile.deleteFile();
}

juce::Array<juce::File> RecordingJournal::find (const juce::File& directory)
{
    juce::Array<juce::File> result;
    directory.findChildFiles (result, juce::File::findFiles, false,
                              "*.recording.json");
    return result;
}
}
