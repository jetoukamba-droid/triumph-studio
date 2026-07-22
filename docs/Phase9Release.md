# Phase 9 — Deterministic Delivery Foundation

Triumph Studio 0.9.5 introduces a real offline stereo export path. Maintenance revisions use JUCE 8's required base `OutputStream` ownership type and mutable writer-options contract, module-specific JUCE includes so the isolated export integration target builds without the application's generated umbrella header, separately rounded material/tail sample counts for deterministic sample-accurate file length, real 44.1/48/96 kHz 16/24-bit delivery presets, and an explicit MSVC UTF-8 source contract for correct Windows typography.

## Export contract

- 44.1, 48, or 96 kHz stereo WAV
- 16-bit or 24-bit PCM with bit-depth-correct fixed-seed TPDF dither
- selectable two-second delivery tail or sample-accurate no-tail master
- worker-thread rendering from an immutable project snapshot
- atomic publication from a `.partial` file
- comp regions, equal-power fades, clip gain, track gain/pan, mute/solo, warp rates, built-in MIDI, and master gain

The exporter refuses any delivery that includes an external VST3 instrument. It never creates a misleading file with the plug-in silently omitted. Safe plug-in graph ownership, freeze/unfreeze, crash isolation, and online/offline tolerance comparison belong to Phase 10.

## Validation

The existing five-suite release gate remains. Timeline math now also verifies deterministic dither and interpolation primitives. Windows validation must additionally export a known audio project, reopen the resulting WAV, confirm duration includes the two-second tail, and compare audible edits against live playback.
