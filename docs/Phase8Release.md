# Triumph Studio Phase 8.0 Release Gate

Version 0.8.1 begins Advanced Editing with a non-destructive take-lane foundation and
whole-take comp selection.

## 0.8.1 maintenance correction

The clip-state snapshot now returns its persisted take-group identity, lane index, and
active-comp flag. A regression assertion covers the first recorded take before a second
lane is added. Project-file format remains version 8 and is fully compatible with 0.8.0.

## Implemented

- Repeat audio recording on an existing armed track
- Stable take-group IDs and zero-based lane indexes in the session model
- Newest matching-start recording becomes the active take without deleting prior audio
- Undoable `Use Take` command and equivalent double-click gesture
- Vertically expanded take-lane visualization with explicit `TAKE` and `COMP` labels
- Multi-source audio-track playback keyed by stable media-source ID
- Only the selected lane is published to playback for a take group
- Split take segments preserve group, lane, and active-comp identity
- Deleting an active take safely promotes a remaining take when required
- Format 8 save/load with in-memory migration of formats 1–7

## Acceptance gate

1. Build Release and require all five CTest executables to pass.
2. Create and save a project, then create and arm one audio track.
3. Rewind to 00:00 and record a vocal or instrument pass through Focusrite ASIO.
4. Arm the same track, rewind to the same position, and record a second pass.
5. Confirm the track expands to `TAKE 1` and `TAKE 2`, with only the newest marked `COMP`.
6. Press Play and confirm only the active take is heard—never both layered together.
7. Select the earlier lane and press `Use Take`; confirm playback changes immediately.
8. Use Undo and Redo and confirm the audible take follows the restored model state.
9. Double-click a lane and confirm it becomes the active comp.
10. Save, close, reopen, and confirm both WAV files, lane order, and comp choice survive.
11. Confirm a format-7 VST3/MIDI project still opens and plays.

## Honest boundary

Phase 8.0 performs whole-take selection. It does not yet splice time ranges from several lanes.
Region comping with boundaries and crossfades, loop recording, swipe comping, clip variants,
transient detection, elastic time, warp markers, pitch/time provider abstraction, and ARA
integration research remain Phase 8.x work. The source-reader pool is a necessary bridge; the
future render graph must publish immutable plans without the current track try-lock.
