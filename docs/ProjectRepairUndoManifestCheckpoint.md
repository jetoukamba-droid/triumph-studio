# FR-10 Project Repair and Undo Manifest Checkpoint

This checkpoint continues FR-6 project and non-destructive editing completion.
It does not add launch packaging or user-interface redesign work.

## Delivered

- Project format 22 persists `repairRecords`.
- Project format 22 persists an `undoManifest` with the current undo and redo
  descriptions.
- Media sources now carry a stable `assetFingerprint` field based on source
  identity metadata and available file size/timestamp evidence.
- Existing missing frozen-render recovery now records a repair event instead of
  silently unfreezing the instrument track.
- The project model preserves repair records and loaded undo manifests through
  snapshot replace/save flows.

## Boundaries

The undo manifest is not full cross-restart undo replay. It is the durable
project-history contract needed before persistent undo replay can be made safe.
Full undo replay still requires command serialization, command migrations, and
project-version compatibility checks.

The media fingerprint is not a full audio checksum. It is a stable metadata
fingerprint that lets later relink/repair work compare media identity without
reading entire audio files in the project loader.

## Acceptance

- Save a project and confirm `formatVersion` is `22`.
- Confirm each saved media source contains `assetFingerprint`.
- Confirm saved files contain `undoManifest`.
- Corrupt a frozen instrument render path, reopen, and confirm the project
  reports a `freeze` repair record while preserving the instrument track.
