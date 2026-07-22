# Plug-in Isolation Boundary

Triumph Studio treats third-party plug-in discovery as untrusted work. VST3 scanning no
longer runs inside the application process. This is a foundations-recovery boundary, not a
claim that live plug-in audio processing is sandboxed.

## Discovery process

`Triumph Plugin Scanner` is built as a separate console executable and copied beside
`Triumph Studio.exe`. The application starts it with an argument array rather than a shell
command, supplies one selected VST3 path and one unique temporary result path, and waits on a
worker thread. The helper has 30 seconds to finish. A timeout is terminated and classified as
a blocked scan; a nonzero exit, missing result, or malformed result is classified as a scanner
failure. None of these paths can prevent the already-running application from continuing.

The scanner writes `TriumphPluginScanResult` format 1 as JSON through a temporary file and
atomic replacement. Each discovered type is serialized using JUCE's plug-in-description XML,
which retains its identifier inputs, format, path, vendor, version, category, instrument flag,
channel counts, container flag, and ARA capability.

## Persistent registry

The application atomically updates the machine-local `plugin-registry.json` after every scan.
`TriumphPluginRegistry` format 2 stores canonical path, `validated` or `blocked` status,
failure text, scanner exit code, attempt/failure counts, successful-scan time, file size and
modification fingerprint, availability, stable identifier, display name, vendor, version,
category, format, instrument/ARA flags, channel counts, and validated descriptions. Format 1
registries remain readable. Standard-folder discovery recursively identifies VST3 bundles,
deduplicates paths, skips unchanged entries (including unchanged blocked entries), and scans
only new or changed binaries. A user-requested registry retry always scans again. Missing files
remain visible without deleting their last known identity or status.

Projects own the selected description and processor state. If instance creation or restoration
fails, the instrument track remains present with an unavailable placeholder and repair action;
editor-window destruction does not destroy project-owned identity or state.

The live processor also reports latency changes through `AudioProcessorListener`. That callback
publishes only atomic latency/pending values. The interface timer consumes the notification,
updates project health, and republishes the prepared delay-compensation plan on the control side.

Host bypass retains the processor rather than unloading it. The engine suppresses new note input,
sends all-notes-off, continues calling the processor, fades its output over 10 ms, and keeps the
reported latency in the compensation graph. This preserves processor state and avoids an abrupt
parallel-path timing jump when bypass changes.

The host disables non-main buses when the processor permits it, prefers a stereo main output,
accepts a mono fallback, and refuses instruments with no usable output. Hosted parameter IDs are
used directly; legacy parameters receive a deterministic index/name fallback. Plug-in automation
is collected into a bounded event list and delivered by splitting the current instrument/insert
process block at changed sample offsets. This is real sample-offset delivery for the current host
paths, but broad host-native VST3 queue/ramp parity remains a compatibility gate.

Natural tails use one bounded 1–30 second rule for live note/stop and instrument freeze. Bypass
and removal use a 10 ms host transition and do not promise a natural tail; a processing fault
mutes immediately. Presets use atomic replacement and bind state to stable plug-in identity plus
a payload checksum before applying it to a live instance.

Each live `processBlock` duration, maximum, and deadline miss is published through atomics. A C++
exception escaping a plug-in is caught, the output is cleared, the slot is bypassed, and the
project retains an unavailable repair placeholder. Access violations, native crashes, and hangs
cannot be safely recovered inside this process; they remain the runtime-isolation gate below.

## Test boundary

`PluginScanServiceTests` launches five subprocess cases:

- a helper that exits with a distinct failure code;
- a helper that hangs until the service timeout terminates it;
- a helper that writes malformed output;
- a helper that exits successfully without writing output;
- the real scanner returning a valid protocol error for a non-plug-in file.

The same test verifies registry migration, metadata, file fingerprints, deterministic retry,
recursive/deduplicated discovery, blocked-to-validated replacement, and description round-trip.
`PluginHostPolicyTests` verifies bus, stable-ID, and tail decisions.
`PluginPresetStoreTests` verifies identity rejection, checksum corruption detection, and atomic
replacement. Windows acceptance must additionally use real scan-crashing VST3 fixtures.

## Deliberately still open

Live `processBlock` execution currently remains in the application process. Grouped or
per-plug-in runtime isolation requires a separate real-time IPC design with bounded shared
audio/MIDI/parameter transport, latency and tail contracts, watchdog policy, state checkpoints,
and deterministic fallback after worker failure. The current helper protocol is intentionally
narrow and must not be reused as an audio-thread message channel.
