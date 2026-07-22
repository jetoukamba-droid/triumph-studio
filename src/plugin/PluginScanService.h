#pragma once

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include <juce_core/juce_core.h>

#include <vector>

namespace triumph
{
class PluginScanService final
{
public:
    struct Result
    {
        std::vector<juce::PluginDescription> descriptions;
        juce::String error;
        int exitCode = -1;
        bool timedOut = false;
        bool crashed = false;

        bool succeeded() const noexcept
        {
            return ! descriptions.empty() && error.isEmpty()
                   && ! timedOut && ! crashed && exitCode == 0;
        }
    };

    struct RegistryEntry
    {
        juce::String path;
        juce::String status;
        juce::String error;
        juce::StringArray descriptionXml;
        juce::String stableId;
        juce::String displayName;
        juce::String manufacturer;
        juce::String version;
        juce::String category;
        juce::String formatName;
        bool isInstrument = false;
        bool hasARAExtension = false;
        bool fileAvailable = false;
        int inputChannels = 0;
        int outputChannels = 0;
        juce::int64 lastScanMilliseconds = 0;
        juce::int64 lastSuccessfulScanMilliseconds = 0;
        juce::int64 fileSize = 0;
        juce::int64 fileModifiedMilliseconds = 0;
        int scanAttemptCount = 0;
        int failureCount = 0;
        int exitCode = -1;
    };

    struct RegistrySummary
    {
        int total = 0;
        int validated = 0;
        int blocked = 0;
        int missing = 0;
        int instruments = 0;
        int audioEffects = 0;
    };

    static std::vector<juce::File> helperExecutableCandidates();
    static juce::File helperExecutable();
    static juce::File helperInstallDirectory();
    static juce::Result copyHelperToDirectory (
        const juce::File& helper,
        const juce::File& targetDirectory);
    static juce::Result ensureHelperBesideApplication();
    static juce::String helperSearchDiagnostic();
    static Result scanVst3 (const juce::File& pluginFile,
                            const juce::File& helper,
                            int timeoutMilliseconds = 30000);
    static juce::Result updateRegistry (const juce::File& registryFile,
                                        const juce::File& pluginFile,
                                        const Result& result);
    static juce::Result clearBlockedEntries (
        const juce::File& registryFile);
    static juce::Result forgetRegistryEntry (
        const juce::File& registryFile,
        const juce::File& pluginFile);
    static std::vector<RegistryEntry> loadRegistry (
        const juce::File& registryFile);
    static RegistrySummary summarizeRegistry (
        const std::vector<RegistryEntry>& entries) noexcept;
    static juce::String describeRegistryEntry (
        const RegistryEntry& entry);
    static juce::String buildRegistryReport (
        const std::vector<RegistryEntry>& entries);
    static std::vector<juce::File> defaultVst3Directories();
    static std::vector<juce::File> discoverVst3Candidates (
        const std::vector<juce::File>& directories);
    static const RegistryEntry* findEntry (
        const std::vector<RegistryEntry>& entries,
        const juce::File& pluginFile) noexcept;
    static bool shouldScan (const juce::File& pluginFile,
                            const RegistryEntry* existing,
                            bool forceRetry = false) noexcept;
};
}
