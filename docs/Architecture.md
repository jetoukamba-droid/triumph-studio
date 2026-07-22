# Triumph Studio Foundations Recovery Architecture

## Ownership

- `ProjectModel` is the editable source of truth.
- `ProjectDocument` converts project snapshots to and from versioned JSON.
- `ProjectWorkspace` owns recent-file settings, recovery snapshots, rolling backups,
  media collection, and missing-media relinking.
- `AudioEngine` owns prepared playback resources, never project identity.
- `AudioRecorder` owns the bounded recording FIFO, background WAV writer, and
  atomic dropped-block telemetry.
- `RecordingPlan` is the allocation-free transport policy for pre-roll, punch, and loop
  capture. It converts project-sample boundaries into exact device-frame spans and assigns
  stable loop-pass numbers without touching the filesystem or project model.
- `MultiTrackRecorder` owns one shared background writer service and one prepared
  `AudioRecorder` per armed track/pass. The message thread opens and finalizes files; the
  callback routes prepared hardware-input or final-device-output spans only.
- `MainComponent` measures input peaks before the render engine modifies the shared
  callback buffer. The audio thread only publishes atomic peak maxima; decay, clip
  hold, painting, and warnings remain on the message thread.
- The Windows build enables JUCE's ASIO backend while retaining Windows Audio modes.
  Device selection and driver ownership stay outside the project document.
- `TempoMap` is framework-independent beat/second/sample conversion. Tempo points are
  normalized on the message thread, never inside the audio callback.
- `MidiCapture` pairs note-on/note-off messages by channel and pitch behind a short
  critical section. The MIDI callback performs no project mutation. Stopping capture
  closes held notes, then the message thread converts the take to beats and commits it
  to `ProjectModel` as one undo transaction.
- `AudioDeviceManager` owns enabled MIDI ports and forwards every enabled input to
  `MainComponent`. Device selection remains machine-local and is never written into a
  project document.
- Instrument render notes carry beat data in the document but receive absolute start/end
  seconds before publication to the engine. Rendering is stable across callback block sizes.
- `Triumph Plugin Scanner` owns VST3 discovery in a companion process. The main application
  enforces a 30-second timeout, parses a versioned JSON result, and atomically records either
  validated descriptions or a blocked failure in the machine-local registry. Only a validated
  description reaches `AudioPluginFormatManager` for asynchronous instance construction.
  The current host deliberately retains one active VST3 instrument slot.
- Live MIDI enters a timestamp-correcting `MidiMessageCollector`; timeline events are added at
  exact sample offsets. The hosted processor receives both streams in the audio callback.
- The project model owns plug-in description XML and processor state. Editor windows are
  non-owning views and are destroyed before their processor is replaced or unloaded.
- `ParameterSmoother` is sample-rate based and preserves state across blocks. UI controls
  publish targets; the audio thread advances smoothing one sample at a time.
- `PianoRollComponent` edits `ProjectModel` commands. It does not own MIDI notes and does
  not participate in audio rendering.
- `ProducerAssistant` is a framework-independent deterministic recommendation engine. The
  UI converts its note results into ordinary project-owned MIDI clips and applies its mix
  suggestions through ordinary undoable track-property commands.
- `HelpAssistant` is a framework-independent bundled knowledge index. It ranks verified
  articles from normalized question terms on the message thread and never touches the audio
  callback, project document, filesystem, or network.
- UI components retain stable IDs and query current model/engine state.
- Imported files become `MediaSourceState` records.
- Timeline instances become `AudioClipState` records.
- Tracks own mixer properties and clip references.
- Recorded audio clips at a shared timeline start receive one take-group ID and stable lane
  indexes. Active-comp state lives in `ProjectModel`; the UI never owns the selection.
- `CompRegionState` stores a bounded override by take-group ID and stable lane index. The
  model resolves baseline and override ranges into immutable playback segments before they
  are published to the engine. Split clip IDs therefore do not invalidate a comp choice.
- Adjacent resolved segments from different take sources overlap by a bounded 10 ms window.
  `AudioTrack` applies complementary square-root gain curves during rendering, producing an
  equal-power transition while all original media remains unchanged.
- An `AudioTrack` owns a prepared source-reader pool keyed by media-source ID. Its immutable
  callback-facing clip list references the correct reader, enabling multiple WAV sources on one
  track without combining or rewriting the original recordings.

