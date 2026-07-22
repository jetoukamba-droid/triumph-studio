#include "core/RealtimeEngineState.h"
#include "core/RealtimeDiagnosticsReport.h"
#include "TestSupport.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

namespace
{
struct TestSnapshot
{
    TestSnapshot (int newValue, std::atomic<int>& destructionCounter)
        : value (newValue), destructions (destructionCounter)
    {
    }

    ~TestSnapshot() { destructions.fetch_add (1); }

    int value = 0;
    std::atomic<int>& destructions;
};
}

int main()
{
    using namespace triumph::realtime;

    ParameterQueue<4> queue;
    REQUIRE (queue.push ({ ParameterCommand::Type::masterGain, 0.25f }));
    REQUIRE (queue.push ({ ParameterCommand::Type::masterMute, 1.0f }));
    REQUIRE (queue.push ({ ParameterCommand::Type::masterGain, 0.75f }));
    REQUIRE (! queue.push ({ ParameterCommand::Type::masterGain, 1.0f }));

    ParameterCommand command;
    REQUIRE (queue.pop (command));
    REQUIRE (command.type == ParameterCommand::Type::masterGain);
    REQUIRE (command.value == 0.25f);
    REQUIRE (queue.pop (command));
    REQUIRE (command.type == ParameterCommand::Type::masterMute);
    REQUIRE (queue.pop (command));
    REQUIRE (! queue.pop (command));

    Telemetry telemetry;
    telemetry.recordCallback (50, 100);
    telemetry.recordCallback (150, 100);
    telemetry.recordCallback (75, 100, true);
    telemetry.recordSnapshotSwap (42);
    telemetry.recordParameterQueueOverflow();
    telemetry.recordMissingRenderStateBlock();
    telemetry.recordOversizedAudioBlock();
    telemetry.recordReadAheadStarvationBlock();
    telemetry.recordDeviceRestart (256, 48000000);
    telemetry.recordDeviceRestart (512, 48000000);
    telemetry.recordDeviceRestart (512, 44100000);
    telemetry.recordGraphBuild (1000, 0);
    telemetry.recordGraphBuild (3000, 4);
    const auto status = telemetry.snapshot();
    REQUIRE (status.callbackCount == 3);
    REQUIRE (status.totalCallbackNanoseconds == 275);
    REQUIRE (status.maximumCallbackNanoseconds == 150);
    REQUIRE (status.deadlineMisses == 1);
    REQUIRE (status.suspectedDropouts == 5);
    REQUIRE (status.snapshotSwaps == 1);
    REQUIRE (status.parameterQueueOverflows == 1);
    REQUIRE (status.renderGeneration == 42);
    REQUIRE (status.missingRenderStateBlocks == 1);
    REQUIRE (status.oversizedAudioBlocks == 1);
    REQUIRE (status.readAheadStarvationBlocks == 1);
    REQUIRE (status.deviceRestartCount == 3);
    REQUIRE (status.variableBlockSizeChanges == 2);
    REQUIRE (status.graphBuildCount == 2);
    REQUIRE (status.totalGraphBuildNanoseconds == 4000);
    REQUIRE (status.maximumGraphBuildNanoseconds == 3000);
    REQUIRE (status.renderRetireBacklogHighWatermark == 4);
    REQUIRE (status.averageCallbackNanoseconds() > 91.0);
    REQUIRE (status.averageCallbackNanoseconds() < 92.0);
    REQUIRE (status.averageGraphBuildNanoseconds() == 2000.0);

    DiagnosticsReportInput reportInput;
    reportInput.applicationName = "Triumph Studio";
    reportInput.versionLabel = "0.22.0-beta.1";
    reportInput.releaseChannel = "Closed Beta 1";
    reportInput.projectName = "Realtime Session";
    reportInput.audioDevice = "Test Output";
    reportInput.sampleRate = 48000.0;
    reportInput.blockSize = 256;
    reportInput.realtime = status;
    reportInput.pluginMaximumProcessNanoseconds = 5000000;
    reportInput.pluginDeadlineMisses = 2;
    reportInput.containedPluginExceptions = 1;
    reportInput.audioDeviceInterruptions = 1;
    reportInput.audioDeviceRecoveryAttempts = 1;
    reportInput.audioDeviceRecoveries = 1;
    reportInput.callbackStallWarnings = 1;
    reportInput.syncSource = "MIDI Clock";
    reportInput.syncLocked = true;
    reportInput.externalTempoBpm = 120.0;
    const auto report = buildDiagnosticsReport (reportInput);
    REQUIRE (hasActionableRealtimeFault (reportInput));
    REQUIRE (report.find ("Triumph Studio Realtime Diagnostics")
             != std::string::npos);
    REQUIRE (report.find ("Missing render-state blocks: 1")
             != std::string::npos);
    REQUIRE (report.find ("Read-ahead/source starvation blocks: 1")
             != std::string::npos);
    REQUIRE (report.find ("Contained exceptions: 1")
             != std::string::npos);
    REQUIRE (report.find ("Verdict: Action required")
             != std::string::npos);

    DiagnosticsReportInput cleanReportInput;
    cleanReportInput.applicationName = "Triumph Studio";
    cleanReportInput.realtime.callbackCount = 1;
    cleanReportInput.realtime.totalCallbackNanoseconds = 100;
    cleanReportInput.realtime.maximumCallbackNanoseconds = 100;
    REQUIRE (! hasActionableRealtimeFault (cleanReportInput));
    REQUIRE (buildDiagnosticsReport (cleanReportInput).find (
                 "Verdict: Clean") != std::string::npos);

    std::atomic<int> destructions { 0 };
    SnapshotExchange<TestSnapshot> exchange;
    exchange.publish (std::make_unique<TestSnapshot> (1, destructions));
    {
        auto firstReader = exchange.acquire();
        REQUIRE (firstReader);
        REQUIRE (firstReader->value == 1);
        exchange.publish (std::make_unique<TestSnapshot> (2, destructions));
        REQUIRE (destructions.load() == 0);
        REQUIRE (exchange.retiredCount() == 1);
        auto secondReader = exchange.acquire();
        REQUIRE (secondReader->value == 2);
        REQUIRE (firstReader->value == 1);
    }
    exchange.reclaim();
    REQUIRE (destructions.load() == 1);
    REQUIRE (exchange.retiredCount() == 0);

    exchange.clear();
    REQUIRE (destructions.load() == 2);
    REQUIRE (! exchange.acquire());

    // Stress one control publisher against one real-time reader. Every reader
    // must see a complete generation and no generation may be reclaimed while
    // its handle is live.
    SnapshotExchange<TestSnapshot> stressExchange;
    std::atomic<int> stressDestructions { 0 };
    std::atomic<bool> writerFinished { false };
    std::atomic<bool> invalidGenerationSeen { false };
    stressExchange.publish (
        std::make_unique<TestSnapshot> (1, stressDestructions));
    std::thread reader ([&]
    {
        while (! writerFinished.load())
        {
            auto snapshot = stressExchange.acquire();
            if (! snapshot || snapshot->value <= 0)
                invalidGenerationSeen.store (true);
        }
    });
    for (int generation = 2; generation <= 10000; ++generation)
        stressExchange.publish (
            std::make_unique<TestSnapshot> (generation, stressDestructions));
    writerFinished.store (true);
    reader.join();
    stressExchange.reclaim();
    REQUIRE (! invalidGenerationSeen.load());
    {
        auto finalSnapshot = stressExchange.acquire();
        REQUIRE (finalSnapshot);
        REQUIRE (finalSnapshot->value == 10000);
    }
    stressExchange.clear();
    stressExchange.reclaim();
    REQUIRE (stressDestructions.load() == 10000);

    return 0;
}
