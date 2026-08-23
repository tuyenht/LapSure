---
name: lapsure-ui
description: Implements and audits LapSure native Win32 Professional Dashboard screens, Vietnamese UX, evidence-state semantics, navigation, DPI/layout, and regression gates. Use for any LapSure UI, screen, dashboard, report-view, workflow or UX change.
---

# LapSure UI Skill

## Required context
Read, in order:
1. `/AGENTS.md`
2. `/docs/COVERAGE_CONTRACT.md`
3. `/docs/PRODUCT_SPEC.md`
4. `/docs/ARCHITECTURE.md`
5. `/docs/ui/LAPSURE_UI_MASTER_SPEC.md`
6. `/docs/ui/ANTIGRAVITY_IMPLEMENTATION_CONTRACT.md`
7. relevant `/docs/ui/screens/*.md`.

## Method

### 1. Identify target
State the Screen ID(s) and user outcome.

### 2. Inspect current code
Find:
- current renderer/control creation;
- existing `MainTab` path;
- model fields actually available;
- current async/cancellation path;
- related report/decision behavior;
- reusable theme/components.

### 3. Evidence audit before UI
For every proposed metric/status, classify it:
- directly evidenced;
- deterministically derived;
- operator-confirmed;
- unavailable/unknown.

Delete or replace any unsupported mockup metric.

### 4. State matrix
Implement relevant states:
- idle/not started;
- loading/running;
- success/good/pass;
- warning;
- fail;
- incomplete;
- not tested;
- unsupported;
- manual required;
- cancelled;
- interrupted;
- empty/no data.

### 5. Implement native reusable UI
Prefer shared:
- design tokens;
- layout helpers;
- status badge;
- progress/coverage component;
- metric card;
- guided stepper;
- evidence row;
- empty/error state;
- next-best-action panel.

Keep data collection out of paint/layout functions.

### 6. Vietnamese copy audit
Use short task-oriented Vietnamese. Explain technical states instead of replacing them with vague green/red labels.

### 7. Build/test
Run all applicable strict build and regression gates defined in the implementation contract. Fix regressions before proceeding.

### 8. Visual audit
Check:
- 1366×768 and 1920×1080 intent;
- 100–150% DPI;
- no clipped primary controls;
- visual hierarchy;
- keyboard/focus behavior;
- color-independent states.

### 9. Evidence-semantic audit
Verify:
- no false PASS;
- required incomplete coverage remains blocking;
- presence != functionality;
- factory != health;
- unavailable provider != hardware fail;
- interrupted != pass;
- reports still preserve evidence.

### 10. Completion report
List changed files, screen IDs, tests/build results, unresolved gaps and intentional visual deviations.
