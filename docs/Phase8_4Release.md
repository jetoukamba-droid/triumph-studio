# Phase 8.4 — Transient Analysis

Triumph Studio 0.8.5 adds a deterministic energy-onset detector that analyzes decoded source audio in a worker thread. It is signal analysis, not AI.

## Workflow

1. Select an audio clip and choose **Detect**.
2. Teal transient lines appear without changing playback.
3. Choose **Warp All** to create editable blue warp anchors in one undoable operation.
4. Continue editing anchors with Ctrl-drag or remove them with right-click.

## Integrity

- Analysis markers are stored separately from playback-affecting warp markers.
- Native source positions are preserved while timeline positions follow warp mapping.
- Trim, split, stretch, save/load, autosave, and undo retain coherent marker state.
- Project format 12 loads formats 1–11.
- Built-in stretching remains varispeed: speed and pitch are linked. This release does not claim pitch preservation or ARA integration.
