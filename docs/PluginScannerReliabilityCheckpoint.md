# FR-16 Plug-in Scanner Reliability Checkpoint

## Scope

FR-16 hardens the crash-isolated VST3 scanner path. Triumph now tries to repair
the scanner install before showing a missing-scanner error.

## Changes

- `PluginScanService` exposes the scanner install folder, helper copy routine,
  and `ensureHelperBesideApplication()`.
- VST3 scan/load actions call the repair path before displaying the scanner
  missing dialog.
- The scanner diagnostic text now reports the expected install folder and every
  candidate path that Triumph checked.
- The copy routine installs both supported helper names:
  `TriumphPluginScanner.exe` and `Triumph Plugin Scanner.exe`.

## Validation

- `PluginScanServiceTests` verifies helper diagnostics, missing-helper errors,
  scanner copy/repair behavior, scanner crash handling, timeout handling,
  malformed/missing output handling, registry update behavior, blocked-entry
  clearing, VST3 discovery, and legacy registry loading.
- Windows Release validation should run the targeted `PluginScanServiceTests`
  and the full registered test suite.
- In-app smoke test: launch via `tools\LaunchTriumphStudio.ps1`, open `VST3`,
  start a scan or choose a plug-in, and confirm the app no longer reports a
  missing scanner when the built scanner exists under `build`.

## Boundary

This checkpoint fixes helper placement and diagnostics. It does not yet add a
full plug-in quarantine browser or long-running hardware/VST compatibility
matrix.
