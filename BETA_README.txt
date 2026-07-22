TRIUMPH STUDIO 0.22.0-beta.1 - CLOSED BETA 1

This is a prerelease engineering build for controlled testing. It is not a
production release and must not be trusted as the only copy of a project or
recording.

Before testing:

1. Keep independent backups of projects and media.
2. Open Device and run Test Output before opening important work.
3. Use copied media and a copied project for plug-in testing.
4. Read KNOWN_ISSUES.md and Beta1Acceptance.md.

The package contains Triumph Studio.exe and its required companion
TriumphPluginScanner.exe. Keep both files together.

When running from a source build, prefer tools\LaunchTriumphStudio.ps1. It
finds the generated Release executable, repairs the scanner helper copy if the
build tree has it in a sibling artifact folder, and then launches the app.

Report a problem with the exact action, audio device type, sample rate, buffer
size, project steps, and whether a Windows crash dump was created.
