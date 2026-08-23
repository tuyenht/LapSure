# LapSure Agent Contract

LapSure is a native Windows/WinPE evidence-oriented laptop verification product.

## Mandatory source-of-truth precedence
1. `docs/COVERAGE_CONTRACT.md`
2. `docs/PRODUCT_SPEC.md`
3. `docs/ARCHITECTURE.md`
4. `docs/USED_LAPTOP_EXPERT_AUDIT.md`
5. `docs/ui/LAPSURE_UI_MASTER_SPEC.md`
6. `docs/ui/UI_STATE_MODEL.md`
7. `docs/ui/DATA_BINDING_CONTRACT.md`
8. Target `docs/ui/screens/Sxx_*.md`
9. Referenced `docs/ui/components/Cxx_*.md`
10. `docs/ui/KNOWN_MOCKUP_DEVIATIONS.md`
11. `docs/ui/references/approved/*`

Mockups never override evidence semantics.

## Non-negotiable product rules
- Evidence before verdict.
- Presence is not functionality.
- Factory mismatch is not hardware-health failure.
- Missing/malformed/timed-out/permission-denied/stale/contradictory/untrusted/unsupported evidence never becomes PASS.
- `UNKNOWN`, `NOT TESTED`, `UNSUPPORTED`, `INCOMPLETE` remain explicit.
- Required coverage incomplete => no clean BUY/PASS.
- Never invent health %, temperature, adapter wattage, exact link rate, factory provenance or benchmark conclusion.
- External engines execute only under existing trust/hash policy.
- Vietnamese-first user UI.
- Keep native C++20/Win32 and Windows/WinPE graceful degradation.

## UI task routing
Read `docs/ui/SCREEN_INDEX.md`, then only the target screen contract, linked component contracts, state/data contracts and matching approved reference. Use `.agents/skills/lapsure-ui/SKILL.md`.

## Engineering behavior
Inspect current code/data sources first, implement only supported claims, build strict MSVC x64 Release, run relevant regression tests, and report unavailable gates honestly.
