#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

#include "ReleaseIdentity.h"

namespace triumph::help
{
struct Article
{
    std::string id;
    std::string title;
    std::string summary;
    std::string steps;
    std::string keywords;
    std::string action;
    std::string actionLabel;
};

struct SearchResult
{
    const Article* article = nullptr;
    int score = 0;
};

inline const std::vector<Article>& articles()
{
    static const std::vector<Article> content {
        {
            "start", "Getting started",
            "Create or import a track, confirm the audio device, then press Play.",
            "1. Use + Import Audio, + Track, or + MIDI.\n"
            "2. Open Device and confirm the output and input channels.\n"
            "3. Use Play, Pause, Stop, Rewind, and the timeline to navigate.\n"
            "4. Save before recording so takes have a durable project folder.",
            "start begin first project import audio track playback transport", "device", "Open Device"
        },
        {
            "record", "Record audio safely",
            "Save the project, arm one audio track, choose the input, and record with monitoring off unless needed.",
            "1. Save the project first.\n"
            "2. Choose + Track to create and arm an audio track.\n"
            "3. Open Device and select the microphone or instrument input channel.\n"
            "4. Press Record. Press Stop Rec when finished.\n"
            "5. Enable Monitor only when needed, and use headphones to prevent feedback.\n"
            "Repeated passes become retained take lanes instead of overwriting earlier WAV files.",
            "record microphone mic guitar vocal take lane arm input monitor feedback headphones", "device", "Open Device"
        },
        {
            "device", "Audio device and latency",
            "Use Device to choose ASIO or Windows Audio, sample rate, buffer size, channels, and run a direct output test.",
            "Open Device, choose the device type and hardware, then select the active input and output channels. Press Test Output and listen for the one-second 440 Hz tone before testing a project.\n"
            "A smaller buffer reduces monitoring delay but increases CPU pressure. Low Latency protects armed paths from added graph delay.\n"
            "If Test Output is silent, choose another hardware output or fix Windows routing. If the tone works but the project is silent, check master level, track mute/solo, and OUT routing. If an active device disconnects, Triumph pauses without rewinding and resumes only after a valid replacement callback is prepared.",
            "device asio wasapi windows audio focusrite speaker buffer sample rate latency no sound input output", "device", "Open Device"
        },
        {
            "audio-edit", "Edit audio clips",
            "Select a clip to split, trim, stretch, warp, reverse, normalize, or pitch-process it non-destructively.",
            "Select a clip in the arrangement. Use Split at the playhead, drag clip edges to trim, or Alt-drag the end to stretch.\n"
            "Use Detect then Warp All for transient-based warp anchors. Open Edit for Reverse, Normalize, and offline pitch processing.\n"
            "Original recorded or imported media is retained; supported edits participate in Undo/Redo.",
            "edit audio clip split trim stretch warp transient reverse normalize pitch comp take", "edit", "Open Edit"
        },
        {
            "midi-record", "Record MIDI",
            "Create an instrument track, enable a MIDI input, arm the track, and record beat-based notes.",
            "1. Press Ctrl+Shift+T or use Add Instrument Track.\n"
            "2. Open Audio Device and enable the intended MIDI input.\n"
            "3. Arm the instrument track and press R or Record.\n"
            "4. Play the controller, then stop recording.\n"
            "Captured channel, pitch, velocity, start, and duration are stored as editable MIDI notes.",
            "midi record controller keyboard instrument arm notes velocity channel", "device", "Open Device"
        },
        {
            "piano-roll", "Edit MIDI in Keys",
            "Use Keys to add, delete, and quantize notes in an instrument track.",
            "Open Keys after creating or selecting an instrument track. Double-click to add a note and right-click to delete one.\n"
            "Use Quantize to align note starts to the supported grid. MIDI remains beat-based, so tempo changes preserve musical timing.",
            "midi keys piano roll note add delete quantize grid edit instrument", "keys", "Open Keys"
        },
        {
            "vst3", "Load and freeze a VST3 instrument",
            "Use VST3 to scan standard folders, load a validated instrument, manage presets and automation, open its editor, bypass it, or freeze it to audio.",
            "Choose VST3 and scan standard folders or load a user-installed instrument. Discovery runs in the separate timeout-bounded scanner, and the registry retains validated, blocked, and missing entries.\n"
            "The VST3 menu can save or load an identity-checked Triumph preset and create automation lanes from stable hosted parameter IDs. Freeze renders retained MIDI, state, and plug-in automation into project-owned audio. Unfreeze restores the MIDI and plug-in state.\n"
            "This build has one active project-owned external instrument slot and does not bundle third-party plug-ins.",
            "vst vst3 plugin plug-in instrument scan registry blocklist preset parameter automation editor bypass freeze unfreeze missing unavailable", "vst3", "Open VST3"
        },
        {
            "mixer", "Mix, route, send, and sidechain",
            "Open Mixer for track, return, bus, master, send, output, mute, solo, meter, and spectrum controls.",
            "Use + Return or + Bus to create mixer channels. Choose OUT to route a track or channel, and + SEND to create a send.\n"
            "Pre/post-fader sends and sidechain routes are stored with the project. Invalid cyclic routing is rejected while the last valid audio plan continues.\n"
            "Mix Balance in AI Producer offers only a starting point; listen and use the meters before delivery.",
            "mixer mix route routing bus return send pre fader post fader sidechain duck mute solo master spectrum", "mixer", "Open Mixer"
        },
        {
            "tempo", "Tempo, meter, markers, metronome, and automation",
            "Open Tempo to edit tempo ramps, signatures, markers, metronome state, and parameter automation.",
            "Tempo points may use step or linear transitions. Add signature events and markers at the playhead.\n"
            "Automation lanes target gain or pan on tracks, returns, buses, or master; the VST3 menu can also add stable plug-in parameter lanes. READ, TOUCH, LATCH, and WRITE modes are supported.\n"
            "The same musical clock drives live playback and offline delivery.",
            "tempo bpm meter signature marker metronome automation read touch latch write curve", "tempo", "Open Tempo"
        },
        {
            "producer", "Use AI Producer",
            "Create deterministic editable chord, drum, and melody MIDI or apply an undoable mix starting balance.",
            "Open AI, choose key, scale, style, length, and variation, then create Chords, Drums, or Melody.\n"
            "Generated results are ordinary MIDI tracks. Channel-10 drums use the built-in kick, snare, and hi-hat sounds.\n"
            "Balance Mix uses track roles and names for conservative gain/pan starting points. It does not analyze audio or master the song.\n"
            "Everything runs locally; no project data is uploaded.",
            "ai producer chord chords drum drums melody variation generate balance mix local", "producer", "Open AI Producer"
        },
        {
            "export", "Export a mix, range, or stems",
            "Use Export Mix for deterministic WAV delivery, selected-range export, or track stems.",
            "Choose Export Mix and select the desired delivery command. Mix and range export use atomic destination replacement; stems use collision-safe names and a JSON manifest.\n"
            "A live external VST3 must be frozen before offline delivery so the export cannot silently omit it. Existing destinations are preserved if a cancellable render fails.",
            "export render bounce mix wav stems range delivery dither 24 bit 48 khz", "export", "Open Export"
        },
        {
            "save", "Save, recover, collect, and relink",
            "Projects use atomic versioned saves, recovery autosaves, rolling backups, and non-destructive media references.",
            "Use Save or Save As for the .triumph document. Collect copies available external media into the project Media folder for portability.\n"
            "Relink searches a chosen folder for missing files without silently guessing duplicate names. Recovery snapshots are offered after an interrupted session, and saved projects keep rolling checkpoints in Backups.",
            "save save as project recovery autosave backup collect media relink missing portable", "save", "Save Project"
        },
        {
            "shortcuts", "Keyboard shortcuts",
            "Use one-key transport and view controls plus standard project and editing shortcuts.",
            "Space: Play or pause\nR: Start or stop recording\nPeriod (.): Stop and return to start\nW or Home: Return to start\nM: Toggle input monitoring\nB: Toggle Mixer\nA: Toggle Tempo and Automation\nS: Toggle Snap\n+ / -: Timeline zoom\nCtrl+T: Add audio track\nCtrl+Shift+T: Add instrument track\nCtrl+I: Import audio\nCtrl+Shift+E: Export mix\nCtrl+S: Save\nCtrl+Shift+S: Save As\nCtrl+O: Open\nCtrl+N: New\nCtrl+Z: Undo\nCtrl+Shift+Z or Ctrl+Y: Redo\nCtrl+E: Split selected clip at the playhead\nDelete or Backspace: Delete selected clip\nF1: Open or close Triumph Assistant",
            "shortcut shortcuts keyboard hotkey transport space record stop rewind monitor mixer automation snap zoom import export track ctrl control save undo redo split delete f1 help", "", ""
        },
        {
            "availability", "Cloud, collaboration, marketplace, and other unavailable features",
            std::string (release::applicationName) + " " + release::versionLabel
                + " does not include account login, cloud backup, collaboration, publishing, or a marketplace.",
            "This closed-beta desktop build is local-first. It does not display fake account, cloud, collaboration, browser, Session Musician, Master Assistant, Performance Mode, or marketplace screens.\n"
            "Those capabilities require real services, authentication, permissions, storage, and product-specific implementation before they can be offered safely.",
            "cloud login account collaboration comments chat publish platform marketplace presets plugins beats browser session musician master assistant performance mode unavailable", "", ""
        },
        {
            "troubleshoot", "Troubleshoot silence or an unavailable control",
            "Check the device, routing, transport, mute/solo state, master level, missing media, and active background work.",
            "For silence: open Device and run Test Output. If the tone works, confirm Play state, track OUT routing, mute/solo, track gain, master gain, and available source media. The RT tooltip separates device recovery, callback-stall, and sustained-silence evidence.\n"
            "Controls are temporarily disabled during recording, export, or advanced audio processing when an unsafe edit must be blocked.\n"
            "A missing VST3 remains explicitly unavailable instead of being silently replaced.",
            "problem troubleshoot no sound silence disabled grey grayed control missing media unavailable error", "device", "Open Device"
        }
    };
    return content;
}

inline std::string normalize (std::string value)
{
    for (auto& character : value)
    {
        const auto byte = static_cast<unsigned char> (character);
        character = std::isalnum (byte) ? static_cast<char> (std::tolower (byte)) : ' ';
    }
    std::string result;
    result.reserve (value.size());
    auto previousSpace = true;
    for (const auto character : value)
    {
        const auto isSpace = character == ' ';
        if (! isSpace || ! previousSpace)
            result.push_back (character);
        previousSpace = isSpace;
    }
    if (! result.empty() && result.back() == ' ') result.pop_back();
    return result;
}

inline std::vector<std::string> tokens (const std::string& value)
{
    const auto normalized = normalize (value);
    std::vector<std::string> result;
    std::string current;
    for (const auto character : normalized)
    {
        if (character == ' ')
        {
            if (! current.empty()) result.push_back (std::exchange (current, {}));
        }
        else current.push_back (character);
    }
    if (! current.empty()) result.push_back (std::move (current));
    return result;
}

inline std::vector<SearchResult> search (const std::string& question,
                                         std::size_t maximumResults = 5)
{
    const auto query = normalize (question);
    const auto words = tokens (question);
    std::vector<SearchResult> results;
    if (query.empty() || maximumResults == 0) return results;

    for (const auto& article : articles())
    {
        const auto title = normalize (article.title);
        const auto summary = normalize (article.summary);
        const auto steps = normalize (article.steps);
        const auto keywords = normalize (article.keywords);
        auto score = 0;
        if (title == query) score += 120;
        if (title.find (query) != std::string::npos) score += 36;
        if (keywords.find (query) != std::string::npos) score += 24;
        if (summary.find (query) != std::string::npos) score += 12;
        for (const auto& word : words)
        {
            if (word.size() <= 1) continue;
            if (title.find (word) != std::string::npos) score += 10;
            if (keywords.find (word) != std::string::npos) score += 7;
            if (summary.find (word) != std::string::npos) score += 4;
            if (steps.find (word) != std::string::npos) score += 1;
        }
        if (score > 0) results.push_back ({ &article, score });
    }

    std::stable_sort (results.begin(), results.end(), [] (const auto& left,
                                                          const auto& right)
    {
        if (left.score != right.score) return left.score > right.score;
        return left.article->title < right.article->title;
    });
    if (results.size() > maximumResults) results.resize (maximumResults);
    return results;
}
}
