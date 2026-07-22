#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace triumph::plugins
{
enum class EdgeKind
{
    audio,
    send,
    sidechain
};

struct Node
{
    std::string id;
    std::int64_t latencySamples = 0;
    bool bypassed = false;
    bool frozen = false;
    bool available = true;
    std::int64_t manualLatencyOffsetSamples = 0;
    bool monitored = false;
};

struct Edge
{
    std::string sourceId;
    std::string destinationId;
    EdgeKind kind = EdgeKind::audio;
    std::int64_t transportLatencySamples = 0;
};

struct Options
{
    bool lowLatencyMonitoring = false;
    std::int64_t maximumCompensationSamples = 262143;
};

struct Compensation
{
    std::string id;
    std::int64_t effectiveLatencySamples = 0;
    std::int64_t inputArrivalSamples = 0;
    std::int64_t outputArrivalSamples = 0;
    std::int64_t delaySamples = 0;
    std::int64_t automationLookaheadSamples = 0;
    bool lowLatencyProtected = false;
};

struct EdgeCompensation
{
    std::string sourceId;
    std::string destinationId;
    EdgeKind kind = EdgeKind::audio;
    std::int64_t sourceArrivalSamples = 0;
    std::int64_t destinationArrivalSamples = 0;
    std::int64_t delaySamples = 0;
    bool lowLatencyBypassed = false;
};

struct Plan
{
    std::int64_t graphLatencySamples = 0;
    std::vector<Compensation> nodes;
    std::vector<EdgeCompensation> edges;
    bool valid = true;
    bool compensationClamped = false;
    bool monitoringPathUncompensated = false;
    std::string error;
};

namespace detail
{
inline std::int64_t clampedDelay (std::int64_t requested,
                                  const Options& options,
                                  bool& wasClamped) noexcept
{
    requested = std::max<std::int64_t> (0, requested);
    const auto maximum = std::max<std::int64_t> (
        0, options.maximumCompensationSamples);
    if (requested > maximum)
    {
        wasClamped = true;
        return maximum;
    }
    return requested;
}
}

inline Plan buildCompensationPlan (const std::vector<Node>& nodes,
                                   const std::vector<Edge>& edges,
                                   const Options& options = {})
{
    Plan result;
    result.nodes.reserve (nodes.size());
    result.edges.resize (edges.size());

    std::unordered_map<std::string, std::size_t> indices;
    indices.reserve (nodes.size());
    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        if (nodes[index].id.empty())
        {
            result.valid = false;
            result.error = "The delay-compensation graph contains an unnamed node.";
            return result;
        }
        if (! indices.emplace (nodes[index].id, index).second)
        {
            result.valid = false;
            result.error = "The delay-compensation graph contains duplicate node IDs.";
            return result;
        }

        // Host bypass preserves the processor's reported latency. Removing it
        // would move every parallel path at the instant bypass is toggled.
        const auto plugInActive = nodes[index].available
                                  && ! nodes[index].frozen;
        const auto reportedLatency = plugInActive
            ? std::max<std::int64_t> (0, nodes[index].latencySamples) : 0;
        const auto correctedLatency = std::max<std::int64_t> (
            0, reportedLatency + nodes[index].manualLatencyOffsetSamples);
        result.nodes.push_back ({ nodes[index].id,
                                  correctedLatency,
                                  0,
                                  correctedLatency,
                                  0,
                                  0,
                                  options.lowLatencyMonitoring
                                      && nodes[index].monitored });
    }

    std::vector<std::vector<std::size_t>> incoming (nodes.size());
    std::vector<std::vector<std::size_t>> outgoing (nodes.size());
    std::vector<int> indegree (nodes.size(), 0);
    for (std::size_t edgeIndex = 0; edgeIndex < edges.size(); ++edgeIndex)
    {
        const auto source = indices.find (edges[edgeIndex].sourceId);
        const auto destination = indices.find (edges[edgeIndex].destinationId);
        if (source == indices.end() || destination == indices.end())
        {
            result.valid = false;
            result.error = "A delay-compensation route references a missing node.";
            return result;
        }
        if (source->second == destination->second)
        {
            result.valid = false;
            result.error = "A delay-compensation route cannot feed itself.";
            return result;
        }
        outgoing[source->second].push_back (edgeIndex);
        incoming[destination->second].push_back (edgeIndex);
        ++indegree[destination->second];
        result.edges[edgeIndex] = { edges[edgeIndex].sourceId,
                                    edges[edgeIndex].destinationId,
                                    edges[edgeIndex].kind,
                                    0, 0, 0, false };
    }

    std::queue<std::size_t> ready;
    for (std::size_t index = 0; index < nodes.size(); ++index)
        if (indegree[index] == 0)
            ready.push (index);

    std::vector<std::size_t> order;
    order.reserve (nodes.size());
    while (! ready.empty())
    {
        const auto node = ready.front();
        ready.pop();
        order.push_back (node);
        for (const auto edgeIndex : outgoing[node])
        {
            const auto destination = indices.at (edges[edgeIndex].destinationId);
            if (--indegree[destination] == 0)
                ready.push (destination);
        }
    }

    if (order.size() != nodes.size())
    {
        result.valid = false;
        result.error = "The delay-compensation graph contains a routing cycle.";
        return result;
    }

    for (const auto nodeIndex : order)
    {
        auto targetArrival = std::int64_t { 0 };
        for (const auto edgeIndex : incoming[nodeIndex])
        {
            const auto source = indices.at (edges[edgeIndex].sourceId);
            const auto arrival = result.nodes[source].outputArrivalSamples
                + std::max<std::int64_t> (
                    0, edges[edgeIndex].transportLatencySamples);
            result.edges[edgeIndex].sourceArrivalSamples = arrival;
            targetArrival = std::max (targetArrival, arrival);
        }

        for (const auto edgeIndex : incoming[nodeIndex])
        {
            const auto source = indices.at (edges[edgeIndex].sourceId);
            const auto protectsMonitoring = options.lowLatencyMonitoring
                                            && nodes[source].monitored;
            const auto requested = targetArrival
                                   - result.edges[edgeIndex].sourceArrivalSamples;
            if (protectsMonitoring && requested > 0)
            {
                result.edges[edgeIndex].lowLatencyBypassed = true;
                result.monitoringPathUncompensated = true;
            }
            else
            {
                result.edges[edgeIndex].delaySamples = detail::clampedDelay (
                    requested, options, result.compensationClamped);
            }
            result.edges[edgeIndex].destinationArrivalSamples =
                result.edges[edgeIndex].sourceArrivalSamples
                + result.edges[edgeIndex].delaySamples;
        }

        auto compensatedInputArrival = std::int64_t { 0 };
        for (const auto edgeIndex : incoming[nodeIndex])
            compensatedInputArrival = std::max (
                compensatedInputArrival,
                result.edges[edgeIndex].destinationArrivalSamples);

        result.nodes[nodeIndex].inputArrivalSamples = compensatedInputArrival;
        result.nodes[nodeIndex].outputArrivalSamples = compensatedInputArrival
            + result.nodes[nodeIndex].effectiveLatencySamples;
        result.nodes[nodeIndex].automationLookaheadSamples = compensatedInputArrival;
    }

    for (std::size_t index = 0; index < nodes.size(); ++index)
        if (outgoing[index].empty())
            result.graphLatencySamples = std::max (
                result.graphLatencySamples,
                result.nodes[index].outputArrivalSamples);

    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        if (! outgoing[index].empty())
        {
            auto maximumRouteDelay = std::int64_t { 0 };
            for (const auto edgeIndex : outgoing[index])
                maximumRouteDelay = std::max (
                    maximumRouteDelay, result.edges[edgeIndex].delaySamples);
            result.nodes[index].delaySamples = maximumRouteDelay;
            continue;
        }

        const auto requested = result.graphLatencySamples
                               - result.nodes[index].outputArrivalSamples;
        if (options.lowLatencyMonitoring && nodes[index].monitored && requested > 0)
        {
            result.monitoringPathUncompensated = true;
            continue;
        }
        result.nodes[index].delaySamples = detail::clampedDelay (
            requested, options, result.compensationClamped);
    }

    return result;
}

