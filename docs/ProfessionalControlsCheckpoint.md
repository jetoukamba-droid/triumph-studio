# Professional Controls and Shortcuts Checkpoint

Triumph Studio 0.21.6 pauses roadmap phase expansion to improve the daily
editing surface. This remains an internal alpha checkpoint.

## Delivered

- Project and transport buttons use resolution-independent paths and drawing
  primitives rather than emoji, Unicode symbols, bitmap assets, or an icon font.
- Icons preserve the existing enabled, pressed, toggle, record, play, import,
  export, and warning colours.
- Every icon button retains a descriptive button name for accessibility and a
  discoverable tooltip; focused buttons receive an additional blue focus ring.
- Clicking toolbar buttons does not steal the main shortcut focus.
- One-key commands cover Play/Pause (`Space`), Record (`R`), Stop and return
  (`.`), Return to start (`W` or `Home`), Monitor (`M`), Mixer (`B`), Tempo and
  Automation (`A`), Snap (`S`), and timeline zoom (`+`/`-`).
- Project commands cover New, Open, Save, Save As, Undo, Redo, Import Audio,
  Export Mix, Add Audio Track, Add Instrument Track, Split, Delete, and Help.
- Recording and offline-processing guards prevent destructive edit shortcuts
  from bypassing the same safety rules as the visible controls.
- `KeyboardShortcutsTests` verifies the public map, case normalization,
  modifier distinctions, and absence of duplicate gestures.

## Honest boundary

This checkpoint adds direct shortcut routing and accessible icon controls. It
does not yet implement a user-editable key-command preferences system, multiple
keymap presets, control-surface assignment, localization, or a complete
screen-reader audit. Those remain release gates.

## Manual acceptance

1. Hover every toolbar symbol and confirm its command name and shortcut appear.
2. Press `Space` twice and confirm playback starts and pauses without rewinding.
3. Press `.` and confirm transport stops and returns to the project start.
4. Press `R` on an armed audio track and confirm recording starts; press `R`
   again and confirm the take closes safely.
5. Press `B`, `A`, `M`, and `S` and confirm the corresponding real controls
   change state.
6. Verify `Ctrl+T`, `Ctrl+Shift+T`, `Ctrl+I`, and `Ctrl+Shift+E` invoke their
   visible toolbar commands.
7. Run the complete Release test suite before using the build.
