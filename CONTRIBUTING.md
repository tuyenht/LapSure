# Contributing to LapSure

LapSure prioritizes correctness, reproducibility and evidence quality over adding more probes.

## Before any code change
- Identify affected diagnostic dimension.
- Define PASS/WARNING/FAIL/NOT TESTED semantics.
- Do not turn missing capability into hardware failure.
- Do not turn presence into functional proof.

## UI / Professional Dashboard changes
Before changing UI:
1. Identify Screen ID in `docs/ui/SCREEN_INDEX.md`.
2. Update the screen contract first if behavior changes.
3. Update component/data/state contracts when shared behavior changes.
4. Update approved visual reference only when visual direction changes.
5. Add/update `KNOWN_MOCKUP_DEVIATIONS.md` if visual concept and evidence semantics differ.
6. Implement.
7. Build/test.
8. Capture executable screenshot and audit against the reference.
9. Update `TRACEABILITY_MATRIX.md` when requirement/code/test mapping changes.

Never make the screenshot the only specification.

## Build
Visual Studio 2022 / MSVC x64 / C++20. Use repository strict build flow.

## Tests
Run `run_source_tests.cmd`; hardware-facing changes need real-machine validation notes.

## Pull requests
Include problem statement, source/evidence, affected Screen/Component IDs, implementation summary, state semantics, tests, visual comparison and hardware-validation notes.

Do not commit third-party diagnostic binaries without provenance/license/contract/hash review.
