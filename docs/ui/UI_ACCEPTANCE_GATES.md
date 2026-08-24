# UI Acceptance Gates

A screen or Professional Dashboard phase is not complete until applicable gates pass.

## Gate A — Product semantics
- No false PASS.
- Required incomplete coverage blocks clean BUY.
- Presence != functionality.
- Factory mismatch != health failure.
- Unsupported/provider/permission/cancel/interruption states remain explicit.
- No fabricated measurements/scores.

## Gate B — Data binding
- Every technical value traces to model/provider, deterministic derivation, or operator evidence.
- Missing values have designed unavailable state.
- No production sample/demo literals.

## Gate C — Visual/UX
- Matches approved hierarchy/layout direction.
- Vietnamese copy canonical.
- One obvious primary action.
- Technical detail is progressive disclosure.
- Non-happy states designed.

## Gate D — Accessibility/DPI
Validate:
- 1366×768 and 1920×1080,
- 100%, 125%, 150% scaling,
- keyboard focus and tab order,
- no color-only status,
- no clipped critical CTA/text.

## Gate E — Engineering
- Native C++20/Win32 preserved.
- Strict MSVC x64 Release build passes.
- Relevant source/behavioral tests pass.
- No UI-thread blocking regression.
- Cancellation/journal/report behavior preserved.
- No unexpected heavy runtime dependency.

## Gate F — Visual verification
- Launch real executable when possible.
- Capture screenshot.
- Compare with target visual.
- Record intentional deviations.
- Evidence reviewer checks semantics independently from visual reviewer.

## P0 dashboard completion
P0 is complete only when S01–S23 are resolved by implementation or explicit staged state, all P0 semantics are correct, and runnable gates are green.
