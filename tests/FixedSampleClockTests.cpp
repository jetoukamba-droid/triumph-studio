#include "core/FixedSampleClock.h"
#include "TestSupport.h"

#include <cmath>
#include <cstdint>

int main()
{
    using namespace triumph::transport;

    REQUIRE (fromProjectSamples (0) == 0);
    REQUIRE (fromProjectSamples (48000) == 48000 * unitsPerSample);
    REQUIRE (toProjectSamples (fromProjectSamples (96000)) == 96000);
    REQUIRE (std::abs (toSeconds (fromSeconds (2.5, 48000.0), 48000.0)
                       - 2.5) < 0.0000001);

    // Cross-rate advancement uses integer fixed-point authority. Repeated blocks
    // remain within one Q20 unit of the mathematically exact position.
    ClockUnits position = 0;
    for (int block = 0; block < 100000; ++block)
        position += advanceForDeviceFrames (256, 48000.0, 44100.0);
    const auto expected = static_cast<long double> (100000) * 256.0L
                          * 48000.0L / 44100.0L
                          * static_cast<long double> (unitsPerSample);
    REQUIRE (std::abs (static_cast<long double> (position) - expected)
             < 100000.0L);
    REQUIRE (std::abs (toSeconds (position, 48000.0)
                       - (100000.0 * 256.0 / 44100.0)) < 0.000001);

    REQUIRE (clamp (-1, fromProjectSamples (10)) == 0);
    REQUIRE (clamp (fromProjectSamples (20), fromProjectSamples (10))
             == fromProjectSamples (10));
    return 0;
}
