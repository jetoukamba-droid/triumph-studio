#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include <juce_core/juce_core.h>

#include <memory>

namespace
{
int writeResult (const juce::File& output,
                 const juce::File& plugin,
                 const juce::OwnedArray<juce::PluginDescription>& descriptions,
                 const juce::String& error)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("format", "TriumphPluginScanResult");
    object->setProperty ("formatVersion", 1);
    object->setProperty ("pluginFile", plugin.getFullPathName());
    object->setProperty ("error", error);
    juce::Array<juce::var> values;
    for (const auto* description : descriptions)
        if (description != nullptr)
            if (const auto xml = description->createXml())
                values.add (xml->toString());
    object->setProperty ("descriptions", juce::var (values));
    juce::TemporaryFile temporary (output);
    if (! temporary.getFile().replaceWithText (
            juce::JSON::toString (juce::var (object.release()), true))
        || ! temporary.overwriteTargetFileWithTemporary())
        return 3;
    return 0;
}
}

int main (int argc, char* argv[])
{
    juce::StringArray arguments;
    for (int index = 1; index < argc; ++index)
        arguments.add (juce::String::fromUTF8 (argv[index]));
    const auto scanIndex = arguments.indexOf ("--scan-vst3");
    const auto outputIndex = arguments.indexOf ("--output");
    if (scanIndex < 0 || outputIndex < 0
        || scanIndex + 1 >= arguments.size()
        || outputIndex + 1 >= arguments.size())
        return 1;

    const juce::File plugin (arguments[scanIndex + 1]);
    const juce::File output (arguments[outputIndex + 1]);
    juce::VST3PluginFormatHeadless format;
    juce::OwnedArray<juce::PluginDescription> descriptions;
    juce::String error;
    if (! plugin.exists())
        error = "The selected VST3 plug-in does not exist.";
    else if (! format.fileMightContainThisPluginType (
                 plugin.getFullPathName()))
        error = "The selected item is not recognized as a VST3 plug-in.";
    else
    {
        format.findAllTypesForFile (descriptions, plugin.getFullPathName());
        if (descriptions.isEmpty())
            error = "No loadable VST3 types were found in the selected plug-in.";
    }
    return writeResult (output, plugin, descriptions, error);
}
