# FR-14 Realtime Diagnostics Surface Checkpoint

This checkpoint builds on FR-13 by making realtime fault attribution visible in
the studio shell for closed-beta testing.

## Implemented

- The transport status strip now keeps the existing `RT` load and `XRUN`
  count, then shows the highest-priority active attribution code:
  - `STATE` for callbacks without a prepared render state.
  - `BUF` for oversized callback blocks.
  - `STREAM` for scheduled clip intersections that did not reach a usable
    source/render pass.
  - `QUEUE` for bounded parameter queue overflow.
  - `VAR` for device block-size or sample-rate changes after the first prepare.
- The realtime status tooltip now exposes the complete FR-13 breakdown:
  callback maximum, deadline misses, missing state, oversized blocks,
  read-ahead/source starvation, snapshot swaps, queue overflows, active render
  generation, device restarts, variable block/rate changes, graph build
  count/average/max, and render-retire backlog high watermark.
- The label remains in the existing transport/status band and preserves the
  professional pro-console layout; the expanded detail lives in the tooltip to
  avoid crowding the timeline.

## Manual acceptance

- Launch the app, hover the realtime status label, and confirm the tooltip
  includes the full realtime diagnostic breakdown.
- Start playback and verify the compact label still reads cleanly at 1280 px
  and 1366 px widths.
- During stress testing, record the first non-zero attribution code and the
  tooltip counters alongside any audio glitch report.
