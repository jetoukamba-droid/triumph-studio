# Triumph Studio Phase 7.2 Release Gate

Version 0.7.2 adds the first real VST3 instrument host to the Phase 7.1 MIDI-device,
performance-recording, tempo, and instrument foundation.

## Implemented

- Project format 7 with backward loading for formats 1–6
- Persistent tempo points, 4/4 meter foundation, MIDI track type, clips, and notes
- Stable IDs for MIDI clips and notes
- Beat-relative note start, duration, velocity, and MIDI channel
- Framework-independent beat/second/sample conversion across tempo changes
- Sample-offset event boundary calculations
- Built-in deterministic sine instrument with absolute-time phase
- Precomputed note start/end times outside the audio callback
- Piano-roll note creation by double-click and deletion by right-click
- Undoable 1/16 note quantization
- Arrangement-level MIDI clip and note visualization
- Editable transport tempo control
- Sample-rate-based master parameter smoothing with block-split equivalence test
- MIDI-input enablement inside the Device panel
- Live note monitoring through the built-in instrument while transport is stopped
- Thread-safe note pairing by MIDI channel and pitch
- Velocity and channel preservation for recorded notes
- Automatic closure of held notes when recording stops
- Transport-position and tempo-map conversion into beat-relative notes
- One-step undo for an entire recorded MIDI take
- Automatic MIDI-clip extension when a take exceeds its previous length
- Explicit selection and validation of an installed Windows VST3 instrument
- Asynchronous instrument instance creation with stale-load cancellation
- One active instrument slot assigned to one project instrument track
- Timestamp-correct live MIDI routing, including CC, pitch, and sustain messages
- Sample-offset timeline note-on/note-off scheduling through the hosted processor
- Hosted stereo output through track gain, pan, mute, solo, and master processing
- Native VST3 editor window with safe processor/editor teardown ordering
- VST3 description and complete processor-state save, autosave, restore, and undo
- Built-in Triumph Instrument fallback when no VST3 is assigned

## Acceptance gate

1. Configure and build Release with Visual Studio.
2. Run all five test executables through CTest and require 100% passing.
3. Create a MIDI track and confirm the piano roll opens.
4. Double-click three different pitches, close the editor, and press Play.
5. Confirm MIDI notes appear in the arrangement and produce audio through ASIO.
6. Change tempo and confirm note spacing/playback follows the new tempo.
7. Reopen Keys, use Quantize 1/16, then verify Undo and Redo.
8. Save, close, reopen, and confirm tempo and notes persist.
9. Open Device, enable a connected USB MIDI controller, and close the panel.
10. With a MIDI track armed and transport stopped, confirm played notes are audible.
11. Press Record, perform notes with different velocities, then press Stop Rec.
12. Open Keys and confirm start time, duration, pitch, velocity, and channel were captured.
13. Use Undo once and confirm the complete recorded take is removed together.
14. Record beyond bar 5 and confirm the MIDI clip grows to contain the take.
15. Save, reopen, and confirm the recorded take persists.
16. Install a trusted VST3 instrument such as Decent Sampler, then press VST3 and load
    its `.vst3` from `C:\Program Files\Common Files\VST3`.
17. Confirm the editor opens and the Arturia keyboard plays the hosted instrument through
    the Focusrite ASIO output while transport is stopped.
18. Confirm sustain pedal, modulation, pitch bend, and velocity reach the instrument.
19. Record a MIDI take, press Play, and confirm timeline notes use the VST3 rather than the
    built-in validation tone.
20. Verify track gain, pan, mute, solo, and master level affect the hosted output.
21. Save, close, reopen, and confirm the instrument and its selected sound return.
22. Remove the VST3 assignment, use Undo, and confirm the saved assignment returns.
23. Confirm a missing or incompatible VST3 reports a visible failure without corrupting the
    project, and that the project remains saveable.
24. Confirm existing format-5 and format-6 projects still load and play.

## Honest boundary

This is not yet a production multi-plug-in graph. Phase 7.2 intentionally hosts one trusted,
user-selected VST3 instrument in the application process. A separate crash-isolated scanner,
global plug-in database and blacklist, multiple instruments/effects, latency compensation,
sidechains, automation, parameter gestures, missing-plug-in replacement, preset management,
MPE, and offline-render parity remain later release gates. The built-in tone generator remains
a deterministic fallback, not the final Triumph Synth.
