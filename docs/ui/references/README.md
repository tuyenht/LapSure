# LapSure Visual Reference Pack

This directory stores the approved visual-design references for the LapSure Professional Dashboard.

## Authority and precedence

Visual mockups define **visual direction only**: information hierarchy, spacing, density, navigation style, cards, status presentation, primary actions and Vietnamese-first UX.

They are **not** diagnostic evidence and never authorize hard-coded sample values.

When anything conflicts, use this precedence:

1. `docs/COVERAGE_CONTRACT.md`
2. `docs/PRODUCT_SPEC.md`
3. `docs/ARCHITECTURE.md`
4. `docs/USED_LAPTOP_EXPERT_AUDIT.md`
5. `docs/ui/LAPSURE_UI_MASTER_SPEC.md`
6. `docs/ui/screens/*.md`
7. `docs/ui/references/*`

A mockup may contain illustrative values. If a value is not supported by real `AuditReport`/model/provider evidence, render the correct `UNKNOWN`, `NOT TESTED`, `UNSUPPORTED`, `INCOMPLETE`, unavailable or operator-confirmed state instead. Never change diagnostic semantics merely to match a screenshot.

## Files

- `CONTACT_SHEET.jpg` — compact overview of the approved design language across the currently designed screens.
- `LapSure_UI_Visual_Reference_Pack.zip` — complete compact visual pack containing all designed screen images and the contact sheet.
- `approved/` — individually promoted references that may be addressed directly by Screen ID.
- `archive/` — superseded concepts. Never use archive material as implementation authority unless explicitly requested.

## Screen map represented in the visual pack

- S01 — Tổng quan / Overview
- S02 — Khởi tạo phiên kiểm định / New session
- S03 — Cam kết người bán / Seller claim
- S04 — Kiểm tra Tự động / Automatic audit
- S05 — Kiểm tra Chức năng / Functional verification
- S06 — Ngoại hình & An toàn / Physical & safety inspection
- S07 — Cổng & Nguồn / Ports & power
- S08 — Stress & Ổn định / Stress & stability
- S09 — Pin & Năng lượng / Battery & power
- S12 — Hiển thị / Display
- S14 — Mạng & Kết nối / Network & connectivity
- S16 — Hồ sơ & Đối chiếu / Factory & seller comparison
- S17 — Thư viện bằng chứng / Evidence library
- S18 — Đánh giá cuối cùng & Báo cáo / Final report
- S19 — Xuất báo cáo & Chia sẻ / Export & share
- S20 — Nhật ký & Sự kiện / Logs & events
- S21 — Cài đặt / Settings

S10, S11, S13, S15, S22 and S23 are governed by their dedicated screen contracts even where an individual approved mockup is not yet promoted.

## Agent usage

Before implementing UI, read this file and `CONTACT_SHEET.jpg` after the product/spec contracts. Use the visual pack to keep the application coherent, but validate every displayed metric/status against the current model and providers.

For per-screen implementation, Screen Contract and evidence semantics always win over pixel fidelity.
