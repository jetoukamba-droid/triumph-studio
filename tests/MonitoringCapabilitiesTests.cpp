#include "core/MonitoringCapabilities.h"

#include "TestSupport.h"

int main()
{
    using namespace triumph::monitoring;

    const auto none = assessDevice (0, 0);
    REQUIRE (! none.softwareControlRoom);
    REQUIRE (! none.dedicatedCueOutput);
    REQUIRE (none.hardwareDirectControl == HardwareDirectControl::unavailable);

    const auto stereo = assessDevice (2, 2);
    REQUIRE (stereo.softwareControlRoom);
    REQUIRE (! stereo.dedicatedCueOutput);
    REQUIRE (stereo.hardwareDirectControl == HardwareDirectControl::driverManaged);

    const auto cueCapable = assessDevice (8, 8);
    REQUIRE (cueCapable.softwareControlRoom);
    REQUIRE (cueCapable.dedicatedCueOutput);
    REQUIRE (cueCapable.inputChannels == 8);
    REQUIRE (cueCapable.outputChannels == 8);
    return 0;
}
