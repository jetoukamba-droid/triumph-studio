# Phase 13 Complete — Tempo and Automation

Phase 13 turns the original fixed-tempo foundation into a project-owned musical timeline.

## Delivered

- Multiple undoable tempo points with step and linear segment shapes
- Closed-form ramp conversion shared by live MIDI, freeze, drawing, and export
- Multiple time-signature events and named timeline markers
- Real meter-aware metronome with distinct bar accents and persisted level
- Gain and pan automation lanes for tracks, returns, buses, and master
- Persisted READ, TOUCH, LATCH, and WRITE modes
- Hold, linear, and smooth curve interpolation
- Timeline-sample-accurate live evaluation at arbitrary device sample rates
- Matching live/offline automation for mix, range, and stem delivery
- Toggleable professional grey/blue Tempo + Automation panel
- Project format 16 with migration from formats 1–15
- New framework-independent `TempoAutomationTests`; nine Windows CTest targets total

## Manual acceptance

1. Configure and build Release, then confirm all nine tests pass.
2. Open Triumph Studio, import audio, and click **Tempo**.
3. Move the playhead, choose a BPM, and click **+ Tempo**; confirm a new point appears.
4. Choose a numerator/denominator and click **+ Signature**; add a project marker.
5. Enable **Metronome**, press Play, and confirm accented bar clicks follow the map.
6. Choose a target and Gain or Pan, choose a mode, then click **+ Lane**.
7. Add points at the playhead or double-click the automation lane. Use the automation
   value control during playback to verify TOUCH, LATCH, and WRITE capture behavior;
   confirm the audible control follows hold, linear, or smooth interpolation.
8. Save, close, and reopen the project; verify the complete timeline persists.
9. Export a mix and verify automation matches live playback while the monitoring
   metronome remains excluded from delivery.
