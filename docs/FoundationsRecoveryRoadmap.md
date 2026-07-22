# Triumph Studio Foundations Recovery Roadmap

## Status reset

The Phase 17 source audit against `Professional_DAW_Architecture_Research.md`
is the baseline for all further work:

| Research area | Built | Partial | Missing | Recovery backlog |
| --- | ---: | ---: | ---: | ---: |
| Architecture | 9 | 5 | 1 | 6 |
| Real-time engine | 3 | 5 | 7 | 12 |
| Buffers, streaming, and clocks | 8 | 6 | 1 | 7 |
| Project and media model | 11 | 2 | 2 | 4 |
| Timeline and editing | 5 | 7 | 3 | 10 |
| Recording and monitoring | 4 | 5 | 6 | 11 |
| Mixer, routing, and PDC | 5 | 8 | 2 | 10 |
| Plug-in hosting and isolation | 2 | 3 | 10 | 13 |
| MIDI, tempo, and automation | 4 | 4 | 7 | 11 |
| Rendering and reliability | 2 | 7 | 6 | 13 |
| **Total** | **53** | **52** | **45** | **97** |

The product is an internal alpha/engineering preview. The old phase number is not
evidence of maturity, and no future checkpoint may be called beta solely because a
visible feature list is long.

Phase 18A remains a completed corrective checkpoint. It replaced the callback try-lock
with prepared render generations, removed the recording callback try-lock, added bounded
parameter delivery, and added real-time telemetry. It improves findings 24, 29, 37, 39,
44, 45, 86, and 149, but the baseline totals above remain frozen until the complete
item-level re-audit is recorded with source and test evidence.

## Recovery sequence

Every recovery milestone is tied to the original numbered findings and the exit tests
in the research. Work is ordered by architectural dependency, not by interface appeal.

### FR-1 - Real-time render and streaming foundation

Backlog: 19 partial-or-missing findings from research ranges 16-45.

- Preserve the immutable prepared-render publication boundary from Phase 18A.
- Replace floating-point transport authority with integer sample position.
- Separate live/record-enabled and playback scheduling classes.
- Add bounded read-ahead starvation reporting and device restart state transitions.
- Add graph-build duration, allocation-pressure, and per-node timing telemetry.
- Stress variable block sizes, device loss, disk starvation, and graph mutation.

Exit gate: long clips stream through buses without callback allocation, locks, or file I/O;
device restart does not corrupt the project; faults are attributed rather than silent.

Implementation checkpoint: version 0.21.5 adds a lock-free device-recovery
ledger, transport-preserving callback stop/restart behavior, bounded default
output reopening, and separate interruption/failure/stall/sustained-silence
diagnostics. FR-1 remains open on explicit read-ahead starvation attribution,
variable-device/block stress, and the long-session hardware matrix.

### FR-2 - Recording safety and recoverability

Backlog: 11 partial-or-missing findings from research range 76-90.

- Introduce explicit armed, pre-roll, recording, punch, loop-pass, finalize, aborted,
  and recovered states.
- Add named input routes and named recording tap points.
- Add off/while-armed/always software-monitor modes.
- Add pre-roll, punch ranges, loop-pass ownership, and independent take files.
- Keep latency alignment and add calibrated per-input record offsets.
- Journal every open capture and recover valid unfinalized takes after forced termination.
- Extend the writer service from one take to simultaneous bounded multitrack capture.

Exit gate: simultaneous takes remain sample-aligned, loop passes remain independently
addressable, and a forced termination leaves a journaled recording that can be recovered.

Implementation checkpoint: version 0.19.0 now drives the explicit audio recording states,
persists per-track routes/taps/offsets, captures simultaneous targets through one bounded
writer service, pre-opens independent loop-pass files, journals every track/pass, and keeps
software monitoring on an independent Control Room gain/dim/mute path. Deterministic source
and decoded-WAV tests pass locally. FR-2 remains open until the Windows 23-test suite,
physical alignment run, and forced-termination recovery gate pass on reference hardware;
larger-than-stereo device surfaces also remain future work.

### FR-3 - Plug-in discovery and isolation

Backlog: 13 partial-or-missing findings from research range 106-120, plus reliability
requirements 146-147.

- Move VST3 discovery and validation into a helper process.
- Persist a registry with stable IDs, scan status, failure reason, vendor, version,
  capabilities, bus layouts, latency, and tail metadata.
