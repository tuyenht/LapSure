# Round 5 Contract Closure & Pilot Readiness Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the correctness, persistence, trust, privacy and product-truth P0 gaps identified by the 2026-08-25 expert audit before formal Precision runtime acceptance.

**Architecture:** Preserve LapSure's Provider → Evidence Model → Decision/Orchestrator → Presentation flow. Add explicit inspection identity, publication state and transactional persistence boundaries rather than mutating `AuditDecision` for I/O failures. Keep external/cloud evidence fail-closed and unverified until provenance is established.

**Tech Stack:** Native C++20/Win32, MSVC x64, CMake/CTest, Python contract tests, GitHub Actions with Draft-PR cost controls.

**Spec:** `docs/superpowers/specs/2026-08-25-contract-closure-pilot-readiness-design.md`

## Global Constraints

- PR #2 stays Draft until Round 5 code + formal pilot gates are complete.
- No new product feature scope.
- No clean PASS/BUY from missing/untrusted/incomplete evidence.
- Hardware decision must remain independent from publication/storage failure.
- External engines stay disabled by empty allowlist hashes until trust gates are intentionally closed.
- Do not use recurring/self-mutating GitHub workflows.
- Run local/source tests during iteration; use one final remote Windows checkpoint after the production-code batch is complete.

---

### Task 1: Correct the repository source of truth

**Files:**
- Modify: `README.md`
- Modify: `docs/PRODUCTION_HARDENING_STATUS.md`
- Create: `docs/superpowers/specs/2026-08-25-contract-closure-pilot-readiness-design.md`
- Create: `docs/superpowers/plans/2026-08-25-contract-closure-pilot-readiness.md`
- Update: PR #2 body/comment

**Produces:** A truthful ledger stating Round 4 is a valid compiled checkpoint but formal pilot is blocked by Round 5 P0 contracts.

- [ ] **Step 1: Remove the report-transaction overclaim**

Replace the README sentence that says HTML/JSON are already saved transactionally with wording that transaction closure is a Round 5 release gate.

- [ ] **Step 2: Replace “physical pilot is next” with “contract closure first”**

List the known P0 workstreams: identity/state root, publication transaction, history transaction/bounds, trust/process, cloud privacy/provenance, product truth/repo hygiene.

- [ ] **Step 3: Update PR evidence without rewriting Round 4 history**

Keep candidate `9ea73849666720ca59ee3b0d8279b2a53492be3d` and run `32800503199` as historical evidence. Add a new audit correction note instead of pretending that checkpoint failed.

- [ ] **Step 4: Verify Draft/cost-control remains unchanged**

Expected: docs-only change must not allocate the ordinary Windows build runner.

---

### Task 2: Reconcile `main` before production changes

**Files:**
- Reconcile Git history: `main` → `feature/s01-s04-visual-alignment-v2`
- Preserve: `.github/workflows/windows-msvc-build.yml`
- Remove from final branch if introduced: `.github/workflows/apply-s01-s04-design-patch.yml`
- Test: `tests/ci_cost_control_policy.py`

**Produces:** PR branch that is not behind `main`, with one cost-controlled workflow and no obsolete design-patch workflow.

- [ ] **Step 1: Compare the four behind commits and workflow blobs**

Verify `windows-msvc-build.yml` is content-identical or choose the stricter cost-controlled variant.

- [ ] **Step 2: Reconcile history without resurrecting obsolete workflow behavior**

The resulting tree must contain only intended validation workflows.

- [ ] **Step 3: Run the CI policy guard locally/source-side**

Run:
```cmd
python tests\ci_cost_control_policy.py
```
Expected: `CI cost-control policy: PASS`.

---

### Task 3: Durable inspection identity and persistent state root

**Files:**
- Modify: `include/lap/model.h` or create focused `include/lap/inspection_session.h`
- Modify/create matching source file
- Modify: `src/main.cpp`
- Modify: `include/lap/journal.h`, `src/journal.cpp`
- Modify: `src/report.cpp`
- Test: compiled behavioral test for identity continuity

**Interfaces:**
- Produces: immutable `inspectionId` created before evidence collection.
- Consumed by: journal, report publication, history and inventory-only mode.

- [ ] **Step 1: Write failing behavioral tests**

Test that one started inspection has one non-empty ID before stress starts and that the same ID is used for HTML/JSON/journal/history naming.

- [ ] **Step 2: Run the focused test and confirm RED**

