#include "core/PluginGraph.h"
#include "TestSupport.h"

int main()
{
    using namespace triumph::plugins;

    const auto parallel = buildCompensationPlan ({
        { "vocal", 128, false, false, true },
        { "drums", 512, false, false, true },
        { "frozen-keys", 2048, false, true, true },
        { "missing-bass", 1024, false, false, false }
    });

    REQUIRE (parallel.valid);
    REQUIRE (parallel.graphLatencySamples == 512);
    REQUIRE (parallel.nodes.size() == 4);
    REQUIRE (parallel.nodes[0].delaySamples == 384);
    REQUIRE (parallel.nodes[1].delaySamples == 0);
    REQUIRE (parallel.nodes[2].effectiveLatencySamples == 0);
    REQUIRE (parallel.nodes[2].delaySamples == 512);
    REQUIRE (parallel.nodes[3].effectiveLatencySamples == 0);

    const std::vector<Node> routingNodes {
        { "dry", 0, false, false, true },
        { "vocal", 128, false, false, true },
        { "drums", 512, false, false, true },
        { "reverb", 384, false, false, true },
        { "master", 0, false, false, true }
    };
    const std::vector<Edge> routingEdges {
        { "dry", "master", EdgeKind::audio, 0 },
        { "vocal", "reverb", EdgeKind::send, 0 },
        { "drums", "reverb", EdgeKind::sidechain, 0 },
        { "reverb", "master", EdgeKind::audio, 0 },
        { "vocal", "master", EdgeKind::audio, 0 }
    };
    const auto routing = buildCompensationPlan (routingNodes, routingEdges);
    REQUIRE (routing.valid);
    REQUIRE (routing.graphLatencySamples == 896);
    REQUIRE (routing.edges[1].delaySamples == 384);
    REQUIRE (routing.edges[2].delaySamples == 0);
    REQUIRE (routing.edges[0].delaySamples == 896);
    REQUIRE (routing.edges[3].delaySamples == 0);
    REQUIRE (routing.edges[4].delaySamples == 768);
    REQUIRE (routing.nodes[3].inputArrivalSamples == 512);
    REQUIRE (routing.nodes[3].outputArrivalSamples == 896);

    const auto manual = buildCompensationPlan ({
        { "corrected", 128, false, false, true, 64, false },
        { "reference", 512, false, false, true, 0, false }
    });
    REQUIRE (manual.nodes[0].effectiveLatencySamples == 192);
    REQUIRE (manual.nodes[0].delaySamples == 320);

    const auto bypassed = buildCompensationPlan ({
        { "bypassed-insert", 256, true, false, true },
        { "reference", 512, false, false, true }
    });
    REQUIRE (bypassed.nodes[0].effectiveLatencySamples == 256);
    REQUIRE (bypassed.nodes[0].delaySamples == 256);

    Options lowLatencyOptions;
    lowLatencyOptions.lowLatencyMonitoring = true;
    const auto lowLatency = buildCompensationPlan ({
        { "armed-vocal", 0, false, false, true, 0, true },
        { "mastering-chain", 1024, false, false, true, 0, false }
    }, lowLatencyOptions);
    REQUIRE (lowLatency.valid);
    REQUIRE (lowLatency.nodes[0].delaySamples == 0);
    REQUIRE (lowLatency.monitoringPathUncompensated);

    const auto cycle = buildCompensationPlan (
        { { "a", 0 }, { "b", 0 } },
        { { "a", "b" }, { "b", "a" } });
    REQUIRE (! cycle.valid);

    REQUIRE (compensatedTimelineSample (1000, 128) == 872);
    REQUIRE (compensatedTimelineSample (64, 128) == 0);
    REQUIRE (compensatedAutomationSample (2048, 512) == 1536);
    return 0;
}
