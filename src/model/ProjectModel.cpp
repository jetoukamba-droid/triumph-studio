#include "ProjectModel.h"

#include "core/TimelineMath.h"
#include "core/TempoMap.h"
#include "core/TimeStretch.h"
#include "core/WarpMap.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace triumph
{
namespace modelIds
{
const juce::Identifier project { "TRIUMPH_PROJECT" };
const juce::Identifier mediaSource { "MEDIA_SOURCE" };
const juce::Identifier track { "TRACK" };
const juce::Identifier clip { "AUDIO_CLIP" };
const juce::Identifier clipVariant { "CLIP_VARIANT" };
const juce::Identifier compRegion { "COMP_REGION" };
const juce::Identifier warpMarker { "WARP_MARKER" };
const juce::Identifier transientMarker { "TRANSIENT_MARKER" };
const juce::Identifier midiClip { "MIDI_CLIP" };
const juce::Identifier midiNote { "MIDI_NOTE" };
const juce::Identifier midiController { "MIDI_CONTROLLER" };
const juce::Identifier midiExpression { "MIDI_EXPRESSION" };
const juce::Identifier tempoPoint { "TEMPO_POINT" };
const juce::Identifier timeSignaturePoint { "TIME_SIGNATURE_POINT" };
const juce::Identifier projectMarker { "PROJECT_MARKER" };
const juce::Identifier automationLane { "AUTOMATION_LANE" };
const juce::Identifier automationPoint { "AUTOMATION_POINT" };
const juce::Identifier mixerChannel { "MIXER_CHANNEL" };
const juce::Identifier mixerSend { "MIXER_SEND" };
const juce::Identifier pluginSlot { "PLUGIN_SLOT" };
const juce::Identifier monitorPath { "MONITOR_PATH" };

const juce::Identifier id { "id" };
const juce::Identifier projectId { "projectId" };
const juce::Identifier name { "name" };
const juce::Identifier path { "path" };
const juce::Identifier sampleRate { "sampleRate" };
const juce::Identifier channels { "channels" };
const juce::Identifier lengthInSamples { "lengthInSamples" };
const juce::Identifier assetFingerprint { "assetFingerprint" };
const juce::Identifier sourceId { "sourceId" };
const juce::Identifier timelineStartSamples { "timelineStartSamples" };
const juce::Identifier sourceOffsetSamples { "sourceOffsetSamples" };
const juce::Identifier fadeInSamples { "fadeInSamples" };
const juce::Identifier fadeOutSamples { "fadeOutSamples" };
const juce::Identifier gain { "gain" };
const juce::Identifier locked { "locked" };
const juce::Identifier takeGroupId { "takeGroupId" };
const juce::Identifier takeLaneIndex { "takeLaneIndex" };
const juce::Identifier activeTake { "activeTake" };
const juce::Identifier timeStretchRatio { "timeStretchRatio" };
const juce::Identifier timeStretchProvider { "timeStretchProvider" };
const juce::Identifier reversed { "reversed" };
const juce::Identifier timelineOffsetSamples { "timelineOffsetSamples" };
const juce::Identifier strength { "strength" };
const juce::Identifier crossfadeSamples { "crossfadeSamples" };
const juce::Identifier pan { "pan" };
const juce::Identifier muted { "muted" };
const juce::Identifier solo { "solo" };
const juce::Identifier recordArmed { "recordArmed" };
const juce::Identifier inputStartChannel { "inputStartChannel" };
const juce::Identifier inputChannelCount { "inputChannelCount" };
const juce::Identifier recordingTapPoint { "recordingTapPoint" };
const juce::Identifier recordOffsetSamples { "recordOffsetSamples" };
const juce::Identifier colour { "colour" };
const juce::Identifier masterGain { "masterGain" };
const juce::Identifier masterMuted { "masterMuted" };
const juce::Identifier timelineSampleRate { "timelineSampleRate" };
const juce::Identifier isInstrument { "isInstrument" };
const juce::Identifier pluginName { "pluginName" };
const juce::Identifier pluginDescriptionXml { "pluginDescriptionXml" };
const juce::Identifier pluginStateBase64 { "pluginStateBase64" };
const juce::Identifier pluginBypassed { "pluginBypassed" };
const juce::Identifier pluginLatencySamples { "pluginLatencySamples" };
const juce::Identifier pluginAvailable { "pluginAvailable" };
const juce::Identifier manualLatencyOffsetSamples { "manualLatencyOffsetSamples" };
const juce::Identifier frozen { "frozen" };
const juce::Identifier freezeSourceId { "freezeSourceId" };
const juce::Identifier startBeat { "startBeat" };
const juce::Identifier lengthBeats { "lengthBeats" };
const juce::Identifier noteNumber { "noteNumber" };
const juce::Identifier velocity { "velocity" };
const juce::Identifier channel { "channel" };
const juce::Identifier noteId { "noteId" };
const juce::Identifier offsetBeats { "offsetBeats" };
const juce::Identifier value { "value" };
const juce::Identifier controller { "controller" };
const juce::Identifier beat { "beat" };
const juce::Identifier bpm { "bpm" };
const juce::Identifier curve { "curve" };
const juce::Identifier numerator { "numerator" };
const juce::Identifier denominator { "denominator" };
const juce::Identifier samplePosition { "samplePosition" };
const juce::Identifier targetId { "targetId" };
const juce::Identifier parameterId { "parameterId" };
const juce::Identifier mode { "mode" };
const juce::Identifier timeSignatureNumerator { "timeSignatureNumerator" };
const juce::Identifier timeSignatureDenominator { "timeSignatureDenominator" };
const juce::Identifier metronomeEnabled { "metronomeEnabled" };
const juce::Identifier metronomeGain { "metronomeGain" };
const juce::Identifier lowLatencyMonitoring { "lowLatencyMonitoring" };
const juce::Identifier inputMonitorMode { "inputMonitorMode" };
const juce::Identifier manualRecordOffsetSamples { "manualRecordOffsetSamples" };
const juce::Identifier recordingMode { "recordingMode" };
const juce::Identifier preRollBars { "preRollBars" };
const juce::Identifier punchStartSamples { "punchStartSamples" };
const juce::Identifier punchEndSamples { "punchEndSamples" };
const juce::Identifier loopStartSamples { "loopStartSamples" };
const juce::Identifier loopEndSamples { "loopEndSamples" };
const juce::Identifier controlRoomGain { "controlRoomGain" };
const juce::Identifier controlRoomDimmed { "controlRoomDimmed" };
const juce::Identifier controlRoomMuted { "controlRoomMuted" };
const juce::Identifier producerRootPitchClass { "producerRootPitchClass" };
const juce::Identifier producerScale { "producerScale" };
const juce::Identifier producerStyle { "producerStyle" };
const juce::Identifier producerBars { "producerBars" };
const juce::Identifier producerVariation { "producerVariation" };
const juce::Identifier kind { "kind" };
const juce::Identifier outputDestinationId { "outputDestinationId" };
const juce::Identifier destinationId { "destinationId" };
const juce::Identifier enabled { "enabled" };
const juce::Identifier preFader { "preFader" };
const juce::Identifier sidechain { "sidechain" };
const juce::Identifier stablePluginId { "stablePluginId" };
const juce::Identifier descriptionXml { "descriptionXml" };
const juce::Identifier stateBase64 { "stateBase64" };
const juce::Identifier role { "role" };
const juce::Identifier isolation { "isolation" };
const juce::Identifier order { "order" };
const juce::Identifier inputChannels { "inputChannels" };
const juce::Identifier outputChannels { "outputChannels" };
const juce::Identifier sidechainChannels { "sidechainChannels" };
const juce::Identifier explicitLayout { "explicitLayout" };
const juce::Identifier available { "available" };
const juce::Identifier latencySamples { "latencySamples" };
const juce::Identifier restartCount { "restartCount" };
const juce::Identifier syncSource { "syncSource" };
const juce::Identifier sendMidiClock { "sendMidiClock" };
const juce::Identifier followExternalStartStop { "followExternalStartStop" };
const juce::Identifier mtcFramesPerSecond { "mtcFramesPerSecond" };
const juce::Identifier jitterToleranceSamples { "jitterToleranceSamples" };
const juce::Identifier outputStartChannel { "outputStartChannel" };
const juce::Identifier outputChannelCount { "outputChannelCount" };
const juce::Identifier dimmed { "dimmed" };
}

namespace
{
juce::String makeAssetFingerprint (const juce::File& file,
                                   double sampleRate,
                                   int channels,
                                   juce::int64 lengthInSamples)
{
    juce::StringArray parts;
    parts.add (file.getFileName());
    parts.add (juce::String (sampleRate, 2));
    parts.add (juce::String (channels));
    parts.add (juce::String (lengthInSamples));
    parts.add (juce::String (file.existsAsFile() ? file.getSize() : 0));
    parts.add (juce::String (file.existsAsFile()
        ? file.getLastModificationTime().toMilliseconds() : 0));
    return parts.joinIntoString ("|");
}

std::vector<warp::Anchor> anchorsForClip (const AudioClipState& clip,
                                          const MediaSourceState& source,
                                          double timelineRate)
{
    const auto nativeSpan = static_cast<juce::int64> (timeline::convertSampleCount (
        stretch::timelineToNativeSamples (clip.lengthInSamples,
                                          clip.timeStretchRatio),
        timelineRate, source.sampleRate));
    std::vector<warp::Anchor> anchors {
        { 0, clip.sourceOffsetSamples },
        { clip.lengthInSamples, clip.sourceOffsetSamples + nativeSpan }
    };
    for (const auto& marker : clip.warpMarkers)
        if (marker.timelineOffsetSamples > 0
            && marker.timelineOffsetSamples < clip.lengthInSamples)
            anchors.push_back ({ marker.timelineOffsetSamples,
                                 marker.sourceOffsetSamples });
    return warp::normalise (std::move (anchors));
}

juce::ValueTree markerNodeFromState (const WarpMarkerState& marker,
                                     juce::int64 clipLength)
{
    juce::ValueTree node (modelIds::warpMarker);
    node.setProperty (modelIds::id,
                      marker.id.isNotEmpty() ? marker.id : juce::Uuid().toString(),
                      nullptr);
    node.setProperty (modelIds::timelineOffsetSamples,
                      juce::jlimit (static_cast<juce::int64> (1),
                                    juce::jmax (static_cast<juce::int64> (1),
                                                clipLength - 1),
                                    marker.timelineOffsetSamples),
                      nullptr);
    node.setProperty (modelIds::sourceOffsetSamples,
                      juce::jmax (static_cast<juce::int64> (0),
                                  marker.sourceOffsetSamples),
                      nullptr);
    return node;
}

juce::ValueTree markerNodeFromState (const TransientMarkerState& marker,
                                     juce::int64 clipLength)
{
    juce::ValueTree node (modelIds::transientMarker);
    node.setProperty (modelIds::id,
                      marker.id.isNotEmpty() ? marker.id : juce::Uuid().toString(),
                      nullptr);
    node.setProperty (modelIds::timelineOffsetSamples,
                      juce::jlimit (static_cast<juce::int64> (1),
                                    juce::jmax (static_cast<juce::int64> (1),
                                                clipLength - 1),
                                    marker.timelineOffsetSamples),
                      nullptr);
    node.setProperty (modelIds::sourceOffsetSamples,
                      juce::jmax (static_cast<juce::int64> (0),
                                  marker.sourceOffsetSamples),
                      nullptr);
    node.setProperty (modelIds::strength,
                      juce::jlimit (0.0f, 1.0f, marker.strength), nullptr);
    return node;
}

ClipVariantState variantFromNode (const juce::ValueTree& node)
{
    ClipVariantState result;
    result.id = node.getProperty (modelIds::id).toString();
    result.name = node.getProperty (modelIds::name).toString();
    result.sourceId = node.getProperty (modelIds::sourceId).toString();
    result.sourceOffsetSamples = static_cast<juce::int64> (
        node.getProperty (modelIds::sourceOffsetSamples, 0));
    result.lengthInSamples = juce::jmax (
        static_cast<juce::int64> (1),
        static_cast<juce::int64> (node.getProperty (
            modelIds::lengthInSamples, 1)));
    result.fadeInSamples = juce::jlimit (
        static_cast<juce::int64> (0), result.lengthInSamples,
        static_cast<juce::int64> (node.getProperty (
            modelIds::fadeInSamples, 0)));
    result.fadeOutSamples = juce::jlimit (
        static_cast<juce::int64> (0), result.lengthInSamples,
        static_cast<juce::int64> (node.getProperty (
            modelIds::fadeOutSamples, 0)));
    result.gain = juce::jmax (
        0.0f, static_cast<float> (node.getProperty (modelIds::gain, 1.0)));
    result.timeStretchRatio = stretch::clampRatio (static_cast<double> (
        node.getProperty (modelIds::timeStretchRatio, 1.0)));
    result.timeStretchProvider = node.getProperty (
        modelIds::timeStretchProvider, stretch::builtInProviderId).toString();
    result.reversed = static_cast<bool> (
        node.getProperty (modelIds::reversed, false));
    for (const auto markerNode : node)
        if (markerNode.hasType (modelIds::warpMarker))
            result.warpMarkers.push_back ({
                markerNode.getProperty (modelIds::id).toString(),
                static_cast<juce::int64> (markerNode.getProperty (
                    modelIds::timelineOffsetSamples, 0)),
                static_cast<juce::int64> (markerNode.getProperty (
                    modelIds::sourceOffsetSamples, 0)) });
        else if (markerNode.hasType (modelIds::transientMarker))
            result.transientMarkers.push_back ({
                markerNode.getProperty (modelIds::id).toString(),
                static_cast<juce::int64> (markerNode.getProperty (
                    modelIds::timelineOffsetSamples, 0)),
                static_cast<juce::int64> (markerNode.getProperty (
                    modelIds::sourceOffsetSamples, 0)),
                static_cast<float> (markerNode.getProperty (
                    modelIds::strength, 0.0)) });
    return result;
}

juce::ValueTree variantNodeFromState (const ClipVariantState& variant)
{
    juce::ValueTree node (modelIds::clipVariant);
    const auto length = juce::jmax (static_cast<juce::int64> (1),
                                   variant.lengthInSamples);
    node.setProperty (modelIds::id,
                      variant.id.isNotEmpty() ? variant.id : juce::Uuid().toString(),
                      nullptr);
    node.setProperty (modelIds::name,
                      variant.name.trim().isNotEmpty()
                          ? variant.name.trim().substring (0, 96)
                          : "Variant",
                      nullptr);
    node.setProperty (modelIds::sourceId, variant.sourceId, nullptr);
    node.setProperty (modelIds::sourceOffsetSamples,
                      juce::jmax (static_cast<juce::int64> (0),
                                  variant.sourceOffsetSamples),
                      nullptr);
    node.setProperty (modelIds::lengthInSamples, length, nullptr);
    node.setProperty (modelIds::fadeInSamples,
                      juce::jlimit (static_cast<juce::int64> (0), length,
                                    variant.fadeInSamples),
                      nullptr);
    node.setProperty (modelIds::fadeOutSamples,
                      juce::jlimit (static_cast<juce::int64> (0), length,
                                    variant.fadeOutSamples),
                      nullptr);
    node.setProperty (modelIds::gain, juce::jmax (0.0f, variant.gain), nullptr);
    node.setProperty (modelIds::timeStretchRatio,
                      stretch::clampRatio (variant.timeStretchRatio), nullptr);
    node.setProperty (modelIds::timeStretchProvider,
                      variant.timeStretchProvider.isNotEmpty()
                          ? variant.timeStretchProvider
                          : stretch::builtInProviderId,
                      nullptr);
    node.setProperty (modelIds::reversed, variant.reversed, nullptr);
    for (const auto& marker : variant.warpMarkers)
        if (marker.timelineOffsetSamples > 0
            && marker.timelineOffsetSamples < length)
            node.addChild (markerNodeFromState (marker, length), -1, nullptr);
    for (const auto& marker : variant.transientMarkers)
        if (marker.timelineOffsetSamples > 0
            && marker.timelineOffsetSamples < length)
            node.addChild (markerNodeFromState (marker, length), -1, nullptr);
    return node;
}

juce::ValueTree variantNodeFromClip (const AudioClipState& clip,
                                     juce::String variantId,
                                     juce::String variantName)
{
    ClipVariantState variant;
    variant.id = variantId.isNotEmpty() ? variantId : juce::Uuid().toString();
    variant.name = variantName.trim().isNotEmpty()
        ? variantName.trim().substring (0, 96)
        : "Variant " + juce::String (clip.variants.size() + 1);
    variant.sourceId = clip.sourceId;
    variant.sourceOffsetSamples = clip.sourceOffsetSamples;
    variant.lengthInSamples = clip.lengthInSamples;
    variant.fadeInSamples = clip.fadeInSamples;
    variant.fadeOutSamples = clip.fadeOutSamples;
    variant.gain = clip.gain;
    variant.timeStretchRatio = clip.timeStretchRatio;
    variant.timeStretchProvider = clip.timeStretchProvider;
    variant.reversed = clip.reversed;
    variant.warpMarkers = clip.warpMarkers;
    variant.transientMarkers = clip.transientMarkers;
    return variantNodeFromState (variant);
}

double ratioForSourceSpan (juce::int64 timelineLength,
                           juce::int64 sourceSpan,
                           double sourceRate,
                           double timelineRate)
{
    const auto nativeTimelineSpan = timeline::convertSampleCount (
        sourceSpan, sourceRate, timelineRate);
    return stretch::clampRatio (nativeTimelineSpan > 0
        ? static_cast<double> (timelineLength) / nativeTimelineSpan
        : 1.0);
}

juce::String mixerKindName (MixerChannelKind kind)
{
    return kind == MixerChannelKind::returnChannel ? "return" : "bus";
}

MixerChannelKind mixerKindFromName (const juce::String& name)
{
    return name == "return" ? MixerChannelKind::returnChannel
                            : MixerChannelKind::bus;
}

juce::String tempoCurveName (TempoCurve curve)
{
    return curve == TempoCurve::linear ? "linear" : "step";
}

TempoCurve tempoCurveFromName (const juce::String& name)
{
    return name == "linear" ? TempoCurve::linear : TempoCurve::step;
}

juce::String automationModeName (AutomationMode mode)
{
    switch (mode)
    {
        case AutomationMode::touch: return "touch";
        case AutomationMode::latch: return "latch";
        case AutomationMode::write: return "write";
        case AutomationMode::read: break;
    }
    return "read";
}

AutomationMode automationModeFromName (const juce::String& name)
{
    if (name == "touch") return AutomationMode::touch;
    if (name == "latch") return AutomationMode::latch;
    if (name == "write") return AutomationMode::write;
    return AutomationMode::read;
}

juce::String automationCurveName (AutomationCurve curve)
{
    switch (curve)
    {
        case AutomationCurve::hold: return "hold";
        case AutomationCurve::smooth: return "smooth";
        case AutomationCurve::linear: break;
    }
    return "linear";
}

AutomationCurve automationCurveFromName (const juce::String& name)
{
    if (name == "hold") return AutomationCurve::hold;
    if (name == "smooth") return AutomationCurve::smooth;
    return AutomationCurve::linear;
}

juce::String syncSourceName (SyncSourceState source)
{
    switch (source)
    {
        case SyncSourceState::midiClock: return "midi-clock";
        case SyncSourceState::midiTimeCode: return "mtc";
        case SyncSourceState::link: return "link";
        case SyncSourceState::internal: break;
    }
    return "internal";
}

SyncSourceState syncSourceFromName (const juce::String& name)
{
    if (name == "midi-clock") return SyncSourceState::midiClock;
    if (name == "mtc") return SyncSourceState::midiTimeCode;
    if (name == "link") return SyncSourceState::link;
    return SyncSourceState::internal;
}

juce::String monitorRoleName (MonitorPathRole role)
{
    switch (role)
    {
        case MonitorPathRole::cue: return "cue";
        case MonitorPathRole::listen: return "listen";
        case MonitorPathRole::talkback: return "talkback";
        case MonitorPathRole::controlRoom: break;
    }
    return "control-room";
}

MonitorPathRole monitorRoleFromName (const juce::String& name)
{
    if (name == "cue") return MonitorPathRole::cue;
    if (name == "listen") return MonitorPathRole::listen;
    if (name == "talkback") return MonitorPathRole::talkback;
    return MonitorPathRole::controlRoom;
}

MixerBusLayoutState busLayoutFromNode (const juce::ValueTree& node)
{
    MixerBusLayoutState layout;
    layout.inputChannels = juce::jlimit (0, 16, static_cast<int> (
        node.getProperty (modelIds::inputChannels, 2)));
    layout.outputChannels = juce::jlimit (1, 16, static_cast<int> (
        node.getProperty (modelIds::outputChannels, 2)));
    layout.sidechainChannels = juce::jlimit (0, 16, static_cast<int> (
        node.getProperty (modelIds::sidechainChannels, 0)));
    layout.explicitLayout = static_cast<bool> (
        node.getProperty (modelIds::explicitLayout, false));
    return layout;
}

void setBusLayoutProperties (juce::ValueTree& node,
                             const MixerBusLayoutState& layout)
{
    node.setProperty (modelIds::inputChannels,
                      juce::jlimit (0, 16, layout.inputChannels), nullptr);
    node.setProperty (modelIds::outputChannels,
                      juce::jlimit (1, 16, layout.outputChannels), nullptr);
    node.setProperty (modelIds::sidechainChannels,
                      juce::jlimit (0, 16, layout.sidechainChannels), nullptr);
    node.setProperty (modelIds::explicitLayout, layout.explicitLayout, nullptr);
}

PluginSlotState pluginSlotFromNode (const juce::ValueTree& node)
{
    PluginSlotState slot;
    slot.id = node.getProperty (modelIds::id).toString();
    slot.stablePluginId = node.getProperty (
        modelIds::stablePluginId).toString();
    slot.name = node.getProperty (modelIds::name).toString();
    slot.descriptionXml = node.getProperty (
        modelIds::descriptionXml).toString();
    slot.stateBase64 = node.getProperty (modelIds::stateBase64).toString();
    slot.role = node.getProperty (modelIds::role).toString() == "instrument"
                    ? PluginSlotRole::instrument
                    : PluginSlotRole::insertEffect;
    slot.isolation = node.getProperty (modelIds::isolation).toString()
                             == "trusted-in-process"
                         ? PluginIsolationMode::trustedInProcess
                         : PluginIsolationMode::workerProcess;
    slot.order = juce::jlimit (0, 15, static_cast<int> (
        node.getProperty (modelIds::order, 0)));
    slot.layout = busLayoutFromNode (node);
    slot.bypassed = static_cast<bool> (
        node.getProperty (modelIds::pluginBypassed, false));
    slot.available = static_cast<bool> (
        node.getProperty (modelIds::available, true));
    slot.latencySamples = juce::jlimit (0, 262143, static_cast<int> (
        node.getProperty (modelIds::latencySamples, 0)));
    slot.restartCount = juce::jlimit (0, 1000000, static_cast<int> (
        node.getProperty (modelIds::restartCount, 0)));
    return slot;
}

void addPluginSlotNode (juce::ValueTree& owner,
                        const PluginSlotState& source,
                        juce::UndoManager* undoManager = nullptr)
{
    auto slot = source;
    juce::ValueTree node (modelIds::pluginSlot);
    node.setProperty (modelIds::id,
                      slot.id.isNotEmpty() ? slot.id
                                           : juce::Uuid().toString(), nullptr);
    node.setProperty (modelIds::stablePluginId, slot.stablePluginId, nullptr);
    node.setProperty (modelIds::name, slot.name, nullptr);
    node.setProperty (modelIds::descriptionXml, slot.descriptionXml, nullptr);
    node.setProperty (modelIds::stateBase64, slot.stateBase64, nullptr);
    node.setProperty (modelIds::role,
                      slot.role == PluginSlotRole::instrument
                          ? "instrument" : "insert", nullptr);
    node.setProperty (modelIds::isolation,
                      slot.isolation == PluginIsolationMode::trustedInProcess
                          ? "trusted-in-process" : "worker", nullptr);
    node.setProperty (modelIds::order, juce::jlimit (0, 15, slot.order), nullptr);
    setBusLayoutProperties (node, slot.layout);
    node.setProperty (modelIds::pluginBypassed, slot.bypassed, nullptr);
    node.setProperty (modelIds::available, slot.available, nullptr);
    node.setProperty (modelIds::latencySamples,
                      juce::jlimit (0, 262143, slot.latencySamples), nullptr);
    node.setProperty (modelIds::restartCount,
                      juce::jmax (0, slot.restartCount), nullptr);
    owner.addChild (node, -1, undoManager);
}

std::vector<PluginSlotState> pluginSlotsFromNode (
    const juce::ValueTree& owner)
{
    std::vector<PluginSlotState> result;
    for (const auto child : owner)
        if (child.hasType (modelIds::pluginSlot))
            result.push_back (pluginSlotFromNode (child));
    std::stable_sort (result.begin(), result.end(), [] (const auto& a,
                                                        const auto& b)
    {
        return a.order < b.order;
    });
    return result;
}

void addMidiExpressionNodes (
    juce::ValueTree& note,
    const std::vector<MidiNoteState::ExpressionPoint>& expression)
{
    for (const auto& point : expression)
    {
        juce::ValueTree node (modelIds::midiExpression);
        node.setProperty (modelIds::id,
                          point.id.isNotEmpty() ? point.id
                                                : juce::Uuid().toString(), nullptr);
        node.setProperty (modelIds::offsetBeats,
                          juce::jmax (0.0, point.offsetBeats), nullptr);
        node.setProperty (modelIds::value,
                          juce::jlimit (-1.0f, 1.0f, point.value), nullptr);
        node.setProperty (modelIds::kind,
                          point.type == MidiNoteState::ExpressionPoint::Type::pressure
                              ? "pressure"
                              : point.type
                                    == MidiNoteState::ExpressionPoint::Type::timbre
                                  ? "timbre" : "pitch", nullptr);
        note.addChild (node, -1, nullptr);
    }
}

std::vector<MixerSendState> mixerSendsFromNode (const juce::ValueTree& owner)
{
    std::vector<MixerSendState> sends;
    for (const auto child : owner)
    {
        if (! child.hasType (modelIds::mixerSend))
            continue;
        sends.push_back ({ child.getProperty (modelIds::id).toString(),
                           child.getProperty (modelIds::destinationId).toString(),
                           juce::jlimit (0.0f, 2.0f, static_cast<float> (
                               child.getProperty (modelIds::gain, 0.0))),
                           static_cast<bool> (child.getProperty (
                               modelIds::enabled, true)),
                           static_cast<bool> (child.getProperty (
                               modelIds::preFader, false)),
                           static_cast<bool> (child.getProperty (
                               modelIds::sidechain, false)) });
    }
    return sends;
}

void addMixerSendNode (juce::ValueTree& owner,
                       const MixerSendState& send)
{
    juce::ValueTree node (modelIds::mixerSend);
    node.setProperty (modelIds::id,
                      send.id.isNotEmpty() ? send.id : juce::Uuid().toString(), nullptr);
    node.setProperty (modelIds::destinationId, send.destinationId, nullptr);
    node.setProperty (modelIds::gain,
                      juce::jlimit (0.0f, 2.0f, send.gain), nullptr);
    node.setProperty (modelIds::enabled, send.enabled, nullptr);
    node.setProperty (modelIds::preFader, send.preFader, nullptr);
    node.setProperty (modelIds::sidechain, send.sidechain, nullptr);
    owner.addChild (node, -1, nullptr);
}

constexpr size_t maxPersistentUndoHistory = 16;

juce::String persistentStateXmlFor (const juce::ValueTree& tree)
{
    if (const auto xml = tree.createXml())
        return xml->toString();
    return {};
}

juce::ValueTree persistentStateFromXml (const juce::String& xmlText)
{
    const auto xml = juce::parseXML (xmlText);
    if (xml == nullptr)
        return {};
    return juce::ValueTree::fromXml (*xml);
}

void trimPersistentHistory (std::vector<PersistentUndoEntryState>& entries)
{
    while (entries.size() > maxPersistentUndoHistory)
        entries.erase (entries.begin());
}
}

ProjectModel::ProjectModel()
    : state (createEmptyState())
{
    state.addListener (this);
}

ProjectModel::~ProjectModel()
{
    state.removeListener (this);
}

void ProjectModel::setChangeCallback (ChangeCallback callback)
{
    onChanged = std::move (callback);
}

void ProjectModel::reset()
{
    replaceWithSnapshot (ProjectState {});
}

juce::String ProjectModel::getProjectId() const
{
    return state.getProperty (modelIds::projectId).toString();
}

juce::String ProjectModel::getName() const
{
    return state.getProperty (modelIds::name, "Untitled Project").toString();
}

void ProjectModel::setName (juce::String newName, bool undoable)
{
    newName = newName.trim();

    if (newName.isEmpty())
        newName = "Untitled Project";

    if (getName() == newName)
        return;

    if (undoable)
        beginUndoTransaction ("Rename Project");

    state.setProperty (modelIds::name, newName, undoable ? &undoManager : nullptr);
}

float ProjectModel::getMasterGain() const
{
    return static_cast<float> (state.getProperty (modelIds::masterGain, 0.9));
}

void ProjectModel::setMasterGain (float newGain, bool undoable)
{
    newGain = juce::jlimit (0.0f, 1.25f, newGain);

    if (std::abs (getMasterGain() - newGain) < 0.0001f)
        return;

    state.setProperty (modelIds::masterGain, newGain, undoable ? &undoManager : nullptr);
}

bool ProjectModel::getMasterMuted() const
{
    return static_cast<bool> (state.getProperty (modelIds::masterMuted, false));
}

void ProjectModel::setMasterMuted (bool shouldBeMuted, bool undoable)
{
    if (getMasterMuted() == shouldBeMuted)
        return;
    if (undoable)
        beginUndoTransaction (shouldBeMuted ? "Mute Master" : "Unmute Master");
    state.setProperty (modelIds::masterMuted, shouldBeMuted,
                       undoable ? &undoManager : nullptr);
}

double ProjectModel::getTimelineSampleRate() const
{
    return static_cast<double> (state.getProperty (modelIds::timelineSampleRate, 48000.0));
}

void ProjectModel::setTimelineSampleRate (double newSampleRate, bool undoable)
{
    newSampleRate = juce::jmax (1.0, newSampleRate);
    state.setProperty (modelIds::timelineSampleRate,
                       newSampleRate,
                       undoable ? &undoManager : nullptr);
}

double ProjectModel::getTempoBpm() const
{
    for (const auto child : state)
        if (child.hasType (modelIds::tempoPoint)
            && static_cast<double> (child.getProperty (modelIds::beat, 0.0)) == 0.0)
            return static_cast<double> (child.getProperty (modelIds::bpm, 120.0));
    return 120.0;
}

void ProjectModel::setTempoBpm (double bpm, bool undoable)
{
    bpm = juce::jlimit (20.0, 400.0, bpm);
    for (auto child : state)
    {
        if (! child.hasType (modelIds::tempoPoint)
            || static_cast<double> (child.getProperty (modelIds::beat, 0.0)) != 0.0)
            continue;
        if (std::abs (static_cast<double> (child.getProperty (modelIds::bpm, 120.0))
                      - bpm) < 0.0001)
            return;
        child.setProperty (modelIds::bpm, bpm, undoable ? &undoManager : nullptr);
        return;
    }
}

std::vector<TempoPointState> ProjectModel::getTempoPoints() const
{
    std::vector<TempoPointState> result;
    for (const auto child : state)
    {
        if (! child.hasType (modelIds::tempoPoint))
            continue;
        result.push_back ({ child.getProperty (modelIds::id).toString(),
                            static_cast<double> (child.getProperty (modelIds::beat, 0.0)),
                            static_cast<double> (child.getProperty (modelIds::bpm, 120.0)),
                            tempoCurveFromName (child.getProperty (
                                modelIds::curve, "step").toString()) });
    }
    std::stable_sort (result.begin(), result.end(), [] (const auto& a, const auto& b)
    {
        return a.beat < b.beat;
    });
    return result;
}

juce::String ProjectModel::addTempoPoint (double beat, double bpm,
                                          TempoCurve curve)
{
    beat = juce::jmax (0.0, beat);
    bpm = juce::jlimit (20.0, 400.0, bpm);
    if (beat < 1.0e-9)
    {
        const auto points = getTempoPoints();
        if (! points.empty())
        {
            updateTempoPoint (points.front().id, 0.0, bpm, curve);
            return points.front().id;
        }
    }
    juce::ValueTree node (modelIds::tempoPoint);
    const auto id = juce::Uuid().toString();
    node.setProperty (modelIds::id, id, nullptr);
    node.setProperty (modelIds::beat, beat, nullptr);
    node.setProperty (modelIds::bpm, bpm, nullptr);
    node.setProperty (modelIds::curve, "step", nullptr);
    beginUndoTransaction ("Add Tempo Point");
    juce::ValueTree previous;
    auto previousBeat = -1.0;
    for (auto child : state)
    {
        if (! child.hasType (modelIds::tempoPoint))
            continue;
        const auto childBeat = static_cast<double> (
            child.getProperty (modelIds::beat, 0.0));
        if (childBeat < beat && childBeat > previousBeat)
        {
            previous = child;
            previousBeat = childBeat;
        }
    }
    suppressNotifications = true;
    if (previous.isValid())
        previous.setProperty (modelIds::curve, tempoCurveName (curve), &undoManager);
    state.addChild (node, -1, &undoManager);
    suppressNotifications = false;
    notifyChanged();
    return id;
}

bool ProjectModel::updateTempoPoint (const juce::String& pointId, double beat,
                                     double bpm, TempoCurve curve)
{
    auto node = findTimelineItem (modelIds::tempoPoint, pointId);
    if (! node.isValid())
        return false;
    const auto oldBeat = static_cast<double> (node.getProperty (modelIds::beat, 0.0));
    if (oldBeat < 1.0e-9)
        beat = 0.0;
    beginUndoTransaction ("Edit Tempo Point");
    node.setProperty (modelIds::beat, juce::jmax (0.0, beat), &undoManager);
    node.setProperty (modelIds::bpm, juce::jlimit (20.0, 400.0, bpm), &undoManager);
    node.setProperty (modelIds::curve, tempoCurveName (curve), &undoManager);
    return true;
}

bool ProjectModel::removeTempoPoint (const juce::String& pointId)
{
    auto node = findTimelineItem (modelIds::tempoPoint, pointId);
    if (! node.isValid()
        || static_cast<double> (node.getProperty (modelIds::beat, 0.0)) < 1.0e-9)
        return false;
    beginUndoTransaction ("Remove Tempo Point");
    state.removeChild (node, &undoManager);
    return true;
}

std::vector<TimeSignaturePointState> ProjectModel::getTimeSignatures() const
{
    std::vector<TimeSignaturePointState> result;
    for (const auto child : state)
        if (child.hasType (modelIds::timeSignaturePoint))
            result.push_back ({ child.getProperty (modelIds::id).toString(),
                                static_cast<double> (child.getProperty (
                                    modelIds::beat, 0.0)),
                                static_cast<int> (child.getProperty (
                                    modelIds::numerator, 4)),
                                static_cast<int> (child.getProperty (
                                    modelIds::denominator, 4)) });
    std::stable_sort (result.begin(), result.end(), [] (const auto& a, const auto& b)
    {
        return a.beat < b.beat;
    });
    return result;
}

juce::String ProjectModel::addTimeSignature (double beat, int numerator,
                                             int denominator)
{
    beat = juce::jmax (0.0, beat);
    if (beat < 1.0e-9)
    {
        const auto points = getTimeSignatures();
        if (! points.empty())
        {
            updateTimeSignature (points.front().id, 0.0, numerator, denominator);
            return points.front().id;
        }
    }
    juce::ValueTree node (modelIds::timeSignaturePoint);
    const auto id = juce::Uuid().toString();
    node.setProperty (modelIds::id, id, nullptr);
    node.setProperty (modelIds::beat, beat, nullptr);
    node.setProperty (modelIds::numerator, juce::jlimit (1, 32, numerator), nullptr);
    node.setProperty (modelIds::denominator,
                      denominator == 1 || denominator == 2 || denominator == 4
                          || denominator == 8 || denominator == 16
                          || denominator == 32 ? denominator : 4, nullptr);
    beginUndoTransaction ("Add Time Signature");
    state.addChild (node, -1, &undoManager);
    return id;
}

bool ProjectModel::updateTimeSignature (const juce::String& pointId, double beat,
                                        int numerator, int denominator)
{
    auto node = findTimelineItem (modelIds::timeSignaturePoint, pointId);
    if (! node.isValid())
        return false;
    const auto primary = static_cast<double> (
        node.getProperty (modelIds::beat, 0.0)) < 1.0e-9;
    if (primary)
        beat = 0.0;
    const auto validDenominator = denominator == 1 || denominator == 2
        || denominator == 4 || denominator == 8 || denominator == 16
        || denominator == 32;
    beginUndoTransaction ("Edit Time Signature");
    node.setProperty (modelIds::beat, juce::jmax (0.0, beat), &undoManager);
    node.setProperty (modelIds::numerator, juce::jlimit (1, 32, numerator), &undoManager);
    node.setProperty (modelIds::denominator,
                      validDenominator ? denominator : 4, &undoManager);
    if (primary)
    {
        state.setProperty (modelIds::timeSignatureNumerator,
                           juce::jlimit (1, 32, numerator), &undoManager);
        state.setProperty (modelIds::timeSignatureDenominator,
                           validDenominator ? denominator : 4, &undoManager);
    }
    return true;
}

bool ProjectModel::removeTimeSignature (const juce::String& pointId)
{
    auto node = findTimelineItem (modelIds::timeSignaturePoint, pointId);
    if (! node.isValid()
        || static_cast<double> (node.getProperty (modelIds::beat, 0.0)) < 1.0e-9)
        return false;
    beginUndoTransaction ("Remove Time Signature");
    state.removeChild (node, &undoManager);
    return true;
}

std::vector<ProjectMarkerState> ProjectModel::getMarkers() const
{
    std::vector<ProjectMarkerState> result;
    for (const auto child : state)
    {
        if (child.hasType (modelIds::projectMarker))
        {
            ProjectMarkerState marker;
            marker.id = child.getProperty (modelIds::id).toString();
            marker.beat = static_cast<double> (child.getProperty (
                modelIds::beat, 0.0));
            marker.name = child.getProperty (
                modelIds::name, "Marker").toString();
            const auto colourValue = static_cast<juce::uint32> (
                static_cast<juce::int64> (child.getProperty (
                    modelIds::colour,
                    static_cast<juce::int64> (0xffdf6b34))));
            marker.colour = juce::Colour (colourValue);
            result.push_back (std::move (marker));
        }
    }
    std::stable_sort (result.begin(), result.end(), [] (const auto& a, const auto& b)
    {
        return a.beat < b.beat;
    });
    return result;
}

juce::String ProjectModel::addMarker (double beat, juce::String name)
{
    juce::ValueTree node (modelIds::projectMarker);
    const auto id = juce::Uuid().toString();
    const auto count = static_cast<int> (getMarkers().size()) + 1;
    node.setProperty (modelIds::id, id, nullptr);
    node.setProperty (modelIds::beat, juce::jmax (0.0, beat), nullptr);
    node.setProperty (modelIds::name,
                      name.trim().isNotEmpty() ? name.trim()
                                               : "Marker " + juce::String (count), nullptr);
    node.setProperty (modelIds::colour, static_cast<juce::int64> (0xffdf6b34), nullptr);
    beginUndoTransaction ("Add Marker");
    state.addChild (node, -1, &undoManager);
    return id;
}

bool ProjectModel::updateMarker (const juce::String& markerId, double beat,
                                 juce::String name)
{
    auto node = findTimelineItem (modelIds::projectMarker, markerId);
    if (! node.isValid())
        return false;
    beginUndoTransaction ("Edit Marker");
    node.setProperty (modelIds::beat, juce::jmax (0.0, beat), &undoManager);
    node.setProperty (modelIds::name,
                      name.trim().isNotEmpty() ? name.trim() : "Marker", &undoManager);
    return true;
}

bool ProjectModel::removeMarker (const juce::String& markerId)
{
    auto node = findTimelineItem (modelIds::projectMarker, markerId);
    if (! node.isValid())
        return false;
    beginUndoTransaction ("Remove Marker");
    state.removeChild (node, &undoManager);
    return true;
}

bool ProjectModel::getMetronomeEnabled() const
{
    return static_cast<bool> (state.getProperty (modelIds::metronomeEnabled, false));
}

void ProjectModel::setMetronomeEnabled (bool enabled, bool undoable)
{
    if (getMetronomeEnabled() == enabled)
        return;
    if (undoable)
        beginUndoTransaction (enabled ? "Enable Metronome" : "Disable Metronome");
    state.setProperty (modelIds::metronomeEnabled, enabled,
                       undoable ? &undoManager : nullptr);
}

float ProjectModel::getMetronomeGain() const
{
    return static_cast<float> (state.getProperty (modelIds::metronomeGain, 0.5));
}

void ProjectModel::setMetronomeGain (float gain, bool undoable)
{
    gain = juce::jlimit (0.0f, 1.0f, gain);
    state.setProperty (modelIds::metronomeGain, gain,
                       undoable ? &undoManager : nullptr);
}

std::vector<AutomationLaneState> ProjectModel::getAutomationLanes() const
{
    std::vector<AutomationLaneState> result;
    for (const auto laneNode : state)
    {
        if (! laneNode.hasType (modelIds::automationLane))
            continue;
        AutomationLaneState lane;
        lane.id = laneNode.getProperty (modelIds::id).toString();
        lane.targetId = laneNode.getProperty (modelIds::targetId, "master").toString();
        lane.parameterId = laneNode.getProperty (modelIds::parameterId, "gain").toString();
        lane.mode = automationModeFromName (laneNode.getProperty (
            modelIds::mode, "read").toString());
        lane.enabled = static_cast<bool> (laneNode.getProperty (modelIds::enabled, true));
        for (const auto pointNode : laneNode)
            if (pointNode.hasType (modelIds::automationPoint))
                lane.points.push_back ({
                    pointNode.getProperty (modelIds::id).toString(),
                    static_cast<juce::int64> (pointNode.getProperty (
                        modelIds::samplePosition, static_cast<juce::int64> (0))),
                    static_cast<float> (pointNode.getProperty (modelIds::gain, 0.0)),
                    automationCurveFromName (pointNode.getProperty (
                        modelIds::curve, "linear").toString()) });
        std::stable_sort (lane.points.begin(), lane.points.end(),
                          [] (const auto& a, const auto& b)
        {
            return a.samplePosition < b.samplePosition;
        });
        result.push_back (std::move (lane));
    }
    return result;
}

AutomationLaneState ProjectModel::getAutomationLane (
    const juce::String& laneId) const
{
    const auto lanes = getAutomationLanes();
    const auto found = std::find_if (lanes.begin(), lanes.end(),
                                     [&laneId] (const auto& lane)
    {
        return lane.id == laneId;
    });
    return found != lanes.end() ? *found : AutomationLaneState {};
}

juce::String ProjectModel::addAutomationLane (juce::String targetId,
                                              juce::String parameterId,
    AutomationMode mode)
{
    if (targetId.isEmpty()) targetId = "master";
    if (parameterId != "gain" && parameterId != "pan"
        && ! parameterId.startsWith ("plugin:"))
        parameterId = "gain";
    for (const auto& lane : getAutomationLanes())
        if (lane.targetId == targetId && lane.parameterId == parameterId)
            return lane.id;
    juce::ValueTree node (modelIds::automationLane);
    const auto id = juce::Uuid().toString();
    node.setProperty (modelIds::id, id, nullptr);
    node.setProperty (modelIds::targetId, targetId, nullptr);
    node.setProperty (modelIds::parameterId, parameterId, nullptr);
    node.setProperty (modelIds::mode, automationModeName (mode), nullptr);
    node.setProperty (modelIds::enabled, true, nullptr);
    beginUndoTransaction ("Add Automation Lane");
    state.addChild (node, -1, &undoManager);
    return id;
}

bool ProjectModel::removeAutomationLane (const juce::String& laneId)
{
    auto node = findAutomationLane (laneId);
    if (! node.isValid()) return false;
    beginUndoTransaction ("Remove Automation Lane");
    state.removeChild (node, &undoManager);
    return true;
}

void ProjectModel::setAutomationLaneMode (const juce::String& laneId,
                                          AutomationMode mode)
{
    auto node = findAutomationLane (laneId);
    if (node.isValid())
        node.setProperty (modelIds::mode, automationModeName (mode), &undoManager);
}

void ProjectModel::setAutomationLaneEnabled (const juce::String& laneId,
                                             bool enabled)
{
    auto node = findAutomationLane (laneId);
    if (node.isValid())
        node.setProperty (modelIds::enabled, enabled, &undoManager);
}

juce::String ProjectModel::addAutomationPoint (const juce::String& laneId,
                                               juce::int64 samplePosition,
                                               float value,
                                               AutomationCurve curve,
                                               bool beginNewTransaction)
{
    auto lane = findAutomationLane (laneId);
    if (! lane.isValid()) return {};
    samplePosition = juce::jmax (static_cast<juce::int64> (0), samplePosition);
    for (auto existing : lane)
    {
        if (! existing.hasType (modelIds::automationPoint)
            || static_cast<juce::int64> (existing.getProperty (
                modelIds::samplePosition, static_cast<juce::int64> (0)))
                != samplePosition)
            continue;
        if (beginNewTransaction)
            beginUndoTransaction ("Edit Automation Point");
        existing.setProperty (modelIds::gain,
                              juce::jlimit (-1.0f, 1.5f, value), &undoManager);
        existing.setProperty (modelIds::curve,
                              automationCurveName (curve), &undoManager);
        return existing.getProperty (modelIds::id).toString();
    }
    juce::ValueTree node (modelIds::automationPoint);
    const auto id = juce::Uuid().toString();
    node.setProperty (modelIds::id, id, nullptr);
    node.setProperty (modelIds::samplePosition,
                      samplePosition, nullptr);
    node.setProperty (modelIds::gain, juce::jlimit (-1.0f, 1.5f, value), nullptr);
    node.setProperty (modelIds::curve, automationCurveName (curve), nullptr);
    if (beginNewTransaction)
        beginUndoTransaction ("Add Automation Point");
    lane.addChild (node, -1, &undoManager);
    return id;
}

bool ProjectModel::updateAutomationPoint (const juce::String& laneId,
                                          const juce::String& pointId,
                                          juce::int64 samplePosition,
                                          float value,
                                          AutomationCurve curve)
{
    auto lane = findAutomationLane (laneId);
    if (! lane.isValid()) return false;
    for (auto node : lane)
    {
        if (! node.hasType (modelIds::automationPoint)
            || node.getProperty (modelIds::id).toString() != pointId)
            continue;
        node.setProperty (modelIds::samplePosition,
                          juce::jmax (static_cast<juce::int64> (0), samplePosition),
                          &undoManager);
        node.setProperty (modelIds::gain, juce::jlimit (-1.0f, 1.5f, value),
                          &undoManager);
        node.setProperty (modelIds::curve, automationCurveName (curve), &undoManager);
        return true;
    }
    return false;
}

bool ProjectModel::removeAutomationPoint (const juce::String& laneId,
                                          const juce::String& pointId)
{
    auto lane = findAutomationLane (laneId);
    if (! lane.isValid()) return false;
    for (int index = 0; index < lane.getNumChildren(); ++index)
    {
        const auto node = lane.getChild (index);
        if (node.hasType (modelIds::automationPoint)
            && node.getProperty (modelIds::id).toString() == pointId)
        {
            beginUndoTransaction ("Remove Automation Point");
            lane.removeChild (index, &undoManager);
            return true;
        }
    }
    return false;
}

bool ProjectModel::getLowLatencyMonitoring() const
{
    return static_cast<bool> (
        state.getProperty (modelIds::lowLatencyMonitoring, false));
}

void ProjectModel::setLowLatencyMonitoring (bool enabled, bool undoable)
{
    if (getLowLatencyMonitoring() == enabled)
        return;
    if (undoable)
        beginUndoTransaction (enabled ? "Enable Low Latency Monitoring"
                                      : "Disable Low Latency Monitoring");
    state.setProperty (modelIds::lowLatencyMonitoring, enabled,
                       undoable ? &undoManager : nullptr);
}

InputMonitorMode ProjectModel::getInputMonitorMode() const
{
    const auto mode = state.getProperty (
        modelIds::inputMonitorMode, "off").toString();
    return mode == "always" ? InputMonitorMode::always
         : mode == "armed" ? InputMonitorMode::whileArmed
                            : InputMonitorMode::off;
}

void ProjectModel::setInputMonitorMode (InputMonitorMode mode, bool undoable)
{
    if (getInputMonitorMode() == mode)
        return;
    if (undoable)
        beginUndoTransaction ("Change Input Monitoring Mode");
    state.setProperty (
        modelIds::inputMonitorMode,
        mode == InputMonitorMode::always ? "always"
            : mode == InputMonitorMode::whileArmed ? "armed" : "off",
        undoable ? &undoManager : nullptr);
}

int ProjectModel::getManualRecordOffsetSamples() const
{
    return static_cast<int> (state.getProperty (
        modelIds::manualRecordOffsetSamples, 0));
}

void ProjectModel::setManualRecordOffsetSamples (int samples, bool undoable)
{
    samples = juce::jlimit (-8192, 8192, samples);
    if (getManualRecordOffsetSamples() == samples)
        return;
    if (undoable)
        beginUndoTransaction ("Change Record Alignment Offset");
    state.setProperty (modelIds::manualRecordOffsetSamples, samples,
                       undoable ? &undoManager : nullptr);
}

RecordingSettingsState ProjectModel::getRecordingSettings() const
{
    RecordingSettingsState result;
    const auto mode = state.getProperty (
        modelIds::recordingMode, "normal").toString();
    result.mode = mode == "punch" ? RecordingMode::punch
                : mode == "loop" ? RecordingMode::loop
                                  : RecordingMode::normal;
    result.preRollBars = juce::jlimit (
        0, 8, static_cast<int> (state.getProperty (modelIds::preRollBars, 0)));
    result.punchStartSamples = juce::jmax (
        static_cast<juce::int64> (0),
        static_cast<juce::int64> (state.getProperty (
            modelIds::punchStartSamples, 0)));
    result.punchEndSamples = juce::jmax (
        result.punchStartSamples,
        static_cast<juce::int64> (state.getProperty (
            modelIds::punchEndSamples, 0)));
    result.loopStartSamples = juce::jmax (
        static_cast<juce::int64> (0),
        static_cast<juce::int64> (state.getProperty (
            modelIds::loopStartSamples, 0)));
    result.loopEndSamples = juce::jmax (
        result.loopStartSamples,
        static_cast<juce::int64> (state.getProperty (
            modelIds::loopEndSamples, 0)));
    result.controlRoomGain = juce::jlimit (
        0.0f, 1.5f, static_cast<float> (state.getProperty (
            modelIds::controlRoomGain, 1.0f)));
    result.controlRoomDimmed = static_cast<bool> (state.getProperty (
        modelIds::controlRoomDimmed, false));
    result.controlRoomMuted = static_cast<bool> (state.getProperty (
        modelIds::controlRoomMuted, false));
    return result;
}

void ProjectModel::setRecordingSettings (
    const RecordingSettingsState& requested, bool undoable)
{
    auto settings = requested;
    settings.preRollBars = juce::jlimit (0, 8, settings.preRollBars);
    settings.punchStartSamples = juce::jmax (
        static_cast<juce::int64> (0), settings.punchStartSamples);
    settings.punchEndSamples = juce::jmax (
        settings.punchStartSamples, settings.punchEndSamples);
    settings.loopStartSamples = juce::jmax (
        static_cast<juce::int64> (0), settings.loopStartSamples);
    settings.loopEndSamples = juce::jmax (
        settings.loopStartSamples, settings.loopEndSamples);
    settings.controlRoomGain = juce::jlimit (
        0.0f, 1.5f, settings.controlRoomGain);
    if (undoable)
        beginUndoTransaction ("Change Recording Setup");
    suppressNotifications = true;
    auto* manager = undoable ? &undoManager : nullptr;
    state.setProperty (modelIds::recordingMode,
        settings.mode == RecordingMode::punch ? "punch"
            : settings.mode == RecordingMode::loop ? "loop" : "normal",
        manager);
    state.setProperty (modelIds::preRollBars, settings.preRollBars, manager);
    state.setProperty (modelIds::punchStartSamples,
                       settings.punchStartSamples, manager);
    state.setProperty (modelIds::punchEndSamples,
                       settings.punchEndSamples, manager);
    state.setProperty (modelIds::loopStartSamples,
                       settings.loopStartSamples, manager);
    state.setProperty (modelIds::loopEndSamples,
                       settings.loopEndSamples, manager);
    state.setProperty (modelIds::controlRoomGain,
                       settings.controlRoomGain, manager);
    state.setProperty (modelIds::controlRoomDimmed,
                       settings.controlRoomDimmed, manager);
    state.setProperty (modelIds::controlRoomMuted,
                       settings.controlRoomMuted, manager);
    suppressNotifications = false;
    notifyChanged();
}

SyncSettingsState ProjectModel::getSyncSettings() const
{
    SyncSettingsState result;
    result.source = syncSourceFromName (state.getProperty (
        modelIds::syncSource, "internal").toString());
    result.sendMidiClock = static_cast<bool> (state.getProperty (
        modelIds::sendMidiClock, false));
    result.followExternalStartStop = static_cast<bool> (state.getProperty (
        modelIds::followExternalStartStop, true));
    result.mtcFramesPerSecond = juce::jlimit (24, 30, static_cast<int> (
        state.getProperty (modelIds::mtcFramesPerSecond, 30)));
    result.jitterToleranceSamples = juce::jlimit (0, 8192, static_cast<int> (
        state.getProperty (modelIds::jitterToleranceSamples, 256)));
    return result;
}

void ProjectModel::setSyncSettings (const SyncSettingsState& requested,
                                    bool undoable)
{
    auto settings = requested;
    settings.mtcFramesPerSecond = settings.mtcFramesPerSecond == 24
        || settings.mtcFramesPerSecond == 25
        || settings.mtcFramesPerSecond == 29
        || settings.mtcFramesPerSecond == 30
            ? settings.mtcFramesPerSecond : 30;
    settings.jitterToleranceSamples = juce::jlimit (
        0, 8192, settings.jitterToleranceSamples);
    if (undoable)
        beginUndoTransaction ("Change Synchronization Setup");
    suppressNotifications = true;
    auto* manager = undoable ? &undoManager : nullptr;
    state.setProperty (modelIds::syncSource,
                       syncSourceName (settings.source), manager);
    state.setProperty (modelIds::sendMidiClock,
                       settings.sendMidiClock, manager);
    state.setProperty (modelIds::followExternalStartStop,
                       settings.followExternalStartStop, manager);
    state.setProperty (modelIds::mtcFramesPerSecond,
                       settings.mtcFramesPerSecond, manager);
    state.setProperty (modelIds::jitterToleranceSamples,
                       settings.jitterToleranceSamples, manager);
    suppressNotifications = false;
    notifyChanged();
}

std::vector<MonitorPathState> ProjectModel::getMonitorPaths() const
{
    std::vector<MonitorPathState> result;
    for (const auto child : state)
    {
        if (! child.hasType (modelIds::monitorPath))
            continue;
        MonitorPathState path;
        path.id = child.getProperty (modelIds::id).toString();
        path.name = child.getProperty (modelIds::name).toString();
        path.role = monitorRoleFromName (child.getProperty (
            modelIds::role, "control-room").toString());
        path.sourceId = child.getProperty (
            modelIds::sourceId, "master").toString();
        path.outputStartChannel = juce::jlimit (0, 63, static_cast<int> (
            child.getProperty (modelIds::outputStartChannel, 0)));
        path.outputChannelCount = juce::jlimit (1, 16, static_cast<int> (
            child.getProperty (modelIds::outputChannelCount, 2)));
        path.gain = juce::jlimit (0.0f, 2.0f, static_cast<float> (
            child.getProperty (modelIds::gain, 1.0f)));
        path.muted = static_cast<bool> (child.getProperty (
            modelIds::muted, false));
        path.dimmed = static_cast<bool> (child.getProperty (
            modelIds::dimmed, false));
        result.push_back (std::move (path));
    }
    return result;
}

void ProjectModel::setMonitorPaths (std::vector<MonitorPathState> paths,
                                    bool undoable)
{
    if (undoable)
        beginUndoTransaction ("Change Monitor Paths");
    suppressNotifications = true;
    auto* manager = undoable ? &undoManager : nullptr;
    for (int index = state.getNumChildren(); index-- > 0;)
        if (state.getChild (index).hasType (modelIds::monitorPath))
            state.removeChild (index, manager);
    for (auto& path : paths)
    {
        juce::ValueTree node (modelIds::monitorPath);
        node.setProperty (modelIds::id,
                          path.id.isNotEmpty() ? path.id
                                               : juce::Uuid().toString(), nullptr);
        node.setProperty (modelIds::name,
                          path.name.isNotEmpty() ? path.name
                                                 : monitorRoleName (path.role), nullptr);
        node.setProperty (modelIds::role, monitorRoleName (path.role), nullptr);
        node.setProperty (modelIds::sourceId,
                          path.sourceId.isNotEmpty() ? path.sourceId
                                                     : "master", nullptr);
        node.setProperty (modelIds::outputStartChannel,
                          juce::jlimit (0, 63, path.outputStartChannel), nullptr);
        node.setProperty (modelIds::outputChannelCount,
                          juce::jlimit (1, 16, path.outputChannelCount), nullptr);
        node.setProperty (modelIds::gain,
                          juce::jlimit (0.0f, 2.0f, path.gain), nullptr);
        node.setProperty (modelIds::muted, path.muted, nullptr);
        node.setProperty (modelIds::dimmed, path.dimmed, nullptr);
        state.addChild (node, -1, manager);
    }
    suppressNotifications = false;
    notifyChanged();
}

int ProjectModel::getTrackCount() const
{
    int count = 0;

    for (const auto child : state)
        if (child.hasType (modelIds::track))
            ++count;

    return count;
}

juce::String ProjectModel::getTrackId (int index) const
{
    int currentIndex = 0;

    for (const auto child : state)
    {
        if (! child.hasType (modelIds::track))
            continue;

        if (currentIndex++ == index)
            return child.getProperty (modelIds::id).toString();
    }

    return {};
}

bool ProjectModel::hasTrack (const juce::String& trackId) const
{
    return findTrack (trackId).isValid();
}

TrackState ProjectModel::getTrackState (const juce::String& trackId) const
{
    TrackState result;
    auto trackNode = findTrack (trackId);

    if (! trackNode.isValid())
        return result;

    result.id = trackNode.getProperty (modelIds::id).toString();
    result.name = trackNode.getProperty (modelIds::name).toString();
    result.colour = juce::Colour (static_cast<juce::uint32> (
        static_cast<juce::int64> (trackNode.getProperty (modelIds::colour))));
    result.gain = static_cast<float> (trackNode.getProperty (modelIds::gain, 0.85));
    result.pan = static_cast<float> (trackNode.getProperty (modelIds::pan, 0.0));
    result.muted = static_cast<bool> (trackNode.getProperty (modelIds::muted, false));
    result.solo = static_cast<bool> (trackNode.getProperty (modelIds::solo, false));
    result.recordArmed = static_cast<bool> (
        trackNode.getProperty (modelIds::recordArmed, false));
    result.inputStartChannel = juce::jlimit (
        0, 63, static_cast<int> (trackNode.getProperty (
            modelIds::inputStartChannel, 0)));
    result.inputChannelCount = juce::jlimit (
        1, 2, static_cast<int> (trackNode.getProperty (
            modelIds::inputChannelCount, 2)));
    const auto storedRecordingTapPoint = trackNode.getProperty (
        modelIds::recordingTapPoint, "hardware-input").toString();
    result.recordingTapPoint = storedRecordingTapPoint == "device-output"
            || storedRecordingTapPoint == "program-output"
        ? RecordingTapPoint::programOutput
        : RecordingTapPoint::hardwareInput;
    result.recordOffsetSamples = juce::jlimit (
        -8192, 8192, static_cast<int> (trackNode.getProperty (
            modelIds::recordOffsetSamples, 0)));
    result.isInstrument = static_cast<bool> (
        trackNode.getProperty (modelIds::isInstrument, false));
    result.pluginName = trackNode.getProperty (modelIds::pluginName).toString();
    result.pluginDescriptionXml = trackNode.getProperty (
        modelIds::pluginDescriptionXml).toString();
    result.pluginStateBase64 = trackNode.getProperty (
        modelIds::pluginStateBase64).toString();
    result.pluginBypassed = static_cast<bool> (
        trackNode.getProperty (modelIds::pluginBypassed, false));
    result.pluginLatencySamples = juce::jmax (0, static_cast<int> (
        trackNode.getProperty (modelIds::pluginLatencySamples, 0)));
    result.pluginAvailable = static_cast<bool> (
        trackNode.getProperty (modelIds::pluginAvailable, true));
    result.manualLatencyOffsetSamples = juce::jlimit (
        -262143, 262143, static_cast<int> (trackNode.getProperty (
            modelIds::manualLatencyOffsetSamples, 0)));
    result.outputDestinationId = trackNode.getProperty (
        modelIds::outputDestinationId, "master").toString();
    if (result.outputDestinationId.isEmpty())
        result.outputDestinationId = "master";
    result.sends = mixerSendsFromNode (trackNode);
    result.layout = busLayoutFromNode (trackNode);
    result.pluginSlots = pluginSlotsFromNode (trackNode);
    if (result.pluginSlots.empty()
        && result.pluginDescriptionXml.isNotEmpty())
    {
        PluginSlotState slot;
        slot.id = "instrument:" + result.id;
        slot.stablePluginId = result.pluginName.isNotEmpty()
            ? result.pluginName : slot.id;
        slot.name = result.pluginName;
        slot.descriptionXml = result.pluginDescriptionXml;
        slot.stateBase64 = result.pluginStateBase64;
        slot.role = PluginSlotRole::instrument;
        slot.order = 0;
        slot.layout.inputChannels = 0;
        slot.layout.outputChannels = 2;
        slot.layout.explicitLayout = true;
        slot.bypassed = result.pluginBypassed;
        slot.available = result.pluginAvailable;
        slot.latencySamples = result.pluginLatencySamples;
        result.pluginSlots.push_back (std::move (slot));
    }
    result.frozen = static_cast<bool> (
        trackNode.getProperty (modelIds::frozen, false));
    result.freezeSourceId = trackNode.getProperty (
        modelIds::freezeSourceId).toString();

    for (const auto child : trackNode)
    {
        if (child.hasType (modelIds::clip))
        {
            AudioClipState clipState;
            clipState.id = child.getProperty (modelIds::id).toString();
            clipState.name = child.getProperty (modelIds::name).toString();
            clipState.sourceId = child.getProperty (modelIds::sourceId).toString();
            clipState.timelineStartSamples = static_cast<juce::int64> (
                child.getProperty (modelIds::timelineStartSamples, 0));
            clipState.sourceOffsetSamples = static_cast<juce::int64> (
                child.getProperty (modelIds::sourceOffsetSamples, 0));
            clipState.lengthInSamples = static_cast<juce::int64> (
                child.getProperty (modelIds::lengthInSamples, 0));
            clipState.fadeInSamples = juce::jlimit (
                static_cast<juce::int64> (0), clipState.lengthInSamples,
                static_cast<juce::int64> (child.getProperty (
                    modelIds::fadeInSamples, 0)));
            clipState.fadeOutSamples = juce::jlimit (
                static_cast<juce::int64> (0), clipState.lengthInSamples,
                static_cast<juce::int64> (child.getProperty (
                    modelIds::fadeOutSamples, 0)));
            clipState.gain = static_cast<float> (child.getProperty (modelIds::gain, 1.0));
            clipState.colour = juce::Colour (static_cast<juce::uint32> (
                static_cast<juce::int64> (child.getProperty (
                    modelIds::colour, static_cast<juce::int64> (0x00000000)))));
            clipState.muted = static_cast<bool> (
                child.getProperty (modelIds::muted, false));
            clipState.locked = static_cast<bool> (
                child.getProperty (modelIds::locked, false));
            clipState.takeGroupId = child.getProperty (modelIds::takeGroupId).toString();
            clipState.takeLaneIndex = static_cast<int> (
                child.getProperty (modelIds::takeLaneIndex, 0));
            clipState.activeTake = static_cast<bool> (
                child.getProperty (modelIds::activeTake, true));
            clipState.timeStretchRatio = stretch::clampRatio (static_cast<double> (
                child.getProperty (modelIds::timeStretchRatio, 1.0)));
            clipState.timeStretchProvider = child.getProperty (
                modelIds::timeStretchProvider, stretch::builtInProviderId).toString();
            clipState.reversed = static_cast<bool> (
                child.getProperty (modelIds::reversed, false));
            for (const auto markerNode : child)
                if (markerNode.hasType (modelIds::warpMarker))
                    clipState.warpMarkers.push_back ({
                        markerNode.getProperty (modelIds::id).toString(),
                        static_cast<juce::int64> (markerNode.getProperty (
                            modelIds::timelineOffsetSamples, 0)),
                        static_cast<juce::int64> (markerNode.getProperty (
                            modelIds::sourceOffsetSamples, 0)) });
                else if (markerNode.hasType (modelIds::transientMarker))
                    clipState.transientMarkers.push_back ({
                        markerNode.getProperty (modelIds::id).toString(),
                        static_cast<juce::int64> (markerNode.getProperty (
                            modelIds::timelineOffsetSamples, 0)),
                        static_cast<juce::int64> (markerNode.getProperty (
                            modelIds::sourceOffsetSamples, 0)),
                        static_cast<float> (markerNode.getProperty (
                            modelIds::strength, 0.0)) });
                else if (markerNode.hasType (modelIds::clipVariant))
                {
                    auto variant = variantFromNode (markerNode);
                    if (variant.id.isNotEmpty() && variant.sourceId.isNotEmpty())
                        clipState.variants.push_back (std::move (variant));
                }
            result.clips.push_back (std::move (clipState));
        }
        else if (child.hasType (modelIds::compRegion))
        {
            CompRegionState region;
            region.id = child.getProperty (modelIds::id).toString();
            region.takeGroupId = child.getProperty (modelIds::takeGroupId).toString();
            region.takeLaneIndex = static_cast<int> (
                child.getProperty (modelIds::takeLaneIndex, 0));
            region.timelineStartSamples = static_cast<juce::int64> (
                child.getProperty (modelIds::timelineStartSamples, 0));
            region.lengthInSamples = static_cast<juce::int64> (
                child.getProperty (modelIds::lengthInSamples, 0));
            region.crossfadeSamples = static_cast<juce::int64> (
                child.getProperty (modelIds::crossfadeSamples, 480));
            result.compRegions.push_back (std::move (region));
        }
        else if (child.hasType (modelIds::midiClip))
        {
            MidiClipState clipState;
            clipState.id = child.getProperty (modelIds::id).toString();
            clipState.startBeat = static_cast<double> (
                child.getProperty (modelIds::startBeat, 0.0));
            clipState.lengthBeats = static_cast<double> (
                child.getProperty (modelIds::lengthBeats, 16.0));
            for (const auto noteNode : child)
            {
                if (noteNode.hasType (modelIds::midiController))
                {
                    clipState.controllers.push_back ({
                        noteNode.getProperty (modelIds::id).toString(),
                        static_cast<double> (noteNode.getProperty (
                            modelIds::beat, 0.0)),
                        juce::jlimit (1, 16, static_cast<int> (
                            noteNode.getProperty (modelIds::channel, 1))),
                        juce::jlimit (0, 127, static_cast<int> (
                            noteNode.getProperty (modelIds::controller, 1))),
                        static_cast<std::uint32_t> (static_cast<juce::int64> (
                            noteNode.getProperty (modelIds::value, 0))),
                        static_cast<std::uint32_t> (static_cast<juce::int64> (
                            noteNode.getProperty (modelIds::noteId, 0))) });
                    continue;
                }
                if (! noteNode.hasType (modelIds::midiNote))
                    continue;
                MidiNoteState note;
                note.id = noteNode.getProperty (modelIds::id).toString();
                note.startBeat = static_cast<double> (noteNode.getProperty (
                    modelIds::startBeat, 0.0));
                note.lengthBeats = static_cast<double> (noteNode.getProperty (
                    modelIds::lengthBeats, 1.0));
                note.noteNumber = static_cast<int> (noteNode.getProperty (
                    modelIds::noteNumber, 60));
                note.velocity = static_cast<float> (noteNode.getProperty (
                    modelIds::velocity, 0.8f));
                note.channel = static_cast<int> (noteNode.getProperty (
                    modelIds::channel, 1));
                note.noteId = static_cast<std::uint32_t> (
                    static_cast<juce::int64> (noteNode.getProperty (
                        modelIds::noteId, 0)));
                for (const auto expressionNode : noteNode)
                {
                    if (! expressionNode.hasType (modelIds::midiExpression))
                        continue;
                    MidiNoteState::ExpressionPoint point;
                    point.id = expressionNode.getProperty (
                        modelIds::id).toString();
                    point.offsetBeats = juce::jmax (0.0, static_cast<double> (
                        expressionNode.getProperty (modelIds::offsetBeats, 0.0)));
                    point.value = juce::jlimit (-1.0f, 1.0f, static_cast<float> (
                        expressionNode.getProperty (modelIds::value, 0.0f)));
                    const auto type = expressionNode.getProperty (
                        modelIds::kind, "pitch").toString();
                    point.type = type == "pressure"
                        ? MidiNoteState::ExpressionPoint::Type::pressure
                        : type == "timbre"
                            ? MidiNoteState::ExpressionPoint::Type::timbre
                            : MidiNoteState::ExpressionPoint::Type::pitch;
                    note.expression.push_back (std::move (point));
                }
                clipState.notes.push_back (std::move (note));
            }
            result.midiClips.push_back (std::move (clipState));
        }
    }

    return result;
}

AudioClipState ProjectModel::getClipState (const juce::String& trackId,
                                           const juce::String& clipId) const
{
    const auto clipNode = findClip (trackId, clipId);
    AudioClipState result;

    if (! clipNode.isValid())
        return result;

    result.id = clipNode.getProperty (modelIds::id).toString();
    result.name = clipNode.getProperty (modelIds::name).toString();
    result.sourceId = clipNode.getProperty (modelIds::sourceId).toString();
    result.timelineStartSamples = static_cast<juce::int64> (
        clipNode.getProperty (modelIds::timelineStartSamples, 0));
    result.sourceOffsetSamples = static_cast<juce::int64> (
        clipNode.getProperty (modelIds::sourceOffsetSamples, 0));
    result.lengthInSamples = static_cast<juce::int64> (
        clipNode.getProperty (modelIds::lengthInSamples, 0));
    result.fadeInSamples = juce::jlimit (
        static_cast<juce::int64> (0), result.lengthInSamples,
        static_cast<juce::int64> (clipNode.getProperty (
            modelIds::fadeInSamples, 0)));
    result.fadeOutSamples = juce::jlimit (
        static_cast<juce::int64> (0), result.lengthInSamples,
        static_cast<juce::int64> (clipNode.getProperty (
            modelIds::fadeOutSamples, 0)));
    result.gain = static_cast<float> (clipNode.getProperty (modelIds::gain, 1.0));
    result.colour = juce::Colour (static_cast<juce::uint32> (
        static_cast<juce::int64> (clipNode.getProperty (
            modelIds::colour, static_cast<juce::int64> (0x00000000)))));
    result.muted = static_cast<bool> (
        clipNode.getProperty (modelIds::muted, false));
    result.locked = static_cast<bool> (
        clipNode.getProperty (modelIds::locked, false));
    result.takeGroupId = clipNode.getProperty (modelIds::takeGroupId).toString();
    result.takeLaneIndex = static_cast<int> (
        clipNode.getProperty (modelIds::takeLaneIndex, 0));
    result.activeTake = static_cast<bool> (
        clipNode.getProperty (modelIds::activeTake, true));
    result.timeStretchRatio = stretch::clampRatio (static_cast<double> (
        clipNode.getProperty (modelIds::timeStretchRatio, 1.0)));
    result.timeStretchProvider = clipNode.getProperty (
        modelIds::timeStretchProvider, stretch::builtInProviderId).toString();
    result.reversed = static_cast<bool> (
        clipNode.getProperty (modelIds::reversed, false));
    for (const auto markerNode : clipNode)
        if (markerNode.hasType (modelIds::warpMarker))
            result.warpMarkers.push_back ({
                markerNode.getProperty (modelIds::id).toString(),
                static_cast<juce::int64> (markerNode.getProperty (
                    modelIds::timelineOffsetSamples, 0)),
                static_cast<juce::int64> (markerNode.getProperty (
                    modelIds::sourceOffsetSamples, 0)) });
        else if (markerNode.hasType (modelIds::transientMarker))
            result.transientMarkers.push_back ({
                markerNode.getProperty (modelIds::id).toString(),
                static_cast<juce::int64> (markerNode.getProperty (
                    modelIds::timelineOffsetSamples, 0)),
                static_cast<juce::int64> (markerNode.getProperty (
                    modelIds::sourceOffsetSamples, 0)),
                static_cast<float> (markerNode.getProperty (
                    modelIds::strength, 0.0)) });
        else if (markerNode.hasType (modelIds::clipVariant))
        {
            auto variant = variantFromNode (markerNode);
            if (variant.id.isNotEmpty() && variant.sourceId.isNotEmpty())
                result.variants.push_back (std::move (variant));
        }
    return result;
}

juce::int64 ProjectModel::getProjectLengthInSamples() const
{
    juce::int64 result = 0;
    std::vector<tempo::Point> points;
    for (const auto& point : getTempoPoints())
        points.push_back ({ point.beat, point.bpm,
            point.curve == TempoCurve::linear ? tempo::SegmentCurve::linear
                                               : tempo::SegmentCurve::step });

    for (int index = 0; index < getTrackCount(); ++index)
    {
        const auto track = getTrackState (getTrackId (index));
        for (const auto& clip : track.clips)
            result = juce::jmax (result,
                                 clip.timelineStartSamples + clip.lengthInSamples);
        for (const auto& clip : track.midiClips)
            result = juce::jmax (result,
                                 static_cast<juce::int64> (tempo::beatToSamples (
                                     clip.startBeat + clip.lengthBeats,
                                     getTimelineSampleRate(), points)));
    }

    for (const auto& marker : getMarkers())
        result = juce::jmax (result,
            static_cast<juce::int64> (tempo::beatToSamples (
                marker.beat, getTimelineSampleRate(), points)));
    for (const auto& lane : getAutomationLanes())
        for (const auto& point : lane.points)
            result = juce::jmax (result, point.samplePosition);

    return result;
}

MediaSourceState ProjectModel::getMediaSourceState (const juce::String& sourceId) const
{
    MediaSourceState result;
    const auto sourceNode = findMediaSource (sourceId);

    if (! sourceNode.isValid())
        return result;

    result.id = sourceNode.getProperty (modelIds::id).toString();
    result.file = juce::File (sourceNode.getProperty (modelIds::path).toString());
    result.sampleRate = static_cast<double> (sourceNode.getProperty (modelIds::sampleRate, 0.0));
    result.channels = static_cast<int> (sourceNode.getProperty (modelIds::channels, 0));
    result.lengthInSamples = static_cast<juce::int64> (
        sourceNode.getProperty (modelIds::lengthInSamples, 0));
    result.assetFingerprint = sourceNode.getProperty (
        modelIds::assetFingerprint).toString();
    return result;
}

int ProjectModel::getMediaSourceCount() const
{
    int count = 0;

    for (const auto child : state)
        if (child.hasType (modelIds::mediaSource))
            ++count;

    return count;
}

juce::String ProjectModel::getMediaSourceId (int index) const
{
    int currentIndex = 0;

    for (const auto child : state)
    {
        if (! child.hasType (modelIds::mediaSource))
            continue;

        if (currentIndex++ == index)
            return child.getProperty (modelIds::id).toString();
    }

    return {};
}

void ProjectModel::setMediaSourceFile (const juce::String& sourceId,
                                       const juce::File& newFile,
                                       bool undoable)
{
    auto sourceNode = findMediaSource (sourceId);

    if (! sourceNode.isValid() || newFile == juce::File()
        || sourceNode.getProperty (modelIds::path).toString() == newFile.getFullPathName())
        return;

    if (undoable)
        beginUndoTransaction ("Relink Media Source");

    sourceNode.setProperty (modelIds::path,
                            newFile.getFullPathName(),
                            undoable ? &undoManager : nullptr);
    sourceNode.setProperty (modelIds::assetFingerprint,
                            makeAssetFingerprint (
                                newFile,
                                static_cast<double> (sourceNode.getProperty (
                                    modelIds::sampleRate, 0.0)),
                                static_cast<int> (sourceNode.getProperty (
                                    modelIds::channels, 0)),
                                static_cast<juce::int64> (
                                    sourceNode.getProperty (
                                        modelIds::lengthInSamples, 0))),
                            undoable ? &undoManager : nullptr);
}

juce::String ProjectModel::addAudioTrack (const juce::File& sourceFile,
                                          double sourceSampleRate,
                                          int sourceChannels,
                                          juce::int64 sourceLengthInSamples)
{
    if (! sourceFile.existsAsFile() || sourceSampleRate <= 0.0
        || sourceChannels <= 0 || sourceLengthInSamples <= 0)
        return {};

    const auto sourceId = juce::Uuid().toString();
    const auto trackId = juce::Uuid().toString();
    const auto clipId = juce::Uuid().toString();

    juce::ValueTree sourceNode (modelIds::mediaSource);
    sourceNode.setProperty (modelIds::id, sourceId, nullptr);
    sourceNode.setProperty (modelIds::path, sourceFile.getFullPathName(), nullptr);
    sourceNode.setProperty (modelIds::sampleRate, sourceSampleRate, nullptr);
    sourceNode.setProperty (modelIds::channels, sourceChannels, nullptr);
    sourceNode.setProperty (modelIds::lengthInSamples, sourceLengthInSamples, nullptr);
    sourceNode.setProperty (modelIds::assetFingerprint,
                            makeAssetFingerprint (
                                sourceFile, sourceSampleRate, sourceChannels,
                                sourceLengthInSamples),
                            nullptr);

    juce::ValueTree trackNode (modelIds::track);
    trackNode.setProperty (modelIds::id, trackId, nullptr);
    trackNode.setProperty (modelIds::name, sourceFile.getFileNameWithoutExtension(), nullptr);
    trackNode.setProperty (modelIds::colour,
                           static_cast<juce::int64> (defaultTrackColour (getTrackCount()).getARGB()),
                           nullptr);
    trackNode.setProperty (modelIds::gain, 0.85f, nullptr);
    trackNode.setProperty (modelIds::pan, 0.0f, nullptr);
    trackNode.setProperty (modelIds::muted, false, nullptr);
    trackNode.setProperty (modelIds::solo, false, nullptr);
    trackNode.setProperty (modelIds::recordArmed, false, nullptr);
    trackNode.setProperty (modelIds::inputStartChannel, 0, nullptr);
    trackNode.setProperty (modelIds::inputChannelCount,
                           juce::jlimit (1, 2, sourceChannels), nullptr);
    trackNode.setProperty (modelIds::recordingTapPoint,
                           "hardware-input", nullptr);
    trackNode.setProperty (modelIds::recordOffsetSamples, 0, nullptr);
    trackNode.setProperty (modelIds::outputDestinationId, "master", nullptr);

    juce::ValueTree clipNode (modelIds::clip);
    clipNode.setProperty (modelIds::id, clipId, nullptr);
    clipNode.setProperty (modelIds::name,
                          sourceFile.getFileNameWithoutExtension(), nullptr);
    clipNode.setProperty (modelIds::sourceId, sourceId, nullptr);
    clipNode.setProperty (modelIds::timelineStartSamples, static_cast<juce::int64> (0), nullptr);
    clipNode.setProperty (modelIds::sourceOffsetSamples, static_cast<juce::int64> (0), nullptr);
    clipNode.setProperty (modelIds::lengthInSamples,
                          juce::jmax (static_cast<juce::int64> (1),
                                      static_cast<juce::int64> (
                                          timeline::convertSampleCount (
                                              sourceLengthInSamples,
                                              sourceSampleRate,
                                              getTimelineSampleRate()))),
                          nullptr);
    clipNode.setProperty (modelIds::gain, 1.0f, nullptr);
    clipNode.setProperty (modelIds::colour,
                          static_cast<juce::int64> (0x00000000), nullptr);
    clipNode.setProperty (modelIds::muted, false, nullptr);
    clipNode.setProperty (modelIds::locked, false, nullptr);
    clipNode.setProperty (modelIds::takeGroupId, {}, nullptr);
    clipNode.setProperty (modelIds::takeLaneIndex, 0, nullptr);
    clipNode.setProperty (modelIds::activeTake, true, nullptr);
    trackNode.addChild (clipNode, -1, nullptr);

    beginUndoTransaction ("Import Audio");
    state.addChild (sourceNode, -1, &undoManager);
    state.addChild (trackNode, -1, &undoManager);
    return trackId;
}

juce::String ProjectModel::addEmptyAudioTrack (juce::String name)
{
    const auto trackId = juce::Uuid().toString();
    name = name.trim();

    juce::ValueTree trackNode (modelIds::track);
    trackNode.setProperty (modelIds::id, trackId, nullptr);
    trackNode.setProperty (modelIds::name,
                           name.isNotEmpty() ? name : "Audio Track",
                           nullptr);
    trackNode.setProperty (modelIds::colour,
                           static_cast<juce::int64> (
                               defaultTrackColour (getTrackCount()).getARGB()),
                           nullptr);
    trackNode.setProperty (modelIds::gain, 0.85f, nullptr);
    trackNode.setProperty (modelIds::pan, 0.0f, nullptr);
    trackNode.setProperty (modelIds::muted, false, nullptr);
    trackNode.setProperty (modelIds::solo, false, nullptr);
    trackNode.setProperty (modelIds::recordArmed, false, nullptr);
    trackNode.setProperty (modelIds::inputStartChannel, 0, nullptr);
    trackNode.setProperty (modelIds::inputChannelCount, 2, nullptr);
    trackNode.setProperty (modelIds::recordingTapPoint,
                           "hardware-input", nullptr);
    trackNode.setProperty (modelIds::recordOffsetSamples, 0, nullptr);
    trackNode.setProperty (modelIds::isInstrument, false, nullptr);
    trackNode.setProperty (modelIds::manualLatencyOffsetSamples, 0, nullptr);
    trackNode.setProperty (modelIds::outputDestinationId, "master", nullptr);

    beginUndoTransaction ("Create Audio Track");
    state.addChild (trackNode, -1, &undoManager);
    return trackId;
}

juce::String ProjectModel::addInstrumentTrack (juce::String name)
{
    const auto trackId = juce::Uuid().toString();
    juce::ValueTree trackNode (modelIds::track);
    trackNode.setProperty (modelIds::id, trackId, nullptr);
    trackNode.setProperty (modelIds::name,
                           name.trim().isNotEmpty() ? name.trim() : "Triumph Instrument",
                           nullptr);
    trackNode.setProperty (modelIds::colour,
                           static_cast<juce::int64> (
                               defaultTrackColour (getTrackCount()).getARGB()), nullptr);
    trackNode.setProperty (modelIds::gain, 0.75f, nullptr);
    trackNode.setProperty (modelIds::pan, 0.0f, nullptr);
    trackNode.setProperty (modelIds::muted, false, nullptr);
    trackNode.setProperty (modelIds::solo, false, nullptr);
    trackNode.setProperty (modelIds::recordArmed, false, nullptr);
    trackNode.setProperty (modelIds::inputStartChannel, 0, nullptr);
    trackNode.setProperty (modelIds::inputChannelCount, 2, nullptr);
    trackNode.setProperty (modelIds::recordingTapPoint,
                           "hardware-input", nullptr);
    trackNode.setProperty (modelIds::recordOffsetSamples, 0, nullptr);
    trackNode.setProperty (modelIds::isInstrument, true, nullptr);
    trackNode.setProperty (modelIds::pluginName, {}, nullptr);
    trackNode.setProperty (modelIds::pluginDescriptionXml, {}, nullptr);
    trackNode.setProperty (modelIds::pluginStateBase64, {}, nullptr);
    trackNode.setProperty (modelIds::pluginBypassed, false, nullptr);
    trackNode.setProperty (modelIds::pluginLatencySamples, 0, nullptr);
    trackNode.setProperty (modelIds::pluginAvailable, true, nullptr);
    trackNode.setProperty (modelIds::manualLatencyOffsetSamples, 0, nullptr);
    trackNode.setProperty (modelIds::outputDestinationId, "master", nullptr);
    trackNode.setProperty (modelIds::frozen, false, nullptr);
    trackNode.setProperty (modelIds::freezeSourceId, {}, nullptr);

    beginUndoTransaction ("Create Instrument Track");
    state.addChild (trackNode, -1, &undoManager);
    return trackId;
}

juce::String ProjectModel::addGeneratedMidiTrack (
    juce::String name, const std::vector<MidiNoteState>& notes,
    double lengthBeats, juce::String undoDescription)
{
    if (notes.empty())
        return {};

    const auto trackId = juce::Uuid().toString();
    juce::ValueTree trackNode (modelIds::track);
    trackNode.setProperty (modelIds::id, trackId, nullptr);
    trackNode.setProperty (modelIds::name,
                           name.trim().isNotEmpty() ? name.trim()
                                                    : "Producer Instrument", nullptr);
    trackNode.setProperty (modelIds::colour,
                           static_cast<juce::int64> (
                               defaultTrackColour (getTrackCount()).getARGB()), nullptr);
    trackNode.setProperty (modelIds::gain, 0.72f, nullptr);
    trackNode.setProperty (modelIds::pan, 0.0f, nullptr);
    trackNode.setProperty (modelIds::muted, false, nullptr);
    trackNode.setProperty (modelIds::solo, false, nullptr);
    trackNode.setProperty (modelIds::recordArmed, false, nullptr);
    trackNode.setProperty (modelIds::inputStartChannel, 0, nullptr);
    trackNode.setProperty (modelIds::inputChannelCount, 2, nullptr);
    trackNode.setProperty (modelIds::recordingTapPoint,
                           "hardware-input", nullptr);
    trackNode.setProperty (modelIds::recordOffsetSamples, 0, nullptr);
    trackNode.setProperty (modelIds::isInstrument, true, nullptr);
    trackNode.setProperty (modelIds::pluginName, {}, nullptr);
    trackNode.setProperty (modelIds::pluginDescriptionXml, {}, nullptr);
    trackNode.setProperty (modelIds::pluginStateBase64, {}, nullptr);
    trackNode.setProperty (modelIds::pluginBypassed, false, nullptr);
    trackNode.setProperty (modelIds::pluginLatencySamples, 0, nullptr);
    trackNode.setProperty (modelIds::pluginAvailable, true, nullptr);
    trackNode.setProperty (modelIds::manualLatencyOffsetSamples, 0, nullptr);
    trackNode.setProperty (modelIds::outputDestinationId, "master", nullptr);
    trackNode.setProperty (modelIds::frozen, false, nullptr);
    trackNode.setProperty (modelIds::freezeSourceId, {}, nullptr);

    juce::ValueTree clipNode (modelIds::midiClip);
    clipNode.setProperty (modelIds::id, juce::Uuid().toString(), nullptr);
    clipNode.setProperty (modelIds::startBeat, 0.0, nullptr);
    auto requiredLength = juce::jmax (1.0, lengthBeats);
    for (const auto& source : notes)
    {
        juce::ValueTree noteNode (modelIds::midiNote);
        noteNode.setProperty (modelIds::id,
                              source.id.isNotEmpty() ? source.id
                                                     : juce::Uuid().toString(), nullptr);
        noteNode.setProperty (modelIds::startBeat,
                              juce::jmax (0.0, source.startBeat), nullptr);
        noteNode.setProperty (modelIds::lengthBeats,
                              juce::jmax (1.0 / 64.0, source.lengthBeats), nullptr);
        noteNode.setProperty (modelIds::noteNumber,
                              juce::jlimit (0, 127, source.noteNumber), nullptr);
        noteNode.setProperty (modelIds::velocity,
                              juce::jlimit (0.01f, 1.0f, source.velocity), nullptr);
        noteNode.setProperty (modelIds::channel,
                              juce::jlimit (1, 16, source.channel), nullptr);
        noteNode.setProperty (modelIds::noteId,
                              static_cast<juce::int64> (source.noteId), nullptr);
        addMidiExpressionNodes (noteNode, source.expression);
        requiredLength = juce::jmax (requiredLength,
                                     source.startBeat + source.lengthBeats);
        clipNode.addChild (noteNode, -1, nullptr);
    }
    clipNode.setProperty (modelIds::lengthBeats, requiredLength, nullptr);
    trackNode.addChild (clipNode, -1, nullptr);

    beginUndoTransaction (undoDescription.trim().isNotEmpty()
                              ? undoDescription.trim() : "Producer MIDI Generation");
    state.addChild (trackNode, -1, &undoManager);
    return trackId;
}

juce::String ProjectModel::addMidiNote (const juce::String& trackId,
                                        const juce::String& clipId,
                                        double startBeat,
                                        double lengthBeats,
                                        int noteNumber,
                                        float velocity)
{
    auto track = findTrack (trackId);
    auto clip = findMidiClip (trackId, clipId);
    if (! clip.isValid())
    {
        if (! track.isValid()
            || ! static_cast<bool> (track.getProperty (modelIds::isInstrument, false))
            || clipId.isNotEmpty())
            return {};

        juce::ValueTree clipNode (modelIds::midiClip);
        clipNode.setProperty (modelIds::id, juce::Uuid().toString(), nullptr);
        clipNode.setProperty (modelIds::startBeat, 0.0, nullptr);
        clipNode.setProperty (
            modelIds::lengthBeats,
            juce::jmax (16.0, startBeat + juce::jmax (1.0 / 64.0, lengthBeats)),
            nullptr);

        beginUndoTransaction ("Add MIDI Note");
        track.addChild (clipNode, -1, &undoManager);
        clip = clipNode;
    }
    else
    {
        beginUndoTransaction ("Add MIDI Note");
    }

    const auto noteId = juce::Uuid().toString();
    juce::ValueTree note (modelIds::midiNote);
    note.setProperty (modelIds::id, noteId, nullptr);
    note.setProperty (modelIds::startBeat, juce::jmax (0.0, startBeat), nullptr);
    note.setProperty (modelIds::lengthBeats, juce::jmax (1.0 / 64.0, lengthBeats), nullptr);
    note.setProperty (modelIds::noteNumber, juce::jlimit (0, 127, noteNumber), nullptr);
    note.setProperty (modelIds::velocity, juce::jlimit (0.01f, 1.0f, velocity), nullptr);
    note.setProperty (modelIds::channel, 1, nullptr);
    note.setProperty (modelIds::noteId, static_cast<juce::int64> (0), nullptr);
    clip.addChild (note, -1, &undoManager);
    return noteId;
}

int ProjectModel::addMidiNotes (const juce::String& trackId,
                                const juce::String& clipId,
                                const std::vector<MidiNoteState>& notes)
{
    auto track = findTrack (trackId);
    auto clip = findMidiClip (trackId, clipId);
    if (! track.isValid() || notes.empty())
        return 0;

    if (! clip.isValid()
        && (! static_cast<bool> (track.getProperty (modelIds::isInstrument, false))
            || clipId.isNotEmpty()))
        return 0;

    beginUndoTransaction ("Record MIDI Take");
    suppressNotifications = true;

    if (! clip.isValid())
    {
        double requiredLengthBeats = 16.0;
        for (const auto& source : notes)
            requiredLengthBeats = juce::jmax (
                requiredLengthBeats,
                juce::jmax (0.0, source.startBeat)
                    + juce::jmax (1.0 / 64.0, source.lengthBeats));

        juce::ValueTree clipNode (modelIds::midiClip);
        clipNode.setProperty (modelIds::id, juce::Uuid().toString(), nullptr);
        clipNode.setProperty (modelIds::startBeat, 0.0, nullptr);
        clipNode.setProperty (modelIds::lengthBeats, requiredLengthBeats,
                              nullptr);
        track.addChild (clipNode, -1, &undoManager);
        clip = clipNode;
    }

    int added = 0;
    auto requiredLengthBeats = static_cast<double> (
        clip.getProperty (modelIds::lengthBeats, 16.0));
    for (const auto& source : notes)
    {
        juce::ValueTree note (modelIds::midiNote);
        note.setProperty (modelIds::id,
                          source.id.isNotEmpty() ? source.id : juce::Uuid().toString(), nullptr);
        note.setProperty (modelIds::startBeat, juce::jmax (0.0, source.startBeat), nullptr);
        note.setProperty (modelIds::lengthBeats,
                          juce::jmax (1.0 / 64.0, source.lengthBeats), nullptr);
        note.setProperty (modelIds::noteNumber,
                          juce::jlimit (0, 127, source.noteNumber), nullptr);
        note.setProperty (modelIds::velocity,
                          juce::jlimit (0.01f, 1.0f, source.velocity), nullptr);
        note.setProperty (modelIds::channel,
                          juce::jlimit (1, 16, source.channel), nullptr);
        note.setProperty (modelIds::noteId,
                          static_cast<juce::int64> (source.noteId), nullptr);
        addMidiExpressionNodes (note, source.expression);
        clip.addChild (note, -1, &undoManager);
        requiredLengthBeats = juce::jmax (requiredLengthBeats,
                                          source.startBeat + source.lengthBeats);
        ++added;
    }
    clip.setProperty (modelIds::lengthBeats,
                      juce::jmax (1.0, requiredLengthBeats), &undoManager);
    track.setProperty (modelIds::recordArmed, false, &undoManager);
    suppressNotifications = false;
    notifyChanged();
    return added;
}

bool ProjectModel::removeMidiNote (const juce::String& trackId,
                                   const juce::String& clipId,
                                   const juce::String& noteId)
{
    auto clip = findMidiClip (trackId, clipId);
    for (const auto child : clip)
    {
        if (child.hasType (modelIds::midiNote)
            && child.getProperty (modelIds::id).toString() == noteId)
        {
            beginUndoTransaction ("Delete MIDI Note");
            clip.removeChild (child, &undoManager);
            return true;
        }
    }
    return false;
}

bool ProjectModel::quantizeMidiClip (const juce::String& trackId,
                                     const juce::String& clipId,
                                     double gridBeats)
{
    auto clip = findMidiClip (trackId, clipId);
    if (! clip.isValid() || gridBeats <= 0.0)
        return false;
    beginUndoTransaction ("Quantize MIDI Notes");
    bool changed = false;
    for (auto child : clip)
    {
        if (! child.hasType (modelIds::midiNote))
            continue;
        const auto original = static_cast<double> (child.getProperty (modelIds::startBeat));
        const auto quantized = tempo::quantizeBeat (original, gridBeats);
        if (std::abs (original - quantized) > 1.0e-9)
        {
            child.setProperty (modelIds::startBeat, quantized, &undoManager);
            changed = true;
        }
    }
    return changed;
}

juce::String ProjectModel::addMidiControllerEvent (
    const juce::String& trackId, const juce::String& clipId,
    const MidiControllerEventState& source)
{
    auto clip = findMidiClip (trackId, clipId);
    if (! clip.isValid())
        return {};
    const auto id = source.id.isNotEmpty()
        ? source.id : juce::Uuid().toString();
    juce::ValueTree event (modelIds::midiController);
    event.setProperty (modelIds::id, id, nullptr);
    event.setProperty (modelIds::beat,
                       juce::jmax (0.0, source.beat), nullptr);
    event.setProperty (modelIds::channel,
                       juce::jlimit (1, 16, source.channel), nullptr);
    event.setProperty (modelIds::controller,
                       juce::jlimit (0, 127, source.controller), nullptr);
    event.setProperty (modelIds::value,
                       static_cast<juce::int64> (source.value), nullptr);
    event.setProperty (modelIds::noteId,
                       static_cast<juce::int64> (source.noteId), nullptr);
    beginUndoTransaction ("Add MIDI Controller Event");
    clip.addChild (event, -1, &undoManager);
    return id;
}

bool ProjectModel::setMidiNoteExpression (
    const juce::String& trackId, const juce::String& clipId,
    const juce::String& noteId,
    std::vector<MidiNoteState::ExpressionPoint> expression)
{
    auto clip = findMidiClip (trackId, clipId);
    if (! clip.isValid())
        return false;
    for (auto note : clip)
    {
        if (! note.hasType (modelIds::midiNote)
            || note.getProperty (modelIds::id).toString() != noteId)
            continue;
        beginUndoTransaction ("Edit Per-note Expression");
        suppressNotifications = true;
        for (int index = note.getNumChildren(); index-- > 0;)
            if (note.getChild (index).hasType (modelIds::midiExpression))
                note.removeChild (index, &undoManager);
        for (const auto& point : expression)
        {
            juce::ValueTree node (modelIds::midiExpression);
            node.setProperty (modelIds::id,
                              point.id.isNotEmpty() ? point.id
                                                    : juce::Uuid().toString(), nullptr);
            node.setProperty (modelIds::offsetBeats,
                              juce::jmax (0.0, point.offsetBeats), nullptr);
            node.setProperty (modelIds::value,
                              juce::jlimit (-1.0f, 1.0f, point.value), nullptr);
            node.setProperty (modelIds::kind,
                              point.type
                                      == MidiNoteState::ExpressionPoint::Type::pressure
                                  ? "pressure"
                                  : point.type
                                            == MidiNoteState::ExpressionPoint::Type::timbre
                                        ? "timbre" : "pitch", nullptr);
            note.addChild (node, -1, &undoManager);
        }
        suppressNotifications = false;
        notifyChanged();
        return true;
    }
    return false;
}

juce::String ProjectModel::getFirstInstrumentTrackId() const
{
    for (int index = 0; index < getTrackCount(); ++index)
    {
        const auto id = getTrackId (index);
        if (getTrackState (id).isInstrument)
            return id;
    }
    return {};
}

ProducerSettingsState ProjectModel::getProducerSettings() const
{
    ProducerSettingsState settings;
    settings.rootPitchClass = juce::jlimit (0, 11, static_cast<int> (
        state.getProperty (modelIds::producerRootPitchClass, 0)));
    settings.scale = state.getProperty (
        modelIds::producerScale, "major").toString();
    if (settings.scale != "minor") settings.scale = "major";
    settings.style = state.getProperty (
        modelIds::producerStyle, "balanced").toString();
    if (settings.style != "chill" && settings.style != "energetic")
        settings.style = "balanced";
    settings.bars = juce::jlimit (1, 16, static_cast<int> (
        state.getProperty (modelIds::producerBars, 4)));
    settings.variation = static_cast<std::uint32_t> (
        static_cast<juce::int64> (state.getProperty (
            modelIds::producerVariation, static_cast<juce::int64> (1))));
    if (settings.variation == 0) settings.variation = 1;
    return settings;
}

void ProjectModel::setProducerSettings (const ProducerSettingsState& source,
                                        bool undoable)
{
    auto settings = source;
    settings.rootPitchClass = juce::jlimit (0, 11, settings.rootPitchClass);
    if (settings.scale != "minor") settings.scale = "major";
    if (settings.style != "chill" && settings.style != "energetic")
        settings.style = "balanced";
    settings.bars = juce::jlimit (1, 16, settings.bars);
    if (settings.variation == 0) settings.variation = 1;
    if (undoable) beginUndoTransaction ("Change Producer Settings");
    auto* manager = undoable ? &undoManager : nullptr;
    suppressNotifications = true;
    state.setProperty (modelIds::producerRootPitchClass,
                       settings.rootPitchClass, manager);
    state.setProperty (modelIds::producerScale, settings.scale, manager);
    state.setProperty (modelIds::producerStyle, settings.style, manager);
    state.setProperty (modelIds::producerBars, settings.bars, manager);
    state.setProperty (modelIds::producerVariation,
                       static_cast<juce::int64> (settings.variation), manager);
    suppressNotifications = false;
    notifyChanged();
}

int ProjectModel::applyTrackMixUpdates (
    const std::vector<TrackMixUpdate>& updates,
    juce::String undoDescription)
{
    auto validCount = 0;
    for (const auto& update : updates)
        if (findTrack (update.trackId).isValid())
            ++validCount;
    if (validCount == 0)
        return 0;

    beginUndoTransaction (undoDescription.trim().isNotEmpty()
                              ? undoDescription.trim() : "Producer Mix Balance");
    suppressNotifications = true;
    for (const auto& update : updates)
    {
        auto trackNode = findTrack (update.trackId);
        if (! trackNode.isValid())
            continue;
        trackNode.setProperty (modelIds::gain,
                               juce::jlimit (0.0f, 1.5f, update.gain),
                               &undoManager);
        trackNode.setProperty (modelIds::pan,
                               juce::jlimit (-1.0f, 1.0f, update.pan),
                               &undoManager);
    }
    suppressNotifications = false;
    notifyChanged();
    return validCount;
}

juce::String ProjectModel::addRecordedClip (const juce::String& trackId,
                                            const juce::File& sourceFile,
                                            double sourceSampleRate,
                                            int sourceChannels,
                                            juce::int64 sourceLengthInSamples,
                                            juce::int64 timelineStartSamples)
{
    auto trackNode = findTrack (trackId);
    if (! trackNode.isValid()
        || static_cast<bool> (trackNode.getProperty (modelIds::isInstrument, false))
        || ! sourceFile.existsAsFile() || sourceSampleRate <= 0.0
        || sourceChannels <= 0 || sourceLengthInSamples <= 0)
        return {};

    juce::String groupId;
    auto laneIndex = 0;
    std::vector<juce::ValueTree> ungroupedAtStart;
    for (const auto child : trackNode)
    {
        if (! child.hasType (modelIds::clip)
            || static_cast<juce::int64> (child.getProperty (
                   modelIds::timelineStartSamples, 0)) != timelineStartSamples)
            continue;
        const auto existingGroup = child.getProperty (modelIds::takeGroupId).toString();
        if (existingGroup.isNotEmpty())
        {
            groupId = existingGroup;
            laneIndex = juce::jmax (
                laneIndex,
                static_cast<int> (child.getProperty (modelIds::takeLaneIndex, 0)) + 1);
        }
        else
            ungroupedAtStart.push_back (child);
    }
    if (groupId.isEmpty())
    {
        groupId = juce::Uuid().toString();
        laneIndex = static_cast<int> (ungroupedAtStart.size());
    }

    const auto sourceId = juce::Uuid().toString();
    const auto clipId = juce::Uuid().toString();

    juce::ValueTree sourceNode (modelIds::mediaSource);
    sourceNode.setProperty (modelIds::id, sourceId, nullptr);
    sourceNode.setProperty (modelIds::path, sourceFile.getFullPathName(), nullptr);
    sourceNode.setProperty (modelIds::sampleRate, sourceSampleRate, nullptr);
    sourceNode.setProperty (modelIds::channels, sourceChannels, nullptr);
    sourceNode.setProperty (modelIds::lengthInSamples, sourceLengthInSamples, nullptr);
    sourceNode.setProperty (modelIds::assetFingerprint,
                            makeAssetFingerprint (
                                sourceFile, sourceSampleRate, sourceChannels,
                                sourceLengthInSamples),
                            nullptr);

    juce::ValueTree clipNode (modelIds::clip);
    clipNode.setProperty (modelIds::id, clipId, nullptr);
    clipNode.setProperty (modelIds::name,
                          sourceFile.getFileNameWithoutExtension(), nullptr);
    clipNode.setProperty (modelIds::sourceId, sourceId, nullptr);
    clipNode.setProperty (modelIds::timelineStartSamples,
                          juce::jmax (static_cast<juce::int64> (0),
                                      timelineStartSamples),
                          nullptr);
    clipNode.setProperty (modelIds::sourceOffsetSamples,
                          static_cast<juce::int64> (0),
                          nullptr);
    clipNode.setProperty (modelIds::lengthInSamples,
                          juce::jmax (static_cast<juce::int64> (1),
                                      static_cast<juce::int64> (
                                          timeline::convertSampleCount (
                                              sourceLengthInSamples,
                                              sourceSampleRate,
                                              getTimelineSampleRate()))),
                          nullptr);
    clipNode.setProperty (modelIds::gain, 1.0f, nullptr);
    clipNode.setProperty (modelIds::colour,
                          static_cast<juce::int64> (0x00000000), nullptr);
    clipNode.setProperty (modelIds::muted, false, nullptr);
    clipNode.setProperty (modelIds::locked, false, nullptr);
    clipNode.setProperty (modelIds::takeGroupId, groupId, nullptr);
    clipNode.setProperty (modelIds::takeLaneIndex, laneIndex, nullptr);
    clipNode.setProperty (modelIds::activeTake, true, nullptr);

    beginUndoTransaction ("Add Recorded Take");
    suppressNotifications = true;
    state.addChild (sourceNode, -1, &undoManager);
    for (int index = 0; index < static_cast<int> (ungroupedAtStart.size()); ++index)
    {
        auto child = ungroupedAtStart[static_cast<size_t> (index)];
        child.setProperty (modelIds::takeGroupId, groupId, &undoManager);
        child.setProperty (modelIds::takeLaneIndex, index, &undoManager);
        child.setProperty (modelIds::activeTake, false, &undoManager);
    }
    for (auto child : trackNode)
        if (child.hasType (modelIds::clip)
            && child.getProperty (modelIds::takeGroupId).toString() == groupId)
            child.setProperty (modelIds::activeTake, false, &undoManager);
    trackNode.addChild (clipNode, -1, &undoManager);
    trackNode.setProperty (modelIds::recordArmed, false, &undoManager);
    suppressNotifications = false;
    notifyChanged();
    return clipId;
}

bool ProjectModel::removeTrack (const juce::String& trackId)
{
    auto trackNode = findTrack (trackId);

    if (! trackNode.isValid())
        return false;

    juce::StringArray referencedSources;
    for (const auto child : trackNode)
        if (child.hasType (modelIds::clip))
            referencedSources.addIfNotAlreadyThere (child.getProperty (modelIds::sourceId).toString());

    beginUndoTransaction ("Delete Track");
    suppressNotifications = true;
    state.removeChild (trackNode, &undoManager);

    for (int index = state.getNumChildren(); index-- > 0;)
    {
        const auto child = state.getChild (index);
        if (child.hasType (modelIds::automationLane)
            && child.getProperty (modelIds::targetId).toString() == trackId)
            state.removeChild (index, &undoManager);
    }

    for (int ownerIndex = state.getNumChildren() - 1;
         ownerIndex >= 0; --ownerIndex)
    {
        auto owner = state.getChild (ownerIndex);
        if (! owner.hasType (modelIds::track)
            && ! owner.hasType (modelIds::mixerChannel))
            continue;
        for (int sendIndex = owner.getNumChildren() - 1;
             sendIndex >= 0; --sendIndex)
        {
            const auto send = owner.getChild (sendIndex);
            if (send.hasType (modelIds::mixerSend)
                && send.getProperty (modelIds::destinationId).toString()
                    == trackId)
                owner.removeChild (sendIndex, &undoManager);
        }
    }

    for (const auto& sourceId : referencedSources)
    {
        bool stillReferenced = false;

        for (int index = 0; index < getTrackCount() && ! stillReferenced; ++index)
        {
            const auto otherTrack = findTrack (getTrackId (index));
            for (const auto child : otherTrack)
                if (child.hasType (modelIds::clip)
                    && child.getProperty (modelIds::sourceId).toString() == sourceId)
                    stillReferenced = true;
        }

        if (! stillReferenced)
        {
            const auto sourceNode = findMediaSource (sourceId);
            if (sourceNode.isValid())
                state.removeChild (sourceNode, &undoManager);
        }
    }

    suppressNotifications = false;
    notifyChanged();
    return true;
}

void ProjectModel::setTrackName (const juce::String& trackId, juce::String newName)
{
    auto trackNode = findTrack (trackId);
    newName = newName.trim();

    if (! trackNode.isValid() || newName.isEmpty()
        || trackNode.getProperty (modelIds::name).toString() == newName)
        return;

    beginUndoTransaction ("Rename Track");
    trackNode.setProperty (modelIds::name, newName, &undoManager);
}

void ProjectModel::setTrackGain (const juce::String& trackId, float newGain)
{
    auto trackNode = findTrack (trackId);
    if (trackNode.isValid())
        trackNode.setProperty (modelIds::gain, juce::jlimit (0.0f, 1.5f, newGain), &undoManager);
}

void ProjectModel::setTrackPan (const juce::String& trackId, float newPan)
{
    auto trackNode = findTrack (trackId);
    if (trackNode.isValid())
        trackNode.setProperty (modelIds::pan, juce::jlimit (-1.0f, 1.0f, newPan), &undoManager);
}

void ProjectModel::setTrackMuted (const juce::String& trackId, bool shouldBeMuted)
{
    auto trackNode = findTrack (trackId);
    if (trackNode.isValid())
    {
        beginUndoTransaction (shouldBeMuted ? "Mute Track" : "Unmute Track");
        trackNode.setProperty (modelIds::muted, shouldBeMuted, &undoManager);
    }
}

void ProjectModel::setTrackSolo (const juce::String& trackId, bool shouldBeSolo)
{
    auto trackNode = findTrack (trackId);
    if (trackNode.isValid())
    {
        beginUndoTransaction (shouldBeSolo ? "Solo Track" : "Unsolo Track");
        trackNode.setProperty (modelIds::solo, shouldBeSolo, &undoManager);
    }
}

void ProjectModel::setTrackRecordArmed (const juce::String& trackId,
                                        bool shouldBeArmed)
{
    auto target = findTrack (trackId);
    if (! target.isValid())
        return;

    if (static_cast<bool> (target.getProperty (
            modelIds::recordArmed, false)) == shouldBeArmed)
        return;

    beginUndoTransaction (shouldBeArmed ? "Arm Track for Recording"
                                        : "Disarm Track");
    target.setProperty (modelIds::recordArmed, shouldBeArmed, &undoManager);
}

void ProjectModel::setTrackInputRoute (const juce::String& trackId,
                                       int firstInputChannel,
                                       int channelCount)
{
    auto track = findTrack (trackId);
    if (! track.isValid())
        return;
    firstInputChannel = juce::jlimit (0, 63, firstInputChannel);
    channelCount = juce::jlimit (1, 2, channelCount);
    beginUndoTransaction ("Change Track Input Route");
    suppressNotifications = true;
    track.setProperty (modelIds::inputStartChannel,
                       firstInputChannel, &undoManager);
    track.setProperty (modelIds::inputChannelCount,
                       channelCount, &undoManager);
    suppressNotifications = false;
    notifyChanged();
}

void ProjectModel::setTrackRecordingTapPoint (
    const juce::String& trackId, RecordingTapPoint tapPoint)
{
    auto track = findTrack (trackId);
    if (! track.isValid())
        return;
    beginUndoTransaction ("Change Recording Tap Point");
    track.setProperty (modelIds::recordingTapPoint,
                       tapPoint == RecordingTapPoint::programOutput
                           ? "device-output" : "hardware-input",
                       &undoManager);
}

void ProjectModel::setTrackRecordOffset (const juce::String& trackId,
                                         int offsetSamples)
{
    auto track = findTrack (trackId);
    if (! track.isValid())
        return;
    beginUndoTransaction ("Change Track Record Offset");
    track.setProperty (modelIds::recordOffsetSamples,
                       juce::jlimit (-8192, 8192, offsetSamples),
                       &undoManager);
}

void ProjectModel::setInstrumentPlugin (const juce::String& trackId,
                                        juce::String pluginName,
                                        juce::String descriptionXml,
                                        juce::String stateBase64)
{
    auto track = findTrack (trackId);
    if (! track.isValid()
        || ! static_cast<bool> (track.getProperty (modelIds::isInstrument, false))
        || static_cast<bool> (track.getProperty (modelIds::frozen, false))
        || descriptionXml.trim().isEmpty())
        return;

    beginUndoTransaction ("Load Instrument Plug-in");
    suppressNotifications = true;
    track.setProperty (modelIds::pluginName, pluginName.trim(), &undoManager);
    track.setProperty (modelIds::pluginDescriptionXml,
                       descriptionXml, &undoManager);
    track.setProperty (modelIds::pluginStateBase64,
                       stateBase64, &undoManager);
    track.setProperty (modelIds::pluginBypassed, false, &undoManager);
    track.setProperty (modelIds::pluginAvailable, true, &undoManager);
    auto instrumentSlot = juce::ValueTree {};
    for (const auto child : track)
        if (child.hasType (modelIds::pluginSlot)
            && child.getProperty (modelIds::role).toString() == "instrument")
        {
            instrumentSlot = child;
            break;
        }
    if (! instrumentSlot.isValid())
    {
        PluginSlotState slot;
        slot.id = juce::Uuid().toString();
        slot.stablePluginId = pluginName.trim() + ":"
            + juce::String::toHexString (descriptionXml.hashCode64());
        slot.name = pluginName.trim();
        slot.descriptionXml = descriptionXml;
        slot.stateBase64 = stateBase64;
        slot.role = PluginSlotRole::instrument;
        slot.order = 0;
        slot.layout = { 0, 2, 0, true };
        addPluginSlotNode (track, slot, &undoManager);
    }
    else
    {
        instrumentSlot.setProperty (modelIds::name,
                                    pluginName.trim(), &undoManager);
        instrumentSlot.setProperty (modelIds::descriptionXml,
                                    descriptionXml, &undoManager);
        instrumentSlot.setProperty (modelIds::stateBase64,
                                    stateBase64, &undoManager);
        instrumentSlot.setProperty (modelIds::pluginBypassed,
                                    false, &undoManager);
        instrumentSlot.setProperty (modelIds::available,
                                    true, &undoManager);
    }
    suppressNotifications = false;
    notifyChanged();
}

void ProjectModel::setBuiltInInstrument (const juce::String& trackId,
                                         juce::String instrumentName)
{
    auto track = findTrack (trackId);
    if (! track.isValid()
        || ! static_cast<bool> (track.getProperty (modelIds::isInstrument, false))
        || static_cast<bool> (track.getProperty (modelIds::frozen, false)))
        return;

    auto normalised = instrumentName.trim();
    if (normalised.isEmpty())
        normalised = "Triumph Piano";

    if (track.getProperty (modelIds::pluginName).toString() == normalised
        && track.getProperty (modelIds::pluginDescriptionXml).toString().isEmpty())
        return;

    beginUndoTransaction ("Change Track Instrument");
    suppressNotifications = true;
    track.setProperty (modelIds::pluginName, normalised, &undoManager);
    track.setProperty (modelIds::pluginDescriptionXml, {}, &undoManager);
    track.setProperty (modelIds::pluginStateBase64, {}, &undoManager);
    track.setProperty (modelIds::pluginBypassed, false, &undoManager);
    track.setProperty (modelIds::pluginLatencySamples, 0, &undoManager);
    track.setProperty (modelIds::pluginAvailable, true, &undoManager);
    for (int index = track.getNumChildren(); index-- > 0;)
        if (track.getChild (index).hasType (modelIds::pluginSlot)
            && track.getChild (index).getProperty (
                modelIds::role).toString() == "instrument")
            track.removeChild (index, &undoManager);
    suppressNotifications = false;
    notifyChanged();
}

void ProjectModel::setInstrumentPluginState (const juce::String& trackId,
                                             juce::String stateBase64)
{
    auto track = findTrack (trackId);
    if (! track.isValid()
        || ! static_cast<bool> (track.getProperty (modelIds::isInstrument, false)))
        return;
    if (track.getProperty (modelIds::pluginStateBase64).toString() == stateBase64)
        return;
    track.setProperty (modelIds::pluginStateBase64, stateBase64, nullptr);
    for (auto child : track)
        if (child.hasType (modelIds::pluginSlot)
            && child.getProperty (modelIds::role).toString() == "instrument")
        {
            child.setProperty (modelIds::stateBase64, stateBase64, nullptr);
            break;
        }
}

void ProjectModel::setInstrumentPluginBypassed (const juce::String& trackId,
                                                bool shouldBeBypassed)
{
    auto track = findTrack (trackId);
    if (! track.isValid()
        || ! static_cast<bool> (track.getProperty (modelIds::isInstrument, false)))
        return;

    beginUndoTransaction (shouldBeBypassed ? "Bypass Instrument Plug-in"
                                           : "Enable Instrument Plug-in");
    track.setProperty (modelIds::pluginBypassed, shouldBeBypassed, &undoManager);
    for (auto child : track)
        if (child.hasType (modelIds::pluginSlot)
            && child.getProperty (modelIds::role).toString() == "instrument")
        {
            child.setProperty (modelIds::pluginBypassed,
                               shouldBeBypassed, &undoManager);
            break;
        }
}

void ProjectModel::setInstrumentPluginHealth (const juce::String& trackId,
                                              int latencySamples,
                                              bool isAvailable)
{
    auto track = findTrack (trackId);
    if (! track.isValid()
        || ! static_cast<bool> (track.getProperty (modelIds::isInstrument, false)))
        return;

    suppressNotifications = true;
    track.setProperty (modelIds::pluginLatencySamples,
                       juce::jmax (0, latencySamples), nullptr);
    track.setProperty (modelIds::pluginAvailable, isAvailable, nullptr);
    for (auto child : track)
        if (child.hasType (modelIds::pluginSlot)
            && child.getProperty (modelIds::role).toString() == "instrument")
        {
            child.setProperty (modelIds::latencySamples,
                               juce::jmax (0, latencySamples), nullptr);
            child.setProperty (modelIds::available, isAvailable, nullptr);
            break;
        }
    suppressNotifications = false;
    notifyChanged();
}

std::vector<PluginSlotState> ProjectModel::getPluginSlots (
    const juce::String& ownerId) const
{
    const auto owner = findMixerOwner (ownerId);
    return owner.isValid() ? pluginSlotsFromNode (owner)
                           : std::vector<PluginSlotState> {};
}

juce::String ProjectModel::addPluginSlot (const juce::String& ownerId,
                                          PluginSlotState slot)
{
    auto owner = findMixerOwner (ownerId);
    if (! owner.isValid() || slot.descriptionXml.trim().isEmpty()
        || getPluginSlots (ownerId).size() >= 16)
        return {};
    slot.id = slot.id.isNotEmpty() ? slot.id : juce::Uuid().toString();
    slot.stablePluginId = slot.stablePluginId.isNotEmpty()
        ? slot.stablePluginId
        : slot.name + ":" + juce::String::toHexString (
            slot.descriptionXml.hashCode64());
    for (const auto& existing : getPluginSlots (ownerId))
        if (existing.id == slot.id
            || (slot.role == PluginSlotRole::instrument
                && existing.role == PluginSlotRole::instrument))
            return {};
    beginUndoTransaction (slot.role == PluginSlotRole::instrument
                              ? "Add Instrument Plug-in"
                              : "Add Insert Plug-in");
    juce::ValueTree node (modelIds::pluginSlot);
    node.setProperty (modelIds::id, slot.id, nullptr);
    node.setProperty (modelIds::stablePluginId, slot.stablePluginId, nullptr);
    node.setProperty (modelIds::name, slot.name, nullptr);
    node.setProperty (modelIds::descriptionXml, slot.descriptionXml, nullptr);
    node.setProperty (modelIds::stateBase64, slot.stateBase64, nullptr);
    node.setProperty (modelIds::role,
                      slot.role == PluginSlotRole::instrument
                          ? "instrument" : "insert", nullptr);
    node.setProperty (modelIds::isolation,
                      slot.isolation == PluginIsolationMode::trustedInProcess
                          ? "trusted-in-process" : "worker", nullptr);
    node.setProperty (modelIds::order,
                      juce::jlimit (0, 15, slot.order), nullptr);
    setBusLayoutProperties (node, slot.layout);
    node.setProperty (modelIds::pluginBypassed, slot.bypassed, nullptr);
    node.setProperty (modelIds::available, slot.available, nullptr);
    node.setProperty (modelIds::latencySamples,
                      juce::jlimit (0, 262143, slot.latencySamples), nullptr);
    node.setProperty (modelIds::restartCount,
                      juce::jmax (0, slot.restartCount), nullptr);
    owner.addChild (node, -1, &undoManager);
    return slot.id;
}

bool ProjectModel::updatePluginSlot (const juce::String& ownerId,
                                     const PluginSlotState& slot)
{
    auto node = findPluginSlot (ownerId, slot.id);
    if (! node.isValid() || slot.stablePluginId.isEmpty()
        || slot.descriptionXml.isEmpty())
        return false;
    beginUndoTransaction ("Update Plug-in Slot");
    suppressNotifications = true;
    node.setProperty (modelIds::stablePluginId,
                      slot.stablePluginId, &undoManager);
    node.setProperty (modelIds::name, slot.name, &undoManager);
    node.setProperty (modelIds::descriptionXml,
                      slot.descriptionXml, &undoManager);
    node.setProperty (modelIds::stateBase64,
                      slot.stateBase64, &undoManager);
    node.setProperty (modelIds::role,
                      slot.role == PluginSlotRole::instrument
                          ? "instrument" : "insert", &undoManager);
    node.setProperty (modelIds::isolation,
                      slot.isolation == PluginIsolationMode::trustedInProcess
                          ? "trusted-in-process" : "worker", &undoManager);
    node.setProperty (modelIds::order,
                      juce::jlimit (0, 15, slot.order), &undoManager);
    node.setProperty (modelIds::inputChannels,
                      juce::jlimit (0, 16, slot.layout.inputChannels), &undoManager);
    node.setProperty (modelIds::outputChannels,
                      juce::jlimit (1, 16, slot.layout.outputChannels), &undoManager);
    node.setProperty (modelIds::sidechainChannels,
                      juce::jlimit (0, 16, slot.layout.sidechainChannels), &undoManager);
    node.setProperty (modelIds::explicitLayout,
                      slot.layout.explicitLayout, &undoManager);
    node.setProperty (modelIds::pluginBypassed, slot.bypassed, &undoManager);
    node.setProperty (modelIds::available, slot.available, &undoManager);
    node.setProperty (modelIds::latencySamples,
                      juce::jlimit (0, 262143, slot.latencySamples), &undoManager);
    node.setProperty (modelIds::restartCount,
                      juce::jmax (0, slot.restartCount), &undoManager);
    suppressNotifications = false;
    notifyChanged();
    return true;
}

bool ProjectModel::removePluginSlot (const juce::String& ownerId,
                                     const juce::String& slotId)
{
    auto owner = findMixerOwner (ownerId);
    auto slot = findPluginSlot (ownerId, slotId);
    if (! owner.isValid() || ! slot.isValid())
        return false;
    beginUndoTransaction ("Remove Plug-in Slot");
    owner.removeChild (slot, &undoManager);
    return true;
}

void ProjectModel::setTrackManualLatencyOffset (const juce::String& trackId,
                                                int offsetSamples,
                                                bool beginNewTransaction)
{
    auto track = findTrack (trackId);
    if (! track.isValid())
        return;
    offsetSamples = juce::jlimit (-262143, 262143, offsetSamples);
    if (static_cast<int> (track.getProperty (
            modelIds::manualLatencyOffsetSamples, 0)) == offsetSamples)
        return;
    if (beginNewTransaction)
        beginUndoTransaction ("Adjust Manual Latency Offset");
    track.setProperty (modelIds::manualLatencyOffsetSamples,
                       offsetSamples, &undoManager);
}

void ProjectModel::setTrackOutputDestination (const juce::String& trackId,
                                              juce::String destinationId)
{
    auto track = findTrack (trackId);
    destinationId = destinationId.trim();
    if (! track.isValid())
        return;
    if (destinationId.isEmpty())
        destinationId = "master";
    if (destinationId != "master"
        && (! findMixerChannel (destinationId).isValid()
            || destinationId == trackId))
        return;
    if (track.getProperty (modelIds::outputDestinationId, "master").toString()
        == destinationId)
        return;
    beginUndoTransaction ("Route Track Output");
    track.setProperty (modelIds::outputDestinationId,
                       destinationId, &undoManager);
}

bool ProjectModel::freezeInstrumentTrack (const juce::String& trackId,
                                          const juce::File& renderedFile,
                                          double sampleRate,
                                          int channels,
                                          juce::int64 lengthInSamples)
{
    auto track = findTrack (trackId);
    if (! track.isValid()
        || ! static_cast<bool> (track.getProperty (modelIds::isInstrument, false))
        || static_cast<bool> (track.getProperty (modelIds::frozen, false))
        || track.getProperty (modelIds::pluginName).toString().trim().isEmpty()
        || ! renderedFile.existsAsFile() || sampleRate <= 0.0
        || channels <= 0 || lengthInSamples <= 0)
        return false;

    const auto sourceId = juce::Uuid().toString();
    juce::ValueTree sourceNode (modelIds::mediaSource);
    sourceNode.setProperty (modelIds::id, sourceId, nullptr);
    sourceNode.setProperty (modelIds::path, renderedFile.getFullPathName(), nullptr);
    sourceNode.setProperty (modelIds::sampleRate, sampleRate, nullptr);
    sourceNode.setProperty (modelIds::channels, channels, nullptr);
    sourceNode.setProperty (modelIds::lengthInSamples, lengthInSamples, nullptr);
    sourceNode.setProperty (modelIds::assetFingerprint,
                            makeAssetFingerprint (
                                renderedFile, sampleRate, channels,
                                lengthInSamples),
                            nullptr);

    juce::ValueTree clipNode (modelIds::clip);
    clipNode.setProperty (modelIds::id, juce::Uuid().toString(), nullptr);
    clipNode.setProperty (modelIds::name, "Frozen Instrument", nullptr);
    clipNode.setProperty (modelIds::sourceId, sourceId, nullptr);
    clipNode.setProperty (modelIds::timelineStartSamples,
                          static_cast<juce::int64> (0), nullptr);
    clipNode.setProperty (modelIds::sourceOffsetSamples,
                          static_cast<juce::int64> (0), nullptr);
    clipNode.setProperty (modelIds::lengthInSamples,
                          juce::jmax (static_cast<juce::int64> (1),
                                      static_cast<juce::int64> (
                                          timeline::convertSampleCount (
                                              lengthInSamples, sampleRate,
                                              getTimelineSampleRate()))), nullptr);
    clipNode.setProperty (modelIds::gain, 1.0f, nullptr);
    clipNode.setProperty (modelIds::colour,
                          static_cast<juce::int64> (0x00000000), nullptr);
    clipNode.setProperty (modelIds::muted, false, nullptr);
    clipNode.setProperty (modelIds::locked, false, nullptr);
    clipNode.setProperty (modelIds::takeGroupId, {}, nullptr);
    clipNode.setProperty (modelIds::takeLaneIndex, 0, nullptr);
    clipNode.setProperty (modelIds::activeTake, true, nullptr);

    beginUndoTransaction ("Freeze Instrument Track");
    suppressNotifications = true;
    state.addChild (sourceNode, -1, &undoManager);
    track.addChild (clipNode, -1, &undoManager);
    track.setProperty (modelIds::freezeSourceId, sourceId, &undoManager);
    track.setProperty (modelIds::frozen, true, &undoManager);
    suppressNotifications = false;
    notifyChanged();
    return true;
}

bool ProjectModel::unfreezeInstrumentTrack (const juce::String& trackId)
{
    auto track = findTrack (trackId);
    if (! track.isValid()
        || ! static_cast<bool> (track.getProperty (modelIds::frozen, false)))
        return false;

    const auto sourceId = track.getProperty (modelIds::freezeSourceId).toString();
    beginUndoTransaction ("Unfreeze Instrument Track");
    suppressNotifications = true;

    for (int index = track.getNumChildren() - 1; index >= 0; --index)
    {
        const auto child = track.getChild (index);
        if (child.hasType (modelIds::clip)
            && child.getProperty (modelIds::sourceId).toString() == sourceId)
            track.removeChild (index, &undoManager);
    }

    auto source = findMediaSource (sourceId);
    if (source.isValid())
        state.removeChild (source, &undoManager);
    track.setProperty (modelIds::freezeSourceId, {}, &undoManager);
    track.setProperty (modelIds::frozen, false, &undoManager);
    suppressNotifications = false;
    notifyChanged();
    return true;
}

void ProjectModel::clearInstrumentPlugin (const juce::String& trackId)
{
    auto track = findTrack (trackId);
    if (! track.isValid()
        || ! static_cast<bool> (track.getProperty (modelIds::isInstrument, false)))
        return;

    beginUndoTransaction ("Remove Instrument Plug-in");
    suppressNotifications = true;
    track.setProperty (modelIds::pluginName, {}, &undoManager);
    track.setProperty (modelIds::pluginDescriptionXml, {}, &undoManager);
    track.setProperty (modelIds::pluginStateBase64, {}, &undoManager);
    track.setProperty (modelIds::pluginBypassed, false, &undoManager);
    track.setProperty (modelIds::pluginLatencySamples, 0, &undoManager);
    track.setProperty (modelIds::pluginAvailable, true, &undoManager);
    for (int index = track.getNumChildren(); index-- > 0;)
        if (track.getChild (index).hasType (modelIds::pluginSlot)
            && track.getChild (index).getProperty (
                modelIds::role).toString() == "instrument")
            track.removeChild (index, &undoManager);
    suppressNotifications = false;
    notifyChanged();
}

juce::String ProjectModel::getArmedTrackId() const
{
    for (int index = 0; index < getTrackCount(); ++index)
    {
        const auto trackId = getTrackId (index);
        if (getTrackState (trackId).recordArmed)
            return trackId;
    }

    return {};
}

std::vector<juce::String> ProjectModel::getArmedTrackIds() const
{
    std::vector<juce::String> result;
    for (int index = 0; index < getTrackCount(); ++index)
    {
        const auto trackId = getTrackId (index);
        if (getTrackState (trackId).recordArmed)
            result.push_back (trackId);
    }
    return result;
}

std::vector<MixerChannelState> ProjectModel::getMixerChannels() const
{
    std::vector<MixerChannelState> result;
    for (const auto child : state)
        if (child.hasType (modelIds::mixerChannel))
            result.push_back (getMixerChannelState (
                child.getProperty (modelIds::id).toString()));
    return result;
}

MixerChannelState ProjectModel::getMixerChannelState (
    const juce::String& channelId) const
{
    MixerChannelState result;
    const auto node = findMixerChannel (channelId);
    if (! node.isValid())
        return result;
    result.id = node.getProperty (modelIds::id).toString();
    result.name = node.getProperty (modelIds::name).toString();
    result.kind = mixerKindFromName (
        node.getProperty (modelIds::kind, "bus").toString());
    result.gain = juce::jlimit (0.0f, 1.5f, static_cast<float> (
        node.getProperty (modelIds::gain, 0.85)));
    result.pan = juce::jlimit (-1.0f, 1.0f, static_cast<float> (
        node.getProperty (modelIds::pan, 0.0)));
    result.muted = static_cast<bool> (node.getProperty (modelIds::muted, false));
    result.solo = static_cast<bool> (node.getProperty (modelIds::solo, false));
    result.outputDestinationId = node.getProperty (
        modelIds::outputDestinationId, "master").toString();
    if (result.outputDestinationId.isEmpty())
        result.outputDestinationId = "master";
    result.sends = mixerSendsFromNode (node);
    result.layout = busLayoutFromNode (node);
    result.pluginSlots = pluginSlotsFromNode (node);
    return result;
}

juce::String ProjectModel::addMixerChannel (MixerChannelKind kind,
                                            juce::String name)
{
    const auto id = juce::Uuid().toString();
    if (name.trim().isEmpty())
    {
        const auto existingChannels = getMixerChannels();
        const auto sameKind = static_cast<int> (std::count_if (
            existingChannels.begin(), existingChannels.end(),
            [kind] (const auto& channel) { return channel.kind == kind; }));
        name = kind == MixerChannelKind::returnChannel
                   ? "FX Return " + juce::String (sameKind + 1)
                   : "Mix Bus " + juce::String (sameKind + 1);
    }

    juce::ValueTree node (modelIds::mixerChannel);
    node.setProperty (modelIds::id, id, nullptr);
    node.setProperty (modelIds::name, name.trim(), nullptr);
    node.setProperty (modelIds::kind, mixerKindName (kind), nullptr);
    node.setProperty (modelIds::gain, 0.85f, nullptr);
    node.setProperty (modelIds::pan, 0.0f, nullptr);
    node.setProperty (modelIds::muted, false, nullptr);
    node.setProperty (modelIds::solo, false, nullptr);
    node.setProperty (modelIds::outputDestinationId, "master", nullptr);
    beginUndoTransaction (kind == MixerChannelKind::returnChannel
                              ? "Create Return Channel" : "Create Bus Channel");
    state.addChild (node, -1, &undoManager);
    return id;
}

bool ProjectModel::removeMixerChannel (const juce::String& channelId)
{
    const auto target = findMixerChannel (channelId);
    if (! target.isValid())
        return false;
    beginUndoTransaction ("Delete Mixer Channel");
    suppressNotifications = true;
    for (int index = state.getNumChildren(); index-- > 0;)
    {
        const auto child = state.getChild (index);
        if (child.hasType (modelIds::automationLane)
            && child.getProperty (modelIds::targetId).toString() == channelId)
            state.removeChild (index, &undoManager);
    }
    for (int ownerIndex = state.getNumChildren() - 1; ownerIndex >= 0; --ownerIndex)
    {
        auto owner = state.getChild (ownerIndex);
        if (! owner.hasType (modelIds::track)
            && ! owner.hasType (modelIds::mixerChannel))
            continue;
        if (owner.getProperty (modelIds::outputDestinationId, "master").toString()
            == channelId)
            owner.setProperty (modelIds::outputDestinationId, "master", &undoManager);
        for (int sendIndex = owner.getNumChildren() - 1;
             sendIndex >= 0; --sendIndex)
        {
            const auto send = owner.getChild (sendIndex);
            if (send.hasType (modelIds::mixerSend)
                && send.getProperty (modelIds::destinationId).toString()
                    == channelId)
                owner.removeChild (sendIndex, &undoManager);
        }
    }
    state.removeChild (target, &undoManager);
    suppressNotifications = false;
    notifyChanged();
    return true;
}

void ProjectModel::setMixerChannelName (const juce::String& channelId,
                                        juce::String name)
{
    auto node = findMixerChannel (channelId);
    name = name.trim();
    if (! node.isValid() || name.isEmpty()
        || node.getProperty (modelIds::name).toString() == name)
        return;
    beginUndoTransaction ("Rename Mixer Channel");
    node.setProperty (modelIds::name, name, &undoManager);
}

void ProjectModel::setMixerChannelGain (const juce::String& channelId, float gain)
{
    auto node = findMixerChannel (channelId);
    if (node.isValid())
        node.setProperty (modelIds::gain, juce::jlimit (0.0f, 1.5f, gain),
                          &undoManager);
}

void ProjectModel::setMixerChannelPan (const juce::String& channelId, float pan)
{
    auto node = findMixerChannel (channelId);
    if (node.isValid())
        node.setProperty (modelIds::pan, juce::jlimit (-1.0f, 1.0f, pan),
                          &undoManager);
}

void ProjectModel::setMixerChannelMuted (const juce::String& channelId, bool muted)
{
    auto node = findMixerChannel (channelId);
    if (! node.isValid())
        return;
    beginUndoTransaction (muted ? "Mute Mixer Channel" : "Unmute Mixer Channel");
    node.setProperty (modelIds::muted, muted, &undoManager);
}

void ProjectModel::setMixerChannelSolo (const juce::String& channelId, bool solo)
{
    auto node = findMixerChannel (channelId);
    if (! node.isValid())
        return;
    beginUndoTransaction (solo ? "Solo Mixer Channel" : "Unsolo Mixer Channel");
    node.setProperty (modelIds::solo, solo, &undoManager);
}

void ProjectModel::setMixerChannelOutputDestination (
    const juce::String& channelId, juce::String destinationId)
{
    auto node = findMixerChannel (channelId);
    destinationId = destinationId.trim();
    if (! node.isValid())
        return;
    if (destinationId.isEmpty())
        destinationId = "master";
    if (destinationId != "master"
        && (! findMixerChannel (destinationId).isValid()
            || destinationId == channelId))
        return;
    if (node.getProperty (modelIds::outputDestinationId, "master").toString()
        == destinationId)
        return;
    beginUndoTransaction ("Route Mixer Channel Output");
    node.setProperty (modelIds::outputDestinationId,
                      destinationId, &undoManager);
}

juce::String ProjectModel::addMixerSend (const juce::String& ownerId,
                                         const juce::String& destinationId,
                                         bool sidechain)
{
    auto owner = findMixerOwner (ownerId);
    if (! owner.isValid() || destinationId.isEmpty()
        || destinationId == ownerId)
        return {};
    if (sidechain)
    {
        if (! findMixerOwner (destinationId).isValid())
            return {};
    }
    else if (! findMixerChannel (destinationId).isValid())
        return {};

    MixerSendState send;
    send.id = juce::Uuid().toString();
    send.destinationId = destinationId;
    send.gain = sidechain ? 1.0f : 0.25f;
    send.sidechain = sidechain;
    juce::ValueTree node (modelIds::mixerSend);
    node.setProperty (modelIds::id, send.id, nullptr);
    node.setProperty (modelIds::destinationId, send.destinationId, nullptr);
    node.setProperty (modelIds::gain, send.gain, nullptr);
    node.setProperty (modelIds::enabled, true, nullptr);
    node.setProperty (modelIds::preFader, false, nullptr);
    node.setProperty (modelIds::sidechain, send.sidechain, nullptr);
    beginUndoTransaction (sidechain ? "Create Sidechain Route" : "Create Send");
    owner.addChild (node, -1, &undoManager);
    return send.id;
}

bool ProjectModel::removeMixerSend (const juce::String& ownerId,
                                    const juce::String& sendId)
{
    auto owner = findMixerOwner (ownerId);
    auto send = findMixerSend (ownerId, sendId);
    if (! owner.isValid() || ! send.isValid())
        return false;
    beginUndoTransaction ("Delete Send");
    owner.removeChild (send, &undoManager);
    return true;
}

void ProjectModel::setMixerSendDestination (const juce::String& ownerId,
                                            const juce::String& sendId,
                                            juce::String destinationId)
{
    auto send = findMixerSend (ownerId, sendId);
    if (! send.isValid() || destinationId == ownerId)
        return;
    const auto sidechain = static_cast<bool> (send.getProperty (
        modelIds::sidechain, false));
    if ((sidechain && ! findMixerOwner (destinationId).isValid())
        || (! sidechain && ! findMixerChannel (destinationId).isValid()))
        return;
    beginUndoTransaction ("Route Send");
    send.setProperty (modelIds::destinationId, destinationId, &undoManager);
}

void ProjectModel::setMixerSendGain (const juce::String& ownerId,
                                     const juce::String& sendId,
                                     float gain)
{
    auto send = findMixerSend (ownerId, sendId);
    if (send.isValid())
        send.setProperty (modelIds::gain, juce::jlimit (0.0f, 2.0f, gain),
                          &undoManager);
}

void ProjectModel::setMixerSendEnabled (const juce::String& ownerId,
                                        const juce::String& sendId,
                                        bool enabled)
{
    auto send = findMixerSend (ownerId, sendId);
    if (! send.isValid())
        return;
    beginUndoTransaction (enabled ? "Enable Send" : "Disable Send");
    send.setProperty (modelIds::enabled, enabled, &undoManager);
}

void ProjectModel::setMixerSendPreFader (const juce::String& ownerId,
                                         const juce::String& sendId,
                                         bool preFader)
{
    auto send = findMixerSend (ownerId, sendId);
    if (! send.isValid())
        return;
    beginUndoTransaction (preFader ? "Set Pre-Fader Send"
                                   : "Set Post-Fader Send");
    send.setProperty (modelIds::preFader, preFader, &undoManager);
}

void ProjectModel::setMixerSendSidechain (const juce::String& ownerId,
                                          const juce::String& sendId,
                                          bool sidechain)
{
    auto send = findMixerSend (ownerId, sendId);
    if (! send.isValid())
        return;
    const auto destinationId = send.getProperty (modelIds::destinationId).toString();
    if (sidechain && ! findMixerOwner (destinationId).isValid())
        return;
    if (! sidechain && ! findMixerChannel (destinationId).isValid())
        return;
    beginUndoTransaction (sidechain ? "Convert Send to Sidechain"
                                    : "Convert Sidechain to Send");
    send.setProperty (modelIds::sidechain, sidechain, &undoManager);
}

bool ProjectModel::moveClip (const juce::String& trackId,
                             const juce::String& clipId,
                             juce::int64 newTimelineStartSamples)
{
    auto clipNode = findClip (trackId, clipId);
    newTimelineStartSamples = juce::jmax (static_cast<juce::int64> (0),
                                          newTimelineStartSamples);

    if (! clipNode.isValid()
        || static_cast<bool> (clipNode.getProperty (modelIds::locked, false))
        || static_cast<juce::int64> (clipNode.getProperty (
               modelIds::timelineStartSamples, 0)) == newTimelineStartSamples)
        return false;

    clipNode.setProperty (modelIds::timelineStartSamples,
                          newTimelineStartSamples,
                          &undoManager);
    return true;
}

bool ProjectModel::trimClipStart (const juce::String& trackId,
                                  const juce::String& clipId,
                                  juce::int64 newTimelineStartSamples)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid()
        || static_cast<bool> (clipNode.getProperty (modelIds::locked, false)))
        return false;

    const auto clip = getClipState (trackId, clipId);
    const auto source = getMediaSourceState (clip.sourceId);
    const auto oldEnd = clip.timelineStartSamples + clip.lengthInSamples;
    newTimelineStartSamples = juce::jlimit (static_cast<juce::int64> (0),
                                            oldEnd - 1,
                                            newTimelineStartSamples);
    const auto newLength = oldEnd - newTimelineStartSamples;
    const auto trimOffset = newTimelineStartSamples - clip.timelineStartSamples;
    const auto anchors = anchorsForClip (clip, source, getTimelineSampleRate());
    const auto newSourceOffset = warp::sourceAt (anchors, trimOffset);
    const auto sourceEnd = warp::sourceAt (anchors, clip.lengthInSamples);

    if (newTimelineStartSamples == clip.timelineStartSamples || newSourceOffset < 0
        || newSourceOffset > source.lengthInSamples)
        return false;

    clipNode.setProperty (modelIds::timelineStartSamples,
                          newTimelineStartSamples,
                          &undoManager);
    clipNode.setProperty (modelIds::sourceOffsetSamples,
                          static_cast<juce::int64> (newSourceOffset),
                          &undoManager);
    clipNode.setProperty (modelIds::lengthInSamples,
                          static_cast<juce::int64> (newLength), &undoManager);
    clipNode.setProperty (modelIds::fadeInSamples,
                          juce::jmin (clip.fadeInSamples, newLength),
                          &undoManager);
    clipNode.setProperty (modelIds::fadeOutSamples,
                          juce::jmin (clip.fadeOutSamples, newLength),
                          &undoManager);
    clipNode.setProperty (modelIds::timeStretchRatio,
                          ratioForSourceSpan (newLength,
                                              sourceEnd - newSourceOffset,
                                              source.sampleRate,
                                              getTimelineSampleRate()),
                          &undoManager);
    for (int index = clipNode.getNumChildren() - 1; index >= 0; --index)
    {
        auto marker = clipNode.getChild (index);
        if (! marker.hasType (modelIds::warpMarker)
            && ! marker.hasType (modelIds::transientMarker))
            continue;
        const auto markerOffset = static_cast<juce::int64> (marker.getProperty (
            modelIds::timelineOffsetSamples, 0));
        if (markerOffset <= trimOffset)
            clipNode.removeChild (index, &undoManager);
        else
            marker.setProperty (modelIds::timelineOffsetSamples,
                                markerOffset - trimOffset, &undoManager);
    }
    return true;
}

