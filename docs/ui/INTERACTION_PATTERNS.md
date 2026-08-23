# LapSure Interaction Patterns

## One primary action
Workflow screens expose one visually dominant next action.

## Guided sequence
S02 → S03 as needed → S04 → S05 → S06/S07 → S16 → S18 → S19. Manual result recording remains gated until the required automatic snapshot exists.

## Drill-down
Summary click/expand exposes evidence without changing result.

## Long-running operations
Off UI thread; show real state/progress/elapsed; cancellation bounded; pause only if supported; no fake ETA precision.

## Manual verification
State what to do, success criteria, result choices, skip consequence and operator-evidence attribution.

## Destructive/safety-sensitive actions
Confirm abandoning active session, deleting evidence/history, closing interrupted journal and disruptive test actions.

## Navigation
Preserve session context; back/navigation never silently discards evidence; S23 is contextual.

## Empty/error states
Design no-data, provider unavailable, permission denied, unsupported and not-tested states with next-best action when available.

## Keyboard
Tab follows visual order; Enter/Space activates; Esc safely closes modal/full-screen test; focus visible.
