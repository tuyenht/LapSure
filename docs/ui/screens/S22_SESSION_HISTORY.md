# S22 — Lịch sử phiên kiểm định

`components: [C01,C02,C03,C04,C06,C09,C10,C12]`

## User outcome
Reopen, search and compare prior inspection sessions/reports when persistence exists.

## Objects
Search/filter; device/model/serial; timestamp; decision; coverage; report availability; compare/reopen.

## Data
Persisted session/report index only when implemented. If persistence is not yet implemented, render an explicit staged/empty state rather than sample sessions.

## Invariant
Historical decisions do not become current evidence for a new session without explicit comparison/context.

## Acceptance
No fake history rows; missing report files are explicit; reopening preserves original evidence provenance.