bool ProjectModel::trimClipEnd (const juce::String& trackId,
                                const juce::String& clipId,
                                juce::int64 newTimelineEndSamples)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid()
        || static_cast<bool> (clipNode.getProperty (modelIds::locked, false)))
        return false;

    const auto clip = getClipState (trackId, clipId);
    const auto source = getMediaSourceState (clip.sourceId);
    newTimelineEndSamples = juce::jmax (clip.timelineStartSamples + 1,
                                        newTimelineEndSamples);
    const auto newLength = newTimelineEndSamples - clip.timelineStartSamples;
    if (! clip.warpMarkers.empty() && newLength > clip.lengthInSamples)
        return false;
    const auto anchors = anchorsForClip (clip, source, getTimelineSampleRate());
    const auto requestedSourceEnd = warp::sourceAt (anchors, newLength);

    if (newLength == clip.lengthInSamples
        || requestedSourceEnd > source.lengthInSamples + 1)
        return false;

    clipNode.setProperty (modelIds::lengthInSamples, newLength, &undoManager);
    clipNode.setProperty (modelIds::fadeInSamples,
                          juce::jmin (clip.fadeInSamples, newLength),
                          &undoManager);
    clipNode.setProperty (modelIds::fadeOutSamples,
                          juce::jmin (clip.fadeOutSamples, newLength),
                          &undoManager);
    clipNode.setProperty (modelIds::timeStretchRatio,
                          ratioForSourceSpan (newLength,
                                              requestedSourceEnd
                                                  - clip.sourceOffsetSamples,
                                              source.sampleRate,
                                              getTimelineSampleRate()),
                          &undoManager);
    for (int index = clipNode.getNumChildren() - 1; index >= 0; --index)
        if ((clipNode.getChild (index).hasType (modelIds::warpMarker)
             || clipNode.getChild (index).hasType (modelIds::transientMarker))
            && static_cast<juce::int64> (clipNode.getChild (index).getProperty (
                   modelIds::timelineOffsetSamples, 0)) >= newLength)
            clipNode.removeChild (index, &undoManager);
    return true;
}

