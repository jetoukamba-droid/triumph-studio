# Phase 9.1 — Professional WAV Delivery Presets

Triumph Studio 0.9.6 adds honest delivery choices on top of the validated deterministic
offline renderer. The Windows build explicitly compiles source text as UTF-8, while popup
menu preset separators use portable ASCII so labels remain correct across Windows locales.

## Delivered

- Master: 48 kHz, 24-bit WAV, two-second tail.
- Distribution: 44.1 kHz, 24-bit WAV, two-second tail.
- CD: 44.1 kHz, 16-bit WAV, two-second tail.
- Hi-Res: 96 kHz, 24-bit WAV, two-second tail.
- No-tail master: 48 kHz, 24-bit WAV ending at the exact project boundary.
- Fixed-seed TPDF dither scaled to the selected integer bit depth.
- Completion reporting that names the actual exported format.

## Acceptance gate

1. Require all five CTest suites to pass in Release.
2. Export the same short project with Master, CD, Hi-Res, and no-tail presets.
3. Reopen every WAV and verify its sample rate, bit depth, channel count, and duration.
4. Confirm the no-tail file ends at the project boundary and the other files add exactly
   two seconds.
5. Confirm repeated exports of an unchanged project and preset are byte-identical.

## Honest boundary

Phase 9.1 exports the full project timeline to integer PCM WAV. It does not yet claim
selection/range export, stems, broadcast metadata, FLAC/MP3 delivery, loudness targeting,
true-peak limiting, sample-rate-converter quality modes, or external VST3 offline parity.
Projects using an external VST3 remain explicitly blocked rather than silently incomplete.
