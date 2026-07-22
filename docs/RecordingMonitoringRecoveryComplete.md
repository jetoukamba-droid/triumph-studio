# Recording and Monitoring Recovery Package

Triumph Studio 0.19.0 implements the source-complete recording recovery package
without changing the honest internal-alpha status.

## Delivered

- Explicit audio transport states for arm, pre-roll, normal recording, punch,
  loop pass, finalize, abort, and recovery.
- Exact capture-span slicing at project/device sample-rate boundaries.
- Independent input route, hardware/device-output tap, and signed record offset
  for every armed audio track.
- Simultaneous bounded capture through one shared background writer service.
- A pre-opened independent WAV and recovery journal for every loop pass and track.
- Loop passes committed as ordinary stable take lanes in the project model.
- Version 20 project persistence and migration from formats 1 through 19.
- Off, While Armed, and Always software monitor modes through Control Room gain,
  dim, and mute after project master gain.
- Honest device capability reporting: portable hardware direct-monitor control is
  driver-managed; dedicated cue/listen/talkback paths are not simulated.

## Verification included

- 17 deterministic framework-independent test executables.
- Linked journal version/migration test.
- Linked multitrack WAV test that decodes the result and verifies routed sample
  content, channel layout, per-track alignment, loop-pass ownership, and cleanup.
- Strict C++20 syntax checks with `-Wall -Wextra -Werror` for every changed JUCE
  translation unit.
- Static lock/allocation/file-I/O audit for playback, the individual writer
  callback, and the multitrack dispatcher.

## Target gates still required

The Windows Release suite now contains 23 tests and must be run on the target
machine. FR-2 remains open until a physical split-signal alignment run and an
administrator-forced process termination demonstrate recovery of every readable
simultaneous take after relaunch. Larger-than-stereo device surfaces and dedicated
cue/listen/talkback routing remain future recovery work.
