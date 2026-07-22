# Phase 12 Complete — Mixer

Triumph Studio 0.12.0 adds one persisted mixer model shared by live playback,
offline mix export, range export, and stem delivery.

## Delivered

- Toggleable docked mixer with track, return, bus, and master strips
- Undoable return-channel and bus creation/deletion
- Track, return, and bus output selection
- Multiple per-owner audible sends with pre/post-fader taps
- Multiple sidechain routes with envelope-driven destination ducking
- Acyclic graph validation and last-valid-plan protection
- Per-channel gain, pan, mute, solo, stereo meters, and sidechain reduction display
- Persisted master mute and smoothed master fader
- Lock-free analyzer sample transfer and live 2048-point spectrum display
- Project format 15 save/load and formats 1–14 migration
- Eight CTest targets, including framework-independent mixer graph coverage

## Windows acceptance test

1. Configure and build Release, then require all eight CTest targets to pass.
2. Open Triumph Studio and import an audio file.
3. Open **Mixer**, create **+ Return** and **+ Bus**, and route the audio track to the bus.
4. Add an audible send to the return. Switch **PRE** on and off while moving the track fader.
5. Add a sidechain route to the bus and confirm the orange gain-reduction indicator responds.
6. Confirm channel meters and the master spectrum respond during playback.
7. Save, close, reopen, and confirm outputs, sends, channel settings, and master mute persist.
8. Export a mix and confirm it follows the same bus/return routing heard during playback.

Phase 12 acceptance is complete when sends, returns, buses, sidechains, and the master
strip remain stable through playback, save/reopen, and deterministic export without a
routing cycle reaching the audio callback.
