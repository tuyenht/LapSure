# Contributing to LapSure

LapSure is currently in build and hardware-validation beta. Contributions should prioritize correctness, reproducibility and evidence quality over adding more probes.

## Before a code change
- Identify whether the change affects identity, health, functionality, stability or decision logic.
- Define PASS / WARNING / FAIL / NOT TESTED semantics.
- Do not turn missing capability into hardware failure.
- Do not turn device presence into functional proof.

## Build target
Use Visual Studio 2022 / MSVC x64 and CMake presets. Run `build_msvc_ci.cmd` for the strict build.

## Tests
Run `run_source_tests.cmd`. Hardware-facing changes should also add a real-machine validation case under `validation/`.

## Pull requests
Include problem statement, evidence/source of expected behavior, implementation summary, verdict semantics, regression tests and hardware-validation notes when applicable.

Do not commit third-party diagnostic binaries without explicit review of provenance, license, command contract and hash pinning.