# Phase 18A Complete - Real-Time Engine Hardening

This is a corrective internal-alpha checkpoint, not a Beta Candidate. Subsequent
work is governed by `FoundationsRecoveryRoadmap.md` and the 97 partial-or-missing
research findings.

Phase 18A removes the audited UI/audio synchronization failure in which
`AudioEngine::getNextAudioBlock()` took `ScopedTryLock(trackLock)` and returned without
playback whenever the interface owned the lock. Version 0.18.0 now renders one fully
prepared generation for the complete block and keeps the last valid generation active
while the control thread prepares a replacement. Project format 18 is unchanged.

## Root cause confirmed

- The callback directly traversed mutable control-side track, MIDI, mixer, delay,
  automation, tempo, and plug-in state.
- One broad `CriticalSection` attempted to make that ownership safe.
- `ScopedTryLock` avoided blocking the audio thread but deliberately returned early
  when the interface was synchronizing a project edit, producing silence.
- Recording teardown used a second callback try-lock, so writer control activity could
  be reported as a dropped input block even when the FIFO had capacity.

## Delivered

- RCU-style `SnapshotExchange` with block-boundary atomic publication
- Reader epochs that keep the selected generation alive for the complete callback
- Control-thread-only retirement and reclamation of old snapshots and plug-ins
- Batched project synchronization that publishes only complete model changes
- Prepared audio-track readers, clip schedules, MIDI timing, tempo/signature data,
  delay lines, mixer routes/buffers, automation, scratch storage, and plug-in buffers
- Last-valid-state behavior when no new generation is ready
- Invalid mixer-graph rejection without replacing the active render state
- Bounded master gain/mute parameter queue with atomic latest-target recovery
- Lock-free recording-writer withdrawal with control-thread reader quiescence
- Callback/deadline/dropout/snapshot/queue telemetry stored only in atomics
- Visible `RT` load and `XRUN` count plus a detailed interface-thread tooltip
- Static playback and recording callback lock/allocation audit
- Pure C++ publication, reclamation, queue-overflow, telemetry, and 10,000-generation
  concurrent stress coverage
- JUCE live-engine regression that renders while repeatedly editing audio clips, MIDI,
  mixer values, and automation

## Callback operations removed or replaced

| Previous callback behavior | Phase 18A behavior |
| --- | --- |
| `ScopedTryLock(trackLock)` | One RCU read handle and atomic snapshot load |
| Early return when the UI owned the lock | Continue with the last published generation |
| Direct reads of mutable project-sync vectors | Reads of one prepared callback generation |
| Callback-visible resource replacement/destruction | Retired ownership reclaimed by the control thread |
| Recording `ScopedTryLock(writerLock)` | Atomic writer withdrawal plus reader quiescence |
| UI reads of mutable meter floats | Atomic meter publication |
| No callback deadline instrumentation | Atomic duration, deadline, dropout, swap, and overflow counters |

## Automated verification

Completed in the build workspace:

- Strict C++20 syntax and warning validation passed for all 21 application `.cpp` files.
- All 13 framework-independent executable regression tests passed, including the new
  `RealtimeEngineStateTests` concurrent 10,000-generation stress run.
- Playback callback source audit found no Triumph Studio lock, allocation, buffer resize,
  file access, reader creation, or logging token.
- Recording callback source audit found no Triumph Studio lock, allocation, buffer resize,
  file access, reader creation, or logging token.
- `AudioEngineRealtimeTests.cpp` passed strict JUCE-aware syntax validation.

The packaged Windows CMake suite now contains 16 tests. The JUCE-linked project-model and
live-engine executables still require the user's Windows JUCE/CMake build environment for
execution; they were not falsely reported as executed in the packaging container.

## Windows acceptance

1. Configure and build Release with Visual Studio.
2. Run `ctest --test-dir build -C Release --output-on-failure` and confirm all 16 tests pass.
3. Play a project while dragging clips and changing mixer/automation values; audio must not
   mute because of interface activity.
4. Watch `RT` load and `XRUN`; hover the status to confirm callback count, maximum duration,
   deadline misses, snapshot swaps, queue overflows, and active generation.
5. Record while starting/stopping interface operations and confirm a disk-drop warning appears
   only for a real FIFO/input failure.
6. Save, reopen, freeze/unfreeze a VST3 instrument, and confirm format-18 compatibility.

## Remaining real-time risks

- Broad VST3 behavior is third-party code and still requires compatibility, crash-isolation,
  and hostile-plug-in testing.
- JUCE's streaming sources and platform audio backends require the planned Windows hardware,
  disk-throughput, buffer-size, and high-track-count matrix.
- Long-session profiling and OS-level glitch correlation remain production release gates.
- Phase 18A telemetry reports suspected deadline misses; it is not presented as a hardware
  driver's authoritative xrun counter.

Do not treat this engineering checkpoint as completion of the production hardware and plug-in
certification gates.
