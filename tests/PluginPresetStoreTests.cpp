#include "plugin/PluginPresetStore.h"
#include "TestSupport.h"

int main()
{
    using namespace triumph;

    const auto directory = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-plugin-preset-test", {}, false);
    REQUIRE (directory.createDirectory().wasOk());
    const auto presetFile = directory.getChildFile (
        "Bright Lead.triumphpreset");

    const char stateBytes[] { 0, 1, 2, 3, 42, 99, 127 };
    const juce::MemoryBlock state (stateBytes, sizeof (stateBytes));
    const auto encoded = state.toBase64Encoding();
    REQUIRE (PluginPresetStore::save (
        presetFile, "VST3:stable-test-id", "Test Instrument", encoded).wasOk());

    PluginPresetStore::Preset loaded;
    REQUIRE (PluginPresetStore::load (
        presetFile, "VST3:stable-test-id", loaded).wasOk());
    REQUIRE (loaded.stablePluginId == "VST3:stable-test-id");
    REQUIRE (loaded.pluginName == "Test Instrument");
    REQUIRE (loaded.stateBase64 == encoded);
    REQUIRE (loaded.stateChecksum.isNotEmpty());
    REQUIRE (loaded.createdMilliseconds > 0);

    PluginPresetStore::Preset wrongPlugin;
    REQUIRE (PluginPresetStore::load (
        presetFile, "VST3:different-id", wrongPlugin).failed());
    REQUIRE (wrongPlugin.stateBase64.isEmpty());

    auto parsed = juce::JSON::parse (presetFile.loadFileAsString());
    auto* object = parsed.getDynamicObject();
    REQUIRE (object != nullptr);
    const juce::MemoryBlock differentState ("different", 9);
    object->setProperty ("stateBase64", differentState.toBase64Encoding());
    REQUIRE (presetFile.replaceWithText (juce::JSON::toString (parsed, true)));
    PluginPresetStore::Preset corrupt;
    const auto corruptResult = PluginPresetStore::load (
        presetFile, "VST3:stable-test-id", corrupt);
    REQUIRE (corruptResult.failed());
    REQUIRE (corruptResult.getErrorMessage().containsIgnoreCase ("checksum"));

    REQUIRE (PluginPresetStore::save (
        presetFile, "VST3:stable-test-id", "Replacement", encoded).wasOk());
    REQUIRE (PluginPresetStore::load (
        presetFile, "VST3:stable-test-id", loaded).wasOk());
    REQUIRE (loaded.pluginName == "Replacement");
    REQUIRE (directory.deleteRecursively());
    return 0;
}
