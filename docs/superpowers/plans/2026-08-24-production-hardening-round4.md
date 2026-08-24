# LapSure Production Hardening Round 4 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate the remaining interaction, elevated-process, trust, persistence/recovery and report-publication release blockers in PR #2 without weakening evidence semantics.

**Architecture:** Keep the existing provider -> evidence -> decision/orchestrator -> presentation flow. Add explicit operation boundaries rather than overloading UI focus state, process command strings or hardware decisions with publication state. Security-sensitive executable launches use explicit canonical paths; persisted session artifacts are transactional and untrusted when reopened.

**Tech Stack:** C++20, Win32, MSVC x64, CMake/CTest, Python source-sanity tests, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-08-24-production-hardening-round4-design.md`

## Global Constraints
- Evidence before verdict.
- Missing/malformed/timed-out/permission-denied/stale/contradictory/untrusted/unsupported evidence never becomes PASS.
- Required coverage incomplete => no clean BUY/PASS.
- Native C++20/Win32 only; Windows and WinPE graceful degradation.
- No slow provider work in paint/layout.
- External engines remain disabled when allowlist hashes are empty.

---

### Task 1: Screen-aware action dispatch and S11 RAM CTA

**Files:**
- Modify: `src/main.cpp`
- Modify: `src/ui_screens_s10_s15_v2.cpp`
- Test: `tests/production_hardening_keyboard_dispatch_sanity.py`
- Create: `tests/s11_primary_action_sanity.py`
- Modify: `run_source_tests.cmd`

**Interfaces:**
- Consumes: `MainTab`, `StartAudit(HWND)`, existing `WM_COMMAND` manual workflow IDs.
- Produces: a single current-screen action dispatch path and a real S11 primary action.

- [ ] **Step 1: Write failing tests**
  - Assert the WM_KEYDOWN block does not contain a global `if (gFocusIndex == 2) StartAudit` route.
  - Assert `MainTab::Memory` is routed to a concrete action.
  - Assert S11 visible primary CTA has a matching hit-test/keyboard route.
- [ ] **Step 2: Verify RED**
  - Run `python tests/production_hardening_keyboard_dispatch_sanity.py` and `python tests/s11_primary_action_sanity.py`.
  - Expected: fail on current global focus-2 audit route and missing S11 action.
- [ ] **Step 3: Implement minimal dispatch fix**
  - Remove the global focus-2 `StartAudit` behavior.
  - Resolve action from `gCurrentTab` first.
  - For S11: when no RAM stress evidence exists, navigate/start the supported stress audit path; when evidence exists, navigate to `MainTab::Stress`/evidence without mutating results.
- [ ] **Step 4: Verify GREEN**
  - Run both focused tests and the full source suite.
- [ ] **Step 5: Commit**
  - `fix(ui): make primary actions screen-aware`

### Task 2: Explicit executable process API

**Files:**
- Modify: `include/lap/process.h`
- Modify: `src/process.cpp`
- Modify: process call sites in `src/main.cpp`, `src/engines.cpp`, `src/stress.cpp`, `src/sensors.cpp`, `src/forensics.cpp`, `src/hardware.cpp`, `src/cloud_lookup.cpp` as discovered.
- Create: `tests/process_security_tests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces:
  - `std::wstring QuoteWindowsArgument(const std::wstring&)`
  - `ProcessResult RunProcessCaptureExecutable(const std::wstring& executablePath, const std::vector<std::wstring>& args, unsigned timeoutMs, const std::atomic_bool* cancel)`
  - legacy `RunProcessCapture(commandLine,...)` may remain only for non-security-sensitive compatibility until all call sites are migrated.

- [ ] **Step 1: Write failing process tests**
  - Verify quoting for empty args, spaces, embedded quotes and trailing backslashes.
  - Verify a test executable whose path contains spaces launches through explicit `lpApplicationName`.
- [ ] **Step 2: Verify RED**
  - Configure/build the focused test target; expected failure because the API is absent.
- [ ] **Step 3: Implement explicit API**
  - Build the command line from quoted args while passing `executablePath.c_str()` as `CreateProcessW` `lpApplicationName`.
  - Preserve timeout, cancellation, pipe draining and job-object semantics.
- [ ] **Step 4: Migrate security-sensitive calls**
  - Bundled engines, sensor bridge and system PowerShell use explicit executable paths.
- [ ] **Step 5: Verify GREEN and commit**
  - `security: use explicit executable paths for process launch`

### Task 3: PowerShell canonical system path

**Files:**
- Modify: `include/lap/environment.h`
- Modify: `src/environment.cpp`
- Modify: PowerShell call sites.
- Extend: `tests/process_security_tests.cpp` or source sanity test.

**Interfaces:**
- Produces: `std::wstring ResolveSystemPowerShellPath()` returning an existing canonical `%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe` path or empty.

- [ ] **Step 1: Write failing test/source invariant**
  - No privileged diagnostic call may execute literal `powershell.exe` via command-line resolution.
- [ ] **Step 2: Verify RED**
- [ ] **Step 3: Implement resolver and migrate calls**
- [ ] **Step 4: Verify GREEN**
- [ ] **Step 5: Commit**
  - `security: pin system powershell executable path`

### Task 4: Engine trust contract completion

**Files:**
- Modify: `include/lap/trust.h`
- Modify: `src/trust.cpp`
- Modify: `src/engines.cpp`, `src/stress.cpp`, `src/sensors.cpp`, `src/environment.cpp`
- Extend: `tests/trust_security_tests.cpp`

