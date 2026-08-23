# LapSure UI/UX Specification Pack

This folder is the implementation-facing UI/UX source of truth for the **Professional Dashboard** evolution of LapSure.

## Documents

- `LAPSURE_UI_MASTER_SPEC.md` — product UX principles, navigation, screen catalog, design system, states and workflow.
- `ANTIGRAVITY_IMPLEMENTATION_CONTRACT.md` — mandatory implementation and acceptance gates for Google Antigravity 2.0 or other coding agents.
- `ANTIGRAVITY_PROMPT.md` — ready-to-use execution prompt.
- `screens/` — detailed contracts for P0 screens that are currently missing or insufficiently implemented.

## Relationship to existing product documents

This pack **does not replace**:
- `../COVERAGE_CONTRACT.md`
- `../PRODUCT_SPEC.md`
- `../ARCHITECTURE.md`
- `../USED_LAPTOP_EXPERT_AUDIT.md`

Those documents remain authoritative for diagnostic truth. UI mockups and layout requirements may never weaken evidence/coverage policy.

## Current code baseline

The repository already has a native Win32 UI with `MainTab` entries in `include/lap/ui_theme.h` and rendering in `src/main.cpp` / `src/ui_renderer.cpp`. The goal is an incremental professional refactor, not a rewrite of the diagnostic engine.

## Visual references

The approved mockups produced during product design should be used as **visual references**, not as data contracts. If a mockup displays a value that the current data model cannot prove, the implementation must show an explicit unavailable/unknown state rather than fabricate the value.
