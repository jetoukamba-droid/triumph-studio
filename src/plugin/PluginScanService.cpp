#include "PluginScanService.h"

#include <algorithm>
#include <iterator>

namespace triumph
{
namespace
{
constexpr const char* protocolName = "TriumphPluginScanResult";

juce::var registryEntryToVar (const PluginScanService::RegistryEntry& entry)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("path", entry.path);
    object->setProperty ("status", entry.status);
    object->setProperty ("error", entry.error);
    object->setProperty ("stableId", entry.stableId);
    object->setProperty ("displayName", entry.displayName);
    object->setProperty ("manufacturer", entry.manufacturer);
    object->setProperty ("version", entry.version);
    object->setProperty ("category", entry.category);
    object->setProperty ("formatName", entry.formatName);
    object->setProperty ("isInstrument", entry.isInstrument);
    object->setProperty ("hasARAExtension", entry.hasARAExtension);
    object->setProperty ("fileAvailable", entry.fileAvailable);
    object->setProperty ("inputChannels", entry.inputChannels);
    object->setProperty ("outputChannels", entry.outputChannels);
    object->setProperty ("lastScanMilliseconds", entry.lastScanMilliseconds);
    object->setProperty ("lastSuccessfulScanMilliseconds",
                         entry.lastSuccessfulScanMilliseconds);
    object->setProperty ("fileSize", entry.fileSize);
    object->setProperty ("fileModifiedMilliseconds",
                         entry.fileModifiedMilliseconds);
    object->setProperty ("scanAttemptCount", entry.scanAttemptCount);
    object->setProperty ("failureCount", entry.failureCount);
    object->setProperty ("exitCode", entry.exitCode);
    juce::Array<juce::var> descriptions;
    for (const auto& xml : entry.descriptionXml)
        descriptions.add (xml);
    object->setProperty ("descriptions", juce::var (descriptions));
    return juce::var (object.release());
}

juce::Result writeRegistry (
    const juce::File& registryFile,
    std::vector<PluginScanService::RegistryEntry> entries)
{
    std::sort (entries.begin(), entries.end(), [] (const auto& left,
                                                   const auto& right)
    {
        return left.path.compareNatural (right.path) < 0;
    });
    auto root = std::make_unique<juce::DynamicObject>();
    root->setProperty ("format", "TriumphPluginRegistry");
    root->setProperty ("formatVersion", 2);
    juce::Array<juce::var> values;
    for (const auto& entry : entries)
        values.add (registryEntryToVar (entry));
    root->setProperty ("entries", juce::var (values));

    if (registryFile.getParentDirectory().createDirectory().failed())
        return juce::Result::fail (
            "The plug-in registry folder could not be created.");
    juce::TemporaryFile temporary (registryFile);
    if (! temporary.getFile().replaceWithText (
            juce::JSON::toString (juce::var (root.release()), true)))
        return juce::Result::fail ("The plug-in registry could not be written.");
    if (! temporary.overwriteTargetFileWithTemporary())
        return juce::Result::fail (
            "The plug-in registry could not be replaced safely.");
    return juce::Result::ok();
}

bool samePath (const juce::String& left, const juce::String& right) noexcept
{
#if JUCE_WINDOWS
    return left.equalsIgnoreCase (right);
#else
    return left == right;
#endif
}

void discoverDirectory (const juce::File& directory,
                        std::vector<juce::File>& result,
                        int depth)
{
    if (! directory.isDirectory() || depth > 16)
        return;
    for (const auto& child : directory.findChildFiles (
             juce::File::findFilesAndDirectories, false))
    {
        if (child.hasFileExtension ("vst3"))
            result.push_back (child);
        else if (child.isDirectory())
            discoverDirectory (child, result, depth + 1);
    }
}

void addUniqueCandidate (std::vector<juce::File>& candidates,
                         const juce::File& candidate)
{
    const auto path = candidate.getFullPathName();
    if (std::none_of (candidates.begin(), candidates.end(),
                      [&path] (const auto& existing)
    {
        return samePath (existing.getFullPathName(), path);
    }))
        candidates.push_back (candidate);
}

juce::StringArray helperExecutableNames()
{
    juce::StringArray names;
#if JUCE_WINDOWS
    names.add ("TriumphPluginScanner.exe");
    names.add ("Triumph Plugin Scanner.exe");
#else
    names.add ("TriumphPluginScanner");
    names.add ("Triumph Plugin Scanner");
#endif
    return names;
}
}

