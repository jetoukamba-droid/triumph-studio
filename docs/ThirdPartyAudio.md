# Third-Party Audio Distribution Gate

Triumph Studio 0.8.4 compiles JUCE with `JUCE_ASIO=1` to expose native ASIO devices on
Windows. The JUCE module can use its bundled ASIO SDK header. This is a source-level
build decision, not permission to distribute a commercial binary without review.

Before any paid, trial, beta, or public Windows installer is shipped:

1. Confirm the company's JUCE licence covers the product, team, revenue, and release model.
2. Review the ASIO SDK licence that accompanies the exact JUCE/SDK revision in the build.
3. Confirm product naming, notices, source changes, redistribution, and trademark usage with
   qualified counsel or a documented internal licence review.
4. Record the JUCE commit/tag, compiler, enabled audio backends, and third-party notices in
   the reproducible release manifest.
5. Test signed builds with current vendor drivers; Triumph Studio does not redistribute or
   replace Focusrite or other hardware-vendor drivers.
6. Confirm that hosted VST3 binaries are discovered from user-installed locations and are not
   bundled, copied, or redistributed without each vendor's express licence.

Windows Audio remains available as a fallback. It can operate class-compliant devices through
Windows, but it is not a universal replacement for a vendor's low-latency driver and control
software. Triumph Studio owns device selection, capture, monitoring, metering, and file safety;
the operating system or hardware vendor owns the kernel-level device driver.

The built-in stretch provider uses JUCE resampling and requires no separately distributed
time-stretch SDK. It is varispeed: changing duration also changes pitch. Phase 14's separate
offline pitch command uses Triumph Studio's own overlap-add implementation and does not add a
third-party binary dependency. It is a functional foundation, not a claim of parity with a
licensed commercial elastique-style processor. Any future replacement provider must pass a
separate licence, quality, latency, and redistribution review before it can be enabled in a
commercial release.

ARA is not enabled or bundled in Phase 14. Any future ARA integration must use the official
SDK under its applicable licence and complete compatibility testing with the intended plug-ins;
see `ARAResearch.md` for the proposed boundary.

Phase 15 adds no AI SDK, cloud inference service, model weights, training corpus, sample pack,
or third-party drum library. Producer-assistant logic and the built-in drum synthesis are
original deterministic source code in this repository. A future learned-model integration
requires a separate model, dataset, privacy, telemetry, redistribution, and commercial-use
review before it may replace or supplement the current local engine.

Phase 16 adds no support-chat SDK, search service, account provider, telemetry client, or web
dependency. Triumph Assistant uses original bundled help text and deterministic local matching.

Phase 17 adds no third-party runtime, binary, SDK, service, model, or data dependency. Its
responsive-layout policy, warning cleanup, tests, and release documentation are original source.

Phase 18A also adds no third-party runtime, binary, SDK, service, model, or data dependency.
Its RCU publication, bounded parameter queue, telemetry counters, callback audits, and stress
tests are original Triumph Studio source and retain the existing JUCE 8 boundary.
