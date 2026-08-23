# Changelog

## v0.1.1-beta — Evidence Correctness & Executable Regression Gate
- Added compiled C++ behavioral tests to Windows CI.
- Required-port and runtime-validation gaps now block acceptance verdicts.
- Empty port evidence remains INCOMPLETE and retests replace the current per-port result.
- SMART requires an explicit health verdict; malformed/incomplete output cannot PASS.
- VRAM parsing uses numeric error counts, valid process completion and mode-specific duration.
- RAM reported coverage now reflects byte-for-byte verification.
- Reports are written as checked UTF-8 and JSON includes runtime/port/chassis/orchestrator evidence.
- Hardened process cancellation, worker exception handling, atomic stress journal writes and portable build provenance.

## v0.1-beta — Build & Real-Machine Validation Gate

### Product rename
- Project renamed to **LapSure**.
- Binary/CMake target standardized as `LapSure` / `LapSure.exe`.
- Report directory standardized as `LapSureReports`.
- Positioning standardized as **Laptop Verification & Diagnostics**.

### Current capabilities
Inventory/identity; EDID/native display verification; storage/battery health evidence; stress/event-delta framework; online partial-coverage RAM test; trusted VRAM/sensor adapters; camera/microphone/stereo/Wi-Fi/Bluetooth functional I/O; physical-port stimulus verification; model-aware chassis profiles; guided Test Orchestrator; HTML/JSON evidence reports; runtime/build validation gate.

### Validation phase
MSVC x64 presets and strict CI build; portable packager; real-machine matrix/checklist; initial Precision 5560/5570/7670 chassis profiles.
