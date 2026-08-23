# S10 — Lưu trữ

`components: [C01,C02,C03,C04,C05,C08,C09,C10,C12]`

## User outcome
Separate storage identity, SMART/NVMe health evidence, filesystem integrity and safe performance.

## Objects
Drive selector; identity; SMART/NVMe; endurance/wear; temperature; usage/history; filesystem/volume integrity; optional bounded performance; evidence.

## Data
`HardwareSnapshot.storage`, `StorageDevice`, volume-integrity findings, trusted SMART/NVMe providers.

## State rules
`smartReadable=false` => no SMART PASS. `reliabilityReadable=false` => no native-health PASS. Provider unavailable => explicit missing evidence/unsupported.

## Invariant
Filesystem clean != SSD health; SMART healthy != filesystem integrity. Unsafe shutdown count alone is not proof of drive failure. No generic “SSD Health 98%” unless it is an accurately labeled hardware-provided wear/endurance metric.

## Acceptance
Per-drive evidence source/status, explicit missing data and safe non-destructive actions only.
