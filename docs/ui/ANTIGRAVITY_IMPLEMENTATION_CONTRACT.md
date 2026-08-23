# Antigravity 2.0 Implementation Contract — LapSure v2

## Mandatory reading order
1. `/AGENTS.md`
2. `/docs/COVERAGE_CONTRACT.md`
3. `/docs/PRODUCT_SPEC.md`
4. `/docs/ARCHITECTURE.md`
5. `/docs/USED_LAPTOP_EXPERT_AUDIT.md`
6. `/docs/ui/README.md`
7. `/docs/ui/LAPSURE_UI_MASTER_SPEC.md`
8. `/docs/ui/SCREEN_INDEX.md`
9. `/docs/ui/UI_STATE_MODEL.md`
10. `/docs/ui/DATA_BINDING_CONTRACT.md`
11. Target screen contract
12. Referenced component contracts
13. `/docs/ui/KNOWN_MOCKUP_DEVIATIONS.md`
14. `/docs/ui/references/MANIFEST.yaml`
15. Matching member from `/docs/ui/references/LapSure_UI_Visual_Reference_Pack.zip` when available
16. Current code/tests

Extract visual members to a temporary working location if detailed inspection is needed. Do not begin from a screenshot alone.

## Pre-change audit
Identify current renderer/control path, real model/provider fields, reusable components, lifecycle/threading, missing/non-happy states, unsupported mockup literals and files/tests at risk.

## Implementation rules
Native C++20/Win32; no Electron/Chromium/WebView/Node/Python runtime for UI convenience; no slow diagnostics in render/layout; no fabricated evidence; preserve trust/hash, WinPE graceful degradation, shared state/data/components and report/journal/cancellation behavior.

## Per-screen sequence
Audit → shared component if missing → implement screen → build → tests → launch/capture → visual review → evidence review → update traceability/deviations if needed.

## Completion
A screen is done only when applicable gates in `UI_ACCEPTANCE_GATES.md` pass. Unavailable Windows/hardware gates are `NOT RUN — environment limitation`.

## Agent separation
Prefer `lapsure-ui-implementer`, `lapsure-ui-reviewer` and `lapsure-evidence-reviewer`; the authoring agent should not be the only reviewer of its own evidence semantics.
