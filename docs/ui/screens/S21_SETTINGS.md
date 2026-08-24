# S21 — Cài đặt

```yaml
screen_id: S21
status: approved-contract
visual_reference: S21_SETTINGS.jpg
components: [C01, C02, C03, C09, C10, C11]
```

## 1. User outcome
Expose safe user-facing settings without casually exposing dangerous engineering controls.

## 2. Primary action
**Lưu thay đổi**

## 3. Entry / exit
- Entry: preserve active inspection/session context and applicable orchestrator gates.
- Exit: navigation must not silently discard evidence or mutate diagnostic truth.
- If mandatory prerequisite evidence is absent, render the correct LOCKED/INCOMPLETE state and explain the next action.

## 4. Page anatomy / object inventory

| Object ID | Object | Binding rule |
|---|---|---|
| S21-O01 | Default mode | Bind to real data/state; unavailable stays explicit |
| S21-O02 | UI preferences | Bind to real data/state; unavailable stays explicit |
| S21-O03 | Report path | Bind to real data/state; unavailable stays explicit |
| S21-O04 | Evidence/provider readiness | Bind to real data/state; unavailable stays explicit |
| S21-O05 | Privacy | Bind to real data/state; unavailable stays explicit |
| S21-O06 | Advanced section gated | Bind to real data/state; unavailable stays explicit |

## 5. Data sources
- `Application settings`
- `Capabilities/provider state`

All values must comply with `../DATA_BINDING_CONTRACT.md`.

## 6. States
Implement applicable states from `../UI_STATE_MODEL.md`, including non-happy states: loading/running, warning, fail, incomplete, not-tested, unsupported/provider-unavailable, permission-denied, cancelled/interrupted and empty.

## 7. Interaction
- Keep one obvious primary action.
- Drill-down exposes evidence; it does not change the result.
- Manual decisions must be stored as operator-confirmed evidence.
- Long-running work must remain off the UI thread.

## 8. Evidence invariant
**Preserve explicit UNKNOWN/NOT TESTED/UNSUPPORTED/INCOMPLETE and do not fabricate values.**

## 9. Visual contract
Primary reference: `../references/approved/S21_SETTINGS.jpg`

Follow `../DESIGN_SYSTEM.md` for typography, spacing, color, components and DPI. The image controls hierarchy/layout direction only; `KNOWN_MOCKUP_DEVIATIONS.md` and evidence contracts override illustrative literals.

## 10. Accessibility
Follow `../ACCESSIBILITY_DPI.md`. Validate keyboard use and 1366×768 / 1920×1080 at 100/125/150% scaling.

## 11. Acceptance criteria
- All displayed technical values are sourced/derived/operator-confirmed or explicitly unavailable.
- Canonical Vietnamese state wording is used.
- Primary CTA is reachable and unambiguous.
- Applicable non-happy states are implemented.
- No false PASS path is introduced.
- Related report/evidence semantics remain unchanged unless explicitly specified and tested.
- Actual executable screenshot is compared with the approved visual direction before completion.
