# Clip fades and crossfades checkpoint

This checkpoint begins FR-6 project and non-destructive editing completion.

## Delivered

- Audio clips persist independent fade-in and fade-out lengths in timeline samples.
- Selected events expose top-edge fade handles and draw the resulting curves.
- Overlapping events can create an equal-power crossfade from the clip context menu.
- The real-time engine and offline renderer use the same equal-power envelope math.
- Fade progress is anchored to the full event, so warp-marker segment boundaries do
  not restart an event's fade.
- Trim operations clamp fades to the remaining event, stretch operations scale them,
  and split operations retain only the original outer fades.
- Fade and crossfade edits participate in Undo and survive project save/reopen.

## Verification

`ProjectModelTests` now covers:

- fade mutation, Undo, and persistence;
- automatic overlap crossfades;
- warp-segment fade continuity;
- split and stretch invariants; and
- decoded offline-render levels at the fade start, midpoint, body, and final sample.

## Remaining FR-6 work

- persistent Undo history across restarts;
- explicit clip variants/alternate versions;
- stable content fingerprints for relinking; and
- durable repair records for missing or incompatible project state.
