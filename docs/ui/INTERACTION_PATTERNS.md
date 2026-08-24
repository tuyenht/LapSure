# LapSure Interaction Patterns

## 1. One primary action
Workflow screens expose one visually dominant next action. Secondary actions must not compete.

## 2. Guided sequencing
Canonical flow:
S02 → optional/required S03 → S04 → S05 → S06/S07 → S16 → S18 → S19.

Manual functional results remain gated until the automatic snapshot required by the orchestrator exists.

## 3. Drill-down
Summary card/row click opens or expands evidence detail without changing the underlying result.
Technical detail must be progressive disclosure.

## 4. Long-running operations
- Run off UI thread.
- Keep UI responsive.
- Show RUNNING + stage + elapsed/progress when real.
- Cancellation is explicit and bounded.
- Pause exists only if operation truly supports it.
- Do not fake remaining-time precision.

## 5. Manual verification
Every human-required test must state:
- what to do,
- what counts as success,
- available result choices,
- effect of skipping,
- evidence attribution as operator-confirmed.

## 6. Destructive / safety-sensitive actions
Require confirmation for:
- abandoning an active session,
- deleting evidence/history,
- closing an interrupted journal without recovery,
- potentially disruptive test actions.

## 7. Navigation
- Sidebar selection changes top-level screen.
- Back navigation should not discard evidence silently.
- Contextual S23 recovery appears on startup/history, not as permanent primary navigation.
- Preserve current session context when moving between detail screens.

## 8. Empty/error states
Every list/table/detail screen needs a designed state for:
- no data,
- provider unavailable,
- permission denied,
- unsupported environment,
- not tested.

Provide a next-best action when one exists.

## 9. Keyboard behavior
- Tab follows visual order.
- Enter/Space activates buttons.
- Esc closes modal/full-screen visual test safely.
- Focus is visible.
