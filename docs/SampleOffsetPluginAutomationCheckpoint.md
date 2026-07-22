# Sample-Offset Plug-in Automation Checkpoint

FR-18 moves hosted plug-in automation delivery from block-boundary-only updates
to bounded sample-offset delivery for the current instrument and insert paths.

## What changed

- Instrument plug-in automation is collected into a fixed-size event list for
  each audio callback block.
- Insert plug-in automation uses the same fixed event delivery path.
- Plug-in processing is split into sub-blocks at automation change offsets, so
  parameter values are applied at the intended sample inside the host block.
- The sub-block slicing contract is now a named delivery mode in code and in
  the copied realtime diagnostics report: `sample-offset sub-block slicing`.
- A deterministic native VST3 parameter queue/ramp plan is now tested as the
  next compatibility contract before live VST3 queue delivery is enabled.
- MIDI input for each sub-block is copied into a scratch MIDI buffer with event
  positions re-based to that sub-block.
- Automation queue overflow is reported through the existing realtime telemetry
  parameter-queue counter instead of silently dropping pressure.

## Acceptance

- Windows Release builds cleanly.
- The full CTest suite remains green.
- `TempoAutomationTests` covers the sample-offset event helper, including
  block-start delivery, in-block automation points, clamped parameter values,
  suppression of an unchanged block-start value, deterministic sub-block
  boundaries for duplicate/in-block parameter events, and native queue/ramp
  planning for explicit points plus through-block ramps.
- `RealtimeEngineStateTests` verifies the diagnostics report identifies the
  current plug-in automation delivery mode.
- Hosted instruments and insert effects continue to process through the same
  contained exception and bypass paths.

## Still open

- Native VST3 parameter queue/ramp delivery is planned and tested at the
  contract level, but still needs live host wiring and broader plug-in
  validation.
- Broad third-party compatibility testing remains a professional FR-3 gate.
- Cross-process live runtime isolation remains open beyond the crash-isolated
  discovery scanner.
