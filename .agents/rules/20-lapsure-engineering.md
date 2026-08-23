# LapSure Engineering Rule

Recommended activation: **Always On** for source changes.

- Keep C++20/Win32 and strict MSVC x64 Release build compatibility.
- Do not introduce Electron/Chromium/WebView or a heavy runtime for UI convenience.
- Keep slow diagnostics off the UI thread.
- Preserve bounded cancellation and crash-safe journal behavior.
- Preserve SHA-256 allowlist/trust behavior for external providers.
- Never hard-code demo/sample diagnostic values in production rendering.
- Add model/provider fields only with real source/evidence and tests.
- After changes run repository-supported build, `run_source_tests.cmd`, behavioral/regression tests and inspect the diff.
- If a gate cannot run in the current environment, report it as not run; do not claim success.
