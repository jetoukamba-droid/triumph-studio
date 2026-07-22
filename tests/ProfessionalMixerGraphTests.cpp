#include "TestSupport.h"

#include "core/MixerGraph.h"
#include "core/PluginGraph.h"

#include <algorithm>

int main()
{
    using namespace triumph;

    const mixer::BusLayout stereo { 2, 2, 0, true };
    const mixer::BusLayout sidechainStereo { 2, 2, 2, true };
    const auto program = mixer::buildPlan ({
        { "dry", mixer::ChannelKind::track, "music", stereo },
        { "key", mixer::ChannelKind::track, "music", stereo },
        { "reverb", mixer::ChannelKind::returnChannel, "music", stereo },
        { "music", mixer::ChannelKind::bus, mixer::masterId,
          sidechainStereo }
    }, {
        { "reverb-send", "dry", "reverb", mixer::RouteKind::send,
          0.4f, true, true },
        { "duck-key", "key", "music", mixer::RouteKind::sidechain,
          1.0f, false, true }
    });
    REQUIRE (program.valid);
    REQUIRE (program.routes.size() == 6);

    const auto monitors = mixer::buildMonitorPlan ({
        { "control-room", mixer::MonitorRole::controlRoom,
          mixer::masterId, 2, 0.9f, false, false },
        { "cue-a", mixer::MonitorRole::cue, "music", 2,
          1.0f, false, false },
        { "listen", mixer::MonitorRole::listen, "dry", 2,
          1.0f, false, true },
        { "talkback", mixer::MonitorRole::talkback, "key", 2,
          0.5f, false, false }
    }, program);
    REQUIRE (monitors.valid);
    REQUIRE (monitors.paths.size() == 4);
    REQUIRE (program.routes.size() == 6);

    const auto badSidechain = mixer::buildPlan ({
        { "mono-key", mixer::ChannelKind::track, mixer::masterId,
          { 1, 1, 0, true } },
        { "target", mixer::ChannelKind::bus, mixer::masterId,
          { 2, 2, 0, true } }
    }, {
        { "sc", "mono-key", "target", mixer::RouteKind::sidechain,
          1.0f, false, true }
    });
    REQUIRE (! badSidechain.valid);

    const std::vector<plugins::Node> nodes {
        { "dry", 192 }, { "key", 64 }, { "reverb", 512 },
        { "music", 128 }, { "master", 0 }
    };
    const std::vector<plugins::Edge> edges {
        { "dry", "music", plugins::EdgeKind::audio, 0 },
        { "key", "music", plugins::EdgeKind::sidechain, 0 },
        { "dry", "reverb", plugins::EdgeKind::send, 0 },
        { "reverb", "music", plugins::EdgeKind::audio, 0 },
        { "music", "master", plugins::EdgeKind::audio, 0 }
    };
    const auto pdc = plugins::buildCompensationPlan (nodes, edges);
    REQUIRE (pdc.valid);
    REQUIRE (pdc.graphLatencySamples == 832);
    const auto keyRoute = std::find_if (
        pdc.edges.begin(), pdc.edges.end(), [] (const auto& edge)
    {
        return edge.sourceId == "key" && edge.destinationId == "music";
    });
    REQUIRE (keyRoute != pdc.edges.end());
    REQUIRE (keyRoute->delaySamples == 640);
    return 0;
}
