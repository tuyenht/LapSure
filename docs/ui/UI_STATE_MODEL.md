# Canonical UI State Model

This file is normative. Screen implementations map backend/model states into these canonical presentation states without losing uncertainty.

| State | Vietnamese | Meaning | Clean PASS? |
|---|---|---|---|
| IDLE | Chưa bắt đầu | no run | No |
| READY | Sẵn sàng | prerequisites met | No |
| LOCKED | Chưa thể thực hiện | gated | No |
| RUNNING | Đang kiểm tra | operation active | No |
| PAUSED | Đã tạm dừng | resumable pause | No |
| PASS | Đạt | required condition actually proved | local only |
| GOOD | Tốt | positive non-binary condition | contextual |
| WARNING | Cần lưu ý | abnormality/note | No clean global PASS by itself |
| FAIL | Không đạt | verified failure | No |
| CHANGED | Có thay đổi | identity/factory/seller delta | not health failure by itself |
| INCOMPLETE | Chưa đủ dữ liệu | mandatory evidence incomplete | No |
| NOT_TESTED | Chưa kiểm tra | not performed | No |
| UNSUPPORTED | Không hỗ trợ | capability unsupported | No |
| MANUAL_REQUIRED | Cần xác nhận | operator evidence required | No |
| PROVIDER_UNAVAILABLE | Không có nguồn dữ liệu | provider unavailable | No |
| PERMISSION_DENIED | Thiếu quyền truy cập | evidence blocked | No |
| CANCELLED | Đã hủy | cancelled | No |
| INTERRUPTED | Bị gián đoạn | crash/reboot journal | No |
| EMPTY | Chưa có dữ liệu | no records | No |
| ERROR | Có lỗi khi kiểm tra | execution/parsing error | No |

## Mappings
`State`: Pass→PASS, Good→GOOD, Warning→WARNING, Fail→FAIL, Changed→CHANGED, NotTested→NOT_TESTED, Unsupported→UNSUPPORTED, Info→neutral info.

`FunctionalStatus`: Pass→PASS, Warning→WARNING, Fail→FAIL, NotTested→NOT_TESTED, Unsupported→UNSUPPORTED, ManualRequired→MANUAL_REQUIRED.

`TestStageState`: Locked→LOCKED, Ready→READY, Running→RUNNING, Passed→PASS, Warning→WARNING, Failed→FAIL, Incomplete→INCOMPLETE.

## Decision labels
BUY→`CÓ THỂ MUA`; BUY WITH NOTES→`CÓ THỂ MUA — CẦN LƯU Ý`; REJECT→`KHÔNG NÊN MUA`; INCOMPLETE→`CHƯA ĐỦ DỮ LIỆU ĐỂ KẾT LUẬN`.

## Hard invariants
Uncertainty/provider/permission/cancel/interruption never render green PASS; enumeration/presence never maps directly to functional PASS; factory mismatch is not automatic hardware FAIL.
