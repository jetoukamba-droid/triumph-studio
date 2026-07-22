#include "StemExporter.h"

#include <memory>

namespace triumph
{
juce::String StemExporter::legalStemName (juce::String name)
{
    name = name.trim();
    const juce::String invalidCharacters ("<>:\"/\\|?*");
    for (int index = 0; index < invalidCharacters.length(); ++index)
        name = name.replaceCharacter (invalidCharacters[index], '_');
    for (int index = 0; index < name.length(); ++index)
        if (name[index] < 32)
            name = name.replaceCharacter (name[index], '_');
    while (name.endsWithChar ('.') || name.endsWithChar (' '))
        name = name.dropLastCharacters (1);
    return name.isNotEmpty() ? name.substring (0, 80) : "Unnamed Track";
}

juce::Result StemExporter::renderTrackStems (
    const ProjectState& snapshot,
    const juce::File& destinationDirectory,
    const Options& options,
    std::atomic<bool>& cancelRequested,
    std::atomic<float>& progress)
{
    progress.store (0.0f);
    if (snapshot.tracks.empty())
        return juce::Result::fail ("The project has no tracks to export.");
    if (destinationDirectory.exists())
        return juce::Result::fail ("The stem destination already exists.");

    const auto temporaryDirectory = destinationDirectory.getSiblingFile (
        "." + destinationDirectory.getFileName() + ".partial-"
            + juce::Uuid().toString());
    temporaryDirectory.deleteRecursively();
    if (temporaryDirectory.createDirectory().failed())
        return juce::Result::fail ("The temporary stem folder could not be created.");

    auto manifest = std::make_unique<juce::DynamicObject>();
    manifest->setProperty ("format", "Triumph Studio Stem Delivery");
    manifest->setProperty ("version", 1);
    manifest->setProperty ("projectName", options.projectName);
    manifest->setProperty ("sampleRate", options.render.sampleRate);
    manifest->setProperty ("bitsPerSample", options.render.bitsPerSample);
    manifest->setProperty ("tailSeconds", options.render.tailSeconds);
    juce::Array<juce::var> trackEntries;
    juce::StringArray usedFileNames;

    const auto trackCount = static_cast<int> (snapshot.tracks.size());
    for (int index = 0; index < trackCount; ++index)
    {
        if (cancelRequested.load())
        {
            temporaryDirectory.deleteRecursively();
            return juce::Result::fail ("Export cancelled.");
        }

        const auto& track = snapshot.tracks[static_cast<std::size_t> (index)];
        const auto number = juce::String (index + 1).paddedLeft ('0', 2);
        const auto stemBase = number + " - " + legalStemName (track.name);
        auto fileName = stemBase + ".wav";
        auto suffix = 2;
        while (usedFileNames.contains (fileName, true))
            fileName = stemBase + " (" + juce::String (suffix++) + ").wav";
        usedFileNames.add (fileName);
        auto renderOptions = options.render;
        renderOptions.isolatedTrackId = track.id;
        renderOptions.progressStart = static_cast<float> (index)
                                      / static_cast<float> (trackCount);
        renderOptions.progressSpan = 1.0f / static_cast<float> (trackCount);
        const auto result = OfflineRenderer::renderStereoWav (
            snapshot, temporaryDirectory.getChildFile (fileName), renderOptions,
            cancelRequested, progress);
        if (result.failed())
        {
            temporaryDirectory.deleteRecursively();
            return result;
        }

        auto entry = std::make_unique<juce::DynamicObject>();
        entry->setProperty ("trackId", track.id);
        entry->setProperty ("trackName", track.name);
        entry->setProperty ("file", fileName);
        entry->setProperty ("muted", track.muted);
        entry->setProperty ("solo", track.solo);
        trackEntries.add (juce::var (entry.release()));
    }

    manifest->setProperty ("tracks", juce::var (trackEntries));
    const auto manifestFile = temporaryDirectory.getChildFile ("manifest.json");
    if (! manifestFile.replaceWithText (
            juce::JSON::toString (juce::var (manifest.release()), true)))
    {
        temporaryDirectory.deleteRecursively();
        return juce::Result::fail ("The stem manifest could not be written.");
    }

    if (cancelRequested.load())
    {
        temporaryDirectory.deleteRecursively();
        return juce::Result::fail ("Export cancelled.");
    }
    if (! temporaryDirectory.moveFileTo (destinationDirectory))
    {
        temporaryDirectory.deleteRecursively();
        return juce::Result::fail ("The completed stem folder could not be published.");
    }
    progress.store (1.0f);
    return juce::Result::ok();
}
}
