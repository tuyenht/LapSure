# LapSure Production Hardening — Current Status

> Current verification ledger for PR #2. Read this together with the Round 5 design/plan and actual GitHub evidence. Evidence is always scoped to the exact commit and gate that produced it; compiled evidence is not physical-hardware evidence.

## Current product state

**Beta 0.1.1 / production-hardening. Round 5 contract closure plus P1 portable-profile provenance remediation is compiled-green. Not production-certified.**

PR #2 remains **Draft**. Do not mark Ready, merge, or claim production-ready until a replacement portable package/provenance checkpoint and formal physical acceptance gates are complete.

## Current production-source candidate

- Branch: `feature/s01-s04-visual-alignment-v2`
- Exact validated production-source candidate: `b5b04711941dc469e8a0a06a61710f60df7c8328`
- Validation date: **2026-08-25**
- Base `main`: `33a8e2865c2685561b0a976f1823505102001d14`
- At validation the branch was **behind 0**, PR #2 was mergeable, and it was returned to **Draft** after the justified Windows checkpoint was launched.
- Markdown-only ledger/readme updates after this SHA do not alter the validated production source. A replacement package/provenance run must nevertheless record its own exact final head SHA.

## Historical package — superseded for formal pilot

The previously frozen package was created from:

`7116ffbacce92011f60be5f9858b8afeb9fe4227`

