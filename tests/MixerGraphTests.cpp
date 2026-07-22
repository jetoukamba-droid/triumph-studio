#include "TestSupport.h"

#include "core/MixerGraph.h"
#include "core/MixerMath.h"

#include <algorithm>
#include <iterator>

int main()
{
    using namespace triumph::mixer;

    const auto routed = buildPlan (
        { { "audio", ChannelKind::track, "bus" },
          { "instrument", ChannelKind::track, "bus" },
          { "return", ChannelKind::returnChannel, "bus" },
          { "bus", ChannelKind::bus, masterId } },
        { { "send-a", "audio", "return", RouteKind::send,
            0.35f, true, true },
          { "sidechain-a", "instrument", "bus", RouteKind::sidechain,
            1.0f, false, true } });

    REQUIRE (routed.valid);
    REQUIRE (routed.processOrder.size() == 4);
    const auto positionOf = [&routed] (const std::string& id)
    {
        const auto channel = std::find_if (
            routed.channels.begin(), routed.channels.end(),
            [&id] (const auto& candidate) { return candidate.id == id; });
        const auto index = static_cast<std::size_t> (
            std::distance (routed.channels.begin(), channel));
        const auto position = std::find (routed.processOrder.begin(),
                                         routed.processOrder.end(), index);
        return static_cast<std::size_t> (
            std::distance (routed.processOrder.begin(), position));
    };
    REQUIRE (positionOf ("audio") < positionOf ("return"));
    REQUIRE (positionOf ("return") < positionOf ("bus"));
    REQUIRE (positionOf ("instrument") < positionOf ("bus"));
    REQUIRE (routed.routes.size() == 6);

    const auto clamped = buildPlan (
        { { "track", ChannelKind::track, masterId },
          { "return", ChannelKind::returnChannel, masterId } },
        { { "send", "track", "return", RouteKind::send,
            4.0f, false, true } });
    REQUIRE (clamped.valid);
    const auto send = std::find_if (clamped.routes.begin(), clamped.routes.end(),
                                    [] (const auto& route)
    {
        return route.id == "send";
    });
    REQUIRE (send != clamped.routes.end());
    REQUIRE (send->gain == 2.0f);

    const auto cycle = buildPlan (
        { { "bus-a", ChannelKind::bus, "bus-b" },
          { "bus-b", ChannelKind::bus, "bus-a" } }, {});
    REQUIRE (! cycle.valid);
    REQUIRE (cycle.error.find ("cycle") != std::string::npos);

    const auto badSend = buildPlan (
        { { "track-a", ChannelKind::track, masterId },
          { "track-b", ChannelKind::track, masterId } },
        { { "send", "track-a", "track-b", RouteKind::send,
            0.5f, false, true } });
    REQUIRE (! badSend.valid);

    const auto sidechainToTrack = buildPlan (
        { { "key", ChannelKind::track, masterId },
          { "target", ChannelKind::track, masterId } },
        { { "sc", "key", "target", RouteKind::sidechain,
            1.0f, false, true } });
    REQUIRE (sidechainToTrack.valid);
    REQUIRE (sidechainToTrack.processOrder.size() == 2);
    REQUIRE (sidechainToTrack.processOrder[0] == 0);
    REQUIRE (sidechainToTrack.processOrder[1] == 1);
    REQUIRE (sidechainDuckGain (0.0f) == 1.0f);
    REQUIRE (sidechainDuckGain (1.0f) > 0.14f);
    REQUIRE (sidechainDuckGain (1.0f) < 0.15f);
    REQUIRE (logarithmicSpectrumBin (0, 128, 2048) == 1);
    REQUIRE (logarithmicSpectrumBin (127, 128, 2048) == 1023);

    return 0;
}
