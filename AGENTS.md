# LapSure Agent Contract

This repository contains **LapSure — Kiểm định & Chẩn đoán Laptop**, a native Windows/WinPE evidence-oriented laptop verification product.

## Mandatory source-of-truth order

When requirements conflict, use this precedence:

1. `docs/COVERAGE_CONTRACT.md` — evidence/coverage truth and anti-false-PASS policy.
2. `docs/PRODUCT_SPEC.md` — product semantics and supported diagnostic behavior.
3. `docs/ARCHITECTURE.md` — architecture, trust boundary, orchestration and WinPE constraints.
4. `docs/USED_LAPTOP_EXPERT_AUDIT.md` — used-laptop risk/decision requirements.
5. `docs/ui/LAPSURE_UI_MASTER_SPEC.md` — approved UX/UI information architecture.
6. `docs/ui/ANTIGRAVITY_IMPLEMENTATION_CONTRACT.md` — implementation gates for agent work.
7. `docs/ui/screens/*.md` — screen-specific contracts.
8. Visual mockups — visual direction only; they never override evidence semantics.

## Non-negotiable product rules

- Evidence before verdict.
- Presence is not functionality.
- Factory mismatch is not hardware-health failure.
- Missing, malformed, timed-out, permission-denied, stale, contradictory or untrusted evidence must never become PASS.
- `UNKNOWN`, `NOT TESTED`, `UNSUPPORTED` and `INCOMPLETE` must remain explicit.
- A clean BUY/PASS verdict is prohibited while a required coverage domain is incomplete.
- Do not invent health percentages, temperatures, adapter wattage, link rate, factory provenance or benchmark conclusions.
- External engines execute only when the existing trust/hash policy permits them.
- Default user-facing language is Vietnamese.
- Keep the diagnostic core native C++/Win32 and portable. Do not add Electron, Chromium, WebView or another heavy runtime merely for UI convenience.
- Preserve Windows/WinPE graceful degradation: unavailable capability is not automatically hardware failure.

## Engineering behavior

Before modifying product/UI/scoring/report logic:
1. Read the relevant source-of-truth documents above.
2. Inspect the current implementation and tests.
3. Identify real data fields/providers available in the current model.
4. Implement only claims that can be supported by those fields/providers.
5. Build MSVC x64 Release with the repository's strict warnings policy.
6. Run the relevant behavioral and sanity/regression tests.
7. Report deviations and unresolved evidence gaps explicitly.

For UI work, also follow `.agents/skills/lapsure-ui/SKILL.md`.
