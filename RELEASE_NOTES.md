# Triumph Studio 0.22.0-beta.1

This is the first closed Beta 1 engineering build. It freezes new roadmap work
and packages the validated 0.21.6 professional-controls checkpoint for focused
real-world testing.

## Beta focus

- Stable import, playback, pause, stop, seek, save, reopen, and WAV export.
- Audible device diagnostics and bounded output-device recovery.
- Non-destructive editing, Alt-right-edge varispeed, pitch processing, and Undo.
- Crash-isolated VST3 discovery and explicit unavailable plug-in state.
- Recording journals, autosave, project recovery, and multitrack capture.
- Branded Triumph Studio application icon and in-app identity mark.
- Word-labelled project and studio controls with symbol-only transport controls.
- Conflict-checked keyboard shortcuts with accessible tooltips.
- Playable piano keys in the MIDI editor for live instrument audition.
- Confirmed, undoable track deletion from every track header.
- Pro DAW charcoal/gold visual redesign across the shell, transport, timeline,
  track lanes, clip bodies, mixer strips, and supporting dialogs.
- Professional audio-clip identity and context actions: rename, duplicate,
  mute, lock, reverse, split at the playhead, colour, and delete.
- Clip mute now affects the resolved playback plan; clip lock protects move,
  trim, stretch, warp, destructive processing, split, and delete operations.
- Clip names, colours, mute state, and lock state persist across save/reopen.
- Post-Beta FR-6 checkpoint: persistent clip fade handles, equal-power overlap
  crossfades, warp-safe fade anchors, and matching real-time/offline rendering.
- Post-Beta FR-7 checkpoint: same-track built-in instrument switching,
  professional bar/beat timeline ruler, draggable playback loop locators, and
  transport loop wrapping.
- Post-Beta FR-8 design-only checkpoint: replaces the old blue visual skin with
  the generated Triumph Studio pro-console look while preserving existing UI
  controls, callbacks, and audio/model behavior.
- FR-8 Fix1: flushes the realtime varispeed resampler when clip playback rate
  changes so rapid edit-stress reconfiguration does not reuse stale resampling
  state.
- Post-Beta FR-9 checkpoint: rebuilds the main studio shell to match the
  generated pro-console reference with persistent inspector and browser/plugin
  side panels, compact professional track lanes, a centered timeline canvas, and
  a visible bottom mixer/master console by default. Track rows can now be
  resized directly from the row bottom edge so dragging down makes lanes bigger
  and dragging up makes them smaller while clip dragging remains intact.
- FR-9 Fix1: shows the Inspector and Browser/Plugins panels at normal 1366px
  laptop width instead of hiding them whenever the top toolbar enters compact
  mode.
- FR-9 Fix2: unifies the timeline origin math so the ruler bar lines,
  playhead, loop handles, click-to-sample conversion, and clip lane grid start
  from the same x-position.
- FR-9 Fix3: lowers the side-panel visibility threshold for scaled Windows
  displays so Inspector and Browser/Plugins stay visible when JUCE reports a
  smaller logical width than the physical screenshot width.
- FR-9 Fix4: restores the original deep-navy/electric-blue colour palette while
  preserving the FR-9 structural layout, timeline alignment, and drag behavior.
- FR-9 Fix5 / FR-3 scanner hardening: accepts the CMake target scanner name
  (`TriumphPluginScanner.exe`) and the spaced product name
  (`Triumph Plugin Scanner.exe`), and copies the helper under both names beside
  the application so VST3 validation can launch reliably.
- FR-9 Fix6 / FR-3 plug-in recovery: preflights the crash-isolated scanner
  before standard-folder scans or direct instrument/effect loads, searches the
  build helper artifact folder as a fallback, and reports every scanner path
  Triumph checked when validation cannot continue.
- Beta 1 finish checkpoint: adds a Windows launch helper that finds the actual
  Release executable, repairs the scanner helper copy from the build tree when
  needed, and avoids the common `artefacts` path typo before final acceptance.
- Post-Beta FR-10 checkpoint: upgrades project files to format 22 with durable
  repair records, a persisted undo manifest, and media asset fingerprints. The
  existing missing frozen-render recovery now records a repair event instead of
  silently changing the project.
- Post-Beta FR-11 checkpoint: upgrades project files to format 23 with
  persistent audio clip variants. Capturing, restoring, deleting, undo/redo, and
  save/load preserve alternate clip source, trim, gain, fade, stretch, reverse,
  warp, and transient state.
- Post-Beta FR-12 checkpoint: upgrades project files to format 24 with bounded
  persistent undo/redo restore points. Supported project-model edits can now be
  undone and redone after save, app restart, and project reload instead of only
  exposing the previous undo labels.
- Post-Beta FR-13 checkpoint: starts FR-1/FR-7 reliability hardening with
  explicit realtime fault attribution for missing render state, oversized audio
  blocks, read-ahead/source starvation, device restarts, variable block-size
  changes, graph-build timing, and render-retire backlog pressure.
- Post-Beta FR-14 checkpoint: surfaces the FR-13 realtime attribution in the
  transport status strip and tooltip so closed-beta testers can report whether
  an audio fault was state, buffer, stream/source, queue, variable-device, graph
  build, plug-in, or hardware recovery pressure.
- Post-Beta FR-15 checkpoint: adds `More > Copy Realtime Diagnostics`, a
  clipboard report that captures the current project, audio device, realtime
  attribution counters, graph-build pressure, plug-in runtime, device recovery,
  sync state, warning counters, and a clean/action-required verdict.
- Post-Beta FR-16 checkpoint: hardens VST3 scanner placement. Triumph now tries
  to repair the crash-isolated scanner beside the app before reporting a
  missing scanner, and the diagnostic text lists the install folder plus every
  checked helper path.
- Post-Beta FR-17 checkpoint: adds actionable plug-in quarantine controls. The
  VST3 registry menu now shows validated, blocked, and missing counts, can copy
  a support-ready registry report, can retry registered plug-ins as instruments
  or insert effects, and can forget one selected registry record without wiping
  the rest of the scanner history.
- Post-Beta FR-18 checkpoint: delivers plug-in automation at sample offsets
  inside each audio callback block. Instrument and insert plug-ins now receive
  bounded automation events by processing sub-blocks at the exact changed
  sample, with MIDI events re-based for each sub-block and queue overflow
  counted by realtime telemetry.

## Automated gate

- A clean Windows x64 Release build must pass all 33 registered tests.
- Packaging is generated from CMake's install graph and receives a SHA-256
  sidecar checksum.

## Release boundary

Beta 1 is a closed prerelease, not a public production release. Public release
still requires installer signing, clean-machine installation, broader hardware
and VST3 compatibility, long-session stress, accessibility review, and
third-party licensing verification.
