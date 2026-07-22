# Known issues in 0.22.0-beta.1

- Live third-party audio effects use an explicitly trusted in-process mode.
  A native plug-in crash or hang can still terminate or freeze the host.
- Native MIDI 2.0 device and plug-in transport is not connected; compatible
  MIDI 1/MPE delivery is used for JUCE VST3 hosting.
- MIDI Clock output, SysEx, LTC, Ableton Link, and control-surface support are
  not available.
- Persistent Undo history across application restarts is not implemented.
- The release has not completed a broad ASIO/WASAPI hardware matrix, a broad
  VST3 matrix, screen-reader certification, or long-session qualification.
- The ZIP is not an installer. Its executables are not code-signed by this
  build process, and Windows may display a reputation warning.
- Do not use the beta as the only copy of a project, recording, preset, or
  source file.
