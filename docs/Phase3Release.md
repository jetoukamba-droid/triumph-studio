# Triumph Studio 0.3.0 — Project Safety

## Release scope

Phase 3 establishes the document lifecycle required before deeper editing, recording, plug-in hosting, and mixer work can safely expand the product.

### Implemented

- Project format 3 with persistent project UUIDs
- Portable relative paths for media inside the project folder
- Atomic Save and explicit Save As
- Unified Save/Discard/Cancel protection for New, Open, Recent, and Quit
- Persistent recent-project list
- Two-minute recovery autosaves and launch-time recovery
- Twenty rolling pre-overwrite checkpoints
- Collect and Save into `Media/`
- Missing-media count, placeholders, and recursive relinking
- Single-instance application behavior

### Verification gate

Before Phase 3 is accepted on Windows, complete this checklist in a Release build:

1. Import two supported audio files and save a project.
2. Adjust track controls, close the app, and verify Save, Discard, and Cancel independently.
3. Use Save As and confirm the original project still opens.
4. Use Collect and confirm the project opens after moving the complete folder.
5. Rename or move one source file, open the project, and relink it.
6. Save repeatedly and inspect the `Backups` folder.
7. Leave a dirty project open for more than two minutes, terminate the process, relaunch, and recover.
8. Run `ctest --test-dir build -C Release --output-on-failure`.

This release is a professional foundation, not a finished production DAW. Recording, arbitrary clip scheduling, real-time graph immutability, plug-in hosting, routing, latency compensation, and installer/licensing systems remain later release gates.
