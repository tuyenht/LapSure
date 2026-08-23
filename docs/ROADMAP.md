# LapSure Roadmap

## Completed architecture milestones
Hardware inventory/core health collection; EDID accuracy gate; stress session and event-delta architecture; trusted external-engine gate; telemetry and decision model; functional test center; camera/mic/stereo/Wi-Fi/Bluetooth I/O; physical port/power verification architecture; guided Test Orchestrator; model-aware chassis profiles; MSVC/portable/real-machine validation scaffolding.

## Current — Beta 0.1.1: Evidence Correctness & Executable Regression
1. Keep MSVC x64 Release at `/W4 /WX` with executable behavioral tests.
2. Prevent missing, malformed, timed-out or untrusted evidence from producing PASS/BUY.
3. Validate SMART/VRAM parsers, decision policy, required-port coverage and report contracts with fixtures.
4. Bound cancellation/process shutdown and make stress journals crash-safe.
5. Run end-to-end on a real Precision 5560 after the evidence gate is green.
6. Validate exact chassis profiles and physical-port labels.
7. Compare inventory/health evidence against BIOS, Dell diagnostics and independent reference tools.
8. Build a false-positive/false-negative log.
9. Validate USB portable launch and custom WinPE behavior.

## Beta 0.2 — Professional Dashboard
After Beta 0.1 is stable: component summary cards; clear PASS/WARNING/FAIL/NOT TESTED hierarchy; persistent guided stepper; model-aware chassis visualization; evidence drill-down; report preview/export; improved localization and Vietnamese-first technician copy.

## Beta 0.3 — Provider & Profile Hardening
Expand verified Dell model/profile coverage; trusted sensor-provider packaging; trusted VRAM-engine release pinning; deeper storage/adapter validation; controlled benchmark baseline fleet.

## RC — Portable / WinPE Certification
One portable package policy; WinPE capability matrix; driver/provider fallback matrix; signed-release strategy; deterministic build/version metadata; regression matrix across supported Windows versions.

## 1.0
Release criteria are based on reproducible real-machine evidence, not the number of implemented probes.
