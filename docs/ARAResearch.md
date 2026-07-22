# ARA Integration Research Boundary

## Phase 14 decision

Triumph Studio does not expose an ARA button, advertise ARA compatibility, compile an ARA SDK,
or simulate an ARA plug-in. Phase 14 establishes the project and rendering boundaries that a
future licensed integration can reuse while keeping the current build truthful and portable.

## Proposed host boundary

A future ARA host layer should translate stable project-owned IDs into document-controller,
musical-context, region-sequence, audio-source, and audio-modification lifetimes. It must read
audio through bounded random-access callbacks backed by `MediaSourceState`, publish tempo and
signature changes from the same `TempoMap` used by playback, and map clip trim, direction,
stretch, warp, and comp state without giving a plug-in ownership of the project model.

All ARA object creation, analysis requests, archive restore, and content updates must remain off
the real-time callback. The audio callback may consume only prepared render state. Undo must be
coordinated with `ProjectModel`, and plug-in archive data must be versioned inside the project
document without silently changing the original audio file.

## Work required before enablement

1. Obtain and review the official ARA SDK and its distribution, trademark, and notice terms.
2. Choose the exact JUCE/ARA host bridge or implement and maintain the host interfaces directly.
3. Define document-controller ownership, thread rules, shutdown ordering, and crash recovery.
4. Add persistent archive data with size limits, validation, migrations, and missing-plug-in UI.
5. Test tempo ramps, meter changes, reverse clips, warps, comp regions, relocated media, Undo,
   Save As, Collect and Save, recovery, and offline delivery.
6. Build an interoperability matrix using licensed versions of the intended ARA plug-ins.
7. Complete commercial licence and release review before displaying an ARA compatibility claim.

Until those gates are complete, Triumph Studio's own non-destructive edits and offline pitch
renderer remain the supported Phase 14 workflow.
