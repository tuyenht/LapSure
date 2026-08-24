# S04 — Kiểm tra Tự động

```yaml
screen_id: S04
status: approved-contract
visual_reference: S04_AUTO_AUDIT.jpg
components: [C01, C02, C03, C04, C06, C07, C08, C10, C12]
```

## 1. User outcome
Run and monitor automatic evidence collection without implying functionality from enumeration.

## 2. Primary action
**Bắt đầu/Tạm dừng/Tiếp tục**

## 3. Entry / exit
- Entry: preserve active inspection/session context and applicable orchestrator gates.
- Exit: navigation must not silently discard evidence or mutate diagnostic truth.
- If mandatory prerequisite evidence is absent, render the correct LOCKED/INCOMPLETE state and explain the next action.

## 4. Page anatomy / object inventory

| Object ID | Object | Binding rule |
|---|---|---|
| S04-O01 | Mode selector | Bind to real data/state; unavailable stays explicit |
| S04-O02 | Overall progress | Bind to real data/state; unavailable stays explicit |
| S04-O03 | Domain rows | Bind to real data/state; unavailable stays explicit |
| S04-O04 | Current step | Bind to real data/state; unavailable stays explicit |
| S04-O05 | Elapsed time | Bind to real data/state; unavailable stays explicit |
| S04-O06 | Evidence source/status | Bind to real data/state; unavailable stays explicit |
| S04-O07 | Live log | Bind to real data/state; unavailable stays explicit |
| S04-O08 | Next action | Bind to real data/state; unavailable stays explicit |

## 5. Data sources
- `OrchestratorSummary`
- `Findings`
- `HardwareSnapshot`
- `Live diagnostic log`

All values must comply with `../DATA_BINDING_CONTRACT.md`.

## 6. States
Implement applicable states from `../UI_STATE_MODEL.md`, including non-happy states: loading/running, warning, fail, incomplete, not-tested, unsupported/provider-unavailable, permission-denied, cancelled/interrupted and empty.

## 7. Interaction
- Keep one obvious primary action.
- Drill-down exposes evidence; it does not change the result.
- Manual decisions must be stored as operator-confirmed evidence.
- Long-running work must remain off the UI thread.

## 8. Evidence invariant
**Enumeration is not functionality. Provider/permission/unsupported states remain explicit.**

## 9. Visual contract
Primary reference: `../references/approved/S04_AUTO_AUDIT.jpg`

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
