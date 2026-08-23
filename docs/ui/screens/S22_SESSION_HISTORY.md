# S22 — Lịch sử phiên kiểm định

## Purpose
Allow buyers/technicians/service centers to reopen completed or interrupted inspection sessions and their reports/evidence.

## Storage policy
Implement only against an actual local session/report index. Do not invent cloud history. The initial implementation may index local LapSure report/session directories.

## List/table
Columns:
- ngày/giờ
- mã phiên
- model
- service tag/serial when available
- mode
- final verdict
- coverage
- status: completed/interrupted/incomplete
- report availability

## Filters
- search model/service tag/session ID
- date range
- verdict
- completed/incomplete/interrupted

## Detail preview
- device identity
- recommendation
- warnings/critical fails
- coverage/confidence
- report/evidence paths
- last activity

## Actions
- `Mở phiên`
- `Mở báo cáo`
- `Mở thư mục bằng chứng`
- `So sánh với phiên hiện tại` only after a real comparison model exists
- `Xóa khỏi lịch sử` requires confirmation and must distinguish index removal from deleting evidence files.

## Empty state
Explain where LapSure stores reports and offer `Bắt đầu phiên kiểm định mới`.

## Acceptance
No session appears unless backed by real local report/journal metadata.
