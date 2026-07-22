# Triumph Studio Phase 6 Release Gate

Version 0.6.0 is the professional audio-I/O hardening milestone.

## Implemented

- JUCE ASIO backend enabled for Windows builds
- Windows Audio, exclusive, low-latency, and DirectSound fallbacks retained
- Independent input-channel selection instead of forced stereo pairs
- True mono 24-bit WAV capture when only one hardware input is active
- Two-channel input peak metering measured before playback rendering
- Green, orange, and red level ranges with a 1.5-second clip hold
- Atomic recording FIFO dropout counters on the real-time path
- Immediate in-recording disk-drop status and post-take lost-duration warning
- Continued reported input-latency compensation and safe background file writing
- Unit coverage for sample-to-millisecond dropout reporting

## Acceptance gate

Before this phase is considered validated on Windows:

1. Configure and build Release with Visual Studio.
2. Run all tests and require 100% passing.
3. Open Device and confirm both ASIO and Windows Audio backends are available.
4. Select Scarlett Solo Input 1 only and confirm the take is a mono WAV.
5. Speak into Input 1 and confirm the first input meter responds without monitoring.
6. Drive the input into clipping and confirm the red indicator remains visible briefly.
7. Record, stop, replay, save, close, and reopen the project.
8. Confirm a normal take produces no dropout warning.

## Honest boundary

Phase 6 does not yet provide aggregate devices, automatic sample-rate conversion between
independent input and output devices, manual record offset calibration, round-trip latency
measurement, multi-device clock synchronization, waveform growth during capture, punch,
pre-roll, take lanes, or comping. ASIO distribution obligations must be resolved before a
commercial installer is released; see `ThirdPartyAudio.md`.
