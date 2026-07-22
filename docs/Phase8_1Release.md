# Triumph Studio Phase 8.1 Release Gate

Version 0.8.2 introduces professional non-destructive region comping on top of the
Phase 8 take-lane model.

## Delivered

- Shift-drag swipe selection on any recorded take lane.
- Undoable range assignment into a dedicated comp lane.
- Overlapping assignments replace only the intersecting range and preserve unaffected
  left and right comp regions.
- Lane-based comp identity survives source-clip splits.
- Baseline whole-take selection remains active outside explicit comp regions.
- `Use Take` or double-click selects a whole take and clears that group's range overrides.
- Playback is resolved by `ProjectModel`, not by the UI.
- Adjacent source changes receive bounded 10 ms equal-power crossfades.
- Project format 9 saves and restores comp regions and crossfade lengths.
- Format 1–8 projects remain loadable.

## Windows acceptance test

1. Create or open a saved project and record at least two matching-start takes on one
   audio track.
2. Hold Shift and drag across a phrase on Take 1. Confirm that a labelled Take 1 block
   appears in the COMP lane.
3. Shift-drag an overlapping phrase on Take 2. Confirm that only the overlapping portion
   changes and that the remaining Take 1 range is preserved.
4. Play across both boundaries. Confirm continuous output without a click or silent gap.
5. Undo and Redo both assignments and confirm playback follows the visible comp lane.
6. Save, close, reopen, and confirm the comp lane and playback choices are unchanged.
7. Run CTest and require all five test executables to pass.

## Honest boundary

Phase 8.1 supports swipe replacement and automatic fixed-length crossfades. Direct dragging
of existing comp boundaries, adjustable fade curves/lengths, loop recording, clip variants,
transient detection, elastic time, warp markers, pitch/time provider abstraction, and ARA
integration research remain later Phase 8.x work.
