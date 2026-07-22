# Foundations Recovery Evidence Ledger

This ledger records only source-backed changes made after the Phase 17
53-built / 52-partial / 45-missing baseline. It does not round a research
finding up merely because one supporting mechanism exists.

## Real-time and clock evidence

| Research finding | Evidence added | Current assessment |
| --- | --- | --- |
| 24 | Complete render generations are built on the control thread and atomically published. | Built for the current graph |
| 29 | Callback consumes one prepared generation; callback audit rejects lock/allocation/file tokens. | Built for Triumph-owned callback code |
| 37 | Callback deadlines and suspected dropouts are atomically counted and visible. | Built; driver-authoritative xrun correlation remains open |
| 39 | JUCE read-ahead playback and bounded threaded recording writer remain outside ordinary callback file I/O. | Partial; starvation attribution and configurable playback/record buffers remain open |
| 41 | `FixedSampleClock` makes Q20 integer project-sample time authoritative; seconds are derived. | Built for internal transport authority |
| 44 | Prepared buffers accept bounded callback sizes and flag oversize blocks. | Built for the current engine |
| 45 | Mixer/input meters publish through atomics and UI reads never lock the callback. | Built |
| 149 | Callback/deadline/snapshot/queue telemetry plus fixed-clock regressions exist. | Partial; disk, graph-build, plug-in, allocation, and UI attribution remain open |

## Interface responsiveness evidence

The clip editor now keeps move, trim, stretch, comp-range, and warp-marker
gestures in a local visual preview and commits one project mutation when the
gesture ends. Previously, every mouse-drag event synchronously rebuilt the
complete engine render state and refreshed every project panel on the message
thread. Track rows also stop repainting full waveform lanes at 30 Hz while the
transport and playhead are idle. This is a corrective performance change and
does not reclassify a research finding by itself.

## Recording safety evidence

| Research finding | Evidence added | Current assessment |
| --- | --- | --- |
| 83 | Persistent monitor modes support Off, While Armed, and Always. Input monitoring is summed after project master gain through independent Control Room gain, dim, and mute state. | Built for the current stereo software-monitor path; dedicated cue outputs and talkback remain FR-4 work |
| 84 | Each armed track persists an input route, tap, and signed record offset. Hardware-input targets additionally receive driver latency plus the project calibration. | Built for the current two-input device surface |
| 86 | Every track/pass writer is opened before use. `MultiTrackRecorder` shares one bounded background writer service; the audio callback only selects prepared passes and pushes channel pointers. | Built for the bounded current recorder graph |
| 87 | `RecordingPlan` drives armed, pre-roll, normal capture, punch, loop-pass, finalizing, finalized, and aborted transitions from the transport. It slices capture at device-frame boundaries even when device and project rates differ. | Built for audio recording; MIDI remains a separate single-target capture path |
| 88 | Simultaneously armed audio tracks retain independent WAV files. Loop wraps switch to a pre-opened pass, with one stable pass number and take lane per file. | Built for the current two-input device surface |
| 89 | Every open track/pass has a versioned journal containing project, track, tap, input channel, pass, timing, drop, and committed-sample metadata. Readable interrupted WAVs are offered to their original track; unused prepared passes are removed. | Built in source and deterministic tests; forced-termination Windows hardware execution remains open |
| 90 | Input meters and monitoring continue with transport stopped. | Built |

`RecordingPlanTests` cover pre-roll, punch-end slicing, loop wrap/pass ownership,
invalid short loops, and 48 kHz project / 44.1 kHz device boundaries.
`MultiTrackRecorderTests` decode the produced WAV files and verify mono hardware-input
and stereo device-output routing, independent offsets, loop-pass ownership, cleanup,
and journal removal. The callback audit now covers playback, the individual writer,
and the multitrack dispatcher.

## Mixer and delay-compensation evidence

| Research finding | Evidence added | Current assessment |
| --- | --- | --- |
| 99 | The hosted processor reports latency changes through `AudioProcessorListener`; the callback-facing notification stores only atomics, and the interface timer commits the new latency so the prepared compensation graph is rebuilt. | Built for the current single instrument slot; broad plug-in compatibility remains open |

## Remaining FR-2 target exit gates

- Execute a forced-termination recovery recording on Windows reference hardware and
  recover every readable simultaneous take after relaunch.
- Run a sustained two-input loop/multitrack session and confirm alignment against a
  physical split signal; deterministic file tests do not replace device validation.
- Expand beyond the current two-input/two-output application surface for larger interfaces.
- Hardware direct-monitor controls remain honestly driver-managed. Dedicated cue outputs,
  listen, and talkback remain in FR-4 rather than being simulated.

## Plug-in discovery and isolation evidence

