# S06 — Ngoại hình & An toàn

```yaml
screen_id: S06
status: approved-contract
visual_reference: S06_PHYSICAL_SAFETY.jpg
components: [C01, C02, C03, C04, C08, C10, C11]
```

## 1. User outcome
Record chassis and safety findings that cannot be inferred electronically.

## 2. Primary action
**Xác nhận bước**

## 3. Entry / exit
- Entry: preserve active inspection/session context and applicable orchestrator gates.
- Exit: navigation must not silently discard evidence or mutate diagnostic truth.
- If mandatory prerequisite evidence is absent, render the correct LOCKED/INCOMPLETE state and explain the next action.

## 4. Page anatomy / object inventory

| Object ID | Object | Binding rule |
|---|---|---|
| S06-O01 | Cracks/dents/warp | Bind to real data/state; unavailable stays explicit |
| S06-O02 | Hinge | Bind to real data/state; unavailable stays explicit |
| S06-O03 | Tamper/screws | Bind to real data/state; unavailable stays explicit |
| S06-O04 | Liquid/corrosion | Bind to real data/state; unavailable stays explicit |
| S06-O05 | Battery swelling | Bind to real data/state; unavailable stays explicit |
| S06-O06 | Charger/cable safety | Bind to real data/state; unavailable stays explicit |
| S06-O07 | Burning smell/electrical instability | Bind to real data/state; unavailable stays explicit |
| S06-O08 | Photo evidence | Bind to real data/state; unavailable stays explicit |

## 5. Data sources
- `Operator-confirmed physical evidence`
- `Safety findings`

All values must comply with `../DATA_BINDING_CONTRACT.md`.

## 6. States
Implement applicable states from `../UI_STATE_MODEL.md`, including non-happy states: loading/running, warning, fail, incomplete, not-tested, unsupported/provider-unavailable, permission-denied, cancelled/interrupted and empty.

## 7. Interaction
- Keep one obvious primary action.
- Drill-down exposes evidence; it does not change the result.
- Manual decisions must be stored as operator-confirmed evidence.
- Long-running work must remain off the UI thread.

## 8. Evidence invariant
**Safety-critical verified defects may drive REJECT and cannot be overridden by price/cosmetics.**

## 9. Visual contract
Primary reference: `../references/approved/S06_PHYSICAL_SAFETY.jpg`

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
