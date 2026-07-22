# Native Plug-in Automation Parity Checkpoint

FR-20 adds the host-side contract needed before Triumph enables native VST3
parameter queue/ramp delivery.

## What changed

- `TempoAutomation` now names the future native delivery mode as
  `native VST3 parameter queue/ramp plan`.
- Automation can be converted into native queue points with stable sample
  offsets, clamped normalized values, and per-segment ramp metadata.
- Ramps that pass through a block without an automation point inside that block
  still receive a block-end queue point, so the host can preserve the curve
  instead of collapsing the change into one block-boundary value.
- The current live engine remains on the already-tested
  `sample-offset sub-block slicing` mode until real VST3 queue compatibility is
  validated against third-party plug-ins.

## Acceptance

- `TempoAutomationTests` covers the current sub-block delivery mode and the
  native VST3 queue/ramp plan.
- Explicit in-block parameter points retain sample offsets and clamped values.
- Duplicate offsets remain deterministic.
- Through-block smooth ramps emit start/end queue points and ramp metadata.

## Still open

- Wire the native queue plan into the VST3 process context when broad plug-in
  compatibility testing is ready.
- Validate live and offline automation parity with real instruments and insert
  effects from the compatibility matrix.
