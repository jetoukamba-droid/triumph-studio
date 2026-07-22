# Triumph Studio 0.4.1 — Arrangement Editing

## Implemented

- Click a clip to select it; selected clips show a blue outline and edge handles.
- Drag the clip body to move it on the timeline.
- Drag either edge to trim non-destructively within available source media.
- Split the selected clip at the playhead with the Split button or `Ctrl+E`.
- Delete the selected clip with Delete or Backspace.
- Toggle a 250 ms grid with Snap.
- Adjust horizontal scale from 30 to 320 pixels per second with Zoom.
- Scroll horizontally through arrangements longer than the visible window.
- Play multiple clip regions and silence across timeline gaps.
- Undo and redo every model-backed clip edit.

## Windows acceptance gate

1. Import both a 44.1 kHz and a 48 kHz file.
2. Move each clip and confirm the start position snaps when Snap is enabled.
3. Disable Snap and verify fine movement.
4. Trim both clip edges, undo, and redo each edit.
5. Place the playhead inside a selected clip and split it.
6. Move the second region to create a gap and verify silence in that gap.
7. Delete a region, undo it, save, close, and reopen the project.
8. Change Zoom and verify horizontal scrolling at higher magnification.
9. Run the Release test suite with `ctest --test-dir build -C Release --output-on-failure`.

## Product boundary

Phase 4 is a real editing milestone, not the final graph engine. Cross-track clip dragging, fades and crossfades, slip editing, time-stretching, tempo-relative bars/beats, recording, plug-in hosting, immutable real-time render plans, and disk-streaming stress certification remain later release gates.

## 0.4.1 test hotfix

Release builds now use always-active test checks instead of the standard `assert` macro. This prevents `NDEBUG` from removing test setup expressions and ensures the Release test suite executes its checks instead of skipping them.
