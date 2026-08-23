# S11 — Bộ nhớ RAM

`components: [C01,C02,C03,C04,C05,C06,C08,C09,C10,C12]`

## User outcome
Show DIMM inventory and exact coverage of online memory testing.

## Objects
Installed total; DIMM list; configured/rated speed; manufacturer/part/serial; bytes tested; passes; mismatches; coverage limitation.

## Data
`installedRamBytes`, `memoryModules[]`, `RamOnlineMetrics`.

## Invariant
A clean online allocated-memory test remains partial coverage and must not be presented as full preboot memory certification.

## Acceptance
Module identity separate from test result; bytes/passes/mismatches visible when real; unsupported/not-run explicit.
