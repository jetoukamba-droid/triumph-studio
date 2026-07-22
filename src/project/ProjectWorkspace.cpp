#include "ProjectWorkspace.h"

#include "ProjectDocument.h"
#include "audio/RecordingJournal.h"

#include <algorithm>

namespace triumph
{
namespace
{
constexpr int maximumRecentProjects = 12;
constexpr int maximumRollingBackups = 20;
const juce::String recentProjectsKey { "recentProjects" };
const juce::String audioDeviceStateKey { "audioDeviceState" };
}

ProjectWorkspace::ProjectWorkspace()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "Triumph Studio";
    options.filenameSuffix = "settings";
    options.folderName = "Triumph Studio";
    options.osxLibrarySubFolder = "Application Support";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    properties.setStorageParameters (options);

    if (auto* settings = properties.getUserSettings())
        recentProjects.restoreFromString (settings->getValue (recentProjectsKey));

    recentProjects.setMaxNumberOfItems (maximumRecentProjects);
}

ProjectWorkspace::~ProjectWorkspace()
{
    saveRecentProjects();
    properties.saveIfNeeded();
}

void ProjectWorkspace::addRecentProject (const juce::File& projectFile)
{
    if (projectFile.existsAsFile())
    {
        recentProjects.addFile (projectFile);
        saveRecentProjects();
    }
}

int ProjectWorkspace::getRecentProjectCount() const
{
    return recentProjects.getNumFiles();
}

juce::File ProjectWorkspace::getRecentProject (int index) const
{
    return recentProjects.getFile (index);
}

void ProjectWorkspace::clearRecentProjects()
{
    recentProjects.clear();
    saveRecentProjects();
}

juce::File ProjectWorkspace::getRecoveryFile (const ProjectModel& project) const
{
    return getRecoveryDirectory().getChildFile (project.getProjectId() + ".autosave.triumph");
}

juce::Result ProjectWorkspace::writeRecoveryFile (const ProjectModel& project) const
{
    const auto directoryResult = getRecoveryDirectory().createDirectory();
    if (directoryResult.failed())
        return juce::Result::fail ("The recovery folder could not be created.");

    return ProjectDocument::save (getRecoveryFile (project), project);
}

void ProjectWorkspace::deleteRecoveryFile (const ProjectModel& project) const
{
    const auto recoveryFile = getRecoveryFile (project);
    if (recoveryFile.existsAsFile())
        recoveryFile.deleteFile();
}

juce::Array<juce::File> ProjectWorkspace::findRecoveryFiles() const
{
    juce::Array<juce::File> files;
    getRecoveryDirectory().findChildFiles (files,
                                            juce::File::findFiles,
                                            false,
                                            "*.autosave.triumph");

    std::sort (files.begin(), files.end(), [] (const auto& left, const auto& right)
    {
        return left.getLastModificationTime() > right.getLastModificationTime();
    });
    return files;
}

juce::File ProjectWorkspace::getRecordingJournalDirectory() const
{
    return getRecoveryDirectory().getChildFile ("RecordingJournals");
}

juce::Array<juce::File> ProjectWorkspace::findRecordingJournals() const
{
    return RecordingJournal::find (getRecordingJournalDirectory());
}

juce::File ProjectWorkspace::getPluginRegistryFile() const
{
    return getRecoveryDirectory().getParentDirectory().getChildFile (
        "plugin-registry.json");
}

juce::Result ProjectWorkspace::createRollingBackup (const juce::File& projectFile,
                                                     const ProjectModel& project) const
{
    if (projectFile == juce::File())
        return juce::Result::ok();

    const auto backupDirectory = projectFile.getParentDirectory().getChildFile ("Backups");
    if (backupDirectory.createDirectory().failed())
        return juce::Result::fail ("The project backup folder could not be created.");

    const auto timestamp = juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S");
    auto backupFile = backupDirectory.getChildFile (
        projectFile.getFileNameWithoutExtension() + "-" + timestamp + ".triumph");

    if (backupFile.existsAsFile())
        backupFile = backupDirectory.getNonexistentChildFile (
            projectFile.getFileNameWithoutExtension() + "-" + timestamp,
            ".triumph",
            false);

    if (projectFile.existsAsFile())
    {
        ProjectState previousState;
        const auto loadResult = ProjectDocument::load (projectFile, previousState);
        if (loadResult.failed())
            return juce::Result::fail ("The previous saved project could not be read for backup: "
                                       + loadResult.getErrorMessage());

        ProjectModel previousProject;
        previousProject.replaceWithSnapshot (previousState);
        const auto saveResult = ProjectDocument::save (backupFile, previousProject);
        if (saveResult.failed())
            return saveResult;
    }
    else
    {
        const auto saveResult = ProjectDocument::save (backupFile, project);
        if (saveResult.failed())
            return saveResult;
    }

    juce::Array<juce::File> backups;
    backupDirectory.findChildFiles (backups,
                                    juce::File::findFiles,
                                    false,
                                    "*.triumph");
    std::sort (backups.begin(), backups.end(), [] (const auto& left, const auto& right)
    {
        return left.getLastModificationTime() > right.getLastModificationTime();
    });

    for (int index = maximumRollingBackups; index < backups.size(); ++index)
        backups.getReference (index).deleteFile();

    return juce::Result::ok();
}

