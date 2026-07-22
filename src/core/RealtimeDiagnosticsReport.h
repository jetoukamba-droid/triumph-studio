#pragma once

#include "core/RealtimeEngineState.h"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace triumph::realtime
{
struct DiagnosticsReportInput
{
    std::string applicationName;
    std::string versionLabel;
    std::string releaseChannel;
    std::string projectName;
    std::string audioDevice;
    double sampleRate = 0.0;
    int blockSize = 0;
    TelemetrySnapshot realtime;
    std::uint64_t pluginLastProcessNanoseconds = 0;
    std::uint64_t pluginMaximumProcessNanoseconds = 0;
    std::uint64_t pluginDeadlineMisses = 0;
    std::uint64_t containedPluginExceptions = 0;
    std::uint64_t instrumentContainedPluginExceptions = 0;
    std::uint64_t audioDeviceInterruptions = 0;
    std::uint64_t audioDeviceRecoveryAttempts = 0;
    std::uint64_t audioDeviceRecoveries = 0;
    std::uint64_t audioDeviceRecoveryFailures = 0;
    std::uint64_t callbackStallWarnings = 0;
    std::uint64_t sustainedSilenceWarnings = 0;
    std::string syncSource;
    bool syncLocked = false;
    double externalTempoBpm = 0.0;
    double mtcPositionSeconds = 0.0;
};

inline bool hasActionableRealtimeFault (
    const DiagnosticsReportInput& input) noexcept
{
    const auto& realtime = input.realtime;
    return realtime.suspectedDropouts > 0
           || realtime.deadlineMisses > 0
           || realtime.missingRenderStateBlocks > 0
           || realtime.oversizedAudioBlocks > 0
           || realtime.readAheadStarvationBlocks > 0
           || realtime.parameterQueueOverflows > 0
           || input.pluginDeadlineMisses > 0
           || input.containedPluginExceptions > 0
           || input.audioDeviceRecoveryFailures > 0
           || input.callbackStallWarnings > 0
           || input.sustainedSilenceWarnings > 0;
}

inline std::string millisecondsFromNanoseconds (std::uint64_t nanoseconds)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision (3)
           << static_cast<double> (nanoseconds) / 1000000.0;
    return stream.str();
}

inline std::string buildDiagnosticsReport (
    const DiagnosticsReportInput& input)
{
    const auto& realtime = input.realtime;
    std::ostringstream report;
    report << input.applicationName << " Realtime Diagnostics\n"
           << "Version: " << input.versionLabel << " / "
           << input.releaseChannel << '\n'
           << "Project: "
           << (input.projectName.empty() ? "Untitled Project"
                                         : input.projectName) << '\n'
           << "Audio device: "
           << (input.audioDevice.empty() ? "No active device"
                                         : input.audioDevice) << '\n'
           << "Sample rate: " << std::fixed << std::setprecision (1)
           << input.sampleRate << " Hz\n"
           << "Block size: " << input.blockSize << " samples\n\n"
           << "Realtime callbacks\n"
           << "  Callback count: " << realtime.callbackCount << '\n'
           << "  Average callback: "
           << millisecondsFromNanoseconds (
                  static_cast<std::uint64_t> (
                      realtime.averageCallbackNanoseconds())) << " ms\n"
           << "  Maximum callback: "
           << millisecondsFromNanoseconds (
                  realtime.maximumCallbackNanoseconds) << " ms\n"
           << "  Deadline misses: " << realtime.deadlineMisses << '\n'
           << "  Suspected dropouts: " << realtime.suspectedDropouts
           << "\n\nFault attribution\n"
           << "  Missing render-state blocks: "
           << realtime.missingRenderStateBlocks << '\n'
           << "  Oversized audio blocks: "
           << realtime.oversizedAudioBlocks << '\n'
           << "  Read-ahead/source starvation blocks: "
           << realtime.readAheadStarvationBlocks << '\n'
           << "  Parameter queue overflows: "
           << realtime.parameterQueueOverflows << '\n'
           << "  Render snapshot swaps: " << realtime.snapshotSwaps
           << '\n'
           << "  Active render generation: "
           << realtime.renderGeneration << "\n\nRender graph\n"
           << "  Graph builds: " << realtime.graphBuildCount << '\n'
           << "  Average graph build: "
           << millisecondsFromNanoseconds (
                  static_cast<std::uint64_t> (
                      realtime.averageGraphBuildNanoseconds()))
           << " ms\n"
           << "  Maximum graph build: "
           << millisecondsFromNanoseconds (
                  realtime.maximumGraphBuildNanoseconds) << " ms\n"
           << "  Retire backlog high watermark: "
           << realtime.renderRetireBacklogHighWatermark
           << "\n\nPlug-in runtime\n"
           << "  Last process: "
           << millisecondsFromNanoseconds (
                  input.pluginLastProcessNanoseconds) << " ms\n"
           << "  Maximum process: "
           << millisecondsFromNanoseconds (
                  input.pluginMaximumProcessNanoseconds) << " ms\n"
           << "  Deadline misses: " << input.pluginDeadlineMisses << '\n'
           << "  Contained exceptions: "
           << input.containedPluginExceptions << '\n'
           << "  Instrument contained exceptions: "
           << input.instrumentContainedPluginExceptions
           << "\n\nAudio device recovery\n"
           << "  Device restarts: " << realtime.deviceRestartCount << '\n'
           << "  Variable block/rate changes: "
           << realtime.variableBlockSizeChanges << '\n'
           << "  Interruptions: " << input.audioDeviceInterruptions << '\n'
           << "  Recovery attempts: "
           << input.audioDeviceRecoveryAttempts << '\n'
           << "  Recoveries: " << input.audioDeviceRecoveries << '\n'
           << "  Recovery failures: "
           << input.audioDeviceRecoveryFailures << "\n\nWarnings\n"
           << "  Callback-stall warnings: "
           << input.callbackStallWarnings << '\n'
           << "  Sustained-silence warnings: "
           << input.sustainedSilenceWarnings << "\n\nExternal sync\n"
           << "  Source: "
           << (input.syncSource.empty() ? "Internal" : input.syncSource)
           << '\n'
           << "  State: " << (input.syncLocked ? "locked" : "waiting")
           << '\n'
           << "  Tempo: " << std::fixed << std::setprecision (2)
           << input.externalTempoBpm << " BPM\n"
           << "  MTC position: " << std::fixed << std::setprecision (3)
           << input.mtcPositionSeconds << " s\n\n"
           << "Verdict: "
           << (hasActionableRealtimeFault (input) ? "Action required"
                                                  : "Clean");
    return report.str();
}
} // namespace triumph::realtime
