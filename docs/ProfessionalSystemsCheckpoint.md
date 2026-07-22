# Professional Graph, Plug-in Rack, and MIDI Checkpoint

Triumph Studio 0.21.6 is an internal alpha checkpoint across FR-1, FR-3, FR-4,
and FR-5. It is intentionally not labeled complete or beta. This document
separates working production paths from contracts that still require a final
runtime or hardware acceptance gate.

## Plug-in rack and containment

Working in this checkpoint:

- Ordered, persistent audio-effect slots on audio tracks, instrument tracks,
  buses, and returns, with a maximum of sixteen total slots per owner.
- VST3 audio-effect validation through the existing crash-isolated scanner.
- Editor access, state capture, bypass, removal, missing/unavailable state,
  stable parameter identity, parameter automation, tail processing, latency
  change notifications, and route-PDC rebuilds.
- Exception containment: an escaping processor exception restores the dry
  block, bypasses the faulty slot, records telemetry, and marks the slot
  unavailable on the control thread.
- A versioned runtime-worker protocol, preallocated triple-buffer audio/MIDI/
  parameter bridge, heartbeat watchdog, fault taxonomy, backoff, and bounded
  restart policy, all covered by a framework-independent regression test.

Open gate:

- Live effects are currently instantiated only when their saved isolation mode
  is `trusted-in-process`. The cross-process worker transport is specified and
  tested as a boundary but is not yet connected to the live JUCE processor.
  Native access violations or hangs therefore still require the runtime worker
  before this area can be called fully isolated.

## Mixer, routing, PDC, and monitoring

Working in this checkpoint:

- One acyclic program graph covers track, bus, return, master, output, send,
  and sidechain routes.
- Explicit main-input, main-output, and sidechain channel layouts are validated
  before a replacement graph can be published.
- PDC is calculated over the complete route graph. Every output, pre/post send,
  and sidechain feed owns its prepared compensation delay line.
- Ordered insert latency is aggregated into each channel node; a live latency
  change rebuilds the prepared graph off the callback.
- Control Room, cue, listen, and talkback are modeled as independent monitor
  paths with explicit hardware-output ranges. Control Room dim/mute does not
  alter program master, cues, or exported audio.
- The mixer UI exposes buses, returns, output routing, pre/post sends,
  sidechains, meters, gain reduction, and effects on every mixer owner.

Open gates:

- Null, sidechain-alignment, and multi-output monitor tests must be repeated on
  Windows reference hardware and a broader interface/channel-layout matrix.
- Offline rendering still needs complete parity for every third-party insert
  and monitor-independent route before the professional delivery gate closes.

## MIDI, MPE, automation, and synchronization

Working in this checkpoint:

- Stable MIDI note identities, high-resolution controller values, and per-note
  pitch, pressure, and timbre survive save/load.
- The piano roll can add MPE points and high-resolution controller events.
- Overlapping expressive notes receive deterministic MPE member channels;
  note, controller, and expression events are delivered at sample offsets.
- MIDI 2.0 UMP note-on, note-off, and per-note-pitch packets plus block
  scheduling are framework-independent, bounded, and regression tested.
- Mixer and plug-in automation persist and play with hold, linear, or smooth
  curves. READ, TOUCH, LATCH, and WRITE recording plus point simplification
  have deterministic core coverage.
- MIDI Clock follows tempo and external start/continue/stop. MTC reconstructs
  all eight quarter-frame nibbles, reports frame rate/lock, and chases the
  project timeline. Sync settings persist in format 21.

Open gates:

- JUCE VST3 delivery currently uses the compatible MIDI 1/MPE representation;
  native MIDI 2.0 device and plug-in transport remains an integration gate.
- MIDI Clock output is visibly disabled until a selectable MIDI-output path is
  implemented. Ableton Link is also visibly disabled rather than simulated.
- SysEx, LTC, full offline third-party plug-in parity, and external-sync jitter
  qualification remain open.

## Verification

- `PluginRuntimeIsolationTests`, `ProfessionalMixerGraphTests`,
  `MidiPerformanceTests`, and `AutomationWriteTests` pass locally under C++20
  with `-Wall -Wextra -Werror`.
- The Windows CMake project registers 32 tests. A clean Release build and all
  32 tests are the acceptance gate for this packaged checkpoint.
- `AudioDeviceRecoveryTests` verifies idempotent interruption accounting,
  bounded retry/failure transitions, one-shot Play-intent recovery, and idle
  recovery without an unintended transport start.
- `AudioEngineRealtimeTests` verifies audible 2.0x/0.5x varispeed and bounds
  the clip-edit stress phase to reject the former reader-rebuild stall.
