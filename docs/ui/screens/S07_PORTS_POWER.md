# S07 — Cổng & Nguồn

```yaml
screen_id: S07
status: approved-contract
visual_reference: S07_PORTS_POWER.jpg
components: [C01, C02, C03, C04, C08, C09, C10, C11, C12]
```

## 1. User outcome
Verify physical connectors and power by guided known-good stimulus.

## 2. Primary action
**Kiểm tra cổng tiếp theo**

## 3. Entry / exit
- Entry: preserve active inspection/session context and applicable orchestrator gates.
- Exit: navigation must not silently discard evidence or mutate diagnostic truth.
- If mandatory prerequisite evidence is absent, render the correct LOCKED/INCOMPLETE state and explain the next action.

## 4. Page anatomy / object inventory

| Object ID | Object | Binding rule |
|---|---|---|
| S07-O01 | Chassis visualization | Bind to real data/state; unavailable stays explicit |
| S07-O02 | Required/optional ports | Bind to real data/state; unavailable stays explicit |
| S07-O03 | Before/after stimulus | Bind to real data/state; unavailable stays explicit |
| S07-O04 | PnP/location/bus evidence | Bind to real data/state; unavailable stays explicit |
| S07-O05 | AC/power state | Bind to real data/state; unavailable stays explicit |
| S07-O06 | Known-good device guidance | Bind to real data/state; unavailable stays explicit |

## 5. Data sources
- `PortPowerSummary`
- `PortProbeResult`
- `PowerProbeResult`
- `ChassisProfile`

All values must comply with `../DATA_BINDING_CONTRACT.md`.

## 6. States
Implement applicable states from `../UI_STATE_MODEL.md`, including non-happy states: loading/running, warning, fail, incomplete, not-tested, unsupported/provider-unavailable, permission-denied, cancelled/interrupted and empty.

## 7. Interaction
- Keep one obvious primary action.
- Drill-down exposes evidence; it does not change the result.
- Manual decisions must be stored as operator-confirmed evidence.
- Long-running work must remain off the UI thread.

## 8. Evidence invariant
**Controller presence never certifies a physical port. Require stimulus/delta evidence.**

## 9. Visual contract
Primary reference: `../references/approved/S07_PORTS_POWER.jpg`

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
