# LapSure UI/UX Master Specification v2

**Status:** normative implementation contract  
**Default language:** Vietnamese  
**Primary platform:** Windows x64; custom x64 WinPE where capabilities exist  
**Primary UX goal:** a first-time user understands current state, what needs attention and what to do next without reading a manual.

## 1. Non-negotiable UX principles
1. Guided before expert.
2. One obvious next action.
3. Evidence before confidence/verdict.
4. Never hide uncertainty.
5. Presence is not functionality.
6. Identity, factory, health, functionality, stability, coverage and confidence stay separate.
7. Vietnamese-first copy.
8. Progressive disclosure.
9. No diagnostic theatre or invented precision.
10. Native/portable Windows UX.

## 2. Information architecture
### Quy trình
S01 Tổng quan; S02 Phiên kiểm định mới; S04 Kiểm tra Tự động; S05 Kiểm tra Chức năng; S06 Ngoại hình & An toàn; S07 Cổng & Nguồn; S08 Stress & Ổn định.

### Chi tiết thiết bị
S09 Pin & Năng lượng; S10 Lưu trữ; S11 Bộ nhớ RAM; S12 Hiển thị; S13 Âm thanh & Camera; S14 Mạng & Kết nối; S15 Thông tin Hệ thống.

### Đánh giá & hồ sơ
S03 Cam kết người bán; S16 Hồ sơ & Đối chiếu; S17 Thư viện bằng chứng; S18 Đánh giá cuối cùng & Báo cáo; S19 Xuất báo cáo & Chia sẻ; S20 Nhật ký & Sự kiện; S22 Lịch sử phiên kiểm định.

### Utility/contextual
S21 Cài đặt. S23 Khôi phục phiên bị gián đoạn is contextual startup/history recovery, not a permanent primary tab. See `SCREEN_INDEX.md`.

## 3. Canonical workflow
Prepare S02 → S03 as purpose requires. Automatic evidence S04. Functional verification S05 with S12/S13/S14. Physical/safety S06 + S07. Final review S16 → S18 → S19. Evidence/history S17/S20/S22. Interactive result recording must not bypass the required automatic snapshot gate.

## 4. Page hierarchy
1. Page header + current context
2. Primary status/task
3. Main task content
4. Evidence/detail
5. Next-best action / secondary help

## 5. Shared normative contracts
- `DESIGN_SYSTEM.md`
- `COMPONENT_CATALOG.md` + `components/`
- `UI_STATE_MODEL.md`
- `DATA_BINDING_CONTRACT.md`
- `INTERACTION_PATTERNS.md`
- `UX_COPY_VI.md`
- `ACCESSIBILITY_DPI.md`
- `TRACEABILITY_MATRIX.md`
- `KNOWN_MOCKUP_DEVIATIONS.md`
- `UI_ACCEPTANCE_GATES.md`

## 6. Evidence UX contract
Every evidence-bearing row/card exposes as applicable: domain, result/status, actual/expected value, source/provider, session/timestamp, confidence, missing evidence and operator confirmation.

## 7. Final report contract
S18 separates recommendation, mandatory coverage, confidence, critical failures, warnings, seller/factory mismatch, unchecked/unsupported required items, causal reasons and negotiation notes after technical truth. Never replace these dimensions with a generic health score.

## 8. Engineering contract
Native C++20/Win32; shared theme/layout/components; diagnostic collection outside rendering; responsive UI thread; bounded cancellation; additive/tested model extensions; preserve HTML/JSON evidence semantics.

## 9. P0 completion
Grouped navigation, shared design/state/data components, S01–S23 resolved, mandatory uncertainty visible, workflow gates correct, interruption safe, final recommendation evidence-gated, strict build/regression gates green where runnable, real-machine certification tracked separately.
