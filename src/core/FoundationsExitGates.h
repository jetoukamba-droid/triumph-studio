#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace triumph::foundations
{
enum class Area
{
    fr1RealtimeStreaming,
    fr2RecordingRecovery,
    fr3PluginIsolation,
    fr4MixerMonitoring
};

enum class Status
{
    notStarted,
    partial,
    complete
};

struct Evidence
{
    bool coreImplementation = false;
    bool deterministicTests = false;
    bool userVisibleDiagnostics = false;
    bool referenceHardwareMatrix = false;
    bool thirdPartyCompatibilityMatrix = false;
    bool forcedFailureRecoveryProof = false;
};

struct Gate
{
    Area area = Area::fr1RealtimeStreaming;
    Status status = Status::notStarted;
    Evidence evidence;
    std::string_view blocker;
};

inline constexpr std::string_view areaName (Area area) noexcept
{
    switch (area)
    {
        case Area::fr1RealtimeStreaming: return "FR-1 realtime render and streaming";
        case Area::fr2RecordingRecovery: return "FR-2 recording and recovery";
        case Area::fr3PluginIsolation:   return "FR-3 plug-in isolation";
        case Area::fr4MixerMonitoring:   return "FR-4 mixer and monitoring";
    }

    return "Unknown foundation area";
}

inline constexpr std::string_view statusName (Status status) noexcept
{
    switch (status)
    {
        case Status::notStarted: return "not-started";
        case Status::partial:    return "partial";
        case Status::complete:   return "complete";
    }

    return "unknown";
}

inline constexpr Status evaluateFr1 (Evidence evidence) noexcept
{
    if (evidence.coreImplementation
        && evidence.deterministicTests
        && evidence.userVisibleDiagnostics
        && evidence.referenceHardwareMatrix
        && evidence.forcedFailureRecoveryProof)
        return Status::complete;

    if (evidence.coreImplementation || evidence.deterministicTests || evidence.userVisibleDiagnostics)
        return Status::partial;

    return Status::notStarted;
}

inline constexpr Status evaluateFr2 (Evidence evidence) noexcept
{
    if (evidence.coreImplementation
        && evidence.deterministicTests
        && evidence.referenceHardwareMatrix
        && evidence.forcedFailureRecoveryProof)
        return Status::complete;

    if (evidence.coreImplementation || evidence.deterministicTests || evidence.forcedFailureRecoveryProof)
        return Status::partial;

    return Status::notStarted;
}

inline constexpr Status evaluateFr3 (Evidence evidence) noexcept
{
    if (evidence.coreImplementation
        && evidence.deterministicTests
        && evidence.userVisibleDiagnostics
        && evidence.thirdPartyCompatibilityMatrix
        && evidence.forcedFailureRecoveryProof)
        return Status::complete;

    if (evidence.coreImplementation || evidence.deterministicTests || evidence.userVisibleDiagnostics)
        return Status::partial;

    return Status::notStarted;
}

inline constexpr Status evaluateFr4 (Evidence evidence) noexcept
{
    if (evidence.coreImplementation
        && evidence.deterministicTests
        && evidence.userVisibleDiagnostics
        && evidence.referenceHardwareMatrix)
        return Status::complete;

    if (evidence.coreImplementation || evidence.deterministicTests || evidence.userVisibleDiagnostics)
        return Status::partial;

    return Status::notStarted;
}

inline constexpr std::array<Gate, 4> currentFr1ToFr4Gates() noexcept
{
    constexpr Evidence fr1 { true, true, true, false, false, false };
    constexpr Evidence fr2 { true, true, true, false, false, false };
    constexpr Evidence fr3 { true, true, true, false, false, false };
    constexpr Evidence fr4 { true, true, true, false, false, false };

    return {{
        { Area::fr1RealtimeStreaming,
          evaluateFr1 (fr1),
          fr1,
          "Needs actual disk read-ahead underrun attribution and long-session device/fault matrix." },
        { Area::fr2RecordingRecovery,
          evaluateFr2 (fr2),
          fr2,
          "Needs Windows reference hardware recording suite and forced-termination recovery proof." },
        { Area::fr3PluginIsolation,
          evaluateFr3 (fr3),
          fr3,
          "Needs broad third-party plug-in compatibility and cross-process live runtime isolation." },
        { Area::fr4MixerMonitoring,
          evaluateFr4 (fr4),
          fr4,
          "Needs hardware null, multi-output monitoring, and third-party offline parity gates." }
    }};
}

inline constexpr std::size_t countByStatus (const std::array<Gate, 4>& gates, Status status) noexcept
{
    std::size_t total = 0;

    for (const auto& gate : gates)
        if (gate.status == status)
            ++total;

    return total;
}
} // namespace triumph::foundations
