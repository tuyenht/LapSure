# S01 — Tổng quan

`components: [C01,C02,C03,C04,C05,C06,C07,C10]`  
`visual: ../references/approved/S01_OVERVIEW.jpg`

## User outcome
Understand the active device/session, whether evidence is sufficient, what needs attention and the next-best action.

## Objects
Device/session identity; recommendation state; mandatory evidence coverage; critical/warning counts; guided workflow; component summaries; next action.

## Data
`AuditReport`, `AuditDecision`, coverage/orchestrator, findings. Follow `DATA_BINDING_CONTRACT.md`.

## States
Idle/ready/running/warning/fail/incomplete/not-tested/unsupported/provider/permission/interrupted as applicable.

## Invariant
Do not show clean BUY/PASS while mandatory coverage is incomplete. Do not show a generic machine-health score; a gauge may represent clearly named evidence coverage only.

## Acceptance
One obvious CTA, explicit uncertainty, Vietnamese copy, DPI/keyboard compliance, no fabricated summary metric, executable screenshot compared with reference.
