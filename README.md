# Triumph Studio

Triumph Studio is a clean-room Windows-first commercial DAW foundation built with C++20 and JUCE 8. Version 0.22.0-beta.1 is the first closed beta engineering build: a controlled prerelease for real-world playback, editing, project, export, recording, device-recovery, and shortcut testing. It is not a production release. The professional-controls surface replaces word-heavy project and transport toolbars with resolution-independent vector icons while preserving descriptive tooltips, accessible button names, visible keyboard focus, toggle state, and compact-window behavior. A conflict-checked command map provides one-key transport, recording, monitoring, mixer, automation, snap, and zoom control plus standard project, track, import, export, and editing shortcuts. The build also retains transport-preserving device recovery, direct 440 Hz hardware testing, guarded Play behavior, active-output reporting, immutable clip schedules, dependable Alt-right-edge varispeed editing, ordered effect racks, route-aware delay compensation, explicit monitor paths, MPE, MIDI 2.0 UMP contracts, MIDI Clock following, and MTC position chase. The live audio-effect rack currently uses explicitly labeled trusted in-process mode with exception containment; the cross-process runtime worker remains an open public-release gate. Read [the Beta 1 known issues](KNOWN_ISSUES.md) before testing.

## Current capabilities

- RCU-style prepared render generations published atomically at audio-block boundaries
- No `CriticalSection`, mutex, `ScopedLock`, or `ScopedTryLock` in Triumph Studio's playback or recording callback code
- Old render generations and replaced plug-ins reclaimed only by the control thread after all audio readers leave
- Prepared track readers, clip schedules, MIDI timing, mixer routes, delay lines, automation, scratch buffers, and plug-in buffers
- Last-valid-render-state continuity while a replacement is being prepared or an invalid mixer graph is rejected
- Bounded master-parameter queue with atomic latest-value recovery and overflow telemetry
- Live callback count, average/max load, deadline miss, suspected dropout, snapshot swap, queue overflow, and render-generation telemetry
- Visible `RT` load and `XRUN` count with a detailed non-audio-thread status tooltip
- Recording-writer teardown through reader quiescence instead of callback try-lock contention
- Integer Q20 project-sample transport authority with cross-device-rate drift regression
- Explicit armed/pre-roll/recording/punch/loop/finalize/abort/recovery state model
- One-second WAV header checkpoints plus atomic active-capture journals
- Interrupted-take recovery offered when the matching project reopens
- Persistent Off, While Armed, and Always input-monitor modes
- Persisted signed record-alignment calibration in device samples
- Crash-isolated VST3 discovery in a timeout-bounded companion process
- Atomic version-2 plug-in registry with stable identity, vendor/version/type, channel/ARA metadata, file fingerprints, availability, attempts, and failure history
- Standard-folder VST3 discovery through the crash-isolated helper, skipping unchanged validated or blocked files deterministically
- Plug-in Registry menu with retry/rescan, missing-file visibility, and explicit blocked-record removal
- Ordered audio-effect racks with up to sixteen persistent slots on tracks, buses, and returns
- VST3 effect loading, editor access, bypass/removal, state capture, latency-change handling, and stable parameter automation
- Versioned runtime-worker protocol, bounded triple-buffer transport contract, heartbeat watchdog, restart budget, and deterministic fault model
- Main-output bus negotiation with explicit rejection of instruments that expose no usable output
- Stable hosted parameter IDs with deterministic legacy fallback, inventory, and bounded sample-offset READ/TOUCH/LATCH/WRITE automation delivery in live playback and freeze
- Atomic `.triumphpreset` save/load with stable plug-in identity and state-payload checksum validation
- One bounded 1–30 second natural-tail policy shared by live note/stop and instrument-freeze paths, with defined fade/mute behavior for bypass, removal, and faults
- Per-plug-in process timing, deadline-miss telemetry, and exception-safe mute/bypass with unavailable-placeholder recovery
- Atomic plug-in latency-change notification followed by control-thread PDC rebuild
- State-preserving VST3 bypass with all-notes-off, continued processing, 10 ms output fade, and retained PDC latency
- Responsive project and transport toolbars validated at the 1280 x 720 minimum window size
- Resolution-independent vector toolbar icons with descriptive tooltips, accessible command names, toggle indication, and keyboard-focus rings
- Conflict-checked keyboard command map for Space, R, period, W/Home, M, B, A, S, zoom, project, track, import/export, undo/redo, split, delete, and F1 actions
- A real compact `More` menu for Recent, Collect, Relink, Low Latency, Split, Use Take, Detect, and Warp All; every item invokes the same existing command as its full-width button
- Expanded toolbars retain direct access to every command at 1380 pixels and wider
- Warning-clean application-source validation under strict GCC diagnostics, including removal of the deprecated JUCE editor call
- ASCII-safe user-facing source text for predictable Windows rendering
- Docked Triumph Assistant opened from Help or F1
- Deterministic search across verified recording, editing, MIDI, mixer, tempo, producer, export, safety, and shortcut guidance
- Quick-topic buttons plus ranked related articles for natural how-to questions
- Context summary showing recording state, track count, and clip selection
- Related-action buttons that open real existing controls instead of simulated workflows
- Explicit truthful answers for unavailable cloud, collaboration, publishing, browser, and marketplace capabilities
- Fully offline question handling with no audio, project data, telemetry, or question upload
- Toggleable AI Producer panel with key, scale, style, length, and variation controls
- Local chord, drum, and melody generation into ordinary editable MIDI clips
- Deterministic variation seeds: the same settings reproduce the same notes
- Audible General MIDI kick, snare, and hi-hat timbres in live and offline rendering
- Project-aware Mix Balance using track roles, track names, count, and instrument state
- One-step Undo/Redo for each generated track and each complete mix balance
- Persisted producer settings in the project document with no network request or upload
- Real system audio output through JUCE
- Native ASIO device support on Windows, with Windows Audio/WASAPI fallbacks
- Independent input-channel selection for true mono microphone or instrument takes
- Live dual-channel input peak meters with 1.5-second clipping indicators
- Mono or stereo audio-input capture to timestamped 24-bit WAV files
- Repeat recording onto an existing audio track with automatically numbered take lanes
- One undoable active-comp selection per take group, with inactive takes retained
- Shift-drag swipe comping from any take lane into a persistent comp lane
- Automatic 10 ms equal-power crossfades at adjacent take-source changes
- Alt-drag clip-end time stretching with undo, save/load, and comp-aware playback
- Provider-neutral stretch state with a truthful built-in varispeed provider
- Ctrl-click warp-marker creation, Ctrl-drag timing edits, and right-click removal
- Piecewise rate playback with warp markers preserved through trim, split, undo, and save
- Worker-thread transient detection over real decoded source samples
- Separate teal analysis markers plus one undoable **Warp All** conversion command
- Toggleable Advanced Audio Edit panel for the selected arrangement clip
- Non-destructive reverse playback with live/offline delivery parity and visible REV state
- Worker-thread peak analysis plus undoable -1 dB clip normalization with a 16x gain safety cap
- Length-preserving -24 to +24 semitone built-in pitch processing to project-owned 24-bit WAV
- Original pitch-shift source media retained so Undo restores the prior clip reference
- Atomic 48 kHz/24-bit stereo WAV export with fixed-seed TPDF dither and a two-second tail
- Real 44.1/48/96 kHz, 16/24-bit delivery presets with bit-depth-correct deterministic dither
- User-cancellable background export that preserves existing destinations and removes partial files
- Selected-clip-range WAV delivery with sample-accurate boundaries
- Atomic per-track stem delivery with collision-safe names and a deterministic JSON manifest
- Export parity for audio comping, fades, gain/pan, mute/solo, warp playback, built-in MIDI, and master gain
- Explicit VST3 export blocking for live external instruments, with delivery enabled after a committed freeze-to-audio
- Undoable overlapping comp-range replacement without rewriting recorded media
- Multi-source playback inside one audio track so distinct recorded takes render correctly
- Background-thread disk writing through a bounded recording FIFO
- Visible recording-dropout detection with lost-block and lost-duration reporting
- Persistent instrument tracks, MIDI clips, and stable MIDI note identities
- Selectable system MIDI inputs in the Device panel
- Live, low-latency MIDI monitoring through the built-in Triumph Instrument
- Manual discovery through a crash-isolated scanner followed by asynchronous loading of a validated Windows VST3 instrument
- One active project-owned VST3 instrument slot with live and timeline MIDI routing
- Native third-party instrument editor windows and full plug-in state persistence
- Undoable, state-preserving VST3 bypass plus persistent available/unavailable safe state
- Offline instrument freeze to project-owned 24-bit stereo WAV with plug-in-tail capture
- Undoable unfreeze that restores retained MIDI, plug-in identity, and processor state
- Live plug-in latency reporting and delay compensation for non-plug-in playback paths
- Acyclic graph-wide delay planning across audio, send, and sidechain route classes
- Route-specific compensation delay lines across channel outputs, pre/post-fader sends, and sidechain feeds
- Explicit mono/stereo main and sidechain layout validation plus independent Control Room, cue, listen, and talkback monitor paths
- Real per-track stereo sample-delay lines instead of one shared playback delay
- Persistent, undoable manual latency offsets from -262143 to +262143 samples
- Project-owned Low Latency mode that protects armed monitoring paths from added delay
- Automation scheduling lookahead derived from each node's compensated input arrival
- Toggleable docked mixer with horizontally scrollable channel strips
- Undoable track, return, and bus output routing through an acyclic signal graph
- Multiple persisted pre-fader or post-fader sends per mixer owner
- Dedicated return channels and buses with gain, pan, mute, solo, and output selection
- Sidechain routes with real envelope-driven destination ducking
- Graph validation that holds the last valid audio plan when a route would create a cycle
- Dedicated master strip with persisted mute and smoothed gain
- Lock-free master-analysis stream with a 2048-point live spectrum display
- Per-channel stereo meters and visible sidechain gain-reduction indication
- Live/offline mixer parity for mix export, selection export, and stems
- Thread-safe MIDI note-on/note-off capture with channel and velocity preservation
- Armed-instrument recording routed into an undoable, auto-extending MIDI take
- Beat-based MIDI note positions, lengths, velocity, channel, and 1/16 quantize
- Editable piano roll with note creation and deletion
- Piano-roll editing for per-note MPE pitch, pressure, and timbre points plus high-resolution controller events
- MIDI 2.0 UMP note/per-note-pitch contracts with sample-offset block scheduling and MIDI 1/MPE compatibility delivery to JUCE VST3 hosts
- MIDI Clock tempo/start/stop following, full MTC quarter-frame reconstruction and position chase, lock/jitter status, and persisted sync settings
- Built-in deterministic Triumph Instrument tone generator for immediate playback
- Toggleable Tempo panel with a seconds ruler, tempo lane, signature/marker lane, automation lane, and playhead
- Multiple undoable tempo points with step or mathematically integrated linear BPM ramps
- Multiple time-signature events, named project markers, and exact beat/second/sample conversion
- Real audio-engine metronome with bar accents, meter-aware beat spacing, and persistent level
- Persisted gain/pan automation lanes targeting tracks, returns, buses, or master
- Functional READ, TOUCH, LATCH, and WRITE capture modes plus hold, linear, and smooth curves
- Timeline-sample-accurate automation at arbitrary device rates with live/offline mixer parity
- Bounded sample-offset automation delivery for current hosted instrument and
  insert plug-in paths
