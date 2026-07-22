# FR-13 Realtime Fault Attribution Checkpoint

This checkpoint starts the FR-1 and FR-7 professional reliability work after
the closed-beta FR-6 project-model gate.

## Implemented

- Realtime telemetry now separates callback deadline misses from missing render
  state, oversized audio blocks, read-ahead/source starvation, device restart
  transitions, variable block-size changes, parameter queue overflow, snapshot
  swaps, and render graph build cost.
- Render graph publication records build count, total/max build time, and the
  retired-render backlog high watermark. These values are measured on the
  control thread and do not add callback allocations.
- Audio-track timeline rendering reports scheduled clip intersections that did
  not reach a usable source/render pass, allowing the engine to attribute
  silent playback to stream/source starvation instead of a generic dropout.
- `RealtimeEngineStateTests` covers every new counter and average calculation.
- `AudioEngineRealtimeTests` verifies normal edit-stress playback does not
  report missing render state, oversized blocks, or read-ahead starvation while
  graph-build and device-restart evidence is recorded.

## Remaining FR-1 / FR-7 work

- Attribute actual disk read-ahead underruns from the file streaming layer, not
  only missing or unusable scheduled sources.
- Add long-session variable-device, variable-block, graph-mutation, and disk
  starvation fault runs on Windows reference hardware.
- Surface the new telemetry in the app status/diagnostics panel with actionable
  labels for testers.
