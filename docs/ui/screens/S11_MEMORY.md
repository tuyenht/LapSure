# S11 — Bộ nhớ (RAM)

## Purpose
Show installed memory topology and the exact scope/results of memory testing.

## Data sources
Use `installedRamBytes`, `memoryModules`, `RamOnlineMetrics`, stress stages and trusted/preboot result only when actually available.

## Summary
- Tổng RAM
- Số mô-đun
- Configured/rated speed when available
- Online test status
- Bytes tested
- Passes
- Mismatches

## Module table
For every DIMM:
- locator/bank
- capacity
- manufacturer
- part number
- serial
- configured speed
- rated speed

Unknown fields remain `Không có dữ liệu`.

## Test section
Explain scope:
- current online allocated-memory pattern test;
- bytes allocated/tested;
- passes;
- mismatches;
- elapsed time;
- cancellation/interruption;
- evidence source.

A clean online test is **partial coverage** and must not be presented as full preboot memory certification.

## Actions
- `Bắt đầu kiểm tra RAM`
- `Chạy lại`
- `Xem bằng chứng`
- show `Kiểm tra trước khi khởi động` only if a real supported workflow exists.

## Failure semantics
Any non-zero confirmed mismatch is prominent and evidence-linked. Inability to allocate/test enough memory remains incomplete, not pass.