- Double-click automation point editing plus playhead-based tempo, signature, marker, lane, and point commands
- Sample-rate-based master parameter smoothing that is independent of block size
- Persistent audio-device, sample-rate, buffer-size, and channel configuration
- Empty audio tracks with exclusive record-arm and automatic armed-track creation
- Optional input monitoring, disabled by default to reduce feedback risk
- Reported input-latency compensation when a recorded take is placed on the timeline
- Recorded media stored inside each saved project's `Recordings` folder
- Multiple simultaneous audio tracks
- WAV, AIFF, FLAC, MP3, and OGG import when supported by the JUCE build
- Waveforms generated from imported audio files
- Play, pause, stop, rewind, and timeline seeking
- Track volume, stereo pan, mute, and solo
- Master output level
- Versioned `.triumph` project documents with atomic replacement on save
- Backward-compatible loading and migration of format 1–17 projects
- Persistent project identity and project-relative media paths
- Save As, recent-project history, and Save/Discard/Cancel protection
- Two-minute recovery autosaves and startup crash recovery
- Twenty rolling project checkpoints in each project's `Backups` folder
- Collect and Save into a portable project `Media` folder
- Missing-media detection and recursive folder relinking
- Selectable audio clips with visible edit handles
- Undoable clip move, start trim, end trim, split, and delete commands
- Optional 250 ms grid snapping and adjustable horizontal timeline zoom
- Timeline-aware playback of multiple clips, gaps, source offsets, and clip gain
- Sample-rate-correct conversion between source time and project timeline time
- Stable UUIDs for media sources, tracks, and clips
- Non-destructive source/clip separation in the project model
- Undo/redo for imports, track properties, and master gain
- Missing-source placeholders and reporting
- Approved cool-grey and blue visual foundation; no black/gold theme
- Pure C++ release-identity, shortcut, timeline, fixed-sample-clock, recording-state, tempo/automation/write, smoothing, MIDI-capture/performance, plug-in-graph/host-policy/runtime-rack, sample-delay, mixer/professional-graph, advanced-edit, producer-assistant, help-assistant, responsive-layout, and real-time-state tests plus JUCE recording-journal, checksummed preset, crash-isolated scanner/registry, project-model, callback-audit, and live-engine stress tests (thirty-three Windows tests total)

