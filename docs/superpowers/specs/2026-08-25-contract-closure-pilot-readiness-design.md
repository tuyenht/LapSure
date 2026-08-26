# Round 5 — Contract Closure & Pilot Readiness Design

**Status:** Approved direction from the 2026-08-25 expert audit  
**PR:** #2 (`feature/s01-s04-visual-alignment-v2`)  
**Product state:** Beta 0.1.1 / production-hardening; not production-certified

## 1. Goal

Close evidence-backed correctness, persistence, trust, privacy and product-truth gaps before the next Precision run is treated as a formal runtime-acceptance pilot.

Round 4 remains valid evidence that code candidate `9ea73849666720ca59ee3b0d8279b2a53492be3d` compiled and passed the recorded automated checkpoint. It is not evidence that every Round 4 design contract was implemented. Round 5 narrows that gap without adding product features.

## 2. Non-goals

- No new diagnostic domains or marketing features.
- No speculative UI redesign.
- No production/stable claim.
- No external engine enablement until the trust boundary is complete.
- No recurring/self-mutating CI.
- No formal model certification from a single physical machine.

## 3. Invariants

1. Missing, malformed, stale, contradictory, timed-out, unsupported or untrusted evidence never becomes clean PASS/BUY.
2. Hardware decision and artifact-publication state are separate truths.
3. One immutable inspection ID is created before evidence collection and is shared by journal, history, HTML and JSON.
4. Persistence mutations are transactional from the caller's point of view: failure does not leave an in-memory state claiming a commit that did not persist.
5. Persisted files are untrusted input and are size/schema/path bounded when reopened.
6. External tools execute only after fail-closed trust validation at the execution boundary.
7. Cloud-derived factory data is not `factoryExact` without authenticated provenance and exact identity matching.
8. User-visible copy must not claim unsupported capabilities or 100% coverage.
9. Source-lint tests are guardrails, not substitutes for compiled behavioral/integration tests.
10. PR #2 remains Draft; remote Windows CI is reserved for one meaningful final code checkpoint.

## 4. Workstreams

### A. Branch reconciliation and CI hygiene

Reconcile the four `main` commits that are currently outside the PR history before final validation. Preserve the current cost-controlled `windows-msvc-build.yml`. The obsolete `apply-s01-s04-design-patch.yml` must not be resurrected into the final candidate.

### B. Durable inspection identity and state root

Create an inspection identity at inspection start, independent of the stress stage. Journal and report/history code consume the same ID and an explicit persistent state/output root. Inventory-only mode also creates one stable ID before writing either report.

### C. Report publication transaction

Introduce a publication result/status separate from `AuditDecision`. Render HTML and JSON to staged files, verify both writes, then publish the pair and history record as one bounded operation. A file-system failure can withhold publication readiness but must not rewrite a valid hardware decision.

### D. Transactional session history

Mutations operate on a candidate snapshot, persist it atomically, and only then replace live state. Loaders enforce a versioned schema plus maximum file/line/field sizes. Delete operations validate every artifact path before state mutation and preserve recoverability if a later file operation fails.

### E. External-engine and process hardening

`VerifyEngine()` must reject zero, malformed and duplicate logical-name entries, and must explicitly reject reparse/redirection cases covered by the documented trust policy. Behavioral tests cover duplicate entries, traversal, absolute paths and reparse behavior. Child process handle inheritance is restricted to the intended stdio handles.

External-engine hashes remain empty until these gates pass and a reviewed release binary is intentionally pinned.

### F. Cloud factory trust and privacy

Cloud lookup is opt-in/disabled by default for Beta unless an existing explicit user setting already provides consent. URL parameters are encoded, response sizes and schema are bounded, and a remote/cache result cannot set `factoryExact=true` without exact returned identity plus authenticated provenance. Unverified cloud/cache results remain advisory evidence.

### G. Product-truth and repository hygiene

Correct S20 provider-unavailable semantics and default runtime-log severity. Remove unsupported claims such as "100% hardware identification" and digital-signature output where the backend is unavailable. Split `build.cmd` from release download/sync behavior and require verification for any future release downloader. Ignore generated report/journal/history artifacts. Replace real device identifiers in committed fixtures with synthetic test identities where appropriate.

### H. Legacy renderer cleanup

Only after the P0 contracts above are green locally, remove obsolete legacy renderer production compilation if it can be done without losing required symbols/tests. Treat this as P1 hygiene and include it in the same final compiled checkpoint rather than triggering a separate runner.

## 5. Verification model

Each workstream must distinguish:

- **contract/source lint** — static guardrails;
- **compiled unit/behavioral tests** — function-level behavior;
- **integration/failure-injection tests** — publication/history/recovery behavior;
- **physical evidence** — actual hardware/runtime behavior.

A large count of source assertions must never be reported as equivalent to behavioral coverage.

## 6. Pilot readiness gate

A formal Precision runtime-acceptance pilot may begin only when:

1. all P0 Round 5 tests pass locally/source-side;
2. branch history is reconciled with `main`;
3. the resulting production-code candidate passes one strict MSVC x64 `/W4 /WX` + CTest checkpoint;
4. package/provenance verification passes for the exact pilot artifact;
5. no known P0 correctness/security/privacy blocker remains.

Before that point, hardware runs are exploratory dry-runs only.

## 7. Release gate

Even after one formal pilot passes, LapSure remains Beta until the documented release gates are satisfied. A model/chassis profile remains draft until at least two independently validated physical units support certification and material discrepancies have been dispositioned.
