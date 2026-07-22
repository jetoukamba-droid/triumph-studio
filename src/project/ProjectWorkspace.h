#pragma once

#include <JuceHeader.h>

#include "model/ProjectModel.h"

#include <utility>
#include <vector>

namespace triumph
{
class ProjectWorkspace final
{
public:
    ProjectWorkspace();
    ~ProjectWorkspace();

    void addRecentProject (const juce::File& projectFile);
    int getRecentProjectCount() const;
    juce::File getRecentProject (int index) const;
    void clearRecentProjects();

    juce::File getRecoveryFile (const ProjectModel& project) const;
    juce::Result writeRecoveryFile (const ProjectModel& project) const;
    void deleteRecoveryFile (const ProjectModel& project) const;
    juce::Array<juce::File> findRecoveryFiles() const;
    juce::File getRecordingJournalDirectory() const;
    juce::Array<juce::File> findRecordingJournals() const;
    juce::File getPluginRegistryFile() const;

    juce::Result createRollingBackup (const juce::File& projectFile,
                                      const ProjectModel& project) const;

    juce::Result collectMedia (ProjectModel& project,
                               const juce::File& projectFile,
                               int& copiedFileCount) const;

    juce::Result relinkMissingMedia (ProjectModel& project,
                                     const juce::File& searchDirectory,
                                     int& relinkedCount,
                                     juce::StringArray& unresolvedFiles) const;

    int countMissingMedia (const ProjectModel& project) const;

    std::unique_ptr<juce::XmlElement> getAudioDeviceState();
    void setAudioDeviceState (const juce::XmlElement& state);

private:
    void saveRecentProjects();
    juce::File getRecoveryDirectory() const;

    juce::ApplicationProperties properties;
    juce::RecentlyOpenedFilesList recentProjects;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProjectWorkspace)
};
}
