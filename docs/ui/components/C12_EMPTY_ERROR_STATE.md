# C12 — Empty/Error/Unsupported State

## Purpose
Reusable native Win32 component. Screen renderers should use the shared implementation rather than duplicate visual/state logic.

## Anatomy
- Icon
- Canonical state
- Explanation
- Recovery/next action

## Rules
- Differentiate no data vs provider error vs unsupported
- Never convert to PASS
- Keep concise

## States
Support applicable states from `UI_STATE_MODEL.md`. A component that carries diagnostic state must preserve explicit uncertainty.

## Accessibility / DPI
Follow `ACCESSIBILITY_DPI.md`; use text with icon/color, keyboard focus where interactive, and DPI-aware dimensions.

## Engineering mapping
Centralize tokens/helpers in shared UI modules. Do not perform slow evidence collection inside rendering.