This build does not display fake plug-ins, fake meters, generated demo sessions, account login, cloud storage, collaboration, publishing, or marketplace screens. Features appear only after their underlying engine is implemented. The AI Producer label describes assistive music-generation workflows; it does not claim to run a trained generative model, analyze audio content, call a cloud service, or upload project data. Triumph Assistant searches bundled product guidance and does not pretend to be an online conversational model.

## Architecture

```text
MainComponent
  ├─ ProjectModel        Source of truth, persistent identity, clips, undo/redo
  ├─ ProjectDocument     Versioned JSON, relative paths, atomic save
  ├─ ProjectWorkspace    Recovery, backups, recent files, collect, relink
  ├─ AudioEngine         Prepared render snapshots, global transport, monitoring
  ├─ AudioRecorder       Lock-free callback handoff, FIFO, background WAV writer
  ├─ MIDI/Tempo Engine   Device input, capture, beat map, event timing, renderer
  ├─ MixerGraph          Validated output/send/sidechain processing order
  ├─ MixerComponent      Track, return, bus, master, meters, analyzer
  ├─ TempoAutomation     Musical timeline and automation editor
  ├─ AdvancedAudioEdit   Reverse, normalization, and offline pitch processing
  ├─ ProducerAssistant   Local MIDI generation and mix starting-point suggestions
  ├─ HelpAssistant       Offline verified guidance and real control navigation
  └─ ArrangementComponent
       └─ TrackRowComponent observes model state; it does not own audio tracks
```

