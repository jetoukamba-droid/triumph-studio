#include "ProjectDocument.h"

#include "core/TimelineMath.h"
#include "core/TimeStretch.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace triumph
{
namespace
{
juce::var makeClipVariantObject (const ClipVariantState& variant)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("id", variant.id);
    object->setProperty ("name", variant.name);
    object->setProperty ("sourceId", variant.sourceId);
    object->setProperty ("sourceOffsetSamples",
                         variant.sourceOffsetSamples);
    object->setProperty ("lengthInSamples", variant.lengthInSamples);
    object->setProperty ("fadeInSamples", variant.fadeInSamples);
    object->setProperty ("fadeOutSamples", variant.fadeOutSamples);
    object->setProperty ("gain", variant.gain);
    object->setProperty ("timeStretchRatio", variant.timeStretchRatio);
    object->setProperty ("timeStretchProvider", variant.timeStretchProvider);
    object->setProperty ("reversed", variant.reversed);
    juce::Array<juce::var> warpMarkers;
    for (const auto& marker : variant.warpMarkers)
    {
        auto markerObject = std::make_unique<juce::DynamicObject>();
        markerObject->setProperty ("id", marker.id);
        markerObject->setProperty ("timelineOffsetSamples",
                                   marker.timelineOffsetSamples);
        markerObject->setProperty ("sourceOffsetSamples",
                                   marker.sourceOffsetSamples);
        warpMarkers.add (juce::var (markerObject.release()));
    }
    object->setProperty ("warpMarkers", juce::var (warpMarkers));
    juce::Array<juce::var> transientMarkers;
    for (const auto& marker : variant.transientMarkers)
    {
        auto markerObject = std::make_unique<juce::DynamicObject>();
        markerObject->setProperty ("id", marker.id);
        markerObject->setProperty ("timelineOffsetSamples",
                                   marker.timelineOffsetSamples);
        markerObject->setProperty ("sourceOffsetSamples",
                                   marker.sourceOffsetSamples);
        markerObject->setProperty ("strength", marker.strength);
        transientMarkers.add (juce::var (markerObject.release()));
    }
    object->setProperty ("transientMarkers", juce::var (transientMarkers));
    return juce::var (object.release());
}

juce::var makeClipObject (const AudioClipState& clip)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("id", clip.id);
    object->setProperty ("name", clip.name);
    object->setProperty ("sourceId", clip.sourceId);
    object->setProperty ("timelineStartSamples", clip.timelineStartSamples);
    object->setProperty ("sourceOffsetSamples", clip.sourceOffsetSamples);
    object->setProperty ("lengthInSamples", clip.lengthInSamples);
    object->setProperty ("fadeInSamples", clip.fadeInSamples);
    object->setProperty ("fadeOutSamples", clip.fadeOutSamples);
    object->setProperty ("gain", clip.gain);
    object->setProperty ("colour",
                         static_cast<juce::int64> (clip.colour.getARGB()));
    object->setProperty ("muted", clip.muted);
    object->setProperty ("locked", clip.locked);
    object->setProperty ("takeGroupId", clip.takeGroupId);
    object->setProperty ("takeLaneIndex", clip.takeLaneIndex);
    object->setProperty ("activeTake", clip.activeTake);
    object->setProperty ("timeStretchRatio", clip.timeStretchRatio);
    object->setProperty ("timeStretchProvider", clip.timeStretchProvider);
    object->setProperty ("reversed", clip.reversed);
    juce::Array<juce::var> warpMarkers;
    for (const auto& marker : clip.warpMarkers)
    {
        auto markerObject = std::make_unique<juce::DynamicObject>();
        markerObject->setProperty ("id", marker.id);
        markerObject->setProperty ("timelineOffsetSamples",
                                   marker.timelineOffsetSamples);
        markerObject->setProperty ("sourceOffsetSamples",
                                   marker.sourceOffsetSamples);
        warpMarkers.add (juce::var (markerObject.release()));
    }
    object->setProperty ("warpMarkers", juce::var (warpMarkers));
    juce::Array<juce::var> transientMarkers;
    for (const auto& marker : clip.transientMarkers)
    {
        auto markerObject = std::make_unique<juce::DynamicObject>();
        markerObject->setProperty ("id", marker.id);
        markerObject->setProperty ("timelineOffsetSamples",
                                   marker.timelineOffsetSamples);
        markerObject->setProperty ("sourceOffsetSamples",
                                   marker.sourceOffsetSamples);
        markerObject->setProperty ("strength", marker.strength);
        transientMarkers.add (juce::var (markerObject.release()));
    }
    object->setProperty ("transientMarkers", juce::var (transientMarkers));
    juce::Array<juce::var> variants;
    for (const auto& variant : clip.variants)
        variants.add (makeClipVariantObject (variant));
    object->setProperty ("variants", juce::var (variants));
    return juce::var (object.release());
}

juce::var makeCompRegionObject (const CompRegionState& region)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("id", region.id);
    object->setProperty ("takeGroupId", region.takeGroupId);
    object->setProperty ("takeLaneIndex", region.takeLaneIndex);
    object->setProperty ("timelineStartSamples", region.timelineStartSamples);
    object->setProperty ("lengthInSamples", region.lengthInSamples);
    object->setProperty ("crossfadeSamples", region.crossfadeSamples);
    return juce::var (object.release());
}

juce::var makeMixerSendObject (const MixerSendState& send)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("id", send.id);
    object->setProperty ("destinationId", send.destinationId);
    object->setProperty ("gain", send.gain);
    object->setProperty ("enabled", send.enabled);
    object->setProperty ("preFader", send.preFader);
    object->setProperty ("sidechain", send.sidechain);
    return juce::var (object.release());
}

void addMixerSends (juce::DynamicObject& object,
                    const std::vector<MixerSendState>& sends)
{
    juce::Array<juce::var> values;
    for (const auto& send : sends)
        values.add (makeMixerSendObject (send));
    object.setProperty ("sends", juce::var (values));
}

void addBusLayout (juce::DynamicObject& object,
                   const MixerBusLayoutState& layout)
{
    object.setProperty ("inputChannels",
                        juce::jlimit (0, 16, layout.inputChannels));
    object.setProperty ("outputChannels",
                        juce::jlimit (1, 16, layout.outputChannels));
    object.setProperty ("sidechainChannels",
                        juce::jlimit (0, 16, layout.sidechainChannels));
    object.setProperty ("explicitLayout", layout.explicitLayout);
}

MixerBusLayoutState busLayoutFromObject (const juce::DynamicObject& object)
{
    MixerBusLayoutState layout;
    layout.inputChannels = juce::jlimit (0, 16, object.hasProperty (
        "inputChannels") ? static_cast<int> (object.getProperty (
            "inputChannels")) : 2);
    layout.outputChannels = juce::jlimit (1, 16, object.hasProperty (
        "outputChannels") ? static_cast<int> (object.getProperty (
            "outputChannels")) : 2);
    layout.sidechainChannels = juce::jlimit (0, 16, object.hasProperty (
        "sidechainChannels") ? static_cast<int> (object.getProperty (
            "sidechainChannels")) : 0);
    layout.explicitLayout = static_cast<bool> (object.getProperty (
        "explicitLayout"));
    return layout;
}

juce::var makePluginSlotObject (const PluginSlotState& slot)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("id", slot.id);
    object->setProperty ("stablePluginId", slot.stablePluginId);
    object->setProperty ("name", slot.name);
    object->setProperty ("descriptionXml", slot.descriptionXml);
    object->setProperty ("stateBase64", slot.stateBase64);
    object->setProperty ("role",
                         slot.role == PluginSlotRole::instrument
                             ? "instrument" : "insert");
    object->setProperty ("isolation",
                         slot.isolation == PluginIsolationMode::trustedInProcess
                             ? "trusted-in-process" : "worker");
    object->setProperty ("order", juce::jlimit (0, 15, slot.order));
    addBusLayout (*object, slot.layout);
    object->setProperty ("bypassed", slot.bypassed);
    object->setProperty ("available", slot.available);
    object->setProperty ("latencySamples",
                         juce::jlimit (0, 262143, slot.latencySamples));
    object->setProperty ("restartCount", juce::jmax (0, slot.restartCount));
    return juce::var (object.release());
}

void addPluginSlots (juce::DynamicObject& object,
                     const std::vector<PluginSlotState>& slots)
{
    juce::Array<juce::var> values;
    for (const auto& slot : slots)
        values.add (makePluginSlotObject (slot));
    object.setProperty ("pluginSlots", juce::var (values));
}

std::vector<PluginSlotState> pluginSlotsFromObject (
    const juce::DynamicObject& object)
{
    std::vector<PluginSlotState> slots;
    if (const auto* values = object.getProperty ("pluginSlots").getArray())
        for (const auto& value : *values)
        {
            const auto* slotObject = value.getDynamicObject();
            if (slotObject == nullptr)
                continue;
            PluginSlotState slot;
            slot.id = slotObject->getProperty ("id").toString();
            if (slot.id.isEmpty()) slot.id = juce::Uuid().toString();
            slot.stablePluginId = slotObject->getProperty (
                "stablePluginId").toString();
            slot.name = slotObject->getProperty ("name").toString();
            slot.descriptionXml = slotObject->getProperty (
                "descriptionXml").toString();
            slot.stateBase64 = slotObject->getProperty (
                "stateBase64").toString();
            slot.role = slotObject->getProperty ("role").toString()
                            == "instrument"
                        ? PluginSlotRole::instrument
                        : PluginSlotRole::insertEffect;
            slot.isolation = slotObject->getProperty ("isolation").toString()
                                     == "trusted-in-process"
                                 ? PluginIsolationMode::trustedInProcess
                                 : PluginIsolationMode::workerProcess;
            slot.order = juce::jlimit (0, 15, static_cast<int> (
                slotObject->getProperty ("order")));
            slot.layout = busLayoutFromObject (*slotObject);
            slot.bypassed = static_cast<bool> (slotObject->getProperty (
                "bypassed"));
            const auto available = slotObject->getProperty ("available");
            slot.available = available.isVoid()
                                 || static_cast<bool> (available);
            slot.latencySamples = juce::jlimit (0, 262143,
                static_cast<int> (slotObject->getProperty ("latencySamples")));
            slot.restartCount = juce::jmax (0, static_cast<int> (
                slotObject->getProperty ("restartCount")));
            if (slot.stablePluginId.isNotEmpty()
                && slot.descriptionXml.isNotEmpty())
                slots.push_back (std::move (slot));
        }
    std::stable_sort (slots.begin(), slots.end(), [] (const auto& a,
                                                      const auto& b)
    {
        return a.order < b.order;
    });
    return slots;
}

