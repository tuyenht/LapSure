# LapSure Production Hardening — Current Status

> Current verification ledger. Read this together with PR #2, the latest expert audit and GitHub evidence before continuing implementation. Historical checkpoint success proves the tested candidate built and passed its recorded gates; it does not prove every design contract was implemented.

## Current product state

**Beta 0.1.1 / production-hardening. Not production-certified. Not yet formal-pilot-ready.**

PR #2 remains Draft. Do not mark Ready, merge, or claim production-ready until the Round 5 contract-closure gates and subsequent physical validation gates are complete.

## Last verified automated checkpoint

- Date: **2026-08-25**
- PR: **#2**
- Branch: `feature/s01-s04-visual-alignment-v2`
- Validated production-code candidate: `9ea73849666720ca59ee3b0d8279b2a53492be3d`
- Round 4 one-shot run: `32800503199`
- Result: **SUCCESS for the recorded automated checkpoint**

The checkpoint proved that candidate passed the generated Round 4 RED/GREEN migration gates, full source regression, strict MSVC x64 Release `/W4 /WX`, compiled behavioral/process/trust suites, inventory-only preflight and package/provenance verification. It also removed the temporary one-shot workflow/migration helpers.

### Important scope correction

The 2026-08-25 expert re-audit found that several Round 4 design contracts were not fully closed by the implementation even though the candidate compiled and the recorded tests passed. Therefore the next release blocker is **Round 5 — Contract Closure & Pilot Readiness**, not formal physical acceptance by itself.

The Round 4 success record must be kept as valid build/test evidence; it must not be overstated as proof of complete production hardening.

## Round 5 source of truth

- Design: `docs/superpowers/specs/2026-08-25-contract-closure-pilot-readiness-design.md`
- Plan: `docs/superpowers/plans/2026-08-25-contract-closure-pilot-readiness.md`

Work must proceed in plan order. Feature expansion is frozen until the P0 contract work is closed.

## P0 blockers before formal pilot

1. **Branch reconciliation / CI hygiene** — reconcile the four `main` commits outside the PR history and do not resurrect the obsolete design-patch workflow.
2. **Durable inspection identity** — create one immutable inspection ID before evidence collection and share it across journal/history/HTML/JSON, including inventory-only mode.
3. **Report publication transaction** — separate hardware decision from publication/storage status; partial file-system failure must not rewrite hardware truth or expose a clean partial bundle.
4. **Transactional bounded session history** — candidate-snapshot commit, rollback on failure, bounded/schema-validated persisted input and safe delete semantics.
5. **External-engine/process trust closure** — reject duplicate logical allowlist entries and documented redirection/reparse cases; restrict child handle inheritance. Keep engine hashes empty until intentionally reviewed.
6. **Cloud factory trust/privacy** — no unverified cloud/cache result may become `factoryExact`; network lookup must be privacy-conscious, bounded, encoded and opt-in/disabled by default until provenance is established.
7. **Product truth / repo hygiene** — fix S20 unavailable/log semantics, remove unsupported UI claims, split build from release-sync behavior, ignore generated identifying artifacts and anonymize committed physical identifiers where appropriate.

## P1 before final compiled candidate when safe

`src/ui_screens.cpp` is still compiled under `_Legacy` symbol renames while canonical v2 translation units provide production screen symbols. Remove obsolete legacy production compilation only after P0 work is locally green and only if symbol/link evidence shows it is safe. Batch this with the same final compiled checkpoint rather than creating a separate Windows run.

## Real-machine validation status

`validation/REAL_MACHINE_MATRIX.tsv` currently records Precision 5560 / 5570 / 7670 full-audit/runtime gates as `NOT RUN`.

Hardware testing before Round 5 code closure may be used as an **exploratory dry-run**, but it must not be recorded as formal runtime acceptance/certification evidence for the final candidate.

Formal Precision pilot may begin only after:

- Round 5 P0 tasks are complete;
- branch history is reconciled;
- local/source and compiled tests are green;
- one strict Windows checkpoint passes on the exact candidate;
- exact package/provenance evidence is recorded.

The formal pilot must then confirm HTML/JSON/inspection identity agreement, required functional/port evidence, no critical false PASS and disposition of every material discrepancy. Model/chassis certification still requires at least two independently validated physical units.

## Evidence interpretation rule

Tests must be reported by proof level:

- contract/source lint;
- compiled unit/behavioral tests;
- integration/failure-injection tests;
- physical runtime evidence.

Do not report a large source-assertion count as equivalent to runtime behavioral coverage.

## CI cost-control rules

- Keep PR #2 Draft during active Round 5 work.
- Draft PR Windows build jobs remain skipped.
- Prefer local/source tests during iteration.
- Use remote Windows CI only for the final meaningful code checkpoint unless a blocking Windows-only defect makes an earlier run necessary.
- Portable packaging/upload remains manual `workflow_dispatch` only.
- No self-mutating recurring workflows.
- Do not rerun a passing checkpoint without a production-code reason.

## Continuation rule

Before every code change:

1. Verify actual PR head and `main` state.
2. Confirm the next unchecked Round 5 task.
3. Add/identify a failing behavioral or contract test before changing behavior where practical.
4. Make the smallest change that closes that task without feature expansion.
5. Run focused tests, then source regression.
6. Do not mark the task closed from a source-string check alone when runtime behavior is the contract.
7. Preserve Draft/cost-control behavior until the formal-pilot candidate exists.
