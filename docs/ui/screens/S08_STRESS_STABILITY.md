# S08 — Stress & Ổn định

`components: [C01,C02,C03,C04,C06,C08,C10,C11,C12]`  
`visual: ../references/approved/S08_STRESS_STABILITY.jpg`

## User outcome
Run controlled workload and understand stability/error deltas using only real telemetry.

## Objects
Stress stages; real progress/elapsed; CPU/GPU telemetry when trusted; WHEA/storage/display/bugcheck deltas; throttle evidence; cancellation/interruption.

## Data
`StressSession`, `StressStageResult`, `TelemetrySummary`, `CpuBenchmarkResult`.

## Invariant
Interrupted/cancelled stages remain incomplete. Unsupported temperature/power is unavailable, not invented. Historical logs are not confused with controlled pre/post deltas.

## Acceptance
UI stays responsive; cancellation bounded; journal state preserved; no false completion.
