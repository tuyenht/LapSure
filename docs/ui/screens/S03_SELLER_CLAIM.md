# S03 — Cam kết người bán

```yaml
screen_id: S03
status: approved-contract
visual_reference: S03_SELLER_CLAIM.jpg
components: [C01, C02, C03, C09, C10, C11]
```

## 1. User outcome
Record seller-declared configuration and commercial terms for later comparison.

## 2. Primary action
**Lưu cam kết**

## 3. Entry / exit
- Entry: preserve active inspection/session context and applicable orchestrator gates.
- Exit: navigation must not silently discard evidence or mutate diagnostic truth.
- If mandatory prerequisite evidence is absent, render the correct LOCKED/INCOMPLETE state and explain the next action.

## 4. Page anatomy / object inventory

| Object ID | Object | Binding rule |
|---|---|---|
| S03-O01 | Model | Bind to real data/state; unavailable stays explicit |
| S03-O02 | CPU | Bind to real data/state; unavailable stays explicit |
| S03-O03 | RAM | Bind to real data/state; unavailable stays explicit |
| S03-O04 | Storage | Bind to real data/state; unavailable stays explicit |
| S03-O05 | GPU | Bind to real data/state; unavailable stays explicit |
| S03-O06 | Display | Bind to real data/state; unavailable stays explicit |
| S03-O07 | Asking price | Bind to real data/state; unavailable stays explicit |
| S03-O08 | Warranty | Bind to real data/state; unavailable stays explicit |
| S03-O09 | Listing/source/notes | Bind to real data/state; unavailable stays explicit |

## 5. Data sources
- `SellerClaim`
- `Evidence attachments when implemented`

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
Primary reference: `../references/approved/S03_SELLER_CLAIM.jpg`

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
