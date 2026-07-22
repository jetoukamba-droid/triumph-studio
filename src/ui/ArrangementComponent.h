#pragma once

#include <JuceHeader.h>

#include "audio/AudioEngine.h"
#include "model/ProjectModel.h"
#include "TrackRowComponent.h"

#include <functional>
#include <vector>

namespace triumph
{
class ArrangementComponent final : public juce::Component
{
public:
    using TrackDeleteRequestCallback = std::function<void (const juce::String&)>;

    ArrangementComponent (ProjectModel& projectModel, AudioEngine& sharedEngine);
    ~ArrangementComponent() override;

    void refreshFromModel();
    void setSnapEnabled (bool shouldSnap);
    bool isSnapEnabled() const noexcept;
    void setPixelsPerSecond (double newPixelsPerSecond);
    double getPixelsPerSecond() const noexcept;
    bool splitSelectedClipAt (double timelineSeconds);
    bool deleteSelectedClip();
    bool activateSelectedTake();
    bool hasSelectedClip() const noexcept;
    void setTrackDeleteRequestCallback (TrackDeleteRequestCallback callback);
    juce::String getSelectedTrackId() const { return selectedTrackId; }
    juce::String getSelectedClipId() const { return selectedClipId; }

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    enum class RulerDragMode
    {
        none,
        seek,
        loopStart,
        loopEnd,
        loopRange
    };

    void rebuildRows();
    void updateContentBounds();
    void selectClip (const juce::String& trackId, const juce::String& clipId);
    int heightForTrack (const juce::String& trackId) const;
    void setHeightForTrack (const juce::String& trackId, int newHeight);
    juce::int64 getGridSamples() const;
    juce::Rectangle<int> getRulerArea() const;
    juce::int64 rulerSampleAtX (float x) const;
    float rulerXForSample (juce::int64 sample) const;
    void updateLoopRangeFromDrag (float x);
    void paintTimelineRuler (juce::Graphics&);

    ProjectModel& project;
    AudioEngine& engine;
    juce::Viewport viewport;
    juce::Component content;
    juce::OwnedArray<TrackRowComponent> rows;
    std::vector<std::pair<juce::String, int>> trackHeights;
    juce::Label emptyState;
    juce::String selectedTrackId;
    juce::String selectedClipId;
    TrackDeleteRequestCallback onTrackDeleteRequested;
    double pixelsPerSecond = 80.0;
    bool snapEnabled = true;
    RulerDragMode rulerDragMode = RulerDragMode::none;
    juce::int64 rulerDragAnchorSamples = 0;
    static constexpr int rulerHeight = 46;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArrangementComponent)
};
}