## Change flow

```text
User gesture
  -> ProjectModel undoable mutation
  -> model change notification
  -> AudioEngine synchronization by Track ID
  -> UI refresh from model snapshot
```

Undo and redo follow the same path, so engine and UI state are reconstructed from the resulting project model rather than reversed independently.

## Real-time boundary

The playback adapter owns one global timeline position and renders clip intersections for each output block. Phase 18A publishes a complete prepared `RenderSnapshot` through an RCU-style pointer exchange. The callback increments a reader epoch, loads one generation at the block boundary, and uses that generation for the complete block. A control-thread publication cannot invalidate it. Superseded snapshots are retired and reclaimed only after the reader count returns to zero.

Tracks, source readers, clip schedules, precomputed MIDI timing, tempo/signature vectors, delay lines, mixer runtime, automation points, scratch buffers, plug-in audio/MIDI buffers, and the pre-resolved routing plan are prepared before publication. The callback contains no Triumph Studio mutex, `CriticalSection`, `ScopedLock`, `ScopedTryLock`, buffer resize, reader construction, file access, state serialization, logging, or graph rebuild. If no replacement is ready, the previous valid generation continues rendering.

During capture, input samples are pushed into JUCE's preallocated threaded-writer FIFOs and one shared `TimeSliceThread` performs WAV I/O. Writer teardown first atomically withdraws the writer pointer, then waits on the control thread until every callback reader leaves. Loop recording switches only between pre-opened pass objects. The recording callback therefore reports a drop only when a writer FIFO rejects data, an input layout is invalid, or the next pass was not prepared; it never waits for the interface. File creation, writer destruction, journal checkpoints, project mutation, and take insertion remain outside the callback.

Hardware input is copied before the shared device buffer is cleared. The engine then renders
the final device output. Each armed audio track selects either source with an independent
mono/stereo route and record offset. “Device output” is named explicitly because software
Control Room monitoring is already present at that boundary; it is not misrepresented as a
pre-monitor program bus.

Software input monitoring is summed after project master gain through its own smoothed
Control Room gain, dim, and mute state. Changing these controls cannot change offline export.
The generic JUCE device layer does not expose a portable hardware direct-monitor controller,
so Triumph reports that capability as driver-managed. Dedicated cue buses, listen, and
talkback are not simulated and remain FR-4 work.

Live MIDI notes are published through atomics for the built-in fallback and through JUCE's thread-safe MIDI collector for the hosted VST3. Plug-in replacement uses the same retired-resource boundary as render snapshots. Synchronous plug-in control work sets a non-blocking control gate and waits outside the callback for the current plug-in user to leave; the callback never waits for control work. Optional monitoring copies the current input block before the output buffer is cleared.

Phase 9 adds an independent offline renderer that consumes an immutable project snapshot on a worker thread. It resolves the same model-owned comp and warp segments, performs deterministic random-access source reads and linear resampling, renders the built-in MIDI instrument, applies track/master state, and publishes mix, range, or stem deliveries atomically. Stem folders use isolated-track passes and a deterministic manifest. Deliveries that require an external VST3 are rejected rather than silently producing incomplete audio; safe plug-in graph ownership and freeze begin in Phase 10.

Phase 10 makes external-instrument lifecycle state project-owned. Bypass, availability,
reported latency, frozen state, and the stable freeze media-source ID live in `ProjectModel`.
`PluginFreezeRenderer` clones the stored instrument asynchronously, schedules retained MIDI
at sample offsets, captures the processor tail, trims reported plug-in latency, and commits an
atomic 24-bit stereo WAV. The model adds that file as ordinary media in one undo transaction;
unfreeze removes only the rendered clip/source references and restores the retained live MIDI
and plug-in state. The live engine delays non-plug-in playback by the active instrument's
reported latency so it reaches the output with the hosted processor path. Frozen tracks use
the normal audio-track scheduler and may enter the deterministic offline delivery path.

Phase 11 replaces the single shared compensation buffer with one prepared stereo
`SampleDelayLine` for every routed track. `PluginGraph` validates an acyclic node/edge
snapshot, topologically computes arrival time at every merge, and assigns independent
delays to audio, send, and sidechain edges. The current track-to-master engine publishes
that plan outside the callback, so the audio thread only reads stable delay values and
processes preallocated ring storage. Armed paths may opt out of newly added delay in Low
Latency mode; this is an explicit monitoring tradeoff and is persisted with the project.
Manual latency offsets correct inaccurate or missing processor reports without moving
clips. Automation lookahead is derived from compensated node-input arrival so later
automation-lane work can schedule parameters against the same graph clock.

