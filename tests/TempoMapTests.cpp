#include "core/TempoMap.h"
#include "TestSupport.h"

#include <cmath>

namespace
{
bool closeEnough (double a, double b)
{
    return std::abs (a - b) < 0.00001;
}
}

int main()
{
    using namespace triumph::tempo;
    const std::vector<Point> constant { { 0.0, 120.0 } };
    REQUIRE (closeEnough (beatToSeconds (4.0, constant), 2.0));
    REQUIRE (closeEnough (secondsToBeat (2.0, constant), 4.0));
    REQUIRE (beatToSamples (4.0, 48000.0, constant) == 96000);

    const std::vector<Point> changing { { 0.0, 120.0 }, { 4.0, 60.0 } };
    REQUIRE (closeEnough (beatToSeconds (6.0, changing), 4.0));
    REQUIRE (closeEnough (secondsToBeat (4.0, changing), 6.0));
    REQUIRE (sampleOffsetInBlock (1010, 1000, 64) == 10);
    REQUIRE (sampleOffsetInBlock (1064, 1000, 64) == -1);
    REQUIRE (closeEnough (quantizeBeat (1.13, 0.25), 1.25));
    return 0;
}