bool ProjectModel::stretchClipToEnd (const juce::String& trackId,
                                     const juce::String& clipId,
                                     juce::int64 newTimelineEndSamples)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid()
        || static_cast<bool> (clipNode.getProperty (modelIds::locked, false)))
        return false;

    const auto clip = getClipState (trackId, clipId);
    const auto nativeTimelineLength = juce::jmax (
        static_cast<juce::int64> (1),
        static_cast<juce::int64> (stretch::timelineToNativeSamples (
            clip.lengthInSamples, clip.timeStretchRatio)));
    const auto requestedLength = juce::jmax (
        static_cast<juce::int64> (1),
        newTimelineEndSamples - clip.timelineStartSamples);
    const auto ratio = stretch::clampRatio (
        static_cast<double> (requestedLength) / nativeTimelineLength);
    const auto newLength = stretch::nativeToTimelineSamples (nativeTimelineLength,
                                                              ratio);
    if (newLength == clip.lengthInSamples
        && std::abs (ratio - clip.timeStretchRatio) < 0.000001)
        return false;

    const auto scale = static_cast<double> (newLength)
                       / static_cast<double> (clip.lengthInSamples);
    clipNode.setProperty (modelIds::lengthInSamples,
                          static_cast<juce::int64> (newLength), &undoManager);
    clipNode.setProperty (modelIds::timeStretchRatio, ratio, &undoManager);
    clipNode.setProperty (modelIds::timeStretchProvider,
                          stretch::builtInProviderId, &undoManager);
    clipNode.setProperty (modelIds::fadeInSamples,
                          static_cast<juce::int64> (std::llround (
                              static_cast<double> (clip.fadeInSamples) * scale)),
                          &undoManager);
    clipNode.setProperty (modelIds::fadeOutSamples,
                          static_cast<juce::int64> (std::llround (
                              static_cast<double> (clip.fadeOutSamples) * scale)),
                          &undoManager);
    for (auto markerNode : clipNode)
        if (markerNode.hasType (modelIds::warpMarker)
            || markerNode.hasType (modelIds::transientMarker))
            markerNode.setProperty (modelIds::timelineOffsetSamples,
                                    static_cast<juce::int64> (std::llround (
                                        static_cast<double> (markerNode.getProperty (
                                            modelIds::timelineOffsetSamples, 0)) * scale)),
                                    &undoManager);
    return true;
}

