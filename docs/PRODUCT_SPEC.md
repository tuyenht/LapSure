# LapSure Product Specification

**Product:** LapSure  
**Positioning:** Laptop Verification & Diagnostics  
**Current milestone:** v0.1.1-beta — Evidence Correctness & Executable Regression Gate  
**Primary audience:** used-laptop buyers, technicians, refurbishers, resellers, service centers

## 1. Mission
LapSure replaces a fragmented laptop-inspection workflow with one evidence-oriented process that answers what hardware is present, whether it is healthy/stable under tests actually performed, whether functional devices and physical ports respond to stimulus, and whether evidence coverage is sufficient for a purchase/acceptance decision.

The product prefers `UNKNOWN`, `NOT TESTED` or `INCOMPLETE` over an unsupported claim.

## 2. Product principles
- Evidence before verdict.
- Presence is not functionality.
- Historical evidence is not current failure proof.
- Factory mismatch is not health failure.
- No invented health percentage.
- Optional engines must be SHA-256 trusted.

## 3. User modes
Quick / Standard / Deep.

## 4. Diagnostic domains
System identity; CPU; Memory; Storage; Battery & power; GPU/VRAM; Display; Functional I/O; Physical ports. Detailed provider/evidence semantics remain governed by `COVERAGE_CONTRACT.md` and implementation modules.

## 5. Factory & chassis profiles
Factory profiles and chassis profiles are distinct. Chassis/profile confidence and validation status must be disclosed; draft/generic data is not production certification.

## 6. Decision model
High-level states: `BUY`, `BUY WITH NOTES`, `INCOMPLETE`, `REJECT`. Stability, thermal, performance, factory/profile, functional, port/power, coverage and confidence remain separate dimensions.

## 7. Guided Test Orchestrator
1. Automatic Hardware Audit
2. Functional Verification
3. Physical Ports & Power
4. Final Review

Each tracks state, completed/total, operator-required and next-best action.

## 8. Reports
Human-readable HTML + structured JSON preserve measurements, expected values, source/evidence, status/severity, confidence/coverage and explicit uncertainty.

## 9. Windows / WinPE
Native C++/Win32. Missing capability degrades explicitly to unsupported/not-tested, not false failure.

## 10. Security & trust
No silent runtime downloads; SHA-256 allowlist for external engines; bounded output/cancellation/timeouts; crash-safe stress journal.

## 11. Current acceptance gate
Strict MSVC x64 Release and behavioral tests; no evidence gap can create BUY/PASS; report contracts validate; portable launch; real-laptop end-to-end; physically verified chassis profiles; false-positive/negative corrections.

## 12. Presentation & UX Contract
LapSure exposes this evidence model through the **Professional Dashboard**. The normative UI/UX implementation layer is `docs/ui/`.

It must preserve:
- evidence-before-verdict semantics;
- explicit UNKNOWN/NOT TESTED/UNSUPPORTED/INCOMPLETE;
- automatic-before-interactive orchestration;
- Vietnamese-first guided workflow;
- separation of identity, factory, health, functionality, stability, coverage and confidence;
- traceability from every displayed technical value to a real data source or explicit unavailable state.

UI visual references may shape hierarchy/layout but may not invent product capabilities or diagnostic truth.

## 13. Current non-goals
No mathematically perfect diagnosis; no fabricated adapter wattage; no fake CPU package temperature; no generic health score; no certification from PnP presence alone; no claim to replace every vendor-specific diagnostic.
