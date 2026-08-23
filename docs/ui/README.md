# LapSure UI/UX — Agent-Executable Design Contract

This directory is the normative UI/UX implementation layer for LapSure.

## Authority order
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

Visual references never override evidence semantics.

## Start here
- `SCREEN_INDEX.md` — all S01–S23 screens.
- `DESIGN_SYSTEM.md` — native visual system.
- `COMPONENT_CATALOG.md` — reusable components.
- `UI_STATE_MODEL.md` — canonical state semantics.
- `DATA_BINDING_CONTRACT.md` — source-to-UI mapping.
- `INTERACTION_PATTERNS.md` — navigation/guided behavior.
- `UX_COPY_VI.md` — canonical Vietnamese copy.
- `ACCESSIBILITY_DPI.md` — Windows keyboard/DPI contract.
- `TRACEABILITY_MATRIX.md` — requirement → screen → data → test.
- `KNOWN_MOCKUP_DEVIATIONS.md` — intentional visual corrections.
- `UI_ACCEPTANCE_GATES.md` — Definition of Done.
- `references/MANIFEST.yaml` — screen/spec/image map.

## Agent rule
For a single screen task: read `AGENTS.md`, `SCREEN_INDEX.md`, the target screen contract, linked components, state/data contracts, matching approved visual, then inspect current code/model/providers. Implement, build, test, capture, compare and audit. Do not infer unavailable data from mockups.