bool ProjectModel::setClipReversed (const juce::String& trackId,
                                    const juce::String& clipId,
                                    bool shouldBeReversed)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid()
        || static_cast<bool> (clipNode.getProperty (modelIds::locked, false))
        || static_cast<bool> (clipNode.getProperty (
               modelIds::reversed, false)) == shouldBeReversed)
        return false;
    beginUndoTransaction (shouldBeReversed ? "Reverse Clip" : "Restore Clip Direction");
    clipNode.setProperty (modelIds::reversed, shouldBeReversed, &undoManager);
    return true;
}

bool ProjectModel::setClipGain (const juce::String& trackId,
                                const juce::String& clipId,
                                float newGain,
                                juce::String undoDescription)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid()
        || static_cast<bool> (clipNode.getProperty (modelIds::locked, false)))
        return false;
    newGain = juce::jlimit (0.0f, 16.0f, newGain);
    if (std::abs (static_cast<float> (clipNode.getProperty (
            modelIds::gain, 1.0)) - newGain) < 0.000001f)
        return false;
    beginUndoTransaction (undoDescription.trim().isNotEmpty()
                              ? undoDescription.trim() : "Change Clip Gain");
    clipNode.setProperty (modelIds::gain, newGain, &undoManager);
    return true;
}

bool ProjectModel::setClipFades (const juce::String& trackId,
                                 const juce::String& clipId,
                                 juce::int64 newFadeInSamples,
                                 juce::int64 newFadeOutSamples,
                                 juce::String undoDescription)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid()
        || static_cast<bool> (clipNode.getProperty (modelIds::locked, false)))
        return false;

    const auto clipLength = juce::jmax (
        static_cast<juce::int64> (0),
        static_cast<juce::int64> (clipNode.getProperty (
            modelIds::lengthInSamples, 0)));
    newFadeInSamples = juce::jlimit (
        static_cast<juce::int64> (0), clipLength, newFadeInSamples);
    newFadeOutSamples = juce::jlimit (
        static_cast<juce::int64> (0), clipLength, newFadeOutSamples);
    const auto currentFadeIn = static_cast<juce::int64> (
        clipNode.getProperty (modelIds::fadeInSamples, 0));
    const auto currentFadeOut = static_cast<juce::int64> (
        clipNode.getProperty (modelIds::fadeOutSamples, 0));
    if (currentFadeIn == newFadeInSamples
        && currentFadeOut == newFadeOutSamples)
        return false;

    beginUndoTransaction (undoDescription.trim().isNotEmpty()
                              ? undoDescription.trim()
                              : "Adjust Clip Fades");
    suppressNotifications = true;
    clipNode.setProperty (modelIds::fadeInSamples,
                          newFadeInSamples, &undoManager);
    clipNode.setProperty (modelIds::fadeOutSamples,
                          newFadeOutSamples, &undoManager);
    suppressNotifications = false;
    notifyChanged();
    return true;
}