`MediaSourceState` identifies a physical audio file. `AudioClipState` is a non-destructive reference containing source, timeline, native-source offset, project-timeline length, gain, direction, stretch ratio/provider, warp anchors, take-group identity, lane number, and baseline active-comp state. `CompRegionState` overrides that baseline for a bounded timeline range by stable take lane. `ProjectModel` resolves these edits into piecewise-rate, crossfaded playback segments; neither UI components nor the audio engine own the edit decision. Pitch shifting creates a new project-owned media source and swaps only the selected clip reference in one undo transaction. `MidiClipState` stores beat-relative notes independently of audio media. Tempo-dependent MIDI playback timing remains precomputed outside the audio callback.

## Windows requirements

1. Windows 10 or Windows 11 (64-bit)
2. Visual Studio with **Desktop development with C++**
3. CMake 3.22 or newer
4. Git

JUCE 8.0.13 is fetched automatically during the first configure. For an offline build, place a JUCE checkout in `JUCE/` or pass `-DTRIUMPH_JUCE_PATH=C:\path\to\JUCE`.

## Build on your current Visual Studio 2026 setup

Open PowerShell in the `triumph-studio` folder and run:

```powershell
& "C:\Program Files\CMake\bin\cmake.exe" -S . -B build -G "Visual Studio 18 2026" -A x64
```

