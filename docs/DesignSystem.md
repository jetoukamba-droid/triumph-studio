# Triumph Studio Design System — Base 1

Status: approved base  
Direction: Cloud Blue controls on a cool neutral grey workspace  
Density: balanced

## Palette

| Role | Value |
|---|---|
| Application background | `#C9CDD2` |
| Workspace | `#BFC4C9` |
| Main panels | `#D9DCE0` |
| Raised panels | `#E2E4E7` |
| Control and clip surfaces | `#F0F2F4` |
| Borders and separators | `#9EA5AC` |
| Primary action | `#2869D8` |
| Primary hover/highlight | `#4F86E3` |
| Selection | `#8DB8FF` |
| Primary text | `#1D2A36` |
| Secondary text | `#5D6873` |
| Record/error | `#D84555` |
| Solo/warning | `#D9822B` |
| Active/success | `#24966D` |

Black backgrounds, gold branding, glossy gradients, and decorative skeuomorphism are excluded.

## Layout

1. Application toolbar: project identity, undo/redo, file actions, audio/CPU status.
2. Transport toolbar: rewind, stop, play, pause, record when implemented, time, tempo, loop, snap, and master output.
3. Left Inspector: selected track/clip properties, routing, inserts, and sends as their engines arrive.
4. Center Arrangement: rulers, tracks, clips, automation, playhead, selections, and editing tools.
5. Right Browser: project media, instruments, effects, samples, presets, favorites, and search.
6. Bottom Dock: mixer, piano roll, audio editor, automation editor, and take editor.

Inspector, Browser, and Bottom Dock must be collapsible. The arrangement must remain usable at 1280×720 and scale cleanly on high-DPI displays.

## Interaction rules

- Blue communicates primary actions, selection, and playback.
- Red is reserved for record-arm, recording, destructive warnings, and errors.
- Orange identifies solo or warning states.
- State must never depend on color alone; use text, icons, borders, or shape changes too.
- No feature is shown as functional before its engine exists.
- UI controls edit the project model through commands. They do not own playback state.
- Audio meters and transport positions are read-only snapshots from the engine.
- All keyboard-accessible commands must have visible focus states.

## Current implementation boundary

Version 0.8.1 adds vertically stacked take lanes inside the existing grey/blue audio-track surface. Inactive lanes remain visible at reduced opacity; the active lane carries a `COMP` text state so selection never depends on colour. `Use Take` and double-click perform the same undoable model command. Green and orange communicate safe and approaching-clip input levels; red remains reserved for record state, clipping, destructive warnings, and errors. Region comping, the full Inspector, Browser, Mixer Dock, controller lanes, automation modes, and multi-slot plug-in rack will be added with their real systems rather than as non-functional decoration.

Version 0.8.2 adds a dedicated compact comp lane above stacked takes. Shift-drag paints a translucent blue selection on the source lane; committed regions appear in the comp lane with a text label identifying their take. Blue remains the interaction and selection colour, while track colours preserve source identity. Crossfade behavior is audible and model-owned rather than represented by decorative controls. Direct boundary-handle editing, the full Inspector, Browser, Mixer Dock, controller lanes, automation modes, and multi-slot plug-in rack remain future work.

Version 0.8.3 adds Alt-drag on a clip's right edit handle for uniform time stretching. A compact blue `RATE 0.80x` label appears only when the clip differs from normal speed, so the state is readable without relying on colour. The interaction reuses the existing resize cursor and never introduces a decorative control. The built-in provider is explicitly varispeed; pitch-preserving labels or controls must not appear until a real pitch-preserving provider exists.

Version 0.8.4 adds slim blue warp lines with triangular handles directly on audio clips. Ctrl-click creates a real source/timeline anchor, Ctrl-drag moves it, and right-click removes it. No separate fake audio editor or toolbar button is shown; every marker immediately changes piecewise playback timing.

Version 0.8.5 adds short teal transient lines in the lower half of audio clips. Opacity communicates detector strength. Transients remain visually distinct from blue warp anchors because analysis alone never changes playback; **Warp All** is the explicit conversion boundary.

Version 0.17 formalizes the 1280 x 720 minimum. Below 1380 pixels, lower-frequency project and selected-clip commands move into a labelled `More` menu while primary transport, recording, panel, Snap, time, tempo, meter, zoom, and master controls remain visible. At 1380 pixels and wider, all direct buttons return. The menu uses the same command availability and handlers as the expanded layout, so responsive presentation never changes behavior.
