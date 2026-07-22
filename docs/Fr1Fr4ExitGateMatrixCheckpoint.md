# FR-21 FR1-FR4 Exit Gate Matrix Checkpoint

FR-21 records the honest state of the first four foundation roadmap areas before
more production features are layered on top. The goal is to prevent a partial
implementation from being treated as a professional DAW exit gate.

## Current matrix

| Area | Current state | Evidence already present | Remaining gate |
| --- | --- | --- | --- |
| FR-1 realtime render and streaming | Partial | Realtime engine state tests, callback attribution, diagnostics surface, diagnostics report | Actual disk read-ahead underrun attribution plus long-session device/fault matrix |
| FR-2 recording and recovery | Partial | Recording state machine, recording plan, recording journal, project repair records, persistent undo manifest | Windows reference hardware recording suite and forced-termination recovery proof |
| FR-3 plug-in isolation | Partial | Crash-isolated scanner, scanner repair, quarantine registry, sample-offset automation evidence, native queue/ramp plan | Broad third-party compatibility and cross-process live runtime isolation |
| FR-4 mixer and monitoring | Partial | Mixer graph tests, monitoring capability diagnostics, realtime/offline render paths | Hardware null, multi-output monitoring, and third-party offline parity gates |

## Deterministic guard

`FoundationsExitGatesTests` verifies that all four areas remain marked partial
until their external proof exists. The test is deliberately conservative: core
code and deterministic tests are not enough to promote a foundation area to
complete without the hardware, recovery, compatibility, or parity matrix named
by the roadmap.

## Professional DAW implication

The product can continue adding higher-level editing and workflow features, but
FR-1 through FR-4 should not be called complete until this matrix goes green with
real machine evidence. That keeps the beta useful without pretending it has
already crossed the production reliability line.
