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
@docs/ui/references/README.md
@docs/ui/references/CONTACT_SHEET.jpg
@docs/ui/screens/S10_STORAGE.md
@docs/ui/screens/S11_MEMORY.md
@docs/ui/screens/S13_AUDIO_CAMERA.md
@docs/ui/screens/S15_SYSTEM_INFO.md
@docs/ui/screens/S22_SESSION_HISTORY.md
@docs/ui/screens/S23_INTERRUPTED_SESSION_RECOVERY.md

Also use the workspace skill:
.agents/skills/lapsure-ui/SKILL.md

VISUAL REFERENCE POLICY:
- `docs/ui/references/CONTACT_SHEET.jpg` and the visual pack define design direction only.
- They are not evidence sources and never authorize hard-coded sample values.
- Screen contracts and product/evidence semantics override pixel fidelity.
- Never use `docs/ui/references/archive/` unless explicitly requested.
- If a mockup shows a value the current model/provider cannot prove, render the correct UNKNOWN / NOT TESTED / UNSUPPORTED / INCOMPLETE / unavailable / operator-confirmed state instead of faking it.

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
1. Audit the current repository and compare the current UI to the master spec and approved visual references.
2. Produce an implementation plan artifact with current gaps, reusable code, model/provider availability, visual deviations and regression risks.
3. Create an isolated feature branch/worktree named `feature/professional-dashboard` if available.
4. Implement in phases defined by `ANTIGRAVITY_IMPLEMENTATION_CONTRACT.md`, starting with foundation/navigation/state components, then workflow screens, device-detail screens, and decision/evidence screens.
5. Reuse and refactor existing Win32 rendering rather than performing an unnecessary application rewrite.
6. Implement the six P0 missing-screen contracts. Any unavailable real evidence must render as an explicit unavailable/not-tested/unsupported state.
7. Keep all user-facing normal copy Vietnamese-first, concise and understandable to a first-time laptop buyer.
8. For each major screen, validate loading/running/pass/warning/fail/incomplete/not-tested/unsupported/manual-required/cancelled/interrupted/empty states where applicable.
9. Build the strict MSVC x64 Release configuration used by CI and run `run_source_tests.cmd` plus the existing behavioral/regression test path.
10. Launch the real executable when possible and capture visual artifacts at representative 1366x768 and 1920x1080 layouts; check 100%, 125% and 150% DPI intent.
11. Compare resulting screens against the approved reference language. List intentional deviations, especially deviations caused by evidence correctness.
12. Inspect the final diff for false PASS paths, hidden incomplete states, invented metrics, UI-thread blocking, cancellation/journal regressions, report regressions, WinPE regressions, duplicated design constants and hard-coded demo data.
13. Fix all runnable P0 defects before reporting completion.

DO NOT STOP AFTER THE PLAN.
Continue autonomously through implementation, build/test, regression fixing, visual audit and evidence-semantic audit for all feasible phases.

FINAL RESPONSE MUST INCLUDE:
- phases/screens completed;
- files created/modified;
- architecture/refactoring summary;
- exact build and test results;
- visual artifacts/screenshots created;
- evidence correctness audit;
- DPI/accessibility audit;
- gates not run and why;
- remaining P0/P1 gaps;
- branch/worktree name;
- commit hashes and PR link if created.

Never say “done” while a known P0 evidence-semantic violation remains.
```
