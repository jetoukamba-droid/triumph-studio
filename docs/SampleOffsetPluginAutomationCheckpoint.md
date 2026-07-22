# Sample-Offset Plug-in Automation Checkpoint

FR-18 moves hosted plug-in automation delivery from block-boundary-only updates
to bounded sample-offset delivery for the current instrument and insert paths.

## What changed

- Instrument plug-in automation is collected into a fixed-size event list for
  each audio callback block.
- Insert plug-in automation uses the same fixed event delivery path.
- Plug-in processing is split into sub-blocks at automation change offsets, so
  parameter values are applied at the intended sample inside the host block.
- MIDI input for each sub-block is copied into a scratch MIDI buffer with event
  positions re-based to that sub-block.
- Automation queue overflow is reported through the existing realtime telemetry
  parameter-queue counter instead of silently dropping pressure.

## Acceptance

- Windows Release builds cleanly.
- The full CTest suite remains green.
- `TempoAutomationTests` covers the sample-offset event helper, including
  block-start delivery, in-block automation points, clamped parameter values,
  and suppression of an unchanged block-start value.
- Hosted instruments and insert effects continue to process through the same
  contained exception and bypass paths.

## Still open

- Native VST3 parameter queue/ramp parity still needs broader plug-in validation.
- Broad third-party compatibility testing remains a professional FR-3 gate.
- Cross-process live runtime isolation remains open beyond the crash-isolated
  discovery scanner.
