# S15 — Thông tin Hệ thống

`components: [C01,C02,C03,C04,C08,C09,C12]`

## User outcome
Present system identity, firmware/security, environment and PnP problem evidence.

## Objects
Model/serial; CPU/mainboard; BIOS/SMBIOS; OS/environment; TPM; Secure Boot; PnP problem devices; runtime/provider readiness.

## Data
`AuditReport`, `MainboardInfo`, `BiosInfo`, `SecurityInfo`, `PnpProblemDevice`, `RuntimeValidationSummary`.

## Invariant
Known/unknown security states stay separate. No PnP problem list is not universal hardware certification. Environment/provider limitations are not hardware failure.

## Acceptance
Technical details copyable/readable; unknown fields explicit; sources available in evidence drill-down.
