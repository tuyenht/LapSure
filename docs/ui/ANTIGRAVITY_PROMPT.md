# Ready-to-use prompt for Google Antigravity 2.0

Use the following prompt after opening the `tuyenht/LapSure` repository as the active Antigravity Project.

```text
You are the lead implementation agent for LapSure Professional Dashboard.

Goal:
Implement the approved Vietnamese-first LapSure UI/UX and the remaining P0 screens in the existing native C++20/Win32 application, while preserving diagnostic correctness, portability, strict build quality and the no-false-PASS evidence policy.

MANDATORY CONTEXT:
First read and obey:
@AGENTS.md
@docs/COVERAGE_CONTRACT.md
@docs/PRODUCT_SPEC.md
@docs/ARCHITECTURE.md
@docs/USED_LAPTOP_EXPERT_AUDIT.md
@docs/ui/LAPSURE_UI_MASTER_SPEC.md
@docs/ui/ANTIGRAVITY_IMPLEMENTATION_CONTRACT.md
@docs/ui/screens/S10_STORAGE.md
@docs/ui/screens/S11_MEMORY.md
@docs/ui/screens/S13_AUDIO_CAMERA.md
@docs/ui/screens/S15_SYSTEM_INFO.md
@docs/ui/screens/S22_SESSION_HISTORY.md
@docs/ui/screens/S23_INTERRUPTED_SESSION_RECOVERY.md

Also use the workspace skill:
.agents/skills/lapsure-ui/SKILL.md

NON-NEGOTIABLE:
- Evidence before verdict.
- Presence is not functionality.
- Never convert missing, malformed, timed-out, stale, contradictory, permission-denied, unsupported or untrusted evidence into PASS.
- Preserve UNKNOWN / NOT TESTED / UNSUPPORTED / INCOMPLETE explicitly.
- Required coverage incomplete => no clean BUY/PASS.
- Never invent health %, temperature, adapter wattage, exact link speed, factory provenance or benchmark values.
- Default user UI language is Vietnamese.
- Keep native C++20/Win32. Do not introduce Electron, WebView, Chromium or a heavy runtime.
- Preserve Windows/WinPE graceful degradation and trusted-engine SHA-256 policy.
- Do not modify diagnostic semantics merely to make a mockup look complete.

EXECUTION:
1. Audit the current repository and compare the current UI to the master spec.
2. Produce an implementation plan artifact with current gaps, reusable code, model/provider availability and regression risks.
3. Create an isolated feature branch/worktree named `feature/professional-dashboard` if available.
4. Implement in phases defined by `ANTIGRAVITY_IMPLEMENTATION_CONTRACT.md`, starting with foundation/navigation/state components, then workflow screens, device-detail screens, and decision/evidence screens.
5. Reuse and refactor existing Win32 rendering rather than performing an unnecessary application rewrite.
6. Implement the six P0 missing-screen contracts. Any unavailable real evidence must render as an explicit unavailable/not-tested/unsupported state; never use demo values in production paths.
7. Preserve automatic-audit-before-interactive gating and journal-backed interrupted-session evidence.
8. After each phase, build and run the relevant tests. Fix regressions before moving forward.
9. Run the strict MSVC x64 Release build, `run_source_tests.cmd`, behavioral/regression tests, and any existing CI-equivalent local gates that the environment supports.
10. Launch the executable and capture screenshots/artifacts for the major screens when possible. Audit visual hierarchy, Vietnamese copy, DPI/clipping, keyboard navigation and status semantics.
11. Inspect the final diff for architecture drift, duplicated UI logic, false PASS paths, hidden incomplete states and report-contract regressions.
12. Commit coherent changes to the feature branch. Do not merge to main until all runnable P0 gates pass.

Do not stop after giving me a plan. Continue implementing autonomously through the full feasible scope.

FINAL RESPONSE MUST INCLUDE:
- phases/screens completed;
- files changed;
- build/test results with exact failures if any;
- screenshots/artifacts created;
- evidence/coverage correctness audit;
- remaining P0/P1 gaps;
- branch and commit hashes;
- any item that could not be verified on the current machine.
```
