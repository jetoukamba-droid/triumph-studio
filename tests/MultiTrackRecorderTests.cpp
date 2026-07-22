#include "audio/MultiTrackRecorder.h"

#include "TestSupport.h"

#include <cmath>

int main()
{
    using namespace triumph;

    const auto directory = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-multitrack-recorder-test", juce::String(), false);
    const auto recordings = directory.getChildFile ("Recordings");
    const auto journals = directory.getChildFile ("Journals");
    REQUIRE (recordings.createDirectory().wasOk());
    REQUIRE (journals.createDirectory().wasOk());

    juce::AudioBuffer<float> inputs (2, 512);
    juce::AudioBuffer<float> program (2, 512);
    for (int sample = 0; sample < 512; ++sample)
    {
        inputs.setSample (0, sample, 0.25f);
        inputs.setSample (1, sample, -0.5f);
        program.setSample (0, sample, 0.1f);
        program.setSample (1, sample, -0.1f);
    }

    MultiTrackRecorder recorder;
    MultiTrackRecorder::StartContext context;
    context.recordingsDirectory = recordings;
    context.journalDirectory = journals;
    context.projectFile = directory.getChildFile ("project.triumph");
    context.projectId = "project-1";
    context.sessionId = "session-1";
    context.fileStem = "Take-test";
    context.sampleRate = 48000.0;
    context.timelineStartSamples = 24000;
    context.availableInputChannels = 2;
    context.availableOutputChannels = 2;

    std::vector<MultiTrackRecorder::Target> targets {
        { "track-1", 0, 1, RecordingTapPoint::hardwareInput, -32 },
        { "track-2", 0, 2, RecordingTapPoint::programOutput, 64 }
    };
    REQUIRE (recorder.start (targets, context).wasOk());
    recorder.processSpan (inputs, program, 0, 0, 256, 1);
    recorder.processSpan (inputs, program, 256, 256, 256, 1);
    auto results = recorder.stop();
    REQUIRE (results.size() == 2);
    REQUIRE (results[0].audio.samples == 512);
    REQUIRE (results[1].audio.samples == 512);
    REQUIRE (results[0].timelineStartSamples == 23968);
    REQUIRE (results[1].timelineStartSamples == 24064);
    REQUIRE (results[0].audio.channels == 1);
    REQUIRE (results[1].audio.channels == 2);
    REQUIRE (RecordingJournal::find (journals).isEmpty());

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    auto inputReader = std::unique_ptr<juce::AudioFormatReader> (
        formats.createReaderFor (results[0].audio.file));
    auto outputReader = std::unique_ptr<juce::AudioFormatReader> (
        formats.createReaderFor (results[1].audio.file));
    REQUIRE (inputReader != nullptr);
    REQUIRE (outputReader != nullptr);
    juce::AudioBuffer<float> routedInput (1, 16);
    juce::AudioBuffer<float> routedOutput (2, 16);
    REQUIRE (inputReader->read (&routedInput, 0, 16, 0, true, false));
    REQUIRE (outputReader->read (&routedOutput, 0, 16, 0, true, true));
    REQUIRE (std::abs (routedInput.getSample (0, 0) - 0.25f) < 0.001f);
    REQUIRE (std::abs (routedOutput.getSample (0, 0) - 0.1f) < 0.001f);
    REQUIRE (std::abs (routedOutput.getSample (1, 0) + 0.1f) < 0.001f);
    inputReader.reset();
    outputReader.reset();

    auto invalidTargets = targets;
    invalidTargets.push_back (
        { "track-invalid", 2, 1, RecordingTapPoint::hardwareInput, 0 });
    REQUIRE (recorder.start (invalidTargets, context).failed());
    REQUIRE (! recorder.isRecording());
    REQUIRE (RecordingJournal::find (journals).isEmpty());

    context.sessionId = "session-loop";
    context.fileStem = "Loop-test";
    context.loopEnabled = true;
    REQUIRE (recorder.start ({ targets.front() }, context).wasOk());
    recorder.processSpan (inputs, program, 0, 0, 512, 1);
    recorder.processSpan (inputs, program, 0, 0, 512, 2);
    REQUIRE (recorder.getCurrentPass() == 2);
    REQUIRE (recorder.service().wasOk());
    auto activeJournals = RecordingJournal::find (journals);
    REQUIRE (activeJournals.size() == 2);
    auto recordingJournalCount = 0;
    auto preparedJournalCount = 0;
    for (const auto& journalFile : activeJournals)
    {
        RecordingJournalEntry entry;
        REQUIRE (RecordingJournal::read (journalFile, entry).wasOk());
        if (entry.status == "recording")
            ++recordingJournalCount;
        else if (entry.status == "prepared")
            ++preparedJournalCount;
    }
    REQUIRE (recordingJournalCount == 1);
    REQUIRE (preparedJournalCount == 1);
    recorder.processSpan (inputs, program, 0, 0, 512, 3);
    REQUIRE (recorder.service().wasOk());
    results = recorder.stop();
    REQUIRE (results.size() == 3);
    REQUIRE (results[0].passNumber == 1);
    REQUIRE (results[1].passNumber == 2);
    REQUIRE (results[2].passNumber == 3);
    REQUIRE (recorder.getPassPreparationStarvationCount() == 0);
    REQUIRE (RecordingJournal::find (journals).isEmpty());

    REQUIRE (directory.deleteRecursively());
    return 0;
}
