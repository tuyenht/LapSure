# LapSure Production Hardening — Current Status

> Current verification ledger for PR #2. Read this together with the Round 5 design/plan and actual GitHub evidence. Evidence is always scoped to the exact commit and gate that produced it; compiled evidence is not physical-hardware evidence.

## Current product state

**Beta 0.1.1 / production-hardening. Round 5 source/compiled closure complete. Not production-certified.**

PR #2 remains **Draft**. Do not mark Ready, merge, or claim production-ready until portable package/provenance and formal physical acceptance gates are complete.

## Current PR candidate

- Branch: `feature/s01-s04-visual-alignment-v2`
- Validated Round 5 production-source candidate: `a20c42121398a3b2c1903347eb772287ab441e85`
- Promotion date: **2026-08-25**
- At promotion the candidate was **ahead of current `main`, behind 0**, and PR #2 was mergeable.
- PR #3 was a validation-only PR and was closed **without merge** after the exact validated SHA was fast-forwarded to PR #2.
- Later documentation-only commits may move the PR head while leaving the validated production-source tree unchanged. Any final package/provenance checkpoint must record its own exact head SHA.

## Round 5 Windows evidence chain

### Run #569 — `32830853638`

- Full source regression/configure reached the Windows build gate.
- Exposed three real compiled blockers: Win32 `LONG`/`int` clamp deduction, unused RAM-stage parameter under `/WX`, and missing `VerifyEngine` linkage in publication tests.
- Result: **useful failure evidence; not a promotable candidate**.

### Run #570 — `32835395238`

- Full source regression: PASS.
- Strict MSVC x64 compile/link: PASS.
- CTest exposed a Windows path-normalization false rejection in `VerifyEngine` while traversal/reparse negative cases remained fail-closed.
- Result: **compile/link evidence; behavioral gate still failed**.

### Run #571 — `32836090653`

Exact candidate: `f428b4e99e897da172e75f63aea14bf2d2179042`.

- Full source regression: PASS.
- Strict MSVC x64 `/W4 /WX` compile/link: PASS.
- All 5 CTest behavioral/security suites: PASS.
- Inventory-only executable exited successfully and published a transactional report generation, but the CI script incorrectly searched only the output-root top level for JSON rather than the `bundle-<token>` generation directory.
- Result: **runtime publication succeeded; CI validation contract was stale**.

### Run #572 — `32837575617`

Exact validated/promoted candidate: `a20c42121398a3b2c1903347eb772287ab441e85`.

Result: **SUCCESS for the complete promotion chain**.

Passed gates:

1. CI cost-control policy.
2. Full `run_source_tests.cmd` regression.
3. Strict MSVC x64 configure.
4. Release compile/link with `/W4 /WX`.
5. All 5 CTest behavioral/security suites.
6. Inventory-only provider preflight.
7. Transactional publication-layout validation: JSON inside `bundle-*`, sibling HTML present, session history committed, no residual `.staging-*` generation.
8. Inventory-only verdict remained `INCOMPLETE`; no stress stage was fabricated.

This proves the exact candidate's source/build/behavioral/inventory integration gates. It does **not** prove real-machine hardware correctness or visual/accessibility acceptance.

## Round 5 closed implementation contracts

1. **Branch reconciliation / CI hygiene**
   - current production candidate includes the public-profile privacy hotfix ancestry;
   - obsolete design-patch workflow is removed from the candidate;
   - Actions are pinned by full SHA in the active Windows workflow;
   - Draft/cost-control rules remain in force.

2. **Durable inspection identity / state root**
   - stable inspection/session identity is created before report publication;
   - journal/report/history paths use an explicit persistent state/output root;
   - missing evidence remains incomplete rather than becoming PASS.

3. **Transactional report publication**
   - HTML/JSON are staged together and atomically promoted as one `bundle-*` generation;
   - publication readiness is separate from hardware decision truth;
   - publication failure cannot rewrite a hardware verdict;
   - failure-injection coverage is compiled and passing.

4. **Transactional bounded session history**
   - bounded/schema-validated replay;
   - atomic candidate-save/swap semantics;
   - history commit is part of the report-publication boundary;
   - corruption/failure coverage is compiled and passing.

5. **External-engine/process trust closure**
   - duplicate logical allowlist entries fail closed;
   - malformed/empty hashes remain untrusted;
   - path traversal, absolute paths and reparse/redirection are rejected;
   - Windows path normalization is not incorrectly treated as reparse redirection;
   - child-process inherited handles are restricted;
   - trusted execution re-verifies at the launch boundary.

