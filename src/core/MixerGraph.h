#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace triumph::mixer
{
inline constexpr const char* masterId = "master";

enum class ChannelKind
{
    track,
    returnChannel,
    bus
};

enum class RouteKind
{
    output,
    send,
    sidechain
};

struct BusLayout
{
    std::size_t inputChannels = 2;
    std::size_t outputChannels = 2;
    std::size_t sidechainChannels = 0;
    bool explicitLayout = false;

    bool isValid() const noexcept
    {
        return inputChannels <= 16 && outputChannels > 0
            && outputChannels <= 16 && sidechainChannels <= 16;
    }
};

struct Channel
{
    Channel() = default;
    Channel (std::string identifier, ChannelKind channelKind,
             std::string destination, BusLayout busLayout = {})
        : id (std::move (identifier)), kind (channelKind),
          outputDestination (std::move (destination)), layout (busLayout)
    {
    }

    std::string id;
    ChannelKind kind = ChannelKind::track;
    std::string outputDestination { masterId };
    BusLayout layout;
};

struct Route
{
    std::string id;
    std::string source;
    std::string destination;
    RouteKind kind = RouteKind::send;
    float gain = 1.0f;
    bool preFader = false;
    bool enabled = true;
};

struct Plan
{
    bool valid = false;
    std::string error;
    std::vector<Channel> channels;
    std::vector<Route> routes;
    std::vector<std::size_t> processOrder;

    std::vector<std::size_t> routesFrom (std::size_t channelIndex) const
    {
        std::vector<std::size_t> result;
        if (channelIndex >= channels.size())
            return result;
        for (std::size_t index = 0; index < routes.size(); ++index)
            if (routes[index].source == channels[channelIndex].id
                && routes[index].enabled)
                result.push_back (index);
        return result;
    }
};

inline Plan buildPlan (std::vector<Channel> channels,
                       std::vector<Route> routes)
{
    Plan result;
    result.channels = std::move (channels);

    std::unordered_map<std::string, std::size_t> channelIndex;
    channelIndex.reserve (result.channels.size());
    for (std::size_t index = 0; index < result.channels.size(); ++index)
    {
        const auto& channel = result.channels[index];
        if (channel.id.empty() || channel.id == masterId)
        {
            result.error = "Mixer channel identifiers must be non-empty and cannot use the master identifier.";
            return result;
        }
        if (! channelIndex.emplace (channel.id, index).second)
        {
            result.error = "Mixer channel identifiers must be unique.";
            return result;
        }
        if (! channel.layout.isValid())
        {
            result.error = "A mixer channel exposes an unsupported bus layout.";
            return result;
        }
    }

    for (const auto& channel : result.channels)
    {
        Route output;
        output.id = channel.id + ":output";
        output.source = channel.id;
        output.destination = channel.outputDestination.empty()
                                 ? masterId : channel.outputDestination;
        output.kind = RouteKind::output;
        routes.push_back (std::move (output));
    }

    std::vector<std::vector<std::size_t>> outgoing (result.channels.size());
    std::vector<std::size_t> indegree (result.channels.size(), 0);
    for (auto& route : routes)
    {
        route.gain = std::clamp (route.gain, 0.0f, 2.0f);
        if (! route.enabled)
        {
            result.routes.push_back (std::move (route));
            continue;
        }

        const auto source = channelIndex.find (route.source);
        if (source == channelIndex.end())
        {
            result.error = "A mixer route refers to a missing source channel.";
            return result;
        }
        if (route.destination.empty())
            route.destination = masterId;
        if (route.source == route.destination)
        {
            result.error = "A mixer channel cannot route to itself.";
            return result;
        }
        if (route.destination == masterId)
        {
            if (route.kind == RouteKind::sidechain)
            {
                result.error = "The master output cannot be a sidechain destination.";
                return result;
            }
            result.routes.push_back (std::move (route));
            continue;
        }

        const auto destination = channelIndex.find (route.destination);
        if (destination == channelIndex.end())
        {
            result.error = "A mixer route refers to a missing destination channel.";
            return result;
        }
        const auto destinationKind = result.channels[destination->second].kind;
        if (route.kind == RouteKind::output
            && destinationKind == ChannelKind::track)
        {
            result.error = "A channel output may target only a bus, return, or master.";
            return result;
        }
        const auto& sourceLayout = result.channels[source->second].layout;
        const auto& destinationLayout = result.channels[destination->second].layout;
        if (route.kind == RouteKind::sidechain
            && destinationLayout.explicitLayout)
        {
            if (destinationLayout.sidechainChannels == 0)
            {
                result.error = "A sidechain route targets a channel without a sidechain bus.";
                return result;
            }
            if (sourceLayout.outputChannels != 1
                && sourceLayout.outputChannels
                       != destinationLayout.sidechainChannels)
            {
                result.error = "A sidechain route has incompatible channel counts.";
                return result;
            }
        }
        else if (route.kind != RouteKind::sidechain
                 && sourceLayout.explicitLayout
                 && destinationLayout.explicitLayout
                 && sourceLayout.outputChannels != 1
                 && destinationLayout.inputChannels != 1
                 && sourceLayout.outputChannels
                        != destinationLayout.inputChannels)
        {
            result.error = "A mixer route has incompatible channel counts.";
            return result;
        }
        if (route.kind == RouteKind::send
            && destinationKind == ChannelKind::track)
        {
            result.error = "An audible send may target only a bus or return channel.";
            return result;
        }

        outgoing[source->second].push_back (destination->second);
        ++indegree[destination->second];
        result.routes.push_back (std::move (route));
    }

    result.processOrder.reserve (result.channels.size());
    std::vector<bool> emitted (result.channels.size(), false);
    while (result.processOrder.size() < result.channels.size())
    {
        auto found = result.channels.size();
        for (std::size_t index = 0; index < result.channels.size(); ++index)
            if (! emitted[index] && indegree[index] == 0)
            {
                found = index;
                break;
            }
        if (found == result.channels.size())
        {
            result.error = "The mixer routing graph contains a cycle.";
            result.processOrder.clear();
            return result;
        }

        emitted[found] = true;
        result.processOrder.push_back (found);
        for (const auto destination : outgoing[found])
            if (indegree[destination] > 0)
                --indegree[destination];
    }

    result.valid = true;
    return result;
}

enum class MonitorRole
{
    controlRoom,
    cue,
    listen,
    talkback
};

struct MonitorPath
{
    std::string id;
    MonitorRole role = MonitorRole::controlRoom;
    std::string source { masterId };
    std::size_t outputChannels = 2;
    float gain = 1.0f;
    bool muted = false;
    bool dimmed = false;
    std::size_t outputStartChannel = 0;
};

struct MonitorPlan
{
    bool valid = false;
    std::string error;
    std::vector<MonitorPath> paths;
};

inline MonitorPlan buildMonitorPlan (std::vector<MonitorPath> paths,
                                     const Plan& programPlan)
{
    MonitorPlan result;
    if (! programPlan.valid)
    {
        result.error = "The monitor plan requires a valid program mixer plan.";
        return result;
    }
    std::unordered_map<std::string, bool> channelIds;
    for (const auto& channel : programPlan.channels)
        channelIds.emplace (channel.id, true);
    std::unordered_map<std::string, bool> pathIds;
    for (auto& path : paths)
    {
        if (path.id.empty() || ! pathIds.emplace (path.id, true).second)
        {
            result.error = "Monitor path identifiers must be non-empty and unique.";
            return result;
        }
        if (path.outputChannels == 0 || path.outputChannels > 16
            || path.outputStartChannel >= 16
            || path.outputStartChannel + path.outputChannels > 16)
        {
            result.error = "A monitor path exposes an unsupported output layout.";
            return result;
        }
        if (path.source != masterId
            && channelIds.find (path.source) == channelIds.end())
        {
            result.error = "A monitor path references a missing program source.";
            return result;
        }
        if (path.role == MonitorRole::talkback && path.source == masterId)
        {
            result.error = "Talkback must use a dedicated input source, not the program master.";
            return result;
        }
        path.gain = std::clamp (path.gain, 0.0f, 2.0f);
    }
    result.paths = std::move (paths);
    result.valid = true;
    return result;
}
}
