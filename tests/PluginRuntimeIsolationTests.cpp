#include "TestSupport.h"

#include "core/PluginRack.h"
#include "core/PluginRuntimeIsolation.h"

#include <array>
#include <string>

int main()
{
    using namespace triumph;

    plugin_runtime::Configuration configuration;
    configuration.slotId = "vocal:eq";
    configuration.stablePluginId = "vendor.eq.v3";
    configuration.maximumRestartAttempts = 2;
    configuration.heartbeatTimeoutNanoseconds = 1000;
    REQUIRE (configuration.isValid());

    plugin_runtime::SharedBlockBridge<2, 16, 8, 8> bridge;
    std::array<float, 8> left { 1, 2, 3, 4, 5, 6, 7, 8 };
    std::array<float, 8> right { 8, 7, 6, 5, 4, 3, 2, 1 };
    const float* inputs[] { left.data(), right.data() };
    const plugin_runtime::ParameterEvent parameters[] {
        { 3, 2, 0.25f }, { 3, 6, 0.75f }
    };
    plugin_runtime::MidiEvent note;
    note.sampleOffset = 4;
    note.size = 3;
    note.bytes[0] = 0x90;
    note.bytes[1] = 60;
    note.bytes[2] = 100;
    const auto sequence = bridge.submit (inputs, 2, 2, 8,
                                         parameters, 2, &note, 1);
    REQUIRE (sequence == 1);
    auto* workerBlock = bridge.claimNextForWorker();
    REQUIRE (workerBlock != nullptr);
    REQUIRE (workerBlock->parameterEvents[1].sampleOffset == 6);
    REQUIRE (workerBlock->midiEvents[0].bytes[1] == 60);
    for (std::size_t sample = 0; sample < 8; ++sample)
    {
        workerBlock->output[sample] = workerBlock->input[sample] * 0.5f;
        workerBlock->output[16 + sample]
            = workerBlock->input[16 + sample] * 0.5f;
    }
    bridge.completeFromWorker (*workerBlock, plugin_runtime::Fault::none,
                               64, 24000);
    std::array<float, 8> outputLeft {};
    std::array<float, 8> outputRight {};
    float* outputs[] { outputLeft.data(), outputRight.data() };
    plugin_runtime::BlockResult result;
    REQUIRE (bridge.consumeLatest (outputs, 2, 8, result));
    REQUIRE (result.sequence == sequence);
    REQUIRE (result.reportedLatencySamples == 64);
    REQUIRE (outputLeft[3] == 2.0f);
    REQUIRE (outputRight[7] == 0.5f);
    REQUIRE (! bridge.consumeLatest (outputs, 2, 8, result));

    plugin_runtime::Watchdog watchdog (configuration);
    REQUIRE (watchdog.beginLaunch (100));
    watchdog.markRunning (200);
    watchdog.heartbeat (900);
    REQUIRE (! watchdog.poll (1500, true));
    REQUIRE (watchdog.poll (2001, true));
    REQUIRE (watchdog.getState() == plugin_runtime::WorkerState::stalled);
    REQUIRE (watchdog.getRestartAttempts() == 1);
    REQUIRE (! watchdog.beginLaunch (watchdog.getRestartAllowedAt() - 1));
    REQUIRE (watchdog.beginLaunch (watchdog.getRestartAllowedAt()));
    watchdog.markRunning (3000);
    REQUIRE (watchdog.poll (3100, false));
    REQUIRE (watchdog.getRestartAttempts() == 2);
    REQUIRE (watchdog.beginLaunch (watchdog.getRestartAllowedAt()));
    watchdog.markRunning (5000);
    REQUIRE (watchdog.poll (5100, false));
    REQUIRE (watchdog.getState() == plugin_runtime::WorkerState::disabled);

    const auto rack = plugin_rack::buildPlan ({
        { "synth", "keys", "vendor.synth", plugin_rack::Role::instrument,
          plugin_rack::Isolation::workerProcess, 0, 0, 2, 0, 64 },
        { "eq", "keys", "vendor.eq", plugin_rack::Role::insertEffect,
          plugin_rack::Isolation::workerProcess, 1, 2, 2, 0, 32 },
        { "limiter", "keys", "vendor.limiter",
          plugin_rack::Role::insertEffect,
          plugin_rack::Isolation::trustedInProcess, 2, 2, 2, 0, 128 }
    });
    REQUIRE (rack.valid);
    REQUIRE (rack.owners.size() == 1);
    REQUIRE (rack.owners.front().slots.size() == 3);
    REQUIRE (rack.owners.front().totalLatencySamples == 224);
    REQUIRE (rack.owners.front().requiresWorker);
    std::string slotId;
    std::string parameterId;
    const auto target = plugin_rack::parameterTargetId ("eq", "cutoff");
    REQUIRE (target == "plugin:eq:cutoff");
    REQUIRE (plugin_rack::parseParameterTargetId (
        target, slotId, parameterId));
    REQUIRE (slotId == "eq");
    REQUIRE (parameterId == "cutoff");

    const auto badRack = plugin_rack::buildPlan ({
        { "eq", "keys", "vendor.eq", plugin_rack::Role::insertEffect,
          plugin_rack::Isolation::workerProcess, 0, 2, 2 },
        { "synth", "keys", "vendor.synth", plugin_rack::Role::instrument,
          plugin_rack::Isolation::workerProcess, 1, 0, 2 }
    });
    REQUIRE (! badRack.valid);
    return 0;
}
