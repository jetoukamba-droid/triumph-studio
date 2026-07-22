#include "PluginPresetStore.h"

#include <iomanip>
#include <sstream>

namespace triumph
{
namespace
{
constexpr const char* presetFormat = "TriumphPluginPreset";
constexpr int presetFormatVersion = 1;
}

juce::Result PluginPresetStore::save (
    const juce::File& destination,
    juce::String stablePluginId,
    juce::String pluginName,
    const juce::String& stateBase64)
{
    if (destination == juce::File())
        return juce::Result::fail ("Choose a preset destination.");
    stablePluginId = stablePluginId.trim();
    if (stablePluginId.isEmpty())
        return juce::Result::fail (
            "The loaded plug-in has no stable identity.");
    juce::MemoryBlock state;
    if (stateBase64.isEmpty() || ! state.fromBase64Encoding (stateBase64))
        return juce::Result::fail (
            "The plug-in did not provide a valid state payload.");

    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("format", presetFormat);
    object->setProperty ("formatVersion", presetFormatVersion);
    object->setProperty ("stablePluginId", stablePluginId);
    object->setProperty ("pluginName", pluginName);
    object->setProperty ("stateBase64", stateBase64);
    object->setProperty ("stateChecksum", checksum (state));
    object->setProperty ("createdMilliseconds",
                         juce::Time::currentTimeMillis());

    if (destination.getParentDirectory().createDirectory().failed())
        return juce::Result::fail (
            "The preset folder could not be created.");
    juce::TemporaryFile temporary (destination);
    if (! temporary.getFile().replaceWithText (
            juce::JSON::toString (juce::var (object.release()), true)))
        return juce::Result::fail (
            "The preset temporary file could not be written.");
    if (! temporary.overwriteTargetFileWithTemporary())
        return juce::Result::fail (
            "The preset could not be replaced safely.");
    return juce::Result::ok();
}

juce::Result PluginPresetStore::load (
    const juce::File& source,
    const juce::String& expectedStablePluginId,
    Preset& preset)
{
    preset = {};
    if (! source.existsAsFile())
        return juce::Result::fail ("The selected preset does not exist.");
    const auto parsed = juce::JSON::parse (source.loadFileAsString());
    const auto* object = parsed.getDynamicObject();
    if (object == nullptr
        || object->getProperty ("format").toString() != presetFormat
        || static_cast<int> (object->getProperty ("formatVersion"))
               != presetFormatVersion)
        return juce::Result::fail (
            "The selected file is not a supported Triumph plug-in preset.");

    Preset loaded;
    loaded.stablePluginId = object->getProperty (
        "stablePluginId").toString();
    loaded.pluginName = object->getProperty ("pluginName").toString();
    loaded.stateBase64 = object->getProperty ("stateBase64").toString();
    loaded.stateChecksum = object->getProperty (
        "stateChecksum").toString();
    loaded.createdMilliseconds = static_cast<juce::int64> (
        object->getProperty ("createdMilliseconds"));
    if (loaded.stablePluginId.isEmpty())
        return juce::Result::fail (
            "The preset is missing its plug-in identity.");
    if (expectedStablePluginId.isNotEmpty()
        && loaded.stablePluginId != expectedStablePluginId)
        return juce::Result::fail (
            "This preset belongs to a different plug-in.");

    juce::MemoryBlock state;
    if (loaded.stateBase64.isEmpty()
        || ! state.fromBase64Encoding (loaded.stateBase64))
        return juce::Result::fail (
            "The preset state payload is damaged.");
    if (loaded.stateChecksum.isEmpty()
        || checksum (state) != loaded.stateChecksum)
        return juce::Result::fail (
            "The preset state checksum does not match.");

    preset = std::move (loaded);
    return juce::Result::ok();
}

juce::String PluginPresetStore::checksum (
    const juce::MemoryBlock& state) noexcept
{
    auto value = UINT64_C (14695981039346656037);
    const auto* bytes = static_cast<const unsigned char*> (state.getData());
    for (std::size_t index = 0; index < state.getSize(); ++index)
    {
        value ^= bytes[index];
        value *= UINT64_C (1099511628211);
    }
    std::ostringstream stream;
    stream << std::hex << std::setw (16) << std::setfill ('0') << value;
    return stream.str();
}
}