Expected failure: current ID is stress-coupled or empty before stress/inventory-only publication.

- [ ] **Step 3: Implement a stable GUID-based inspection identity**

Use Windows GUID APIs or an equivalent collision-resistant local generator. Do not derive identity from timestamps or hardware identifiers.

- [ ] **Step 4: Pass an explicit state/output root into journal operations**

Do not derive crash-recovery truth solely from `<appDir>/reports`.

- [ ] **Step 5: Re-run focused and source regression tests**

Expected: identity continuity PASS; no existing evidence semantics regress.

---

### Task 4: Separate hardware decision from transactional publication

**Files:**
- Modify: `include/lap/report.h`
- Modify: `src/report.cpp`
- Modify: `src/main.cpp`
- Modify: `include/lap/session_history.h`, `src/session_history.cpp` as required
- Test: new compiled publication transaction/failure-injection tests

**Interfaces:**
- Produces: `ReportPublicationResult` with explicit publication status and HTML/JSON paths.
- Constraint: publication failure must not mutate `AuditDecision::overall`.

- [ ] **Step 1: Write failing tests for partial write failure**

Cases: HTML stage succeeds/JSON stage fails; JSON succeeds/history commit fails; final rename fails.

Expected in every case: no published pair is advertised and hardware decision is unchanged.

- [ ] **Step 2: Confirm current implementation is RED**

Current sequential `SaveHtmlReport()` → `SaveJsonReport()` → history behavior should fail at least one transaction assertion.

- [ ] **Step 3: Implement staged pair publication**

Render/write both artifacts to temporary/staging paths, flush/close successfully, then publish final files in a bounded operation. Remove stale staging files on failure.

- [ ] **Step 4: Replace `MarkReportPersistenceIncomplete()` decision mutation**

Represent I/O failure as publication status/coverage of publication, not as a rewritten hardware verdict.

- [ ] **Step 5: Re-run publication tests and full source regression**

Expected: all failure-injection cases PASS and no clean report is exposed from a partial transaction.

---

### Task 5: Transactional and bounded session history

**Files:**
- Modify: `include/lap/session_history.h`
- Modify: `src/session_history.cpp`
- Test: compiled history transaction tests

**Interfaces:**
- Produces: load/mutate/save semantics that commit live memory only after durable index persistence.

- [ ] **Step 1: Write failing rollback tests**

Inject index-write/rename failure and assert `GetSessionHistorySnapshot()` remains equal to the pre-mutation snapshot.

- [ ] **Step 2: Write parser-bound tests**

Cover oversized index, oversized line/field, malformed schema/version and invalid status/verdict values.

- [ ] **Step 3: Implement candidate-snapshot commit**

Copy `gHistory` to a candidate vector, mutate the candidate, persist candidate atomically, then swap into `gHistory` only after success.

- [ ] **Step 4: Harden delete semantics**

Validate all artifact paths first. Do not erase the live entry until the persisted history transition has succeeded; preserve recoverable state if artifact deletion fails.

- [ ] **Step 5: Re-run focused tests**

Expected: rollback, bounds and safe-delete tests PASS.

---

### Task 6: External-engine and elevated process hardening

**Files:**
- Modify: `src/trust.cpp`
- Modify: `src/process.cpp`
- Modify: `tests/trust_security_tests.cpp`
- Modify: `tests/process_security_tests.cpp`
- Update: `SECURITY.md` only if implementation contract changes

**Interfaces:**
- `VerifyEngine()` rejects duplicate logical names and documented redirection/reparse cases.
- `RunProcessCaptureExecutable()` limits inherited handles to intended stdio handles.

- [ ] **Step 1: Add failing duplicate-manifest test**

A manifest with two case-insensitive entries for one logical engine must be rejected as ambiguous even if one hash matches.

- [ ] **Step 2: Add failing reparse/redirection tests where supported by the test environment**

Candidate engine paths that traverse a reparse point outside the trusted tree must remain untrusted.

- [ ] **Step 3: Implement exact-one allowlist match**

Parse all relevant entries; trust only when exactly one valid logical entry exists and its SHA-256 matches.

- [ ] **Step 4: Restrict handle inheritance**

Use `STARTUPINFOEX`/`PROC_THREAD_ATTRIBUTE_HANDLE_LIST` or an equivalently strict Windows mechanism so the child receives only required pipe/std handles.

- [ ] **Step 5: Run compiled trust/process tests**

Expected: all legacy and new adversarial cases PASS.

---