std::vector<juce::File> PluginScanService::helperExecutableCandidates()
{
    std::vector<juce::File> result;
    const auto application = juce::File::getSpecialLocation (
        juce::File::currentExecutableFile);
    const auto applicationDirectory = application.getParentDirectory();
    const auto names = helperExecutableNames();

    for (const auto& name : names)
        addUniqueCandidate (result, applicationDirectory.getChildFile (name));

    const auto configurationDirectoryName = applicationDirectory.getFileName();
    const auto buildDirectory = applicationDirectory
        .getParentDirectory().getParentDirectory();
    if (buildDirectory.isDirectory())
        for (const auto& name : names)
            addUniqueCandidate (
                result,
                buildDirectory.getChildFile ("TriumphPluginScanner_artifacts")
                    .getChildFile (configurationDirectoryName)
                    .getChildFile (name));

    return result;
}

juce::File PluginScanService::helperExecutable()
{
    const auto candidates = helperExecutableCandidates();
    for (const auto& candidate : candidates)
        if (candidate.existsAsFile())
            return candidate;
    return candidates.empty() ? juce::File() : candidates.front();
}

juce::File PluginScanService::helperInstallDirectory()
{
    return juce::File::getSpecialLocation (
        juce::File::currentExecutableFile).getParentDirectory();
}

juce::Result PluginScanService::copyHelperToDirectory (
    const juce::File& helper,
    const juce::File& targetDirectory)
{
    if (! helper.existsAsFile())
        return juce::Result::fail (
            "The crash-isolated plug-in scanner was not found at "
            + helper.getFullPathName());
    if (targetDirectory.createDirectory().failed()
        || ! targetDirectory.isDirectory())
        return juce::Result::fail (
            "The scanner install folder could not be prepared: "
            + targetDirectory.getFullPathName());

    for (const auto& name : helperExecutableNames())
    {
        const auto target = targetDirectory.getChildFile (name);
        if (samePath (helper.getFullPathName(), target.getFullPathName()))
            continue;
        if (! helper.copyFileTo (target))
            return juce::Result::fail (
                "The scanner helper could not be copied to "
                + target.getFullPathName());
    }
    return juce::Result::ok();
}

juce::Result PluginScanService::ensureHelperBesideApplication()
{
    const auto helper = helperExecutable();
    const auto installDirectory = helperInstallDirectory();
    const auto result = copyHelperToDirectory (helper, installDirectory);
    if (result.failed())
        return result;

    for (const auto& name : helperExecutableNames())
        if (! installDirectory.getChildFile (name).existsAsFile())
            return juce::Result::fail (
                "The scanner helper was not installed beside the app as "
                + name);

    return juce::Result::ok();
}

juce::String PluginScanService::helperSearchDiagnostic()
{
    juce::StringArray lines;
    lines.add ("Scanner install folder: "
               + helperInstallDirectory().getFullPathName());
    lines.add ("Triumph looked for the crash-isolated scanner here:");
    for (const auto& candidate : helperExecutableCandidates())
        lines.add (juce::String ("- ") + candidate.getFullPathName()
                   + (candidate.existsAsFile() ? " (found)" : ""));
    return lines.joinIntoString ("\n");
}

PluginScanService::Result PluginScanService::scanVst3 (
    const juce::File& pluginFile,
    const juce::File& helper,
    int timeoutMilliseconds)
{
    Result result;
    if (! pluginFile.exists())
    {
        result.error = "The selected VST3 plug-in does not exist.";
        return result;
    }
    if (! helper.existsAsFile())
    {
        result.error = "The crash-isolated plug-in scanner is missing.\n\n"
                       + helperSearchDiagnostic();
        return result;
    }

    const auto output = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getChildFile (
            "triumph-plugin-scan-" + juce::Uuid().toString() + ".json");
    juce::StringArray arguments;
    arguments.add (helper.getFullPathName());
    arguments.add ("--scan-vst3");
    arguments.add (pluginFile.getFullPathName());
    arguments.add ("--output");
    arguments.add (output.getFullPathName());

    juce::ChildProcess child;
    if (! child.start (arguments, 0))
    {
        result.error = "The crash-isolated plug-in scanner could not be started.";
        return result;
    }
    if (! child.waitForProcessToFinish (juce::jmax (1000, timeoutMilliseconds)))
    {
        child.kill();
        result.timedOut = true;
        result.error = "The VST3 scan exceeded its time limit and was terminated.";
        output.deleteFile();
        return result;
    }

    result.exitCode = static_cast<int> (child.getExitCode());
    if (result.exitCode != 0)
    {
        result.crashed = true;
        result.error = "The VST3 scanner exited abnormally (code "
                       + juce::String (result.exitCode) + ").";
    }
    if (! output.existsAsFile())
    {
        if (result.error.isEmpty())
        {
            result.crashed = true;
            result.error = "The VST3 scanner exited without a result file.";
        }
        return result;
    }

    const auto parsed = juce::JSON::parse (output.loadFileAsString());
    output.deleteFile();
    const auto* object = parsed.getDynamicObject();
    if (object == nullptr
        || object->getProperty ("format").toString() != protocolName
        || static_cast<int> (object->getProperty ("formatVersion")) != 1)
    {
        result.crashed = true;
        result.error = "The VST3 scanner returned an invalid result.";
        return result;
    }

    const auto workerError = object->getProperty ("error").toString();
    if (result.error.isEmpty() && workerError.isNotEmpty())
        result.error = workerError;
    if (const auto* descriptions = object->getProperty (
            "descriptions").getArray())
        for (const auto& value : *descriptions)
        {
            const auto xml = juce::parseXML (value.toString());
            juce::PluginDescription description;
            if (xml != nullptr && description.loadFromXml (*xml))
                result.descriptions.push_back (std::move (description));
        }
    if (result.descriptions.empty() && result.error.isEmpty())
        result.error = "No loadable VST3 types were found in the selected plug-in.";
    return result;
}

