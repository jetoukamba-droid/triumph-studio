# Closed Beta 1 acceptance

The automated suite is necessary but does not make the build ready by itself.
Complete this checklist on the packaged Windows x64 build.

## Required release checks

- [ ] The SHA-256 sidecar matches the generated ZIP.
- [ ] The ZIP extracts on a clean path and contains both executables.
- [ ] `tools\LaunchTriumphStudio.ps1 -NoLaunch` reports the Release app path
      and leaves `TriumphPluginScanner.exe` beside `Triumph Studio.exe`.
- [ ] About reports `0.22.0-beta.1` and `Closed Beta 1`.
- [ ] Device Test Output produces an audible 440 Hz tone.
- [ ] Import, Play/Pause, Stop, seek, and master output produce audio.
- [ ] Save, close, reopen, and media relinking preserve the test project.
- [ ] WAV export completes and the exported audio is audible.
- [ ] Record and stop create a playable WAV-backed take without a dropout.
- [ ] Alt-right-edge varispeed changes both duration and audible pitch.
- [ ] Space, period, B, A, S, Ctrl+S, Ctrl+I, and Ctrl+Shift+E work.
- [ ] The application survives at least a 30-minute edit/playback session.
- [ ] No new crash dump is produced during the acceptance session.

## Beta decision

Ship this build only as a closed beta after every required check passes. A
failed check is a release blocker; record the exact reproduction steps and
repair it before distributing the ZIP.