juce::var makeTrackObject (const TrackState& track)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("id", track.id);
    object->setProperty ("name", track.name);
    object->setProperty ("colour", static_cast<juce::int64> (track.colour.getARGB()));
    object->setProperty ("gain", track.gain);
    object->setProperty ("pan", track.pan);
    object->setProperty ("muted", track.muted);
    object->setProperty ("solo", track.solo);
    object->setProperty ("recordArmed", track.recordArmed);
    object->setProperty ("inputStartChannel", track.inputStartChannel);
    object->setProperty ("inputChannelCount", track.inputChannelCount);
    object->setProperty ("recordingTapPoint",
                         track.recordingTapPoint == RecordingTapPoint::programOutput
                             ? "device-output" : "hardware-input");
    object->setProperty ("recordOffsetSamples", track.recordOffsetSamples);
    object->setProperty ("isInstrument", track.isInstrument);
    object->setProperty ("pluginName", track.pluginName);
    object->setProperty ("pluginDescriptionXml", track.pluginDescriptionXml);
    object->setProperty ("pluginStateBase64", track.pluginStateBase64);
    object->setProperty ("pluginBypassed", track.pluginBypassed);
    object->setProperty ("pluginLatencySamples", track.pluginLatencySamples);
    object->setProperty ("pluginAvailable", track.pluginAvailable);
    object->setProperty ("manualLatencyOffsetSamples",
                         track.manualLatencyOffsetSamples);
    object->setProperty ("outputDestinationId", track.outputDestinationId);
    object->setProperty ("frozen", track.frozen);
    object->setProperty ("freezeSourceId", track.freezeSourceId);
    addBusLayout (*object, track.layout);
    addPluginSlots (*object, track.pluginSlots);

    juce::Array<juce::var> clips;
    for (const auto& clip : track.clips)
        clips.add (makeClipObject (clip));

    object->setProperty ("clips", juce::var (clips));

    juce::Array<juce::var> compRegions;
    for (const auto& region : track.compRegions)
        compRegions.add (makeCompRegionObject (region));
    object->setProperty ("compRegions", juce::var (compRegions));

    juce::Array<juce::var> midiClips;
    for (const auto& clip : track.midiClips)
    {
        auto clipObject = std::make_unique<juce::DynamicObject>();
        clipObject->setProperty ("id", clip.id);
        clipObject->setProperty ("startBeat", clip.startBeat);
        clipObject->setProperty ("lengthBeats", clip.lengthBeats);
        juce::Array<juce::var> notes;
        for (const auto& note : clip.notes)
        {
            auto noteObject = std::make_unique<juce::DynamicObject>();
            noteObject->setProperty ("id", note.id);
            noteObject->setProperty ("startBeat", note.startBeat);
            noteObject->setProperty ("lengthBeats", note.lengthBeats);
            noteObject->setProperty ("noteNumber", note.noteNumber);
            noteObject->setProperty ("velocity", note.velocity);
            noteObject->setProperty ("channel", note.channel);
            noteObject->setProperty ("noteId",
                                     static_cast<juce::int64> (note.noteId));
            juce::Array<juce::var> expression;
            for (const auto& point : note.expression)
            {
                auto pointObject = std::make_unique<juce::DynamicObject>();
                pointObject->setProperty ("id", point.id);
                pointObject->setProperty ("offsetBeats", point.offsetBeats);
                pointObject->setProperty ("value", point.value);
                pointObject->setProperty (
                    "type",
                    point.type == MidiNoteState::ExpressionPoint::Type::pressure
                        ? "pressure"
                        : point.type == MidiNoteState::ExpressionPoint::Type::timbre
                            ? "timbre" : "pitch");
                expression.add (juce::var (pointObject.release()));
            }
            noteObject->setProperty ("expression", juce::var (expression));
            notes.add (juce::var (noteObject.release()));
        }
        clipObject->setProperty ("notes", juce::var (notes));
        juce::Array<juce::var> controllers;
        for (const auto& event : clip.controllers)
        {
            auto eventObject = std::make_unique<juce::DynamicObject>();
            eventObject->setProperty ("id", event.id);
            eventObject->setProperty ("beat", event.beat);
            eventObject->setProperty ("channel", event.channel);
            eventObject->setProperty ("controller", event.controller);
            eventObject->setProperty ("value",
                                      static_cast<juce::int64> (event.value));
            eventObject->setProperty ("noteId",
                                      static_cast<juce::int64> (event.noteId));
            controllers.add (juce::var (eventObject.release()));
        }
        clipObject->setProperty ("controllers", juce::var (controllers));
        midiClips.add (juce::var (clipObject.release()));
    }
    object->setProperty ("midiClips", juce::var (midiClips));
    addMixerSends (*object, track.sends);
    return juce::var (object.release());
}

juce::var makeMixerChannelObject (const MixerChannelState& channel)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("id", channel.id);
    object->setProperty ("name", channel.name);
    object->setProperty ("kind",
                         channel.kind == MixerChannelKind::returnChannel
                             ? "return" : "bus");
    object->setProperty ("gain", channel.gain);
    object->setProperty ("pan", channel.pan);
    object->setProperty ("muted", channel.muted);
    object->setProperty ("solo", channel.solo);
    object->setProperty ("outputDestinationId", channel.outputDestinationId);
    addBusLayout (*object, channel.layout);
    addPluginSlots (*object, channel.pluginSlots);
    addMixerSends (*object, channel.sends);
    return juce::var (object.release());
}

std::vector<MixerSendState> mixerSendsFromObject (const juce::DynamicObject& object)
{
    std::vector<MixerSendState> sends;
    if (const auto* values = object.getProperty ("sends").getArray())
        for (const auto& value : *values)
        {
            const auto* sendObject = value.getDynamicObject();
            if (sendObject == nullptr)
                continue;
            MixerSendState send;
            send.id = sendObject->getProperty ("id").toString();
            if (send.id.isEmpty())
                send.id = juce::Uuid().toString();
            send.destinationId = sendObject->getProperty (
                "destinationId").toString();
            send.gain = juce::jlimit (0.0f, 2.0f, static_cast<float> (
                sendObject->getProperty ("gain")));
            const auto enabled = sendObject->getProperty ("enabled");
            send.enabled = enabled.isVoid() || static_cast<bool> (enabled);
            send.preFader = static_cast<bool> (
                sendObject->getProperty ("preFader"));
            send.sidechain = static_cast<bool> (
                sendObject->getProperty ("sidechain"));
            if (send.destinationId.isNotEmpty())
                sends.push_back (std::move (send));
        }
    return sends;
}

juce::var makeSourceObject (const MediaSourceState& source,
                            const juce::File& projectDirectory)
{
    auto object = std::make_unique<juce::DynamicObject>();
    const auto canUseRelativePath = source.file.getParentDirectory() == projectDirectory
                                    || source.file.isAChildOf (projectDirectory);
    object->setProperty ("id", source.id);
    object->setProperty ("path",
                         canUseRelativePath ? source.file.getRelativePathFrom (projectDirectory)
                                            : source.file.getFullPathName());
    object->setProperty ("pathIsRelative", canUseRelativePath);
    object->setProperty ("sampleRate", source.sampleRate);
    object->setProperty ("channels", source.channels);
    object->setProperty ("lengthInSamples", source.lengthInSamples);
    object->setProperty ("assetFingerprint", source.assetFingerprint);
    return juce::var (object.release());
}

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

juce::var makeRepairRecordObject (const ProjectRepairRecordState& record)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("id", record.id);
    object->setProperty ("severity", record.severity);
    object->setProperty ("area", record.area);
    object->setProperty ("subjectId", record.subjectId);
    object->setProperty ("message", record.message);
    return juce::var (object.release());
}

ProjectRepairRecordState repairRecordFromObject (
    const juce::DynamicObject& object)
{
    ProjectRepairRecordState record;
    record.id = object.getProperty ("id").toString();
    if (record.id.isEmpty())
        record.id = juce::Uuid().toString();
    record.severity = object.getProperty ("severity").toString();
    if (record.severity.isEmpty())
        record.severity = "warning";
    record.area = object.getProperty ("area").toString();
    record.subjectId = object.getProperty ("subjectId").toString();
    record.message = object.getProperty ("message").toString();
    return record;
}

void addRepairRecord (ProjectState& state,
                      juce::String area,
                      juce::String subjectId,
                      juce::String message)
{
    ProjectRepairRecordState record;
    record.id = juce::Uuid().toString();
    record.area = std::move (area);
    record.subjectId = std::move (subjectId);
    record.message = std::move (message);
    state.repairRecords.push_back (std::move (record));
}

juce::var makePersistentUndoEntryObject (
    const PersistentUndoEntryState& entry)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("description", entry.description);
    object->setProperty ("stateXml", entry.stateXml);
    return juce::var (object.release());
}

juce::var makePersistentUndoEntriesObject (
    const std::vector<PersistentUndoEntryState>& entries)
{
    juce::Array<juce::var> values;
    for (const auto& entry : entries)
        if (entry.stateXml.isNotEmpty())
            values.add (makePersistentUndoEntryObject (entry));
    return juce::var (values);
}

PersistentUndoEntryState persistentUndoEntryFromObject (
    const juce::DynamicObject& object)
{
    PersistentUndoEntryState entry;
    entry.description = object.getProperty ("description").toString();
    if (entry.description.isEmpty())
        entry.description = "Edit Project";
    entry.stateXml = object.getProperty ("stateXml").toString();
    return entry;
}

std::vector<PersistentUndoEntryState> persistentUndoEntriesFromObject (
    const juce::DynamicObject& object,
    const juce::Identifier& property)
{
    std::vector<PersistentUndoEntryState> entries;
    if (const auto* values = object.getProperty (property).getArray())
        for (const auto& value : *values)
            if (const auto* entryObject = value.getDynamicObject())
            {
                auto entry = persistentUndoEntryFromObject (*entryObject);
                if (entry.stateXml.isNotEmpty())
                    entries.push_back (std::move (entry));
            }
    return entries;
}

juce::var makeUndoManifestObject (const UndoManifestState& manifest)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("hadUndo", manifest.hadUndo);
    object->setProperty ("hadRedo", manifest.hadRedo);
    object->setProperty ("undoDescription", manifest.undoDescription);
    object->setProperty ("redoDescription", manifest.redoDescription);
    object->setProperty ("historyFormatVersion", 1);
    object->setProperty ("undoHistory",
                         makePersistentUndoEntriesObject (manifest.undoHistory));
    object->setProperty ("redoHistory",
                         makePersistentUndoEntriesObject (manifest.redoHistory));
    return juce::var (object.release());
}

UndoManifestState undoManifestFromObject (const juce::DynamicObject& object)
{
    UndoManifestState manifest;
    manifest.hadUndo = static_cast<bool> (object.getProperty ("hadUndo"));
    manifest.hadRedo = static_cast<bool> (object.getProperty ("hadRedo"));
    manifest.undoDescription = object.getProperty (
        "undoDescription").toString();
    manifest.redoDescription = object.getProperty (
        "redoDescription").toString();
    manifest.undoHistory = persistentUndoEntriesFromObject (
        object, "undoHistory");
    manifest.redoHistory = persistentUndoEntriesFromObject (
        object, "redoHistory");
    return manifest;
}

juce::int64 int64Property (const juce::DynamicObject& object,
                           const juce::Identifier& property,
                           juce::int64 fallback = 0);

