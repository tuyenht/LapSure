# S23 — Khôi phục phiên bị gián đoạn

`components: [C01,C03,C04,C06,C08,C10,C11,C12]`

## User outcome
Handle journal-backed interrupted tests safely without converting partial work into PASS.

## Objects
Detected interrupted session; completed/invalid/incomplete work; journal path/evidence; resume/restart/close options; safety explanation.

## Data
`StressSession.previousInterruptedSessionDetected`, `journalPath`, journal evidence.

## Invariant
Interrupted work never silently becomes PASS. Closing/discarding recovery context requires explicit confirmation and must preserve evidence according to policy.

## Acceptance
Shown context matches journal; next action clear; recovered/restarted outcome remains evidence-correct and reportable.
