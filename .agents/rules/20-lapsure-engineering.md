# LapSure Engineering Rule
Recommended activation: **Always On** for source changes.

- C++20/Win32; strict MSVC x64 Release.
- No Electron/Chromium/WebView/Node/Python runtime for UI convenience.
- Slow diagnostics off UI thread.
- Preserve bounded cancellation, journal and SHA-256 trust policy.
- No production demo/sample values.
- Model/provider extensions require real source/evidence and tests.
- Run repository-supported build and regression gates; unavailable gates are NOT RUN, never falsely PASS.
