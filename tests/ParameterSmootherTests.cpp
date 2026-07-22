#include "core/ParameterSmoother.h"
#include "TestSupport.h"

#include <cmath>

int main()
{
    triumph::ParameterSmoother oneBlock;
    triumph::ParameterSmoother splitBlocks;
    oneBlock.reset (48000.0, 20.0, 0.0f);
    splitBlocks.reset (48000.0, 20.0, 0.0f);
    oneBlock.setTarget (1.0f);
    splitBlocks.setTarget (1.0f);

    for (int sample = 0; sample < 512; ++sample)
        oneBlock.process();
    for (int block = 0; block < 4; ++block)
        for (int sample = 0; sample < 128; ++sample)
            splitBlocks.process();

    REQUIRE (std::abs (oneBlock.getCurrent() - splitBlocks.getCurrent()) < 0.000001f);
    REQUIRE (oneBlock.getCurrent() > 0.0f && oneBlock.getCurrent() < 1.0f);
    return 0;
}