**Interfaces:**
- `EngineTrust` gains `canonicalPath`.
- `VerifyEngine` requires exactly one logical manifest entry and a regular non-reparse engine path within the application root.

- [ ] **Step 1: Write failing tests**
  - duplicate logical entries rejected;
  - malformed SHA rejected;
  - traversal/absolute paths rejected;
  - directory/reparse case rejected where test environment supports it;
  - returned canonical path is the verified executable.
- [ ] **Step 2: Verify RED**
- [ ] **Step 3: Implement fail-closed verification**
- [ ] **Step 4: Reverify immediately before each external-engine launch and execute `canonicalPath`**
- [ ] **Step 5: Verify GREEN and commit**
  - `security: complete external engine trust boundary`

### Task 5: Durable inspection identity and recovery root

**Files:**
- Modify: `include/lap/model.h`
- Modify: `include/lap/journal.h`
- Modify: `src/journal.cpp`
- Modify: `src/main.cpp`
- Modify: `src/stress.cpp`
- Modify: `src/report.cpp`
- Extend: `tests/behavioral_tests.cpp`
- Extend: `tests/s22_s23_persistence_recovery_sanity.py`

**Interfaces:**
- `AuditReport::sessionId` is created at inspection start.
- Journal APIs take a persistent state root rather than deriving `appDir/reports` internally.

- [ ] **Step 1: Write failing tests**
  - report stems share the inspection id before stress;
  - journal path follows supplied persistent root;
  - journal write failure is surfaced rather than ignored.
- [ ] **Step 2: Verify RED**
- [ ] **Step 3: Add GUID session id at StartAudit/new inspection**
- [ ] **Step 4: Route journal to persistent output/state root and propagate failures as explicit evidence/publication limitation**
- [ ] **Step 5: Verify GREEN and commit**
  - `reliability: make session recovery state durable`

### Task 6: Transactional session history

**Files:**
- Modify: `include/lap/session_history.h`
- Modify: `src/session_history.cpp`
- Extend: `tests/behavioral_tests.cpp`

**Interfaces:**
- Loaded rows are size-capped and schema-validated.
- Mutation uses copy -> atomic index write -> swap; live state does not change when persistence fails.

- [ ] **Step 1: Write failing tests**
  - invalid status/verdict is rejected or marked untrusted;
  - oversized index/field is bounded;
  - failed index write leaves in-memory snapshot unchanged.
- [ ] **Step 2: Verify RED**
- [ ] **Step 3: Implement validated parser and transactional mutation**
- [ ] **Step 4: Verify GREEN and commit**
  - `reliability: validate and transact session history`

### Task 7: Report publication transaction

**Files:**
- Modify: `include/lap/report.h`
- Modify: `src/report.cpp`
- Modify: `src/main.cpp`
- Modify: `include/lap/model.h` only if a publication status structure is needed.
- Extend: `tests/behavioral_tests.cpp`

**Interfaces:**
- One bundle save operation stages HTML + JSON under one session id and commits final artifacts only when both writes succeed.
- Hardware `AuditDecision` remains unchanged by disk I/O; export/UI checks publication readiness separately.

- [ ] **Step 1: Write failing bundle tests**
  - same stem for HTML/JSON;
  - partial write does not publish a trusted final pair;
  - persistence failure does not rewrite a BUY hardware decision.
- [ ] **Step 2: Verify RED**
- [ ] **Step 3: Implement staged bundle save and publication status**
- [ ] **Step 4: Update S18/S19/current-report gates to use publication readiness**
- [ ] **Step 5: Verify GREEN and commit**
  - `reliability: publish report bundles transactionally`

### Task 8: S20 evidence-state cleanup

**Files:**
- Modify: `src/ui_screens_s16_s21_v2.cpp`
- Create/extend: `tests/s16_s21_evidence_sanity.py`

**Interfaces:**
- Default runtime logs map to informational state.
- Event counts render unavailable when the provider query did not succeed.

- [ ] **Step 1: Write failing source/behavior tests**
- [ ] **Step 2: Verify RED**
- [ ] **Step 3: Implement semantic mapping**
- [ ] **Step 4: Verify GREEN and commit**
  - `fix(ui): keep unavailable event evidence explicit`

### Task 9: Release hygiene and legacy renderer removal

**Files:**
- Modify: `CMakeLists.txt`
- Remove obsolete legacy production routing after canonical renderer linkage is verified.
- Reconcile `.github/workflows` with `main` and ensure obsolete `apply-s01-s04-design-patch.yml` is deleted.
- Modify: `SECURITY.md` only if implementation wording changes.

- [ ] **Step 1: Add source invariant that production target does not compile legacy S01-S23 fallback symbols and only approved workflow files remain**
- [ ] **Step 2: Verify RED**
- [ ] **Step 3: Remove legacy renderer production compilation/routing and obsolete workflow**
- [ ] **Step 4: Verify GREEN and commit**
  - `chore(release): remove legacy ui and one-shot workflow`

### Task 10: Final verification

- [ ] Run `run_source_tests.cmd`.
- [ ] Configure strict MSVC x64 Release with warnings as errors.
- [ ] Build `LapSure` and all tests.
- [ ] Run CTest behavioral/trust/process suites.
- [ ] Run inventory-only provider preflight.
- [ ] Package portable distribution.
- [ ] Verify portable package integrity.
- [ ] Confirm PR #2 is mergeable and final head CI is green.
- [ ] Do not claim production-ready until the real-machine matrix has actual completed pilot evidence.
