#include "core/MidiCapture.h"
#include "TestSupport.h"

#include <cmath>

int main()
{
    triumph::MidiCapture capture;
    capture.start (100.0);
    capture.processNoteOn (1, 60, 0.75f, 100.25);
    capture.processNoteOff (1, 60, 100.75);
    capture.processNoteOn (2, 64, 0.5f, 101.0);
    const auto notes = capture.stop (101.5);
    REQUIRE (notes.size() == 2);
    REQUIRE (notes[0].noteNumber == 60);
    REQUIRE (std::abs (notes[0].startSeconds - 0.25) < 0.00001);
    REQUIRE (std::abs (notes[0].lengthSeconds - 0.5) < 0.00001);
    REQUIRE (notes[1].channel == 2);
    REQUIRE (std::abs (notes[1].lengthSeconds - 0.5) < 0.00001);
    return 0;
}
