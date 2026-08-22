# LapSure Product Specification

**Product:** LapSure  
**Positioning:** Laptop Verification & Diagnostics  
**Current milestone:** v0.1-beta — Windows/MSVC Build & Real-Machine Validation Gate  
**Primary audience:** used-laptop buyers, technicians, refurbishers, resellers, service centers

## 1. Mission
LapSure replaces a fragmented laptop-inspection workflow with one evidence-oriented process that answers: what hardware is present, whether it is healthy/stable under tests actually performed, whether functional devices and physical ports respond to stimulus, and whether evidence coverage is sufficient for a purchase/acceptance decision.

The product must prefer `UNKNOWN`, `NOT TESTED` or `INCOMPLETE` over an unsupported claim.

## 2. Product principles
- **Evidence before verdict:** conclusions retain source/evidence and confidence where practical.
- **Presence is not functionality:** enumeration alone never certifies a component.
- **Historical evidence is not current failure proof:** controlled pre/post deltas are stronger than old logs.
- **Factory mismatch is not health failure:** factory state and component health are separate dimensions.
- **No invented health percentage:** wear percentages are shown only where hardware exposes meaningful capacity/endurance data.
- **Optional engines must be trusted:** external providers are SHA-256 gated; missing/mismatched engines never create PASS.

## 3. User modes
- **Quick:** short initial screening.
- **Standard:** default pre-purchase/pre-sale inspection.
- **Deep:** longer stability/integrity workflow for technicians/workstations.

## 4. Diagnostic domains
### System identity
Manufacturer/model, Service Tag/serial where exposed, BIOS, mainboard, OS/environment and architecture.

### CPU
Identity/topology, sustained load, controlled event-error delta, utilization telemetry, optional trusted package temperature/power/throttling telemetry and versioned local microbenchmark baselines.

### Memory
Installed/module inventory, speed/type where available, online allocated-memory pattern testing with bytes/passes/mismatches. A clean online result remains partial coverage; preboot full-memory certification is separate.

### Storage
Identity/capacity/interface, SMART/NVMe evidence through supported trusted providers, temperature/usage/endurance indicators where available, with factory-vs-current identity kept separate.

### Battery & power
Design/full-charge capacity, wear from actual capacities, charge/AC state. Adapter wattage remains unknown unless a trusted OEM/USB-PD provider proves it.

### GPU / VRAM
GPU identity/memory inventory, NVIDIA telemetry when available, trusted VRAM-integrity engine adapter. No VRAM PASS without an actual trusted integrity test.

### Display
EDID acquisition/validation, native detailed timing vs current Windows mode, touch presence, and operator visual color/dead-pixel workflow.

### Functional I/O
Camera PASS requires an actual Media Foundation sample; microphone captures PCM signal evidence; stereo L/R requires operator confirmation; Wi-Fi uses native WLAN association/signal evidence; Bluetooth checks radio/stack accessibility.

### Physical ports
Baseline before stimulus, known-good device plug/unplug, PnP/location/bus evidence, USB4/Thunderbolt topology deltas where available. Exact link rate is not guessed from device names.

## 5. Factory & chassis profiles
Two separate profile types are used: **factory profiles** for expected unit configuration and **chassis profiles** for model-aware required/optional physical ports. Initial chassis data exists for Dell Precision 5560, 5570 and 7670, but is not production-certified until validated against official references and physical hardware.

## 6. Decision model
High-level states include `BUY`, `BUY WITH NOTES`, `INCOMPLETE`, and `REJECT`. Stability, thermal, performance, factory/profile state, functional state, port/power state, coverage and confidence remain separate dimensions. Missing critical/deep evidence prevents an unjustified clean verdict.

## 7. Guided Test Orchestrator
1. Automatic Hardware Audit
2. Functional Verification
3. Physical Ports & Power
4. Final Review

Each stage tracks state, completed/total units, operator-required flag and next-best action.

## 8. Reports
Required outputs are human-readable HTML and structured JSON. Reports preserve actual measurements, expected values when known, evidence/source, status/severity, confidence/coverage and explicit NOT TESTED/unsupported states.

## 9. Windows / WinPE
The diagnostic core remains native C++/Win32 to minimize dependencies and maximize compatibility with custom x64 WinPE. Capability gaps must degrade explicitly to unsupported/not-tested rather than false hardware failure.

## 10. Security & trust
No silent runtime downloads; external engines are SHA-256 allowlisted; output capture is bounded; cancellation/timeouts are controlled; interrupted stress sessions use a journal so reboot/crash does not erase evidence.

## 11. Current acceptance gate
The project is not production-ready until MSVC x64 Release builds, strict Windows CI is green, the EXE launches on Windows, portable launch works, at least one real laptop completes the full workflow, initial chassis profiles are physically verified, and false-positive/false-negative findings are corrected.

## 12. Current non-goals
No claim of mathematically perfect diagnosis; no fabricated adapter wattage; no fake CPU package temperature from unrelated thermal zones; no generic health score without a meaningful metric; no certification from PnP presence alone; no claim to replace every vendor-specific manufacturer diagnostic.