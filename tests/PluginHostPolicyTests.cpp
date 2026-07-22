#include "core/PluginHostPolicy.h"
#include "TestSupport.h"

#include <cmath>

int main()
{
    using namespace triumph::plugin;

    REQUIRE (! instrumentBusPolicy (false, 2).accepted);
    REQUIRE (! instrumentBusPolicy (true, 0).accepted);
    REQUIRE (instrumentBusPolicy (true, 1).renderChannels == 1);
    REQUIRE (instrumentBusPolicy (true, 2).renderChannels == 2);
    REQUIRE (instrumentBusPolicy (true, 16).renderChannels == 2);

    REQUIRE (tailPolicy (TailAction::transportStop, 0.0).renderTail);
    REQUIRE (tailPolicy (TailAction::transportStop, 0.0).seconds == 1.0);
    REQUIRE (tailPolicy (TailAction::freeze, 75.0).seconds == 30.0);
    REQUIRE (tailPolicy (TailAction::offlineRender,
                         std::nan ("")).seconds == 1.0);
    REQUIRE (! tailPolicy (TailAction::bypass, 4.0).renderTail);
    REQUIRE (tailPolicy (TailAction::bypass, 4.0).fadeMilliseconds == 10.0);
    REQUIRE (! tailPolicy (TailAction::removal, 4.0).renderTail);
    REQUIRE (! tailPolicy (TailAction::fault, 4.0).renderTail);
    REQUIRE (tailSamples (TailAction::noteInput, 2.5, 48000.0) == 120000);
    REQUIRE (tailSamples (TailAction::fault, 2.5, 48000.0) == 0);

    REQUIRE (stableParameterId ("cutoff", 7, "Cutoff")
             == "plugin:cutoff");
    const auto fallback = stableParameterId ({}, 7, "Cutoff");
    REQUIRE (fallback.starts_with ("plugin:legacy:7:"));
    REQUIRE (fallback == stableParameterId ({}, 7, "Cutoff"));
    REQUIRE (fallback != stableParameterId ({}, 8, "Cutoff"));
    REQUIRE (fallback != stableParameterId ({}, 7, "Resonance"));
    return 0;
}
