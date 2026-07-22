#include "core/PluginGraph.h"
#include "core/SampleDelayLine.h"
#include "TestSupport.h"

#include <array>

int main()
{
    triumph::SampleDelayLine line;
    line.prepare (1, 4, 8);
    std::array<float, 8> impulse { 1.0f, 0.0f, 0.0f, 0.0f,
                                  0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 8> output {};
    const float* inputChannels[] { impulse.data() };
    float* outputChannels[] { output.data() };
    line.processAdd (inputChannels, outputChannels, 1, 8);
    REQUIRE (output[0] == 0.0f);
    REQUIRE (output[3] == 0.0f);
    REQUIRE (output[4] == 1.0f);

    const auto plan = triumph::plugins::buildCompensationPlan ({
        { "fast", 0, false, false, true },
        { "slow", 6, false, false, true }
    });
    REQUIRE (plan.valid);
    triumph::SampleDelayLine fast;
    triumph::SampleDelayLine slow;
    fast.prepare (1, static_cast<int> (plan.nodes[0].delaySamples), 16);
    slow.prepare (1, static_cast<int> (plan.nodes[1].delaySamples), 16);
    std::array<float, 16> fastInput {};
    std::array<float, 16> slowInput {};
    std::array<float, 16> aligned {};
    fastInput[0] = 1.0f;
    slowInput[6] = 1.0f; // models the processor's reported six-sample latency
    const float* fastChannels[] { fastInput.data() };
    const float* slowChannels[] { slowInput.data() };
    float* alignedChannels[] { aligned.data() };
    fast.processAdd (fastChannels, alignedChannels, 1, 16);
    slow.processAdd (slowChannels, alignedChannels, 1, 16);
    REQUIRE (aligned[5] == 0.0f);
    REQUIRE (aligned[6] == 2.0f);

    fast.clear();
    REQUIRE (fast.isPrepared());
    return 0;
}
