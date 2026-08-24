# Accessibility & DPI Contract

## Required desktop targets
- 1366×768
- 1920×1080
- Windows scaling 100%, 125%, 150%

## Layout
- No clipped primary CTA.
- No overlapping cards.
- Reflow or scroll before shrinking type.
- Right guidance rail may collapse.
- Dense tables may horizontally scroll only when unavoidable.

## Keyboard
- All primary controls reachable.
- Visible focus rectangle/style.
- Tab order follows visual order.
- Enter/Space activate buttons.
- Esc closes modal/full-screen test without losing recorded evidence.

## Status accessibility
- Never color alone.
- Use icon/shape + text.
- Avoid low-contrast gray-on-gray status labels.
- Disabled controls must remain understandable, with gating reason available.

## Text
- Body target 13–15 px equivalent.
- Supporting text 12–13 px.
- Technical values may use monospace.
- Do not encode critical meaning only in tooltip/hover.

## Dialogs
- Initial focus on safest sensible control.
- Destructive action is not default.
- Screen-reader-friendly control labels should be used where native accessibility permits.
