#pragma once

#include <juce_core/juce_core.h>

namespace triumph
{
class PluginPresetStore final
{
public:
    struct Preset
    {
        juce::String stablePluginId;
        juce::String pluginName;
        juce::String stateBase64;
        juce::String stateChecksum;
        juce::int64 createdMilliseconds = 0;
    };

    static juce::Result save (const juce::File& destination,
                              juce::String stablePluginId,
                              juce::String pluginName,
                              const juce::String& stateBase64);
    static juce::Result load (const juce::File& source,
                              const juce::String& expectedStablePluginId,
                              Preset& preset);

private:
    static juce::String checksum (const juce::MemoryBlock& state) noexcept;
};
}
