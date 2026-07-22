#include "TestSupport.h"

#include "core/HelpAssistant.h"

int main()
{
    using namespace triumph::help;

    REQUIRE (articles().size() >= 14);
    REQUIRE (normalize ("  MIDI & Keys! ") == "midi keys");
    const auto tokenized = tokens ("How do I record a microphone?");
    REQUIRE (tokenized.size() == 6);

    const auto recording = search ("How do I record my microphone?");
    REQUIRE (! recording.empty());
    REQUIRE (recording.front().article->id == "record");

    const auto routing = search ("create a sidechain send in the mixer");
    REQUIRE (! routing.empty());
    REQUIRE (routing.front().article->id == "mixer");

    const auto pianoRoll = search ("quantize notes in piano roll");
    REQUIRE (! pianoRoll.empty());
    REQUIRE (pianoRoll.front().article->id == "piano-roll");

    const auto cloud = search ("cloud collaboration and marketplace");
    REQUIRE (! cloud.empty());
    REQUIRE (cloud.front().article->id == "availability");
    REQUIRE (cloud.front().article->steps.find ("does not display fake")
             != std::string::npos);

    const auto exportHelp = search ("export track stems");
    REQUIRE (! exportHelp.empty());
    REQUIRE (exportHelp.front().article->action == "export");

    REQUIRE (search ("", 5).empty());
    REQUIRE (search ("record", 1).size() == 1);
    return 0;
}
