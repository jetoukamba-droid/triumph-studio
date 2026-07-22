# FR-12 Persistent Undo Checkpoint

This checkpoint completes the remaining FR-6 closed-beta project-model gate.
It does not add new UI controls or change editing gestures.

## Delivered

- Project format 24 persists bounded undo and redo restore points inside
  `undoManifest`.
- `ProjectModel` records a project-tree restore point at each undo transaction
  boundary and keeps the history bounded to prevent runaway project growth.
- Reopened projects can use Undo and Redo when the in-memory JUCE undo stack is
  empty but persisted restore points are available.
- ProjectModelTests cover save, reload into a fresh model, restart Undo, and
  restart Redo against a concrete automation edit.

## Boundaries

The persistent history stores safe project-model restore points. It is not a
general binary serialization of JUCE undo actions, third-party plug-in editor
state, or arbitrary command objects. Broader command serialization, command
migrations, and compatibility auditing remain FR-7/public-release hardening.

## Acceptance

- Save a project and confirm `formatVersion` is `24`.
- Confirm saved files contain `undoManifest.undoHistory`.
- Reopen the project in a fresh model and confirm Undo changes the latest saved
  edit and Redo restores it.