ClipVariantState clipVariantFromObject (const juce::DynamicObject& object)
{
    ClipVariantState variant;
    variant.id = object.getProperty ("id").toString();
    if (variant.id.isEmpty())
        variant.id = juce::Uuid().toString();
    variant.name = object.getProperty ("name").toString();
    if (variant.name.isEmpty())
        variant.name = "Variant";
    variant.sourceId = object.getProperty ("sourceId").toString();
    variant.sourceOffsetSamples = juce::jmax (
        static_cast<juce::int64> (0),
        int64Property (object, "sourceOffsetSamples"));
    variant.lengthInSamples = juce::jmax (
        static_cast<juce::int64> (1),
        int64Property (object, "lengthInSamples", 1));
    variant.fadeInSamples = juce::jlimit (
        static_cast<juce::int64> (0), variant.lengthInSamples,
        int64Property (object, "fadeInSamples"));
    variant.fadeOutSamples = juce::jlimit (
        static_cast<juce::int64> (0), variant.lengthInSamples,
        int64Property (object, "fadeOutSamples"));
    variant.gain = juce::jmax (
        0.0f, object.hasProperty ("gain")
                  ? static_cast<float> (object.getProperty ("gain")) : 1.0f);
    const auto stretchRatio = object.hasProperty ("timeStretchRatio")
        ? static_cast<double> (object.getProperty ("timeStretchRatio")) : 1.0;
    variant.timeStretchRatio = stretch::clampRatio (stretchRatio);
    variant.timeStretchProvider = object.getProperty (
        "timeStretchProvider").toString();
    if (variant.timeStretchProvider.isEmpty())
        variant.timeStretchProvider = stretch::builtInProviderId;
    variant.reversed = static_cast<bool> (object.getProperty ("reversed"));

    if (const auto* markers = object.getProperty ("warpMarkers").getArray())
        for (const auto& markerValue : *markers)
        {
            const auto* markerObject = markerValue.getDynamicObject();
            if (markerObject == nullptr)
                continue;
            WarpMarkerState marker;
            marker.id = markerObject->getProperty ("id").toString();
            if (marker.id.isEmpty())
                marker.id = juce::Uuid().toString();
            marker.timelineOffsetSamples = int64Property (
                *markerObject, "timelineOffsetSamples");
            marker.sourceOffsetSamples = juce::jmax (
                static_cast<juce::int64> (0),
                int64Property (*markerObject, "sourceOffsetSamples"));
            if (marker.timelineOffsetSamples > 0
                && marker.timelineOffsetSamples < variant.lengthInSamples)
                variant.warpMarkers.push_back (std::move (marker));
        }

    if (const auto* markers = object.getProperty (
            "transientMarkers").getArray())
        for (const auto& markerValue : *markers)
        {
            const auto* markerObject = markerValue.getDynamicObject();
            if (markerObject == nullptr)
                continue;
            TransientMarkerState marker;
            marker.id = markerObject->getProperty ("id").toString();
            if (marker.id.isEmpty())
                marker.id = juce::Uuid().toString();
            marker.timelineOffsetSamples = int64Property (
                *markerObject, "timelineOffsetSamples");
            marker.sourceOffsetSamples = juce::jmax (
                static_cast<juce::int64> (0),
                int64Property (*markerObject, "sourceOffsetSamples"));
            marker.strength = juce::jlimit (0.0f, 1.0f, static_cast<float> (
                markerObject->getProperty ("strength")));
            if (marker.timelineOffsetSamples > 0
                && marker.timelineOffsetSamples < variant.lengthInSamples)
                variant.transientMarkers.push_back (std::move (marker));
        }

    return variant;
}

juce::int64 int64Property (const juce::DynamicObject& object,
                           const juce::Identifier& property,
                           juce::int64 fallback)
{
    const auto value = object.getProperty (property);
    return value.isVoid() ? fallback : static_cast<juce::int64> (value);
}

void migrateLegacyClipLengthsToTimelineRate (ProjectState& state)
{
    if (state.timelineSampleRate <= 0.0)
        state.timelineSampleRate = 48000.0;

    for (auto& track : state.tracks)
    {
        for (auto& clip : track.clips)
        {
            const auto source = std::find_if (state.mediaSources.begin(),
                                              state.mediaSources.end(),
                                              [&clip] (const auto& candidate)
            {
                return candidate.id == clip.sourceId;
            });

            if (source != state.mediaSources.end() && source->sampleRate > 0.0)
                clip.lengthInSamples = juce::jmax (
                    static_cast<juce::int64> (1),
                    static_cast<juce::int64> (timeline::convertSampleCount (
                        clip.lengthInSamples,
                        source->sampleRate,
                        state.timelineSampleRate)));
        }
    }
}
}

juce::Result ProjectDocument::save (const juce::File& destination,
                                    const ProjectModel& project)
{
    const auto snapshot = project.createSnapshot();
    auto root = std::make_unique<juce::DynamicObject>();
    root->setProperty ("format", "TriumphStudioProject");
    root->setProperty ("formatVersion", currentFormatVersion);
    root->setProperty ("projectId", snapshot.projectId);
    root->setProperty ("name", snapshot.name);
    root->setProperty ("masterGain", snapshot.masterGain);
    root->setProperty ("masterMuted", snapshot.masterMuted);
    root->setProperty ("timelineSampleRate", snapshot.timelineSampleRate);
    root->setProperty ("timeSignatureNumerator", snapshot.timeSignatureNumerator);
    root->setProperty ("timeSignatureDenominator", snapshot.timeSignatureDenominator);
    root->setProperty ("metronomeEnabled", snapshot.metronomeEnabled);
    root->setProperty ("metronomeGain", snapshot.metronomeGain);
    root->setProperty ("lowLatencyMonitoring", snapshot.lowLatencyMonitoring);
    root->setProperty (
        "inputMonitorMode",
        snapshot.inputMonitorMode == InputMonitorMode::always ? "always"
            : snapshot.inputMonitorMode == InputMonitorMode::whileArmed
                  ? "armed" : "off");
    root->setProperty ("manualRecordOffsetSamples",
                       snapshot.manualRecordOffsetSamples);
    const auto& recording = snapshot.recordingSettings;
    root->setProperty ("recordingMode",
        recording.mode == RecordingMode::punch ? "punch"
            : recording.mode == RecordingMode::loop ? "loop" : "normal");
    root->setProperty ("preRollBars", recording.preRollBars);
    root->setProperty ("punchStartSamples", recording.punchStartSamples);
    root->setProperty ("punchEndSamples", recording.punchEndSamples);
    root->setProperty ("loopStartSamples", recording.loopStartSamples);
    root->setProperty ("loopEndSamples", recording.loopEndSamples);
    root->setProperty ("controlRoomGain", recording.controlRoomGain);
    root->setProperty ("controlRoomDimmed", recording.controlRoomDimmed);
    root->setProperty ("controlRoomMuted", recording.controlRoomMuted);
    const auto& sync = snapshot.syncSettings;
    root->setProperty (
        "syncSource",
        sync.source == SyncSourceState::midiClock ? "midi-clock"
            : sync.source == SyncSourceState::midiTimeCode ? "mtc"
            : sync.source == SyncSourceState::link ? "link" : "internal");
    root->setProperty ("sendMidiClock", sync.sendMidiClock);
    root->setProperty ("followExternalStartStop",
                       sync.followExternalStartStop);
    root->setProperty ("mtcFramesPerSecond", sync.mtcFramesPerSecond);
    root->setProperty ("jitterToleranceSamples",
                       sync.jitterToleranceSamples);
    root->setProperty ("producerRootPitchClass",
                       snapshot.producerSettings.rootPitchClass);
    root->setProperty ("producerScale", snapshot.producerSettings.scale);
    root->setProperty ("producerStyle", snapshot.producerSettings.style);
    root->setProperty ("producerBars", snapshot.producerSettings.bars);
    root->setProperty ("producerVariation", static_cast<juce::int64> (
        snapshot.producerSettings.variation));

    juce::Array<juce::var> tempoPoints;
    for (const auto& point : snapshot.tempoPoints)
    {
        auto object = std::make_unique<juce::DynamicObject>();
        object->setProperty ("id", point.id);
        object->setProperty ("beat", point.beat);
        object->setProperty ("bpm", point.bpm);
        object->setProperty ("curve", point.curve == TempoCurve::linear
                                          ? "linear" : "step");
        tempoPoints.add (juce::var (object.release()));
    }
    root->setProperty ("tempoPoints", juce::var (tempoPoints));

    juce::Array<juce::var> timeSignatures;
    for (const auto& signature : snapshot.timeSignatures)
    {
        auto object = std::make_unique<juce::DynamicObject>();
        object->setProperty ("id", signature.id);
        object->setProperty ("beat", signature.beat);
        object->setProperty ("numerator", signature.numerator);
        object->setProperty ("denominator", signature.denominator);
        timeSignatures.add (juce::var (object.release()));
    }
    root->setProperty ("timeSignatures", juce::var (timeSignatures));

    juce::Array<juce::var> markers;
    for (const auto& marker : snapshot.markers)
    {
        auto object = std::make_unique<juce::DynamicObject>();
        object->setProperty ("id", marker.id);
        object->setProperty ("beat", marker.beat);
        object->setProperty ("name", marker.name);
        object->setProperty ("colour",
                             static_cast<juce::int64> (marker.colour.getARGB()));
        markers.add (juce::var (object.release()));
    }
    root->setProperty ("markers", juce::var (markers));

    juce::Array<juce::var> automationLanes;
    for (const auto& lane : snapshot.automationLanes)
    {
        auto object = std::make_unique<juce::DynamicObject>();
        object->setProperty ("id", lane.id);
        object->setProperty ("targetId", lane.targetId);
        object->setProperty ("parameterId", lane.parameterId);
        const auto mode = lane.mode == AutomationMode::touch ? "touch"
            : lane.mode == AutomationMode::latch ? "latch"
            : lane.mode == AutomationMode::write ? "write" : "read";
        object->setProperty ("mode", mode);
        object->setProperty ("enabled", lane.enabled);
        juce::Array<juce::var> points;
        for (const auto& point : lane.points)
        {
            auto pointObject = std::make_unique<juce::DynamicObject>();
            pointObject->setProperty ("id", point.id);
            pointObject->setProperty ("samplePosition", point.samplePosition);
            pointObject->setProperty ("value", point.value);
            const auto curve = point.curve == AutomationCurve::hold ? "hold"
                : point.curve == AutomationCurve::smooth ? "smooth" : "linear";
            pointObject->setProperty ("curve", curve);
            points.add (juce::var (pointObject.release()));
        }
        object->setProperty ("points", juce::var (points));
        automationLanes.add (juce::var (object.release()));
    }
    root->setProperty ("automationLanes", juce::var (automationLanes));

    juce::Array<juce::var> sources;
    for (const auto& source : snapshot.mediaSources)
        sources.add (makeSourceObject (source, destination.getParentDirectory()));
    root->setProperty ("mediaSources", juce::var (sources));

    juce::Array<juce::var> tracks;
    for (const auto& track : snapshot.tracks)
        tracks.add (makeTrackObject (track));
    root->setProperty ("tracks", juce::var (tracks));

    juce::Array<juce::var> mixerChannels;
    for (const auto& channel : snapshot.mixerChannels)
        mixerChannels.add (makeMixerChannelObject (channel));
    root->setProperty ("mixerChannels", juce::var (mixerChannels));

    juce::Array<juce::var> monitorPaths;
    for (const auto& path : snapshot.monitorPaths)
    {
        auto object = std::make_unique<juce::DynamicObject>();
        object->setProperty ("id", path.id);
        object->setProperty ("name", path.name);
        object->setProperty (
            "role",
            path.role == MonitorPathRole::cue ? "cue"
                : path.role == MonitorPathRole::listen ? "listen"
                : path.role == MonitorPathRole::talkback ? "talkback"
                                                         : "control-room");
        object->setProperty ("sourceId", path.sourceId);
        object->setProperty ("outputStartChannel", path.outputStartChannel);
        object->setProperty ("outputChannelCount", path.outputChannelCount);
        object->setProperty ("gain", path.gain);
        object->setProperty ("muted", path.muted);
        object->setProperty ("dimmed", path.dimmed);
        monitorPaths.add (juce::var (object.release()));
    }
    root->setProperty ("monitorPaths", juce::var (monitorPaths));

    juce::Array<juce::var> repairRecords;
    for (const auto& record : snapshot.repairRecords)
        repairRecords.add (makeRepairRecordObject (record));
    root->setProperty ("repairRecords", juce::var (repairRecords));
    root->setProperty ("undoManifest",
                       makeUndoManifestObject (snapshot.undoManifest));

    const auto json = juce::JSON::toString (juce::var (root.release()), true);

    if (destination.getParentDirectory().createDirectory().failed())
        return juce::Result::fail ("The project folder could not be created.");

    juce::TemporaryFile temporary (destination);

    if (! temporary.getFile().replaceWithText (json))
        return juce::Result::fail ("The temporary project file could not be written.");

    if (! temporary.overwriteTargetFileWithTemporary())
        return juce::Result::fail ("The project file could not be replaced safely.");

    return juce::Result::ok();
}

