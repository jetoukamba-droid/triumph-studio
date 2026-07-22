#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace triumph::plugin_rack
{
enum class Role
{
    instrument,
    insertEffect
};

enum class Isolation
{
    workerProcess,
    trustedInProcess
};

struct Slot
{
    std::string id;
    std::string ownerId;
    std::string stablePluginId;
    Role role = Role::insertEffect;
    Isolation isolation = Isolation::workerProcess;
    std::uint32_t order = 0;
    std::uint32_t inputChannels = 2;
    std::uint32_t outputChannels = 2;
    std::uint32_t sidechainChannels = 0;
    std::int64_t latencySamples = 0;
    bool bypassed = false;
    bool available = true;
};

struct OwnerPlan
{
    std::string ownerId;
    std::vector<std::size_t> slots;
    std::int64_t totalLatencySamples = 0;
    std::uint32_t finalChannels = 2;
    bool hasInstrument = false;
    bool requiresWorker = false;
};

struct Plan
{
    bool valid = false;
    std::string error;
    std::vector<Slot> slots;
    std::vector<OwnerPlan> owners;
};

inline Plan buildPlan (std::vector<Slot> slots,
                       std::uint32_t maximumSlotsPerOwner = 16)
{
    Plan result;
    result.slots = std::move (slots);
    std::unordered_map<std::string, std::size_t> slotIds;
    std::unordered_map<std::string, std::vector<std::size_t>> ownerSlots;
    for (std::size_t index = 0; index < result.slots.size(); ++index)
    {
        auto& slot = result.slots[index];
        if (slot.id.empty() || slot.ownerId.empty()
            || slot.stablePluginId.empty())
        {
            result.error = "Plug-in slots require stable slot, owner, and plug-in identifiers.";
            return result;
        }
        if (! slotIds.emplace (slot.id, index).second)
        {
            result.error = "Plug-in slot identifiers must be unique.";
            return result;
        }
        if (slot.inputChannels > 16 || slot.outputChannels == 0
            || slot.outputChannels > 16 || slot.sidechainChannels > 16)
        {
            result.error = "A plug-in slot exposes an unsupported bus layout.";
            return result;
        }
        slot.latencySamples = std::clamp<std::int64_t> (
            slot.latencySamples, 0, 262143);
        ownerSlots[slot.ownerId].push_back (index);
    }

    result.owners.reserve (ownerSlots.size());
    for (auto& [ownerId, indices] : ownerSlots)
    {
        if (indices.size() > maximumSlotsPerOwner)
        {
            result.error = "A channel exceeds the configured plug-in slot limit.";
            return result;
        }
        std::stable_sort (indices.begin(), indices.end(), [&] (auto a, auto b)
        {
            return result.slots[a].order < result.slots[b].order;
        });
        OwnerPlan owner;
        owner.ownerId = ownerId;
        owner.slots = indices;
        auto previousOrder = std::uint32_t { 0 };
        auto first = true;
        auto channels = std::uint32_t { 2 };
        for (const auto index : indices)
        {
            const auto& slot = result.slots[index];
            if (! first && slot.order == previousOrder)
            {
                result.error = "Plug-in slot order must be unique within a channel.";
                return result;
            }
            if (slot.role == Role::instrument)
            {
                if (! first || owner.hasInstrument)
                {
                    result.error = "An instrument must be the first and only instrument slot on a channel.";
                    return result;
                }
                owner.hasInstrument = true;
                channels = slot.outputChannels;
            }
            else
            {
                if (slot.inputChannels != channels
                    && slot.inputChannels != 1 && channels != 1)
                {
                    result.error = "Adjacent plug-in slots have incompatible main buses.";
                    return result;
                }
                channels = slot.outputChannels;
            }
            if (slot.available)
                owner.totalLatencySamples = std::min<std::int64_t> (
                    262143, owner.totalLatencySamples + slot.latencySamples);
            owner.requiresWorker = owner.requiresWorker
                || slot.isolation == Isolation::workerProcess;
            previousOrder = slot.order;
            first = false;
        }
        owner.finalChannels = channels;
        result.owners.push_back (std::move (owner));
    }
    std::sort (result.owners.begin(), result.owners.end(), [] (const auto& a,
                                                               const auto& b)
    {
        return a.ownerId < b.ownerId;
    });
    result.valid = true;
    return result;
}

inline std::string parameterTargetId (std::string_view slotId,
                                      std::string_view parameterId)
{
    return "plugin:" + std::string (slotId) + ":"
        + std::string (parameterId);
}

inline bool parseParameterTargetId (std::string_view target,
                                    std::string& slotId,
                                    std::string& parameterId)
{
    constexpr std::string_view prefix { "plugin:" };
    if (! target.starts_with (prefix))
        return false;
    target.remove_prefix (prefix.size());
    const auto separator = target.find (':');
    if (separator == std::string_view::npos || separator == 0
        || separator + 1 >= target.size())
        return false;
    slotId.assign (target.substr (0, separator));
    parameterId.assign (target.substr (separator + 1));
    return true;
}
}
