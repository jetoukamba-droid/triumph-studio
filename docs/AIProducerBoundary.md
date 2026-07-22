# AI Producer Boundary

## What Phase 15 does

The AI Producer panel is a set of local assistive tools. Chord, Drum, and Melody create real
MIDI tracks from the selected key, major/minor scale, style, length, and deterministic variation.
The resulting notes can be edited in the piano roll, routed to an installed VST3 instrument,
saved, exported through supported render paths, and removed or restored with Undo/Redo.

Mix Balance reads track count, instrument state, and role words in track names. It applies a
conservative gain/pan starting point with headroom and stereo separation in one undoable edit.
It is useful as an initial organization pass, not a substitute for listening, meters, mix
decisions, or mastering.

## What Phase 15 does not do

- It does not call an online service or upload audio, MIDI, metadata, names, or project IDs.
- It does not embed or advertise a trained neural model.
- It does not analyze waveform content, loudness, frequency balance, or masking.
- It does not generate audio samples, clone artists, or imitate a named copyrighted work.
- It does not silently change a project; every result is visible, editable, saved, and undoable.

The `AI` label refers to computer-assisted musical recommendations. The implementation remains
truthful about its deterministic rules so a future model provider can be evaluated and added
behind an explicit, consented boundary rather than simulated in the current product.
