# Plug-in Discovery, Hosting, and Isolation Recovery Package

Triumph Studio 0.20.0 delivers the source-complete FR-3 recovery checkpoint while
retaining the honest internal-alpha status.

## Delivered

- Recursive standard-folder VST3 discovery that runs every untrusted probe in the
  separate timeout-bounded scanner process.
- Version-2 atomic registry with migration from version 1, stable identity and
  descriptive metadata, file fingerprints, availability, attempt/failure counts,
  deterministic unchanged-file skipping, explicit retry, and blocklist removal.
- Explicit live instrument bus policy: non-main buses are disabled when supported,
  stereo is preferred, mono is accepted, and a plug-in with no output is rejected.
- Stable hosted parameter IDs with deterministic legacy fallback, visible parameter
  selection, persistent automation lanes, and bounded sample-offset automation
  delivery in live playback and instrument freeze.
- Atomic `.triumphpreset` save/load bound to stable plug-in identity and checked for
  state-payload corruption before live application.
- One bounded 1–30 second natural-tail rule for supported live and freeze paths, plus
  explicit 10 ms bypass/removal and immediate fault-mute behavior.
- Per-plug-in last/max process time and deadline misses in the real-time tooltip.
- Exception-safe live processing: a C++ exception clears output, bypasses the slot,
  records the fault, and retains the project-owned unavailable placeholder.
- Missing plug-in identity, processor state, automation, and repair action remain in
  the project rather than deleting the instrument track.

## Verification included

- `PluginScanServiceTests`: nonzero exit, timeout, malformed/missing output, real
  scanner protocol error, registry v1 migration, metadata/fingerprints, blocklist,
  retry, recursive discovery, and deduplication.
- `PluginHostPolicyTests`: mono/stereo/no-output decisions, stable and fallback
  parameter IDs, bounded natural tails, bypass/removal fades, and fault mute.
- `PluginPresetStoreTests`: round-trip, wrong-plug-in rejection, checksum corruption,
  and atomic replacement.
- `ProjectModelTests`: persistent plug-in parameter automation identity and points.
- Existing callback audit: no locks, file access, reader creation, resizing, or
  container growth was introduced into Triumph-owned playback/recording callbacks.
- Strict C++20 warnings-as-errors syntax validation for every changed translation unit.

## Deliberately still open

This package does not claim native runtime sandboxing. `processBlock` still executes in
Triumph Studio's process; access violations and hangs require a real-time shared-memory/
IPC worker design, watchdog, checkpoint, and deterministic failover. The current host
also remains one live VST3 instrument slot. Broad sidechain/multi-output bus exposure,
host-native VST3 parameter queue/ramp parity, and broad real-plug-in compatibility
certification remain future work. The Windows target must pass all 25 tests before
this checkpoint is accepted.