inline Plan buildCompensationPlan (const std::vector<Node>& nodes,
                                   const Options& options = {})
{
    auto graphNodes = nodes;
    graphNodes.push_back ({ "__triumph_master__", 0, false, false, true, 0, false });
    std::vector<Edge> graphEdges;
    graphEdges.reserve (nodes.size());
    for (const auto& node : nodes)
        graphEdges.push_back ({ node.id, "__triumph_master__", EdgeKind::audio, 0 });

    auto plan = buildCompensationPlan (graphNodes, graphEdges, options);
    if (! plan.valid)
        return plan;

    for (std::size_t index = 0; index < nodes.size(); ++index)
    {
        plan.nodes[index].delaySamples = plan.edges[index].delaySamples;
        plan.nodes[index].automationLookaheadSamples =
            plan.nodes[index].inputArrivalSamples;
    }
    plan.nodes.resize (nodes.size());
    return plan;
}

inline std::int64_t compensatedTimelineSample (std::int64_t sourceSample,
                                               std::int64_t processingLatencySamples)
{
    return std::max<std::int64_t> (
        0, sourceSample - std::max<std::int64_t> (0, processingLatencySamples));
}

inline std::int64_t compensatedAutomationSample (
    std::int64_t timelineSample,
    std::int64_t upstreamArrivalSamples)
{
    return compensatedTimelineSample (timelineSample, upstreamArrivalSamples);
}
}
