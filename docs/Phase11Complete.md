# Phase 11 Complete — Delay Compensation

Triumph Studio 0.11.0 closes Phase 11 as one consolidated milestone. The phase
replaces the Phase 10 single-path workaround with a bounded routing-wide timing
contract and real per-track delay processing.

## Delivered

- acyclic node/edge validation with deterministic topological planning
- independent audio, send, and sidechain route classification
- reported, bypassed, frozen, missing, and manually corrected latency accounting
- per-merge arrival calculation and route-specific compensation delays
- preallocated stereo `SampleDelayLine` processing for every live track path
- independent compensation for decoded audio, built-in MIDI, and hosted VST3 output
- Low Latency project mode that prevents added delay on armed monitoring paths
- persistent, undoable per-track manual latency calibration in samples
- graph-latency telemetry in the Low Latency control tooltip
- automation lookahead calculation from compensated upstream arrival
- format 14 save/load with formats 1–13 migration

## Automated acceptance contract

1. Parallel zero- and six-sample paths produce their impulses on the same output sample.
2. Audio, send, and sidechain inputs align independently at each merge.
3. Bypassed, frozen, and unavailable processors contribute no reported plug-in latency.
4. Manual positive and negative calibration changes effective path latency deterministically.
5. A routing cycle or missing node is rejected instead of publishing a partial plan.
6. Low Latency mode leaves an armed path free of newly added delay and reports the tradeoff.
7. Project format 14 persists Low Latency mode and manual offsets while older projects load.
8. A Windows Release build passes all seven CTest targets.

## Manual Windows acceptance checklist

1. Open a project containing an audio track and a live VST3 instrument.
2. Confirm the Low Latency tooltip reports the instrument's graph latency in samples.
3. Play a sharp transient beside a sharp instrument note and confirm the paths stay aligned.
4. Change a track's OFFSET control and confirm the correction survives Save, close, and reopen.
5. Arm an audio track, enable Low Latency, and confirm monitoring remains immediate.
6. Disable Low Latency and confirm full alignment resumes for ordinary playback.
7. Bypass, freeze, or make the instrument unavailable and confirm its reported delay is removed.

## Honest boundary

The planner and live delay engine support audio, send, and sidechain route classes, but
Phase 11 does not add mixer send or sidechain creation controls. The current product graph
still publishes track-to-master routes and one active external VST3 instrument slot.
Automation compensation math is implemented and tested; editable automation lanes remain
in their later roadmap phase. Low Latency mode intentionally allows an armed path to be
temporarily unaligned when avoiding added delay is more important than full playback nulling.
