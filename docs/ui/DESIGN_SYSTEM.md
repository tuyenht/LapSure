# LapSure Design System

**Status:** normative implementation contract  
**Target:** native C++20 / Win32 desktop UI  
**Default language:** Vietnamese

## Design intent
LapSure should feel like a professional Windows diagnostic instrument: calm, evidence-oriented and usable by novices while retaining expert drill-down. Visual decoration must never create diagnostic certainty.

## Color semantics
Use existing `UiColors` in `include/lap/ui_theme.h` as implementation baseline and centralize further tokens there/shared native modules.
- dark navy sidebar
- light neutral content
- white cards
- LapSure blue primary
- green good/pass
- amber warning
- red fail
- blue info/running
- gray neutral/unknown

Never use green for UNKNOWN/UNSUPPORTED/NOT TESTED.

## Typography
Segoe UI / Segoe UI Variable with system fallback. Page 22–26 px; section 16–18; body 13–15; support 12–13; technical data monospace. Never shrink text to rescue an overcrowded layout.

## Spacing and geometry
4 px scale: 4/8/12/16/24/32/48. Subtle card border/radius. Practical button hit target ~32–36 px at 100%. Consistent alignment/rhythm.

## Page anatomy
Page header → context → primary task/status → main content → evidence/detail → next-best action.

## Components
Use C01–C12 under `components/`. Do not duplicate semantic state/spacing/color logic in screen renderers.

## Iconography
Consistent Windows-compatible resource/vector icon family with text fallback. Emoji is not the sole production icon system.

## Responsive targets
1366×768 and 1920×1080 at 100/125/150% DPI. Reflow/scroll before shrinking typography. Primary CTA remains reachable.

## Status rendering
Icon/shape + canonical Vietnamese label + semantic color; never color alone.

## Visual fidelity policy
Approved mockups define hierarchy/density/layout direction only. They do not authorize fabricated measurements, generic health scores, invented provider success or hidden missing evidence. See `KNOWN_MOCKUP_DEVIATIONS.md`.
