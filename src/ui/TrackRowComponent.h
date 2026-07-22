#pragma once

#include <JuceHeader.h>

#include "audio/AudioEngine.h"
#include "core/ArrangementLayout.h"
#include "model/ProjectModel.h"

#include <functional>

namespace triumph
{
class TrackRowComponent final : public juce::Component,
                                private juce::Timer
{
public:
    using ClipSelectionCallback = std::function<void (const juce::String&,
                                                       const juce::String&)>;
    using TrackDeleteRequestCallback = std::function<void (const juce::String&)>;
    using TrackHeightChangeCallback = std::function<void (const juce::String&, int)>;

    static constexpr int controlWidth = arrangement::trackHeaderWidth;
    static constexpr int preferredHeight = arrangement::preferredTrackHeight;
    static constexpr int eventDisplayInset = arrangement::eventDisplayInset;

    TrackRowComponent (ProjectModel& projectModel,
                       AudioEngine& sharedEngine,
                       juce::String stableTrackId);
    ~TrackRowComponent() override;

    const juce::String& getTrackId() const noexcept;
    int getPreferredHeight() const;
    void refreshFromModel();
    void setTimelineView (double newPixelsPerSecond,
                          bool shouldSnap,
                          juce::int64 newGridSamples);
    void setSelectedClip (const juce::String& clipId);
    void setTrackSelected (bool shouldBeSelected);
    void setClipSelectionCallback (ClipSelectionCallback callback);
    void setTrackDeleteRequestCallback (TrackDeleteRequestCallback callback);
    void setTrackHeightChangeCallback (TrackHeightChangeCallback callback);
    void setUserHeight (int newHeight);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    enum class DragMode
    {
        none,
        seek,
        compRange,
        warpMarker,
        move,
        trimStart,
        trimEnd,
        stretchEnd,
        fadeIn,
        fadeOut,
        trackHeight
    };

    void timerCallback() override;
    void seekFromMouseX (float x);
    juce::Rectangle<int> getTimelineArea() const;
    juce::Rectangle<int> getCompLaneArea() const;
    juce::Rectangle<int> getClipBounds (const AudioClipState& clip) const;
    juce::String findClipAt (juce::Point<float> position) const;
    DragMode editModeAt (const AudioClipState& clip,
                         juce::Point<float> position) const;
    juce::String findWarpMarkerAt (const AudioClipState& clip, float x) const;
    juce::String clipDisplayName (const AudioClipState& clip) const;
    void showTrackContextMenu();
    void showTrackRenameDialog();
    void showTrackInformation();
    void populateInstrumentSelector (const TrackState& track);
    void showClipContextMenu (const juce::String& clipId);
    void showClipRenameDialog (const juce::String& clipId);
    void showClipInformation (const juce::String& clipId);
    juce::int64 snapped (juce::int64 sample) const;
    juce::int64 sampleAtX (float x) const;
    bool hasTakeLanes() const;
    juce::String formatDuration (double seconds) const;

    ProjectModel& project;
    AudioEngine& engine;
    const juce::String trackId;

    juce::Label nameLabel;
    juce::Label infoLabel;
    juce::TextButton muteButton { "M" };
    juce::TextButton soloButton { "S" };
    juce::TextButton recordButton { "R" };
    juce::ComboBox instrumentSelector;
    juce::Label gainLabel { {}, "VOL" };
    juce::Slider gainSlider;
    juce::Label panLabel { {}, "PAN" };
    juce::Slider panSlider;
    juce::Label latencyLabel { {}, "OFFSET" };
    juce::Slider latencySlider;

    ClipSelectionCallback onClipSelected;
    TrackDeleteRequestCallback onTrackDeleteRequested;
    TrackHeightChangeCallback onTrackHeightChanged;
    juce::String selectedClipId;
    bool trackSelected = false;
    DragMode dragMode = DragMode::none;
    AudioClipState dragOriginClip;
    AudioClipState dragPreviewClip;
    juce::String draggedWarpMarkerId;
    bool draggedWarpMarkerWasNew = false;
    float dragOriginX = 0.0f;
    float dragOriginY = 0.0f;
    int userHeight = preferredHeight;
    int resizeStartHeight = preferredHeight;
    juce::int64 compRangeAnchor = 0;
    juce::int64 compRangeCurrent = 0;
    double pixelsPerSecond = 80.0;
    bool snapEnabled = true;
    juce::int64 gridSamples = 12000;
    double lastPaintedPlayheadSeconds = -1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackRowComponent)
};
}
