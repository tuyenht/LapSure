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
14. Matching approved visual
15. Current code/tests

Do not begin from the screenshot alone.

## Pre-change audit
For the target screen identify:
- existing renderer/control path,
- real model/provider fields,
- reusable components,
- lifecycle/threading,
- missing/non-happy states,
- unsupported mockup literals,
- files/tests at risk.

## Implementation rules
- Native C++20/Win32 remains.
- No Electron/Chromium/WebView/Node/Python runtime for UI convenience.
- Do not perform slow diagnostics in render/layout.
- Do not fabricate or mutate evidence.
- Keep external-engine trust/hash boundary.
- Preserve WinPE graceful degradation.
- Use shared state/data/component contracts.
- Refactor monolithic UI incrementally instead of adding more duplication.

## Per-screen sequence
Audit → implement shared component if missing → implement screen → build → tests → launch/capture → visual review → evidence review → update traceability/deviations if needed.

## Completion
A screen is done only when applicable gates in `UI_ACCEPTANCE_GATES.md` pass.
If a Windows/hardware-only gate cannot run, state `NOT RUN — environment limitation`.

## Agent separation
Prefer:
- `lapsure-ui-implementer` to write code,
- `lapsure-ui-reviewer` to audit visual/UX,
- `lapsure-evidence-reviewer` to audit evidence semantics.

The authoring agent should not be the only reviewer of its own evidence semantics.
