# LapSure UI/UX — Agent-Executable Design Contract

This directory is the normative UI/UX implementation layer for LapSure.

## Authority order

When requirements conflict, use this precedence:

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

- `SCREEN_INDEX.md` — machine/human navigation to all S01–S23 screens.
- `DESIGN_SYSTEM.md` — visual system and native Win32 implementation intent.
- `COMPONENT_CATALOG.md` — reusable component inventory.
- `UI_STATE_MODEL.md` — canonical status/state semantics.
- `DATA_BINDING_CONTRACT.md` — source-to-UI mapping and no-fabrication rules.
- `INTERACTION_PATTERNS.md` — navigation, dialogs, guided tests, live updates.
- `UX_COPY_VI.md` — canonical Vietnamese copy.
- `ACCESSIBILITY_DPI.md` — Windows keyboard/DPI/accessibility contract.
- `TRACEABILITY_MATRIX.md` — requirement → screen → data → code/test mapping.
- `KNOWN_MOCKUP_DEVIATIONS.md` — places where visual concepts must not be copied literally.
- `UI_ACCEPTANCE_GATES.md` — Definition of Done.
- `references/MANIFEST.yaml` — machine-readable screen-to-reference map.

## Agent rule

For a single screen task:
1. Read `AGENTS.md`.
2. Read `SCREEN_INDEX.md`.
3. Read the target screen contract.
4. Read only the linked component contracts.
5. Read `DATA_BINDING_CONTRACT.md` and applicable state rules.
6. Inspect the matching approved visual reference.
7. Inspect current code/model/providers.
8. Implement, build, test, capture, compare, audit.

Do not infer unavailable data from mockups.
