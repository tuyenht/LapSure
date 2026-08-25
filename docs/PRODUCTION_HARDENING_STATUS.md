# LapSure Production Hardening — Current Status

> Current verification ledger for PR #2. Read this together with the Round 5 design/plan and actual GitHub evidence. Historical checkpoint success proves only the exact candidate and gates that were executed; source implementation presence is not equivalent to compiled/runtime validation.

## Current product state

**Beta 0.1.1 / production-hardening. Not production-certified. Not yet formal-pilot-ready.**

PR #2 remains **Draft**. Do not mark Ready, merge, or claim production-ready until Round 5 compiled closure and subsequent physical acceptance gates are complete.

## Current PR candidate

- Branch: `feature/s01-s04-visual-alignment-v2`
- Current Round 5 source candidate: `f6597e5df7ce071a61fc23c4e1f3c9197aef76b6`
- Reconciliation against `main` on 2026-08-25: **ahead 199 / behind 0**
- Normal Draft PR Windows runs for the Task 7/8 pushes were **skipped** by cost-control policy; no new compiled-green claim is attached to `f6597e5d...` yet.

## Last verified compiled automated checkpoint

- Date: **2026-08-25**
- Validated production-code candidate: `9ea73849666720ca59ee3b0d8279b2a53492be3d`
- Round 4 one-shot run: `32800503199`
- Result: **SUCCESS for the recorded automated checkpoint**

That checkpoint proved the exact Round 4 candidate passed full source regression, strict MSVC x64 Release `/W4 /WX`, compiled behavioral/process/trust suites, inventory-only preflight and package/provenance verification. It does **not** prove later Round 5 changes compile or pass runtime gates.

## Round 5 source of truth

- Design: `docs/superpowers/specs/2026-08-25-contract-closure-pilot-readiness-design.md`
- Plan: `docs/superpowers/plans/2026-08-25-contract-closure-pilot-readiness.md`

Feature expansion remains frozen until Round 5 contract closure.

## Round 5 implementation ledger

The following implementation work is present on the current PR branch, but later items still require the final compiled checkpoint before being treated as validated runtime behavior:

1. **Branch reconciliation / CI hygiene** — reconciled; obsolete design-patch automation remains removed; current branch is not behind `main`.
2. **Durable inspection identity / app entry** — Round 5 implementation and source contracts are present.
3. **Report publication transaction** — hardware truth is separated from publication state; publication implementation/tests are present.
4. **Transactional bounded session history** — bounded/schema-validated persistence and transaction implementation/tests are present.
5. **External-engine/process trust closure** — duplicate allowlist, reparse/redirection and restricted child-handle inheritance implementation/tests are present.
6. **Cloud factory trust/privacy** — landed in `d46d4be92753ab75eb0e2a96dd3488a71eda11ed`:
   - normal GUI/inventory network lookup is disabled by default;
   - technician pre-cache is explicit opt-in;
   - request/response sizes are bounded, query values encoded, redirects prohibited and host/HTTPS constrained;
   - cloud/cache provenance remains advisory/unauthenticated and cannot silently become trusted factory truth;
   - normal factory-profile loading excludes mutable cache data.
7. **Product truth / repository hygiene** — landed in `f6597e5df7ce071a61fc23c4e1f3c9197aef76b6`:
   - S20 uses explicit provider-unavailable semantics instead of presenting unavailable event counts as confirmed zero;
   - ordinary live logs remain informational rather than PASS-like;
   - `build.cmd` is local-source-build only and release synchronization is a separate SHA-256-verified operation;
   - runtime evidence artifacts are ignored by default;
   - the public machine-specific Precision profile was replaced with an explicit synthetic `SAMPLE` fixture.

### Verification level of Tasks 6–7

Source/contract changes are present and were reviewed against their intended invariants. The current chat environment could not clone the GitHub snapshot for a fresh whole-repository source-suite execution, and Draft PR CI intentionally skipped the Windows runner. Therefore **do not describe the current head as full-regression-green or compiled-green yet**.

## Task 9 — legacy renderer production compilation

`src/ui_screens.cpp` is still listed in the production target under `_Legacy` symbol renames while canonical S01–S23 implementations are provided by the evidence-bound v2/Round 5 translation units.

The Round 5 plan requires **actual link/build evidence**, not source-string inference, before removing that object from the production target. `src/ui_components.cpp` must remain compiled because it contains canonical shared UI primitives even though two legacy shell functions inside it are renamed.

Recommended closure sequence:

1. Prepare the minimal candidate that removes only `src/ui_screens.cpp` from the `LapSure` production target.
2. Run full source regression + strict MSVC `/W4 /WX` + CTest on that exact candidate.
3. Promote the removal only if the exact candidate links and all required gates pass; otherwise keep the known routing and record legacy compilation as deferred cleanup.

Do not remove the legacy renderer from the PR solely on substring/source evidence.

## Final Round 5 compiled checkpoint

Before formal pilot, the exact final candidate must pass:

```cmd
run_source_tests.cmd
```

```powershell
cmake --preset msvc-x64-ci
cmake --build --preset build-msvc-x64-ci
ctest --test-dir out/build/msvc-x64-ci -C Release --output-on-failure
```

The GitHub workflow additionally runs the inventory-only provider preflight. A manual `workflow_dispatch` also packages the portable candidate, verifies package/commit provenance and uploads the exact artifact.

Record the final commit SHA, workflow/run/job IDs and package SHA-256. Do not make further speculative production changes after that checkpoint.

## Real-machine validation status

`validation/REAL_MACHINE_MATRIX.tsv` currently records Precision 5560 / 5570 / 7670 full-audit/runtime gates as `NOT RUN`.

Hardware testing before Round 5 compiled closure may be used only as an **exploratory dry-run**. Formal Precision runtime acceptance may begin only after:

- Round 5 source/compiled gates are complete;
- branch history remains reconciled;
- one strict Windows checkpoint passes on the exact candidate;
- exact package/provenance evidence is recorded.

The formal pilot must confirm HTML/JSON/inspection identity agreement, required functional/port evidence, no critical false PASS and disposition of every material discrepancy. Model/chassis certification still requires at least two independently validated physical units.

## Evidence interpretation rule

Always report proof level explicitly:

- source/contract checks;
- compiled unit/behavioral tests;
- integration/failure-injection tests;
- physical runtime evidence.

Never report source assertions as equivalent to compiled/runtime or physical evidence.

## CI cost-control rules

- Keep PR #2 Draft during active Round 5 work and until physical acceptance policy permits otherwise.
- Draft PR Windows jobs remain skipped.
- Documentation/design/Markdown-only changes must not allocate a Windows runner.
- Use a remote Windows runner only for a meaningful candidate checkpoint or a Windows-only blocker.
- Portable packaging/upload remains manual `workflow_dispatch` only.
- No recurring/self-mutating validation workflows.
- Do not rerun an already-proven candidate without a production-code reason.

## Continuation rule

Before every production change:

1. Verify actual PR head and `main` state.
2. Confirm the next unresolved Round 5 contract.
3. Add/identify a failing behavioral or contract test before changing behavior where practical.
4. Make the smallest evidence-backed change.
5. Run focused verification, then full source/compiled regression at the required proof level.
6. Do not mark a runtime contract closed from a source-string check alone.
7. Preserve Draft/cost-control behavior until the formal-pilot candidate exists.