std::vector<PluginScanService::RegistryEntry>
PluginScanService::loadRegistry (const juce::File& registryFile)
{
    std::vector<RegistryEntry> result;
    if (! registryFile.existsAsFile())
        return result;
    const auto parsed = juce::JSON::parse (registryFile.loadFileAsString());
    const auto* root = parsed.getDynamicObject();
    if (root == nullptr
        || root->getProperty ("format").toString() != "TriumphPluginRegistry"
        || (static_cast<int> (root->getProperty ("formatVersion")) != 1
            && static_cast<int> (root->getProperty ("formatVersion")) != 2))
        return result;
    if (const auto* entries = root->getProperty ("entries").getArray())
        for (const auto& value : *entries)
        {
            const auto* object = value.getDynamicObject();
            if (object == nullptr)
                continue;
            RegistryEntry entry;
            entry.path = object->getProperty ("path").toString();
            entry.status = object->getProperty ("status").toString();
            entry.error = object->getProperty ("error").toString();
            entry.stableId = object->getProperty ("stableId").toString();
            entry.displayName = object->getProperty ("displayName").toString();
            entry.manufacturer = object->getProperty (
                "manufacturer").toString();
            entry.version = object->getProperty ("version").toString();
            entry.category = object->getProperty ("category").toString();
            entry.formatName = object->getProperty ("formatName").toString();
            entry.isInstrument = static_cast<bool> (
                object->getProperty ("isInstrument"));
            entry.hasARAExtension = static_cast<bool> (
                object->getProperty ("hasARAExtension"));
            entry.fileAvailable = juce::File (entry.path).exists();
            entry.inputChannels = juce::jmax (0, static_cast<int> (
                object->getProperty ("inputChannels")));
            entry.outputChannels = juce::jmax (0, static_cast<int> (
                object->getProperty ("outputChannels")));
            entry.lastScanMilliseconds = static_cast<juce::int64> (
                object->getProperty ("lastScanMilliseconds"));
            entry.lastSuccessfulScanMilliseconds = static_cast<juce::int64> (
                object->getProperty ("lastSuccessfulScanMilliseconds"));
            entry.fileSize = static_cast<juce::int64> (
                object->getProperty ("fileSize"));
            entry.fileModifiedMilliseconds = static_cast<juce::int64> (
                object->getProperty ("fileModifiedMilliseconds"));
            entry.scanAttemptCount = juce::jmax (0, static_cast<int> (
                object->getProperty ("scanAttemptCount")));
            entry.failureCount = juce::jmax (0, static_cast<int> (
                object->getProperty ("failureCount")));
            entry.exitCode = static_cast<int> (object->getProperty ("exitCode"));
            if (const auto* descriptions = object->getProperty (
                    "descriptions").getArray())
                for (const auto& description : *descriptions)
                    entry.descriptionXml.add (description.toString());
            if (entry.path.isNotEmpty())
                result.push_back (std::move (entry));
        }
    return result;
}

