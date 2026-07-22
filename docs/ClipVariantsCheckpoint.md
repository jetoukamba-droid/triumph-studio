# FR-11 Clip Variants Checkpoint

This checkpoint continues the FR-6 project-safety work after repair records and
the undo manifest.

## What changed

- Project format 23 persists audio clip variants.
- A clip variant captures alternate source identity, source trim, length, gain,
  fades, stretch ratio/provider, reverse state, warp markers, and transient
  markers.
- Restoring a variant keeps the clip on the same timeline start while swapping
  the captured edit state underneath it.
- Capture, restore, delete, Undo/Redo, snapshot replacement, duplicate, and
  save/load paths preserve variant identity.

## Not done here

- Full cross-restart undo replay remains open.
- Variant UI menus are a later presentation task; this checkpoint establishes
  the model and persistence contract first.

## Verification

- `ProjectModelTests` covers capture, restore, undo, redo, save, and reopen.
- Full CTest should remain green before packaging.