### Task 7: Cloud factory trust and privacy boundary

**Files:**
- Modify: `src/cloud_lookup.cpp`
- Modify: `src/main.cpp` or focused configuration/policy module
- Modify: `src/profile.cpp`
- Test: cloud/profile parsing and trust-policy tests
- Update: relevant docs/security/privacy wording

**Interfaces:**
- Unverified cloud/cache profile must not set `factoryExact=true`.
- Network request uses encoded parameters and bounded response input.

- [ ] **Step 1: Add failing policy tests**

Cases: response omits service tag; mismatched tag; oversized response; malformed schema; unsigned/unverified cache.

Expected: none may become exact factory truth.

- [ ] **Step 2: Implement URL encoding and response byte cap**

Reject response beyond a documented maximum before parsing.

- [ ] **Step 3: Enforce exact returned identity matching**

Never fill a missing remote service tag with the requested value and then treat that as proof.

- [ ] **Step 4: Make cloud lookup opt-in/disabled by default until provenance exists**

If there is no existing persisted consent mechanism, default to no network lookup and keep local evidence explicit.

- [ ] **Step 5: Re-run cloud/profile tests**

Expected: advisory data remains advisory; no unverified exact claim.

---

### Task 8: Product truth, repo privacy and build hygiene

**Files:**
- Modify: `src/ui_screens_s16_s21_v2.cpp`
- Modify: `src/main.cpp`
- Modify: `.gitignore`
- Modify: `build.cmd`
- Create if useful: `sync_release.cmd` or `tools/download_verified_release.ps1`
- Modify/anonymize committed real-device fixture(s)
- Tests: S20 behavioral/contract tests, build-script policy test

- [ ] **Step 1: Add failing S20 semantics tests**

Provider query failure must show unavailable for all event counters; ordinary runtime logs default to `Info`, not `Good`.

- [ ] **Step 2: Remove unsupported user-visible claims**

Replace “100%” hardware identification and digital-signature/export claims that have no active backend.

- [ ] **Step 3: Split build from release sync**

`build.cmd` must fail clearly when local build prerequisites are absent; it must not silently download a release and report build success.

- [ ] **Step 4: Protect generated/private artifacts**

Ignore reports, journals, session history and other generated hardware-identifying outputs.

- [ ] **Step 5: Anonymize committed physical identifiers**

Use synthetic fixture identities unless a real identifier is intentionally required and approved for publication.

- [ ] **Step 6: Run focused tests**

Expected: product-truth and repo-hygiene policy gates PASS.

---

### Task 9: Legacy renderer cleanup and final local verification

**Files:**
- Modify: `CMakeLists.txt`
- Remove/retire legacy source routing only after verifying canonical v2 symbols are complete
- Test: full source suite + CTest locally where Windows toolchain is available

- [ ] **Step 1: Prove no runtime symbol still depends on legacy renderer objects**

Use link/build evidence, not only substring assertions.

- [ ] **Step 2: Remove obsolete production compilation if safe**

Keep test fixtures only where they provide independent value.

- [ ] **Step 3: Run full local verification**

```cmd
run_source_tests.cmd
```

```powershell
cmake --preset msvc-x64-ci
cmake --build --preset build-msvc-x64-ci
ctest --test-dir out/build/msvc-x64-ci -C Release --output-on-failure
```

Expected: all required tests PASS with `/W4 /WX`.

---

### Task 10: One final remote checkpoint and formal pilot handoff

**Files:**
- No speculative production changes after this gate.
- Update: `docs/PRODUCTION_HARDENING_STATUS.md`, PR #2 evidence and validation records.

- [ ] **Step 1: Dispatch exactly one meaningful strict Windows checkpoint**

Only after Tasks 2–9 are complete and locally green.

- [ ] **Step 2: Verify exact source candidate and artifact provenance**

Record commit SHA, workflow/run/job IDs, package hash and test conclusions.

- [ ] **Step 3: Mark Round 5 code candidate pilot-ready, not production-ready**

Keep PR Draft until physical acceptance evidence is complete.

- [ ] **Step 4: Run formal Precision pilot**

Use `validation/PILOT_RUNBOOK.md`, `validation/VALIDATION_CHECKLIST.md`, session record and discrepancy log. HTML/JSON/inspection identity and required functional/port evidence must agree.

- [ ] **Step 5: Only then reassess Ready for Review/merge**

Any pilot-driven production source change requires another strict candidate checkpoint before merge.
