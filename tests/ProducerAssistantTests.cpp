#include "TestSupport.h"

#include "core/ProducerAssistant.h"
#include "core/BuiltInInstrument.h"

#include <cmath>

int main()
{
    using namespace triumph::producer;

    REQUIRE (clampedRoot (12) == 0);
    REQUIRE (clampedRoot (-1) == 11);
    REQUIRE (clampedBars (100) == 16);

    Request request;
    request.rootPitchClass = 0;
    request.scale = Scale::major;
    request.style = Style::balanced;
    request.bars = 4;
    request.variation = 42;

    const auto chords = generateChords (request);
    REQUIRE (chords.size() == 16);
    REQUIRE (chords[0].noteNumber == 48);
    REQUIRE (chords[1].noteNumber == 60);
    REQUIRE (chords[2].noteNumber == 64);
    REQUIRE (chords[3].noteNumber == 67);
    REQUIRE (chords[4].startBeat == 4.0);
    REQUIRE (chords.back().startBeat == 12.0);

    request.variation = 43;
    const auto alternateChords = generateChords (request);
    REQUIRE (alternateChords[10].noteNumber != chords[10].noteNumber);
    request.variation = 42;

    request.scale = Scale::minor;
    const auto minorChords = generateChords (request);
    REQUIRE (minorChords[2].noteNumber == 63);

    request.scale = Scale::major;
    const auto drums = generateDrums (request);
    REQUIRE (drums.size() >= 52);
    for (const auto& note : drums)
    {
        REQUIRE (note.channel == 10);
        REQUIRE (note.noteNumber == 36 || note.noteNumber == 38
                 || note.noteNumber == 42);
    }
    request.variation = 43;
    const auto alternateDrums = generateDrums (request);
    REQUIRE (alternateDrums.size() != drums.size());
    request.variation = 42;

    const auto melodyA = generateMelody (request);
    const auto melodyB = generateMelody (request);
    REQUIRE (! melodyA.empty());
    REQUIRE (melodyA.size() == melodyB.size());
    for (std::size_t index = 0; index < melodyA.size(); ++index)
    {
        REQUIRE (melodyA[index].noteNumber == melodyB[index].noteNumber);
        REQUIRE (std::abs (melodyA[index].startBeat
                           - melodyB[index].startBeat) < 1.0e-9);
    }
    request.variation = 43;
    const auto melodyC = generateMelody (request);
    auto foundDifference = melodyC.size() != melodyA.size();
    for (std::size_t index = 0;
         ! foundDifference && index < melodyA.size() && index < melodyC.size(); ++index)
        foundDifference = melodyA[index].noteNumber != melodyC[index].noteNumber;
    REQUIRE (foundDifference);

    const std::vector<TrackDescriptor> descriptors {
        { "Kick", false }, { "Lead Vocal", false },
        { "Wide Pad", true }, { "Guitar", false }
    };
    const auto mix = suggestMix (descriptors);
    REQUIRE (mix.size() == descriptors.size());
    REQUIRE (mix[0].pan == 0.0f);
    REQUIRE (mix[1].pan == 0.0f);
    REQUIRE (std::abs (mix[2].pan) > 0.2f);
    REQUIRE (std::abs (mix[3].pan) > 0.2f);
    for (const auto& move : mix)
    {
        REQUIRE (move.gain >= 0.42f && move.gain <= 0.86f);
        REQUIRE (! move.reason.empty());
    }

    const auto kick = triumph::instrument::renderSample (
        36, 10, 1.0f, 0.01, 0.09, 480, 48000.0);
    const auto snareA = triumph::instrument::renderSample (
        38, 10, 1.0f, 0.01, 0.09, 480, 48000.0);
    const auto snareB = triumph::instrument::renderSample (
        38, 10, 1.0f, 0.01, 0.09, 480, 48000.0);
    REQUIRE (std::abs (kick) > 0.001f);
    REQUIRE (std::abs (snareA) > 0.001f);
    REQUIRE (snareA == snareB);

    return 0;
}
