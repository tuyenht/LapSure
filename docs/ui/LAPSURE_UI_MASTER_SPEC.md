# LapSure UI/UX Master Specification

**Status:** implementation contract  
**Default language:** Vietnamese  
**Primary platform:** Windows x64; custom x64 WinPE where capabilities exist  
**Product:** LapSure — Kiểm định & Chẩn đoán Laptop  
**Primary UX goal:** a first-time user should understand what is happening, what needs attention, and what to do next without reading a manual.

---

## 1. UX principles

1. **Guided before expert.** The primary flow must guide a buyer/technician from start to final decision. Deep technical evidence is available on demand.
2. **One obvious next action.** Every workflow screen shows a primary next action and explains why it is next.
3. **Evidence before confidence.** UI confidence, coverage and verdicts must be derived from evidence already represented by the product model.
4. **Never hide uncertainty.** Unknown/unsupported/not-tested/incomplete states are visible and understandable.
5. **Presence is not functionality.** Detected hardware can be shown as present without being marked functional.
6. **Separate dimensions.** Identity, factory expectation, health, functionality, stability, coverage and confidence remain visually and semantically distinct.
7. **Vietnamese-first copy.** Technical acronyms may remain English when industry-standard, but explanatory copy is Vietnamese.
8. **Progressive disclosure.** Novices get summary cards and instructions; experts can drill into raw values, providers, logs and evidence.
9. **No diagnostic theatre.** Do not show fake gauges, invented percentages or implied precision that the engine does not have.
10. **Portable native desktop.** Visual improvements must not undermine the native/portable Windows and WinPE goals.

---

## 2. Final information architecture

Use a grouped sidebar. Do not present 20+ flat items with equal visual weight.

### Group A — Quy trình
- **S01 Tổng quan**
- **S02 Phiên kiểm định mới**
- **S04 Kiểm tra Tự động**
- **S05 Kiểm tra Chức năng**
- **S06 Ngoại hình & An toàn**
- **S07 Cổng & Nguồn**
- **S08 Stress & Ổn định**

### Group B — Chi tiết thiết bị (collapsible)
- **S09 Pin & Năng lượng**
- **S10 Lưu trữ**
- **S11 Bộ nhớ (RAM)**
- **S12 Hiển thị (Màn hình)**
- **S13 Âm thanh & Camera**
- **S14 Mạng & Kết nối**
- **S15 Thông tin Hệ thống**

### Group C — Đánh giá & hồ sơ
- **S03 Cam kết người bán**
- **S16 Hồ sơ & Đối chiếu**
- **S17 Thư viện bằng chứng**
- **S18 Đánh giá cuối cùng & Báo cáo**
- **S19 Xuất báo cáo & Chia sẻ**
- **S20 Nhật ký & Sự kiện**
- **S22 Lịch sử phiên kiểm định**

### Utility
- **S21 Cài đặt**
- **S23 Khôi phục phiên bị gián đoạn** appears contextually on startup or from session history, not as a permanent primary tab.

The current `MainTab` enum may be evolved incrementally. Preserve backward behavior while introducing grouped navigation.

---

## 3. Canonical guided workflow

### Stage 0 — Prepare
S02 New Session → optional/required S03 Seller Claim depending inspection purpose.

### Stage 1 — Automatic Hardware Audit
S04 runs the automatic snapshot and permitted automatic/stress providers.

Interactive results are blocked until the automatic snapshot is complete, preserving workflow integrity.

### Stage 2 — Functional Verification
S05 orchestrates display, keyboard/touchpad, speaker, camera, microphone, Wi‑Fi/Bluetooth and other operator-required tests. Domain-specific pages S12/S13/S14 provide detail.

### Stage 3 — Physical & Safety Verification
S06 checks chassis/safety. S07 verifies physical ports/power by guided known-good stimulus.

### Stage 4 — Final Review
S16 compares actual/factory/seller claims → S18 explains verdict → S19 exports/share package.

At any point after evidence exists, S17/S20 provide drill-down.

---

## 4. Screen catalog