6. **Cloud factory privacy/provenance**
   - normal runtime network lookup is disabled by default;
   - technician pre-cache is explicit opt-in;
   - request/response sizes are bounded, query values encoded, redirects prohibited and host/HTTPS constrained;
   - cache/cloud provenance remains advisory/unauthenticated and cannot silently become Factory Exact;
   - mutable cache data is excluded from normal trusted local-profile loading.

7. **Product truth / repository hygiene**
   - S20 shows provider-unavailable semantics rather than confirmed zero when Event Log evidence is unavailable;
   - ordinary live logs are informational rather than PASS-like;
   - `build.cmd` builds source only; release synchronization is a separate verified operation;
   - runtime report/history/journal artifacts are ignored by default;
   - current public tree uses a synthetic SAMPLE factory fixture rather than a machine-specific public identifier.

8. **Legacy renderer production cleanup**
   - `src/ui_screens.cpp` is no longer compiled into the production `LapSure` target;
   - canonical v2/Round 5 S01–S23 renderers remain linked;
   - `src/ui_components.cpp` remains because it contains canonical shared UI primitives;
   - strict compile/link evidence for the removal is PASS in #572.

9. **Inventory-only validation alignment**
   - CI validates the actual transactional `bundle-*` layout rather than assuming top-level JSON;
   - sibling HTML, committed history and cleanup of staging generations are checked;
   - inventory-only remains non-stress and `INCOMPLETE`.

## Remaining release gate — portable package and provenance

Before formal pilot, run **one manual `workflow_dispatch`** on the exact final PR head. It must:

1. repeat source/strict build/CTest/inventory checks;
2. package the portable distribution;
3. verify package integrity and expected commit provenance;
4. upload the ZIP and checksum artifact;
5. record the exact final commit SHA, workflow/run/job IDs, ZIP SHA-256 and `LapSure.exe` SHA-256.

Do not make speculative production changes after this package checkpoint. If a production-code change becomes necessary, the candidate must be revalidated and re-packaged.

## Formal real-machine validation gate

`validation/REAL_MACHINE_MATRIX.tsv` still records Precision 5560 / 5570 / 7670 formal full-audit/runtime gates as `NOT RUN`.

Formal Precision runtime acceptance may begin only after package/provenance evidence is recorded. The pilot must include:

- exact verified package/hash used for the run;
- HTML/JSON/inspection-identity agreement;
- required functional and port stimulus evidence;
- no critical false PASS;
- offline/provider-unavailable and interrupted/recovery behavior where applicable;
- screenshot acceptance at required resolutions/scales;
- keyboard/focus/accessibility checks at the documented acceptance level;
- discrepancy log with disposition of every material discrepancy.

A single machine does not certify a model. Model/chassis certification still requires at least two independently validated physical units.

## Evidence interpretation rule

Always state proof level explicitly:

- source/contract checks;
- compiled unit/behavioral tests;
- integration/failure-injection tests;
- package/provenance evidence;
- physical runtime/visual/accessibility evidence.

Never report source or compiled assertions as equivalent to physical validation.

## CI cost-control rules

- Keep PR #2 Draft until physical acceptance policy permits otherwise.
- Draft PR synchronization must not allocate a Windows build runner.
- Documentation/design/Markdown-only changes must not allocate a Windows runner.
- Remote Windows CI is reserved for meaningful candidate checkpoints or blocking Windows-only defects.
- Portable packaging/upload remains manual `workflow_dispatch` only.
- No recurring or self-mutating validation workflows.
- Do not rerun an already-proven production-source candidate without a release/provenance or production-code reason.

## Branch hygiene after promotion

Keep during final package/pilot closure:

- `main`
- `feature/s01-s04-visual-alignment-v2`

Validation/scratch branches can be removed after their evidence is preserved in PR/history. `hotfix/public-profile-privacy` is identical to `main` at `33a8e2865c2685561b0a976f1823505102001d14`. PR #3 is already closed without merge; its branch and earlier Round 5/task scratch branches are no longer integration targets.

## Continuation rule

Before every production change:

1. Verify actual PR head and `main` state.
2. Confirm whether the change is required by a remaining package/pilot discrepancy.
3. Add or identify a failing behavioral/contract test before changing behavior where practical.
4. Make the smallest evidence-backed change.
5. Re-run only the proof level invalidated by the change, then the required final checkpoint.
6. Keep hardware truth, publication truth and physical acceptance evidence separate.
7. Preserve Draft/cost-control behavior until formal pilot evidence is complete.