```powershell
& "C:\Program Files\CMake\bin\cmake.exe" --build build --config Release
```

The executable is produced at:

```text
build\TriumphStudio_artefacts\Release\Triumph Studio.exe
```

JUCE names the generated folder `artefacts` with an `e`. To avoid path typos
and to verify the scanner helper is beside the app, launch with:

```powershell
.\tools\LaunchTriumphStudio.ps1
```

After the complete Release suite passes, create the closed-beta ZIP and its
SHA-256 sidecar with:

```powershell
& "C:\Program Files\CMake\bin\cmake.exe" --build build --config Release --target package
```

## Run tests

```powershell
& "C:\Program Files\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
```

## Project layout

```text
src/
  audio/       Multitrack playback, monitoring, and background WAV capture
  core/        Framework-independent timeline, tempo, smoothing, and MIDI capture
  model/       Project model, IDs, source/clip/track state, undo/redo
  plugin/      Helper-process discovery, persistent registry, and preset integrity
  project/     Serialization, migration, recovery, backup, and media portability
  ui/          Arrangement, track controls, and grey/blue design system
  Main.cpp
  MainComponent.*
tests/
docs/
```

## Current project format

Format version 24 is readable JSON. It adds bounded persistent undo/redo restore points to format 23 audio clip variants, format 22 project repair records, media asset fingerprints, and the persisted undo manifest; format 21 added plug-in slots, explicit channel bus layouts, monitor paths, sync settings, high-resolution MIDI controllers, stable note identities, and per-note expression. Audio clip positions, automation points, marker timeline offsets, comp ranges, and lengths use the project timeline sample rate; clip source offsets use the source file's native rate; tempo, signatures, project markers, MIDI, MPE, and controller events use beat-relative positions. Media inside the project folder uses portable relative paths; external media remains an absolute reference until Collect and Save copies it into `Media/`. Saving uses a temporary adjacent file followed by atomic replacement. Formats 1–23 remain loadable and are written as format 24 on their next save.

See `docs/ProfessionalControlsCheckpoint.md`, `docs/ProfessionalSystemsCheckpoint.md`, `docs/PersistentUndoCheckpoint.md`, `docs/RealtimeFaultAttributionCheckpoint.md`, `docs/RealtimeDiagnosticsSurfaceCheckpoint.md`, `docs/RealtimeDiagnosticsReportCheckpoint.md`, `docs/PluginScannerReliabilityCheckpoint.md`, `docs/PluginQuarantineControlsCheckpoint.md`, `docs/SampleOffsetPluginAutomationCheckpoint.md`, `docs/DesignSystem.md`, `docs/Architecture.md`, `docs/ProjectSafety.md`, `docs/PluginIsolationBoundary.md`, `docs/PluginHostingRecoveryComplete.md`, `docs/FoundationsRecoveryEvidence.md`, `docs/Phase18AComplete.md`, `docs/BetaReleaseChecklist.md`, `docs/HelpAssistantBoundary.md`, `docs/AIProducerBoundary.md`, `docs/ARAResearch.md`, and `docs/ThirdPartyAudio.md` for the current product rules.
