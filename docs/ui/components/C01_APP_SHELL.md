# C01 — App Shell

## Purpose
Reusable native Win32 component. Screen renderers should use the shared implementation rather than duplicate visual/state logic.

## Anatomy
- Dark navy grouped sidebar
- Light content surface
- Optional contextual status strip

## Rules
- DPI-aware layout
- Preserve session context
- No diagnostic work in paint

## States
Support applicable states from `UI_STATE_MODEL.md`. A component that carries diagnostic state must preserve explicit uncertainty.

## Accessibility / DPI
Follow `ACCESSIBILITY_DPI.md`; use text with icon/color, keyboard focus where interactive, and DPI-aware dimensions.

## Engineering mapping
Centralize tokens/helpers in shared UI modules. Do not perform slow evidence collection inside rendering.
