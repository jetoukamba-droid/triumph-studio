#include "TestSupport.h"
#include "core/FoundationsExitGates.h"

#include <string_view>

int main()
{
    using namespace triumph::foundations;

    REQUIRE (areaName (Area::fr1RealtimeStreaming).find ("FR-1") != std::string_view::npos);
    REQUIRE (areaName (Area::fr4MixerMonitoring).find ("mixer") != std::string_view::npos);
    REQUIRE (statusName (Status::notStarted) == "not-started");
    REQUIRE (statusName (Status::partial) == "partial");
    REQUIRE (statusName (Status::complete) == "complete");

    constexpr Evidence empty {};
    REQUIRE (evaluateFr1 (empty) == Status::notStarted);
    REQUIRE (evaluateFr2 (empty) == Status::notStarted);
    REQUIRE (evaluateFr3 (empty) == Status::notStarted);
    REQUIRE (evaluateFr4 (empty) == Status::notStarted);

    constexpr Evidence deterministicCore { true, true, true, false, false, false };
    REQUIRE (evaluateFr1 (deterministicCore) == Status::partial);
    REQUIRE (evaluateFr2 (deterministicCore) == Status::partial);
    REQUIRE (evaluateFr3 (deterministicCore) == Status::partial);
    REQUIRE (evaluateFr4 (deterministicCore) == Status::partial);

    constexpr Evidence hardwareRecoveryComplete { true, true, true, true, false, true };
    REQUIRE (evaluateFr1 (hardwareRecoveryComplete) == Status::complete);
    REQUIRE (evaluateFr2 (hardwareRecoveryComplete) == Status::complete);
    REQUIRE (evaluateFr4 (hardwareRecoveryComplete) == Status::complete);

    constexpr Evidence pluginComplete { true, true, true, false, true, true };
    REQUIRE (evaluateFr3 (pluginComplete) == Status::complete);

    constexpr auto gates = currentFr1ToFr4Gates();
    REQUIRE (gates.size() == 4);
    REQUIRE (countByStatus (gates, Status::partial) == 4);
    REQUIRE (countByStatus (gates, Status::complete) == 0);
    REQUIRE (countByStatus (gates, Status::notStarted) == 0);

    REQUIRE (gates[0].evidence.coreImplementation);
    REQUIRE (gates[0].evidence.deterministicTests);
    REQUIRE (! gates[0].evidence.referenceHardwareMatrix);
    REQUIRE (gates[1].blocker.find ("forced-termination") != std::string_view::npos);
    REQUIRE (gates[2].blocker.find ("cross-process") != std::string_view::npos);
    REQUIRE (gates[3].blocker.find ("multi-output") != std::string_view::npos);

    return 0;
}