juce::Result ProjectDocument::load (const juce::File& source,
                                    ProjectState& destination)
{
    if (! source.existsAsFile())
        return juce::Result::fail ("The selected project file does not exist.");

    const auto parsed = juce::JSON::parse (source.loadFileAsString());

    if (! parsed.isObject())
        return juce::Result::fail ("This is not a valid Triumph Studio project.");

    const auto* root = parsed.getDynamicObject();

    if (root == nullptr || root->getProperty ("format").toString() != "TriumphStudioProject")
        return juce::Result::fail ("The selected file is not a Triumph Studio project.");

    const auto version = static_cast<int> (root->getProperty ("formatVersion"));

    if (version == 1)
    {
        const auto result = loadVersionOne (*root, destination);
        if (result.wasOk())
            migrateLegacyClipLengthsToTimelineRate (destination);
        return result;
    }

    if (version == 2)
    {
        const auto result = loadVersionTwo (*root, destination);
        if (result.wasOk())
            migrateLegacyClipLengthsToTimelineRate (destination);
        return result;
    }

    if (version == 3)
    {
        const auto result = loadVersionThree (*root, source.getParentDirectory(), destination);
        if (result.wasOk())
            migrateLegacyClipLengthsToTimelineRate (destination);
        return result;
    }

    if (version == 4)
        return loadVersionFour (*root, source.getParentDirectory(), destination);

    if (version == 5)
        return loadVersionFive (*root, source.getParentDirectory(), destination);

    if (version == 6)
        return loadVersionSix (*root, source.getParentDirectory(), destination);

    if (version == 7)
        return loadVersionSeven (*root, source.getParentDirectory(), destination);

    if (version == 8)
        return loadVersionEight (*root, source.getParentDirectory(), destination);

    if (version == 9)
        return loadVersionNine (*root, source.getParentDirectory(), destination);

    if (version == 10)
        return loadVersionTen (*root, source.getParentDirectory(), destination);

    if (version == 11)
        return loadVersionEleven (*root, source.getParentDirectory(), destination);
    if (version == 12)
        return loadVersionTwelve (*root, source.getParentDirectory(), destination);
    if (version == 13)
        return loadVersionThirteen (*root, source.getParentDirectory(), destination);
    if (version == 14)
        return loadVersionFourteen (*root, source.getParentDirectory(), destination);
    if (version == 15)
        return loadVersionFifteen (*root, source.getParentDirectory(), destination);
    if (version == 16)
        return loadVersionSixteen (*root, source.getParentDirectory(), destination);
    if (version == 17)
        return loadVersionSeventeen (*root, source.getParentDirectory(), destination);
    if (version == 18)
        return loadVersionEighteen (*root, source.getParentDirectory(), destination);
    if (version == 19)
        return loadVersionNineteen (*root, source.getParentDirectory(), destination);
    if (version == 20)
        return loadVersionTwenty (*root, source.getParentDirectory(), destination);
    if (version == 21)
        return loadVersionTwentyOne (*root, source.getParentDirectory(), destination);
    if (version == 22)
        return loadVersionTwentyTwo (*root, source.getParentDirectory(), destination);
    if (version == 23)
        return loadVersionTwentyThree (*root, source.getParentDirectory(), destination);
    if (version == currentFormatVersion)
        return loadVersionTwentyFour (*root, source.getParentDirectory(), destination);

    return juce::Result::fail ("This project uses an unsupported file format version.");
}