- Add blocklist, retry/rescan, and missing-plug-in placeholders.
- Keep processor state independent of editor-window lifetime.
- Add safe bypass, tail-preserving removal, latency-change notifications, parameter IDs,
  and sample-offset parameter queues.
- Define the process boundary needed for optional per-plug-in runtime isolation.

Exit gate: a scan-crashing plug-in cannot prevent application launch; a failed scan is
recorded and blocklisted; projects reopen with either restored state or a repairable
placeholder; PDC reacts to latency changes.

### FR-4 - Mixer, PDC, and monitoring graph

Backlog: 10 partial-or-missing findings from research range 91-105.

- Calculate compensation over the complete routing graph, including sends and sidechains.
- Make mono/stereo/bus/sidechain layouts explicit connection contracts.
- Separate program master from monitor, cue, dim, listen, and talkback paths.
- Preserve alignment across pre/post-fader sends and live low-latency exceptions.

Exit gate: parallel paths null after compensation, sidechains remain aligned, and monitor
changes never alter exported program audio.

### FR-5 - MIDI, automation, and synchronization

Backlog: 11 partial-or-missing findings from research range 121-135.

- Add controller lanes, SysEx, MPE-ready per-note expression, and MIDI 2.0 UMP boundaries.
- Deliver sample-offset plug-in MIDI and parameter automation.
- Add automation record/simplify plus read, touch, latch, and write semantics.
- Add transport-master abstraction for internal, MIDI clock, MTC, and later LTC sync.

Exit gate: MIDI and automation render consistently online/offline across block sizes and
tempo changes, with explicit documented limits for external-clock jitter.

Implementation checkpoint: version 0.21.0 adds the multi-insert trusted rack,
complete route-aware PDC preparation, explicit monitor paths, MPE expression,
high-resolution controllers, MIDI 2.0 UMP contracts, automation-write/simplify
coverage, MIDI Clock following, and MTC position chase. FR-3 remains open on the
cross-process live worker; FR-4 remains open on hardware null/multi-output and
third-party offline-parity gates; FR-5 remains open on native MIDI 2.0 transport,
SysEx, clock output, Link/LTC, and external-jitter qualification. See
`ProfessionalSystemsCheckpoint.md`.

### FR-6 - Project and non-destructive editing completion

Backlog: 14 partial-or-missing findings from research ranges 46-75.

- Finish command coverage, persistent undo history, clip variants, and asset fingerprints.
- Finish exact sample-boundary loop, crossfade, fade-shape, and time-domain invariants.
- Add project repair records for missing or incompatible state.

Exit gate: all supported edits survive save/load/undo, project moves relink by stable
identity, and corrupt or missing assets preserve repair information.

Implementation checkpoint: the post-Beta 1 FR-6 fade/crossfade checkpoint adds
persistent per-clip fade-in and fade-out lengths, equal-power overlap crossfades,
top-edge fade handles, and sample-accurate real-time/offline gain envelopes. Fade
anchors remain continuous across warp-generated playback segments; split, trim,
and stretch operations preserve or transform fade state deliberately; save/reopen
and Undo are covered by the project-model tests. At that checkpoint, FR-6
remained open on persistent Undo across restarts, clip variants, stable asset
fingerprints, and repair records.

Implementation checkpoint: the post-Beta FR-10 project-safety checkpoint upgrades
saved projects to format 22 with media asset fingerprints, persisted repair
records, and an undo manifest that records current undo/redo labels. Missing
frozen-render recovery now produces a durable repair record instead of silently
changing the project. At that checkpoint, FR-6 remained open on full
cross-restart undo replay and clip variants.

Implementation checkpoint: the post-Beta FR-11 clip-variants checkpoint upgrades
saved projects to format 23 with persistent alternate audio clip states. Clip
variants capture and restore source identity, source offset, length, gain, fades,
stretch provider/ratio, reverse state, warp markers, and transient markers with
Undo/Redo and save/load coverage. FR-6 remains open on full cross-restart undo
replay.

Implementation checkpoint: the post-Beta FR-12 persistent-undo checkpoint
upgrades saved projects to format 24 with bounded undo/redo restore points.
Supported project-model edits now survive save/reopen with actionable Undo and
Redo after restart, not just remembered labels. FR-6 is complete for the current
closed-beta project model; wider command serialization and migration remain
future public-release hardening under FR-7.

