#include "core/AudioOutputHealth.h"
#include "TestSupport.h"

#include <cmath>

int main()
{
    using namespace triumph::output;

    REQUIRE (assess ({}) == Availability::noDevice);
    REQUIRE (assess ({ true, 0, 48000.0, 256 })
             == Availability::noActiveChannels);
    REQUIRE (assess ({ true, 2, 0.0, 256 }) == Availability::invalidFormat);
    REQUIRE (assess ({ true, 2, 48000.0, 0 }) == Availability::invalidFormat);
    REQUIRE (assess ({ true, 2, 48000.0, 256 }) == Availability::ready);

    REQUIRE (testToneEnvelope (-1, 1000, 100) == 0.0f);
    REQUIRE (testToneEnvelope (0, 1000, 100) == 0.0f);
    REQUIRE (std::abs (testToneEnvelope (50, 1000, 100) - 0.5f) < 0.0001f);
    REQUIRE (testToneEnvelope (500, 1000, 100) == 1.0f);
    REQUIRE (std::abs (testToneEnvelope (949, 1000, 100) - 0.5f) < 0.0001f);
    REQUIRE (testToneEnvelope (999, 1000, 100) == 0.0f);

    REQUIRE (! callbacksStalled (false, 10, 10, 60, 30));
    REQUIRE (! callbacksStalled (true, 11, 10, 60, 30));
    REQUIRE (! callbacksStalled (true, 10, 10, 29, 30));
    REQUIRE (callbacksStalled (true, 10, 10, 30, 30));
    return 0;
}
