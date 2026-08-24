# Canonical UI State Model

This file is normative. Screen-specific implementations must map backend/model states into these canonical presentation states without losing uncertainty.

| Canonical UI state | Vietnamese | Meaning | Clean PASS allowed? |
|---|---|---|---|
| IDLE | Chưa bắt đầu | No run initiated | No |
| READY | Sẵn sàng | Preconditions satisfied | No |
| LOCKED | Chưa thể thực hiện | Gated by prior workflow | No |
| RUNNING | Đang kiểm tra | Operation active | No |
| PAUSED | Đã tạm dừng | Resumable pause | No |
| PASS | Đạt | Test actually proved required condition | Yes, local domain only |
| GOOD | Tốt | Positive non-binary condition | Context dependent |
| WARNING | Cần lưu ý | Non-critical abnormality / note | No clean global PASS by itself |
| FAIL | Không đạt | Verified failure | No |
| CHANGED | Có thay đổi | Identity/factory/seller delta | Not a health failure by itself |
| INCOMPLETE | Chưa đủ dữ liệu | Mandatory evidence incomplete | No |
| NOT_TESTED | Chưa kiểm tra | Test not performed | No |
| UNSUPPORTED | Không hỗ trợ | Environment/provider cannot support test | No |
| MANUAL_REQUIRED | Cần xác nhận | Operator stimulus/judgment required | No until recorded |
| PROVIDER_UNAVAILABLE | Không có nguồn dữ liệu | Expected provider unavailable | No |
| PERMISSION_DENIED | Thiếu quyền truy cập | Evidence blocked by privileges | No |
| CANCELLED | Đã hủy | User/system cancelled | No |
| INTERRUPTED | Bị gián đoạn | Crash/reboot/interruption journal detected | No |
| EMPTY | Chưa có dữ liệu | No applicable records | No |
| ERROR | Có lỗi khi kiểm tra | Execution/parsing failure | No |

## Backend mappings

### `State`
- Pass → PASS
- Good → GOOD
- Warning → WARNING
- Fail → FAIL
- Changed → CHANGED
- NotTested → NOT_TESTED
- Unsupported → UNSUPPORTED
- Info → neutral informational presentation

### `FunctionalStatus`
- Pass → PASS
- Warning → WARNING
- Fail → FAIL
- NotTested → NOT_TESTED
- Unsupported → UNSUPPORTED
- ManualRequired → MANUAL_REQUIRED

### `TestStageState`
- Locked → LOCKED
- Ready → READY
- Running → RUNNING
- Passed → PASS
- Warning → WARNING
- Failed → FAIL
- Incomplete → INCOMPLETE

## Decision labels

- `BUY` → `CÓ THỂ MUA`
- `BUY WITH NOTES` → `CÓ THỂ MUA — CẦN LƯU Ý`
- `REJECT` → `KHÔNG NÊN MUA`
- `INCOMPLETE` → `CHƯA ĐỦ DỮ LIỆU ĐỂ KẾT LUẬN`

## Hard invariants

- UNKNOWN/NOT_TESTED/UNSUPPORTED/INCOMPLETE/permission/provider errors never render as green PASS.
- Enumeration/presence never maps directly to functional PASS.
- Factory mismatch maps to CHANGED/WARNING as policy dictates, not automatic hardware FAIL.
- Interrupted/cancelled work never maps to completed PASS.