juce::Result ProjectWorkspace::collectMedia (ProjectModel& project,
                                              const juce::File& projectFile,
                                              int& copiedFileCount) const
{
    copiedFileCount = 0;

    if (projectFile == juce::File())
        return juce::Result::fail ("Save the project before collecting its media.");

    const auto mediaDirectory = projectFile.getParentDirectory().getChildFile ("Media");
    if (mediaDirectory.createDirectory().failed())
        return juce::Result::fail ("The project Media folder could not be created.");

    std::vector<std::pair<juce::String, juce::File>> stagedPaths;

    for (int index = 0; index < project.getMediaSourceCount(); ++index)
    {
        const auto sourceId = project.getMediaSourceId (index);
        const auto source = project.getMediaSourceState (sourceId);

        if (! source.file.existsAsFile())
            return juce::Result::fail ("Cannot collect missing media: "
                                       + source.file.getFullPathName());

        if (source.file.getParentDirectory() == mediaDirectory
            || source.file.isAChildOf (mediaDirectory))
            continue;

        auto destination = mediaDirectory.getChildFile (source.file.getFileName());

        if (destination.existsAsFile())
            destination = mediaDirectory.getNonexistentChildFile (
                source.file.getFileNameWithoutExtension(),
                source.file.getFileExtension(),
                false);

        if (! source.file.copyFileTo (destination))
            return juce::Result::fail ("Could not copy " + source.file.getFileName()
                                       + " into the project Media folder.");

        stagedPaths.emplace_back (sourceId, destination);
    }

    for (const auto& [sourceId, destination] : stagedPaths)
    {
        project.setMediaSourceFile (sourceId, destination, false);
        ++copiedFileCount;
    }

    return juce::Result::ok();
}

juce::Result ProjectWorkspace::relinkMissingMedia (ProjectModel& project,
                                                    const juce::File& searchDirectory,
                                                    int& relinkedCount,
                                                    juce::StringArray& unresolvedFiles) const
{
    relinkedCount = 0;
    unresolvedFiles.clear();

    if (! searchDirectory.isDirectory())
        return juce::Result::fail ("Choose a valid folder to search for missing media.");

    std::vector<std::pair<juce::String, juce::File>> stagedPaths;

    for (int index = 0; index < project.getMediaSourceCount(); ++index)
    {
        const auto sourceId = project.getMediaSourceId (index);
        const auto source = project.getMediaSourceState (sourceId);

        if (source.file.existsAsFile())
            continue;

        juce::Array<juce::File> matches;
        searchDirectory.findChildFiles (matches,
                                        juce::File::findFiles,
                                        true,
                                        source.file.getFileName());

        if (matches.isEmpty())
            unresolvedFiles.add (source.file.getFileName());
        else if (matches.size() == 1)
            stagedPaths.emplace_back (sourceId, matches.getFirst());
        else
            unresolvedFiles.add (source.file.getFileName() + " (multiple matches found)");
    }

    for (const auto& [sourceId, replacement] : stagedPaths)
    {
        project.setMediaSourceFile (sourceId, replacement, false);
        ++relinkedCount;
    }

    return juce::Result::ok();
}

int ProjectWorkspace::countMissingMedia (const ProjectModel& project) const
{
    int missingCount = 0;

    for (int index = 0; index < project.getMediaSourceCount(); ++index)
        if (! project.getMediaSourceState (project.getMediaSourceId (index)).file.existsAsFile())
            ++missingCount;

    return missingCount;
}

std::unique_ptr<juce::XmlElement> ProjectWorkspace::getAudioDeviceState()
{
    if (const auto* settings = properties.getUserSettings())
        return juce::parseXML (settings->getValue (audioDeviceStateKey));

    return {};
}

void ProjectWorkspace::setAudioDeviceState (const juce::XmlElement& state)
{
    if (auto* settings = properties.getUserSettings())
    {
        settings->setValue (audioDeviceStateKey, state.toString());
        settings->saveIfNeeded();
    }
}

void ProjectWorkspace::saveRecentProjects()
{
    if (auto* settings = properties.getUserSettings())
    {
        settings->setValue (recentProjectsKey, recentProjects.toString());
        settings->saveIfNeeded();
    }
}

juce::File ProjectWorkspace::getRecoveryDirectory() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Triumph Studio")
        .getChildFile ("Recovery");
}
}