Phase 12 publishes a validated `MixerGraph` beside the delay plan. Tracks, returns,
and buses are explicit nodes; main outputs, audible sends, and sidechains are typed
edges. Configuration and cycle detection stay on the message thread. The callback
receives a stable topological order plus pre-resolved runtime route indexes, clears
only prepared channel buffers, applies pre/post-fader taps, accumulates sidechain
control signals, and sums the result into the master buffer. Invalid edits leave the
last valid audio plan running and surface a routing warning in the mixer.

`MixerComponent` observes the same project-owned routing state as the engine. Its
channel strips do not own route truth. Master samples enter a lock-free SPSC FIFO;
the UI thread drains that stream, applies a Hann window and 2048-point FFT, and paints
the spectrum without blocking or allocating in the audio callback. Offline mix and
stem rendering rebuild the same typed routing graph from an immutable snapshot.

Phase 13 extends `TempoMap` with step and linear BPM segments. Linear ramps use the
closed-form integral of reciprocal tempo for beat-to-time conversion and the matching
exponential inverse for time-to-beat conversion; MIDI, VST3 playhead data, freeze,
arrangement drawing, recording conversion, and offline delivery therefore share one clock.
Time-signature events, markers, metronome state, automation lanes, modes, and curve points
remain project-owned and undoable.

`TempoAutomationComponent` is an observing editor, not an alternate timeline model. The
live engine receives normalized immutable automation point vectors outside the callback.
Track/return/bus gain and pan are evaluated for every output sample against the project
timeline sample clock, including when the hardware device rate differs. Master automation
uses the same clock. The offline renderer evaluates the same hold, linear, and smooth
curves for mix, range, and stem delivery. The metronome computes beat boundaries from the
prepared tempo/signature state and synthesizes short bar-accented clicks without heap
allocation in the callback; it is a monitoring aid and is intentionally excluded from
offline delivery.

Phase 14 keeps clip direction and normalization non-destructive. Direction is one persisted
clip flag. Playback resolution publishes it with each immutable segment; the live source pool
uses a separately prepared reverse reader chain, while offline delivery performs the matching
descending source-time mapping. Normalization scans the selected decoded source range on a
worker thread and commits only an undoable clip-gain change.

Pitch processing is intentionally offline. `AdvancedAudioRenderer` first resolves the selected
clip's trim, stretch, warp, and direction into timeline-rate audio, then applies the built-in
length-preserving overlap-add pitch algorithm and publishes a 24-bit WAV in the project's
`Processed/` directory. `ProjectModel` adds that WAV as a new media source and swaps the
selected clip reference in one Undo operation. The original media record and file are not
rewritten. The worker never mutates project state; its result is committed on the message
thread only if the target clip still exists.

Phase 15 adds local producer assistance without creating a second project model or remote-data
path. `ProducerAssistant` accepts only explicit musical settings plus small track descriptors.
Chord, drum, and melody requests are deterministic pure C++ functions. A generation command
creates one instrument track, one MIDI clip, and all stable note records in a single Undo
transaction. The built-in instrument renders channel-10 kick, snare, and hi-hat notes with
matching deterministic live/offline synthesis, so the drum assistant is immediately audible.

Mix Balance reads an immutable project snapshot and derives conservative gain/pan starting
points from track count, instrument state, and role words in track names. It does not inspect
audio samples or claim mastered output. All suggested track changes commit as one Undo
transaction. Producer settings persist with the project; generated MIDI and mix state remain
ordinary document data. No project audio, metadata, prompt, or identifier is sent off-device.

Phase 16 adds `HelpAssistant` as a read-only support surface. Its article catalog documents
only implemented controls and explicitly identifies unavailable service-backed capabilities.
Search normalizes a question, applies deterministic field-weighted matching, and presents a
stable ranked result list. The UI may request an existing control action from `MainComponent`,
but the knowledge engine cannot mutate the project. Actions open the same Device, Mixer,
Tempo, Keys, Edit, VST3, AI Producer, Export, or Save path already used by the main interface.

