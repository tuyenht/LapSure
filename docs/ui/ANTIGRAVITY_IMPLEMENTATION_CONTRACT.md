# Antigravity 2.0 Implementation Contract — LapSure

This document defines how an autonomous coding agent should implement the LapSure Professional Dashboard safely.

## 1. Required reading before code changes

Read in this exact order:
1. `/AGENTS.md`
2. `/docs/COVERAGE_CONTRACT.md`
3. `/docs/PRODUCT_SPEC.md`
4. `/docs/ARCHITECTURE.md`
5. `/docs/USED_LAPTOP_EXPERT_AUDIT.md`
6. `/docs/ui/LAPSURE_UI_MASTER_SPEC.md`
7. Relevant `/docs/ui/screens/*.md`
8. Current source/tests.

Do not begin by rewriting UI from mockups.

## 2. Pre-change audit

Before coding, produce a concise implementation plan containing:
- existing UI architecture and reusable helpers;
- current `MainTab`/navigation behavior;
- actual model fields/providers available for the target screen;
- discrepancies between current implementation and master spec;
- any mockup value that is not supported by real evidence;
- files expected to change;
- regression risks.

Then implement without waiting for another confirmation unless a requirement is truly impossible or unsafe.

## 3. Architectural constraints

- Native C++20/Win32 remains the product shell/core.
- Do not add Electron, Chromium, WebView, Node, Python runtime or network-delivered UI dependency.
- Custom x64 WinPE compatibility must not be intentionally broken.
- No silent runtime downloads.
- Existing SHA-256 trust boundary for external engines remains mandatory.
- UI must not directly invent or mutate diagnostic evidence.
- Long-running collection stays off the UI thread.

## 4. UI/data boundary

Every displayed diagnostic value must be one of:
1. directly available from `AuditReport`/related model;
2. derived deterministically from existing validated fields;
3. marked unavailable/unknown/not tested;
4. operator-confirmed and recorded as such.

If a mockup asks for a value not represented by the model, do **not** hard-code sample data. Either:
- add a properly sourced model/provider change with tests; or
- render an explicit unavailable state.

## 5. Implementation order

### Phase A — foundation
- centralize design tokens/components;
- add grouped sidebar/navigation;
- preserve existing screen reachability;
- add reusable status/coverage/empty-state components;
- establish DPI/layout helpers.

### Phase B — workflow shell
- S01, S02, S03, S04, S05, S06, S07, S08;
- preserve automatic-before-interactive gating;
- ensure next-best-action behavior.

### Phase C — device detail
- S09–S15, including all P0 missing-screen contracts.

### Phase D — decision/evidence
- S16–S23;
- final report explanation;
- history/recovery;
- export/evidence/log workflows.

Do not combine all phases into an unreviewable single source file. Refactor rendering into maintainable modules when needed.

## 6. Acceptance gates per screen

A screen is complete only when all applicable checks pass.

### Visual/UX
- Vietnamese copy is correct and concise.
- Primary action is obvious.
- No clipping at supported window/DPI targets.
- Loading/empty/running/warning/fail/incomplete/unsupported states are handled.
- Keyboard focus/tab behavior is usable.
- Color is not the only status signal.

### Evidence semantics
- no false PASS;
- no fabricated percentage/value;
- presence not treated as functionality;
- provider failure exposed;
- manual evidence labeled manual;
- required incomplete coverage blocks clean verdict;
- factory mismatch separated from health.

### Engineering
- x64 Release build succeeds with strict warnings;
- relevant source sanity tests pass;
- behavioral tests pass;
- no new unbounded wait/cancellation path;
- no unexpected runtime dependency;
- HTML/JSON report behavior not silently degraded.

## 7. Required commands/gates

Use the repository-supported build/test flow. At minimum:
- build the MSVC x64 Release target/preset used by CI;
- run `run_source_tests.cmd`;
- run the behavioral test executable/CTest path used by the current repository/CI;
- inspect Git diff before completion.

If the environment cannot run a Windows-only gate, state exactly which gate could not be run; never claim it passed.

## 8. Visual verification

After implementing a major screen:
- launch the real executable if possible;
- capture a screenshot/artifact;
- compare hierarchy, spacing, copy and states against `LAPSURE_UI_MASTER_SPEC.md`;
- list intentional deviations.

Mockups are directional. Evidence semantics win over mockup fidelity.

## 9. Completion report format

Return:
1. Summary of implemented phases/screens.
2. Files changed.
3. Build results.
4. Test results.
5. Evidence/coverage audit findings.
6. Visual/UX deviations.
7. Remaining P0/P1 gaps.
8. Commit/branch/PR references if created.

Never report “done” while a known P0 evidence-semantic violation remains.