| ID | Screen | Primary purpose | Primary action |
|---|---|---|---|
| S01 | Tổng quan | Current device/session status and next-best action | Bắt đầu/Tiếp tục kiểm tra |
| S02 | Phiên kiểm định mới | Create inspection context and mode | Bắt đầu phiên kiểm định |
| S03 | Cam kết người bán | Record seller listing/configuration/terms | Lưu cam kết |
| S04 | Kiểm tra Tự động | Run and monitor automatic evidence collection | Bắt đầu/Tạm dừng/Tiếp tục |
| S05 | Kiểm tra Chức năng | Guided operator functional checks | Tiếp tục bài kiểm tra |
| S06 | Ngoại hình & An toàn | Chassis, hinge, liquid, swelling, charger safety | Xác nhận bước |
| S07 | Cổng & Nguồn | Model-aware physical connector stimulus | Kiểm tra cổng tiếp theo |
| S08 | Stress & Ổn định | Controlled load, telemetry, error deltas | Bắt đầu/Dừng test |
| S09 | Pin & Năng lượng | Battery capacities, wear, charge/discharge evidence | Bắt đầu phiên pin |
| S10 | Lưu trữ | Storage identity, SMART/NVMe, integrity and safe performance | Kiểm tra lại/Xem bằng chứng |
| S11 | Bộ nhớ (RAM) | DIMM inventory and memory-test coverage | Bắt đầu/Chạy lại kiểm tra |
| S12 | Hiển thị | EDID/current mode + guided visual defects | Màu tiếp theo/Xác nhận |
| S13 | Âm thanh & Camera | Camera sample, mic evidence, stereo L/R | Chạy bài tiếp theo |
| S14 | Mạng & Kết nối | Wi‑Fi/Bluetooth/LAN evidence and stability | Bắt đầu kiểm tra kết nối |
| S15 | Thông tin Hệ thống | BIOS/mainboard/security/PnP/environment evidence | Xem/copy chi tiết |
| S16 | Hồ sơ & Đối chiếu | Actual vs factory vs seller claim | Xử lý chênh lệch |
| S17 | Thư viện bằng chứng | Review evidence objects by source/domain | Mở/Gắn vào báo cáo |
| S18 | Đánh giá cuối cùng & Báo cáo | Evidence-gated recommendation and explanation | Xuất báo cáo |
| S19 | Xuất báo cáo & Chia sẻ | HTML/JSON/PDF/print/package options | Xuất |
| S20 | Nhật ký & Sự kiện | Search/filter system and inspection event evidence | Mở bằng chứng |
| S21 | Cài đặt | Mode/UI/evidence/security settings | Lưu thay đổi |
| S22 | Lịch sử phiên kiểm định | Reopen/search/compare past sessions | Mở phiên |
| S23 | Khôi phục phiên bị gián đoạn | Recover journal-backed interrupted test | Khôi phục/Đóng phiên |

---

## 5. Global layout contract

### Desktop target
- Optimize first for 1366×768 through 1920×1080 at Windows 100–150% DPI.
- Layout must remain usable at minimum supported window size; avoid clipped primary actions.
- Prefer vertical scrolling inside the content region instead of shrinking text below readable sizes.

### Shell
- Left navigation: dark navy, grouped sections, active item blue.
- Main content: light neutral background with white cards.
- Header: screen title, concise subtitle, optional Help/Quick settings.
- Persistent bottom/status strip is optional but if present must show real state only (engine/providers/database/report policy).
- Right guidance rail may be used for workflow screens, but should collapse at narrower widths.

### Content hierarchy
1. Page title + current context.
2. Primary status/progress/decision.
3. Main task content.
4. Evidence/detail.
5. Secondary/help information.

Do not let logs, developer diagnostics or raw JSON dominate novice screens.

---

## 6. Design system baseline

Existing `UiColors` is the implementation baseline. Refactor to tokens rather than scattering RGB literals.

### Core semantic tokens
- `surface/sidebar`: current `SidebarBg`
- `surface/content`: current `ContentBg`
- `surface/card`: current `CardBg`
- `brand/primary`: current `PrimaryBlue`
- `status/good`: current `SuccessGreen`
- `status/warning`: current `WarnAmber`
- `status/fail`: current `FailRed`
- `status/info`: current `InfoBlue`
- `status/neutral`: gray family

### Typography
Primary typeface: Segoe UI / Segoe UI Variable when safely available; fall back to Windows system UI fonts.
- Page title: 22–26 px equivalent, semibold/bold.
- Section title: 16–18 px, semibold.
- Body: 13–15 px.
- Supporting text: 12–13 px.
- Technical/raw data: Consolas or equivalent monospaced font.

Never reduce font size solely to fit a crowded design. Reflow instead.

### Spacing
Adopt a 4 px base scale:
- 4 micro
- 8 compact
- 12
- 16 normal
- 24 section
- 32 major separation

### Components
Reusable components should cover:
- Sidebar item/group
- Page header
- Primary/secondary/destructive buttons
- Status badge
- Metric card
- Coverage badge/progress
- Evidence source chip
- Guided stepper
- Data table
- Expandable evidence row
- Empty/error/unsupported state
- Confirmation/safety dialog
- Evidence preview card
- Next-best-action panel

### Color rules
Never rely on color alone. Every status requires text and/or icon.
Warning is amber/orange; Fail is red; Active/running is blue; Good/Pass is green; Not tested/unsupported is neutral gray with explicit wording.

---

## 7. Canonical UI state vocabulary

The code currently exposes multiple state types. UI must map them without losing meaning.

### Finding state mapping
- `Pass` → **ĐẠT**
- `Good` → **TỐT**
- `Warning` → **CẦN LƯU Ý**
- `Fail` → **KHÔNG ĐẠT**
- `Changed` → **CÓ THAY ĐỔI**
- `NotTested` → **CHƯA KIỂM TRA**
- `Unsupported` → **KHÔNG HỖ TRỢ**
- `Info` → **THÔNG TIN**

