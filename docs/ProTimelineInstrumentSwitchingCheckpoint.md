# Pro timeline and instrument switching checkpoint

This checkpoint continues the original roadmap after FR-6 fades/crossfades.

## Delivered

- Instrument tracks now expose a same-track instrument selector.
- Built-in instrument choices include Triumph Piano, Triumph Synth, Triumph
  Drums, and Triumph Sampler.
- Switching built-in instruments is undoable, clears stale external plug-in
  state, and preserves the track, MIDI clips, notes, colour, mute/solo/arm
  state, sends, and automation ownership.
- The arrangement now has a dedicated professional timeline ruler above the
  lanes instead of relying only on per-track grid drawing.
- The ruler draws bars and beats from the project tempo/signature.
- Clicking the ruler seeks the playhead.
- Dragging in the ruler creates a playback loop range.
- Dragging locator handles adjusts loop start/end.
- Double-clicking the ruler clears the loop range.
- The audio engine wraps transport playback at the saved loop range so audio,
  MIDI, metronome, and plug-in playback follow the same locator.

## Verification

`ProjectModelTests` covers same-track built-in instrument switching, Undo, and
MIDI-note preservation. A full CMake/JUCE build still needs to run on the
Windows reference machine because this container does not include CMake or a
JUCE checkout.
