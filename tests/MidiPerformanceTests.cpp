#include "TestSupport.h"

#include "core/MidiPerformance.h"

#include <cmath>
#include <vector>

int main()
{
    using namespace triumph::midi;

    const auto noteOn = midi2NoteOn (3, 1, 60, 0x8000);
    REQUIRE (noteOn.wordCount == 2);
    REQUIRE ((noteOn.words[0] >> 28) == 4);
    REQUIRE (((noteOn.words[0] >> 24) & 0x0f) == 3);
    REQUIRE (((noteOn.words[0] >> 8) & 0x7f) == 60);
    REQUIRE ((noteOn.words[1] >> 16) == 0x8000);

    const auto pitch = midi2PerNotePitch (0, 2, 64, 0x90000000u);
    REQUIRE (pitch.wordCount == 2);
    REQUIRE (pitch.words[1] == 0x90000000u);

    std::vector<TimedEvent> timeline {
        { 1005, 0, noteOn },
        { 1001, 0, pitch },
        { 1032, 0, midi2NoteOff (0, 1, 60) },
        { 999, 0, noteOn }
    };
    const auto block = eventsForBlock (1000, 32, timeline);
    REQUIRE (block.size() == 2);
    REQUIRE (block[0].sampleOffset == 1);
    REQUIRE (block[1].sampleOffset == 5);

    MpeZoneAllocator mpe (2, 4);
    REQUIRE (mpe.allocate (100).value() == 2);
    REQUIRE (mpe.allocate (101).value() == 3);
    REQUIRE (mpe.allocate (102).value() == 4);
    REQUIRE (! mpe.allocate (103).has_value());
    mpe.release (101);
    REQUIRE (mpe.allocate (103).value() == 3);

    SyncFollower sync (48000.0);
    sync.setSource (SyncSource::midiClock);
    for (int pulse = 0; pulse < 12; ++pulse)
        REQUIRE (sync.observeMidiClock (pulse * 1000));
    sync.poll (11100);
    const auto status = sync.status();
    REQUIRE (status.locked);
    REQUIRE (std::abs (status.tempoBpm - 120.0) < 0.001);
    REQUIRE (status.receivedPulses == 12);
    sync.poll (40000);
    REQUIRE (! sync.status().locked);

    SyncFollower mtc (48000.0);
    mtc.setSource (SyncSource::midiTimeCode);
    const std::uint8_t timecode[] { 12, 0, 3, 0, 2, 0, 1, 6 };
    for (std::uint8_t part = 0; part < 8; ++part)
        mtc.observeMtcQuarterFrame (
            part, timecode[part], 1000 + part * 100);
    REQUIRE (mtc.status().locked);
    REQUIRE (std::abs (mtc.status().timecodeSeconds - 3723.4) < 0.001);
    REQUIRE (mtc.status().timecodeFramesPerSecond == 30.0);
    mtc.poll (100000);
    REQUIRE (! mtc.status().locked);

    SampleOffsetParameterQueue<4> queue;
    REQUIRE (queue.push ({ 2, 31, 1.2f }, 32));
    REQUIRE (queue.push ({ 1, 3, -1.0f }, 32));
    REQUIRE (queue.push ({ 3, 17, 0.5f }, 32));
    REQUIRE (queue[0].sampleOffset == 3);
    REQUIRE (queue[0].value == 0.0f);
    REQUIRE (queue[2].sampleOffset == 31);
    REQUIRE (queue[2].value == 1.0f);
    return 0;
}