bool ProjectModel::createClipCrossfade (const juce::String& trackId,
                                        const juce::String& leftClipId)
{
    auto leftNode = findClip (trackId, leftClipId);
    const auto track = getTrackState (trackId);
    const auto left = getClipState (trackId, leftClipId);
    if (! leftNode.isValid() || left.id.isEmpty() || left.locked)
        return false;

    const auto leftEnd = left.timelineStartSamples + left.lengthInSamples;
    const AudioClipState* right = nullptr;
    for (const auto& candidate : track.clips)
    {
        if (candidate.id == left.id || candidate.locked
            || candidate.timelineStartSamples <= left.timelineStartSamples
            || candidate.timelineStartSamples >= leftEnd)
            continue;
        if (right == nullptr
            || candidate.timelineStartSamples < right->timelineStartSamples)
            right = &candidate;
    }
    if (right == nullptr)
        return false;

    const auto rightEnd = right->timelineStartSamples + right->lengthInSamples;
    const auto overlap = juce::jmax (
        static_cast<juce::int64> (0),
        juce::jmin (leftEnd, rightEnd) - right->timelineStartSamples);
    if (overlap <= 0)
        return false;

    auto rightNode = findClip (trackId, right->id);
    if (! rightNode.isValid())
        return false;

    beginUndoTransaction ("Create Clip Crossfade");
    suppressNotifications = true;
    leftNode.setProperty (modelIds::fadeOutSamples, overlap, &undoManager);
    rightNode.setProperty (modelIds::fadeInSamples, overlap, &undoManager);
    suppressNotifications = false;
    notifyChanged();
    return true;
}

