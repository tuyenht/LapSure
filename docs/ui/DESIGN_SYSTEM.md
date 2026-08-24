# LapSure Design System

**Status:** normative implementation contract  
**Target:** native C++20 / Win32 desktop UI  
**Default language:** Vietnamese

## 1. Design intent

LapSure must feel like a professional Windows diagnostic instrument: calm, evidence-oriented, trustworthy and easy for a first-time used-laptop buyer while retaining expert drill-down.

The visual system must never create diagnostic certainty through decoration. Status is communicated by text/icon/structure first, color second.

## 2. Foundations

### Color semantics
Use existing `UiColors` in `include/lap/ui_theme.h` as implementation baseline and centralize further tokens there or in shared native theme modules.

- `surface/sidebar` — dark navy
- `surface/content` — light neutral
- `surface/card` — white
- `brand/primary` — LapSure blue
- `status/good` — green
- `status/warning` — amber
- `status/fail` — red
- `status/info/running` — blue
- `status/neutral` — gray
- `focus` — high-contrast primary outline

Do not use green for UNKNOWN/UNSUPPORTED/NOT TESTED.

### Typography
Prefer Segoe UI / Segoe UI Variable when available, with system fallback.
- Page title: 22–26 px equivalent, Semibold/Bold
- Section heading: 16–18 px, Semibold
- Body: 13–15 px
- Supporting: 12–13 px
- Technical evidence: Consolas or equivalent monospace

Never shrink text to rescue an overcrowded layout.

### Spacing
4 px base scale: 4 / 8 / 12 / 16 / 24 / 32 / 48.

### Geometry
- Cards: subtle border, modest radius; avoid glossy/3D effects.
- Buttons: minimum practical hit target ~32–36 px high at 100% scaling.
- Use consistent left alignment and 8/16/24 px vertical rhythm.
- Tables may be dense but must preserve 13 px+ readable text.

## 3. Page anatomy

Every screen should use:
1. Page header
2. Context / device-session identity
3. Primary status or task
4. Main content
5. Evidence/detail
6. Next-best action where workflow-driven

Right guidance rail is optional and collapses at narrower widths.

## 4. Components

Canonical reusable components are C01–C12 under `components/`.
Do not duplicate status colors, spacing or state wording inside screen renderers.

## 5. Iconography

Use a consistent Windows-compatible resource/vector icon family with text fallback.
Emoji may appear in design docs but must not be the sole production icon mechanism.

## 6. Motion / live updates

- Avoid decorative animation.
- Running progress may update smoothly but must not imply fake precision.
- Live logs update without stealing keyboard focus.
- State transitions should not reorder content unexpectedly unless required.

## 7. Responsive Windows desktop behavior

Primary validation targets:
- 1366×768
- 1920×1080
- DPI: 100%, 125%, 150%

Prefer content reflow/scroll over shrinking typography.
Primary CTA must remain reachable.

## 8. Status rendering rule

Every state-bearing component uses:
- icon or shape,
- canonical Vietnamese label,
- optional concise explanation,
- semantic color.

Never color alone.

## 9. Visual fidelity policy

Approved mockups define hierarchy, density and layout direction.
They do not authorize:
- fabricated measurements,
- generic machine-health scores,
- invented provider success,
- hiding missing evidence.

See `KNOWN_MOCKUP_DEVIATIONS.md`.
