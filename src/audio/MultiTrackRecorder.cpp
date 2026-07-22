#include "MultiTrackRecorder.h"

#include <utility>

namespace triumph
{
MultiTrackRecorder::MultiTrackRecorder()
{
    writerThread.startThread();
}

MultiTrackRecorder::~MultiTrackRecorder()
{
    stop();
    writerThread.stopThread (5000);
}

juce::Result MultiTrackRecorder::start (std::vector<Target> targets,
                                        StartContext context)
{
    stop();
    if (targets.empty())
        return juce::Result::fail ("Arm at least one audio track.");
    if (context.sampleRate <= 0.0 || context.projectId.isEmpty()
        || context.sessionId.isEmpty())
        return juce::Result::fail ("The recording session metadata is invalid.");

    for (auto& target : targets)
    {
        target.channelCount = juce::jlimit (1, 2, target.channelCount);
        target.firstInputChannel = juce::jmax (0, target.firstInputChannel);
        target.recordOffsetSamples = juce::jlimit (
            -8192, 8192, target.recordOffsetSamples);
        const auto available = target.tapPoint == RecordingTapPoint::hardwareInput
            ? context.availableInputChannels : context.availableOutputChannels;
        if (target.trackId.isEmpty()
            || target.firstInputChannel + target.channelCount > available)
            return juce::Result::fail (
                "An armed track uses an unavailable recording input route.");
    }

    if (context.recordingsDirectory.createDirectory().failed()
        || context.journalDirectory.createDirectory().failed())
        return juce::Result::fail (
            "The recording or journal directory could not be created.");

    configuredTargets = std::move (targets);
    activeContext = std::move (context);
    passPreparationStarvations.store (0);
    auto result = preparePass (1, true);
    if (result.failed())
    {
        stop();
        return result;
    }
    if (activeContext.loopEnabled)
    {
        result = preparePass (2, false);
        if (result.failed())
        {
            stop();
            return result;
        }
    }
    active.store (true, std::memory_order_release);
    return juce::Result::ok();
}

juce::Result MultiTrackRecorder::preparePass (int passNumber,
                                               bool activateImmediately)
{
    auto pass = std::make_unique<Pass>();
    pass->number = passNumber;
    pass->timelineStartSamples = activeContext.timelineStartSamples;
    pass->recordingJournalPublished = activateImmediately;
    pass->targets.reserve (configuredTargets.size());

    for (std::size_t index = 0; index < configuredTargets.size(); ++index)
    {
        const auto& target = configuredTargets[index];
        const auto suffix = "-T" + juce::String (static_cast<int> (index + 1))
                            + "-P" + juce::String (passNumber);
        auto audioFile = activeContext.recordingsDirectory.getChildFile (
            activeContext.fileStem + suffix + ".wav");
        if (audioFile.existsAsFile())
            audioFile = activeContext.recordingsDirectory.getNonexistentChildFile (
                activeContext.fileStem + suffix, ".wav", false);

        AudioRecorder::JournalContext journal;
        journal.entry.sessionId = activeContext.sessionId + suffix;
        journal.entry.projectId = activeContext.projectId;
        journal.entry.trackId = target.trackId;
        journal.entry.tapPoint = target.tapPoint
                                     == RecordingTapPoint::programOutput
                                 ? "device-output" : "hardware-input";
        journal.entry.status = activateImmediately ? "recording" : "prepared";
        journal.entry.passNumber = passNumber;
        journal.entry.inputStartChannel = target.firstInputChannel;
        journal.entry.projectFile = activeContext.projectFile;
        journal.entry.timelineStartSamples = activeContext.timelineStartSamples
                                             + target.recordOffsetSamples;
        journal.entry.createdAtMilliseconds = juce::Time::currentTimeMillis();
        journal.journalFile = activeContext.journalDirectory.getChildFile (
            journal.entry.sessionId + ".recording.json");

        PassTarget prepared;
        prepared.target = target;
        prepared.journalFile = journal.journalFile;
        prepared.recorder = std::make_unique<AudioRecorder> (&writerThread);
        const auto result = prepared.recorder->start (
            audioFile, activeContext.sampleRate, target.channelCount,
            std::move (journal));
        if (result.failed())
        {
            for (auto& opened : pass->targets)
            {
                const auto openedAudio = opened.recorder->stop();
                RecordingJournal::remove (opened.journalFile);
                if (openedAudio.file.existsAsFile())
                    openedAudio.file.deleteFile();
            }
            return result;
        }
        pass->targets.push_back (std::move (prepared));
    }

    auto* rawPass = pass.get();
    passes.push_back (std::move (pass));
    if (activateImmediately)
    {
        rawPass->activated.store (true, std::memory_order_release);
        activePass.store (rawPass, std::memory_order_release);
    }
    else
        preparedPass.store (rawPass, std::memory_order_release);
    return juce::Result::ok();
}

MultiTrackRecorder::Pass* MultiTrackRecorder::passForAudioThread (
    int passNumber) noexcept
{
    auto* current = activePass.load (std::memory_order_acquire);
    if (current != nullptr && current->number == passNumber)
        return current;

    auto* next = preparedPass.load (std::memory_order_acquire);
    if (next == nullptr || next->number != passNumber)
    {
        passPreparationStarvations.fetch_add (1, std::memory_order_relaxed);
        return nullptr;
    }
    next = preparedPass.exchange (nullptr, std::memory_order_acq_rel);
    if (next == nullptr || next->number != passNumber)
        return nullptr;
    next->activated.store (true, std::memory_order_release);
    activePass.store (next, std::memory_order_release);
    return next;
}

void MultiTrackRecorder::processSpan (
    const juce::AudioBuffer<float>& hardwareInput,
    const juce::AudioBuffer<float>& programOutput,
    int hardwareStartSample, int programStartSample,
    int numSamples, int passNumber) noexcept
{
    if (! active.load (std::memory_order_acquire) || numSamples <= 0)
        return;
    auto* pass = passForAudioThread (passNumber);
    if (pass == nullptr)
        return;
    for (auto& destination : pass->targets)
    {
        const auto& source = destination.target.tapPoint
                                 == RecordingTapPoint::programOutput
                             ? programOutput : hardwareInput;
        const auto startSample = destination.target.tapPoint
                                     == RecordingTapPoint::programOutput
                                 ? programStartSample : hardwareStartSample;
        destination.recorder->processInput (
            source, startSample, numSamples,
            destination.target.firstInputChannel);
    }
}

void MultiTrackRecorder::finalisePass (Pass& pass)
{
    if (pass.finalised)
        return;
    pass.finalised = true;
    for (auto& source : pass.targets)
    {
        auto audio = source.recorder->stop();
        if (! pass.activated.load (std::memory_order_acquire)
            || ! audio.isValid())
        {
            RecordingJournal::remove (source.journalFile);
            if (audio.file.existsAsFile())
                audio.file.deleteFile();
            continue;
        }
        completedResults.push_back ({
            source.target, pass.number,
            pass.timelineStartSamples + source.target.recordOffsetSamples,
            std::move (audio)
        });
    }
}

juce::Result MultiTrackRecorder::service()
{
    if (! active.load (std::memory_order_acquire))
        return juce::Result::ok();
    auto* current = activePass.load (std::memory_order_acquire);
    auto* prepared = preparedPass.load (std::memory_order_acquire);
    if (current != nullptr
        && current->activated.load (std::memory_order_acquire)
        && ! current->recordingJournalPublished)
    {
        for (auto& target : current->targets)
        {
            const auto result = target.recorder->setJournalStatus ("recording");
            if (result.failed())
                return result;
        }
        current->recordingJournalPublished = true;
    }
    for (auto& pass : passes)
        if (pass.get() != current && pass.get() != prepared
            && pass->activated.load (std::memory_order_acquire))
            finalisePass (*pass);

    if (activeContext.loopEnabled && current != nullptr && prepared == nullptr)
        return preparePass (current->number + 1, false);
    return juce::Result::ok();
}

std::vector<MultiTrackRecorder::CaptureResult> MultiTrackRecorder::stop()
{
    active.store (false, std::memory_order_release);
    activePass.store (nullptr, std::memory_order_release);
    preparedPass.store (nullptr, std::memory_order_release);
    for (auto& pass : passes)
        finalisePass (*pass);
    passes.clear();
    configuredTargets.clear();
    activeContext = {};
    auto result = std::move (completedResults);
    completedResults.clear();
    return result;
}

juce::Result MultiTrackRecorder::checkpointJournals()
{
    for (auto& pass : passes)
    {
        if (! pass->activated.load (std::memory_order_acquire)
            || pass->finalised)
            continue;
        for (auto& target : pass->targets)
        {
            const auto result = target.recorder->checkpointJournal();
            if (result.failed())
                return result;
        }
    }
    return juce::Result::ok();
}

bool MultiTrackRecorder::isRecording() const noexcept
{
    return active.load (std::memory_order_acquire);
}

int MultiTrackRecorder::getCurrentPass() const noexcept
{
    const auto* pass = activePass.load (std::memory_order_acquire);
    return pass != nullptr ? pass->number : 0;
}

juce::int64 MultiTrackRecorder::getSamplesRecorded() const noexcept
{
    const auto* pass = activePass.load (std::memory_order_acquire);
    if (pass == nullptr || pass->targets.empty())
        return 0;
    return pass->targets.front().recorder->getSamplesRecorded();
}

int MultiTrackRecorder::getDroppedBlockCount() const noexcept
{
    auto total = 0;
    for (const auto& pass : passes)
        if (! pass->finalised)
            for (const auto& target : pass->targets)
                total += target.recorder->getDroppedBlockCount();
    return total;
}

juce::int64 MultiTrackRecorder::getDroppedSampleCount() const noexcept
{
    juce::int64 total = 0;
    for (const auto& pass : passes)
        if (! pass->finalised)
            for (const auto& target : pass->targets)
                total += target.recorder->getDroppedSampleCount();
    return total;
}

int MultiTrackRecorder::getPassPreparationStarvationCount() const noexcept
{
    return passPreparationStarvations.load (std::memory_order_relaxed);
}
}