juce::Result PluginScanService::updateRegistry (
    const juce::File& registryFile,
    const juce::File& pluginFile,
    const Result& result)
{
    auto entries = loadRegistry (registryFile);
    const auto path = pluginFile.getFullPathName();
    auto match = std::find_if (entries.begin(), entries.end(),
                               [&path] (const auto& entry)
    {
        return samePath (entry.path, path);
    });
    if (match == entries.end())
    {
        entries.push_back ({});
        match = std::prev (entries.end());
    }
    match->path = path;
    match->fileAvailable = pluginFile.exists();
    match->status = result.succeeded() ? "validated" : "blocked";
    match->error = result.error;
    match->exitCode = result.exitCode;
    match->lastScanMilliseconds = juce::Time::currentTimeMillis();
    match->fileSize = pluginFile.getSize();
    match->fileModifiedMilliseconds =
        pluginFile.getLastModificationTime().toMilliseconds();
    ++match->scanAttemptCount;
    if (! result.succeeded())
        ++match->failureCount;
    else
    {
        match->failureCount = 0;
        match->lastSuccessfulScanMilliseconds = match->lastScanMilliseconds;
    }
    match->descriptionXml.clear();
    for (const auto& description : result.descriptions)
        if (const auto xml = description.createXml())
            match->descriptionXml.add (xml->toString());

    if (result.succeeded())
    {
        const auto preferred = std::find_if (
            result.descriptions.begin(), result.descriptions.end(),
            [] (const auto& description) { return description.isInstrument; });
        const auto& description = preferred != result.descriptions.end()
                                      ? *preferred : result.descriptions.front();
        match->stableId = description.createIdentifierString();
        match->displayName = description.name;
        match->manufacturer = description.manufacturerName;
        match->version = description.version;
        match->category = description.category;
        match->formatName = description.pluginFormatName;
        match->isInstrument = description.isInstrument;
        match->hasARAExtension = description.hasARAExtension;
        match->inputChannels = juce::jmax (0, description.numInputChannels);
        match->outputChannels = juce::jmax (0, description.numOutputChannels);
    }

    return writeRegistry (registryFile, std::move (entries));
}

juce::Result PluginScanService::clearBlockedEntries (
    const juce::File& registryFile)
{
    auto entries = loadRegistry (registryFile);
    entries.erase (std::remove_if (
        entries.begin(), entries.end(), [] (const auto& entry)
        {
            return entry.status == "blocked";
        }), entries.end());
    return writeRegistry (registryFile, std::move (entries));
}

juce::Result PluginScanService::forgetRegistryEntry (
    const juce::File& registryFile,
    const juce::File& pluginFile)
{
    auto entries = loadRegistry (registryFile);
    const auto path = pluginFile.getFullPathName();
    const auto originalSize = entries.size();
    entries.erase (std::remove_if (
        entries.begin(), entries.end(), [&path] (const auto& entry)
        {
            return samePath (entry.path, path);
        }), entries.end());
    if (entries.size() == originalSize)
        return juce::Result::fail (
            "The selected plug-in registry record was not found.");
    return writeRegistry (registryFile, std::move (entries));
}

PluginScanService::RegistrySummary PluginScanService::summarizeRegistry (
    const std::vector<RegistryEntry>& entries) noexcept
{
    RegistrySummary summary;
    summary.total = static_cast<int> (entries.size());
    for (const auto& entry : entries)
    {
        if (! entry.fileAvailable)
            ++summary.missing;
        if (entry.status == "validated")
            ++summary.validated;
        else if (entry.status == "blocked")
            ++summary.blocked;
        if (entry.status == "validated")
        {
            if (entry.isInstrument)
                ++summary.instruments;
            else
                ++summary.audioEffects;
        }
    }
    return summary;
}

juce::String PluginScanService::describeRegistryEntry (
    const RegistryEntry& entry)
{
    auto displayName = entry.displayName.isNotEmpty()
        ? entry.displayName : juce::File (entry.path).getFileName();
    if (entry.version.isNotEmpty())
        displayName += " " + entry.version;

    juce::StringArray details;
    details.add (entry.status == "validated" ? "validated"
                 : entry.status == "blocked" ? "blocked"
                                             : "unknown");
    if (! entry.fileAvailable)
        details.add ("missing file");
    if (entry.manufacturer.isNotEmpty())
        details.add (entry.manufacturer);
    if (entry.formatName.isNotEmpty())
        details.add (entry.formatName);
    if (entry.isInstrument)
        details.add ("instrument");
    else if (entry.outputChannels > 0 || entry.inputChannels > 0)
        details.add ("audio effect");
    if (entry.scanAttemptCount > 0)
        details.add (juce::String (entry.scanAttemptCount) + " scans");
    if (entry.failureCount > 0)
        details.add (juce::String (entry.failureCount) + " failures");

    auto label = displayName + " [" + details.joinIntoString (", ") + "]";
    if (entry.error.isNotEmpty())
        label += " - " + entry.error.upToFirstOccurrenceOf (
            "\n", false, false);
    return label;
}

