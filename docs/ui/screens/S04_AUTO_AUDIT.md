# S04 — Kiểm tra Tự động

`components: [C01,C02,C03,C04,C06,C07,C08,C10,C12]`  
`visual: ../references/approved/S04_AUTO_AUDIT.jpg`

## User outcome
Run and monitor automatic evidence collection as an operational control center.

## Objects
Mode selector; completed/total progress; domain rows; current step; elapsed time; evidence/provider source; warning/failure reason; live log; next-best action.

## Data
`OrchestratorSummary`, findings, `HardwareSnapshot`, runtime/live log.

## States
Waiting, running, complete, warning, fail, incomplete, not-tested, unsupported/provider/permission error, cancelled, interrupted.

## Invariant
Enumeration/presence is not functional PASS. Provider/permission/unsupported states remain explicit. Do not fake ETA or progress precision.

## Acceptance
Automatic snapshot gate remains intact; CPU is visible as its own domain where applicable; user can tell what is happening, what is wrong and what comes next.
