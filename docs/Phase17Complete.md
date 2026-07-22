# Historical Phase 17 Checkpoint - Superseded Beta Claim

This document records what the Phase 17 checkpoint changed; it is not the current maturity
assessment. The later 150-finding source audit withdrew the Beta Candidate label and classified
the code as an internal alpha with 53 built, 52 partial, and 45 missing findings. Current work and
release gates are defined in `FoundationsRecoveryRoadmap.md` and `BetaReleaseChecklist.md`.

Phase 17 preserved the Phase 16 feature set and concentrated on predictable Windows layout,
clean diagnostics, and regression coverage. Project format 18 was unchanged at that checkpoint.

## Delivered

- Supported minimum window size of 1280 x 720
- Compact project and transport layouts below 1380 pixels
- One real `More` menu for lower-frequency project and selected-clip commands
- Shared command handlers and enabled state between compact menu items and full-width buttons
- Expanded direct-button layout at 1380 pixels and wider
- Replacement of the deprecated JUCE plug-in editor call
- Explicit offline-render channel initialization for strict warning-clean compilation
- ASCII-safe user-facing source text to prevent Windows encoding artifacts
- Framework-independent `ResponsiveLayoutTests`
- Complete regression suite expanded to thirteen Windows tests
- Release checklist separating completed engineering from remaining production gates

## Manual acceptance

1. Launch at 1280 x 720 and confirm both toolbar rows remain usable without clipped controls.
2. Open `More` and confirm Recent, Collect, Relink, Low Latency, Split, Use Take, Detect, and Warp All appear.
3. With no clip selected, confirm clip commands in `More` are disabled.
4. Import audio, select a clip, and confirm Split, Detect, and Warp All follow the same availability as at full width.
5. Resize to at least 1380 pixels and confirm direct toolbar buttons return and `More` disappears.
6. Open Help > About and confirm Triumph Studio 0.17.0 / Phase 17.
7. Scan visible labels and status text for encoding artifacts.
8. Save, close, reopen, and confirm the format-18 project and media are unchanged.

The Phase 17 Beta Candidate claim is superseded and must not be used in product labeling.
