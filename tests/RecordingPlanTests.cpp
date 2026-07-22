#include "core/RecordingPlan.h"

#include "TestSupport.h"

using namespace triumph::recording;

int main()
{
    Plan normal;
    REQUIRE (normal.configure ({ SessionMode::normal, 48000, 0, 24000 },
                               48000.0, 48000.0, 512));
    REQUIRE (normal.getState() == State::preRoll);
    REQUIRE (normal.getTransportStartSample() == 24000);
    auto block = normal.processBlock (47000, 512);
    REQUIRE (block.spanCount == 0);
    block = normal.processBlock (47800, 512);
    REQUIRE (block.captureStarted);
    REQUIRE (block.spanCount == 1);
    REQUIRE (block.spans[0].frameOffset == 200);
    REQUIRE (block.spans[0].frameCount == 312);

    Plan punch;
    REQUIRE (punch.configure ({ SessionMode::punch, 1000, 1400, 500 },
                              48000.0, 48000.0, 512));
    block = punch.processBlock (900, 512);
    REQUIRE (block.spanCount == 1);
    REQUIRE (block.spans[0].frameOffset == 100);
    REQUIRE (block.spans[0].frameCount == 400);
    REQUIRE (block.captureFinished);
    REQUIRE (punch.getState() == State::finalizing);

    Plan loop;
    REQUIRE (loop.configure ({ SessionMode::loop, 1000, 2000, 0 },
                             48000.0, 48000.0, 512));
    block = loop.processBlock (1800, 512);
    REQUIRE (block.spanCount == 2);
    REQUIRE (block.spans[0].frameOffset == 0);
    REQUIRE (block.spans[0].frameCount == 200);
    REQUIRE (block.spans[0].passNumber == 1);
    REQUIRE (block.spans[1].frameOffset == 200);
    REQUIRE (block.spans[1].frameCount == 312);
    REQUIRE (block.spans[1].passNumber == 2);
    REQUIRE (block.loopWrapped);
    REQUIRE (block.wrappedTimelinePosition == 1312);
    REQUIRE (loop.getCurrentPass() == 2);
    REQUIRE (loop.getState() == State::loopPass);

    Plan invalid;
    REQUIRE (! invalid.configure ({ SessionMode::loop, 0, 128, 0 },
                                  48000.0, 48000.0, 512));

    Plan rateConverted;
    REQUIRE (rateConverted.configure (
        { SessionMode::punch, 48000, 96000, 0 },
        48000.0, 44100.0, 512));
    block = rateConverted.processBlock (47500, 512);
    REQUIRE (block.captureStarted);
    REQUIRE (block.spanCount == 1);
    REQUIRE (block.spans[0].frameOffset == 460);
    REQUIRE (block.spans[0].frameCount == 52);
    return 0;
}
