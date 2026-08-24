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
S01 Tổng quan  
S02 Phiên kiểm định mới  
S04 Kiểm tra Tự động  
S05 Kiểm tra Chức năng  
S06 Ngoại hình & An toàn  
S07 Cổng & Nguồn  
S08 Stress & Ổn định

### Chi tiết thiết bị
S09 Pin & Năng lượng  
S10 Lưu trữ  
S11 Bộ nhớ RAM  
S12 Hiển thị  
S13 Âm thanh & Camera  
S14 Mạng & Kết nối  
S15 Thông tin Hệ thống

### Đánh giá & hồ sơ
S03 Cam kết người bán  
S16 Hồ sơ & Đối chiếu  
S17 Thư viện bằng chứng  
S18 Đánh giá cuối cùng & Báo cáo  
S19 Xuất báo cáo & Chia sẻ  
S20 Nhật ký & Sự kiện  
S22 Lịch sử phiên kiểm định

### Utility/contextual
S21 Cài đặt  
S23 Khôi phục phiên bị gián đoạn — contextual startup/history recovery, not a permanent primary tab.

See `SCREEN_INDEX.md` for all contracts.

## 3. Canonical workflow
Prepare: S02 → S03 as inspection purpose requires.  
Automatic evidence: S04.  
Functional verification: S05 with detail S12/S13/S14.  
Physical/safety: S06 + S07.  
Final review: S16 → S18 → S19.  
Evidence/history: S17/S20/S22 available when context exists.

Interactive result recording must not bypass the required automatic snapshot gate.

## 4. Page hierarchy
1. Page header + current context
2. Primary status/task
3. Main task content
4. Evidence/detail
5. Next-best action / secondary help

## 5. Shared normative contracts
- Visual system: `DESIGN_SYSTEM.md`
- Components: `COMPONENT_CATALOG.md` + `components/`
- States: `UI_STATE_MODEL.md`
- Data: `DATA_BINDING_CONTRACT.md`
- Interaction: `INTERACTION_PATTERNS.md`
- Vietnamese copy: `UX_COPY_VI.md`
- Accessibility/DPI: `ACCESSIBILITY_DPI.md`
- Traceability: `TRACEABILITY_MATRIX.md`
- Mockup corrections: `KNOWN_MOCKUP_DEVIATIONS.md`
- Definition of Done: `UI_ACCEPTANCE_GATES.md`

## 6. Evidence UX contract
Every evidence-bearing row/card must be able to expose as applicable:
- domain,
- result/status,
- actual value,
- expected value,
- source/provider,
- session/timestamp,
- confidence,
- missing evidence,
- operator confirmation.

## 7. Final report contract
S18 separates:
- recommendation,
- mandatory evidence coverage,
- confidence,
- critical failures,
- warnings,
- seller/factory mismatches,
- unchecked/unsupported required items,
- causal reasons,
- negotiation notes only after technical truth.

Never replace these dimensions with a generic health score.

## 8. Engineering contract
- Native C++20/Win32.
- Shared theme/layout/component helpers.
- Diagnostic collection outside rendering.
- Responsive UI thread.
- Bounded cancellation.
- Additive/tested model extensions.
- Preserve HTML/JSON evidence semantics.

## 9. P0 completion
Professional Dashboard P0 requires:
- grouped navigation,
- shared design/state/data component system,
- all S01–S23 contracts resolved,
- mandatory uncertainty states visible,
- workflow gates correct,
- interrupted-session handling safe,
- final recommendation evidence-gated,
- strict build/regression gates green where runnable,
- real-machine certification tracked separately.