juce::String PluginScanService::buildRegistryReport (
    const std::vector<RegistryEntry>& entries)
{
    const auto summary = summarizeRegistry (entries);
    juce::StringArray lines;
    lines.add ("Triumph Studio Plug-in Registry");
    lines.add ("Total: " + juce::String (summary.total));
    lines.add ("Validated: " + juce::String (summary.validated));
    lines.add ("Blocked: " + juce::String (summary.blocked));
    lines.add ("Missing files: " + juce::String (summary.missing));
    lines.add ("Instruments: " + juce::String (summary.instruments));
    lines.add ("Audio effects: " + juce::String (summary.audioEffects));
    lines.add (juce::String());

    for (const auto& entry : entries)
    {
        lines.add (describeRegistryEntry (entry));
        lines.add ("  Path: " + entry.path);
        if (entry.stableId.isNotEmpty())
            lines.add ("  Stable ID: " + entry.stableId);
        if (entry.lastScanMilliseconds > 0)
            lines.add ("  Last scan: "
                       + juce::Time (entry.lastScanMilliseconds)
                            .toISO8601 (true));
        if (entry.lastSuccessfulScanMilliseconds > 0)
            lines.add ("  Last success: "
                       + juce::Time (
                             entry.lastSuccessfulScanMilliseconds)
                            .toISO8601 (true));
        if (entry.error.isNotEmpty())
            lines.add ("  Error: " + entry.error.replace ("\n", " / "));
    }
    return lines.joinIntoString ("\n");
}

std::vector<juce::File> PluginScanService::defaultVst3Directories()
{
    std::vector<juce::File> result;
#if JUCE_WINDOWS
    const auto commonFiles = juce::SystemStats::getEnvironmentVariable (
        "CommonProgramFiles", "C:\\Program Files\\Common Files");
    result.push_back (juce::File (commonFiles).getChildFile ("VST3"));
    const auto localAppData = juce::SystemStats::getEnvironmentVariable (
        "LOCALAPPDATA", {});
    if (localAppData.isNotEmpty())
        result.push_back (juce::File (localAppData)
            .getChildFile ("Programs").getChildFile ("Common").getChildFile (
                "VST3"));
#elif JUCE_MAC
    result.push_back (juce::File ("/Library/Audio/Plug-Ins/VST3"));
    result.push_back (juce::File::getSpecialLocation (
        juce::File::userHomeDirectory).getChildFile (
            "Library/Audio/Plug-Ins/VST3"));
#else
    result.push_back (juce::File ("/usr/lib/vst3"));
    result.push_back (juce::File::getSpecialLocation (
        juce::File::userHomeDirectory).getChildFile (".vst3"));
#endif
    result.erase (std::remove_if (result.begin(), result.end(),
                                  [] (const auto& directory)
    {
        return ! directory.isDirectory();
    }), result.end());
    return result;
}

std::vector<juce::File> PluginScanService::discoverVst3Candidates (
    const std::vector<juce::File>& directories)
{
    std::vector<juce::File> result;
    for (const auto& directory : directories)
        discoverDirectory (directory, result, 0);
    std::sort (result.begin(), result.end(), [] (const auto& left,
                                                 const auto& right)
    {
        return left.getFullPathName().compareNatural (
            right.getFullPathName()) < 0;
    });
    result.erase (std::unique (result.begin(), result.end(),
                              [] (const auto& left, const auto& right)
    {
        return samePath (left.getFullPathName(), right.getFullPathName());
    }), result.end());
    return result;
}

const PluginScanService::RegistryEntry* PluginScanService::findEntry (
    const std::vector<RegistryEntry>& entries,
    const juce::File& pluginFile) noexcept
{
    const auto path = pluginFile.getFullPathName();
    const auto match = std::find_if (
        entries.begin(), entries.end(), [&path] (const auto& entry)
    {
        return samePath (entry.path, path);
    });
    return match != entries.end() ? &*match : nullptr;
}

bool PluginScanService::shouldScan (const juce::File& pluginFile,
                                    const RegistryEntry* existing,
                                    bool forceRetry) noexcept
{
    if (forceRetry || existing == nullptr)
        return true;
    if (! pluginFile.exists())
        return false;
    const auto sameFingerprint = existing->fileSize == pluginFile.getSize()
        && existing->fileModifiedMilliseconds
               == pluginFile.getLastModificationTime().toMilliseconds();
    return ! sameFingerprint;
}
}