| Research finding | Evidence added | Current assessment |
| --- | --- | --- |
| 106 | Processor ownership and saved state remain independent of editor-window lifetime. | Built for the current single instrument slot |
| 109 | Description XML and processor state survive project save/load; unavailable instances retain project identity. | Built for the current single instrument slot |
| 110 | Bypass is project-owned and undoable; the processor remains loaded and processing, receives all-notes-off, fades its host output over 10 ms, and retains reported PDC latency. | Built for the current instrument slot; broad compatibility testing remains open |
| 111 | One 1–30 second natural-tail policy now drives live note/stop and freeze paths; bypass/removal use a defined 10 ms transition and faults mute immediately. Live external delivery remains blocked until freeze. | Built for the currently supported single-instrument paths |
| 112 | Standard-folder VST3 discovery runs in `Triumph Plugin Scanner`. Registry v2 stores identity, metadata, file fingerprint/availability, attempts, success/failure times, and descriptions. New/changed files scan; unchanged validated or blocked files skip; explicit retry always scans. | Built for current VST3 discovery and inventory |
| 113 | Discovery happens only in a timeout-bounded helper process; failed, crashed, malformed, missing-output, or hung helpers return a contained result. The process-fault suite passed in the 23-test Windows 0.19 baseline. | Built for synthetic faults; a real scan-crashing VST3 fixture remains a compatibility gate |
| 120 | The host negotiates a usable mono/stereo instrument output, inventories stable parameter IDs, delivers bounded sample-offset automation to current instrument/insert plug-in paths, checksums identity-bound presets, measures live processing, preserves missing placeholders, and rebuilds PDC on latency change. | Partial; native VST3 parameter-queue/ramp parity, broad multi-bus exposure, compatibility coverage, and runtime process isolation remain open |
| 146 | Untrusted discovery has a process boundary and persistent blocklist. Live timing is attributed, and escaping C++ exceptions mute/bypass while retaining recovery state. | Partial; native crash/hang containment still requires a runtime worker process |
| 147 | Missing or failed plug-ins retain description/state and an unavailable repair action instead of deleting the track structure. | Built for the current single instrument slot |

## Still open before the full professional FR-3 gate can pass

- Execute a real scan-crashing VST3 fixture and a broad vendor compatibility matrix on Windows.
- Complete host-native VST3 parameter queue/ramp parity and broad third-party
  sample-accurate automation compatibility beyond the current bounded sub-block
  delivery path.
- Expand from one instrument slot to multiple track/bus inserts, audio effects, and exposed
  sidechain/multi-output buses.
- Add grouped or per-plug-in runtime process isolation with shared audio/MIDI/parameter transport,
  watchdog, state checkpoints, and deterministic crash/hang recovery.

The baseline count will be reclassified only after the relevant recovery exit gate
passes and the complete numbered audit is repeated.

## Current verification state

- The 0.21.6 professional-controls source registers 32 Windows tests,
  including the conflict-checked keyboard shortcut map.
  `AudioDeviceRecoveryTests` passes locally under strict C++20 warnings-as-errors
  and verifies bounded recovery attempts plus one-shot transport resume intent.
  The integration preserves the fixed-sample position while a callback device
  is absent and resumes only after a valid device has prepared. A clean Windows
  build, complete 31-test run, and physical unplug/replug exercise remain the
  target acceptance gate.

- The 0.21.0 professional-systems source registers 28 Windows tests. The new
  runtime-isolation/rack, professional mixer graph, MIDI performance/MTC, and
  automation-write tests pass locally under strict C++20 warnings-as-errors.
  Format 21 project-model coverage persists plug-in slots/layouts, monitor paths,
  sync, high-resolution controllers, and per-note expression. A clean Windows
  Release build and complete 28-test run remain the package acceptance gate.
- Live multi-insert effects currently use the explicit `trusted-in-process`
  mode with C++ exception containment. The worker protocol/watchdog exists, but
  native crash/hang containment remains open until its cross-process transport
  is connected. Native MIDI 2.0 delivery, SysEx, clock output, Link/LTC, and
  third-party offline insert parity also remain open and are not rounded up.

- All application and JUCE-aware test translation units pass strict C++20 syntax checking with
  warnings treated as errors in the recovery workspace.
- All seventeen framework-independent executables pass locally, including recording-plan
  and monitoring-capability coverage.
- `RecordingJournalTests` passes as a linked JUCE-core executable locally.
- `MultiTrackRecorderTests` passes as a linked JUCE audio-formats executable locally and
  verifies decoded channel content, per-track offsets, independent loop passes, and cleanup.
- The 0.19 `PluginScanServiceTests` passed both locally and on Windows, including nonzero
  scanner exit, timeout termination, malformed output, missing output, valid error protocol,
  registry replacement, and blocked-record clearing. The expanded 0.20 registry/discovery
  assertions pass strict compile validation and await the 25-test Windows run.
- A clean Windows Release build completes, including the scanner helper, application,
  project-model tests, and real-time engine tests.
- The complete 0.19 Windows CTest suite passes 23 of 23 tests. The real-time audio test is the
  longest case at roughly 100 seconds; the total observed suite time was 105.87 seconds.
- The forced-termination and physical split-signal hardware gates must still run before the
  complete FR-2 professional gate is marked passed.
- The clip-gesture responsiveness correction passes strict C++20 syntax checking with
  warnings treated as errors, and all fifteen framework-independent regression executables
  still pass locally. A Windows interaction check remains required before this corrective
  item is considered verified on the target interface.
- The 0.20.0 FR-3 source adds two tests, bringing the Windows Release suite to 25. The new
  framework-independent plug-in host-policy test and linked JUCE-core preset integrity test
  pass locally. All modified application, scanner, model, preset, UI, and freeze translation
  units pass strict C++20 warnings-as-errors syntax validation. The complete 25-test Windows
  run remains the target acceptance gate for this package.