### Functional additions
- `ManualRequired` → **CẦN XÁC NHẬN**
- `Cancelled` → **ĐÃ HỦY**
- interrupted journal/session → **BỊ GIÁN ĐOẠN**

### Decision/coverage strings
Preserve explicit:
- `INCOMPLETE`
- `UNKNOWN`
- `NOT TESTED`
- `NOT SCORED`
- `PARTIAL`

User-facing Vietnamese explanations may accompany these strings, but the semantics must not be collapsed into green/pass states.

### Suggested user-facing verdict labels
- `BUY` → **CÓ THỂ MUA**
- `BUY WITH NOTES` → **CÓ THỂ MUA — CẦN LƯU Ý**
- `REJECT` → **KHÔNG NÊN MUA**
- `INCOMPLETE` → **CHƯA ĐỦ DỮ LIỆU ĐỂ KẾT LUẬN**

Do not invent a generic overall “health score” as a replacement for the decision model. If a score-like visualization exists, it must represent a clearly named measurable quantity such as **Độ bao phủ bằng chứng**, not a fabricated machine-health percentage.

---

## 8. Automatic audit screen contract (S04)

This is the operational control center, not a static inventory page.

Must show:
- mode: Nhanh / Tiêu chuẩn / Chuyên sâu;
- completed/total progress;
- current item;
- elapsed time;
- status per domain;
- evidence source/provider summary;
- explicit warning/error reason;
- next-best action;
- live log collapsed by default or in lower panel.

Per-item states include:
`waiting`, `running`, `complete`, `warning`, `failed`, `not-tested`, `unsupported`, `cancelled`, `interrupted/incomplete`.

Do not show PASS merely because enumeration succeeded.

---

## 9. Functional and physical UX rules

Interactive tests must:
- tell the operator exactly what to do;
- show what counts as a successful observation;
- offer **Đạt / Cảnh báo / Lỗi / Bỏ qua** or equivalent when human judgment is required;
- record that the evidence was operator-confirmed and its confidence where supported;
- explain that skipped mandatory checks keep the final decision incomplete.

For ports:
- use model-aware labels when a validated chassis profile exists;
- when a profile is draft/generic, visually disclose the lower confidence;
- verify by stimulus/delta rather than controller presence.

---

## 10. Final report UX rules

S18 must clearly separate:
1. Final recommendation.
2. Coverage/confidence.
3. Critical fails/warnings.
4. Why the recommendation was produced.
5. Seller/factory mismatches.
6. Unchecked/unsupported required items.
7. Negotiation notes only after technical truth.

Every recommendation must allow drill-down to exact findings/evidence.

Price or cosmetic considerations may influence negotiation guidance but may never turn an electrical/safety/critical technical reject into BUY.

---

## 11. Evidence UX contract summary

Every evidence-bearing UI row/card should be able to expose:
- domain;
- result/status;
- actual measured value;
- expected value if known;
- source/provider;
- timestamp/session;
- confidence if available;
- missing evidence when incomplete;
- operator action when manual.

Evidence source labels should use stable technical names where helpful: WMI/CIM, SMBIOS, SetupAPI, EDID, SMART/NVMe provider, trusted engine, operator confirmation, journal, event log.

---

## 12. Accessibility and Windows behavior

- Keyboard navigation for all primary controls.
- Visible focus states.
- Tab order follows visual order.
- Enter/Space activates buttons; Esc exits modal/full-screen visual tests where appropriate.
- Do not use emoji as the sole production icon mechanism; use a consistent icon set or native vector/resource icons with text fallback.
- Respect Windows scaling/DPI.
- Do not place essential meaning only in hover state.
- Destructive actions require confirmation.
- Stress/battery/port procedures must surface safety/instruction notes before or during execution.

---

## 13. Engineering constraints

- Preserve the existing C++20 / Win32 project and strict MSVC build.
- Prefer reusable native rendering/layout components over repeated manual drawing blocks.
- Keep diagnostic collection outside UI rendering code.
- UI thread must remain responsive during tests.
- Worker cancellation must remain bounded and safe.
- Avoid blocking UI on slow provider calls.
- Report regeneration must preserve current evidence/decision semantics.
- Any model extension for UI must be additive and covered by behavioral tests.

---

## 14. P0 completion scope

Professional Dashboard implementation is not considered P0-complete until:
- grouped navigation exists;
- S01–S21 existing/core views use the shared design/state system;
- the six detailed missing contracts S10, S11, S13, S15, S22, S23 are implemented or explicitly staged with no false UI claim;
- automatic/manual workflow gating is preserved;
- all mandatory uncertain states are visible;
- final report cannot imply acceptance with incomplete required coverage;
- build and regression gates are green;
- real-device validation remains a separate release gate.

See `screens/` for the six P0 missing-screen contracts.
