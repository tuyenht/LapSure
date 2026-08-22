# LapSure Roadmap

## Completed architecture milestones
Hardware inventory/core health collection; EDID accuracy gate; stress session and event-delta architecture; trusted external-engine gate; telemetry and decision model; functional test center; camera/mic/stereo/Wi-Fi/Bluetooth I/O; physical port/power verification architecture; guided Test Orchestrator; model-aware chassis profiles; MSVC/portable/real-machine validation scaffolding.

## Current — Beta 0.1: Build & Real-Machine Validation
1. Compile on Windows with MSVC x64 Release.
2. Fix compiler/runtime failures before broad feature additions.
3. Run end-to-end on a real Precision 5560/5570/7670.
4. Validate exact chassis profiles and physical-port labels.
5. Compare inventory/health evidence against BIOS, Dell diagnostics and independent reference tools.
6. Build a false-positive/false-negative log.
7. Validate USB portable launch and custom WinPE behavior.

## Beta 0.2 — Professional Dashboard
After Beta 0.1 is stable: component summary cards; clear PASS/WARNING/FAIL/NOT TESTED hierarchy; persistent guided stepper; model-aware chassis visualization; evidence drill-down; report preview/export; improved localization and Vietnamese-first technician copy.

## Beta 0.3 — Provider & Profile Hardening
Expand verified Dell model/profile coverage; trusted sensor-provider packaging; trusted VRAM-engine release pinning; deeper storage/adapter validation; controlled benchmark baseline fleet.

## RC — Portable / WinPE Certification
One portable package policy; WinPE capability matrix; driver/provider fallback matrix; signed-release strategy; deterministic build/version metadata; regression matrix across supported Windows versions.

## 1.0
Release criteria are based on reproducible real-machine evidence, not the number of implemented probes.