bool ProjectModel::setClipName (const juce::String& trackId,
                                const juce::String& clipId,
                                juce::String newName)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid())
        return false;
    newName = newName.trim().substring (0, 96);
    if (clipNode.getProperty (modelIds::name).toString() == newName)
        return false;
    beginUndoTransaction ("Rename Clip");
    clipNode.setProperty (modelIds::name, newName, &undoManager);
    return true;
}

bool ProjectModel::setClipMuted (const juce::String& trackId,
                                 const juce::String& clipId,
                                 bool shouldBeMuted)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid()
        || static_cast<bool> (clipNode.getProperty (
               modelIds::muted, false)) == shouldBeMuted)
        return false;
    beginUndoTransaction (shouldBeMuted ? "Mute Clip" : "Unmute Clip");
    clipNode.setProperty (modelIds::muted, shouldBeMuted, &undoManager);
    return true;
}

bool ProjectModel::setClipLocked (const juce::String& trackId,
                                  const juce::String& clipId,
                                  bool shouldBeLocked)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid()
        || static_cast<bool> (clipNode.getProperty (
               modelIds::locked, false)) == shouldBeLocked)
        return false;
    beginUndoTransaction (shouldBeLocked ? "Lock Clip" : "Unlock Clip");
    clipNode.setProperty (modelIds::locked, shouldBeLocked, &undoManager);
    return true;
}

bool ProjectModel::setClipColour (const juce::String& trackId,
                                  const juce::String& clipId,
                                  juce::Colour newColour)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid())
        return false;
    const auto argb = static_cast<juce::int64> (newColour.getARGB());
    if (static_cast<juce::int64> (clipNode.getProperty (
            modelIds::colour, static_cast<juce::int64> (0x00000000))) == argb)
        return false;
    beginUndoTransaction (newColour.isTransparent() ? "Use Track Colour"
                                                     : "Set Clip Colour");
    clipNode.setProperty (modelIds::colour, argb, &undoManager);
    return true;
}

bool ProjectModel::replaceClipWithProcessedAudio (
    const juce::String& trackId, const juce::String& clipId,
    const juce::File& renderedFile, double sampleRate, int channels,
    juce::int64 sourceLengthInSamples, juce::String undoDescription)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid()
        || static_cast<bool> (clipNode.getProperty (modelIds::locked, false))
        || ! renderedFile.existsAsFile()
        || sampleRate <= 0.0 || channels <= 0 || sourceLengthInSamples <= 0)
        return false;

    const auto sourceId = juce::Uuid().toString();
    juce::ValueTree sourceNode (modelIds::mediaSource);
    sourceNode.setProperty (modelIds::id, sourceId, nullptr);
    sourceNode.setProperty (modelIds::path, renderedFile.getFullPathName(), nullptr);
    sourceNode.setProperty (modelIds::sampleRate, sampleRate, nullptr);
    sourceNode.setProperty (modelIds::channels, channels, nullptr);
    sourceNode.setProperty (modelIds::lengthInSamples,
                            sourceLengthInSamples, nullptr);
    sourceNode.setProperty (modelIds::assetFingerprint,
                            makeAssetFingerprint (
                                renderedFile, sampleRate, channels,
                                sourceLengthInSamples),
                            nullptr);

    beginUndoTransaction (undoDescription.trim().isNotEmpty()
                              ? undoDescription.trim() : "Process Clip Audio");
    suppressNotifications = true;
    state.addChild (sourceNode, -1, &undoManager);
    clipNode.setProperty (modelIds::sourceId, sourceId, &undoManager);
    clipNode.setProperty (modelIds::sourceOffsetSamples,
                          static_cast<juce::int64> (0), &undoManager);
    clipNode.setProperty (modelIds::timeStretchRatio, 1.0, &undoManager);
    clipNode.setProperty (modelIds::timeStretchProvider,
                          stretch::builtInProviderId, &undoManager);
    clipNode.setProperty (modelIds::reversed, false, &undoManager);
    for (int index = clipNode.getNumChildren() - 1; index >= 0; --index)
        if (clipNode.getChild (index).hasType (modelIds::warpMarker)
            || clipNode.getChild (index).hasType (modelIds::transientMarker))
            clipNode.removeChild (index, &undoManager);
    suppressNotifications = false;
    notifyChanged();
    return true;
}

juce::String ProjectModel::addWarpMarker (const juce::String& trackId,
                                          const juce::String& clipId,
                                          juce::int64 timelineSample)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid()
        || static_cast<bool> (clipNode.getProperty (modelIds::locked, false)))
        return {};
    const auto clip = getClipState (trackId, clipId);
    const auto offset = timelineSample - clip.timelineStartSamples;
    if (offset <= 0 || offset >= clip.lengthInSamples)
        return {};
    for (const auto& marker : clip.warpMarkers)
        if (std::abs (marker.timelineOffsetSamples - offset) <= 1)
            return marker.id;

    const auto source = getMediaSourceState (clip.sourceId);
    const auto anchors = anchorsForClip (clip, source, getTimelineSampleRate());
    const auto markerId = juce::Uuid().toString();
    juce::ValueTree marker (modelIds::warpMarker);
    marker.setProperty (modelIds::id, markerId, nullptr);
    marker.setProperty (modelIds::timelineOffsetSamples, offset, nullptr);
    marker.setProperty (modelIds::sourceOffsetSamples,
                        static_cast<juce::int64> (
                            warp::sourceAt (anchors, offset)), nullptr);
    beginUndoTransaction ("Add Warp Marker");
    clipNode.addChild (marker, -1, &undoManager);
    return markerId;
}

bool ProjectModel::moveWarpMarker (const juce::String& trackId,
                                   const juce::String& clipId,
                                   const juce::String& markerId,
                                   juce::int64 timelineSample)
{
    auto clipNode = findClip (trackId, clipId);
    const auto clip = getClipState (trackId, clipId);
    if (! clipNode.isValid() || clip.id.isEmpty() || clip.locked)
        return false;
    auto markerNode = juce::ValueTree();
    for (auto child : clipNode)
        if (child.hasType (modelIds::warpMarker)
            && child.getProperty (modelIds::id).toString() == markerId)
            markerNode = child;
    if (! markerNode.isValid())
        return false;

    auto minimum = static_cast<juce::int64> (1);
    auto maximum = clip.lengthInSamples - 1;
    const auto current = static_cast<juce::int64> (markerNode.getProperty (
        modelIds::timelineOffsetSamples, 0));
    for (const auto& marker : clip.warpMarkers)
    {
        if (marker.id == markerId)
            continue;
        if (marker.timelineOffsetSamples < current)
            minimum = juce::jmax (minimum, marker.timelineOffsetSamples + 1);
        else if (marker.timelineOffsetSamples > current)
            maximum = juce::jmin (maximum, marker.timelineOffsetSamples - 1);
    }
    const auto offset = juce::jlimit (minimum, maximum,
                                      timelineSample - clip.timelineStartSamples);
    if (offset == current)
        return false;
    markerNode.setProperty (modelIds::timelineOffsetSamples, offset, &undoManager);
    return true;
}

bool ProjectModel::removeWarpMarker (const juce::String& trackId,
                                     const juce::String& clipId,
                                     const juce::String& markerId)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid()
        || static_cast<bool> (clipNode.getProperty (modelIds::locked, false)))
        return false;
    for (int index = 0; index < clipNode.getNumChildren(); ++index)
        if (clipNode.getChild (index).hasType (modelIds::warpMarker)
            && clipNode.getChild (index).getProperty (modelIds::id).toString()
                   == markerId)
        {
            beginUndoTransaction ("Remove Warp Marker");
            clipNode.removeChild (index, &undoManager);
            return true;
        }
    return false;
}

bool ProjectModel::setTransientMarkers (
    const juce::String& trackId, const juce::String& clipId,
    const std::vector<TransientMarkerState>& markers)
{
    auto clipNode = findClip (trackId, clipId);
    const auto clip = getClipState (trackId, clipId);
    if (! clipNode.isValid() || clip.id.isEmpty() || clip.locked)
        return false;
    const auto source = getMediaSourceState (clip.sourceId);
    const auto anchors = anchorsForClip (clip, source, getTimelineSampleRate());
    beginUndoTransaction ("Detect Transients");
    for (int index = clipNode.getNumChildren() - 1; index >= 0; --index)
        if (clipNode.getChild (index).hasType (modelIds::transientMarker))
            clipNode.removeChild (index, &undoManager);
    for (const auto& detection : markers)
    {
        const auto offset = juce::jlimit (static_cast<juce::int64> (1),
                                          clip.lengthInSamples - 1,
                                          static_cast<juce::int64> (
                                              warp::timelineAt (
                                                  anchors,
                                                  detection.sourceOffsetSamples)));
        juce::ValueTree marker (modelIds::transientMarker);
        marker.setProperty (modelIds::id,
                            detection.id.isNotEmpty() ? detection.id
                                                      : juce::Uuid().toString(), nullptr);
        marker.setProperty (modelIds::timelineOffsetSamples, offset, nullptr);
        marker.setProperty (modelIds::sourceOffsetSamples,
                            detection.sourceOffsetSamples, nullptr);
        marker.setProperty (modelIds::strength,
                            juce::jlimit (0.0f, 1.0f, detection.strength), nullptr);
        clipNode.addChild (marker, -1, &undoManager);
    }
    return true;
}

bool ProjectModel::clearTransientMarkers (const juce::String& trackId,
                                          const juce::String& clipId)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid()
        || static_cast<bool> (clipNode.getProperty (modelIds::locked, false)))
        return false;
    bool changed = false;
    beginUndoTransaction ("Clear Transients");
    for (int index = clipNode.getNumChildren() - 1; index >= 0; --index)
        if (clipNode.getChild (index).hasType (modelIds::transientMarker))
        {
            clipNode.removeChild (index, &undoManager);
            changed = true;
        }
    return changed;
}

int ProjectModel::convertTransientsToWarpMarkers (const juce::String& trackId,
                                                   const juce::String& clipId)
{
    auto clipNode = findClip (trackId, clipId);
    const auto clip = getClipState (trackId, clipId);
    if (! clipNode.isValid() || clip.locked || clip.transientMarkers.empty())
        return 0;
    beginUndoTransaction ("Convert Transients To Warp Markers");
    int added = 0;
    for (const auto& transient : clip.transientMarkers)
    {
        const auto duplicate = std::any_of (clip.warpMarkers.begin(),
                                            clip.warpMarkers.end(),
                                            [&transient] (const auto& warpMarker)
        {
            return std::abs (warpMarker.timelineOffsetSamples
                             - transient.timelineOffsetSamples) <= 1;
        });
        if (duplicate || transient.timelineOffsetSamples <= 0
            || transient.timelineOffsetSamples >= clip.lengthInSamples)
            continue;
        juce::ValueTree marker (modelIds::warpMarker);
        marker.setProperty (modelIds::id, juce::Uuid().toString(), nullptr);
        marker.setProperty (modelIds::timelineOffsetSamples,
                            transient.timelineOffsetSamples, nullptr);
        marker.setProperty (modelIds::sourceOffsetSamples,
                            transient.sourceOffsetSamples, nullptr);
        clipNode.addChild (marker, -1, &undoManager);
        ++added;
    }
    return added;
}

juce::String ProjectModel::splitClip (const juce::String& trackId,
                                      const juce::String& clipId,
                                      juce::int64 splitTimelineSample)
{
    auto trackNode = findTrack (trackId);
    auto clipNode = findClip (trackId, clipId);

    if (! trackNode.isValid() || ! clipNode.isValid()
        || static_cast<bool> (clipNode.getProperty (modelIds::locked, false)))
        return {};

    const auto clip = getClipState (trackId, clipId);
    const auto clipEnd = clip.timelineStartSamples + clip.lengthInSamples;
    if (splitTimelineSample <= clip.timelineStartSamples
        || splitTimelineSample >= clipEnd)
        return {};

    const auto firstLength = splitTimelineSample - clip.timelineStartSamples;
    const auto secondId = juce::Uuid().toString();
    const auto source = getMediaSourceState (clip.sourceId);
    const auto anchors = anchorsForClip (clip, source, getTimelineSampleRate());
    const auto secondSourceOffset = warp::sourceAt (anchors, firstLength);
    const auto originalSourceEnd = warp::sourceAt (anchors, clip.lengthInSamples);
    const auto firstRatio = ratioForSourceSpan (
        firstLength, secondSourceOffset - clip.sourceOffsetSamples,
        source.sampleRate, getTimelineSampleRate());
    const auto secondLength = clipEnd - splitTimelineSample;
    const auto secondRatio = ratioForSourceSpan (
        secondLength, originalSourceEnd - secondSourceOffset,
        source.sampleRate, getTimelineSampleRate());

    juce::ValueTree secondClip (modelIds::clip);
    secondClip.setProperty (modelIds::id, secondId, nullptr);
    secondClip.setProperty (modelIds::name, clip.name, nullptr);
    secondClip.setProperty (modelIds::sourceId, clip.sourceId, nullptr);
    secondClip.setProperty (modelIds::timelineStartSamples, splitTimelineSample, nullptr);
    secondClip.setProperty (modelIds::sourceOffsetSamples,
                            static_cast<juce::int64> (secondSourceOffset), nullptr);
    secondClip.setProperty (modelIds::lengthInSamples, secondLength, nullptr);
    secondClip.setProperty (modelIds::fadeInSamples, 0, nullptr);
    secondClip.setProperty (modelIds::fadeOutSamples,
                            juce::jmin (clip.fadeOutSamples, secondLength), nullptr);
    secondClip.setProperty (modelIds::gain, clip.gain, nullptr);
    secondClip.setProperty (modelIds::colour,
                            static_cast<juce::int64> (clip.colour.getARGB()), nullptr);
    secondClip.setProperty (modelIds::muted, clip.muted, nullptr);
    secondClip.setProperty (modelIds::locked, clip.locked, nullptr);
    secondClip.setProperty (modelIds::takeGroupId, clip.takeGroupId, nullptr);
    secondClip.setProperty (modelIds::takeLaneIndex, clip.takeLaneIndex, nullptr);
    secondClip.setProperty (modelIds::activeTake, clip.activeTake, nullptr);
    secondClip.setProperty (modelIds::timeStretchRatio,
                            secondRatio, nullptr);
    secondClip.setProperty (modelIds::timeStretchProvider,
                            clip.timeStretchProvider, nullptr);
    secondClip.setProperty (modelIds::reversed, clip.reversed, nullptr);

    for (const auto& marker : clip.warpMarkers)
        if (marker.timelineOffsetSamples > firstLength)
        {
            juce::ValueTree markerNode (modelIds::warpMarker);
            markerNode.setProperty (modelIds::id, juce::Uuid().toString(), nullptr);
            markerNode.setProperty (modelIds::timelineOffsetSamples,
                                    marker.timelineOffsetSamples - firstLength, nullptr);
            markerNode.setProperty (modelIds::sourceOffsetSamples,
                                    marker.sourceOffsetSamples, nullptr);
            secondClip.addChild (markerNode, -1, nullptr);
        }
    for (const auto& marker : clip.transientMarkers)
        if (marker.timelineOffsetSamples > firstLength)
        {
            juce::ValueTree markerNode (modelIds::transientMarker);
            markerNode.setProperty (modelIds::id, juce::Uuid().toString(), nullptr);
            markerNode.setProperty (modelIds::timelineOffsetSamples,
                                    marker.timelineOffsetSamples - firstLength, nullptr);
            markerNode.setProperty (modelIds::sourceOffsetSamples,
                                    marker.sourceOffsetSamples, nullptr);
            markerNode.setProperty (modelIds::strength, marker.strength, nullptr);
            secondClip.addChild (markerNode, -1, nullptr);
        }

    beginUndoTransaction ("Split Clip");
    clipNode.setProperty (modelIds::lengthInSamples, firstLength, &undoManager);
    clipNode.setProperty (modelIds::fadeInSamples,
                          juce::jmin (clip.fadeInSamples, firstLength),
                          &undoManager);
    clipNode.setProperty (modelIds::fadeOutSamples, 0, &undoManager);
    clipNode.setProperty (modelIds::timeStretchRatio, firstRatio, &undoManager);
    for (int index = clipNode.getNumChildren() - 1; index >= 0; --index)
        if ((clipNode.getChild (index).hasType (modelIds::warpMarker)
             || clipNode.getChild (index).hasType (modelIds::transientMarker))
            && static_cast<juce::int64> (clipNode.getChild (index).getProperty (
                   modelIds::timelineOffsetSamples, 0)) >= firstLength)
            clipNode.removeChild (index, &undoManager);
    const auto insertionIndex = trackNode.indexOf (clipNode) + 1;
    trackNode.addChild (secondClip, insertionIndex, &undoManager);
    return secondId;
}

juce::String ProjectModel::duplicateClip (
    const juce::String& trackId, const juce::String& clipId,
    juce::int64 newTimelineStartSamples)
{
    auto trackNode = findTrack (trackId);
    auto clipNode = findClip (trackId, clipId);
    if (! trackNode.isValid() || ! clipNode.isValid())
        return {};

    const auto clip = getClipState (trackId, clipId);
    if (newTimelineStartSamples < 0)
        newTimelineStartSamples = clip.timelineStartSamples
                                  + clip.lengthInSamples;
    newTimelineStartSamples = juce::jmax (static_cast<juce::int64> (0),
                                          newTimelineStartSamples);

    auto duplicate = clipNode.createCopy();
    const auto duplicateId = juce::Uuid().toString();
    duplicate.setProperty (modelIds::id, duplicateId, nullptr);
    duplicate.setProperty (modelIds::timelineStartSamples,
                           newTimelineStartSamples, nullptr);
    duplicate.setProperty (modelIds::takeGroupId, juce::String(), nullptr);
    duplicate.setProperty (modelIds::takeLaneIndex, 0, nullptr);
    duplicate.setProperty (modelIds::activeTake, true, nullptr);
    for (auto marker : duplicate)
        if (marker.hasType (modelIds::warpMarker)
            || marker.hasType (modelIds::transientMarker))
            marker.setProperty (modelIds::id, juce::Uuid().toString(), nullptr);
        else if (marker.hasType (modelIds::clipVariant))
            marker.setProperty (modelIds::id, juce::Uuid().toString(), nullptr);

    beginUndoTransaction ("Duplicate Clip");
    trackNode.addChild (duplicate, trackNode.indexOf (clipNode) + 1,
                        &undoManager);
    return duplicateId;
}

juce::String ProjectModel::captureClipVariant (const juce::String& trackId,
                                               const juce::String& clipId,
                                               juce::String variantName)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid())
        return {};

    const auto clip = getClipState (trackId, clipId);
    if (clip.sourceId.isEmpty() || clip.lengthInSamples <= 0)
        return {};

    const auto variantId = juce::Uuid().toString();
    auto variantNode = variantNodeFromClip (clip, variantId, std::move (variantName));
    beginUndoTransaction ("Capture Clip Variant");
    clipNode.addChild (variantNode, -1, &undoManager);
    return variantId;
}

bool ProjectModel::restoreClipVariant (const juce::String& trackId,
                                       const juce::String& clipId,
                                       const juce::String& variantId)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid()
        || static_cast<bool> (clipNode.getProperty (modelIds::locked, false)))
        return false;

    juce::ValueTree variantNode;
    for (const auto child : clipNode)
        if (child.hasType (modelIds::clipVariant)
            && child.getProperty (modelIds::id).toString() == variantId)
        {
            variantNode = child;
            break;
        }

    if (! variantNode.isValid())
        return false;

    const auto variant = variantFromNode (variantNode);
    if (variant.sourceId.isEmpty() || ! findMediaSource (variant.sourceId).isValid())
        return false;

    const auto length = juce::jmax (static_cast<juce::int64> (1),
                                   variant.lengthInSamples);
    beginUndoTransaction ("Restore Clip Variant");
    suppressNotifications = true;
    clipNode.setProperty (modelIds::sourceId, variant.sourceId, &undoManager);
    clipNode.setProperty (modelIds::sourceOffsetSamples,
                          juce::jmax (static_cast<juce::int64> (0),
                                      variant.sourceOffsetSamples),
                          &undoManager);
    clipNode.setProperty (modelIds::lengthInSamples, length, &undoManager);
    clipNode.setProperty (modelIds::fadeInSamples,
                          juce::jlimit (static_cast<juce::int64> (0), length,
                                        variant.fadeInSamples),
                          &undoManager);
    clipNode.setProperty (modelIds::fadeOutSamples,
                          juce::jlimit (static_cast<juce::int64> (0), length,
                                        variant.fadeOutSamples),
                          &undoManager);
    clipNode.setProperty (modelIds::gain,
                          juce::jmax (0.0f, variant.gain), &undoManager);
    clipNode.setProperty (modelIds::timeStretchRatio,
                          stretch::clampRatio (variant.timeStretchRatio),
                          &undoManager);
    clipNode.setProperty (modelIds::timeStretchProvider,
                          variant.timeStretchProvider.isNotEmpty()
                              ? variant.timeStretchProvider
                              : stretch::builtInProviderId,
                          &undoManager);
    clipNode.setProperty (modelIds::reversed, variant.reversed, &undoManager);
    for (int index = clipNode.getNumChildren() - 1; index >= 0; --index)
        if (clipNode.getChild (index).hasType (modelIds::warpMarker)
            || clipNode.getChild (index).hasType (modelIds::transientMarker))
            clipNode.removeChild (index, &undoManager);
    for (const auto& marker : variant.warpMarkers)
        if (marker.timelineOffsetSamples > 0
            && marker.timelineOffsetSamples < length)
            clipNode.addChild (markerNodeFromState (marker, length), -1,
                               &undoManager);
    for (const auto& marker : variant.transientMarkers)
        if (marker.timelineOffsetSamples > 0
            && marker.timelineOffsetSamples < length)
            clipNode.addChild (markerNodeFromState (marker, length), -1,
                               &undoManager);
    suppressNotifications = false;
    notifyChanged();
    return true;
}

