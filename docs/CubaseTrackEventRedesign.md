# Track and Event Redesign Checkpoint

This checkpoint makes the arrangement interaction model explicit and removes
the permanent destructive control from every track header.

## Track headers

- Audio and instrument tracks have distinct type badges.
- Clicking a header selects the track.
- Right-clicking the header or empty lane opens the track menu.
- The track menu contains Track Information, Rename, Mute, Solo, Record Arm,
  and Delete Track.
- Delete Track still uses the application-level confirmation dialog and Undo.
- The former red `x` track button is removed.
- Creating an audio or instrument track creates only the track header/lane.
  The event display stays empty until import, recording, or explicit MIDI note
  entry creates an audio event or MIDI part.

## Ruler and event display

- The ruler is a straight time scale aligned to the event display origin.
- The ruler is not painted as a titled panel and does not show a permanent
  `TIMELINE` label.
- Track height dragging remains a row operation: dragging the row bottom edge
  down enlarges the lane, and dragging up reduces it within the bounded range.

## Audio events and MIDI parts

- Events use a dedicated title strip above their waveform or note body.
- Event names remain visible independently of waveform density.
- Selected audio events show bright left/right trim edges and lower-corner
  grips that correspond to the existing direct trim gestures.
- Alt-drag on the right edge keeps the existing varispeed stretch behavior.
- Right-clicking an audio event exposes Clip Information, Rename, Duplicate,
  Mute, Lock, Reverse, Split, Colour, and Delete.
- MIDI parts use the same header/body hierarchy and display their track name.

## Verification

- The permanent TrackRow delete-button code is absent.
- The track and clip information/menu paths are present.
- Twenty-seven portable regression executables pass in the source checkpoint.
- A full Windows Release build and CTest run remain required after extraction.
