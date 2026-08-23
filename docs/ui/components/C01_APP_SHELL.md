# C01 — App Shell

## Purpose
Shared native Win32 window/content/sidebar/status architecture.

## Anatomy
Dark navy grouped sidebar; light main content surface; optional contextual status strip; session context preserved across navigation.

## Rules
DPI-aware layout; no diagnostic work inside paint/layout; primary content can scroll; do not lose active session when navigating.

## States/accessibility
Follow `UI_STATE_MODEL.md` and `ACCESSIBILITY_DPI.md`; focusable navigation and color-independent state signals.