Implementation checkpoint: the post-Beta FR-13 realtime fault-attribution
checkpoint starts the remaining FR-1/FR-7 hardening. Realtime status now
separates callback deadlines, missing render state, oversized blocks,
read-ahead/source starvation, device restarts, variable block-size changes,
parameter queue overflow, snapshot swaps, graph-build timing, and render-retire
backlog pressure. FR-1 remains open on actual disk read-ahead underrun
attribution and long-session hardware/fault matrices.

Implementation checkpoint: the post-Beta FR-14 realtime diagnostics-surface
checkpoint exposes those FR-13 counters in the transport status strip and
tooltip so closed-beta test reports can identify state, buffer, stream/source,
queue, variable-device, graph-build, plug-in, and hardware-recovery pressure
without a debugger.

Implementation checkpoint: the post-Beta FR-15 realtime diagnostics-report
checkpoint adds a clipboard report from `More > Copy Realtime Diagnostics`.
The report preserves the compact status strip while giving testers a structured
project/device/realtime/plug-in/recovery/sync snapshot with a clean or
action-required verdict for issue reports.

Implementation checkpoint: the post-Beta FR-16 plug-in scanner reliability
checkpoint repairs the crash-isolated VST3 scanner placement when the built
helper exists but is missing beside the app. Remaining scanner failures now
include the expected install folder and every checked helper path.

Implementation checkpoint: the post-Beta FR-17 plug-in quarantine controls
checkpoint makes the VST3 registry actionable. The registry reports validated,
blocked, missing, instrument, and audio-effect counts, can copy a structured
support report, can retry a registered plug-in deliberately, can load a
registered plug-in as an insert effect on the selected channel, and can forget
one selected registry record without clearing unrelated scan history. FR-3
remains open on broad real-plug-in compatibility and cross-process live runtime
isolation.

Implementation checkpoint: the post-Beta FR-18 sample-offset plug-in automation
checkpoint moves hosted parameter delivery off the block boundary for current
instrument and insert plug-in paths. Automation points are collected into a
bounded per-block event list, plug-in processing is split at the changed sample
offsets, MIDI events are re-based for each sub-block, and queue overflow is
attributed through realtime telemetry. FR-3 remains open on host-native VST3
parameter queue/ramp parity, broad third-party compatibility, expanded bus
coverage, and cross-process live runtime isolation.

Implementation checkpoint: FR-19 names and tests the current delivery contract
as `sample-offset sub-block slicing`, uses the same helper in the audio engine
and deterministic automation tests, and exposes the delivery mode in `More >
Copy Realtime Diagnostics`. Native VST3 parameter queues/ramps remain a later
compatibility gate.

Implementation checkpoint: FR-20 adds a deterministic host-native parameter
queue/ramp plan for the next VST3 compatibility step. The plan converts
automation into stable sample offsets, clamped values, and ramp metadata,
including ramps that cross a block without an automation point inside that
block. The live engine still uses the FR-18/FR-19 sub-block delivery path until
third-party VST3 queue/ramp parity is wired and validated.

### FR-7 - Reliability and product architecture

Backlog: 19 partial-or-missing findings from research ranges 1-15 and 136-150.

- Formalize engine, services, commands, persistence, and presentation boundaries.
- Add deterministic online/offline parity tests, crash traces, and fault injection.
- Attribute callback, disk, graph-build, scan, plug-in, allocation, and UI costs separately.
- Build the stable command/parameter model required for accessibility, localization,
  keyboard navigation, control surfaces, and OSC.

Exit gate: unattended stress covers device loss, disk starvation, corrupt projects,
missing media, missing plug-ins, scan crashes, graph mutation, and long recording.

## Release gates

- **Internal alpha:** current status; incomplete professional systems are clearly labeled.
- **Feature-complete alpha:** FR-1 through FR-6 exit gates pass on Windows reference hardware.
- **Beta:** FR-7 automated fault suite passes, plug-in scanning is crash-isolated, recording
  recovery is demonstrated after forced termination, and the hardware/plug-in matrix is green.
- **Production:** signed delivery, clean-machine upgrade/uninstall, accessibility, security,
  compatibility, long-session, and audio-quality gates all pass.

The backlog count only decreases when a finding has implementation evidence, an automated
or repeatable acceptance test, and updated documentation. Partial work is never rounded up.