Historical evidence:
- workflow_dispatch run `32839326920` (#576)
- Job `97775129173`
- Artifact `9560018587` (`LapSure-windows-x64-portable`)
- ZIP SHA-256 `232806b55e26c2fb873855d8eaa0fe6091956eca5d69267d2db95516edfd4567`
- `LapSure.exe` SHA-256 `b9b985466a41447c5eecc54c6bee250361b92ef3118242dd51ef3d264a71187e`
- GitHub artifact digest `sha256:e8fd76fe65595e6e76c3e0a8c7b36f32e91867038875ffe76de0d472206d464b`

This remains valid **historical package evidence only**. It is **not approved for the formal physical pilot** because an independent post-freeze security review found a real P1 acceptance-verdict-integrity defect in mutable portable profile trust.

## P1 acceptance-verdict integrity finding and closure

### Finding

LapSure currently runs elevated for some diagnostic providers while the portable application directory can be user-writable. Static factory JSON and chassis `.profile` files in that directory were therefore mutable after extraction.

Before the remediation:
- a matching static factory JSON could be treated as trusted/exact factory truth and influence Factory PASS/FAIL evidence;
- a chassis profile could self-declare `validationStatus=physical-verified` and change the required-port gate;
- both values have a direct path into `BuildAuditDecision()`.

This was classified **P1 — Acceptance Verdict Integrity** and was sufficient to break the old package freeze.

### Remediation

1. **Factory static JSON is advisory only**
   - `LoadFactoryProfile()` may parse a top-level portable static file for advisory metadata;
   - it does not set `loaded`, `exact`, or `trustedProvenance` for decision truth;
   - mutable cache remains excluded from the normal factory-truth loader.

2. **Decision factory boundary is fail-closed**
   - `LoadDecisionFactoryProfile()` requires `loaded && exact && trustedProvenance`;
   - otherwise it clears the profile before hardware collectors can emit Factory PASS/FAIL expectations.

3. **Cloud decision boundary remains provenance-gated**
   - `LookupFactoryProfileForDecision()` requires success + exact identity match + authenticated provenance together;
   - HTTPS/cache success by itself is insufficient.

4. **Chassis metadata cannot self-certify**
   - `LoadDecisionChassisProfile()` preserves advisory port guidance;
   - any disk-supplied `physical-verified` authority is downgraded to `static-unverified`;
   - mutable portable metadata therefore cannot unlock a clean acceptance verdict.

5. **Production routing is centralized**
   - `src/main_round5.cpp` routes GUI/inventory decision paths through the safe wrappers;
   - technician `RunBatchPreCache()` remains a separate explicit opt-in path and is not disabled by this routing.

6. **Regression coverage**
   - `LapSureProfileProvenanceTests` creates attacker-controlled factory/chassis files and verifies advisory-vs-decision behavior;
   - Round 5 cloud/profile source sanity was updated to reject the old `trustedProvenance=true` assumption;
   - `SECURITY.md` documents the portable-profile boundary.

### Residual limitation

Mutable profile data cannot become authenticated factory/chassis truth until LapSure introduces a protected trust mechanism such as an embedded/hash-pinned allowlist, signed metadata, or an ACL-protected installation boundary. This is intentional fail-closed behavior, not an implicit certification claim.

## Fresh Windows checkpoint — run #588, attempt 2

Workflow run: `32851743158`  
Exact production-source SHA: `b5b04711941dc469e8a0a06a61710f60df7c8328`  
Successful job: `97814496751`

The first attempt of #588 was cancelled by the workflow concurrency rule when PR #2 was immediately converted back to Draft. It was **not** a compile/test failure. The same job was rerun using the original ready-event context while the PR itself remained Draft.

Fresh evidence from successful attempt 2:

1. **CI cost-control policy: PASS**
2. **Full `run_source_tests.cmd` regression: PASS**
   - includes `Round 5 cloud/profile privacy and provenance contract: PASS`
3. **Strict MSVC x64 configure: PASS**
4. **Release compile/link with `/W4 /WX`: PASS**
5. **CTest: 6/6 PASS, 0 failed**
   - `LapSureBehaviorTests`
   - `LapSureReportPublicationTests`
   - `LapSureSessionHistoryTests`
   - `LapSureTrustSecurityTests`
   - `LapSureProcessSecurityTests`
   - `LapSureProfileProvenanceTests`
6. **Inventory-only provider preflight: PASS**
   - executable exited successfully;
   - JSON published inside a transactional `bundle-*` generation;
   - sibling HTML present;
   - `session_history.tsv` committed;
   - no residual `.staging-*` directory;
   - no stress completion/stages fabricated;
   - verdict remained `INCOMPLETE`.

This is fresh source/build/behavioral/integration evidence for the P1-remediated production source. It does **not** prove physical-hardware correctness, visual acceptance, accessibility acceptance, or model certification.

## Previous Round 5 evidence chain

### Run #569 — `32830853638`

- Source/configure reached Windows build gate.
- Exposed three real compiled blockers.
- Result: useful failure evidence; not promotable.

### Run #570 — `32835395238`

- Full source regression: PASS.
- Strict compile/link: PASS.
- CTest exposed a Windows path-normalization trust false rejection.
- Result: compile/link evidence; behavioral gate still failed.

### Run #571 — `32836090653`

Exact candidate `f428b4e99e897da172e75f63aea14bf2d2179042`.

- Source regression: PASS.
- Strict `/W4 /WX` compile/link: PASS.
- Then-current 5 CTest suites: PASS.
- Inventory executable succeeded, while CI still assumed reports were at output-root top level.
- Result: implementation worked; validation script was stale.

### Run #572 — `32837575617`

Exact production-source candidate `a20c42121398a3b2c1903347eb772287ab441e85`.

- CI cost-control: PASS.
- Full source regression: PASS.
- Strict MSVC x64 compile/link: PASS.
- Then-current 5 CTest suites: PASS.
- Inventory-only transactional publication preflight: PASS.

This remains historical pre-P1 evidence.

## Closed Round 5 implementation contracts

1. **Branch reconciliation / CI hygiene**
   - obsolete design-patch workflow removed;
   - active Actions pinned by full SHA;
   - Draft/cost-control rules enforced.

2. **Durable inspection identity / state root**
   - stable inspection/session identity established before publication;
   - journal/report/history use an explicit persistent state/output root.

3. **Transactional report publication**
   - HTML/JSON staged together and promoted as one `bundle-*` generation;
   - publication readiness separated from hardware decision truth;
   - publication failure cannot rewrite hardware verdict.

4. **Transactional bounded session history**
   - bounded/schema-validated replay;
   - atomic candidate-save/swap semantics;
   - corruption/failure coverage compiled and passing.

5. **External-engine/process trust closure**
   - malformed/duplicate allowlist entries fail closed;
   - traversal/reparse/redirection rejected;
   - child-process inherited handles restricted;
   - trusted engine re-verification occurs at launch boundary.

6. **Cloud/profile privacy and provenance**
   - normal runtime cloud network access disabled by default;
   - technician pre-cache explicit opt-in;
   - encoded/bounded HTTPS request, redirects prohibited, identity checked;
   - unauthenticated cloud/cache/static portable factory data remains advisory;
   - mutable chassis metadata cannot self-assert physical-verification authority.

7. **Product truth / repository hygiene**
   - EventLog unavailable state remains conservative;
   - ordinary logs are informational;
   - source build separated from release synchronization;
   - runtime report/history/journal artifacts ignored;
   - public repository uses synthetic SAMPLE factory fixture.

8. **Legacy renderer production cleanup**
   - `src/ui_screens.cpp` is not part of the production `LapSure` target;
   - canonical v2/Round 5 renderers link under strict MSVC.

9. **Inventory-only validation alignment**
   - CI validates actual transactional bundle layout;
   - inventory-only remains non-stress and `INCOMPLETE`.

## Remaining gate before formal pilot — replacement package/provenance

A new manual `workflow_dispatch` must now be run on the exact final PR head. It must:

1. repeat source regression;
2. repeat strict MSVC x64 build/link;
3. run all **6** CTest suites;
4. repeat inventory-only transactional preflight;
5. build the portable ZIP;
6. verify package integrity and expected commit provenance;
7. upload the replacement artifact;
8. record exact final commit SHA, workflow/run/job IDs, ZIP SHA-256, `LapSure.exe` SHA-256 and GitHub artifact digest.

Do **not** reuse artifact `9560018587` for the formal pilot.

The connected GitHub toolset used for this hardening session does not expose a workflow-dispatch write action, so this manual packaging event must be launched through GitHub Actions UI/API by an authorized operator. Do not weaken the workflow by enabling package creation on ordinary PR events merely to bypass this control.

## Formal real-machine validation gate

`validation/REAL_MACHINE_MATRIX.tsv` still records Precision 5560 / 5570 / 7670 formal full-audit/runtime gates as `NOT RUN`.

Formal Precision runtime acceptance may begin only after replacement package/provenance evidence is recorded. The pilot must include:

- exact verified package/hash used for the run;
- HTML/JSON/inspection-identity agreement;
- required functional and port stimulus evidence;
- no critical false PASS;
- offline/provider-unavailable and interrupted/recovery behavior where applicable;
- screenshot acceptance at required resolutions/scales;
- keyboard/focus/accessibility checks at the documented acceptance level;
- discrepancy log with disposition of every material discrepancy.

A single machine does not certify a model. Model/chassis certification still requires independently validated physical evidence, and mutable portable profile text alone cannot constitute authenticated certification.

## Evidence interpretation rule

Always state proof level explicitly:

- source/contract checks;
- compiled unit/behavioral tests;
- integration/failure-injection tests;
- package/provenance evidence;
- physical runtime/visual/accessibility evidence.

Never report source or compiled assertions as equivalent to physical validation.

## CI cost-control rules

- Keep PR #2 Draft except for the minimum justified event transition needed to launch a Windows checkpoint when no workflow-dispatch write path is available.
- Draft PR synchronization must not allocate a normal Windows build runner.
- Documentation/design/Markdown-only changes must not allocate a Windows runner.
- Remote Windows CI is reserved for meaningful candidate checkpoints or blocking Windows-only/security defects.
- Portable packaging/upload remains manual `workflow_dispatch` only.
- No recurring or self-mutating validation workflows.
- If a production-code change occurs after a package checkpoint, revalidate and repackage before physical pilot.

## Continuation rule

Before every production change:

1. Verify actual PR head and `main` state.
2. Confirm the change is required by a remaining package/pilot discrepancy or validated security defect.
3. Add or identify a failing behavioral/contract test before changing behavior where practical.
4. Make the smallest evidence-backed change.
5. Re-run only the proof level invalidated by the change, then the required final checkpoint.
6. Keep hardware truth, publication truth, profile provenance and physical acceptance evidence separate.
7. Preserve Draft/cost-control behavior until formal pilot evidence is complete.
