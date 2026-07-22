# Triumph Studio Phase 5 Release Gate

Version 0.5.2 is the first hardware-input recording milestone. Maintenance revisions
correct JUCE 8 Windows const-qualification and output-stream API compatibility.

## Delivered

- Persistent JUCE audio-device settings with a dedicated Device dialog
- Up to two active hardware inputs and two outputs
- Empty audio-track creation and exclusive per-track record-arm
- Automatic armed-track creation when Record is pressed without a target
- Timestamped mono/stereo 24-bit WAV capture in the project `Recordings` folder
- Bounded FIFO transfer from the audio callback to a background writer thread
- Recording transport that continues beyond the previous project end
- Reported input-latency compensation at take insertion
- Optional software input monitoring, off by default
- Format version 5 persistence for record-arm state
- Unit coverage for arm exclusivity, recorded-take insertion, format version, and latency math

## Operator flow

1. Save the project or press Record and choose its project file.
2. Open Device and select the driver, input channels, sample rate, and buffer size.
3. Create an empty track with `+ Track` and arm its `R` button, or let Record create one.
4. Use headphones before enabling Monitor.
5. Press Record. Press Record again or Stop to close and place the take.
6. Save the project. The WAV already exists in `Recordings/`; Save persists its clip reference.

## Honest boundary

Phase 5 records one mono or stereo take to one empty track at a time. Punch ranges,
pre-roll/count-in, loop recording, take lanes, comping, waveform growth during capture,
input gain meters, dropout telemetry, manual record offsets, direct-monitor control, and
multi-device drift correction remain later commercial release gates. Plug-in hosting and
an immutable lock-free render graph are also not part of this milestone.
