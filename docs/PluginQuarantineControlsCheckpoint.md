# FR-17 Plug-in Quarantine Controls Checkpoint

## Scope

FR-17 turns the VST3 registry from a passive scan cache into an actionable
quarantine surface. The app can now summarize plug-in health, copy a registry
report for support, retry a registered plug-in deliberately, load a registered
plug-in as an insert effect, and forget one bad registry record without wiping
the rest of the scanner history.

## Changes

- `PluginScanService` now exposes a registry summary with validated, blocked,
  missing, instrument, and audio-effect counts.
- The service can describe an individual registry entry and build a clipboard
  report containing status, identity, path, scan times, and failure text.
- Registry cleanup can target one selected plug-in path instead of only clearing
  every blocked record.
- The VST3 menu now groups each registry entry with explicit actions:
  load or retry as an instrument, load as an audio effect on the selected
  channel, forget the selected registry record, and view the last failure.
- The registry submenu title shows health counts so testers can see whether
  scan problems are blocked or missing-file cases before opening a report.

## Validation

- `PluginScanServiceTests` covers registry summaries, report text, blocked and
  missing counts, individual entry descriptions, targeted record forgetting,
  blocked-record clearing, scanner failure paths, and successful replacement of
  a blocked entry.
- Windows Release validation should run the targeted `PluginScanServiceTests`
  and the full registered test suite.

## Boundary

This checkpoint improves quarantine visibility and recovery controls. Live
third-party `processBlock()` execution still runs inside the application
process; grouped or per-plug-in runtime isolation remains a later FR-3 exit-gate
task.
