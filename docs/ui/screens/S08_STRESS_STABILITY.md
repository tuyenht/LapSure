# S08 — Stress & Ổn định

```yaml
screen_id: S08
status: approved-contract
visual_reference: S08_STRESS_STABILITY.jpg
components: [C01, C02, C03, C04, C06, C08, C10, C11, C12]
```

## 1. User outcome
Run controlled workload and show real telemetry/error deltas.

## 2. Primary action
**Bắt đầu/Dừng kiểm tra**

## 3. Entry / exit
- Entry: preserve active inspection/session context and applicable orchestrator gates.
- Exit: navigation must not silently discard evidence or mutate diagnostic truth.
- If mandatory prerequisite evidence is absent, render the correct LOCKED/INCOMPLETE state and explain the next action.

## 4. Page anatomy / object inventory

| Object ID | Object | Binding rule |
|---|---|---|
| S08-O01 | Stress stages | Bind to real data/state; unavailable stays explicit |
| S08-O02 | Progress/elapsed | Bind to real data/state; unavailable stays explicit |
| S08-O03 | CPU/GPU telemetry if trusted | Bind to real data/state; unavailable stays explicit |
| S08-O04 | WHEA/storage/display/bugcheck deltas | Bind to real data/state; unavailable stays explicit |
| S08-O05 | Throttle evidence | Bind to real data/state; unavailable stays explicit |
| S08-O06 | Cancel/interruption state | Bind to real data/state; unavailable stays explicit |

## 5. Data sources
- `StressSession`
- `StressStageResult`
- `TelemetrySummary`
- `CpuBenchmarkResult`

All values must comply with `../DATA_BINDING_CONTRACT.md`.

## 6. States
Implement applicable states from `../UI_STATE_MODEL.md`, including non-happy states: loading/running, warning, fail, incomplete, not-tested, unsupported/provider-unavailable, permission-denied, cancelled/interrupted and empty.

## 7. Interaction
- Keep one obvious primary action.
- Drill-down exposes evidence; it does not change the result.
- Manual decisions must be stored as operator-confirmed evidence.
- Long-running work must remain off the UI thread.

## 8. Evidence invariant
**Interrupted/cancelled stages remain incomplete; only display telemetry actually collected.**

## 9. Visual contract
Primary reference: `../references/approved/S08_STRESS_STABILITY.jpg`

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