bool ProjectModel::removeClipVariant (const juce::String& trackId,
                                      const juce::String& clipId,
                                      const juce::String& variantId)
{
    auto clipNode = findClip (trackId, clipId);
    if (! clipNode.isValid())
        return false;
    for (int index = 0; index < clipNode.getNumChildren(); ++index)
        if (clipNode.getChild (index).hasType (modelIds::clipVariant)
            && clipNode.getChild (index).getProperty (
                modelIds::id).toString() == variantId)
        {
            beginUndoTransaction ("Delete Clip Variant");
            clipNode.removeChild (index, &undoManager);
            return true;
        }
    return false;
}

bool ProjectModel::removeClip (const juce::String& trackId,
                               const juce::String& clipId)
{
    auto trackNode = findTrack (trackId);
    auto clipNode = findClip (trackId, clipId);
    if (! trackNode.isValid() || ! clipNode.isValid()
        || static_cast<bool> (clipNode.getProperty (modelIds::locked, false)))
        return false;

    const auto sourceId = clipNode.getProperty (modelIds::sourceId).toString();
    const auto groupId = clipNode.getProperty (modelIds::takeGroupId).toString();
    const auto wasActive = static_cast<bool> (
        clipNode.getProperty (modelIds::activeTake, true));
    beginUndoTransaction ("Delete Clip");
    trackNode.removeChild (clipNode, &undoManager);

    if (wasActive && groupId.isNotEmpty())
    {
        auto activeSegmentRemains = false;
        for (const auto child : trackNode)
            if (child.hasType (modelIds::clip)
                && child.getProperty (modelIds::takeGroupId).toString() == groupId
                && static_cast<bool> (child.getProperty (modelIds::activeTake, true)))
                activeSegmentRemains = true;
        if (! activeSegmentRemains)
            for (auto child : trackNode)
                if (child.hasType (modelIds::clip)
                    && child.getProperty (modelIds::takeGroupId).toString() == groupId)
                {
                    child.setProperty (modelIds::activeTake, true, &undoManager);
                    break;
                }
    }

    bool sourceStillReferenced = false;
    for (int trackIndex = 0; trackIndex < getTrackCount() && ! sourceStillReferenced;
         ++trackIndex)
    {
        const auto otherTrack = findTrack (getTrackId (trackIndex));
        for (const auto child : otherTrack)
            if (child.hasType (modelIds::clip)
                && child.getProperty (modelIds::sourceId).toString() == sourceId)
                sourceStillReferenced = true;
    }

    if (! sourceStillReferenced)
    {
        const auto sourceNode = findMediaSource (sourceId);
        if (sourceNode.isValid())
            state.removeChild (sourceNode, &undoManager);
    }

    return true;
}

bool ProjectModel::selectActiveTake (const juce::String& trackId,
                                     const juce::String& clipId)
{
    auto trackNode = findTrack (trackId);
    auto target = findClip (trackId, clipId);
    if (! trackNode.isValid() || ! target.isValid())
        return false;

    const auto groupId = target.getProperty (modelIds::takeGroupId).toString();
    if (groupId.isEmpty())
        return false;

    const auto targetLane = static_cast<int> (
        target.getProperty (modelIds::takeLaneIndex, 0));
    beginUndoTransaction ("Select Active Take");
    suppressNotifications = true;
    auto changed = false;
    for (int index = trackNode.getNumChildren() - 1; index >= 0; --index)
    {
        const auto child = trackNode.getChild (index);
        if (child.hasType (modelIds::compRegion)
            && child.getProperty (modelIds::takeGroupId).toString() == groupId)
        {
            trackNode.removeChild (index, &undoManager);
            changed = true;
        }
    }
    for (auto child : trackNode)
    {
        if (! child.hasType (modelIds::clip)
            || child.getProperty (modelIds::takeGroupId).toString() != groupId)
            continue;
        const auto shouldBeActive = static_cast<int> (
            child.getProperty (modelIds::takeLaneIndex, 0)) == targetLane;
        if (static_cast<bool> (child.getProperty (modelIds::activeTake, true))
            != shouldBeActive)
        {
            child.setProperty (modelIds::activeTake, shouldBeActive, &undoManager);
            changed = true;
        }
    }
    suppressNotifications = false;
    if (changed)
        notifyChanged();
    return changed;
}

bool ProjectModel::assignCompRegion (const juce::String& trackId,
                                     const juce::String& clipId,
                                     juce::int64 timelineStartSamples,
                                     juce::int64 timelineEndSamples)
{
    auto trackNode = findTrack (trackId);
    const auto target = getClipState (trackId, clipId);
    if (! trackNode.isValid() || target.id.isEmpty() || target.takeGroupId.isEmpty())
        return false;

    const auto targetEnd = target.timelineStartSamples + target.lengthInSamples;
    timelineStartSamples = juce::jlimit (target.timelineStartSamples,
                                         targetEnd,
                                         timelineStartSamples);
    timelineEndSamples = juce::jlimit (target.timelineStartSamples,
                                       targetEnd,
                                       timelineEndSamples);
    if (timelineEndSamples < timelineStartSamples)
        std::swap (timelineStartSamples, timelineEndSamples);
    if (timelineEndSamples <= timelineStartSamples)
        return false;

    std::vector<juce::ValueTree> existing;
    for (const auto child : trackNode)
        if (child.hasType (modelIds::compRegion)
            && child.getProperty (modelIds::takeGroupId).toString()
                   == target.takeGroupId)
            existing.push_back (child);

    beginUndoTransaction ("Assign Comp Region");
    suppressNotifications = true;
    std::vector<juce::ValueTree> rightRemainders;
    for (auto region : existing)
    {
        const auto start = static_cast<juce::int64> (
            region.getProperty (modelIds::timelineStartSamples, 0));
        const auto end = start + static_cast<juce::int64> (
            region.getProperty (modelIds::lengthInSamples, 0));
        if (end <= timelineStartSamples || start >= timelineEndSamples)
            continue;

        if (start < timelineStartSamples && end > timelineEndSamples)
        {
            juce::ValueTree right (modelIds::compRegion);
            right.setProperty (modelIds::id, juce::Uuid().toString(), nullptr);
            right.setProperty (modelIds::takeGroupId, target.takeGroupId, nullptr);
            right.setProperty (modelIds::takeLaneIndex,
                               region.getProperty (modelIds::takeLaneIndex, 0), nullptr);
            right.setProperty (modelIds::timelineStartSamples,
                               timelineEndSamples, nullptr);
            right.setProperty (modelIds::lengthInSamples,
                               end - timelineEndSamples, nullptr);
            right.setProperty (modelIds::crossfadeSamples,
                               region.getProperty (modelIds::crossfadeSamples, 480), nullptr);
            rightRemainders.push_back (right);
            region.setProperty (modelIds::lengthInSamples,
                                timelineStartSamples - start, &undoManager);
        }
        else if (start < timelineStartSamples)
            region.setProperty (modelIds::lengthInSamples,
                                timelineStartSamples - start, &undoManager);
        else if (end > timelineEndSamples)
        {
            region.setProperty (modelIds::timelineStartSamples,
                                timelineEndSamples, &undoManager);
            region.setProperty (modelIds::lengthInSamples,
                                end - timelineEndSamples, &undoManager);
        }
        else
            trackNode.removeChild (region, &undoManager);
    }

    for (auto& region : rightRemainders)
        trackNode.addChild (region, -1, &undoManager);

    juce::ValueTree region (modelIds::compRegion);
    region.setProperty (modelIds::id, juce::Uuid().toString(), nullptr);
    region.setProperty (modelIds::takeGroupId, target.takeGroupId, nullptr);
    region.setProperty (modelIds::takeLaneIndex, target.takeLaneIndex, nullptr);
    region.setProperty (modelIds::timelineStartSamples, timelineStartSamples, nullptr);
    region.setProperty (modelIds::lengthInSamples,
                        timelineEndSamples - timelineStartSamples, nullptr);
    region.setProperty (modelIds::crossfadeSamples,
                        juce::jmax (static_cast<juce::int64> (1),
                                    static_cast<juce::int64> (std::llround (
                                        getTimelineSampleRate() * 0.010))),
                        nullptr);
    trackNode.addChild (region, -1, &undoManager);
    suppressNotifications = false;
    notifyChanged();
    return true;
}

bool ProjectModel::clearCompRegions (const juce::String& trackId,
                                     const juce::String& takeGroupId)
{
    auto trackNode = findTrack (trackId);
    if (! trackNode.isValid() || takeGroupId.isEmpty())
        return false;

    beginUndoTransaction ("Clear Comp Regions");
    auto changed = false;
    suppressNotifications = true;
    for (int index = trackNode.getNumChildren() - 1; index >= 0; --index)
    {
        const auto child = trackNode.getChild (index);
        if (child.hasType (modelIds::compRegion)
            && child.getProperty (modelIds::takeGroupId).toString() == takeGroupId)
        {
            trackNode.removeChild (index, &undoManager);
            changed = true;
        }
    }
    suppressNotifications = false;
    if (changed)
        notifyChanged();
    return changed;
}

std::vector<AudioPlaybackSegmentState> ProjectModel::resolveAudioPlayback (
    const juce::String& trackId) const
{
    const auto track = getTrackState (trackId);
    std::vector<AudioPlaybackSegmentState> result;
    juce::StringArray groups;

    const auto segmentFor = [this] (const AudioClipState& clip,
                                    juce::int64 start,
                                    juce::int64 end)
    {
        const auto source = getMediaSourceState (clip.sourceId);
        const auto anchors = anchorsForClip (clip, source, getTimelineSampleRate());
        const auto relativeStart = start - clip.timelineStartSamples;
        const auto relativeEnd = end - clip.timelineStartSamples;
        const auto logicalStart = clip.reversed
            ? juce::jlimit (static_cast<juce::int64> (0), clip.lengthInSamples,
                            clip.lengthInSamples - relativeEnd)
            : relativeStart;
        const auto logicalEnd = clip.reversed
            ? juce::jlimit (static_cast<juce::int64> (0), clip.lengthInSamples,
                            clip.lengthInSamples - relativeStart)
            : relativeEnd;
        AudioPlaybackSegmentState segment {
            clip.id, clip.sourceId, start,
            warp::sourceAt (anchors, logicalStart),
            end - start, clip.gain, 0, 0,
            warp::playbackRate (anchors, logicalStart, logicalEnd,
                                getTimelineSampleRate(), source.sampleRate),
            clip.reversed
        };
        segment.fadeInSamples = clip.fadeInSamples;
        segment.fadeOutSamples = clip.fadeOutSamples;
        segment.fadeAnchorStartSamples = clip.timelineStartSamples;
        segment.fadeAnchorLengthSamples = clip.lengthInSamples;
        return segment;
    };

    for (const auto& clip : track.clips)
    {
        if (clip.muted)
            continue;
        if (clip.takeGroupId.isEmpty())
        {
            std::vector<juce::int64> clipBoundaries {
                clip.timelineStartSamples,
                clip.timelineStartSamples + clip.lengthInSamples
            };
            for (const auto& marker : clip.warpMarkers)
                clipBoundaries.push_back (clip.timelineStartSamples
                    + (clip.reversed
                           ? clip.lengthInSamples - marker.timelineOffsetSamples
                           : marker.timelineOffsetSamples));
            std::sort (clipBoundaries.begin(), clipBoundaries.end());
            for (size_t index = 0; index + 1 < clipBoundaries.size(); ++index)
                if (clipBoundaries[index + 1] > clipBoundaries[index])
                    result.push_back (segmentFor (clip, clipBoundaries[index],
                                                  clipBoundaries[index + 1]));
        }
        else
            groups.addIfNotAlreadyThere (clip.takeGroupId);
    }

    for (const auto& groupId : groups)
    {
        std::vector<juce::int64> boundaries;
        for (const auto& clip : track.clips)
            if (clip.takeGroupId == groupId && ! clip.muted)
            {
                boundaries.push_back (clip.timelineStartSamples);
                boundaries.push_back (clip.timelineStartSamples + clip.lengthInSamples);
                for (const auto& marker : clip.warpMarkers)
                    boundaries.push_back (clip.timelineStartSamples
                        + (clip.reversed
                               ? clip.lengthInSamples - marker.timelineOffsetSamples
                               : marker.timelineOffsetSamples));
            }
        for (const auto& region : track.compRegions)
            if (region.takeGroupId == groupId)
            {
                boundaries.push_back (region.timelineStartSamples);
                boundaries.push_back (region.timelineStartSamples + region.lengthInSamples);
            }
        std::sort (boundaries.begin(), boundaries.end());
        boundaries.erase (std::unique (boundaries.begin(), boundaries.end()),
                          boundaries.end());

        std::vector<AudioPlaybackSegmentState> resolved;
        for (size_t index = 0; index + 1 < boundaries.size(); ++index)
        {
            const auto start = boundaries[index];
            const auto end = boundaries[index + 1];
            if (end <= start)
                continue;

            auto desiredLane = -1;
            for (const auto& region : track.compRegions)
                if (region.takeGroupId == groupId
                    && region.timelineStartSamples <= start
                    && region.timelineStartSamples + region.lengthInSamples >= end)
                {
                    desiredLane = region.takeLaneIndex;
                    break;
                }

            const AudioClipState* chosen = nullptr;
            for (const auto& clip : track.clips)
            {
                if (clip.takeGroupId != groupId || clip.muted
                    || clip.timelineStartSamples > start
                    || clip.timelineStartSamples + clip.lengthInSamples < end)
                    continue;
                if ((desiredLane >= 0 && clip.takeLaneIndex == desiredLane)
                    || (desiredLane < 0 && clip.activeTake))
                {
                    chosen = &clip;
                    break;
                }
            }
            if (chosen == nullptr && desiredLane >= 0)
                for (const auto& clip : track.clips)
                    if (clip.takeGroupId == groupId && ! clip.muted
                        && clip.activeTake
                        && clip.timelineStartSamples <= start
                        && clip.timelineStartSamples + clip.lengthInSamples >= end)
                    {
                        chosen = &clip;
                        break;
                    }
            if (chosen == nullptr)
                continue;

            auto segment = segmentFor (*chosen, start, end);
            if (! resolved.empty())
            {
                auto& previous = resolved.back();
                const auto previousSource = getMediaSourceState (previous.sourceId);
                const auto previousSourceLength = static_cast<juce::int64> (
                    timeline::convertSampleCount (stretch::timelineToNativeSamples (
                                                      previous.lengthInSamples,
                                                      1.0 / previous.playbackRate),
                                                  getTimelineSampleRate(),
                                                  previousSource.sampleRate));
                if (previous.id == segment.id
                    && previous.timelineStartSamples + previous.lengthInSamples
                           == segment.timelineStartSamples
                    && std::abs (previous.playbackRate - segment.playbackRate) < 0.000001
                    && std::abs ((previous.sourceOffsetSamples + previousSourceLength)
                                 - segment.sourceOffsetSamples) <= 1)
                {
                    previous.lengthInSamples += segment.lengthInSamples;
                    continue;
                }
            }
            resolved.push_back (std::move (segment));
        }

        const auto groupHasCompRegions = std::any_of (
            track.compRegions.begin(), track.compRegions.end(),
            [&groupId] (const auto& region) { return region.takeGroupId == groupId; });
        if (groupHasCompRegions)
            for (size_t index = 0; index + 1 < resolved.size(); ++index)
            {
                auto& left = resolved[index];
                auto& right = resolved[index + 1];
                const auto boundary = left.timelineStartSamples + left.lengthInSamples;
                if (boundary != right.timelineStartSamples || left.id == right.id
                    || left.sourceId == right.sourceId)
                    continue;
                const auto leftClip = getClipState (trackId, left.id);
                const auto rightClip = getClipState (trackId, right.id);
                auto requested = static_cast<juce::int64> (std::llround (
                    getTimelineSampleRate() * 0.010));
                for (const auto& region : track.compRegions)
                    if (region.takeGroupId == groupId
                        && (region.timelineStartSamples == boundary
                            || region.timelineStartSamples + region.lengthInSamples
                                   == boundary))
                        requested = region.crossfadeSamples;
                const auto half = juce::jmax (
                    static_cast<juce::int64> (0),
                    std::min ({ requested / 2,
                                left.lengthInSamples / 2,
                                right.lengthInSamples / 2,
                                leftClip.timelineStartSamples + leftClip.lengthInSamples
                                    - boundary,
                                boundary - rightClip.timelineStartSamples }));
                if (half <= 0)
                    continue;
                const auto full = half * 2;
                if (left.reversed)
                {
                    const auto leftSource = getMediaSourceState (left.sourceId);
                    left.sourceOffsetSamples -= static_cast<juce::int64> (
                        timeline::convertSampleCount (
                            stretch::timelineToNativeSamples (
                                half, 1.0 / left.playbackRate),
                            getTimelineSampleRate(), leftSource.sampleRate));
                }
                left.lengthInSamples += half;
                left.fadeOutSamples = full;
                right.timelineStartSamples -= half;
                right.lengthInSamples += half;
                if (! right.reversed)
                {
                    const auto rightSource = getMediaSourceState (right.sourceId);
                    right.sourceOffsetSamples -= static_cast<juce::int64> (
                        timeline::convertSampleCount (
                            stretch::timelineToNativeSamples (
                                half, 1.0 / right.playbackRate),
                            getTimelineSampleRate(), rightSource.sampleRate));
                }
                right.fadeInSamples = full;
            }

        result.insert (result.end(), resolved.begin(), resolved.end());
    }

    std::sort (result.begin(), result.end(), [] (const auto& left, const auto& right)
    {
        return left.timelineStartSamples < right.timelineStartSamples;
    });
    return result;
}

void ProjectModel::beginUndoTransaction (const juce::String& description)
{
    capturePersistentUndoState (description);
    undoManager.beginNewTransaction (description);
}

bool ProjectModel::canUndo() const
{
    return undoManager.canUndo() || ! persistentUndoHistory.empty();
}

bool ProjectModel::canRedo() const
{
    return undoManager.canRedo() || ! persistentRedoHistory.empty();
}

juce::String ProjectModel::getUndoDescription() const
{
    if (undoManager.canUndo())
        return undoManager.getUndoDescription();
    return persistentUndoHistory.empty() ? juce::String {}
                                         : persistentUndoHistory.back().description;
}

juce::String ProjectModel::getRedoDescription() const
{
    if (undoManager.canRedo())
        return undoManager.getRedoDescription();
    return persistentRedoHistory.empty() ? juce::String {}
                                         : persistentRedoHistory.back().description;
}

bool ProjectModel::undo()
{
    suppressNotifications = true;
    const auto liveUndoDescription = getUndoDescription();
    const auto currentStateXml = persistentStateXmlFor (state);
    const auto changed = undoManager.undo();
    suppressNotifications = false;
    if (changed)
    {
        if (! persistentUndoHistory.empty())
            persistentUndoHistory.pop_back();
        if (currentStateXml.isNotEmpty())
        {
            persistentRedoHistory.push_back ({ liveUndoDescription, currentStateXml });
            trimPersistentHistory (persistentRedoHistory);
        }
        notifyChanged();
    }
    else if (! persistentUndoHistory.empty())
    {
        auto entry = persistentUndoHistory.back();
        persistentUndoHistory.pop_back();
        if (currentStateXml.isNotEmpty())
        {
            persistentRedoHistory.push_back ({ entry.description, currentStateXml });
            trimPersistentHistory (persistentRedoHistory);
        }
        return restorePersistentUndoState (entry);
    }
    return changed;
}

bool ProjectModel::redo()
{
    suppressNotifications = true;
    const auto liveRedoDescription = getRedoDescription();
    const auto currentStateXml = persistentStateXmlFor (state);
    const auto changed = undoManager.redo();
    suppressNotifications = false;
    if (changed)
    {
        if (! persistentRedoHistory.empty())
            persistentRedoHistory.pop_back();
        if (currentStateXml.isNotEmpty())
        {
            persistentUndoHistory.push_back ({ liveRedoDescription, currentStateXml });
            trimPersistentHistory (persistentUndoHistory);
        }
        notifyChanged();
    }
    else if (! persistentRedoHistory.empty())
    {
        auto entry = persistentRedoHistory.back();
        persistentRedoHistory.pop_back();
        if (currentStateXml.isNotEmpty())
        {
            persistentUndoHistory.push_back ({ entry.description, currentStateXml });
            trimPersistentHistory (persistentUndoHistory);
        }
        return restorePersistentUndoState (entry);
    }
    return changed;
}

void ProjectModel::clearUndoHistory()
{
    undoManager.clearUndoHistory();
    persistentUndoHistory.clear();
    persistentRedoHistory.clear();
    loadedUndoManifest = {};
}

void ProjectModel::capturePersistentUndoState (const juce::String& description)
{
    if (restoringPersistentHistory || ! state.isValid())
        return;

    auto cleanDescription = description.trim();
    if (cleanDescription.isEmpty())
        cleanDescription = "Edit Project";

    const auto stateXml = persistentStateXmlFor (state);
    if (stateXml.isEmpty())
        return;

    if (! persistentUndoHistory.empty()
        && persistentUndoHistory.back().stateXml == stateXml)
    {
        persistentUndoHistory.back().description = cleanDescription;
        return;
    }

    persistentUndoHistory.push_back ({ cleanDescription, stateXml });
    trimPersistentHistory (persistentUndoHistory);
    persistentRedoHistory.clear();
    loadedUndoManifest = {};
}

bool ProjectModel::restorePersistentUndoState (
    const PersistentUndoEntryState& entry)
{
    const auto replacement = persistentStateFromXml (entry.stateXml);
    if (! replacement.isValid() || ! replacement.hasType (modelIds::project))
        return false;

    restoringPersistentHistory = true;
    suppressNotifications = true;
    state.removeListener (this);
    state = replacement;
    state.addListener (this);
    undoManager.clearUndoHistory();
    suppressNotifications = false;
    restoringPersistentHistory = false;
    notifyChanged();
    return true;
}

ProjectState ProjectModel::createSnapshot() const
{
    ProjectState snapshot;
    snapshot.projectId = getProjectId();
    snapshot.name = getName();
    snapshot.masterGain = getMasterGain();
    snapshot.masterMuted = getMasterMuted();
    snapshot.timelineSampleRate = getTimelineSampleRate();
    snapshot.tempoPoints = getTempoPoints();
    snapshot.timeSignatureNumerator = static_cast<int> (
        state.getProperty (modelIds::timeSignatureNumerator, 4));
    snapshot.timeSignatureDenominator = static_cast<int> (
        state.getProperty (modelIds::timeSignatureDenominator, 4));
    snapshot.metronomeEnabled = getMetronomeEnabled();
    snapshot.metronomeGain = getMetronomeGain();
    snapshot.timeSignatures = getTimeSignatures();
    snapshot.markers = getMarkers();
    snapshot.automationLanes = getAutomationLanes();
    snapshot.lowLatencyMonitoring = getLowLatencyMonitoring();
    snapshot.inputMonitorMode = getInputMonitorMode();
    snapshot.manualRecordOffsetSamples = getManualRecordOffsetSamples();
    snapshot.recordingSettings = getRecordingSettings();
    snapshot.syncSettings = getSyncSettings();
    snapshot.monitorPaths = getMonitorPaths();
    snapshot.producerSettings = getProducerSettings();
    snapshot.repairRecords = repairRecords;
    snapshot.undoManifest = loadedUndoManifest;
    if (canUndo() || canRedo())
    {
        snapshot.undoManifest.hadUndo = canUndo();
        snapshot.undoManifest.hadRedo = canRedo();
        snapshot.undoManifest.undoDescription = getUndoDescription();
        snapshot.undoManifest.redoDescription = getRedoDescription();
    }
    snapshot.undoManifest.undoHistory = persistentUndoHistory;
    snapshot.undoManifest.redoHistory = persistentRedoHistory;

    for (const auto child : state)
    {
        if (child.hasType (modelIds::mediaSource))
            snapshot.mediaSources.push_back (
                getMediaSourceState (child.getProperty (modelIds::id).toString()));
    }

    for (int index = 0; index < getTrackCount(); ++index)
        snapshot.tracks.push_back (getTrackState (getTrackId (index)));

    snapshot.mixerChannels = getMixerChannels();

    return snapshot;
}

