# LapSure Roadmap

## Completed architecture milestones
Hardware inventory/core health; EDID; stress/event-delta; trusted engine gate; telemetry/decision model; functional I/O; physical port/power verification; orchestrator; model-aware chassis profiles; MSVC/portable/real-machine validation scaffold.

## Current — Beta 0.1.1: Evidence Correctness & Executable Regression
1. Strict MSVC x64 Release `/W4 /WX`.
2. Prevent missing/malformed/timed-out/untrusted evidence from PASS/BUY.
3. Validate provider parsers/decision/report contracts.
4. Bound cancellation and make stress journal crash-safe.
5. Real-device E2E after evidence gate.
6. Validate physical chassis profiles/port labels.
7. Compare evidence against BIOS/OEM/independent references.
8. Maintain false-positive/false-negative log.
9. Validate portable/WinPE behavior.

## Beta 0.2 — Professional Dashboard

### Scope
- shared native design system/components;
- grouped navigation;
- clear PASS/WARNING/FAIL/NOT TESTED hierarchy;
- persistent guided stepper/next-best action;
- model-aware chassis visualization;
- evidence drill-down;
- report preview/export;
- Vietnamese-first UX;
- screen contracts S01–S23;
- session history/recovery.

### Acceptance gates
Beta 0.2 is complete only when:
- all S01–S23 contracts are resolved by implementation or explicit staged state;
- state/data/component contracts are centralized;
- no generic fabricated health score exists;
- mandatory uncertainty states remain explicit;
- automatic/manual workflow gates are preserved;
- required incomplete coverage blocks clean BUY;
- 1366×768 and 1920×1080 are usable;
- 100/125/150% DPI is validated;
- keyboard navigation/focus is validated;
- approved visual references are audited against actual executable screenshots;
- strict build/regression/report gates are green where runnable.

Normative UI Definition of Done: `docs/ui/UI_ACCEPTANCE_GATES.md`.

## Beta 0.3 — Provider & Profile Hardening
Expand verified profiles, trusted sensor/VRAM providers, deeper storage/adapter validation, controlled benchmark baseline fleet.

## RC — Portable / WinPE Certification
Portable package policy, WinPE capability matrix, driver/provider fallback matrix, signed-release strategy, deterministic metadata, Windows regression matrix.

## 1.0
Release is based on reproducible real-machine evidence, not probe count.