Help queries remain transient UI state. They are not serialized, logged, sent over a network,
or used for telemetry. F1 toggles the same docked panel as Help. The assistant remains usable
during recording because it performs no project edit and no background I/O.

Phase 17 is a stabilization layer rather than a new feature subsystem. The framework-independent
`ResponsiveLayout` policy selects compact project and transport toolbars below 1380 pixels and
defines the supported 1280 x 720 minimum. In compact mode, lower-frequency commands move into
one `More` menu. Those menu items trigger the same `MainComponent` commands and use the same
enabled state as their full-width buttons, so resizing cannot create a second behavior path or
alternate project state. At wider sizes, the original direct buttons remain visible.

This phase does not change real-time ownership, persistence, audio rendering, or project format.
Compiler cleanup replaces a deprecated JUCE editor call and fully initializes offline-render
channel state; it does not change the plug-in or mixer lifecycle.

Phase 18A changes real-time ownership without changing the project document. `ProjectModel` and
the interface keep editable control state. A batched synchronization transaction builds one new
prepared render generation after the complete model change, avoiding partially published track,
MIDI, mixer, delay, or automation state. The audio thread never reads the control-side vectors.

`SnapshotExchange` publishes one raw callback-facing pointer but retains unique ownership on the
control side. Its read handle increments a sequentially consistent reader epoch before loading the
pointer. Publication moves the prior owner into a retired list; only a later control-thread reclaim
with zero readers may destroy it. This prevents reference-count decrements, heap deletion, reader
destruction, and plug-in destruction on the audio thread.

Master gain and mute use a bounded SPSC command queue for block-boundary delivery. Their atomics
also retain the newest target, so queue overflow is observable but cannot leave the engine stuck on
an older value. Structural edits publish another prepared generation. Invalid mixer graphs retain
the last valid plan and never replace the active generation with incomplete state.

Callback telemetry stores only integer atomics: callback count, accumulated and maximum duration,
deadline misses, suspected dropouts, snapshot swaps, parameter-queue overflows, and active render
generation. The callback performs no string formatting or logging. The 30 Hz interface timer reads
a copy, computes load against the current hardware deadline, and presents `RT` and `XRUN` status.

The Phase 18A Windows suite adds a framework-independent RCU/queue/telemetry stress test, a static
playback-and-recording callback lock/allocation audit, and a JUCE live-engine stress test that edits
audio clips, MIDI, mixer state, and automation while rendering. These tests supplement rather than
replace the hardware, long-session, high-track-count, and broad VST3 release gates.

Foundations recovery adds an independent scanner executable beside the application. Scanner failure,
missing output, invalid protocol, and timeout never execute discovery code in the application process.
The registry is replaced atomically and retains failure reason and serialized JUCE descriptions,
including stable identifier inputs, vendor, version, channel counts, and capabilities. Runtime audio
processing is still in-process; scan isolation must not be described as per-plug-in runtime sandboxing.
Instrument bypass keeps that in-process instance alive, drains notes, fades host output, and preserves
its reported PDC latency instead of unloading the processor at the bypass boundary.

## Compatibility

Project format version 20 adds recording mode, pre-roll, punch/loop ranges, Control Room
state, and per-track input route, recording tap, and record offset. It retains version 19
persistent input-monitor mode and signed manual record-alignment calibration, plus version 18 local producer root, scale, style, bar-length, and variation
settings, version 17 clip playback direction, version 16 tempo curve
shape, time-signature events, markers, metronome state, and parameter automation lanes,
version 15 master mute,
output routing, sends, returns, buses, and sidechains, version 14 Low Latency mode and track latency calibration,
version 13 plug-in lifecycle/freeze identity, version 12 transient markers, version 11 warp
anchors, version 10 stretch providers, version 9 comp regions, version 8 take groups,
version 7 VST3, version 6 MIDI, and earlier audio state. Formats 1–19 are migrated in
memory. Saving always writes version 20 using a temporary
adjacent file followed by safe replacement.

## Project lifecycle

New, Open, Recent, and application Quit all pass through the same unsaved-change decision. Save continues only after a successful atomic write; Discard removes that project's recovery snapshot; Cancel leaves the current session untouched. A dirty project receives a recovery snapshot after two minutes, while an explicit successful save removes recovery state and creates a rolling checkpoint.
