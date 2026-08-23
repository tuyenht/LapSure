# S10 — Lưu trữ

## Purpose
Provide an evidence-based view of each physical storage device without collapsing identity, controller health, filesystem integrity and performance into one generic score.

## Data sources
Use existing `HardwareSnapshot.storage`, findings, volume-integrity evidence and trusted SMART/NVMe providers. Add provider-backed fields only when properly implemented and tested.

## Header
- Title: **Lưu trữ**
- Subtitle: `Danh tính ổ đĩa, SMART/NVMe, độ bền, nhiệt độ, lỗi và kiểm tra an toàn.`
- Drive selector when multiple physical devices exist.

## Summary cards
Show only supported metrics:
- Model / capacity / interface
- Health verdict from actual provider
- `Percentage Used` or endurance when exposed
- Temperature when exposed
- Power-on hours
- Unsafe shutdowns
- Media/uncorrectable errors
- Filesystem/volume integrity status

Never reinterpret `unsafeShutdowns > 0` as proof of drive failure.

## Main sections
1. **Danh tính ổ đĩa**
   - model, serial, firmware, interface, capacity.
2. **SMART / NVMe Health**
   - critical warning, available spare, percentage used, endurance, error entries, media/read/write uncorrectable errors.
3. **Lịch sử sử dụng**
   - power-on hours, cycles, data read/written when meaningful.
4. **Phân vùng & tính toàn vẹn**
   - show volume dirty/integrity findings separately from controller health.
5. **Hiệu năng an toàn**
   - only if a non-destructive, bounded benchmark exists; show baseline/version/source.
6. **Bằng chứng**
   - provider, timestamp, confidence, missing evidence.

## State rules
- Provider unavailable → **KHÔNG HỖ TRỢ/THIẾU BẰNG CHỨNG**, not green PASS.
- `smartReadable=false` → no SMART PASS.
- `reliabilityReadable=false` → no native-health PASS.
- Filesystem clean does not prove SSD health; SMART healthy does not prove filesystem clean.
- Critical warning/media errors may drive fail/reject according to existing decision policy.

## Actions
- `Kiểm tra lại`
- `Xem bằng chứng`
- `Mở Nhật ký & Sự kiện`
- optional `Chạy kiểm tra hiệu năng an toàn` only when implemented.

## Acceptance
No generic “SSD Health 98%” unless that exact percentage is a meaningful hardware-provided endurance/capacity metric and is labeled accurately.
