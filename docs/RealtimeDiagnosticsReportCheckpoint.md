# FR-15 Realtime Diagnostics Report Checkpoint

## Scope

FR-15 turns the FR-13/FR-14 realtime evidence into a copyable tester report.
The status strip still stays compact, but closed-beta testers can now use
`More > Copy Realtime Diagnostics` to place the current realtime health report
on the clipboard.

## Evidence Included

- Version, release channel, project name, audio device, sample rate, and block
  size.
- Callback count, average/maximum callback duration, deadline misses, and
  suspected dropouts.
- Missing render-state, oversized block, read-ahead/source starvation,
  parameter queue, render snapshot, active generation, graph build, and render
  retire backlog counters.
- Plug-in process timing, plug-in deadline misses, contained plug-in exceptions,
  device recovery, callback-stall, sustained-silence, and external-sync state.
- A final `Verdict` field that says `Clean` or `Action required`.

## Validation

- `RealtimeEngineStateTests` covers report formatting and verdict selection for
  clean and unhealthy telemetry.
- A Windows Release build should pass all registered tests.
- In-app smoke test: launch Triumph, open `More > Copy Realtime Diagnostics`,
  paste into a text editor, and confirm the report includes the current project,
  device, realtime counters, plug-in runtime, device recovery, sync, and verdict.

## Notes

This checkpoint deliberately avoids adding a new visible diagnostics panel. It
adds a low-risk reporting path for real app testing before larger timeline and
plug-in workflow work resumes.
