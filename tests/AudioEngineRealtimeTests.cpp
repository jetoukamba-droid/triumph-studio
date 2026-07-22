#include <JuceHeader.h>

#include "audio/AudioEngine.h"
#include "TestSupport.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <memory>
#include <thread>
#include <vector>

namespace
{
juce::File createSourceFile()
{
    auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getNonexistentChildFile (
                        "triumph-realtime-source", ".wav", false);
    auto stream = std::unique_ptr<juce::OutputStream> (file.createOutputStream());
    if (stream == nullptr)
        return {};

    juce::WavAudioFormat wav;
    auto writer = wav.createWriterFor (
        stream, juce::AudioFormatWriterOptions {}
                    .withSampleRate (48000.0)
                    .withNumChannels (2)
                    .withBitsPerSample (24));
    if (writer == nullptr)
        return {};

    juce::AudioBuffer<float> block (2, 48000);
    for (int sample = 0; sample < block.getNumSamples(); ++sample)
    {
        const auto value = static_cast<float> (
            0.12 * std::sin (2.0 * juce::MathConstants<double>::pi
                             * 220.0 * sample / 48000.0));
        block.setSample (0, sample, value);
        block.setSample (1, sample, value);
    }
    for (int second = 0; second < 10; ++second)
        REQUIRE (writer->writeFromAudioSampleBuffer (
            block, 0, block.getNumSamples()));
    writer.reset();
    return file;
}

triumph::AudioEngine::MidiTrackPlayback midiTrack()
{
    triumph::AudioEngine::MidiTrackPlayback track;
    track.id = "midi-track";
    track.notes.push_back ({ 0.0, 20.0, 0.0, 0.0, 60, 0.65f, 1 });
    return track;
}

std::vector<triumph::AudioEngine::MixerChannel> mixerChannels (float gain)
{
    triumph::AudioEngine::MixerChannel audio;
    audio.id = "audio-track";
    audio.gain = gain;
    triumph::AudioEngine::MixerChannel midi;
    midi.id = "midi-track";
    midi.gain = gain;
    return { audio, midi };
}

double estimateFrequency (triumph::AudioEngine& engine,
                          int sampleRate,
                          int blockSize)
{
    juce::AudioBuffer<float> output (2, blockSize);
    auto previous = 0.0f;
    auto positiveCrossings = 0;
    auto warmupRemaining = sampleRate;
    auto rendered = 0;
    while (warmupRemaining > 0)
    {
        const auto count = juce::jmin (blockSize, warmupRemaining);
        output.clear();
        juce::AudioSourceChannelInfo info (&output, 0, count);
        engine.getNextAudioBlock (info);
        warmupRemaining -= count;
        std::this_thread::sleep_for (std::chrono::milliseconds (1));
    }
    while (rendered < sampleRate)
    {
        const auto count = juce::jmin (blockSize, sampleRate - rendered);
        output.clear();
        juce::AudioSourceChannelInfo info (&output, 0, count);
        engine.getNextAudioBlock (info);
        for (int sample = 0; sample < count; ++sample)
        {
            const auto current = output.getSample (0, sample);
            if (previous <= 0.0f && current > 0.0f)
                ++positiveCrossings;
            previous = current;
        }
        rendered += count;
    }
    return static_cast<double> (positiveCrossings);
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    const auto sourceFile = createSourceFile();
    REQUIRE (sourceFile.existsAsFile());

    {
        triumph::AudioEngine engine;
        engine.beginRenderStateUpdate();
        REQUIRE (engine.addTrack ("audio-track", sourceFile) != nullptr);
        triumph::AudioTrack::ClipPlayback clip;
        clip.id = "clip";
        clip.sourceId = "source";
        clip.sourceFile = sourceFile;
        clip.lengthInTimelineSamples = 480000;
        engine.configureTrackClips ("audio-track", { clip }, 48000.0);
        engine.configureMidiTracks ({ midiTrack() }, { { 0.0, 120.0 } });
        engine.configureMusicalTimeline ({ { 0.0, 120.0 } },
                                         { { 0.0, 4, 4 } }, false, 0.5f);
        engine.configureDelayCompensation (
            { { "audio-track" }, { "midi-track" } }, false);
        REQUIRE (engine.configureMixer (mixerChannels (0.8f)));
        engine.configureAutomation ({});
        engine.endRenderStateUpdate();
        engine.setProjectTimeline (48000.0, 480000);
        engine.prepareToPlay (256, 48000.0);
        engine.play();

        std::atomic<bool> editFailed { false };
        const auto editStressStarted = std::chrono::steady_clock::now();
        std::thread editor ([&]
        {
            for (int iteration = 0; iteration < 40; ++iteration)
            {
                auto editedClip = clip;
                editedClip.gain = iteration % 2 == 0 ? 0.65f : 1.0f;
                editedClip.playbackRate = iteration % 2 == 0 ? 0.5 : 2.0;
                engine.beginRenderStateUpdate();
                engine.configureTrackClips (
                    "audio-track", { editedClip }, 48000.0);
                engine.configureMidiTracks (
                    { midiTrack() }, { { 0.0, 120.0 } });
                if (! engine.configureMixer (
                        mixerChannels (iteration % 2 == 0 ? 0.7f : 0.9f)))
                    editFailed.store (true);
                triumph::AudioEngine::AutomationPlayback lane;
                lane.targetId = "audio-track";
                lane.parameterId = "gain";
                lane.points = { { 0, 0.75f }, { 480000, 0.95f } };
                engine.configureAutomation ({ lane });
                engine.endRenderStateUpdate();
                std::this_thread::sleep_for (std::chrono::milliseconds (1));
            }
        });

        juce::AudioBuffer<float> output (2, 256);
        juce::AudioSourceChannelInfo info (&output, 0, output.getNumSamples());
        auto silentBlocks = 0;
        for (int block = 0; block < 200; ++block)
        {
            output.clear();
            engine.getNextAudioBlock (info);
            const auto peak = juce::jmax (
                output.getMagnitude (0, 0, output.getNumSamples()),
                output.getMagnitude (1, 0, output.getNumSamples()));
            if (peak <= 0.000001f)
                ++silentBlocks;
            std::this_thread::sleep_for (std::chrono::milliseconds (1));
        }
        editor.join();
        const auto editStressMilliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds> (
                std::chrono::steady_clock::now() - editStressStarted).count();
        std::cout << "Clip edit stress: " << editStressMilliseconds << " ms\n";

        REQUIRE (! editFailed.load());
        REQUIRE (editStressMilliseconds < 15000);
        REQUIRE (silentBlocks == 0);
        const auto telemetry = engine.getRealtimeStatus();
        REQUIRE (telemetry.callbackCount == 200);
        REQUIRE (telemetry.snapshotSwaps > 1);
        REQUIRE (telemetry.renderGeneration > 1);
        REQUIRE (telemetry.deviceRestartCount == 1);
        REQUIRE (telemetry.variableBlockSizeChanges == 0);
        REQUIRE (telemetry.graphBuildCount > 1);
        REQUIRE (telemetry.maximumGraphBuildNanoseconds > 0);
        REQUIRE (telemetry.missingRenderStateBlocks == 0);
        REQUIRE (telemetry.oversizedAudioBlocks == 0);
        REQUIRE (telemetry.readAheadStarvationBlocks == 0);

        engine.stopAndRewind();
        auto fastClip = clip;
        fastClip.playbackRate = 2.0;
        engine.beginRenderStateUpdate();
        engine.configureTrackClips ("audio-track", { fastClip }, 48000.0);
        engine.configureMidiTracks ({}, { { 0.0, 120.0 } });
        REQUIRE (engine.configureMixer ({ mixerChannels (1.0f).front() }));
        engine.configureAutomation ({});
        engine.endRenderStateUpdate();
        engine.play();
        const auto fastFrequency = estimateFrequency (engine, 48000, 256);
        std::cout << "2.0x varispeed frequency: " << fastFrequency << " Hz\n";
        REQUIRE (fastFrequency > 350.0);
        REQUIRE (fastFrequency < 500.0);

        engine.stopAndRewind();
        auto slowClip = clip;
        slowClip.playbackRate = 0.5;
        engine.beginRenderStateUpdate();
        engine.configureTrackClips ("audio-track", { slowClip }, 48000.0);
        engine.endRenderStateUpdate();
        engine.play();
        const auto slowFrequency = estimateFrequency (engine, 48000, 256);
        std::cout << "0.5x varispeed frequency: " << slowFrequency << " Hz\n";
        REQUIRE (slowFrequency > 80.0);
        REQUIRE (slowFrequency < 180.0);
        engine.stopAndRewind();
        engine.releaseResources();
    }

    REQUIRE (sourceFile.deleteFile());
    return 0;
}
