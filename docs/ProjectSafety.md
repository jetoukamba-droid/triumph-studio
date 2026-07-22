# Triumph Studio Project Safety

## Save contract

- `.triumph` files are readable, versioned JSON documents. Version 18 preserves
  project-timeline and native-source sample-rate semantics plus stable MIDI clip/note
  identity, beat timing, tempo, meter, velocity, channel, instrument-track type, VST3
  identity, complete processor state, take-group identity, stable lane order, and the
  active whole-take comp choice, bounded region-comp overrides, crossfade lengths,
  non-destructive clip stretch ratio/provider identity, stable warp/transient anchors,
  plug-in bypass/availability/latency, frozen-render source identity, master mute,
  track/return/bus output destinations, send levels and tap positions, sidechain routes,
  tempo ramp shape, time-signature events, project markers, metronome state, and stable
  automation lanes with target, parameter, mode, points, and curve interpolation,
  plus each audio clip's forward or reverse playback direction and local producer root,
  scale, style, length, and variation settings.
- Every new project receives a persistent UUID.
- A save is written to an adjacent temporary file before replacing its target.
- Successful explicit saves clear the matching recovery snapshot.
- New, Open, Recent, and Quit never discard a dirty project without an explicit decision.

## Recovery and checkpoints

Dirty projects are autosaved every two minutes to the user's application-data recovery folder. On the next launch, Triumph Studio offers the most recent valid recovery file. Recovery opens as an unsaved project so the original document is not silently overwritten.

Before overwriting an existing project, Triumph Studio copies the previously saved document to a timestamped checkpoint in `Backups/`. A project's first save also creates its initial checkpoint. Triumph Studio retains the newest twenty checkpoints per project folder.

## Media portability

External source files remain non-destructive references. Collect and Save copies every available external source into `Media/`, updates the source records, and then saves the project. Files inside the project folder are serialized as relative paths so the complete folder can be moved to another drive or computer.

If a source cannot be found, its track remains in the project as a missing-media placeholder. Relink searches a selected folder recursively by source filename, updates unambiguous matches, and reports missing or duplicate-name results instead of silently guessing.

## Recording safety

- A project must be saved before recording starts, establishing a durable project folder.
- Takes are written as timestamped 24-bit WAV files inside `Recordings/`.
- The recording file is flushed and closed before it is added to the project model.
- If model insertion fails, the WAV is preserved and its full path is shown.
- New, Open, Save, and Quit stop and commit an active take before continuing.
- Input monitoring defaults to off; the UI warns the user to use headphones when enabled.
- Recording another pass never overwrites an earlier WAV. The new file is inserted as a new
  lane and the prior takes remain recoverable through Undo or `Use Take`.
- Swipe comping changes only project metadata. It never trims, replaces, or writes into a
  recorded take, and every range assignment participates in Undo/Redo.
- Time stretching changes only clip metadata. Original audio files remain untouched, and
  stretch, trim, split, save/load, and comp resolution share the same source-time mapping.
- Warp edits preserve the source anchor while moving its timeline position. They never
  rewrite recorded or imported media and participate in the same Undo/Redo history.
- Transient analysis is stored independently from warp anchors. Detection never changes
  playback; explicit conversion, trimming, splitting, stretching, and clearing are undoable.
- Reverse changes only persisted clip metadata and participates in Undo/Redo; neither live
  playback nor offline delivery rewrites the source file.
- Normalization analyzes decoded source samples on a worker and changes only clip gain.
- Pitch processing writes a unique 24-bit WAV into `Processed/`, closes it before model
  insertion, and swaps the clip's media reference in one undoable transaction. A cancelled
  or failed render removes its partial file; the original source remains referenced for Undo.

## Commercial quality boundary

Phase 15 producer assistance runs locally and never uploads project data. Generated tracks use
ordinary stable MIDI notes, and Mix Balance changes ordinary gain/pan properties, so Save,
Undo/Redo, recovery, and migration keep their existing safety contracts. Mix Balance is a
name/role-based starting-point heuristic; it does not listen to the audio, detect clipping, or
replace metering and critical monitoring. The local generators are deterministic composition
helpers, not a claim that a trained generative model is embedded in the application.

Triumph Assistant also runs locally. Questions are matched against source-controlled bundled
articles and remain transient; they are not written into the project, recovery snapshot,
recent-file settings, logs, analytics, or network requests. Article actions invoke existing
explicit UI commands only after the user clicks them. The assistant never starts recording,
changes mixer state, or edits a project merely because a question was asked.

Phase 17 compact toolbars do not introduce alternate project commands. Each `More` menu item
invokes the same existing save, media, latency, comp, or clip-edit path as the corresponding
full-width button, and it mirrors that command's enabled state. Resizing therefore does not
change project data, bypass undo protection, or alter the format-18 save contract.

Freeze renders are written to a partial file and moved into place only after a complete render.
A live external VST3 remains blocked from offline mix/stem delivery; freezing converts it to
ordinary project media and removes that incomplete-delivery risk. Missing or invalid stored
plug-in data enters an explicit unavailable state instead of being silently substituted.

These safeguards reduce routine project-loss risk but are not a substitute for user backups or source control. Plug-in discovery now runs in a separate timeout-bounded scanner with a persistent blocklist, deterministic retry, and synthetic process-fault coverage. Live plug-in audio execution still occurs in the application process: timing is attributed and escaping C++ exceptions are muted, but a native access violation or hang can still take down the application. A production release therefore still requires grouped or per-plug-in runtime process isolation and broad real-binary compatibility coverage. The built-in overlap-add pitch processor remains a functional editing foundation; production mastering use still requires broader artifact, transient, stereo-image, and long-file quality testing. The live host still exposes one active project-owned instrument plug-in slot and built-in sidechain ducking rather than arbitrary insert effects on every channel. Future releases should add checksummed media identity, interactive duplicate-name disambiguation, configurable autosave retention, background copy progress/cancellation, and broader fault-injection tests.
