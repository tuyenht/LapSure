# LapSure

**Laptop Verification & Diagnostics for Windows and WinPE**

> **Kiểm đúng máy. Biết đúng tình trạng. Mua đúng giá.**

LapSure is a native Windows diagnostic and verification tool for used-laptop inspection, technicians, resellers and buyers who need evidence rather than assumptions. It combines hardware identity, health checks, stress/stability testing, functional I/O verification, physical-port testing, model-aware chassis profiles and structured reports in one guided workflow.

## Status
**v0.1.1-beta — Evidence Correctness & Executable Regression Gate**

The architecture, strict Windows build and portable packaging gates are in place. Beta 0.1.1 is converting migration-era source assertions into executable evidence-policy tests and hardening false-PASS boundaries before physical-machine validation. A build or source-level PASS is **not** proof that every diagnostic path is production-ready.

## What LapSure verifies
- Identity/configuration: model, Service Tag where exposed, BIOS, CPU, RAM, GPU, storage, battery, display and EDID.
- Storage health: SMART/NVMe evidence and endurance/usage indicators where supported.
- Battery: design/full-charge capacity, wear evidence and charge state.
- Display: EDID native timing, current mode, touch presence and visual-test workflow.
- Stability: CPU sustained load, online partial-coverage RAM test, trusted VRAM-engine adapter, pre/post hardware-event deltas.
- Functional I/O: camera frame capture, microphone signal, stereo L/R, Wi-Fi association/signal and Bluetooth radio access.
- Physical ports: stimulus-based plug/unplug verification, PnP/location evidence and USB4/Thunderbolt topology signals.
- Factory/model profiles: exact factory profiles where available plus model-aware chassis/required-port profiles.
- Reports: evidence-oriented HTML/JSON with PASS / WARNING / FAIL / NOT TESTED / INCOMPLETE semantics.

## Core principle: never fake a PASS
LapSure distinguishes **detected**, **tested**, **verified**, and **not tested**. A detected GPU does not prove VRAM integrity; a USB controller does not prove every physical port; camera presence does not prove frame delivery; online Windows RAM testing is not full preboot memory certification; missing/untrusted engines remain NOT TESTED or WARNING.

## Guided workflow
1. Automatic Hardware Audit
2. Functional Verification
3. Physical Ports & Power
4. Final Evidence Review

## Supported environment
Primary target: Windows 10/11 x64, Visual Studio 2022/MSVC, native C++20/Win32. The core is dependency-light so the same diagnostic engine can also run in a custom x64 WinPE environment, with unavailable capabilities reported honestly.

## Build
From **x64 Native Tools Command Prompt for VS 2022** run `build_msvc_x64.cmd`; use `build_msvc_ci.cmd` for strict CI. Run `run_source_tests.cmd` for the complete source regression suite. The portable packager is `package_portable.ps1`; it creates `LapSure-windows-x64-portable.zip`, writes the executable SHA-256 inside `BUILD_HASH.txt`, and emits a separate ZIP checksum file.

## Initial model-aware profiles
Initial chassis-profile architecture covers Dell Precision 5560, 5570 and 7670. These profiles require official-reference and physical-machine validation before production certification.

## Documentation
- [Product Specification](docs/PRODUCT_SPEC.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Roadmap](docs/ROADMAP.md)
- [Brand & Product Naming](docs/BRANDING.md)
- [Contributing](CONTRIBUTING.md)
- [Security](SECURITY.md)

## Validation policy
A model/profile is not production-certified until it passes real-machine validation. The initial target is at least two physical units per model for chassis/port-profile certification.

## License
No open-source license has been selected yet. Until a license is explicitly added, copyright remains with the repository owner and normal GitHub viewing/forking mechanics do not grant a general redistribution license.