void ProjectModel::replaceWithSnapshot (const ProjectState& snapshot)
{
    repairRecords = snapshot.repairRecords;
    loadedUndoManifest = snapshot.undoManifest;
    persistentUndoHistory = snapshot.undoManifest.undoHistory;
    persistentRedoHistory = snapshot.undoManifest.redoHistory;

    auto replacement = createEmptyState();
    replacement.setProperty (modelIds::projectId,
                             snapshot.projectId.isNotEmpty() ? snapshot.projectId
                                                             : juce::Uuid().toString(),
                             nullptr);
    replacement.setProperty (modelIds::name,
                             snapshot.name.trim().isNotEmpty() ? snapshot.name.trim()
                                                               : "Untitled Project",
                             nullptr);
    replacement.setProperty (modelIds::masterGain,
                             juce::jlimit (0.0f, 1.25f, snapshot.masterGain),
                             nullptr);
    replacement.setProperty (modelIds::masterMuted,
                             snapshot.masterMuted, nullptr);
    replacement.setProperty (modelIds::timelineSampleRate,
                             juce::jmax (1.0, snapshot.timelineSampleRate),
                             nullptr);
    replacement.setProperty (modelIds::timeSignatureNumerator,
                             juce::jlimit (1, 32, snapshot.timeSignatureNumerator), nullptr);
    replacement.setProperty (modelIds::timeSignatureDenominator,
                             juce::jlimit (1, 32, snapshot.timeSignatureDenominator), nullptr);
    replacement.setProperty (modelIds::metronomeEnabled,
                             snapshot.metronomeEnabled, nullptr);
    replacement.setProperty (modelIds::metronomeGain,
                             juce::jlimit (0.0f, 1.0f, snapshot.metronomeGain), nullptr);
    replacement.setProperty (modelIds::lowLatencyMonitoring,
                             snapshot.lowLatencyMonitoring, nullptr);
    replacement.setProperty (
        modelIds::inputMonitorMode,
        snapshot.inputMonitorMode == InputMonitorMode::always ? "always"
            : snapshot.inputMonitorMode == InputMonitorMode::whileArmed
                  ? "armed" : "off",
        nullptr);
    replacement.setProperty (modelIds::manualRecordOffsetSamples,
                             juce::jlimit (-8192, 8192,
                                           snapshot.manualRecordOffsetSamples),
                             nullptr);
    const auto& recording = snapshot.recordingSettings;
    replacement.setProperty (modelIds::recordingMode,
        recording.mode == RecordingMode::punch ? "punch"
            : recording.mode == RecordingMode::loop ? "loop" : "normal",
        nullptr);
    replacement.setProperty (modelIds::preRollBars,
                             juce::jlimit (0, 8, recording.preRollBars), nullptr);
    replacement.setProperty (modelIds::punchStartSamples,
                             juce::jmax (static_cast<juce::int64> (0),
                                         recording.punchStartSamples), nullptr);
    replacement.setProperty (modelIds::punchEndSamples,
                             juce::jmax (recording.punchStartSamples,
                                         recording.punchEndSamples), nullptr);
    replacement.setProperty (modelIds::loopStartSamples,
                             juce::jmax (static_cast<juce::int64> (0),
                                         recording.loopStartSamples), nullptr);
    replacement.setProperty (modelIds::loopEndSamples,
                             juce::jmax (recording.loopStartSamples,
                                         recording.loopEndSamples), nullptr);
    replacement.setProperty (modelIds::controlRoomGain,
                             juce::jlimit (0.0f, 1.5f,
                                           recording.controlRoomGain), nullptr);
    replacement.setProperty (modelIds::controlRoomDimmed,
                             recording.controlRoomDimmed, nullptr);
    replacement.setProperty (modelIds::controlRoomMuted,
                             recording.controlRoomMuted, nullptr);
    const auto& sync = snapshot.syncSettings;
    replacement.setProperty (modelIds::syncSource,
                             syncSourceName (sync.source), nullptr);
    replacement.setProperty (modelIds::sendMidiClock,
                             sync.sendMidiClock, nullptr);
    replacement.setProperty (modelIds::followExternalStartStop,
                             sync.followExternalStartStop, nullptr);
    replacement.setProperty (modelIds::mtcFramesPerSecond,
                             sync.mtcFramesPerSecond == 24
                                     || sync.mtcFramesPerSecond == 25
                                     || sync.mtcFramesPerSecond == 29
                                 ? sync.mtcFramesPerSecond : 30, nullptr);
    replacement.setProperty (modelIds::jitterToleranceSamples,
                             juce::jlimit (0, 8192,
                                           sync.jitterToleranceSamples), nullptr);
    replacement.setProperty (modelIds::producerRootPitchClass,
                             juce::jlimit (0, 11,
                                           snapshot.producerSettings.rootPitchClass), nullptr);
    replacement.setProperty (modelIds::producerScale,
                             snapshot.producerSettings.scale == "minor"
                                 ? "minor" : "major", nullptr);
    replacement.setProperty (modelIds::producerStyle,
                             snapshot.producerSettings.style == "chill"
                                 || snapshot.producerSettings.style == "energetic"
                                 ? snapshot.producerSettings.style : "balanced", nullptr);
    replacement.setProperty (modelIds::producerBars,
                             juce::jlimit (1, 16,
                                           snapshot.producerSettings.bars), nullptr);
    replacement.setProperty (modelIds::producerVariation,
                             static_cast<juce::int64> (
                                 snapshot.producerSettings.variation == 0
                                     ? 1 : snapshot.producerSettings.variation), nullptr);

    for (int index = replacement.getNumChildren(); index-- > 0;)
        if (replacement.getChild (index).hasType (modelIds::tempoPoint)
            || replacement.getChild (index).hasType (modelIds::timeSignaturePoint))
            replacement.removeChild (index, nullptr);

    const auto tempoPoints = snapshot.tempoPoints.empty()
                                 ? std::vector<TempoPointState> {
                                       { {}, 0.0, 120.0, TempoCurve::step } }
                                 : snapshot.tempoPoints;
    for (const auto& point : tempoPoints)
    {
        juce::ValueTree node (modelIds::tempoPoint);
        node.setProperty (modelIds::id,
                          point.id.isNotEmpty() ? point.id : juce::Uuid().toString(), nullptr);
        node.setProperty (modelIds::beat, juce::jmax (0.0, point.beat), nullptr);
        node.setProperty (modelIds::bpm, juce::jlimit (20.0, 400.0, point.bpm), nullptr);
        node.setProperty (modelIds::curve, tempoCurveName (point.curve), nullptr);
        replacement.addChild (node, -1, nullptr);
    }

    const auto signatures = snapshot.timeSignatures.empty()
        ? std::vector<TimeSignaturePointState> {
              { {}, 0.0, snapshot.timeSignatureNumerator,
                snapshot.timeSignatureDenominator } }
        : snapshot.timeSignatures;
    for (const auto& signature : signatures)
    {
        juce::ValueTree node (modelIds::timeSignaturePoint);
        node.setProperty (modelIds::id,
                          signature.id.isNotEmpty() ? signature.id
                                                    : juce::Uuid().toString(), nullptr);
        node.setProperty (modelIds::beat, juce::jmax (0.0, signature.beat), nullptr);
        node.setProperty (modelIds::numerator,
                          juce::jlimit (1, 32, signature.numerator), nullptr);
        node.setProperty (modelIds::denominator,
                          juce::jlimit (1, 32, signature.denominator), nullptr);
        replacement.addChild (node, -1, nullptr);
    }

    for (const auto& marker : snapshot.markers)
    {
        juce::ValueTree node (modelIds::projectMarker);
        node.setProperty (modelIds::id,
                          marker.id.isNotEmpty() ? marker.id
                                                : juce::Uuid().toString(), nullptr);
        node.setProperty (modelIds::beat, juce::jmax (0.0, marker.beat), nullptr);
        node.setProperty (modelIds::name,
                          marker.name.isNotEmpty() ? marker.name : "Marker", nullptr);
        node.setProperty (modelIds::colour,
                          static_cast<juce::int64> (marker.colour.getARGB()), nullptr);
        replacement.addChild (node, -1, nullptr);
    }

    for (const auto& lane : snapshot.automationLanes)
    {
        juce::ValueTree laneNode (modelIds::automationLane);
        laneNode.setProperty (modelIds::id,
                              lane.id.isNotEmpty() ? lane.id
                                                   : juce::Uuid().toString(), nullptr);
        laneNode.setProperty (modelIds::targetId,
                              lane.targetId.isNotEmpty() ? lane.targetId : "master", nullptr);
        const auto parameterId = lane.parameterId == "gain"
                || lane.parameterId == "pan"
                || lane.parameterId.startsWith ("plugin:")
            ? lane.parameterId : juce::String { "gain" };
        laneNode.setProperty (modelIds::parameterId, parameterId, nullptr);
        laneNode.setProperty (modelIds::mode, automationModeName (lane.mode), nullptr);
        laneNode.setProperty (modelIds::enabled, lane.enabled, nullptr);
        for (const auto& point : lane.points)
        {
            juce::ValueTree pointNode (modelIds::automationPoint);
            pointNode.setProperty (modelIds::id,
                                   point.id.isNotEmpty() ? point.id
                                                        : juce::Uuid().toString(), nullptr);
            pointNode.setProperty (modelIds::samplePosition,
                                   juce::jmax (static_cast<juce::int64> (0),
                                               point.samplePosition), nullptr);
            pointNode.setProperty (modelIds::gain,
                                   juce::jlimit (-1.0f, 1.5f, point.value), nullptr);
            pointNode.setProperty (modelIds::curve,
                                   automationCurveName (point.curve), nullptr);
            laneNode.addChild (pointNode, -1, nullptr);
        }
        replacement.addChild (laneNode, -1, nullptr);
    }

    for (const auto& source : snapshot.mediaSources)
    {
        juce::ValueTree sourceNode (modelIds::mediaSource);
        sourceNode.setProperty (modelIds::id,
                                source.id.isNotEmpty() ? source.id : juce::Uuid().toString(),
                                nullptr);
        sourceNode.setProperty (modelIds::path, source.file.getFullPathName(), nullptr);
        sourceNode.setProperty (modelIds::sampleRate, source.sampleRate, nullptr);
        sourceNode.setProperty (modelIds::channels, source.channels, nullptr);
        sourceNode.setProperty (modelIds::lengthInSamples, source.lengthInSamples, nullptr);
        sourceNode.setProperty (modelIds::assetFingerprint,
                                source.assetFingerprint.isNotEmpty()
                                    ? source.assetFingerprint
                                    : makeAssetFingerprint (
                                          source.file, source.sampleRate,
                                          source.channels,
                                          source.lengthInSamples),
                                nullptr);
        replacement.addChild (sourceNode, -1, nullptr);
    }

    for (int index = 0; index < static_cast<int> (snapshot.tracks.size()); ++index)
    {
        const auto& track = snapshot.tracks[static_cast<size_t> (index)];
        juce::ValueTree trackNode (modelIds::track);
        trackNode.setProperty (modelIds::id,
                               track.id.isNotEmpty() ? track.id : juce::Uuid().toString(),
                               nullptr);
        trackNode.setProperty (modelIds::name,
                               track.name.isNotEmpty() ? track.name : "Audio Track",
                               nullptr);
        trackNode.setProperty (modelIds::colour,
                               static_cast<juce::int64> ((track.colour.isTransparent()
                                                             ? defaultTrackColour (index)
                                                             : track.colour).getARGB()),
                               nullptr);
        trackNode.setProperty (modelIds::gain, juce::jlimit (0.0f, 1.5f, track.gain), nullptr);
        trackNode.setProperty (modelIds::pan, juce::jlimit (-1.0f, 1.0f, track.pan), nullptr);
        trackNode.setProperty (modelIds::muted, track.muted, nullptr);
        trackNode.setProperty (modelIds::solo, track.solo, nullptr);
        trackNode.setProperty (modelIds::recordArmed, track.recordArmed, nullptr);
        trackNode.setProperty (modelIds::inputStartChannel,
                               juce::jlimit (0, 63, track.inputStartChannel), nullptr);
        trackNode.setProperty (modelIds::inputChannelCount,
                               juce::jlimit (1, 2, track.inputChannelCount), nullptr);
        trackNode.setProperty (modelIds::recordingTapPoint,
                               track.recordingTapPoint
                                       == RecordingTapPoint::programOutput
                                   ? "device-output" : "hardware-input",
                               nullptr);
        trackNode.setProperty (modelIds::recordOffsetSamples,
                               juce::jlimit (-8192, 8192,
                                             track.recordOffsetSamples), nullptr);
        trackNode.setProperty (modelIds::isInstrument, track.isInstrument, nullptr);
        trackNode.setProperty (modelIds::pluginName, track.pluginName, nullptr);
        trackNode.setProperty (modelIds::pluginDescriptionXml,
                               track.pluginDescriptionXml, nullptr);
        trackNode.setProperty (modelIds::pluginStateBase64,
                               track.pluginStateBase64, nullptr);
        trackNode.setProperty (modelIds::pluginBypassed,
                               track.pluginBypassed, nullptr);
        trackNode.setProperty (modelIds::pluginLatencySamples,
                               juce::jmax (0, track.pluginLatencySamples), nullptr);
        trackNode.setProperty (modelIds::pluginAvailable,
                               track.pluginAvailable, nullptr);
        trackNode.setProperty (modelIds::manualLatencyOffsetSamples,
                               juce::jlimit (-262143, 262143,
                                             track.manualLatencyOffsetSamples), nullptr);
        trackNode.setProperty (modelIds::outputDestinationId,
                               track.outputDestinationId.isNotEmpty()
                                   ? track.outputDestinationId : "master", nullptr);
        trackNode.setProperty (modelIds::frozen, track.frozen, nullptr);
        trackNode.setProperty (modelIds::freezeSourceId,
                               track.freezeSourceId, nullptr);
        setBusLayoutProperties (trackNode, track.layout);
        for (const auto& slot : track.pluginSlots)
            addPluginSlotNode (trackNode, slot);

        for (const auto& clip : track.clips)
        {
            juce::ValueTree clipNode (modelIds::clip);
            clipNode.setProperty (modelIds::id,
                                  clip.id.isNotEmpty() ? clip.id : juce::Uuid().toString(),
                                  nullptr);
            clipNode.setProperty (modelIds::name, clip.name, nullptr);
            clipNode.setProperty (modelIds::sourceId, clip.sourceId, nullptr);
            clipNode.setProperty (modelIds::timelineStartSamples,
                                  juce::jmax (static_cast<juce::int64> (0),
                                              clip.timelineStartSamples),
                                  nullptr);
            clipNode.setProperty (modelIds::sourceOffsetSamples,
                                  juce::jmax (static_cast<juce::int64> (0),
                                              clip.sourceOffsetSamples),
                                  nullptr);
            clipNode.setProperty (modelIds::lengthInSamples,
                                  juce::jmax (static_cast<juce::int64> (0),
                                              clip.lengthInSamples),
                                  nullptr);
            clipNode.setProperty (modelIds::fadeInSamples,
                                  juce::jlimit (
                                      static_cast<juce::int64> (0),
                                      juce::jmax (static_cast<juce::int64> (0),
                                                  clip.lengthInSamples),
                                      clip.fadeInSamples), nullptr);
            clipNode.setProperty (modelIds::fadeOutSamples,
                                  juce::jlimit (
                                      static_cast<juce::int64> (0),
                                      juce::jmax (static_cast<juce::int64> (0),
                                                  clip.lengthInSamples),
                                      clip.fadeOutSamples), nullptr);
            clipNode.setProperty (modelIds::gain, juce::jmax (0.0f, clip.gain), nullptr);
            clipNode.setProperty (modelIds::colour,
                                  static_cast<juce::int64> (clip.colour.getARGB()),
                                  nullptr);
            clipNode.setProperty (modelIds::muted, clip.muted, nullptr);
            clipNode.setProperty (modelIds::locked, clip.locked, nullptr);
            clipNode.setProperty (modelIds::takeGroupId, clip.takeGroupId, nullptr);
            clipNode.setProperty (modelIds::takeLaneIndex,
                                  juce::jmax (0, clip.takeLaneIndex), nullptr);
            clipNode.setProperty (modelIds::activeTake, clip.activeTake, nullptr);
            clipNode.setProperty (modelIds::timeStretchRatio,
                                  stretch::clampRatio (clip.timeStretchRatio), nullptr);
            clipNode.setProperty (modelIds::timeStretchProvider,
                                  clip.timeStretchProvider.isNotEmpty()
                                      ? clip.timeStretchProvider
                                      : stretch::builtInProviderId,
                                  nullptr);
            clipNode.setProperty (modelIds::reversed, clip.reversed, nullptr);
            for (const auto& marker : clip.warpMarkers)
            {
                if (marker.timelineOffsetSamples <= 0
                    || marker.timelineOffsetSamples >= clip.lengthInSamples)
                    continue;
                juce::ValueTree markerNode (modelIds::warpMarker);
                markerNode.setProperty (modelIds::id,
                                        marker.id.isNotEmpty() ? marker.id
                                                               : juce::Uuid().toString(),
                                        nullptr);
                markerNode.setProperty (modelIds::timelineOffsetSamples,
                                        marker.timelineOffsetSamples, nullptr);
                markerNode.setProperty (modelIds::sourceOffsetSamples,
                                        marker.sourceOffsetSamples, nullptr);
                clipNode.addChild (markerNode, -1, nullptr);
            }
            for (const auto& marker : clip.transientMarkers)
            {
                if (marker.timelineOffsetSamples <= 0
                    || marker.timelineOffsetSamples >= clip.lengthInSamples)
                    continue;
                juce::ValueTree markerNode (modelIds::transientMarker);
                markerNode.setProperty (modelIds::id,
                                        marker.id.isNotEmpty() ? marker.id
                                                               : juce::Uuid().toString(), nullptr);
                markerNode.setProperty (modelIds::timelineOffsetSamples,
                                        marker.timelineOffsetSamples, nullptr);
                markerNode.setProperty (modelIds::sourceOffsetSamples,
                                        marker.sourceOffsetSamples, nullptr);
                markerNode.setProperty (modelIds::strength,
                                        juce::jlimit (0.0f, 1.0f, marker.strength), nullptr);
                clipNode.addChild (markerNode, -1, nullptr);
            }
            for (const auto& variant : clip.variants)
                if (variant.sourceId.isNotEmpty() && variant.lengthInSamples > 0)
                    clipNode.addChild (variantNodeFromState (variant), -1,
                                       nullptr);
            trackNode.addChild (clipNode, -1, nullptr);
        }

        for (const auto& region : track.compRegions)
        {
            if (region.takeGroupId.isEmpty() || region.lengthInSamples <= 0)
                continue;
            juce::ValueTree regionNode (modelIds::compRegion);
            regionNode.setProperty (modelIds::id,
                                    region.id.isNotEmpty() ? region.id
                                                          : juce::Uuid().toString(),
                                    nullptr);
            regionNode.setProperty (modelIds::takeGroupId, region.takeGroupId, nullptr);
            regionNode.setProperty (modelIds::takeLaneIndex,
                                    juce::jmax (0, region.takeLaneIndex), nullptr);
            regionNode.setProperty (modelIds::timelineStartSamples,
                                    juce::jmax (static_cast<juce::int64> (0),
                                                region.timelineStartSamples),
                                    nullptr);
            regionNode.setProperty (modelIds::lengthInSamples,
                                    region.lengthInSamples, nullptr);
            regionNode.setProperty (modelIds::crossfadeSamples,
                                    juce::jmax (static_cast<juce::int64> (0),
                                                region.crossfadeSamples),
                                    nullptr);
            trackNode.addChild (regionNode, -1, nullptr);
        }

        for (const auto& clip : track.midiClips)
        {
            juce::ValueTree clipNode (modelIds::midiClip);
            clipNode.setProperty (modelIds::id,
                                  clip.id.isNotEmpty() ? clip.id : juce::Uuid().toString(), nullptr);
            clipNode.setProperty (modelIds::startBeat, juce::jmax (0.0, clip.startBeat), nullptr);
            clipNode.setProperty (modelIds::lengthBeats,
                                  juce::jmax (1.0 / 64.0, clip.lengthBeats), nullptr);
            for (const auto& note : clip.notes)
            {
                juce::ValueTree noteNode (modelIds::midiNote);
                noteNode.setProperty (modelIds::id,
                                      note.id.isNotEmpty() ? note.id : juce::Uuid().toString(), nullptr);
                noteNode.setProperty (modelIds::startBeat, juce::jmax (0.0, note.startBeat), nullptr);
                noteNode.setProperty (modelIds::lengthBeats,
                                      juce::jmax (1.0 / 64.0, note.lengthBeats), nullptr);
                noteNode.setProperty (modelIds::noteNumber,
                                      juce::jlimit (0, 127, note.noteNumber), nullptr);
                noteNode.setProperty (modelIds::velocity,
                                      juce::jlimit (0.01f, 1.0f, note.velocity), nullptr);
                noteNode.setProperty (modelIds::channel,
                                      juce::jlimit (1, 16, note.channel), nullptr);
                noteNode.setProperty (modelIds::noteId,
                                      static_cast<juce::int64> (note.noteId), nullptr);
                addMidiExpressionNodes (noteNode, note.expression);
                clipNode.addChild (noteNode, -1, nullptr);
            }
            for (const auto& controllerEvent : clip.controllers)
            {
                juce::ValueTree node (modelIds::midiController);
                node.setProperty (modelIds::id,
                                  controllerEvent.id.isNotEmpty()
                                      ? controllerEvent.id
                                      : juce::Uuid().toString(), nullptr);
                node.setProperty (modelIds::beat,
                                  juce::jmax (0.0, controllerEvent.beat), nullptr);
                node.setProperty (modelIds::channel,
                                  juce::jlimit (1, 16,
                                                controllerEvent.channel), nullptr);
                node.setProperty (modelIds::controller,
                                  juce::jlimit (0, 127,
                                                controllerEvent.controller), nullptr);
                node.setProperty (modelIds::value,
                                  static_cast<juce::int64> (
                                      controllerEvent.value), nullptr);
                node.setProperty (modelIds::noteId,
                                  static_cast<juce::int64> (
                                      controllerEvent.noteId), nullptr);
                clipNode.addChild (node, -1, nullptr);
            }
            trackNode.addChild (clipNode, -1, nullptr);
        }

        for (const auto& send : track.sends)
            addMixerSendNode (trackNode, send);

        replacement.addChild (trackNode, -1, nullptr);
    }

    for (const auto& channel : snapshot.mixerChannels)
    {
        juce::ValueTree channelNode (modelIds::mixerChannel);
        channelNode.setProperty (modelIds::id,
                                 channel.id.isNotEmpty() ? channel.id
                                                         : juce::Uuid().toString(), nullptr);
        channelNode.setProperty (modelIds::name,
                                 channel.name.isNotEmpty() ? channel.name
                                     : (channel.kind == MixerChannelKind::returnChannel
                                            ? "FX Return" : "Mix Bus"), nullptr);
        channelNode.setProperty (modelIds::kind,
                                 mixerKindName (channel.kind), nullptr);
        channelNode.setProperty (modelIds::gain,
                                 juce::jlimit (0.0f, 1.5f, channel.gain), nullptr);
        channelNode.setProperty (modelIds::pan,
                                 juce::jlimit (-1.0f, 1.0f, channel.pan), nullptr);
        channelNode.setProperty (modelIds::muted, channel.muted, nullptr);
        channelNode.setProperty (modelIds::solo, channel.solo, nullptr);
        channelNode.setProperty (modelIds::outputDestinationId,
                                 channel.outputDestinationId.isNotEmpty()
                                     ? channel.outputDestinationId : "master", nullptr);
        setBusLayoutProperties (channelNode, channel.layout);
        for (const auto& slot : channel.pluginSlots)
            addPluginSlotNode (channelNode, slot);
        for (const auto& send : channel.sends)
            addMixerSendNode (channelNode, send);
        replacement.addChild (channelNode, -1, nullptr);
    }

    for (const auto& path : snapshot.monitorPaths)
    {
        juce::ValueTree node (modelIds::monitorPath);
        node.setProperty (modelIds::id,
                          path.id.isNotEmpty() ? path.id
                                               : juce::Uuid().toString(), nullptr);
        node.setProperty (modelIds::name,
                          path.name.isNotEmpty() ? path.name
                                                 : monitorRoleName (path.role), nullptr);
        node.setProperty (modelIds::role, monitorRoleName (path.role), nullptr);
        node.setProperty (modelIds::sourceId,
                          path.sourceId.isNotEmpty() ? path.sourceId
                                                     : "master", nullptr);
        node.setProperty (modelIds::outputStartChannel,
                          juce::jlimit (0, 63, path.outputStartChannel), nullptr);
        node.setProperty (modelIds::outputChannelCount,
                          juce::jlimit (1, 16, path.outputChannelCount), nullptr);
        node.setProperty (modelIds::gain,
                          juce::jlimit (0.0f, 2.0f, path.gain), nullptr);
        node.setProperty (modelIds::muted, path.muted, nullptr);
        node.setProperty (modelIds::dimmed, path.dimmed, nullptr);
        replacement.addChild (node, -1, nullptr);
    }

    suppressNotifications = true;
    state.removeListener (this);
    state = replacement;
    state.addListener (this);
    undoManager.clearUndoHistory();
    suppressNotifications = false;
    notifyChanged();
}

juce::ValueTree ProjectModel::createEmptyState()
{
    juce::ValueTree result (modelIds::project);
    result.setProperty (modelIds::projectId, juce::Uuid().toString(), nullptr);
    result.setProperty (modelIds::name, "Untitled Project", nullptr);
    result.setProperty (modelIds::masterGain, 0.9f, nullptr);
    result.setProperty (modelIds::masterMuted, false, nullptr);
    result.setProperty (modelIds::timelineSampleRate, 48000.0, nullptr);
    result.setProperty (modelIds::timeSignatureNumerator, 4, nullptr);
    result.setProperty (modelIds::timeSignatureDenominator, 4, nullptr);
    result.setProperty (modelIds::metronomeEnabled, false, nullptr);
    result.setProperty (modelIds::metronomeGain, 0.5f, nullptr);
    result.setProperty (modelIds::lowLatencyMonitoring, false, nullptr);
    result.setProperty (modelIds::inputMonitorMode, "off", nullptr);
    result.setProperty (modelIds::manualRecordOffsetSamples, 0, nullptr);
    result.setProperty (modelIds::recordingMode, "normal", nullptr);
    result.setProperty (modelIds::preRollBars, 0, nullptr);
    result.setProperty (modelIds::punchStartSamples,
                        static_cast<juce::int64> (0), nullptr);
    result.setProperty (modelIds::punchEndSamples,
                        static_cast<juce::int64> (0), nullptr);
    result.setProperty (modelIds::loopStartSamples,
                        static_cast<juce::int64> (0), nullptr);
    result.setProperty (modelIds::loopEndSamples,
                        static_cast<juce::int64> (0), nullptr);
    result.setProperty (modelIds::controlRoomGain, 1.0f, nullptr);
    result.setProperty (modelIds::controlRoomDimmed, false, nullptr);
    result.setProperty (modelIds::controlRoomMuted, false, nullptr);
    result.setProperty (modelIds::syncSource, "internal", nullptr);
    result.setProperty (modelIds::sendMidiClock, false, nullptr);
    result.setProperty (modelIds::followExternalStartStop, true, nullptr);
    result.setProperty (modelIds::mtcFramesPerSecond, 30, nullptr);
    result.setProperty (modelIds::jitterToleranceSamples, 256, nullptr);
    result.setProperty (modelIds::producerRootPitchClass, 0, nullptr);
    result.setProperty (modelIds::producerScale, "major", nullptr);
    result.setProperty (modelIds::producerStyle, "balanced", nullptr);
    result.setProperty (modelIds::producerBars, 4, nullptr);
    result.setProperty (modelIds::producerVariation,
                        static_cast<juce::int64> (1), nullptr);
    juce::ValueTree tempoNode (modelIds::tempoPoint);
    tempoNode.setProperty (modelIds::id, juce::Uuid().toString(), nullptr);
    tempoNode.setProperty (modelIds::beat, 0.0, nullptr);
    tempoNode.setProperty (modelIds::bpm, 120.0, nullptr);
    tempoNode.setProperty (modelIds::curve, "step", nullptr);
    result.addChild (tempoNode, -1, nullptr);
    juce::ValueTree signatureNode (modelIds::timeSignaturePoint);
    signatureNode.setProperty (modelIds::id, juce::Uuid().toString(), nullptr);
    signatureNode.setProperty (modelIds::beat, 0.0, nullptr);
    signatureNode.setProperty (modelIds::numerator, 4, nullptr);
    signatureNode.setProperty (modelIds::denominator, 4, nullptr);
    result.addChild (signatureNode, -1, nullptr);
    return result;
}

juce::Colour ProjectModel::defaultTrackColour (int index)
{
    static constexpr juce::uint32 colours[] {
        0xffb97d3e, 0xff4f9f73, 0xff64b5c4, 0xff6b8fcf,
        0xffc65f76, 0xffd8aa47, 0xff9a7358, 0xff6a8d58
    };
    return juce::Colour (colours[static_cast<size_t> (index)
                                 % (sizeof (colours) / sizeof (colours[0]))]);
}

juce::ValueTree ProjectModel::findTrack (const juce::String& trackId) const
{
    for (const auto child : state)
        if (child.hasType (modelIds::track)
            && child.getProperty (modelIds::id).toString() == trackId)
            return child;

    return {};
}

juce::ValueTree ProjectModel::findMixerChannel (
    const juce::String& channelId) const
{
    for (const auto child : state)
        if (child.hasType (modelIds::mixerChannel)
            && child.getProperty (modelIds::id).toString() == channelId)
            return child;
    return {};
}

juce::ValueTree ProjectModel::findMixerOwner (const juce::String& ownerId) const
{
    const auto track = findTrack (ownerId);
    return track.isValid() ? track : findMixerChannel (ownerId);
}

juce::ValueTree ProjectModel::findMixerSend (const juce::String& ownerId,
                                             const juce::String& sendId) const
{
    const auto owner = findMixerOwner (ownerId);
    for (const auto child : owner)
        if (child.hasType (modelIds::mixerSend)
            && child.getProperty (modelIds::id).toString() == sendId)
            return child;
    return {};
}

juce::ValueTree ProjectModel::findTimelineItem (const juce::Identifier& type,
                                                const juce::String& itemId) const
{
    for (const auto child : state)
        if (child.hasType (type)
            && child.getProperty (modelIds::id).toString() == itemId)
            return child;
    return {};
}

juce::ValueTree ProjectModel::findAutomationLane (
    const juce::String& laneId) const
{
    return findTimelineItem (modelIds::automationLane, laneId);
}

juce::ValueTree ProjectModel::findMediaSource (const juce::String& sourceId) const
{
    for (const auto child : state)
        if (child.hasType (modelIds::mediaSource)
            && child.getProperty (modelIds::id).toString() == sourceId)
            return child;

    return {};
}

juce::ValueTree ProjectModel::findClip (const juce::String& trackId,
                                        const juce::String& clipId) const
{
    const auto trackNode = findTrack (trackId);

    for (const auto child : trackNode)
        if (child.hasType (modelIds::clip)
            && child.getProperty (modelIds::id).toString() == clipId)
            return child;

    return {};
}

juce::ValueTree ProjectModel::findMidiClip (const juce::String& trackId,
                                            const juce::String& clipId) const
{
    const auto trackNode = findTrack (trackId);
    for (const auto child : trackNode)
        if (child.hasType (modelIds::midiClip)
            && child.getProperty (modelIds::id).toString() == clipId)
            return child;
    return {};
}

juce::ValueTree ProjectModel::findPluginSlot (
    const juce::String& ownerId, const juce::String& slotId) const
{
    const auto owner = findMixerOwner (ownerId);
    for (const auto child : owner)
        if (child.hasType (modelIds::pluginSlot)
            && child.getProperty (modelIds::id).toString() == slotId)
            return child;
    return {};
}

void ProjectModel::notifyChanged()
{
    if (! suppressNotifications && onChanged)
        onChanged();
}

void ProjectModel::valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&)
{
    notifyChanged();
}

void ProjectModel::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&)
{
    notifyChanged();
}

void ProjectModel::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int)
{
    notifyChanged();
}

void ProjectModel::valueTreeChildOrderChanged (juce::ValueTree&, int, int)
{
    notifyChanged();
}

void ProjectModel::valueTreeParentChanged (juce::ValueTree&)
{
    notifyChanged();
}
}
