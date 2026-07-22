#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>

#include "model/ProjectModel.h"

namespace triumph
{
class ProjectDocument final
{
public:
    static constexpr int currentFormatVersion = 24;

    static juce::Result save (const juce::File& destination,
                              const ProjectModel& project);

    static juce::Result load (const juce::File& source,
                              ProjectState& destination);

private:
    static juce::Result loadVersionOne (const juce::DynamicObject& root,
                                        ProjectState& destination);
    static juce::Result loadVersionTwo (const juce::DynamicObject& root,
                                        ProjectState& destination);
    static juce::Result loadVersionThree (const juce::DynamicObject& root,
                                          const juce::File& projectDirectory,
                                          ProjectState& destination);
    static juce::Result loadVersionFour (const juce::DynamicObject& root,
                                         const juce::File& projectDirectory,
                                         ProjectState& destination);
    static juce::Result loadVersionFive (const juce::DynamicObject& root,
                                         const juce::File& projectDirectory,
                                         ProjectState& destination);
    static juce::Result loadVersionSix (const juce::DynamicObject& root,
                                        const juce::File& projectDirectory,
                                        ProjectState& destination);
    static juce::Result loadVersionSeven (const juce::DynamicObject& root,
                                          const juce::File& projectDirectory,
                                          ProjectState& destination);
    static juce::Result loadVersionEight (const juce::DynamicObject& root,
                                          const juce::File& projectDirectory,
                                          ProjectState& destination);
    static juce::Result loadVersionNine (const juce::DynamicObject& root,
                                         const juce::File& projectDirectory,
                                         ProjectState& destination);
    static juce::Result loadVersionTen (const juce::DynamicObject& root,
                                        const juce::File& projectDirectory,
                                        ProjectState& destination);
    static juce::Result loadVersionEleven (const juce::DynamicObject& root,
                                           const juce::File& projectDirectory,
                                           ProjectState& destination);
    static juce::Result loadVersionTwelve (const juce::DynamicObject& root,
                                           const juce::File& projectDirectory,
                                           ProjectState& destination);
    static juce::Result loadVersionThirteen (const juce::DynamicObject& root,
                                             const juce::File& projectDirectory,
                                             ProjectState& destination);
    static juce::Result loadVersionFourteen (const juce::DynamicObject& root,
                                             const juce::File& projectDirectory,
                                             ProjectState& destination);
    static juce::Result loadVersionFifteen (const juce::DynamicObject& root,
                                            const juce::File& projectDirectory,
                                            ProjectState& destination);
    static juce::Result loadVersionSixteen (const juce::DynamicObject& root,
                                            const juce::File& projectDirectory,
                                            ProjectState& destination);
    static juce::Result loadVersionSeventeen (const juce::DynamicObject& root,
                                              const juce::File& projectDirectory,
                                              ProjectState& destination);
    static juce::Result loadVersionEighteen (const juce::DynamicObject& root,
                                             const juce::File& projectDirectory,
                                             ProjectState& destination);
    static juce::Result loadVersionNineteen (const juce::DynamicObject& root,
                                             const juce::File& projectDirectory,
                                             ProjectState& destination);
    static juce::Result loadVersionTwenty (const juce::DynamicObject& root,
                                           const juce::File& projectDirectory,
                                           ProjectState& destination);
    static juce::Result loadVersionTwentyOne (const juce::DynamicObject& root,
                                              const juce::File& projectDirectory,
                                              ProjectState& destination);
    static juce::Result loadVersionTwentyTwo (const juce::DynamicObject& root,
                                              const juce::File& projectDirectory,
                                              ProjectState& destination);
    static juce::Result loadVersionTwentyThree (const juce::DynamicObject& root,
                                                const juce::File& projectDirectory,
                                                ProjectState& destination);
    static juce::Result loadVersionTwentyFour (const juce::DynamicObject& root,
                                               const juce::File& projectDirectory,
                                               ProjectState& destination);
};
}
