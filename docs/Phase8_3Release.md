# Triumph Studio Phase 8.3 Release Gate

Version 0.8.4 adds non-destructive manual warp markers backed by real piecewise playback.

## Delivered

- Stable per-clip warp-marker IDs and source/timeline anchors.
- Ctrl-click creation, Ctrl-drag movement, and right-click removal.
- Piecewise playback-rate resolution between adjacent anchors.
- Warp-aware comp playback and crossfade scheduling.
- Marker preservation through uniform stretch, trim, split, undo/redo, and save/load.
- Project format 11 with backward loading for formats 1–10.
- Pure mapping tests and JUCE project-model persistence/playback tests.

## Honest boundary

Warp playback still uses the built-in varispeed provider, so pitch follows speed. Automatic
transient detection, pitch-preserving processing, formant controls, clip variants, and ARA
integration remain later release gates.
