# Phase 9.2 — Cancellable Atomic Delivery

Triumph Studio 0.9.7 exposes the renderer's cancellation contract as a real user workflow.

## Delivered

- Export Mix becomes Cancel Export while a render is active.
- A second click requests cancellation without blocking the message thread.
- The toolbar reports cancellation progress and completion without presenting cancellation
  as an application error.
- The renderer closes and deletes its `.partial` file after cancellation.
- A pre-existing destination is never modified until the complete replacement is ready.
- The integration suite verifies cancelled output cleanup and destination preservation.

## Acceptance gate

1. Require all five Release CTest suites to pass.
2. Start a Hi-Res export of a multi-minute project.
3. Confirm the button changes to Cancel Export and progress remains responsive.
4. Cancel before completion and confirm the status reads Mix export cancelled.
5. Confirm no `.partial` file remains and an earlier file at the destination is unchanged.
6. Export again without cancelling and confirm normal delivery still succeeds.

## Honest boundary

Cancellation is cooperative at renderer block boundaries. It does not yet persist export jobs
across application restarts or provide a queue of simultaneous deliveries. External VST3
projects remain blocked until safe offline graph cloning or freeze is implemented.
