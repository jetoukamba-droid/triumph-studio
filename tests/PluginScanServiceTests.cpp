#include "plugin/PluginScanService.h"
#include "TestSupport.h"

int main (int argc, char* argv[])
{
    using namespace triumph;

    REQUIRE (argc == 3);
    const juce::File failureHelper (juce::String::fromUTF8 (argv[1]));
    const juce::File scannerHelper (juce::String::fromUTF8 (argv[2]));
    REQUIRE (failureHelper.existsAsFile());
    REQUIRE (scannerHelper.existsAsFile());

    auto directory = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-plugin-registry-test", juce::String {}, false);
    REQUIRE (directory.createDirectory().wasOk());
    const auto registryFile = directory.getChildFile ("plugin-registry.json");
    const auto pluginFile = directory.getChildFile ("Not A Plugin.txt");
    REQUIRE (pluginFile.replaceWithText ("not a plug-in"));

    const auto helperCandidates =
        PluginScanService::helperExecutableCandidates();
    REQUIRE (! helperCandidates.empty());
    REQUIRE (PluginScanService::helperSearchDiagnostic()
                 .containsIgnoreCase ("TriumphPluginScanner"));

    const auto missingHelper = directory.getChildFile (
        "MissingTriumphPluginScanner.exe");
    const auto missingHelperResult = PluginScanService::scanVst3 (
        pluginFile, missingHelper, 1000);
    REQUIRE (! missingHelperResult.succeeded());
    REQUIRE (missingHelperResult.error.containsIgnoreCase (
        "crash-isolated plug-in scanner is missing"));
    REQUIRE (missingHelperResult.error.containsIgnoreCase (
        "TriumphPluginScanner"));
    REQUIRE (PluginScanService::helperSearchDiagnostic()
                 .containsIgnoreCase ("Scanner install folder"));

    const auto repairedAppFolder = directory.getChildFile (
        "repaired-app-folder");
    REQUIRE (PluginScanService::copyHelperToDirectory (
        scannerHelper, repairedAppFolder).wasOk());
#if JUCE_WINDOWS
    REQUIRE (repairedAppFolder.getChildFile (
        "TriumphPluginScanner.exe").existsAsFile());
    REQUIRE (repairedAppFolder.getChildFile (
        "Triumph Plugin Scanner.exe").existsAsFile());
#else
    REQUIRE (repairedAppFolder.getChildFile (
        "TriumphPluginScanner").existsAsFile());
    REQUIRE (repairedAppFolder.getChildFile (
        "Triumph Plugin Scanner").existsAsFile());
#endif

    const auto failedProcess = PluginScanService::scanVst3 (
        pluginFile, failureHelper, 5000);
    REQUIRE (failedProcess.crashed);
    REQUIRE (! failedProcess.timedOut);
    REQUIRE (failedProcess.exitCode != 0);
    REQUIRE (failedProcess.error.isNotEmpty());

    const auto timeoutPlugin = directory.getChildFile ("Timeout.txt");
    REQUIRE (timeoutPlugin.replaceWithText ("timeout sentinel"));
    const auto timedOutProcess = PluginScanService::scanVst3 (
        timeoutPlugin, failureHelper, 1000);
    REQUIRE (timedOutProcess.timedOut);
    REQUIRE (! timedOutProcess.succeeded());
    REQUIRE (timedOutProcess.error.containsIgnoreCase ("time limit"));

    const auto malformedPlugin = directory.getChildFile ("Malformed.txt");
    REQUIRE (malformedPlugin.replaceWithText ("malformed sentinel"));
    const auto malformedProcess = PluginScanService::scanVst3 (
        malformedPlugin, failureHelper, 5000);
    REQUIRE (malformedProcess.crashed);
    REQUIRE (malformedProcess.exitCode == 0);
    REQUIRE (malformedProcess.error.containsIgnoreCase ("invalid result"));

    const auto missingOutputPlugin = directory.getChildFile (
        "Missing Output.txt");
    REQUIRE (missingOutputPlugin.replaceWithText ("missing-output sentinel"));
    const auto missingOutputProcess = PluginScanService::scanVst3 (
        missingOutputPlugin, failureHelper, 5000);
    REQUIRE (missingOutputProcess.crashed);
    REQUIRE (missingOutputProcess.exitCode == 0);
    REQUIRE (missingOutputProcess.error.containsIgnoreCase (
        "without a result file"));

    const auto protocolFailure = PluginScanService::scanVst3 (
        pluginFile, scannerHelper, 5000);
    REQUIRE (! protocolFailure.crashed);
    REQUIRE (! protocolFailure.timedOut);
    REQUIRE (protocolFailure.exitCode == 0);
    REQUIRE (protocolFailure.descriptions.empty());
    REQUIRE (protocolFailure.error.isNotEmpty());

    PluginScanService::Result failedScan;
    failedScan.error = "The scanner process terminated unexpectedly.";
    failedScan.exitCode = 9;
    failedScan.crashed = true;
    REQUIRE (PluginScanService::updateRegistry (
        registryFile, pluginFile, failedScan).wasOk());

    auto entries = PluginScanService::loadRegistry (registryFile);
    REQUIRE (entries.size() == 1);
    REQUIRE (entries.front().path == pluginFile.getFullPathName());
    REQUIRE (entries.front().status == "blocked");
    REQUIRE (entries.front().error == failedScan.error);
    REQUIRE (entries.front().exitCode == 9);
    REQUIRE (entries.front().descriptionXml.isEmpty());
    REQUIRE (entries.front().fileAvailable);
    REQUIRE (entries.front().scanAttemptCount == 1);
    REQUIRE (entries.front().failureCount == 1);
    REQUIRE (entries.front().lastSuccessfulScanMilliseconds == 0);

    juce::PluginDescription description;
    description.name = "Registry Test Instrument";
    description.descriptiveName = description.name;
    description.pluginFormatName = "VST3";
    description.fileOrIdentifier = pluginFile.getFullPathName();
    description.manufacturerName = "Triumph Test";
    description.version = "1.0";
    description.category = "Instrument";
    description.uniqueId = 0x12345678;
    description.deprecatedUid = description.uniqueId;
    description.isInstrument = true;
    description.hasARAExtension = true;
    description.numInputChannels = 0;
    description.numOutputChannels = 2;

    PluginScanService::Result successfulScan;
    successfulScan.descriptions.push_back (description);
    successfulScan.exitCode = 0;
    REQUIRE (successfulScan.succeeded());
    REQUIRE (PluginScanService::updateRegistry (
        registryFile, pluginFile, successfulScan).wasOk());

    entries = PluginScanService::loadRegistry (registryFile);
    REQUIRE (entries.size() == 1);
    REQUIRE (entries.front().status == "validated");
    REQUIRE (entries.front().error.isEmpty());
    REQUIRE (entries.front().exitCode == 0);
    REQUIRE (entries.front().descriptionXml.size() == 1);
    REQUIRE (entries.front().stableId == description.createIdentifierString());
    REQUIRE (entries.front().displayName == description.name);
    REQUIRE (entries.front().manufacturer == description.manufacturerName);
    REQUIRE (entries.front().version == description.version);
    REQUIRE (entries.front().category == description.category);
    REQUIRE (entries.front().formatName == description.pluginFormatName);
    REQUIRE (entries.front().isInstrument);
    REQUIRE (entries.front().hasARAExtension);
    REQUIRE (entries.front().inputChannels == 0);
    REQUIRE (entries.front().outputChannels == 2);
    REQUIRE (entries.front().fileAvailable);
    REQUIRE (entries.front().scanAttemptCount == 2);
    REQUIRE (entries.front().failureCount == 0);
    REQUIRE (entries.front().lastSuccessfulScanMilliseconds > 0);
    REQUIRE (entries.front().fileSize == pluginFile.getSize());
    REQUIRE (entries.front().fileModifiedMilliseconds
             == pluginFile.getLastModificationTime().toMilliseconds());
    auto summary = PluginScanService::summarizeRegistry (entries);
    REQUIRE (summary.total == 1);
    REQUIRE (summary.validated == 1);
    REQUIRE (summary.blocked == 0);
    REQUIRE (summary.missing == 0);
    REQUIRE (summary.instruments == 1);
    REQUIRE (summary.audioEffects == 0);
    REQUIRE (PluginScanService::describeRegistryEntry (entries.front())
                 .contains ("Registry Test Instrument"));
    const auto validatedReport = PluginScanService::buildRegistryReport (
        entries);
    REQUIRE (validatedReport.contains ("Total: 1"));
    REQUIRE (validatedReport.contains ("Validated: 1"));
    REQUIRE (validatedReport.contains ("Instruments: 1"));

    const auto* registered = PluginScanService::findEntry (
        entries, pluginFile);
    REQUIRE (registered != nullptr);
    REQUIRE (! PluginScanService::shouldScan (
        pluginFile, registered, false));
    REQUIRE (PluginScanService::shouldScan (
        pluginFile, registered, true));
    REQUIRE (pluginFile.replaceWithText (
        "not a plug-in, but its fingerprint has changed"));
    REQUIRE (PluginScanService::shouldScan (
        pluginFile, registered, false));

    const auto xml = juce::parseXML (entries.front().descriptionXml[0]);
    juce::PluginDescription restored;
    REQUIRE (xml != nullptr);
    REQUIRE (restored.loadFromXml (*xml));
    REQUIRE (restored.name == description.name);
    REQUIRE (restored.isInstrument);

    const auto secondPlugin = directory.getChildFile ("Blocked Again.vst3");
    REQUIRE (PluginScanService::updateRegistry (
        registryFile, secondPlugin, failedScan).wasOk());
    entries = PluginScanService::loadRegistry (registryFile);
    REQUIRE (entries.size() == 2);
    REQUIRE (! PluginScanService::findEntry (
        entries, secondPlugin)->fileAvailable);
    summary = PluginScanService::summarizeRegistry (entries);
    REQUIRE (summary.total == 2);
    REQUIRE (summary.validated == 1);
    REQUIRE (summary.blocked == 1);
    REQUIRE (summary.missing == 1);
    const auto mixedReport = PluginScanService::buildRegistryReport (entries);
    REQUIRE (mixedReport.contains ("Blocked: 1"));
    REQUIRE (mixedReport.contains ("Missing files: 1"));
    REQUIRE (mixedReport.contains ("The scanner process terminated unexpectedly."));
    REQUIRE (PluginScanService::forgetRegistryEntry (
        registryFile, secondPlugin).wasOk());
    entries = PluginScanService::loadRegistry (registryFile);
    REQUIRE (entries.size() == 1);
    REQUIRE (PluginScanService::findEntry (entries, secondPlugin) == nullptr);
    REQUIRE (PluginScanService::updateRegistry (
        registryFile, secondPlugin, failedScan).wasOk());
    entries = PluginScanService::loadRegistry (registryFile);
    REQUIRE (entries.size() == 2);
    REQUIRE (PluginScanService::clearBlockedEntries (registryFile).wasOk());
    entries = PluginScanService::loadRegistry (registryFile);
    REQUIRE (entries.size() == 1);
    REQUIRE (entries.front().path == pluginFile.getFullPathName());
    REQUIRE (entries.front().status == "validated");

    const auto discoveryRoot = directory.getChildFile ("discovery");
    const auto firstVst3 = discoveryRoot.getChildFile ("One.vst3");
    const auto secondVst3 = discoveryRoot.getChildFile (
        "Vendor").getChildFile ("Two.vst3");
    REQUIRE (firstVst3.createDirectory().wasOk());
    REQUIRE (secondVst3.createDirectory().wasOk());
    REQUIRE (discoveryRoot.getChildFile ("Readme.txt").replaceWithText (
        "not a plug-in"));
    const auto candidates = PluginScanService::discoverVst3Candidates (
        { discoveryRoot, discoveryRoot });
    REQUIRE (candidates.size() == 2);
    REQUIRE (candidates[0].getFileName() == "One.vst3");
    REQUIRE (candidates[1].getFileName() == "Two.vst3");

    const auto legacyRegistry = directory.getChildFile (
        "plugin-registry-v1.json");
    auto legacyEntry = std::make_unique<juce::DynamicObject>();
    legacyEntry->setProperty ("path", firstVst3.getFullPathName());
    legacyEntry->setProperty ("status", "blocked");
    legacyEntry->setProperty ("error", "legacy failure");
    legacyEntry->setProperty ("lastScanMilliseconds", 1234);
    legacyEntry->setProperty ("exitCode", 7);
    legacyEntry->setProperty (
        "descriptions", juce::var (juce::Array<juce::var> {}));
    juce::Array<juce::var> legacyEntries;
    legacyEntries.add (juce::var (legacyEntry.release()));
    auto legacyRoot = std::make_unique<juce::DynamicObject>();
    legacyRoot->setProperty ("format", "TriumphPluginRegistry");
    legacyRoot->setProperty ("formatVersion", 1);
    legacyRoot->setProperty ("entries", juce::var (legacyEntries));
    REQUIRE (legacyRegistry.replaceWithText (
        juce::JSON::toString (juce::var (legacyRoot.release()), true)));
    const auto legacyLoaded = PluginScanService::loadRegistry (
        legacyRegistry);
    REQUIRE (legacyLoaded.size() == 1);
    REQUIRE (legacyLoaded.front().path == firstVst3.getFullPathName());
    REQUIRE (legacyLoaded.front().status == "blocked");
    REQUIRE (legacyLoaded.front().stableId.isEmpty());
    REQUIRE (legacyLoaded.front().scanAttemptCount == 0);
    REQUIRE (legacyLoaded.front().failureCount == 0);

    REQUIRE (directory.deleteRecursively());
    return 0;
}
