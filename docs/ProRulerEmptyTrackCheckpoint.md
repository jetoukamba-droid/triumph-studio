# Pro Ruler and Empty Track Checkpoint

This checkpoint tightens the arrangement model around a professional DAW
project-window pattern: the track list is separate from the event display, the
ruler is a time scale across the event display, and clips or MIDI parts appear
only when there is recorded, imported, or explicitly edited content.

## Scope

- Added `core/ArrangementLayout.h` as the shared source of truth for the track
  header width, event-display inset, ruler origin, and track-height limits.
- Removed the visible `TIMELINE` panel label from the ruler paint path.
- Kept track height drag behavior intact while making the clamp range explicit.
- Changed instrument-track creation so new tracks are empty.
- Changed MIDI recording and explicit MIDI note entry so the first MIDI part is
  created on demand.

## Verification

- `ArrangementLayoutTests` checks ruler/event-display origin alignment,
  track-height clamping, no ruler panel label, and no placeholder clip on new
  instrument tracks.
- `ProjectModelTests` checks empty instrument-track creation, MIDI recording
  creating the first part, undo returning to an empty armed track, and explicit
  MIDI note entry creating a part on demand.
