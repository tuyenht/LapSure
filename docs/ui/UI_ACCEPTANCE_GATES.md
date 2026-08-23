# UI Acceptance Gates

## Gate A — Product semantics
No false PASS; required incomplete coverage blocks clean BUY; presence != functionality; factory mismatch != health failure; uncertainty/provider/permission/cancel/interruption explicit; no fabricated measurements/scores.

## Gate B — Data binding
Every technical value traces to model/provider, documented derivation or operator evidence; missing values have designed state; no production demo literals.

## Gate C — Visual/UX
Match approved hierarchy/layout direction; canonical Vietnamese copy; one primary action; progressive technical detail; non-happy states designed.

## Gate D — Accessibility/DPI
Validate 1366×768/1920×1080, 100/125/150% scaling, keyboard focus/tab order, color-independent status, no clipped critical CTA/text.

## Gate E — Engineering
Native C++20/Win32; strict MSVC x64 Release; relevant tests; no UI-thread blocking/cancellation/journal/report regression; no unexpected heavy runtime.

## Gate F — Visual verification
Launch real executable when possible, capture screenshot, compare target visual, record intentional deviations, perform independent evidence review.

P0 dashboard is complete only when S01–S23 are resolved and runnable gates are green.
