#include "core/KeyboardShortcuts.h"
#include "TestSupport.h"

int main()
{
    using namespace triumph::shortcuts;

    REQUIRE (hasUniqueGestures());
    REQUIRE (commandFor (' ', noModifier) == Command::togglePlayback);
    REQUIRE (commandFor ('r', noModifier) == Command::toggleRecord);
    REQUIRE (commandFor ('M', noModifier) == Command::toggleMonitor);
    REQUIRE (commandFor ('S', noModifier) == Command::toggleSnap);
    REQUIRE (commandFor ('S', commandModifier) == Command::saveProject);
    REQUIRE (commandFor ('S', commandModifier | shiftModifier)
             == Command::saveProjectAs);
    REQUIRE (commandFor ('E', commandModifier) == Command::splitClip);
    REQUIRE (commandFor ('E', commandModifier | shiftModifier)
             == Command::exportMix);
    REQUIRE (commandFor ('T', commandModifier) == Command::addAudioTrack);
    REQUIRE (commandFor ('T', commandModifier | shiftModifier)
             == Command::addInstrumentTrack);
    REQUIRE (commandFor ('Z', commandModifier | shiftModifier)
             == Command::redo);
    REQUIRE (commandFor ('Q', noModifier) == Command::none);
    REQUIRE (labelFor (Command::togglePlayback)[0] != '\0');
    return 0;
}
