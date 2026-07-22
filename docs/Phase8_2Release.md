# Triumph Studio Phase 8.2 Release Gate

Version 0.8.3 adds the first production-shaped elastic-audio foundation without claiming
pitch preservation that the current engine does not provide.

## Delivered

- Persistent per-clip stretch ratio and provider identity in project format 10.
- Alt-drag on the right clip edge to stretch duration from 25% to 400%.
- Undo/redo-safe stretch commands owned by `ProjectModel`.
- Stretch-aware start trim, end trim, split, comp resolution, and crossfade expansion.
- Real variable-rate playback through JUCE's resampling source.
- A visible rate label on stretched clips in the approved cool-grey/blue UI.
- Backward loading for project formats 1–9 with a neutral 1.0 ratio.
- Framework-independent ratio/mapping tests and JUCE project-model coverage.

## Honest product boundary

The built-in `builtin-resample` provider is varispeed. Duration, speed, and pitch change
together. This release does not claim pitch-preserving elastic audio, formant control,
transient detection, warp markers, clip variants, or ARA integration. The provider field is
the seam for a later quality-reviewed and commercially licensed algorithm.

## Windows release check

Configure and build Release, run all CTest suites, launch the app, import a known audio file,
Alt-drag its right edge, and verify rate-labelled playback, save/load, undo/redo, trim, split,
and comp playback with both the Focusrite ASIO and Windows Audio backends.
