#include "model/ProjectModel.h"
#include "project/ProjectDocument.h"
#include "audio/OfflineRenderer.h"
#include "audio/StemExporter.h"
#include "TestSupport.h"
#include <cmath>

namespace
{
bool closeEnough (float a, float b)
{
    return std::abs (a - b) < 0.0001f;
}
}

int main()
{
    using namespace triumph;

    auto testFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                        .getNonexistentChildFile ("triumph-model-test", ".wav", false);
    REQUIRE (testFile.replaceWithText ("test"));

    ProjectModel project;
    REQUIRE (project.getTrackCount() == 0);
    REQUIRE (project.getName() == "Untitled Project");
    REQUIRE (project.getProjectId().isNotEmpty());
    const auto projectId = project.getProjectId();

    const auto trackId = project.addAudioTrack (testFile, 48000.0, 2, 96000);
    REQUIRE (trackId.isNotEmpty());
    REQUIRE (project.getTrackCount() == 1);
    REQUIRE (project.hasTrack (trackId));

    const auto imported = project.getTrackState (trackId);
    REQUIRE (imported.id == trackId);
    REQUIRE (imported.clips.size() == 1);
    REQUIRE (imported.clips.front().lengthInSamples == 96000);
    REQUIRE (project.getMediaSourceState (imported.clips.front().sourceId).file == testFile);
    ProjectModel latencyProject;
    const auto latencyTrack = latencyProject.addEmptyAudioTrack ("Latency Test");
    REQUIRE (! latencyProject.getLowLatencyMonitoring());
    latencyProject.setLowLatencyMonitoring (true);
    REQUIRE (latencyProject.getLowLatencyMonitoring());
    REQUIRE (latencyProject.undo());
    REQUIRE (! latencyProject.getLowLatencyMonitoring());
    latencyProject.setTrackManualLatencyOffset (latencyTrack, 384);
    REQUIRE (latencyProject.getTrackState (
        latencyTrack).manualLatencyOffsetSamples == 384);
    REQUIRE (latencyProject.undo());
    REQUIRE (latencyProject.getTrackState (
        latencyTrack).manualLatencyOffsetSamples == 0);

    ProjectModel mixerProject;
    const auto mixerTrack = mixerProject.addEmptyAudioTrack ("Mixer Track");
    const auto returnId = mixerProject.addMixerChannel (
        MixerChannelKind::returnChannel, "Plate Return");
    const auto busId = mixerProject.addMixerChannel (
        MixerChannelKind::bus, "Music Bus");
    REQUIRE (mixerProject.getMixerChannels().size() == 2);
    mixerProject.setTrackOutputDestination (mixerTrack, busId);
    REQUIRE (mixerProject.getTrackState (mixerTrack).outputDestinationId == busId);
    const auto sendId = mixerProject.addMixerSend (mixerTrack, returnId);
    REQUIRE (sendId.isNotEmpty());
    mixerProject.setMixerSendGain (mixerTrack, sendId, 0.42f);
    mixerProject.setMixerSendPreFader (mixerTrack, sendId, true);
    const auto sidechainId = mixerProject.addMixerSend (mixerTrack, busId, true);
    REQUIRE (sidechainId.isNotEmpty());
    const auto routedTrack = mixerProject.getTrackState (mixerTrack);
    REQUIRE (routedTrack.sends.size() == 2);
    REQUIRE (closeEnough (routedTrack.sends.front().gain, 0.42f));
    REQUIRE (routedTrack.sends.front().preFader);
    REQUIRE (routedTrack.sends.back().sidechain);
    mixerProject.setMasterMuted (true);
    REQUIRE (mixerProject.getMasterMuted());
    REQUIRE (mixerProject.undo());
    REQUIRE (! mixerProject.getMasterMuted());

    const auto clipId = imported.clips.front().id;
    project.beginUndoTransaction ("Move Clip");
    REQUIRE (project.moveClip (trackId, clipId, 24000));
    REQUIRE (project.getClipState (trackId, clipId).timelineStartSamples == 24000);
    REQUIRE (project.undo());
    REQUIRE (project.getClipState (trackId, clipId).timelineStartSamples == 0);

    project.beginUndoTransaction ("Trim Clip End");
    REQUIRE (project.trimClipEnd (trackId, clipId, 72000));
    REQUIRE (project.getClipState (trackId, clipId).lengthInSamples == 72000);
    REQUIRE (project.undo());
    REQUIRE (project.getClipState (trackId, clipId).lengthInSamples == 96000);

    project.beginUndoTransaction ("Trim Clip Start");
    REQUIRE (project.trimClipStart (trackId, clipId, 24000));
    REQUIRE (project.getClipState (trackId, clipId).sourceOffsetSamples == 24000);
    REQUIRE (project.getClipState (trackId, clipId).lengthInSamples == 72000);
    REQUIRE (project.undo());

    const auto secondClipId = project.splitClip (trackId, clipId, 48000);
    REQUIRE (secondClipId.isNotEmpty());
    REQUIRE (project.getTrackState (trackId).clips.size() == 2);
    REQUIRE (project.getClipState (trackId, clipId).lengthInSamples == 48000);
    REQUIRE (project.getClipState (trackId, secondClipId).timelineStartSamples == 48000);
    REQUIRE (project.getClipState (trackId, secondClipId).sourceOffsetSamples == 48000);
    REQUIRE (project.undo());
    REQUIRE (project.getTrackState (trackId).clips.size() == 1);

    ProjectModel clipEditingProject;
    const auto clipEditingTrackId = clipEditingProject.addAudioTrack (
        testFile, 48000.0, 2, 96000);
    const auto clipEditingId = clipEditingProject.getTrackState (
        clipEditingTrackId).clips.front().id;
    const auto customClipColour = juce::Colour (0xff22c55e);
    REQUIRE (clipEditingProject.setClipName (
        clipEditingTrackId, clipEditingId, "Verse Lead"));
    REQUIRE (clipEditingProject.setClipColour (
        clipEditingTrackId, clipEditingId, customClipColour));
    REQUIRE (clipEditingProject.setClipMuted (
        clipEditingTrackId, clipEditingId, true));
    REQUIRE (clipEditingProject.resolveAudioPlayback (
        clipEditingTrackId).empty());
    REQUIRE (clipEditingProject.setClipMuted (
        clipEditingTrackId, clipEditingId, false));
    REQUIRE (clipEditingProject.setClipLocked (
        clipEditingTrackId, clipEditingId, true));
    REQUIRE (! clipEditingProject.moveClip (
        clipEditingTrackId, clipEditingId, 24000));
    REQUIRE (clipEditingProject.splitClip (
        clipEditingTrackId, clipEditingId, 48000).isEmpty());
    REQUIRE (! clipEditingProject.removeClip (
        clipEditingTrackId, clipEditingId));
    REQUIRE (clipEditingProject.setClipLocked (
        clipEditingTrackId, clipEditingId, false));
    const auto originalVariantId = clipEditingProject.captureClipVariant (
        clipEditingTrackId, clipEditingId, "Original Edit");
    REQUIRE (originalVariantId.isNotEmpty());
    REQUIRE (clipEditingProject.setClipFades (
        clipEditingTrackId, clipEditingId, 1200, 2400));
    REQUIRE (clipEditingProject.setClipGain (
        clipEditingTrackId, clipEditingId, 0.50f, "Audition Clip Gain"));
    REQUIRE (clipEditingProject.setClipReversed (
        clipEditingTrackId, clipEditingId, true));
    REQUIRE (clipEditingProject.restoreClipVariant (
        clipEditingTrackId, clipEditingId, originalVariantId));
    auto restoredVariantClip = clipEditingProject.getClipState (
        clipEditingTrackId, clipEditingId);
    REQUIRE (restoredVariantClip.variants.size() == 1);
    REQUIRE (restoredVariantClip.fadeInSamples == 0);
    REQUIRE (restoredVariantClip.fadeOutSamples == 0);
    REQUIRE (closeEnough (restoredVariantClip.gain, 1.0f));
    REQUIRE (! restoredVariantClip.reversed);
    REQUIRE (clipEditingProject.undo());
    restoredVariantClip = clipEditingProject.getClipState (
        clipEditingTrackId, clipEditingId);
    REQUIRE (restoredVariantClip.fadeInSamples == 1200);
    REQUIRE (restoredVariantClip.fadeOutSamples == 2400);
    REQUIRE (closeEnough (restoredVariantClip.gain, 0.50f));
    REQUIRE (restoredVariantClip.reversed);
    REQUIRE (clipEditingProject.redo());
    restoredVariantClip = clipEditingProject.getClipState (
        clipEditingTrackId, clipEditingId);
    REQUIRE (! restoredVariantClip.reversed);
    const auto duplicateClipId = clipEditingProject.duplicateClip (
        clipEditingTrackId, clipEditingId);
    REQUIRE (duplicateClipId.isNotEmpty());
    const auto sourceClip = clipEditingProject.getClipState (
        clipEditingTrackId, clipEditingId);
    const auto duplicatedClip = clipEditingProject.getClipState (
        clipEditingTrackId, duplicateClipId);
    REQUIRE (duplicatedClip.sourceId == sourceClip.sourceId);
    REQUIRE (duplicatedClip.name == "Verse Lead");
    REQUIRE (duplicatedClip.colour == customClipColour);
    REQUIRE (duplicatedClip.timelineStartSamples
             == sourceClip.timelineStartSamples + sourceClip.lengthInSamples);
    REQUIRE (clipEditingProject.setClipMuted (
        clipEditingTrackId, duplicateClipId, true));
    REQUIRE (clipEditingProject.setClipLocked (
        clipEditingTrackId, duplicateClipId, true));
    const auto clipEditingFile = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-clip-editing-test", ".triumph", false);
    REQUIRE (ProjectDocument::save (
        clipEditingFile, clipEditingProject).wasOk());
    ProjectState loadedClipEditingSnapshot;
    REQUIRE (ProjectDocument::load (
        clipEditingFile, loadedClipEditingSnapshot).wasOk());
    REQUIRE (loadedClipEditingSnapshot.tracks.size() == 1);
    REQUIRE (loadedClipEditingSnapshot.tracks.front().clips.size() == 2);
    REQUIRE (loadedClipEditingSnapshot.tracks.front().clips.front().variants.size()
             == 1);
    REQUIRE (loadedClipEditingSnapshot.tracks.front().clips.front().variants.front().name
             == "Original Edit");
    const auto& loadedDuplicate = loadedClipEditingSnapshot.tracks.front().clips.back();
    REQUIRE (loadedDuplicate.name == "Verse Lead");
    REQUIRE (loadedDuplicate.colour == customClipColour);
    REQUIRE (loadedDuplicate.muted);
    REQUIRE (loadedDuplicate.locked);

    ProjectModel fadeProject;
    const auto fadeTrackId = fadeProject.addAudioTrack (
        testFile, 48000.0, 2, 96000);
    const auto fadeClipId = fadeProject.getTrackState (
        fadeTrackId).clips.front().id;
    REQUIRE (fadeProject.setClipFades (
        fadeTrackId, fadeClipId, 12000, 24000));
    auto fadedClip = fadeProject.getClipState (fadeTrackId, fadeClipId);
    REQUIRE (fadedClip.fadeInSamples == 12000);
    REQUIRE (fadedClip.fadeOutSamples == 24000);
    auto fadedPlayback = fadeProject.resolveAudioPlayback (fadeTrackId);
    REQUIRE (fadedPlayback.size() == 1);
    REQUIRE (fadedPlayback.front().fadeInSamples == 12000);
    REQUIRE (fadedPlayback.front().fadeOutSamples == 24000);
    REQUIRE (fadedPlayback.front().fadeAnchorStartSamples == 0);
    REQUIRE (fadedPlayback.front().fadeAnchorLengthSamples == 96000);

    const auto fadeWarpMarkerId = fadeProject.addWarpMarker (
        fadeTrackId, fadeClipId, 48000);
    REQUIRE (fadeWarpMarkerId.isNotEmpty());
    fadedPlayback = fadeProject.resolveAudioPlayback (fadeTrackId);
    REQUIRE (fadedPlayback.size() == 2);
    for (const auto& segment : fadedPlayback)
    {
        REQUIRE (segment.fadeInSamples == 12000);
        REQUIRE (segment.fadeOutSamples == 24000);
        REQUIRE (segment.fadeAnchorStartSamples == 0);
        REQUIRE (segment.fadeAnchorLengthSamples == 96000);
    }

    const auto fadeCopyId = fadeProject.duplicateClip (
        fadeTrackId, fadeClipId);
    REQUIRE (fadeCopyId.isNotEmpty());
    REQUIRE (fadeProject.moveClip (fadeTrackId, fadeCopyId, 72000));
    REQUIRE (fadeProject.createClipCrossfade (fadeTrackId, fadeClipId));
    fadedClip = fadeProject.getClipState (fadeTrackId, fadeClipId);
    const auto crossfadedCopy = fadeProject.getClipState (
        fadeTrackId, fadeCopyId);
    REQUIRE (fadedClip.fadeOutSamples == 24000);
    REQUIRE (crossfadedCopy.fadeInSamples == 24000);
    REQUIRE (fadeProject.undo());
    REQUIRE (fadeProject.getClipState (
        fadeTrackId, fadeCopyId).fadeInSamples == 12000);

    const auto fadeProjectFile = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-clip-fade-test", ".triumph", false);
    REQUIRE (ProjectDocument::save (fadeProjectFile, fadeProject).wasOk());
    ProjectState loadedFadeSnapshot;
    REQUIRE (ProjectDocument::load (
        fadeProjectFile, loadedFadeSnapshot).wasOk());
    REQUIRE (loadedFadeSnapshot.tracks.size() == 1);
    REQUIRE (loadedFadeSnapshot.tracks.front().clips.size() == 2);
    REQUIRE (loadedFadeSnapshot.tracks.front().clips.front().fadeInSamples
             == 12000);
    REQUIRE (loadedFadeSnapshot.tracks.front().clips.front().fadeOutSamples
             == 24000);
    REQUIRE (loadedFadeSnapshot.tracks.front().clips.back().fadeInSamples
             == 12000);
    REQUIRE (loadedFadeSnapshot.tracks.front().clips.back().fadeOutSamples
             == 24000);

    ProjectModel fadeTransformProject;
    const auto fadeTransformTrack = fadeTransformProject.addAudioTrack (
        testFile, 48000.0, 2, 96000);
    const auto fadeTransformClip = fadeTransformProject.getTrackState (
        fadeTransformTrack).clips.front().id;
    REQUIRE (fadeTransformProject.setClipFades (
        fadeTransformTrack, fadeTransformClip, 12000, 24000));
    const auto splitFadeClip = fadeTransformProject.splitClip (
        fadeTransformTrack, fadeTransformClip, 48000);
    REQUIRE (splitFadeClip.isNotEmpty());
    REQUIRE (fadeTransformProject.getClipState (
        fadeTransformTrack, fadeTransformClip).fadeInSamples == 12000);
    REQUIRE (fadeTransformProject.getClipState (
        fadeTransformTrack, fadeTransformClip).fadeOutSamples == 0);
    REQUIRE (fadeTransformProject.getClipState (
        fadeTransformTrack, splitFadeClip).fadeInSamples == 0);
    REQUIRE (fadeTransformProject.getClipState (
        fadeTransformTrack, splitFadeClip).fadeOutSamples == 24000);
    REQUIRE (fadeTransformProject.undo());
    REQUIRE (fadeTransformProject.getTrackState (
        fadeTransformTrack).clips.size() == 1);
    REQUIRE (fadeTransformProject.stretchClipToEnd (
        fadeTransformTrack, fadeTransformClip, 192000));
    REQUIRE (fadeTransformProject.getClipState (
        fadeTransformTrack, fadeTransformClip).fadeInSamples == 24000);
    REQUIRE (fadeTransformProject.getClipState (
        fadeTransformTrack, fadeTransformClip).fadeOutSamples == 48000);

    ProjectModel sampleRateConversion;
    const auto convertedTrackId = sampleRateConversion.addAudioTrack (testFile,
                                                                      44100.0,
                                                                      2,
                                                                      44100);
    REQUIRE (convertedTrackId.isNotEmpty());
    REQUIRE (sampleRateConversion.getTrackState (convertedTrackId)
                .clips.front().lengthInSamples == 48000);

    ProjectModel stretchProject;
    const auto stretchTrackId = stretchProject.addAudioTrack (testFile,
                                                               48000.0, 2, 96000);
    const auto stretchClipId = stretchProject.getTrackState (
        stretchTrackId).clips.front().id;
    stretchProject.beginUndoTransaction ("Stretch Clip");
    REQUIRE (stretchProject.stretchClipToEnd (stretchTrackId,
                                               stretchClipId, 192000));
    auto stretched = stretchProject.getClipState (stretchTrackId, stretchClipId);
    REQUIRE (stretched.lengthInSamples == 192000);
    REQUIRE (std::abs (stretched.timeStretchRatio - 2.0) < 0.0001);
    REQUIRE (stretchProject.resolveAudioPlayback (stretchTrackId).front().playbackRate
             == 0.5);
    REQUIRE (stretchProject.undo());
    REQUIRE (stretchProject.getClipState (stretchTrackId, stretchClipId)
                .lengthInSamples == 96000);
    stretchProject.beginUndoTransaction ("Stretch Clip");
    REQUIRE (stretchProject.stretchClipToEnd (stretchTrackId,
                                               stretchClipId, 192000));
    stretchProject.beginUndoTransaction ("Trim Clip Start");
    REQUIRE (stretchProject.trimClipStart (stretchTrackId,
                                            stretchClipId, 48000));
    REQUIRE (stretchProject.getClipState (stretchTrackId, stretchClipId)
                .sourceOffsetSamples == 24000);
    REQUIRE (stretchProject.undo());
    const auto stretchedSecond = stretchProject.splitClip (stretchTrackId,
                                                            stretchClipId, 96000);
    REQUIRE (stretchedSecond.isNotEmpty());
    REQUIRE (stretchProject.getClipState (stretchTrackId, stretchedSecond)
                .sourceOffsetSamples == 48000);
    REQUIRE (std::abs (stretchProject.getClipState (stretchTrackId, stretchedSecond)
                           .timeStretchRatio - 2.0) < 0.0001);
    auto stretchProjectFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                  .getNonexistentChildFile (
                                      "triumph-stretch-project-test", ".triumph", false);
    REQUIRE (ProjectDocument::save (stretchProjectFile, stretchProject).wasOk());
    ProjectState loadedStretchSnapshot;
    REQUIRE (ProjectDocument::load (stretchProjectFile, loadedStretchSnapshot).wasOk());
    REQUIRE (loadedStretchSnapshot.tracks.front().clips.size() == 2);
    REQUIRE (std::abs (loadedStretchSnapshot.tracks.front().clips.front()
                           .timeStretchRatio - 2.0) < 0.0001);
    REQUIRE (loadedStretchSnapshot.tracks.front().clips.front()
                 .timeStretchProvider == "builtin-resample");

    ProjectModel warpProject;
    const auto warpTrackId = warpProject.addAudioTrack (testFile, 48000.0, 2, 96000);
    const auto warpClipId = warpProject.getTrackState (warpTrackId).clips.front().id;
    const auto warpMarkerId = warpProject.addWarpMarker (warpTrackId,
                                                         warpClipId, 48000);
    REQUIRE (warpMarkerId.isNotEmpty());
    REQUIRE (warpProject.getClipState (warpTrackId, warpClipId)
                .warpMarkers.front().sourceOffsetSamples == 48000);
    warpProject.beginUndoTransaction ("Move Warp Marker");
    REQUIRE (warpProject.moveWarpMarker (warpTrackId, warpClipId,
                                         warpMarkerId, 24000));
    const auto warpedPlayback = warpProject.resolveAudioPlayback (warpTrackId);
    REQUIRE (warpedPlayback.size() == 2);
    REQUIRE (std::abs (warpedPlayback.front().playbackRate - 2.0) < 0.0001);
    REQUIRE (warpProject.undo());
    REQUIRE (warpProject.getClipState (warpTrackId, warpClipId)
                .warpMarkers.front().timelineOffsetSamples == 48000);
    REQUIRE (warpProject.setClipReversed (warpTrackId, warpClipId, true));
    const auto reversedWarpPlayback = warpProject.resolveAudioPlayback (warpTrackId);
    REQUIRE (reversedWarpPlayback.size() == 2);
    REQUIRE (reversedWarpPlayback.front().reversed);
    REQUIRE (reversedWarpPlayback.back().reversed);
    REQUIRE (reversedWarpPlayback.front().sourceOffsetSamples == 48000);
    REQUIRE (reversedWarpPlayback.back().sourceOffsetSamples == 0);
    REQUIRE (warpProject.undo());
    auto warpProjectFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                               .getNonexistentChildFile (
                                   "triumph-warp-project-test", ".triumph", false);
    REQUIRE (ProjectDocument::save (warpProjectFile, warpProject).wasOk());
    ProjectState loadedWarpSnapshot;
    REQUIRE (ProjectDocument::load (warpProjectFile, loadedWarpSnapshot).wasOk());
    REQUIRE (loadedWarpSnapshot.tracks.front().clips.front().warpMarkers.size() == 1);
    REQUIRE (loadedWarpSnapshot.tracks.front().clips.front().warpMarkers.front().id
             == warpMarkerId);

    ProjectModel transientProject;
    const auto transientTrackId = transientProject.addAudioTrack (
        testFile, 48000.0, 2, 96000);
    const auto transientClipId = transientProject.getTrackState (
        transientTrackId).clips.front().id;
    const auto playbackBeforeDetection = transientProject.resolveAudioPlayback (
        transientTrackId);
    REQUIRE (transientProject.setTransientMarkers (transientTrackId,
        transientClipId, { { "onset-a", 0, 24000, 0.9f },
                           { "onset-b", 0, 72000, 0.7f } }));
    REQUIRE (transientProject.getClipState (transientTrackId, transientClipId)
                 .transientMarkers.size() == 2);
    const auto playbackAfterDetection = transientProject.resolveAudioPlayback (
        transientTrackId);
    REQUIRE (playbackAfterDetection.size() == playbackBeforeDetection.size());
    REQUIRE (transientProject.convertTransientsToWarpMarkers (
        transientTrackId, transientClipId) == 2);
    REQUIRE (transientProject.getClipState (transientTrackId, transientClipId)
                 .warpMarkers.size() == 2);
    REQUIRE (transientProject.undo());
    REQUIRE (transientProject.getClipState (transientTrackId, transientClipId)
                 .warpMarkers.empty());
    auto transientProjectFile = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-transient-project-test", ".triumph", false);
    REQUIRE (ProjectDocument::save (transientProjectFile, transientProject).wasOk());
    ProjectState loadedTransientSnapshot;
    REQUIRE (ProjectDocument::load (transientProjectFile,
                                     loadedTransientSnapshot).wasOk());
    REQUIRE (loadedTransientSnapshot.tracks.front().clips.front()
                 .transientMarkers.size() == 2);

    ProjectModel recordingProject;
    const auto firstEmptyTrack = recordingProject.addEmptyAudioTrack ("Lead Vocal");
    const auto secondEmptyTrack = recordingProject.addEmptyAudioTrack ("Harmony");
    REQUIRE (firstEmptyTrack.isNotEmpty() && secondEmptyTrack.isNotEmpty());
    recordingProject.setTrackRecordArmed (firstEmptyTrack, true);
    REQUIRE (recordingProject.getArmedTrackId() == firstEmptyTrack);
    recordingProject.setTrackRecordArmed (secondEmptyTrack, true);
    REQUIRE (recordingProject.getArmedTrackId() == firstEmptyTrack);
    REQUIRE (recordingProject.getTrackState (firstEmptyTrack).recordArmed);
    REQUIRE (recordingProject.getArmedTrackIds().size() == 2);
    const auto recordedClip = recordingProject.addRecordedClip (secondEmptyTrack,
                                                                 testFile,
                                                                 44100.0,
                                                                 1,
                                                                 44100,
                                                                 24000);
    REQUIRE (recordedClip.isNotEmpty());
    REQUIRE (recordingProject.getArmedTrackId() == firstEmptyTrack);
    REQUIRE (recordingProject.getArmedTrackIds().size() == 1);
    REQUIRE (recordingProject.getClipState (secondEmptyTrack, recordedClip)
                .timelineStartSamples == 24000);
    REQUIRE (recordingProject.getClipState (secondEmptyTrack, recordedClip)
                .lengthInSamples == 48000);
    REQUIRE (recordingProject.getClipState (secondEmptyTrack, recordedClip)
                .takeGroupId.isNotEmpty());
    REQUIRE (recordingProject.getClipState (secondEmptyTrack, recordedClip)
                .takeLaneIndex == 0);
    REQUIRE (recordingProject.getClipState (secondEmptyTrack, recordedClip)
                .activeTake);
    recordingProject.setTrackRecordArmed (secondEmptyTrack, true);
    const auto secondTake = recordingProject.addRecordedClip (secondEmptyTrack,
                                                               testFile,
                                                               44100.0,
                                                               1,
                                                               44100,
                                                               24000);
    REQUIRE (secondTake.isNotEmpty());
    const auto firstTakeState = recordingProject.getClipState (
        secondEmptyTrack, recordedClip);
    const auto secondTakeState = recordingProject.getClipState (
        secondEmptyTrack, secondTake);
    REQUIRE (firstTakeState.takeGroupId.isNotEmpty());
    REQUIRE (firstTakeState.takeGroupId == secondTakeState.takeGroupId);
    REQUIRE (firstTakeState.takeLaneIndex == 0);
    REQUIRE (secondTakeState.takeLaneIndex == 1);
    REQUIRE (! firstTakeState.activeTake);
    REQUIRE (secondTakeState.activeTake);
    REQUIRE (recordingProject.selectActiveTake (secondEmptyTrack, recordedClip));
    REQUIRE (recordingProject.getClipState (secondEmptyTrack, recordedClip).activeTake);
    REQUIRE (! recordingProject.getClipState (secondEmptyTrack, secondTake).activeTake);
    REQUIRE (recordingProject.undo());
    REQUIRE (recordingProject.getClipState (secondEmptyTrack, secondTake).activeTake);
    REQUIRE (recordingProject.assignCompRegion (secondEmptyTrack,
                                                 recordedClip,
                                                 36000,
                                                 48000));
    auto compedTrack = recordingProject.getTrackState (secondEmptyTrack);
    REQUIRE (compedTrack.compRegions.size() == 1);
    REQUIRE (compedTrack.compRegions.front().takeGroupId
             == firstTakeState.takeGroupId);
    REQUIRE (compedTrack.compRegions.front().takeLaneIndex == 0);
    REQUIRE (compedTrack.compRegions.front().timelineStartSamples == 36000);
    REQUIRE (compedTrack.compRegions.front().lengthInSamples == 12000);
    REQUIRE (compedTrack.compRegions.front().crossfadeSamples == 480);
    const auto resolvedComp = recordingProject.resolveAudioPlayback (secondEmptyTrack);
    REQUIRE (resolvedComp.size() == 3);
    REQUIRE (resolvedComp.front().sourceId == secondTakeState.sourceId);
    REQUIRE (resolvedComp[1].sourceId == firstTakeState.sourceId);
    REQUIRE (resolvedComp.back().sourceId == secondTakeState.sourceId);
    REQUIRE (resolvedComp.front().fadeOutSamples > 0);
    REQUIRE (resolvedComp[1].fadeInSamples > 0);
    REQUIRE (resolvedComp[1].fadeOutSamples > 0);
    REQUIRE (resolvedComp.back().fadeInSamples > 0);
    REQUIRE (recordingProject.assignCompRegion (secondEmptyTrack,
                                                 secondTake,
                                                 42000,
                                                 54000));
    REQUIRE (recordingProject.getTrackState (secondEmptyTrack).compRegions.size() == 2);
    REQUIRE (recordingProject.undo());
    REQUIRE (recordingProject.getTrackState (secondEmptyTrack).compRegions.size() == 1);
    recordingProject.setTrackRecordArmed (firstEmptyTrack, true);
    auto recordingProjectFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                                    .getNonexistentChildFile (
                                        "triumph-recording-project-test",
                                        ".triumph",
                                        false);
    REQUIRE (ProjectDocument::save (recordingProjectFile, recordingProject).wasOk());
    ProjectState loadedRecordingSnapshot;
    REQUIRE (ProjectDocument::load (recordingProjectFile, loadedRecordingSnapshot).wasOk());
    REQUIRE (loadedRecordingSnapshot.tracks.size() == 2);
    REQUIRE (loadedRecordingSnapshot.tracks.front().recordArmed);
    REQUIRE (loadedRecordingSnapshot.tracks[1].clips.size() == 2);
    REQUIRE (loadedRecordingSnapshot.tracks[1].clips[1].takeLaneIndex == 1);
    REQUIRE (loadedRecordingSnapshot.tracks[1].clips[1].activeTake);
    REQUIRE (loadedRecordingSnapshot.tracks[1].compRegions.size() == 1);
    REQUIRE (loadedRecordingSnapshot.tracks[1].compRegions.front().takeLaneIndex == 0);
    REQUIRE (loadedRecordingSnapshot.tracks[1].compRegions.front().lengthInSamples
             == 12000);
    REQUIRE (loadedRecordingSnapshot.tracks[1].compRegions.front().crossfadeSamples
             == 480);
    REQUIRE (recordingProject.selectActiveTake (secondEmptyTrack, recordedClip));
    REQUIRE (recordingProject.getTrackState (secondEmptyTrack).compRegions.empty());
    REQUIRE (recordingProject.getClipState (secondEmptyTrack, recordedClip).activeTake);
    REQUIRE (recordingProject.undo());
    REQUIRE (recordingProject.getTrackState (secondEmptyTrack).compRegions.size() == 1);
    REQUIRE (recordingProject.getClipState (secondEmptyTrack, secondTake).activeTake);

    ProjectModel midiProject;
    const auto instrumentTrack = midiProject.addInstrumentTrack ("Keys");
    REQUIRE (instrumentTrack.isNotEmpty());
    const auto instrument = midiProject.getTrackState (instrumentTrack);
    REQUIRE (instrument.isInstrument);
    REQUIRE (instrument.midiClips.size() == 1);
    const auto midiClipId = instrument.midiClips.front().id;
    midiProject.setTrackRecordArmed (instrumentTrack, true);
    REQUIRE (midiProject.getArmedTrackId() == instrumentTrack);
    std::vector<MidiNoteState> capturedNotes {
        { {}, 17.0, 0.5, 64, 0.62f, 2 },
        { {}, 18.0, 1.0, 67, 0.81f, 2 }
    };
    REQUIRE (midiProject.addMidiNotes (instrumentTrack,
                                       midiClipId,
                                       capturedNotes) == 2);
    auto recordedMidi = midiProject.getTrackState (instrumentTrack);
    REQUIRE (! recordedMidi.recordArmed);
    REQUIRE (recordedMidi.midiClips.front().notes.size() == 2);
    REQUIRE (recordedMidi.midiClips.front().lengthBeats >= 19.0);
    REQUIRE (recordedMidi.midiClips.front().notes.front().channel == 2);
    REQUIRE (midiProject.undo());
    recordedMidi = midiProject.getTrackState (instrumentTrack);
    REQUIRE (recordedMidi.recordArmed);
    REQUIRE (recordedMidi.midiClips.front().notes.empty());
    midiProject.setTrackRecordArmed (instrumentTrack, false);
    const auto midiNoteId = midiProject.addMidiNote (instrumentTrack,
                                                     midiClipId,
                                                     1.13,
                                                     0.5,
                                                     60,
                                                     0.75f);
    REQUIRE (midiNoteId.isNotEmpty());
    REQUIRE (midiProject.quantizeMidiClip (instrumentTrack, midiClipId, 0.25));
    REQUIRE (std::abs (midiProject.getTrackState (instrumentTrack)
                           .midiClips.front().notes.front().startBeat - 1.25) < 0.0001);
    midiProject.setTempoBpm (90.0);
    REQUIRE (std::abs (midiProject.getTempoBpm() - 90.0) < 0.0001);
    midiProject.setInstrumentPlugin (instrumentTrack,
                                     "DecentSampler",
                                     "<PLUGIN name=\"DecentSampler\"/>",
                                     "dGVzdA==");
    REQUIRE (midiProject.getTrackState (instrumentTrack).pluginName
             == "DecentSampler");
    midiProject.setBuiltInInstrument (instrumentTrack, "Triumph Synth");
    auto switchedInstrument = midiProject.getTrackState (instrumentTrack);
    REQUIRE (switchedInstrument.id == instrumentTrack);
    REQUIRE (switchedInstrument.pluginName == "Triumph Synth");
    REQUIRE (switchedInstrument.pluginDescriptionXml.isEmpty());
    REQUIRE (switchedInstrument.pluginStateBase64.isEmpty());
    REQUIRE (switchedInstrument.midiClips.size() == 1);
    REQUIRE (switchedInstrument.midiClips.front().notes.size() == 1);
    REQUIRE (midiProject.undo());
    REQUIRE (midiProject.getTrackState (instrumentTrack).pluginName
             == "DecentSampler");
    midiProject.setBuiltInInstrument (instrumentTrack, "Triumph Drums");
    REQUIRE (midiProject.getTrackState (instrumentTrack).pluginName
             == "Triumph Drums");
    midiProject.setInstrumentPluginBypassed (instrumentTrack, true);
    midiProject.setInstrumentPluginHealth (instrumentTrack, 256, true);
    REQUIRE (midiProject.getTrackState (instrumentTrack).pluginBypassed);
    REQUIRE (midiProject.getTrackState (instrumentTrack).pluginLatencySamples == 256);
    midiProject.setInstrumentPluginBypassed (instrumentTrack, false);
    REQUIRE (midiProject.freezeInstrumentTrack (
        instrumentTrack, testFile, 48000.0, 2, 96000));
    const auto frozenInstrument = midiProject.getTrackState (instrumentTrack);
    REQUIRE (frozenInstrument.frozen);
    REQUIRE (frozenInstrument.freezeSourceId.isNotEmpty());
    REQUIRE (frozenInstrument.clips.size() == 1);
    const auto secondInstrumentTrack = midiProject.addInstrumentTrack ("Layer");
    midiProject.setInstrumentPlugin (secondInstrumentTrack,
                                     "SecondInstrument",
                                     "<PLUGIN name=\"SecondInstrument\"/>",
                                     "c2Vjb25k");
    REQUIRE (midiProject.getTrackState (instrumentTrack).pluginName
             == "Triumph Drums");
    REQUIRE (midiProject.getTrackState (secondInstrumentTrack).pluginName
             == "SecondInstrument");
    REQUIRE (midiProject.undo());
    REQUIRE (midiProject.getTrackState (instrumentTrack).pluginName
             == "Triumph Drums");
    REQUIRE (midiProject.getTrackState (secondInstrumentTrack).pluginName.isEmpty());
    REQUIRE (midiProject.undo());
    REQUIRE (midiProject.getTrackCount() == 1);
    auto midiProjectFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                               .getNonexistentChildFile (
                                   "triumph-midi-project-test", ".triumph", false);
    REQUIRE (ProjectDocument::save (midiProjectFile, midiProject).wasOk());
    ProjectState loadedMidiSnapshot;
    REQUIRE (ProjectDocument::load (midiProjectFile, loadedMidiSnapshot).wasOk());
    REQUIRE (loadedMidiSnapshot.tracks.size() == 1);
    REQUIRE (loadedMidiSnapshot.tracks.front().isInstrument);
    REQUIRE (loadedMidiSnapshot.tracks.front().midiClips.size() == 1);
    REQUIRE (loadedMidiSnapshot.tracks.front().midiClips.front().notes.size() == 1);
    REQUIRE (loadedMidiSnapshot.tracks.front().pluginName == "Triumph Drums");
    REQUIRE (loadedMidiSnapshot.tracks.front().pluginStateBase64.isEmpty());
    REQUIRE (loadedMidiSnapshot.tracks.front().frozen);
    REQUIRE (loadedMidiSnapshot.tracks.front().freezeSourceId.isNotEmpty());
    REQUIRE (loadedMidiSnapshot.tracks.front().pluginLatencySamples == 256);
    REQUIRE (std::abs (loadedMidiSnapshot.tempoPoints.front().bpm - 90.0) < 0.0001);

    ProjectModel trackDeletionProject;
    const auto deletionTrack = trackDeletionProject.addInstrumentTrack ("Delete Me");
    const auto deletionLane = trackDeletionProject.addAutomationLane (
        deletionTrack, "gain", AutomationMode::read);
    REQUIRE (trackDeletionProject.hasTrack (deletionTrack));
    REQUIRE (deletionLane.isNotEmpty());
    REQUIRE (trackDeletionProject.removeTrack (deletionTrack));
    REQUIRE (! trackDeletionProject.hasTrack (deletionTrack));
    REQUIRE (trackDeletionProject.getAutomationLane (deletionLane).id.isEmpty());
    REQUIRE (trackDeletionProject.getUndoDescription() == "Delete Track");
    REQUIRE (trackDeletionProject.undo());
    REQUIRE (trackDeletionProject.hasTrack (deletionTrack));
    REQUIRE (trackDeletionProject.getAutomationLane (deletionLane).id
             == deletionLane);

    ProjectModel producerProject;
    ProducerSettingsState producerSettings;
    producerSettings.rootPitchClass = 9;
    producerSettings.scale = "minor";
    producerSettings.style = "energetic";
    producerSettings.bars = 8;
    producerSettings.variation = 77;
    producerProject.setProducerSettings (producerSettings, false);
    std::vector<MidiNoteState> generatedNotes {
        { {}, 0.0, 1.0, 57, 0.8f, 1 },
        { {}, 1.0, 1.0, 60, 0.7f, 1 },
        { {}, 2.0, 1.0, 64, 0.75f, 1 }
    };
    const auto generatedTrack = producerProject.addGeneratedMidiTrack (
        "AI Melody A minor", generatedNotes, 32.0,
        "Producer Create Melody");
    REQUIRE (generatedTrack.isNotEmpty());
    REQUIRE (producerProject.getTrackCount() == 1);
    REQUIRE (producerProject.getTrackState (generatedTrack).isInstrument);
    REQUIRE (producerProject.getTrackState (generatedTrack)
                 .midiClips.front().notes.size() == 3);
    REQUIRE (producerProject.getTrackState (generatedTrack)
                 .midiClips.front().lengthBeats == 32.0);
    REQUIRE (producerProject.undo());
    REQUIRE (producerProject.getTrackCount() == 0);
    REQUIRE (producerProject.redo());
    REQUIRE (producerProject.getTrackCount() == 1);
    REQUIRE (producerProject.applyTrackMixUpdates (
        { { generatedTrack, 0.58f, -0.24f } }) == 1);
    REQUIRE (closeEnough (producerProject.getTrackState (
        generatedTrack).gain, 0.58f));
    REQUIRE (closeEnough (producerProject.getTrackState (
        generatedTrack).pan, -0.24f));
    REQUIRE (producerProject.undo());
    REQUIRE (closeEnough (producerProject.getTrackState (
        generatedTrack).gain, 0.72f));
    REQUIRE (producerProject.getProducerSettings().rootPitchClass == 9);
    REQUIRE (producerProject.getProducerSettings().variation == 77);

    auto missingFreezeJson = juce::JSON::parse (
        midiProjectFile.loadFileAsString()).clone();
    auto missingFreezeSourcesValue = missingFreezeJson.getProperty (
        "mediaSources", juce::var());
    auto* missingFreezeSources = missingFreezeSourcesValue.getArray();
    REQUIRE (missingFreezeSources != nullptr && missingFreezeSources->size() == 1);
    missingFreezeSources->getReference (0).getDynamicObject()->setProperty (
        "path", midiProjectFile.getSiblingFile ("missing-freeze.wav")
                    .getFullPathName());
    missingFreezeSources->getReference (0).getDynamicObject()->setProperty (
        "pathIsRelative", false);
    auto missingFreezeProjectFile = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-missing-freeze-project", ".triumph", false);
    REQUIRE (missingFreezeProjectFile.replaceWithText (
        juce::JSON::toString (missingFreezeJson, true)));
    ProjectState missingFreezeSnapshot;
    REQUIRE (ProjectDocument::load (
        missingFreezeProjectFile, missingFreezeSnapshot).wasOk());
    REQUIRE (! missingFreezeSnapshot.tracks.front().frozen);
    REQUIRE (missingFreezeSnapshot.tracks.front().freezeSourceId.isEmpty());
    REQUIRE (missingFreezeSnapshot.tracks.front().clips.empty());
    REQUIRE (missingFreezeSnapshot.tracks.front().pluginName == "Triumph Drums");
    REQUIRE (missingFreezeSnapshot.repairRecords.size() == 1);
    REQUIRE (missingFreezeSnapshot.repairRecords.front().area == "freeze");
    REQUIRE (missingFreezeSnapshot.repairRecords.front().subjectId
             == missingFreezeSnapshot.tracks.front().id);

    ProjectModel restoredFrozenProject;
    restoredFrozenProject.replaceWithSnapshot (loadedMidiSnapshot);
    REQUIRE (restoredFrozenProject.unfreezeInstrumentTrack (instrumentTrack));
    REQUIRE (! restoredFrozenProject.getTrackState (instrumentTrack).frozen);
    REQUIRE (restoredFrozenProject.getTrackState (instrumentTrack).clips.empty());

    REQUIRE (project.canUndo());
    REQUIRE (project.undo());
    REQUIRE (project.getTrackCount() == 0);
    REQUIRE (project.redo());
    REQUIRE (project.getTrackCount() == 1);

    project.beginUndoTransaction ("Adjust Track Volume");
    project.setTrackGain (trackId, 0.42f);
    REQUIRE (closeEnough (project.getTrackState (trackId).gain, 0.42f));
    REQUIRE (project.undo());
    REQUIRE (closeEnough (project.getTrackState (trackId).gain, 0.85f));

    const auto snapshot = project.createSnapshot();
    REQUIRE (snapshot.projectId == projectId);
    ProjectModel restored;
    restored.replaceWithSnapshot (snapshot);
    REQUIRE (restored.getProjectId() == projectId);
    REQUIRE (restored.getTrackCount() == 1);
    REQUIRE (restored.getTrackId (0) == trackId);
    REQUIRE (restored.getTrackState (trackId).clips.front().sourceId
            == imported.clips.front().sourceId);

    auto projectFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                           .getNonexistentChildFile ("triumph-project-test", ".triumph", false);
    restored.setLowLatencyMonitoring (true, false);
    restored.setInputMonitorMode (InputMonitorMode::whileArmed, false);
    restored.setManualRecordOffsetSamples (-96, false);
    RecordingSettingsState recordingSettings;
    recordingSettings.mode = RecordingMode::loop;
    recordingSettings.preRollBars = 2;
    recordingSettings.loopStartSamples = 24000;
    recordingSettings.loopEndSamples = 120000;
    recordingSettings.controlRoomGain = 0.72f;
    recordingSettings.controlRoomDimmed = true;
    restored.setRecordingSettings (recordingSettings, false);
    restored.setTrackInputRoute (trackId, 1, 1);
    restored.setTrackRecordingTapPoint (
        trackId, RecordingTapPoint::programOutput);
    restored.setTrackRecordOffset (trackId, 64);
    restored.setTrackManualLatencyOffset (trackId, -128);
    REQUIRE (restored.setClipReversed (trackId, clipId, true));
    REQUIRE (restored.getClipState (trackId, clipId).reversed);
    const auto restoredReturn = restored.addMixerChannel (
        MixerChannelKind::returnChannel, "Room Return");
    const auto restoredBus = restored.addMixerChannel (
        MixerChannelKind::bus, "Main Bus");
    restored.setTrackOutputDestination (trackId, restoredBus);
    const auto restoredSend = restored.addMixerSend (trackId, restoredReturn);
    restored.setMixerSendGain (trackId, restoredSend, 0.33f);
    restored.setMixerSendPreFader (trackId, restoredSend, true);
    restored.setMasterMuted (true);
    restored.setProducerSettings (producerSettings, false);
    const auto tempoPoint = restored.addTempoPoint (
        8.0, 96.0, TempoCurve::linear);
    REQUIRE (tempoPoint.isNotEmpty());
    const auto signaturePoint = restored.addTimeSignature (8.0, 3, 4);
    REQUIRE (signaturePoint.isNotEmpty());
    const auto projectMarker = restored.addMarker (4.0, "Verse");
    REQUIRE (projectMarker.isNotEmpty());
    restored.setMetronomeEnabled (true);
    restored.setMetronomeGain (0.37f);
    const auto automationLane = restored.addAutomationLane (
        trackId, "gain", AutomationMode::touch);
    REQUIRE (automationLane.isNotEmpty());
    REQUIRE (restored.addAutomationPoint (
        automationLane, 0, 0.25f, AutomationCurve::hold).isNotEmpty());
    REQUIRE (restored.addAutomationPoint (
        automationLane, 48000, 0.90f, AutomationCurve::smooth).isNotEmpty());
    const auto pluginAutomationLane = restored.addAutomationLane (
        trackId, "plugin:cutoff", AutomationMode::read);
    REQUIRE (pluginAutomationLane.isNotEmpty());
    REQUIRE (restored.addAutomationPoint (
        pluginAutomationLane, 24000, 0.50f,
        AutomationCurve::linear).isNotEmpty());
    REQUIRE (ProjectDocument::save (projectFile, restored).wasOk());

    ProjectState loadedSnapshot;
    REQUIRE (ProjectDocument::load (projectFile, loadedSnapshot).wasOk());
    REQUIRE (loadedSnapshot.projectId == projectId);
    REQUIRE (loadedSnapshot.tracks.size() == 1);
    REQUIRE (loadedSnapshot.tracks.front().id == trackId);
    REQUIRE (loadedSnapshot.tracks.front().clips.size() == 1);
    REQUIRE (loadedSnapshot.mediaSources.front().id == imported.clips.front().sourceId);
    REQUIRE (loadedSnapshot.mediaSources.front().file == testFile);
    REQUIRE (loadedSnapshot.mediaSources.front().assetFingerprint.isNotEmpty());
    REQUIRE (loadedSnapshot.undoManifest.hadUndo);
    REQUIRE (loadedSnapshot.undoManifest.undoDescription.isNotEmpty());
    REQUIRE (! loadedSnapshot.undoManifest.undoHistory.empty());

    ProjectModel restartUndoProject;
    restartUndoProject.replaceWithSnapshot (loadedSnapshot);
    REQUIRE (restartUndoProject.canUndo());
    REQUIRE (restartUndoProject.getAutomationLane (
        pluginAutomationLane).points.size() == 1);
    REQUIRE (restartUndoProject.undo());
    REQUIRE (restartUndoProject.getAutomationLane (
        pluginAutomationLane).points.empty());
    REQUIRE (restartUndoProject.canRedo());
    REQUIRE (restartUndoProject.redo());
    REQUIRE (restartUndoProject.getAutomationLane (
        pluginAutomationLane).points.size() == 1);

    const auto savedJson = juce::JSON::parse (projectFile.loadFileAsString());
    REQUIRE (savedJson.isObject());
    REQUIRE (static_cast<int> (savedJson.getProperty ("formatVersion", 0)) == 24);
    REQUIRE (static_cast<bool> (savedJson.getProperty (
        "lowLatencyMonitoring", false)));
    const auto* undoManifest = savedJson.getProperty (
        "undoManifest", juce::var()).getDynamicObject();
    REQUIRE (undoManifest != nullptr);
    REQUIRE (static_cast<bool> (undoManifest->getProperty ("hadUndo")));
    const auto* undoHistory = undoManifest->getProperty (
        "undoHistory").getArray();
    REQUIRE (undoHistory != nullptr);
    REQUIRE (! undoHistory->isEmpty());
    const auto savedTracksValue = savedJson.getProperty ("tracks", juce::var());
    const auto* savedTracks = savedTracksValue.getArray();
    REQUIRE (savedTracks != nullptr && savedTracks->size() == 1);
    REQUIRE (static_cast<int> (savedTracks->getFirst().getProperty (
        "manualLatencyOffsetSamples", 0)) == -128);
    REQUIRE (loadedSnapshot.lowLatencyMonitoring);
    REQUIRE (loadedSnapshot.inputMonitorMode == InputMonitorMode::whileArmed);
    REQUIRE (loadedSnapshot.manualRecordOffsetSamples == -96);
    REQUIRE (loadedSnapshot.recordingSettings.mode == RecordingMode::loop);
    REQUIRE (loadedSnapshot.recordingSettings.preRollBars == 2);
    REQUIRE (loadedSnapshot.recordingSettings.loopStartSamples == 24000);
    REQUIRE (loadedSnapshot.recordingSettings.loopEndSamples == 120000);
    REQUIRE (closeEnough (loadedSnapshot.recordingSettings.controlRoomGain,
                          0.72f));
    REQUIRE (loadedSnapshot.recordingSettings.controlRoomDimmed);
    REQUIRE (loadedSnapshot.tracks.front().inputStartChannel == 1);
    REQUIRE (loadedSnapshot.tracks.front().inputChannelCount == 1);
    REQUIRE (loadedSnapshot.tracks.front().recordingTapPoint
             == RecordingTapPoint::programOutput);
    REQUIRE (loadedSnapshot.tracks.front().recordOffsetSamples == 64);
    REQUIRE (loadedSnapshot.tracks.front().manualLatencyOffsetSamples == -128);
    REQUIRE (loadedSnapshot.tracks.front().clips.front().reversed);
    REQUIRE (loadedSnapshot.tracks.front().clips.front().id == clipId);
    REQUIRE (restored.resolveAudioPlayback (trackId).front().reversed);
    REQUIRE (loadedSnapshot.masterMuted);
    REQUIRE (loadedSnapshot.mixerChannels.size() == 2);
    REQUIRE (loadedSnapshot.tracks.front().outputDestinationId == restoredBus);
    REQUIRE (loadedSnapshot.tracks.front().sends.size() == 1);
    REQUIRE (closeEnough (loadedSnapshot.tracks.front().sends.front().gain, 0.33f));
    REQUIRE (loadedSnapshot.tracks.front().sends.front().preFader);
    REQUIRE (loadedSnapshot.metronomeEnabled);
    REQUIRE (closeEnough (loadedSnapshot.metronomeGain, 0.37f));
    REQUIRE (loadedSnapshot.tempoPoints.size() == 2);
    REQUIRE (loadedSnapshot.tempoPoints.front().curve == TempoCurve::linear);
    REQUIRE (loadedSnapshot.timeSignatures.size() == 2);
    REQUIRE (loadedSnapshot.timeSignatures.back().numerator == 3);
    REQUIRE (loadedSnapshot.markers.size() == 1);
    REQUIRE (loadedSnapshot.markers.front().name == "Verse");
    REQUIRE (loadedSnapshot.automationLanes.size() == 2);
    REQUIRE (loadedSnapshot.automationLanes.front().mode == AutomationMode::touch);
    REQUIRE (loadedSnapshot.automationLanes.front().points.size() == 2);
    REQUIRE (loadedSnapshot.automationLanes.front().points.back().curve
             == AutomationCurve::smooth);
    REQUIRE (loadedSnapshot.automationLanes.back().parameterId
             == "plugin:cutoff");
    REQUIRE (loadedSnapshot.automationLanes.back().points.size() == 1);
    REQUIRE (loadedSnapshot.producerSettings.rootPitchClass == 9);
    REQUIRE (loadedSnapshot.producerSettings.scale == "minor");
    REQUIRE (loadedSnapshot.producerSettings.style == "energetic");
    REQUIRE (loadedSnapshot.producerSettings.bars == 8);
    REQUIRE (loadedSnapshot.producerSettings.variation == 77);

    auto defaultProducerJson = savedJson.clone();
    auto* defaultProducerObject = defaultProducerJson.getDynamicObject();
    REQUIRE (defaultProducerObject != nullptr);
    defaultProducerObject->removeProperty ("producerRootPitchClass");
    defaultProducerObject->removeProperty ("producerScale");
    defaultProducerObject->removeProperty ("producerStyle");
    defaultProducerObject->removeProperty ("producerBars");
    defaultProducerObject->removeProperty ("producerVariation");
    auto defaultProducerFile = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-project-v18-default-producer", ".triumph", false);
    REQUIRE (defaultProducerFile.replaceWithText (
        juce::JSON::toString (defaultProducerJson, true)));
    ProjectState defaultProducerSnapshot;
    REQUIRE (ProjectDocument::load (
        defaultProducerFile, defaultProducerSnapshot).wasOk());
    REQUIRE (defaultProducerSnapshot.producerSettings.rootPitchClass == 0);
    REQUIRE (defaultProducerSnapshot.producerSettings.scale == "major");
    REQUIRE (defaultProducerSnapshot.producerSettings.style == "balanced");
    REQUIRE (defaultProducerSnapshot.producerSettings.bars == 4);
    REQUIRE (defaultProducerSnapshot.producerSettings.variation == 1);

    auto versionEighteenJson = savedJson.clone();
    auto* versionEighteenObject = versionEighteenJson.getDynamicObject();
    REQUIRE (versionEighteenObject != nullptr);
    versionEighteenObject->setProperty ("formatVersion", 18);
    versionEighteenObject->removeProperty ("inputMonitorMode");
    versionEighteenObject->removeProperty ("manualRecordOffsetSamples");
    auto versionEighteenFile = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-project-v18-monitor-defaults", ".triumph", false);
    REQUIRE (versionEighteenFile.replaceWithText (
        juce::JSON::toString (versionEighteenJson, true)));
    ProjectState versionEighteenSnapshot;
    REQUIRE (ProjectDocument::load (
        versionEighteenFile, versionEighteenSnapshot).wasOk());
    REQUIRE (versionEighteenSnapshot.inputMonitorMode == InputMonitorMode::off);
    REQUIRE (versionEighteenSnapshot.manualRecordOffsetSamples == 0);

    auto versionNineteenJson = savedJson.clone();
    auto* versionNineteenObject = versionNineteenJson.getDynamicObject();
    REQUIRE (versionNineteenObject != nullptr);
    versionNineteenObject->setProperty ("formatVersion", 19);
    for (const auto& property : {
             juce::Identifier ("recordingMode"),
             juce::Identifier ("preRollBars"),
             juce::Identifier ("punchStartSamples"),
             juce::Identifier ("punchEndSamples"),
             juce::Identifier ("loopStartSamples"),
             juce::Identifier ("loopEndSamples"),
             juce::Identifier ("controlRoomGain"),
             juce::Identifier ("controlRoomDimmed"),
             juce::Identifier ("controlRoomMuted") })
        versionNineteenObject->removeProperty (property);
    auto* versionNineteenTracks = versionNineteenJson.getProperty (
        "tracks", juce::var()).getArray();
    REQUIRE (versionNineteenTracks != nullptr);
    for (auto& trackValue : *versionNineteenTracks)
        if (auto* trackObject = trackValue.getDynamicObject())
        {
            trackObject->removeProperty ("inputStartChannel");
            trackObject->removeProperty ("inputChannelCount");
            trackObject->removeProperty ("recordingTapPoint");
            trackObject->removeProperty ("recordOffsetSamples");
        }
    auto versionNineteenFile = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-project-v19-recording-defaults", ".triumph", false);
    REQUIRE (versionNineteenFile.replaceWithText (
        juce::JSON::toString (versionNineteenJson, true)));
    ProjectState versionNineteenSnapshot;
    REQUIRE (ProjectDocument::load (
        versionNineteenFile, versionNineteenSnapshot).wasOk());
    REQUIRE (versionNineteenSnapshot.recordingSettings.mode
             == RecordingMode::normal);
    REQUIRE (versionNineteenSnapshot.recordingSettings.preRollBars == 0);
    REQUIRE (closeEnough (
        versionNineteenSnapshot.recordingSettings.controlRoomGain, 1.0f));
    REQUIRE (versionNineteenSnapshot.tracks.front().inputStartChannel == 0);
    REQUIRE (versionNineteenSnapshot.tracks.front().inputChannelCount == 2);
    REQUIRE (versionNineteenSnapshot.tracks.front().recordingTapPoint
             == RecordingTapPoint::hardwareInput);
    REQUIRE (versionNineteenSnapshot.tracks.front().recordOffsetSamples == 0);
    const auto mediaSourcesValue = savedJson.getProperty ("mediaSources", juce::var());
    const auto* mediaSources = mediaSourcesValue.getArray();
    REQUIRE (mediaSources != nullptr && mediaSources->size() == 1);
    REQUIRE (static_cast<bool> (mediaSources->getFirst().getProperty ("pathIsRelative", false)));
    REQUIRE (mediaSources->getFirst().getProperty (
        "assetFingerprint", juce::var()).toString().isNotEmpty());

    auto legacyJson = savedJson.clone();
    legacyJson.getDynamicObject()->setProperty ("formatVersion", 3);
    const auto legacySourcesValue = legacyJson.getProperty ("mediaSources", juce::var());
    auto* legacySources = legacySourcesValue.getArray();
    REQUIRE (legacySources != nullptr && legacySources->size() == 1);
    legacySources->getReference (0).getDynamicObject()->setProperty ("sampleRate", 44100.0);
    legacySources->getReference (0).getDynamicObject()->setProperty (
        "lengthInSamples", static_cast<juce::int64> (44100));
    const auto legacyTracksValue = legacyJson.getProperty ("tracks", juce::var());
    auto* legacyTracks = legacyTracksValue.getArray();
    REQUIRE (legacyTracks != nullptr && legacyTracks->size() == 1);
    const auto legacyClipsValue = legacyTracks->getReference (0).getProperty (
        "clips", juce::var());
    auto* legacyClips = legacyClipsValue.getArray();
    REQUIRE (legacyClips != nullptr && legacyClips->size() == 1);
    legacyClips->getReference (0).getDynamicObject()->setProperty (
        "lengthInSamples", static_cast<juce::int64> (44100));

    auto legacyFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                          .getNonexistentChildFile ("triumph-project-v3-test",
                                                    ".triumph",
                                                    false);
    REQUIRE (legacyFile.replaceWithText (juce::JSON::toString (legacyJson, true)));
    ProjectState migratedLegacy;
    REQUIRE (ProjectDocument::load (legacyFile, migratedLegacy).wasOk());
    REQUIRE (migratedLegacy.tracks.front().clips.front().lengthInSamples == 48000);

    auto renderSource = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getNonexistentChildFile ("triumph-render-source", ".wav", false);
    {
        std::unique_ptr<juce::OutputStream> stream (
            renderSource.createOutputStream());
        REQUIRE (stream != nullptr);
        juce::WavAudioFormat wav;
        auto sourceWriterOptions = juce::AudioFormatWriterOptions {}
            .withSampleRate (48000.0).withNumChannels (1).withBitsPerSample (24);
        auto writer = wav.createWriterFor (stream, sourceWriterOptions);
        REQUIRE (writer != nullptr);
        juce::AudioBuffer<float> tone (1, 48000);
        for (int sample = 0; sample < tone.getNumSamples(); ++sample)
            tone.setSample (0, sample, 0.2f * std::sin (
                6.283185307179586 * 440.0 * sample / 48000.0));
        REQUIRE (writer->writeFromAudioSampleBuffer (tone, 0, tone.getNumSamples()));
    }
    ProjectModel renderProject;
    REQUIRE (renderProject.addAudioTrack (renderSource, 48000.0, 1, 48000)
                 .isNotEmpty());

    ProjectModel frozenRenderProject;
    const auto frozenRenderTrack = frozenRenderProject.addInstrumentTrack (
        "Frozen Instrument");
    frozenRenderProject.setInstrumentPlugin (
        frozenRenderTrack, "Test VST3", "<PLUGIN name=\"Test VST3\"/>", {});
    auto blockedExternalFile = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-live-plugin-export", ".wav", false);
    std::atomic<bool> pluginRenderCancel { false };
    std::atomic<float> pluginRenderProgress { 0.0f };
    OfflineRenderer::Options pluginRenderOptions;
    pluginRenderOptions.tailSeconds = 0.0;
    REQUIRE (OfflineRenderer::renderStereoWav (
        frozenRenderProject.createSnapshot(), blockedExternalFile,
        pluginRenderOptions, pluginRenderCancel, pluginRenderProgress).failed());
    REQUIRE (! blockedExternalFile.existsAsFile());
    REQUIRE (frozenRenderProject.freezeInstrumentTrack (
        frozenRenderTrack, renderSource, 48000.0, 1, 48000));
    auto frozenExternalFile = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-frozen-plugin-export", ".wav", false);
    REQUIRE (OfflineRenderer::renderStereoWav (
        frozenRenderProject.createSnapshot(), frozenExternalFile,
        pluginRenderOptions, pluginRenderCancel, pluginRenderProgress).wasOk());
    REQUIRE (frozenExternalFile.existsAsFile());

    auto renderedFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getNonexistentChildFile ("triumph-rendered-mix", ".wav", false);
    std::atomic<bool> cancelRender { false };
    std::atomic<float> renderProgress { 0.0f };
    OfflineRenderer::Options renderOptions;
    renderOptions.tailSeconds = 0.1;
    REQUIRE (OfflineRenderer::renderStereoWav (
        renderProject.createSnapshot(), renderedFile, renderOptions,
        cancelRender, renderProgress).wasOk());
    juce::AudioFormatManager renderFormats;
    renderFormats.registerBasicFormats();
    auto renderedReader = std::unique_ptr<juce::AudioFormatReader> (
        renderFormats.createReaderFor (renderedFile));
    REQUIRE (renderedReader != nullptr);
    REQUIRE (renderedReader->numChannels == 2);
    REQUIRE (renderedReader->sampleRate == 48000.0);
    REQUIRE (renderedReader->bitsPerSample == 24);
    REQUIRE (renderedReader->lengthInSamples == 52800);
    REQUIRE (renderProgress.load() == 1.0f);

    auto renderedCdFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                              .getNonexistentChildFile (
                                  "triumph-rendered-cd-mix", ".wav", false);
    renderOptions.sampleRate = 44100.0;
    renderOptions.bitsPerSample = 16;
    renderProgress.store (0.0f);
    REQUIRE (OfflineRenderer::renderStereoWav (
        renderProject.createSnapshot(), renderedCdFile, renderOptions,
        cancelRender, renderProgress).wasOk());
    auto renderedCdReader = std::unique_ptr<juce::AudioFormatReader> (
        renderFormats.createReaderFor (renderedCdFile));
    REQUIRE (renderedCdReader != nullptr);
    REQUIRE (renderedCdReader->numChannels == 2);
    REQUIRE (renderedCdReader->sampleRate == 44100.0);
    REQUIRE (renderedCdReader->bitsPerSample == 16);
    REQUIRE (renderedCdReader->lengthInSamples == 48510);
    REQUIRE (renderProgress.load() == 1.0f);

    auto renderedRangeFile = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-rendered-range", ".wav", false);
    renderOptions.sampleRate = 48000.0;
    renderOptions.bitsPerSample = 24;
    renderOptions.tailSeconds = 0.0;
    renderOptions.timelineStartSamples = 12000;
    renderOptions.timelineEndSamples = 36000;
    renderProgress.store (0.0f);
    REQUIRE (OfflineRenderer::renderStereoWav (
        renderProject.createSnapshot(), renderedRangeFile, renderOptions,
        cancelRender, renderProgress).wasOk());
    auto renderedRangeReader = std::unique_ptr<juce::AudioFormatReader> (
        renderFormats.createReaderFor (renderedRangeFile));
    REQUIRE (renderedRangeReader != nullptr);
    REQUIRE (renderedRangeReader->lengthInSamples == 24000);

    auto fadeRenderSource = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-fade-render-source", ".wav", false);
    {
        std::unique_ptr<juce::OutputStream> stream (
            fadeRenderSource.createOutputStream());
        REQUIRE (stream != nullptr);
        juce::WavAudioFormat wav;
        auto writer = wav.createWriterFor (
            stream, juce::AudioFormatWriterOptions {}
                        .withSampleRate (48000.0)
                        .withNumChannels (1)
                        .withBitsPerSample (24));
        REQUIRE (writer != nullptr);
        juce::AudioBuffer<float> constantSignal (1, 48000);
        for (int sample = 0; sample < constantSignal.getNumSamples(); ++sample)
            constantSignal.setSample (0, sample, 0.25f);
        REQUIRE (writer->writeFromAudioSampleBuffer (
            constantSignal, 0, constantSignal.getNumSamples()));
    }
    ProjectModel fadeRenderProject;
    const auto fadeRenderTrack = fadeRenderProject.addAudioTrack (
        fadeRenderSource, 48000.0, 1, 48000);
    const auto fadeRenderClip = fadeRenderProject.getTrackState (
        fadeRenderTrack).clips.front().id;
    REQUIRE (fadeRenderProject.setClipFades (
        fadeRenderTrack, fadeRenderClip, 12000, 12000));
    auto fadeRenderedFile = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-fade-rendered", ".wav", false);
    OfflineRenderer::Options fadeRenderOptions;
    fadeRenderOptions.sampleRate = 48000.0;
    fadeRenderOptions.bitsPerSample = 24;
    fadeRenderOptions.tailSeconds = 0.0;
    renderProgress.store (0.0f);
    REQUIRE (OfflineRenderer::renderStereoWav (
        fadeRenderProject.createSnapshot(), fadeRenderedFile,
        fadeRenderOptions, cancelRender, renderProgress).wasOk());
    auto fadeRenderedReader = std::unique_ptr<juce::AudioFormatReader> (
        renderFormats.createReaderFor (fadeRenderedFile));
    REQUIRE (fadeRenderedReader != nullptr);
    REQUIRE (fadeRenderedReader->lengthInSamples == 48000);
    juce::AudioBuffer<float> fadeRenderedBuffer (2, 48000);
    REQUIRE (fadeRenderedReader->read (&fadeRenderedBuffer, 0, 48000,
                                       0, true, true));
    const auto fullLevel = std::abs (fadeRenderedBuffer.getSample (0, 12000));
    const auto halfFadeLevel = std::abs (fadeRenderedBuffer.getSample (0, 6000));
    REQUIRE (fullLevel > 0.05f);
    REQUIRE (std::abs (fadeRenderedBuffer.getSample (0, 0)) < 0.0001f);
    REQUIRE (halfFadeLevel > fullLevel * 0.68f);
    REQUIRE (halfFadeLevel < fullLevel * 0.73f);
    REQUIRE (std::abs (fadeRenderedBuffer.getSample (0, 47999))
             < fullLevel * 0.02f);

    auto invalidIsolationFile = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-invalid-isolation", ".wav", false);
    renderOptions.timelineStartSamples = 0;
    renderOptions.timelineEndSamples = -1;
    renderOptions.isolatedTrackId = "missing-track";
    const auto invalidIsolationResult = OfflineRenderer::renderStereoWav (
        renderProject.createSnapshot(), invalidIsolationFile, renderOptions,
        cancelRender, renderProgress);
    REQUIRE (invalidIsolationResult.failed());
    REQUIRE (! invalidIsolationFile.existsAsFile());
    renderOptions.isolatedTrackId.clear();

    const auto stemDirectory = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-stem-delivery", juce::String {}, false);
    StemExporter::Options stemOptions;
    stemOptions.projectName = "Stem Test";
    stemOptions.render.tailSeconds = 0.0;
    renderProgress.store (0.0f);
    REQUIRE (StemExporter::renderTrackStems (
        renderProject.createSnapshot(), stemDirectory, stemOptions,
        cancelRender, renderProgress).wasOk());
    REQUIRE (stemDirectory.isDirectory());
    const auto renderTrack = renderProject.createSnapshot().tracks.front();
    const auto expectedStem = stemDirectory.getChildFile (
        "01 - " + StemExporter::legalStemName (renderTrack.name) + ".wav");
    REQUIRE (expectedStem.existsAsFile());
    auto stemReader = std::unique_ptr<juce::AudioFormatReader> (
        renderFormats.createReaderFor (expectedStem));
    REQUIRE (stemReader != nullptr);
    REQUIRE (stemReader->lengthInSamples == 48000);
    const auto manifestFile = stemDirectory.getChildFile ("manifest.json");
    REQUIRE (manifestFile.existsAsFile());
    const auto manifest = juce::JSON::parse (manifestFile.loadFileAsString());
    REQUIRE (manifest.isObject());
    REQUIRE (manifest.getProperty ("projectName", juce::var()) == "Stem Test");
    const auto manifestTracksValue = manifest.getProperty ("tracks", juce::var());
    const auto* manifestTracks = manifestTracksValue.getArray();
    REQUIRE (manifestTracks != nullptr && manifestTracks->size() == 1);
    REQUIRE (renderProgress.load() == 1.0f);

    std::atomic<float> protectedStemProgress { 0.0f };
    const auto protectedStemResult = StemExporter::renderTrackStems (
        renderProject.createSnapshot(), stemDirectory, stemOptions,
        cancelRender, protectedStemProgress);
    REQUIRE (protectedStemResult.failed());
    REQUIRE (expectedStem.existsAsFile());

    const auto cancelledStemDirectory = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-cancelled-stems", juce::String {}, false);
    const auto partialStemPattern = "." + cancelledStemDirectory.getFileName()
                                    + ".partial-*";
    const auto partialStemCountBefore = cancelledStemDirectory.getParentDirectory()
        .findChildFiles (juce::File::findDirectories, false, partialStemPattern).size();
    cancelRender.store (true);
    const auto cancelledStemResult = StemExporter::renderTrackStems (
        renderProject.createSnapshot(), cancelledStemDirectory, stemOptions,
        cancelRender, protectedStemProgress);
    REQUIRE (cancelledStemResult.failed());
    REQUIRE (! cancelledStemDirectory.exists());
    REQUIRE (cancelledStemDirectory.getParentDirectory().findChildFiles (
        juce::File::findDirectories, false, partialStemPattern).size()
             == partialStemCountBefore);
    cancelRender.store (false);

    auto cancelledFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                             .getNonexistentChildFile (
                                 "triumph-cancelled-mix", ".wav", false);
    REQUIRE (cancelledFile.replaceWithText ("preserve-existing-destination"));
    const auto cancelledPartial = cancelledFile.getSiblingFile (
        cancelledFile.getFileName() + ".partial");
    cancelRender.store (true);
    renderProgress.store (0.0f);
    const auto cancelResult = OfflineRenderer::renderStereoWav (
        renderProject.createSnapshot(), cancelledFile, renderOptions,
        cancelRender, renderProgress);
    REQUIRE (cancelResult.failed());
    REQUIRE (cancelResult.getErrorMessage().containsIgnoreCase ("cancel"));
    REQUIRE (cancelledFile.loadFileAsString() == "preserve-existing-destination");
    REQUIRE (! cancelledPartial.existsAsFile());

    ProjectModel professionalGraphProject;
    const auto graphTrackId = professionalGraphProject.addInstrumentTrack (
        "MPE Keys");
    professionalGraphProject.setInstrumentPlugin (
        graphTrackId, "Test Instrument", "<PLUGIN name=\"Test Instrument\" />",
        "instrument-state");
    PluginSlotState insertSlot;
    insertSlot.stablePluginId = "vendor.stereo-delay";
    insertSlot.name = "Stereo Delay";
    insertSlot.descriptionXml = "<PLUGIN name=\"Stereo Delay\" />";
    insertSlot.stateBase64 = "delay-state";
    insertSlot.role = PluginSlotRole::insertEffect;
    insertSlot.order = 1;
    insertSlot.layout = { 2, 2, 2, true };
    insertSlot.latencySamples = 96;
    const auto insertSlotId = professionalGraphProject.addPluginSlot (
        graphTrackId, insertSlot);
    REQUIRE (insertSlotId.isNotEmpty());
    REQUIRE (professionalGraphProject.getPluginSlots (
        graphTrackId).size() == 2);

    const auto graphBusId = professionalGraphProject.addMixerChannel (
        MixerChannelKind::bus, "Music Bus");
    PluginSlotState busSlot;
    busSlot.stablePluginId = "vendor.bus-compressor";
    busSlot.name = "Bus Compressor";
    busSlot.descriptionXml = "<PLUGIN name=\"Bus Compressor\" />";
    busSlot.order = 0;
    busSlot.layout = { 2, 2, 2, true };
    REQUIRE (professionalGraphProject.addPluginSlot (
        graphBusId, busSlot).isNotEmpty());

    SyncSettingsState syncSettings;
    syncSettings.source = SyncSourceState::midiClock;
    syncSettings.sendMidiClock = true;
    syncSettings.followExternalStartStop = true;
    syncSettings.jitterToleranceSamples = 384;
    professionalGraphProject.setSyncSettings (syncSettings, false);
    professionalGraphProject.setMonitorPaths ({
        { "control", "Control Room", MonitorPathRole::controlRoom,
          "master", 0, 2, 0.9f, false, false },
        { "cue", "Cue A", MonitorPathRole::cue,
          graphBusId, 2, 2, 1.0f, false, false },
        { "talk", "Talkback", MonitorPathRole::talkback,
          graphTrackId, 4, 1, 0.5f, false, true }
    }, false);

    const auto graphTrack = professionalGraphProject.getTrackState (
        graphTrackId);
    REQUIRE (! graphTrack.midiClips.empty());
    const auto graphClipId = graphTrack.midiClips.front().id;
    const auto expressiveNoteId = professionalGraphProject.addMidiNote (
        graphTrackId, graphClipId, 1.0, 2.0, 64, 0.8f);
    REQUIRE (professionalGraphProject.setMidiNoteExpression (
        graphTrackId, graphClipId, expressiveNoteId,
        { { "pitch", 0.25, 0.5f,
            MidiNoteState::ExpressionPoint::Type::pitch },
          { "pressure", 0.5, 0.75f,
            MidiNoteState::ExpressionPoint::Type::pressure } }));
    REQUIRE (professionalGraphProject.addMidiControllerEvent (
        graphTrackId, graphClipId,
        { "mod", 1.5, 1, 1, 0x80000000u, 0 }).isNotEmpty());

    const auto professionalGraphFile = juce::File::getSpecialLocation (
        juce::File::tempDirectory).getNonexistentChildFile (
            "triumph-professional-graph", ".triumph", false);
    REQUIRE (ProjectDocument::save (
        professionalGraphFile, professionalGraphProject).wasOk());
    ProjectState professionalGraphLoaded;
    REQUIRE (ProjectDocument::load (
        professionalGraphFile, professionalGraphLoaded).wasOk());
    REQUIRE (professionalGraphLoaded.syncSettings.source
             == SyncSourceState::midiClock);
    REQUIRE (professionalGraphLoaded.syncSettings.sendMidiClock);
    REQUIRE (professionalGraphLoaded.monitorPaths.size() == 3);
    const auto loadedGraphTrack = std::find_if (
        professionalGraphLoaded.tracks.begin(),
        professionalGraphLoaded.tracks.end(),
        [&graphTrackId] (const auto& candidate)
    {
        return candidate.id == graphTrackId;
    });
    REQUIRE (loadedGraphTrack != professionalGraphLoaded.tracks.end());
    REQUIRE (loadedGraphTrack->pluginSlots.size() == 2);
    REQUIRE (loadedGraphTrack->pluginSlots[1].stablePluginId
             == "vendor.stereo-delay");
    REQUIRE (loadedGraphTrack->pluginSlots[1].layout.sidechainChannels == 2);
    REQUIRE (loadedGraphTrack->midiClips.front().controllers.size() == 1);
    REQUIRE (loadedGraphTrack->midiClips.front().notes.back()
                 .expression.size() == 2);
    REQUIRE (professionalGraphLoaded.mixerChannels.front()
                 .pluginSlots.size() == 1);

    REQUIRE (testFile.deleteFile());
    REQUIRE (projectFile.deleteFile());
    REQUIRE (defaultProducerFile.deleteFile());
    REQUIRE (versionEighteenFile.deleteFile());
    REQUIRE (versionNineteenFile.deleteFile());
    REQUIRE (legacyFile.deleteFile());
    REQUIRE (recordingProjectFile.deleteFile());
    REQUIRE (midiProjectFile.deleteFile());
    REQUIRE (missingFreezeProjectFile.deleteFile());
    REQUIRE (renderSource.deleteFile());
    REQUIRE (frozenExternalFile.deleteFile());
    REQUIRE (renderedFile.deleteFile());
    REQUIRE (renderedCdFile.deleteFile());
    REQUIRE (renderedRangeFile.deleteFile());
    REQUIRE (stemDirectory.deleteRecursively());
    REQUIRE (cancelledFile.deleteFile());
    REQUIRE (stretchProjectFile.deleteFile());
    REQUIRE (warpProjectFile.deleteFile());
    REQUIRE (professionalGraphFile.deleteFile());
    return 0;
}
