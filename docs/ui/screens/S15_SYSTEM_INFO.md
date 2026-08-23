# S15 — Thông tin Hệ thống

## Purpose
Provide technician-grade identity, firmware, security, PnP and environment evidence without overwhelming the guided workflow.

## Sections
1. **Thiết bị**
   - manufacturer/model/service tag when exposed
   - mainboard manufacturer/product/serial
2. **BIOS / SMBIOS**
   - vendor/version/release date/SMBIOS
3. **Hệ điều hành & môi trường**
   - Windows/WinPE
   - architecture
   - relevant runtime capability status
4. **Bảo mật**
   - TPM present/ready
   - Secure Boot known/enabled
5. **Thiết bị có vấn đề (PnP)**
   - problem code
   - device
   - description
   - instance ID drill-down
6. **Runtime / provider integrity**
   - trusted engines/runtime validation status
   - explicit missing/untrusted provider information

## UX
Use summary cards plus expandable technical tables. Provide copy buttons for Service Tag, BIOS version and instance IDs.

## Semantics
- BIOS “old” may be advisory only unless an authoritative OEM currency provider is implemented.
- PnP Code 10/22/28/43 must be represented accurately and interpreted through existing policy; do not hard-code blanket causality such as “Code 43 = reballed/dead GPU”.
- TPM/Secure Boot state is security evidence, not generic hardware-health score.

## Actions
- `Sao chép thông tin`
- `Xem bằng chứng`
- `Mở Nhật ký & Sự kiện`
- OEM lookup only when an explicit trusted/navigation workflow exists.