juce::Result ProjectDocument::loadVersionOne (const juce::DynamicObject& root,
                                               ProjectState& destination)
{
    ProjectState loaded;
    loaded.name = root.getProperty ("name").toString();
    loaded.masterGain = static_cast<float> (root.getProperty ("masterGain"));

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    const auto tracksValue = root.getProperty ("tracks");

    if (const auto* tracks = tracksValue.getArray())
    {
        loaded.mediaSources.reserve (static_cast<size_t> (tracks->size()));
        loaded.tracks.reserve (static_cast<size_t> (tracks->size()));

        for (const auto& trackValue : *tracks)
        {
            const auto* object = trackValue.getDynamicObject();
            if (object == nullptr)
                continue;

            MediaSourceState source;
            source.id = juce::Uuid().toString();
            source.file = juce::File (object->getProperty ("sourceFile").toString());

            auto reader = std::unique_ptr<juce::AudioFormatReader> (
                formats.createReaderFor (source.file));

            if (reader != nullptr)
            {
                source.sampleRate = reader->sampleRate;
                source.channels = static_cast<int> (reader->numChannels);
                source.lengthInSamples = reader->lengthInSamples;
            }
            source.assetFingerprint = makeAssetFingerprint (
                source.file, source.sampleRate, source.channels,
                source.lengthInSamples);

            TrackState track;
            track.id = juce::Uuid().toString();
            track.name = object->getProperty ("name").toString();
            track.gain = static_cast<float> (object->getProperty ("gain"));
            track.pan = static_cast<float> (object->getProperty ("pan"));
            track.muted = static_cast<bool> (object->getProperty ("muted"));
            track.solo = static_cast<bool> (object->getProperty ("solo"));
            track.recordArmed = static_cast<bool> (object->getProperty ("recordArmed"));

            AudioClipState clip;
            clip.id = juce::Uuid().toString();
            clip.sourceId = source.id;
            clip.lengthInSamples = source.lengthInSamples;
            track.clips.push_back (clip);

            loaded.mediaSources.push_back (std::move (source));
            loaded.tracks.push_back (std::move (track));
        }
    }

    destination = std::move (loaded);
    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionTwo (const juce::DynamicObject& root,
                                               ProjectState& destination)
{
    ProjectState loaded;
    loaded.name = root.getProperty ("name").toString();
    loaded.masterGain = static_cast<float> (root.getProperty ("masterGain"));
    loaded.timelineSampleRate = static_cast<double> (root.getProperty ("timelineSampleRate"));

    const auto sourcesValue = root.getProperty ("mediaSources");
    if (const auto* sources = sourcesValue.getArray())
    {
        loaded.mediaSources.reserve (static_cast<size_t> (sources->size()));

        for (const auto& sourceValue : *sources)
        {
            const auto* object = sourceValue.getDynamicObject();
            if (object == nullptr)
                continue;

            MediaSourceState source;
            source.id = object->getProperty ("id").toString();
            source.file = juce::File (object->getProperty ("path").toString());
            source.sampleRate = static_cast<double> (object->getProperty ("sampleRate"));
            source.channels = static_cast<int> (object->getProperty ("channels"));
            source.lengthInSamples = int64Property (*object, "lengthInSamples");
            source.assetFingerprint = object->getProperty (
                "assetFingerprint").toString();
            if (source.assetFingerprint.isEmpty())
                source.assetFingerprint = makeAssetFingerprint (
                    source.file, source.sampleRate, source.channels,
                    source.lengthInSamples);

            if (source.id.isEmpty())
                return juce::Result::fail ("A media source is missing its stable ID.");

            loaded.mediaSources.push_back (std::move (source));
        }
    }

    const auto tracksValue = root.getProperty ("tracks");
    if (const auto* tracks = tracksValue.getArray())
    {
        loaded.tracks.reserve (static_cast<size_t> (tracks->size()));

        for (const auto& trackValue : *tracks)
        {
            const auto* object = trackValue.getDynamicObject();
            if (object == nullptr)
                continue;

            TrackState track;
            track.id = object->getProperty ("id").toString();
            track.name = object->getProperty ("name").toString();
            track.colour = juce::Colour (static_cast<juce::uint32> (
                int64Property (*object, "colour", 0xff2869d8)));
            track.gain = static_cast<float> (object->getProperty ("gain"));
            track.pan = static_cast<float> (object->getProperty ("pan"));
            track.muted = static_cast<bool> (object->getProperty ("muted"));
            track.solo = static_cast<bool> (object->getProperty ("solo"));
            track.recordArmed = static_cast<bool> (object->getProperty ("recordArmed"));

            if (track.id.isEmpty())
                return juce::Result::fail ("A track is missing its stable ID.");

            const auto clipsValue = object->getProperty ("clips");
            if (const auto* clips = clipsValue.getArray())
            {
                track.clips.reserve (static_cast<size_t> (clips->size()));

                for (const auto& clipValue : *clips)
                {
                    const auto* clipObject = clipValue.getDynamicObject();
                    if (clipObject == nullptr)
                        continue;

                    AudioClipState clip;
                    clip.id = clipObject->getProperty ("id").toString();
                    clip.sourceId = clipObject->getProperty ("sourceId").toString();
                    clip.timelineStartSamples = int64Property (*clipObject,
                                                               "timelineStartSamples");
                    clip.sourceOffsetSamples = int64Property (*clipObject,
                                                              "sourceOffsetSamples");
                    clip.lengthInSamples = int64Property (*clipObject, "lengthInSamples");
                    clip.gain = static_cast<float> (clipObject->getProperty ("gain"));

                    if (clip.id.isEmpty() || clip.sourceId.isEmpty())
                        return juce::Result::fail ("An audio clip has invalid identity data.");

                    track.clips.push_back (std::move (clip));
                }
            }

            loaded.tracks.push_back (std::move (track));
        }
    }

    destination = std::move (loaded);
    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionThree (const juce::DynamicObject& root,
                                                 const juce::File& projectDirectory,
                                                 ProjectState& destination)
{
    ProjectState loaded;
    loaded.projectId = root.getProperty ("projectId").toString();
    loaded.name = root.getProperty ("name").toString();
    loaded.masterGain = static_cast<float> (root.getProperty ("masterGain"));
    loaded.timelineSampleRate = static_cast<double> (root.getProperty ("timelineSampleRate"));

    if (loaded.projectId.isEmpty())
        return juce::Result::fail ("This project is missing its persistent project ID.");

    const auto sourcesValue = root.getProperty ("mediaSources");
    if (const auto* sources = sourcesValue.getArray())
    {
        loaded.mediaSources.reserve (static_cast<size_t> (sources->size()));

        for (const auto& sourceValue : *sources)
        {
            const auto* object = sourceValue.getDynamicObject();
            if (object == nullptr)
                continue;

            MediaSourceState source;
            source.id = object->getProperty ("id").toString();
            const auto storedPath = object->getProperty ("path").toString();
            const auto pathIsRelative = static_cast<bool> (
                object->getProperty ("pathIsRelative"));
            source.file = pathIsRelative ? projectDirectory.getChildFile (storedPath)
                                         : juce::File (storedPath);
            source.sampleRate = static_cast<double> (object->getProperty ("sampleRate"));
            source.channels = static_cast<int> (object->getProperty ("channels"));
            source.lengthInSamples = int64Property (*object, "lengthInSamples");
            source.assetFingerprint = object->getProperty (
                "assetFingerprint").toString();
            if (source.assetFingerprint.isEmpty())
                source.assetFingerprint = makeAssetFingerprint (
                    source.file, source.sampleRate, source.channels,
                    source.lengthInSamples);

            if (source.id.isEmpty())
                return juce::Result::fail ("A media source is missing its stable ID.");

            if (pathIsRelative
                && source.file.getParentDirectory() != projectDirectory
                && ! source.file.isAChildOf (projectDirectory))
                return juce::Result::fail ("A relative media path escapes the project folder.");

            loaded.mediaSources.push_back (std::move (source));
        }
    }

    const auto tracksValue = root.getProperty ("tracks");
    if (const auto* tracks = tracksValue.getArray())
    {
        loaded.tracks.reserve (static_cast<size_t> (tracks->size()));

        for (const auto& trackValue : *tracks)
        {
            const auto* object = trackValue.getDynamicObject();
            if (object == nullptr)
                continue;

            TrackState track;
            track.id = object->getProperty ("id").toString();
            track.name = object->getProperty ("name").toString();
            track.colour = juce::Colour (static_cast<juce::uint32> (
                int64Property (*object, "colour", 0xff2869d8)));
            track.gain = static_cast<float> (object->getProperty ("gain"));
            track.pan = static_cast<float> (object->getProperty ("pan"));
            track.muted = static_cast<bool> (object->getProperty ("muted"));
            track.solo = static_cast<bool> (object->getProperty ("solo"));

            if (! object->getProperty ("recordArmed").isVoid())
                track.recordArmed = static_cast<bool> (object->getProperty ("recordArmed"));

            if (track.id.isEmpty())
                return juce::Result::fail ("A track is missing its stable ID.");

            const auto clipsValue = object->getProperty ("clips");
            if (const auto* clips = clipsValue.getArray())
            {
                track.clips.reserve (static_cast<size_t> (clips->size()));

                for (const auto& clipValue : *clips)
                {
                    const auto* clipObject = clipValue.getDynamicObject();
                    if (clipObject == nullptr)
                        continue;

                    AudioClipState clip;
                    clip.id = clipObject->getProperty ("id").toString();
                    clip.sourceId = clipObject->getProperty ("sourceId").toString();
                    clip.timelineStartSamples = int64Property (*clipObject,
                                                               "timelineStartSamples");
                    clip.sourceOffsetSamples = int64Property (*clipObject,
                                                              "sourceOffsetSamples");
                    clip.lengthInSamples = int64Property (*clipObject, "lengthInSamples");
                    clip.gain = static_cast<float> (clipObject->getProperty ("gain"));

                    if (clip.id.isEmpty() || clip.sourceId.isEmpty())
                        return juce::Result::fail ("An audio clip has invalid identity data.");

                    track.clips.push_back (std::move (clip));
                }
            }

            loaded.tracks.push_back (std::move (track));
        }
    }

    destination = std::move (loaded);
    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionFour (const juce::DynamicObject& root,
                                                const juce::File& projectDirectory,
                                                ProjectState& destination)
{
    return loadVersionThree (root, projectDirectory, destination);
}

juce::Result ProjectDocument::loadVersionFive (const juce::DynamicObject& root,
                                                const juce::File& projectDirectory,
                                                ProjectState& destination)
{
    return loadVersionFour (root, projectDirectory, destination);
}

juce::Result ProjectDocument::loadVersionSix (const juce::DynamicObject& root,
                                               const juce::File& projectDirectory,
                                               ProjectState& destination)
{
    const auto baseResult = loadVersionFive (root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;

    destination.timeSignatureNumerator = juce::jlimit (
        1, 32, static_cast<int> (root.getProperty ("timeSignatureNumerator")));
    destination.timeSignatureDenominator = juce::jlimit (
        1, 32, static_cast<int> (root.getProperty ("timeSignatureDenominator")));
    destination.timeSignatures = { { juce::Uuid().toString(), 0.0,
                                     destination.timeSignatureNumerator,
                                     destination.timeSignatureDenominator } };
    destination.tempoPoints.clear();

    const auto tempoPointsValue = root.getProperty ("tempoPoints");
    if (const auto* points = tempoPointsValue.getArray())
    {
        for (const auto& pointValue : *points)
        {
            const auto* pointObject = pointValue.getDynamicObject();
            if (pointObject == nullptr)
                continue;
            destination.tempoPoints.push_back ({
                pointObject->getProperty ("id").toString(),
                static_cast<double> (pointObject->getProperty ("beat")),
                static_cast<double> (pointObject->getProperty ("bpm")) });
        }
    }

    if (destination.tempoPoints.empty())
        destination.tempoPoints.push_back ({ juce::Uuid().toString(), 0.0, 120.0 });

    const auto tracksValue = root.getProperty ("tracks");
    if (const auto* tracks = tracksValue.getArray())
    {
        for (const auto& trackValue : *tracks)
        {
            const auto* trackObject = trackValue.getDynamicObject();
            if (trackObject == nullptr)
                continue;
            const auto trackId = trackObject->getProperty ("id").toString();
            const auto match = std::find_if (destination.tracks.begin(),
                                             destination.tracks.end(),
                                             [&trackId] (const auto& track)
            {
                return track.id == trackId;
            });
            if (match == destination.tracks.end())
                continue;

            match->isInstrument = static_cast<bool> (
                trackObject->getProperty ("isInstrument"));
            const auto midiClipsValue = trackObject->getProperty ("midiClips");
            if (const auto* clips = midiClipsValue.getArray())
            {
                for (const auto& clipValue : *clips)
                {
                    const auto* clipObject = clipValue.getDynamicObject();
                    if (clipObject == nullptr)
                        continue;
                    MidiClipState clip;
                    clip.id = clipObject->getProperty ("id").toString();
                    clip.startBeat = static_cast<double> (clipObject->getProperty ("startBeat"));
                    clip.lengthBeats = static_cast<double> (
                        clipObject->getProperty ("lengthBeats"));
                    const auto notesValue = clipObject->getProperty ("notes");
                    if (const auto* notes = notesValue.getArray())
                    {
                        for (const auto& noteValue : *notes)
                        {
                            const auto* noteObject = noteValue.getDynamicObject();
                            if (noteObject == nullptr)
                                continue;
                            clip.notes.push_back ({
                                noteObject->getProperty ("id").toString(),
                                static_cast<double> (noteObject->getProperty ("startBeat")),
                                static_cast<double> (noteObject->getProperty ("lengthBeats")),
                                static_cast<int> (noteObject->getProperty ("noteNumber")),
                                static_cast<float> (noteObject->getProperty ("velocity")),
                                static_cast<int> (noteObject->getProperty ("channel")) });
                        }
                    }
                    match->midiClips.push_back (std::move (clip));
                }
            }
        }
    }

    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionSeven (const juce::DynamicObject& root,
                                                 const juce::File& projectDirectory,
                                                 ProjectState& destination)
{
    const auto baseResult = loadVersionSix (root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;

    const auto tracksValue = root.getProperty ("tracks");
    if (const auto* tracks = tracksValue.getArray())
        for (const auto& trackValue : *tracks)
        {
            const auto* object = trackValue.getDynamicObject();
            if (object == nullptr)
                continue;
            const auto id = object->getProperty ("id").toString();
            const auto match = std::find_if (destination.tracks.begin(),
                                             destination.tracks.end(),
                                             [&id] (const auto& track)
            {
                return track.id == id;
            });
            if (match == destination.tracks.end() || ! match->isInstrument)
                continue;
            match->pluginName = object->getProperty ("pluginName").toString();
            match->pluginDescriptionXml = object->getProperty (
                "pluginDescriptionXml").toString();
            match->pluginStateBase64 = object->getProperty (
                "pluginStateBase64").toString();
        }

    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionEight (const juce::DynamicObject& root,
                                                 const juce::File& projectDirectory,
                                                 ProjectState& destination)
{
    const auto baseResult = loadVersionSeven (root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;

    const auto tracksValue = root.getProperty ("tracks");
    if (const auto* tracks = tracksValue.getArray())
        for (const auto& trackValue : *tracks)
        {
            const auto* object = trackValue.getDynamicObject();
            if (object == nullptr)
                continue;
            const auto id = object->getProperty ("id").toString();
            const auto track = std::find_if (destination.tracks.begin(),
                                             destination.tracks.end(),
                                             [&id] (const auto& candidate)
            {
                return candidate.id == id;
            });
            if (track == destination.tracks.end())
                continue;

            const auto clipsValue = object->getProperty ("clips");
            if (const auto* clips = clipsValue.getArray())
                for (const auto& clipValue : *clips)
                {
                    const auto* clipObject = clipValue.getDynamicObject();
                    if (clipObject == nullptr)
                        continue;
                    const auto clipId = clipObject->getProperty ("id").toString();
                    const auto clip = std::find_if (track->clips.begin(),
                                                    track->clips.end(),
                                                    [&clipId] (const auto& candidate)
                    {
                        return candidate.id == clipId;
                    });
                    if (clip == track->clips.end())
                        continue;
                    clip->takeGroupId = clipObject->getProperty ("takeGroupId").toString();
                    clip->takeLaneIndex = juce::jmax (
                        0, static_cast<int> (clipObject->getProperty ("takeLaneIndex")));
                    clip->activeTake = clipObject->getProperty ("activeTake").isVoid()
                                           ? true
                                           : static_cast<bool> (
                                                 clipObject->getProperty ("activeTake"));
                }

            juce::StringArray groups;
            for (const auto& clip : track->clips)
                if (clip.takeGroupId.isNotEmpty())
                    groups.addIfNotAlreadyThere (clip.takeGroupId);
            for (const auto& groupId : groups)
            {
                auto selectedLane = -1;
                auto lowestLane = std::numeric_limits<int>::max();
                for (const auto& clip : track->clips)
                    if (clip.takeGroupId == groupId)
                    {
                        lowestLane = juce::jmin (lowestLane, clip.takeLaneIndex);
                        if (selectedLane < 0 && clip.activeTake)
                            selectedLane = clip.takeLaneIndex;
                    }
                if (selectedLane < 0)
                    selectedLane = lowestLane;
                for (auto& clip : track->clips)
                    if (clip.takeGroupId == groupId)
                        clip.activeTake = clip.takeLaneIndex == selectedLane;
            }
        }

    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionNine (const juce::DynamicObject& root,
                                                const juce::File& projectDirectory,
                                                ProjectState& destination)
{
    const auto baseResult = loadVersionEight (root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;

    const auto tracksValue = root.getProperty ("tracks");
    if (const auto* tracks = tracksValue.getArray())
        for (const auto& trackValue : *tracks)
        {
            const auto* object = trackValue.getDynamicObject();
            if (object == nullptr)
                continue;
            const auto id = object->getProperty ("id").toString();
            const auto track = std::find_if (destination.tracks.begin(),
                                             destination.tracks.end(),
                                             [&id] (const auto& candidate)
            {
                return candidate.id == id;
            });
            if (track == destination.tracks.end())
                continue;

            const auto regionsValue = object->getProperty ("compRegions");
            if (const auto* regions = regionsValue.getArray())
                for (const auto& regionValue : *regions)
                {
                    const auto* regionObject = regionValue.getDynamicObject();
                    if (regionObject == nullptr)
                        continue;
                    CompRegionState region;
                    region.id = regionObject->getProperty ("id").toString();
                    region.takeGroupId = regionObject->getProperty (
                        "takeGroupId").toString();
                    region.takeLaneIndex = juce::jmax (
                        0, static_cast<int> (regionObject->getProperty (
                            "takeLaneIndex")));
                    region.timelineStartSamples = juce::jmax (
                        static_cast<juce::int64> (0),
                        int64Property (*regionObject, "timelineStartSamples"));
                    region.lengthInSamples = juce::jmax (
                        static_cast<juce::int64> (0),
                        int64Property (*regionObject, "lengthInSamples"));
                    region.crossfadeSamples = juce::jmax (
                        static_cast<juce::int64> (0),
                        int64Property (*regionObject, "crossfadeSamples", 480));
                    if (region.id.isEmpty())
                        region.id = juce::Uuid().toString();
                    if (region.takeGroupId.isNotEmpty() && region.lengthInSamples > 0)
                        track->compRegions.push_back (std::move (region));
                }
        }

    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionTen (const juce::DynamicObject& root,
                                               const juce::File& projectDirectory,
                                               ProjectState& destination)
{
    const auto baseResult = loadVersionNine (root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;

    const auto tracksValue = root.getProperty ("tracks");
    if (const auto* tracks = tracksValue.getArray())
        for (const auto& trackValue : *tracks)
        {
            const auto* trackObject = trackValue.getDynamicObject();
            if (trackObject == nullptr)
                continue;
            const auto trackId = trackObject->getProperty ("id").toString();
            auto track = std::find_if (destination.tracks.begin(),
                                       destination.tracks.end(),
                                       [&trackId] (const auto& candidate)
            {
                return candidate.id == trackId;
            });
            if (track == destination.tracks.end())
                continue;

            const auto clipsValue = trackObject->getProperty ("clips");
            if (const auto* clips = clipsValue.getArray())
                for (const auto& clipValue : *clips)
                {
                    const auto* clipObject = clipValue.getDynamicObject();
                    if (clipObject == nullptr)
                        continue;
                    const auto clipId = clipObject->getProperty ("id").toString();
                    auto clip = std::find_if (track->clips.begin(), track->clips.end(),
                                              [&clipId] (const auto& candidate)
                    {
                        return candidate.id == clipId;
                    });
                    if (clip == track->clips.end())
                        continue;
                    const auto ratioValue = clipObject->getProperty ("timeStretchRatio");
                    clip->timeStretchRatio = stretch::clampRatio (
                        ratioValue.isVoid() ? 1.0 : static_cast<double> (ratioValue));
                    clip->timeStretchProvider = clipObject->getProperty (
                        "timeStretchProvider").toString();
                    if (clip->timeStretchProvider.isEmpty())
                        clip->timeStretchProvider = stretch::builtInProviderId;
                }
        }
    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionEleven (const juce::DynamicObject& root,
                                                  const juce::File& projectDirectory,
                                                  ProjectState& destination)
{
    const auto baseResult = loadVersionTen (root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;
    const auto tracksValue = root.getProperty ("tracks");
    if (const auto* tracks = tracksValue.getArray())
        for (const auto& trackValue : *tracks)
        {
            const auto* trackObject = trackValue.getDynamicObject();
            if (trackObject == nullptr)
                continue;
            const auto trackId = trackObject->getProperty ("id").toString();
            auto track = std::find_if (destination.tracks.begin(), destination.tracks.end(),
                                       [&trackId] (const auto& candidate)
            {
                return candidate.id == trackId;
            });
            if (track == destination.tracks.end())
                continue;
            const auto clipsValue = trackObject->getProperty ("clips");
            if (const auto* clips = clipsValue.getArray())
                for (const auto& clipValue : *clips)
                {
                    const auto* clipObject = clipValue.getDynamicObject();
                    if (clipObject == nullptr)
                        continue;
                    const auto clipId = clipObject->getProperty ("id").toString();
                    auto clip = std::find_if (track->clips.begin(), track->clips.end(),
                                              [&clipId] (const auto& candidate)
                    {
                        return candidate.id == clipId;
                    });
                    if (clip == track->clips.end())
                        continue;
                    const auto markersValue = clipObject->getProperty ("warpMarkers");
                    if (const auto* markers = markersValue.getArray())
                        for (const auto& markerValue : *markers)
                        {
                            const auto* markerObject = markerValue.getDynamicObject();
                            if (markerObject == nullptr)
                                continue;
                            WarpMarkerState marker;
                            marker.id = markerObject->getProperty ("id").toString();
                            marker.timelineOffsetSamples = juce::jmax (
                                static_cast<juce::int64> (1),
                                int64Property (*markerObject, "timelineOffsetSamples"));
                            marker.sourceOffsetSamples = juce::jmax (
                                static_cast<juce::int64> (0),
                                int64Property (*markerObject, "sourceOffsetSamples"));
                            if (marker.id.isEmpty())
                                marker.id = juce::Uuid().toString();
                            if (marker.timelineOffsetSamples < clip->lengthInSamples)
                                clip->warpMarkers.push_back (std::move (marker));
                        }
                }
        }
    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionTwelve (const juce::DynamicObject& root,
                                                  const juce::File& projectDirectory,
                                                  ProjectState& destination)
{
    const auto baseResult = loadVersionEleven (root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;
    if (const auto* tracks = root.getProperty ("tracks").getArray())
        for (const auto& trackValue : *tracks)
        {
            const auto* trackObject = trackValue.getDynamicObject();
            if (trackObject == nullptr)
                continue;
            const auto trackId = trackObject->getProperty ("id").toString();
            auto track = std::find_if (destination.tracks.begin(), destination.tracks.end(),
                                       [&trackId] (const auto& candidate)
            {
                return candidate.id == trackId;
            });
            if (track == destination.tracks.end())
                continue;
            if (const auto* clips = trackObject->getProperty ("clips").getArray())
                for (const auto& clipValue : *clips)
                {
                    const auto* clipObject = clipValue.getDynamicObject();
                    if (clipObject == nullptr)
                        continue;
                    const auto clipId = clipObject->getProperty ("id").toString();
                    auto clip = std::find_if (track->clips.begin(), track->clips.end(),
                                              [&clipId] (const auto& candidate)
                    {
                        return candidate.id == clipId;
                    });
                    if (clip == track->clips.end())
                        continue;
                    if (const auto* markers = clipObject->getProperty (
                            "transientMarkers").getArray())
                        for (const auto& markerValue : *markers)
                        {
                            const auto* markerObject = markerValue.getDynamicObject();
                            if (markerObject == nullptr)
                                continue;
                            TransientMarkerState marker;
                            marker.id = markerObject->getProperty ("id").toString();
                            marker.timelineOffsetSamples = int64Property (
                                *markerObject, "timelineOffsetSamples");
                            marker.sourceOffsetSamples = int64Property (
                                *markerObject, "sourceOffsetSamples");
                            marker.strength = juce::jlimit (0.0f, 1.0f,
                                static_cast<float> (markerObject->getProperty (
                                    "strength")));
                            if (marker.id.isEmpty())
                                marker.id = juce::Uuid().toString();
                            if (marker.timelineOffsetSamples > 0
                                && marker.timelineOffsetSamples < clip->lengthInSamples)
                                clip->transientMarkers.push_back (std::move (marker));
                        }
                }
        }
    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionThirteen (
    const juce::DynamicObject& root,
    const juce::File& projectDirectory,
    ProjectState& destination)
{
    const auto baseResult = loadVersionTwelve (root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;

    if (const auto* tracks = root.getProperty ("tracks").getArray())
        for (const auto& trackValue : *tracks)
        {
            const auto* object = trackValue.getDynamicObject();
            if (object == nullptr)
                continue;
            const auto id = object->getProperty ("id").toString();
            auto track = std::find_if (destination.tracks.begin(),
                                       destination.tracks.end(),
                                       [&id] (const auto& candidate)
            {
                return candidate.id == id;
            });
            if (track == destination.tracks.end() || ! track->isInstrument)
                continue;

            track->pluginBypassed = static_cast<bool> (
                object->getProperty ("pluginBypassed"));
            track->pluginLatencySamples = juce::jmax (0, static_cast<int> (
                object->getProperty ("pluginLatencySamples")));
            const auto available = object->getProperty ("pluginAvailable");
            track->pluginAvailable = available.isVoid()
                                         ? true
                                         : static_cast<bool> (available);
            track->frozen = static_cast<bool> (object->getProperty ("frozen"));
            track->freezeSourceId = object->getProperty (
                "freezeSourceId").toString();

            if (track->frozen)
            {
                const auto source = std::find_if (
                    destination.mediaSources.begin(),
                    destination.mediaSources.end(),
                    [&track] (const auto& candidate)
                {
                    return candidate.id == track->freezeSourceId;
                });
                const auto clip = std::find_if (
                    track->clips.begin(), track->clips.end(),
                    [&track] (const auto& candidate)
                {
                    return candidate.sourceId == track->freezeSourceId;
                });
                if (track->freezeSourceId.isEmpty()
                    || source == destination.mediaSources.end()
                    || ! source->file.existsAsFile()
                    || clip == track->clips.end())
                {
                    const auto invalidFreezeSourceId = track->freezeSourceId;
                    if (invalidFreezeSourceId.isNotEmpty())
                    {
                        track->clips.erase (std::remove_if (
                            track->clips.begin(), track->clips.end(),
                            [&invalidFreezeSourceId] (const auto& candidate)
                        {
                            return candidate.sourceId == invalidFreezeSourceId;
                        }), track->clips.end());
                        destination.mediaSources.erase (std::remove_if (
                            destination.mediaSources.begin(),
                            destination.mediaSources.end(),
                            [&invalidFreezeSourceId] (const auto& candidate)
                        {
                            return candidate.id == invalidFreezeSourceId;
                        }), destination.mediaSources.end());
                    }
                    track->frozen = false;
                    track->freezeSourceId.clear();
                    addRepairRecord (
                        destination,
                        "freeze",
                        track->id,
                        "Removed a missing frozen-instrument render and restored the instrument track.");
                }
            }
        }

    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionFourteen (
    const juce::DynamicObject& root,
    const juce::File& projectDirectory,
    ProjectState& destination)
{
    const auto baseResult = loadVersionThirteen (
        root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;

    const auto lowLatency = root.getProperty ("lowLatencyMonitoring");
    destination.lowLatencyMonitoring = ! lowLatency.isVoid()
        && static_cast<bool> (lowLatency);

    if (const auto* tracks = root.getProperty ("tracks").getArray())
        for (const auto& trackValue : *tracks)
        {
            const auto* object = trackValue.getDynamicObject();
            if (object == nullptr)
                continue;
            const auto id = object->getProperty ("id").toString();
            auto track = std::find_if (destination.tracks.begin(),
                                       destination.tracks.end(),
                                       [&id] (const auto& candidate)
            {
                return candidate.id == id;
            });
            if (track == destination.tracks.end())
                continue;
            track->manualLatencyOffsetSamples = juce::jlimit (
                -262143, 262143, static_cast<int> (
                    object->getProperty ("manualLatencyOffsetSamples")));
        }

    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionFifteen (
    const juce::DynamicObject& root,
    const juce::File& projectDirectory,
    ProjectState& destination)
{
    const auto baseResult = loadVersionFourteen (
        root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;

    destination.masterMuted = static_cast<bool> (
        root.getProperty ("masterMuted"));

    if (const auto* tracks = root.getProperty ("tracks").getArray())
        for (const auto& trackValue : *tracks)
        {
            const auto* object = trackValue.getDynamicObject();
            if (object == nullptr)
                continue;
            const auto id = object->getProperty ("id").toString();
            auto track = std::find_if (destination.tracks.begin(),
                                       destination.tracks.end(),
                                       [&id] (const auto& candidate)
            {
                return candidate.id == id;
            });
            if (track == destination.tracks.end())
                continue;
            track->outputDestinationId = object->getProperty (
                "outputDestinationId").toString();
            if (track->outputDestinationId.isEmpty())
                track->outputDestinationId = "master";
            track->sends = mixerSendsFromObject (*object);
        }

    if (const auto* channels = root.getProperty ("mixerChannels").getArray())
        for (const auto& channelValue : *channels)
        {
            const auto* object = channelValue.getDynamicObject();
            if (object == nullptr)
                continue;
            MixerChannelState channel;
            channel.id = object->getProperty ("id").toString();
            if (channel.id.isEmpty())
                channel.id = juce::Uuid().toString();
            channel.name = object->getProperty ("name").toString();
            channel.kind = object->getProperty ("kind").toString() == "return"
                               ? MixerChannelKind::returnChannel
                               : MixerChannelKind::bus;
            channel.gain = juce::jlimit (0.0f, 1.5f, static_cast<float> (
                object->getProperty ("gain")));
            channel.pan = juce::jlimit (-1.0f, 1.0f, static_cast<float> (
                object->getProperty ("pan")));
            channel.muted = static_cast<bool> (object->getProperty ("muted"));
            channel.solo = static_cast<bool> (object->getProperty ("solo"));
            channel.outputDestinationId = object->getProperty (
                "outputDestinationId").toString();
            if (channel.outputDestinationId.isEmpty())
                channel.outputDestinationId = "master";
            channel.sends = mixerSendsFromObject (*object);
            destination.mixerChannels.push_back (std::move (channel));
        }

    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionSixteen (
    const juce::DynamicObject& root,
    const juce::File& projectDirectory,
    ProjectState& destination)
{
    const auto baseResult = loadVersionFifteen (
        root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;

    const auto metronomeEnabled = root.getProperty ("metronomeEnabled");
    destination.metronomeEnabled = ! metronomeEnabled.isVoid()
        && static_cast<bool> (metronomeEnabled);
    destination.metronomeGain = juce::jlimit (0.0f, 1.0f, static_cast<float> (
        root.getProperty ("metronomeGain")));

    if (const auto* values = root.getProperty ("tempoPoints").getArray())
        for (const auto& value : *values)
        {
            const auto* object = value.getDynamicObject();
            if (object == nullptr)
                continue;
            const auto id = object->getProperty ("id").toString();
            auto point = std::find_if (destination.tempoPoints.begin(),
                                      destination.tempoPoints.end(),
                                      [&id] (const auto& candidate)
            {
                return candidate.id == id;
            });
            if (point != destination.tempoPoints.end())
                point->curve = object->getProperty ("curve").toString() == "linear"
                                   ? TempoCurve::linear : TempoCurve::step;
        }

    destination.timeSignatures.clear();
    if (const auto* values = root.getProperty ("timeSignatures").getArray())
        for (const auto& value : *values)
        {
            const auto* object = value.getDynamicObject();
            if (object == nullptr)
                continue;
            TimeSignaturePointState signature;
            signature.id = object->getProperty ("id").toString();
            if (signature.id.isEmpty()) signature.id = juce::Uuid().toString();
            signature.beat = juce::jmax (0.0, static_cast<double> (
                object->getProperty ("beat")));
            signature.numerator = juce::jlimit (1, 32, static_cast<int> (
                object->getProperty ("numerator")));
            signature.denominator = juce::jlimit (1, 32, static_cast<int> (
                object->getProperty ("denominator")));
            destination.timeSignatures.push_back (std::move (signature));
        }
    if (destination.timeSignatures.empty())
        destination.timeSignatures.push_back ({ juce::Uuid().toString(), 0.0,
            destination.timeSignatureNumerator,
            destination.timeSignatureDenominator });

    destination.markers.clear();
    if (const auto* values = root.getProperty ("markers").getArray())
        for (const auto& value : *values)
        {
            const auto* object = value.getDynamicObject();
            if (object == nullptr)
                continue;
            ProjectMarkerState marker;
            marker.id = object->getProperty ("id").toString();
            if (marker.id.isEmpty()) marker.id = juce::Uuid().toString();
            marker.beat = juce::jmax (0.0, static_cast<double> (
                object->getProperty ("beat")));
            marker.name = object->getProperty ("name").toString();
            if (marker.name.isEmpty()) marker.name = "Marker";
            marker.colour = juce::Colour (static_cast<juce::uint32> (
                static_cast<juce::int64> (object->getProperty ("colour"))));
            destination.markers.push_back (std::move (marker));
        }

    destination.automationLanes.clear();
    if (const auto* values = root.getProperty ("automationLanes").getArray())
        for (const auto& value : *values)
        {
            const auto* object = value.getDynamicObject();
            if (object == nullptr)
                continue;
            AutomationLaneState lane;
            lane.id = object->getProperty ("id").toString();
            if (lane.id.isEmpty()) lane.id = juce::Uuid().toString();
            lane.targetId = object->getProperty ("targetId").toString();
            if (lane.targetId.isEmpty()) lane.targetId = "master";
            lane.parameterId = object->getProperty ("parameterId").toString();
            if (lane.parameterId != "gain" && lane.parameterId != "pan"
                && ! lane.parameterId.startsWith ("plugin:"))
                lane.parameterId = "gain";
            const auto mode = object->getProperty ("mode").toString();
            lane.mode = mode == "touch" ? AutomationMode::touch
                : mode == "latch" ? AutomationMode::latch
                : mode == "write" ? AutomationMode::write
                                    : AutomationMode::read;
            const auto enabled = object->getProperty ("enabled");
            lane.enabled = enabled.isVoid() || static_cast<bool> (enabled);
            if (const auto* points = object->getProperty ("points").getArray())
                for (const auto& pointValue : *points)
                {
                    const auto* pointObject = pointValue.getDynamicObject();
                    if (pointObject == nullptr)
                        continue;
                    AutomationPointState point;
                    point.id = pointObject->getProperty ("id").toString();
                    if (point.id.isEmpty()) point.id = juce::Uuid().toString();
                    point.samplePosition = juce::jmax (
                        static_cast<juce::int64> (0),
                        int64Property (*pointObject,
                                       juce::Identifier ("samplePosition")));
                    point.value = juce::jlimit (-1.0f, 1.5f, static_cast<float> (
                        pointObject->getProperty ("value")));
                    const auto curve = pointObject->getProperty ("curve").toString();
                    point.curve = curve == "hold" ? AutomationCurve::hold
                        : curve == "smooth" ? AutomationCurve::smooth
                                             : AutomationCurve::linear;
                    lane.points.push_back (std::move (point));
                }
            destination.automationLanes.push_back (std::move (lane));
        }

    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionSeventeen (
    const juce::DynamicObject& root,
    const juce::File& projectDirectory,
    ProjectState& destination)
{
    const auto baseResult = loadVersionSixteen (
        root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;

    if (const auto* tracks = root.getProperty ("tracks").getArray())
        for (const auto& trackValue : *tracks)
        {
            const auto* trackObject = trackValue.getDynamicObject();
            if (trackObject == nullptr)
                continue;
            const auto trackId = trackObject->getProperty ("id").toString();
            auto track = std::find_if (destination.tracks.begin(),
                                       destination.tracks.end(),
                                       [&trackId] (const auto& candidate)
            {
                return candidate.id == trackId;
            });
            if (track == destination.tracks.end())
                continue;
            if (const auto* clips = trackObject->getProperty ("clips").getArray())
                for (const auto& clipValue : *clips)
                {
                    const auto* clipObject = clipValue.getDynamicObject();
                    if (clipObject == nullptr)
                        continue;
                    const auto clipId = clipObject->getProperty ("id").toString();
                    auto clip = std::find_if (track->clips.begin(), track->clips.end(),
                                              [&clipId] (const auto& candidate)
                    {
                        return candidate.id == clipId;
                    });
                    if (clip != track->clips.end())
                        clip->reversed = static_cast<bool> (
                            clipObject->getProperty ("reversed"));
                }
        }
    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionEighteen (
    const juce::DynamicObject& root,
    const juce::File& projectDirectory,
    ProjectState& destination)
{
    const auto baseResult = loadVersionSeventeen (
        root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;

    destination.producerSettings.rootPitchClass = juce::jlimit (
        0, 11, root.hasProperty ("producerRootPitchClass")
            ? static_cast<int> (root.getProperty ("producerRootPitchClass")) : 0);
    destination.producerSettings.scale = root.getProperty (
        "producerScale").toString() == "minor" ? "minor" : "major";
    const auto style = root.getProperty ("producerStyle").toString();
    destination.producerSettings.style = style == "chill" || style == "energetic"
        ? style : juce::String ("balanced");
    destination.producerSettings.bars = juce::jlimit (
        1, 16, root.hasProperty ("producerBars")
            ? static_cast<int> (root.getProperty ("producerBars")) : 4);
    const auto variation = root.hasProperty ("producerVariation")
        ? static_cast<juce::int64> (root.getProperty ("producerVariation"))
        : static_cast<juce::int64> (1);
    destination.producerSettings.variation = static_cast<std::uint32_t> (
        juce::jlimit (static_cast<juce::int64> (1),
                      static_cast<juce::int64> (
                          std::numeric_limits<std::uint32_t>::max()),
                      variation));
    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionNineteen (
    const juce::DynamicObject& root,
    const juce::File& projectDirectory,
    ProjectState& destination)
{
    const auto baseResult = loadVersionEighteen (
        root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;

    const auto monitorMode = root.getProperty ("inputMonitorMode").toString();
    destination.inputMonitorMode = monitorMode == "always"
        ? InputMonitorMode::always
        : monitorMode == "armed" ? InputMonitorMode::whileArmed
                                  : InputMonitorMode::off;
    destination.manualRecordOffsetSamples = juce::jlimit (
        -8192, 8192,
        root.hasProperty ("manualRecordOffsetSamples")
            ? static_cast<int> (root.getProperty ("manualRecordOffsetSamples"))
            : 0);
    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionTwenty (
    const juce::DynamicObject& root,
    const juce::File& projectDirectory,
    ProjectState& destination)
{
    const auto baseResult = loadVersionNineteen (
        root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;

    const auto mode = root.getProperty ("recordingMode").toString();
    destination.recordingSettings.mode = mode == "punch"
        ? RecordingMode::punch
        : mode == "loop" ? RecordingMode::loop : RecordingMode::normal;
    destination.recordingSettings.preRollBars = juce::jlimit (
        0, 8, root.hasProperty ("preRollBars")
            ? static_cast<int> (root.getProperty ("preRollBars")) : 0);
    destination.recordingSettings.punchStartSamples = juce::jmax (
        static_cast<juce::int64> (0), static_cast<juce::int64> (
            root.getProperty ("punchStartSamples")));
    destination.recordingSettings.punchEndSamples = juce::jmax (
        destination.recordingSettings.punchStartSamples,
        static_cast<juce::int64> (root.getProperty ("punchEndSamples")));
    destination.recordingSettings.loopStartSamples = juce::jmax (
        static_cast<juce::int64> (0), static_cast<juce::int64> (
            root.getProperty ("loopStartSamples")));
    destination.recordingSettings.loopEndSamples = juce::jmax (
        destination.recordingSettings.loopStartSamples,
        static_cast<juce::int64> (root.getProperty ("loopEndSamples")));
    destination.recordingSettings.controlRoomGain = juce::jlimit (
        0.0f, 1.5f, root.hasProperty ("controlRoomGain")
            ? static_cast<float> (root.getProperty ("controlRoomGain"))
            : 1.0f);
    destination.recordingSettings.controlRoomDimmed = static_cast<bool> (
        root.getProperty ("controlRoomDimmed"));
    destination.recordingSettings.controlRoomMuted = static_cast<bool> (
        root.getProperty ("controlRoomMuted"));

    if (const auto* tracks = root.getProperty ("tracks").getArray())
        for (const auto& trackValue : *tracks)
        {
            const auto* object = trackValue.getDynamicObject();
            if (object == nullptr)
                continue;
            const auto id = object->getProperty ("id").toString();
            auto track = std::find_if (
                destination.tracks.begin(), destination.tracks.end(),
                [&id] (const auto& candidate) { return candidate.id == id; });
            if (track == destination.tracks.end())
                continue;
            track->inputStartChannel = juce::jlimit (
                0, 63, static_cast<int> (
                    object->getProperty ("inputStartChannel")));
            track->inputChannelCount = juce::jlimit (
                1, 2, object->hasProperty ("inputChannelCount")
                    ? static_cast<int> (
                        object->getProperty ("inputChannelCount"))
                    : 2);
            const auto tapPoint = object->getProperty (
                "recordingTapPoint").toString();
            track->recordingTapPoint = tapPoint == "device-output"
                    || tapPoint == "program-output"
                ? RecordingTapPoint::programOutput
                : RecordingTapPoint::hardwareInput;
            track->recordOffsetSamples = juce::jlimit (
                -8192, 8192, static_cast<int> (
                    object->getProperty ("recordOffsetSamples")));
        }
    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionTwentyOne (
    const juce::DynamicObject& root,
    const juce::File& projectDirectory,
    ProjectState& destination)
{
    const auto baseResult = loadVersionTwenty (
        root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;

    const auto syncSource = root.getProperty ("syncSource").toString();
    destination.syncSettings.source = syncSource == "midi-clock"
        ? SyncSourceState::midiClock
        : syncSource == "mtc" ? SyncSourceState::midiTimeCode
        : syncSource == "link" ? SyncSourceState::link
                                 : SyncSourceState::internal;
    destination.syncSettings.sendMidiClock = static_cast<bool> (
        root.getProperty ("sendMidiClock"));
    const auto followStartStop = root.getProperty (
        "followExternalStartStop");
    destination.syncSettings.followExternalStartStop = followStartStop.isVoid()
        || static_cast<bool> (followStartStop);
    const auto frames = static_cast<int> (root.getProperty (
        "mtcFramesPerSecond"));
    destination.syncSettings.mtcFramesPerSecond = frames == 24 || frames == 25
            || frames == 29 || frames == 30
        ? frames : 30;
    destination.syncSettings.jitterToleranceSamples = juce::jlimit (
        0, 8192, root.hasProperty ("jitterToleranceSamples")
            ? static_cast<int> (root.getProperty ("jitterToleranceSamples"))
            : 256);

    destination.monitorPaths.clear();
    if (const auto* values = root.getProperty ("monitorPaths").getArray())
        for (const auto& value : *values)
        {
            const auto* object = value.getDynamicObject();
            if (object == nullptr)
                continue;
            MonitorPathState path;
            path.id = object->getProperty ("id").toString();
            if (path.id.isEmpty()) path.id = juce::Uuid().toString();
            path.name = object->getProperty ("name").toString();
            const auto role = object->getProperty ("role").toString();
            path.role = role == "cue" ? MonitorPathRole::cue
                : role == "listen" ? MonitorPathRole::listen
                : role == "talkback" ? MonitorPathRole::talkback
                                       : MonitorPathRole::controlRoom;
            path.sourceId = object->getProperty ("sourceId").toString();
            if (path.sourceId.isEmpty()) path.sourceId = "master";
            path.outputStartChannel = juce::jlimit (0, 63, static_cast<int> (
                object->getProperty ("outputStartChannel")));
            path.outputChannelCount = juce::jlimit (1, 16, static_cast<int> (
                object->getProperty ("outputChannelCount")));
            path.gain = juce::jlimit (0.0f, 2.0f, static_cast<float> (
                object->getProperty ("gain")));
            path.muted = static_cast<bool> (object->getProperty ("muted"));
            path.dimmed = static_cast<bool> (object->getProperty ("dimmed"));
            destination.monitorPaths.push_back (std::move (path));
        }

    if (const auto* tracks = root.getProperty ("tracks").getArray())
        for (const auto& trackValue : *tracks)
        {
            const auto* object = trackValue.getDynamicObject();
            if (object == nullptr)
                continue;
            const auto id = object->getProperty ("id").toString();
            auto track = std::find_if (
                destination.tracks.begin(), destination.tracks.end(),
                [&id] (const auto& candidate) { return candidate.id == id; });
            if (track == destination.tracks.end())
                continue;
            track->layout = busLayoutFromObject (*object);
            track->pluginSlots = pluginSlotsFromObject (*object);
            if (track->pluginSlots.empty()
                && track->pluginDescriptionXml.isNotEmpty())
            {
                PluginSlotState slot;
                slot.id = "instrument:" + track->id;
                slot.stablePluginId = track->pluginName.isNotEmpty()
                    ? track->pluginName : slot.id;
                slot.name = track->pluginName;
                slot.descriptionXml = track->pluginDescriptionXml;
                slot.stateBase64 = track->pluginStateBase64;
                slot.role = PluginSlotRole::instrument;
                slot.layout = { 0, 2, 0, true };
                slot.bypassed = track->pluginBypassed;
                slot.available = track->pluginAvailable;
                slot.latencySamples = track->pluginLatencySamples;
                track->pluginSlots.push_back (std::move (slot));
            }

            if (const auto* clips = object->getProperty ("clips").getArray())
                for (const auto& clipValue : *clips)
                {
                    const auto* clipObject = clipValue.getDynamicObject();
                    if (clipObject == nullptr)
                        continue;
                    const auto clipId = clipObject->getProperty ("id").toString();
                    auto clip = std::find_if (
                        track->clips.begin(), track->clips.end(),
                        [&clipId] (const auto& candidate)
                    {
                        return candidate.id == clipId;
                    });
                    if (clip == track->clips.end())
                        continue;
                    clip->name = clipObject->getProperty ("name").toString();
                    clip->colour = juce::Colour (static_cast<juce::uint32> (
                        int64Property (*clipObject, "colour", 0x00000000)));
                    clip->muted = static_cast<bool> (
                        clipObject->getProperty ("muted"));
                    clip->locked = static_cast<bool> (
                        clipObject->getProperty ("locked"));
                    clip->fadeInSamples = juce::jlimit (
                        static_cast<juce::int64> (0), clip->lengthInSamples,
                        int64Property (*clipObject, "fadeInSamples"));
                    clip->fadeOutSamples = juce::jlimit (
                        static_cast<juce::int64> (0), clip->lengthInSamples,
                        int64Property (*clipObject, "fadeOutSamples"));
                }

            if (const auto* clips = object->getProperty ("midiClips").getArray())
                for (const auto& clipValue : *clips)
                {
                    const auto* clipObject = clipValue.getDynamicObject();
                    if (clipObject == nullptr)
                        continue;
                    const auto clipId = clipObject->getProperty ("id").toString();
                    auto clip = std::find_if (
                        track->midiClips.begin(), track->midiClips.end(),
                        [&clipId] (const auto& candidate)
                    {
                        return candidate.id == clipId;
                    });
                    if (clip == track->midiClips.end())
                        continue;
                    if (const auto* notes = clipObject->getProperty (
                            "notes").getArray())
                        for (const auto& noteValue : *notes)
                        {
                            const auto* noteObject = noteValue.getDynamicObject();
                            if (noteObject == nullptr)
                                continue;
                            const auto noteId = noteObject->getProperty (
                                "id").toString();
                            auto note = std::find_if (
                                clip->notes.begin(), clip->notes.end(),
                                [&noteId] (const auto& candidate)
                            {
                                return candidate.id == noteId;
                            });
                            if (note == clip->notes.end())
                                continue;
                            note->noteId = static_cast<std::uint32_t> (
                                int64Property (*noteObject,
                                    juce::Identifier ("noteId")));
                            note->expression.clear();
                            if (const auto* expression = noteObject->getProperty (
                                    "expression").getArray())
                                for (const auto& pointValue : *expression)
                                {
                                    const auto* pointObject =
                                        pointValue.getDynamicObject();
                                    if (pointObject == nullptr)
                                        continue;
                                    MidiNoteState::ExpressionPoint point;
                                    point.id = pointObject->getProperty (
                                        "id").toString();
                                    if (point.id.isEmpty())
                                        point.id = juce::Uuid().toString();
                                    point.offsetBeats = juce::jmax (0.0,
                                        static_cast<double> (pointObject->getProperty (
                                            "offsetBeats")));
                                    point.value = juce::jlimit (-1.0f, 1.0f,
                                        static_cast<float> (pointObject->getProperty (
                                            "value")));
                                    const auto type = pointObject->getProperty (
                                        "type").toString();
                                    point.type = type == "pressure"
                                        ? MidiNoteState::ExpressionPoint::Type::pressure
                                        : type == "timbre"
                                            ? MidiNoteState::ExpressionPoint::Type::timbre
                                            : MidiNoteState::ExpressionPoint::Type::pitch;
                                    note->expression.push_back (std::move (point));
                                }
                        }
                    clip->controllers.clear();
                    if (const auto* controllers = clipObject->getProperty (
                            "controllers").getArray())
                        for (const auto& eventValue : *controllers)
                        {
                            const auto* eventObject = eventValue.getDynamicObject();
                            if (eventObject == nullptr)
                                continue;
                            MidiControllerEventState event;
                            event.id = eventObject->getProperty ("id").toString();
                            if (event.id.isEmpty()) event.id = juce::Uuid().toString();
                            event.beat = juce::jmax (0.0, static_cast<double> (
                                eventObject->getProperty ("beat")));
                            event.channel = juce::jlimit (1, 16, static_cast<int> (
                                eventObject->getProperty ("channel")));
                            event.controller = juce::jlimit (0, 127,
                                static_cast<int> (eventObject->getProperty (
                                    "controller")));
                            event.value = static_cast<std::uint32_t> (
                                int64Property (*eventObject,
                                    juce::Identifier ("value")));
                            event.noteId = static_cast<std::uint32_t> (
                                int64Property (*eventObject,
                                    juce::Identifier ("noteId")));
                            clip->controllers.push_back (std::move (event));
                        }
                }
        }

    if (const auto* channels = root.getProperty ("mixerChannels").getArray())
        for (const auto& channelValue : *channels)
        {
            const auto* object = channelValue.getDynamicObject();
            if (object == nullptr)
                continue;
            const auto id = object->getProperty ("id").toString();
            auto channel = std::find_if (
                destination.mixerChannels.begin(),
                destination.mixerChannels.end(),
                [&id] (const auto& candidate) { return candidate.id == id; });
            if (channel == destination.mixerChannels.end())
                continue;
            channel->layout = busLayoutFromObject (*object);
            channel->pluginSlots = pluginSlotsFromObject (*object);
        }
    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionTwentyTwo (
    const juce::DynamicObject& root,
    const juce::File& projectDirectory,
    ProjectState& destination)
{
    const auto baseResult = loadVersionTwentyOne (
        root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;

    if (const auto* records = root.getProperty ("repairRecords").getArray())
        for (const auto& value : *records)
            if (const auto* object = value.getDynamicObject())
            {
                auto record = repairRecordFromObject (*object);
                if (record.area.isNotEmpty() || record.message.isNotEmpty())
                    destination.repairRecords.push_back (std::move (record));
            }

    if (const auto* manifest = root.getProperty (
            "undoManifest").getDynamicObject())
        destination.undoManifest = undoManifestFromObject (*manifest);

    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionTwentyThree (
    const juce::DynamicObject& root,
    const juce::File& projectDirectory,
    ProjectState& destination)
{
    const auto baseResult = loadVersionTwentyTwo (
        root, projectDirectory, destination);
    if (baseResult.failed())
        return baseResult;

    if (const auto* tracks = root.getProperty ("tracks").getArray())
        for (const auto& trackValue : *tracks)
        {
            const auto* trackObject = trackValue.getDynamicObject();
            if (trackObject == nullptr)
                continue;
            const auto trackId = trackObject->getProperty ("id").toString();
            auto track = std::find_if (
                destination.tracks.begin(), destination.tracks.end(),
                [&trackId] (const auto& candidate)
                {
                    return candidate.id == trackId;
                });
            if (track == destination.tracks.end())
                continue;
            if (const auto* clips = trackObject->getProperty ("clips").getArray())
                for (const auto& clipValue : *clips)
                {
                    const auto* clipObject = clipValue.getDynamicObject();
                    if (clipObject == nullptr)
                        continue;
                    const auto clipId = clipObject->getProperty ("id").toString();
                    auto clip = std::find_if (
                        track->clips.begin(), track->clips.end(),
                        [&clipId] (const auto& candidate)
                        {
                            return candidate.id == clipId;
                        });
                    if (clip == track->clips.end())
                        continue;
                    clip->variants.clear();
                    if (const auto* variants = clipObject->getProperty (
                            "variants").getArray())
                        for (const auto& variantValue : *variants)
                        {
                            const auto* variantObject =
                                variantValue.getDynamicObject();
                            if (variantObject == nullptr)
                                continue;
                            auto variant = clipVariantFromObject (
                                *variantObject);
                            if (variant.sourceId.isNotEmpty()
                                && variant.lengthInSamples > 0)
                                clip->variants.push_back (std::move (variant));
                        }
                }
        }

    return juce::Result::ok();
}

juce::Result ProjectDocument::loadVersionTwentyFour (
    const juce::DynamicObject& root,
    const juce::File& projectDirectory,
    ProjectState& destination)
{
    return loadVersionTwentyThree (root, projectDirectory, destination);
}
}
