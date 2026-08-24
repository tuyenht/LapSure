# C02 — Sidebar

## Purpose
Reusable native Win32 component. Screen renderers should use the shared implementation rather than duplicate visual/state logic.

## Anatomy
- Groups: Quy trình, Chi tiết thiết bị, Đánh giá & hồ sơ, Utility
- Active item blue + text/icon

## Rules
- Keyboard reachable
- Collapsible device-detail group
- S23 not permanent primary tab

## States
Support applicable states from `UI_STATE_MODEL.md`. A component that carries diagnostic state must preserve explicit uncertainty.

## Accessibility / DPI
Follow `ACCESSIBILITY_DPI.md`; use text with icon/color, keyboard focus where interactive, and DPI-aware dimensions.

## Engineering mapping
Centralize tokens/helpers in shared UI modules. Do not perform slow evidence collection inside rendering.
