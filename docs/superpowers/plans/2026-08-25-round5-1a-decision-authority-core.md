# Round 5.1A — Decision Authority Core Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the mutable-string/profile-status acceptance path with a typed, capability-aware, session-evidence decision core that can legitimately reach `BUY WITH NOTES` for complete advisory-chassis sessions without weakening fail-closed provenance.

**Architecture:** Add a focused decision layer beside `AuditReport`: normalize capability truth, resolve reusable authority, preserve session port attestation separately from chassis metadata, freeze one immutable/versioned `RequirementSnapshot`, then feed the same `DecisionContext` to coverage and scoring. Production call sites move from macro substitution to explicit safe loaders; reports persist the frozen decision/coverage metadata instead of recomputing requiredness later.

**Tech Stack:** C++20, Win32 x64, CMake/CTest, MSVC `/W4 /WX`, existing LapSure native providers and transactional report/session-history pipeline.

**Spec:** `docs/superpowers/specs/2026-08-25-decision-authority-happy-path-closure-design.md`

## Global Constraints

- Review baseline: `df3ab209c4afba21ac42ed7bbbb2dfcb615419b6`.
- Run #592 / `d7944104b90f6290f8f444de572f06a90d16a676` remains historical runtime/security/package evidence only.
- PR #2 stays Draft during 5.1A.
- C++ remains C++20; strict Windows evidence remains MSVC x64 `/W4 /WX`.
- No third-party GPU/thermal binary is enabled in 5.1A. Issue #8 owns trusted provider execution.
- `Unknown` never equals `AbsentConfirmed`.
- Trusted critical machine/seller-claim failures may produce `REJECT`; LapSure self-integrity/runtime-validation/provider-trust failures produce `INCOMPLETE`/invalid evidence, not laptop `REJECT`.
- Mutable portable factory/chassis/profile data remains advisory. No raw parser, UI action, report/history input, or mutable file may mint production `Certified` authority.
- Expected chassis ports and observed/session-attested ports remain distinct; correction cannot silently shrink the required denominator.
- Coverage and scoring consume exactly one immutable `RequirementSnapshot`.
- Policy versions are exact: `decision=5.1.0`, `coverage=5.1.0`, `authority=5.1.0`.
- Every behavior change follows TDD RED → verify RED → minimal GREEN → focused GREEN → affected regressions → commit.
- 5.1B provider trust and 5.1C physical/release work are out of scope.

---

## File Structure

### New files
- `include/lap/decision_context.h` — authority, normalized-capability, requirement, and immutable decision-context contracts.
- `src/decision_context.cpp` — portable authority resolution and context construction.
- `include/lap/decision_policy.h` — capability normalization and requirement-policy APIs.
- `src/decision_policy.cpp` — dGPU truth normalization and `RequirementSnapshot` construction.
- `include/lap/port_attestation.h` — session-local physical-port evidence APIs.
- `src/port_attestation.cpp` — expected-vs-observed attestation behavior.
- `tests/decision_authority_tests.cpp` — compiled 5.1A reachability/security regression target.

### Existing files modified
- `include/lap/model.h` — core capability/collection enums, stable guided-port ID, session attestation storage, frozen decision-output metadata.
- `include/lap/scoring.h`, `src/scoring.cpp` — context-consuming coverage/verdict APIs.
- `src/inventory.cpp` — explicit GPU enumeration outcome.
- `include/lap/chassis_profile.h` — raw/advisory loader only after typed resolver migration.
- `include/lap/port_selector.h`, `src/port_selector.cpp` — stable port ID selection.
- `src/app_runtime_state.ipp`, `src/app_audit.ipp` — explicit safe loaders, attestation lifecycle, typed scoring.
- `src/main_round5.cpp` — remove preprocessor trust routing.
- `src/report.cpp` — serialize frozen policy/authority/coverage output.
- `include/lap/session_history.h`, `src/session_history.cpp` — schema v2 policy versions with strict v1 compatibility.
- `tests/behavioral_tests.cpp`, `tests/report_publication_tests.cpp`, `tests/session_history_tests.cpp`, `tests/profile_provenance_security_tests.cpp` — migrated/expanded regressions.
- `CMakeLists.txt` — new production sources and `LapSureDecisionAuthorityTests`.
- source sanity test(s) invoked by `run_source_tests.cmd` — ban macro trust routing and stale mutable authority checks.

---

### Task 1: Establish the Real RED Happy-Path Deadlock

**Files:**
- Create: `tests/decision_authority_tests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes current `BuildAuditDecision(const AuditReport&)` only.
- Produces a compiling failing test that isolates the current `validationStatus != physical-verified` deadlock.

- [ ] **Step 1: Write the healthy advisory fixture**

```cpp
#include "lap/scoring.h"
#include <iostream>

namespace {
int failures = 0;
void Expect(bool ok, const char* name) {
    if (!ok) { std::cerr << "FAIL " << name << '\n'; ++failures; }
    else std::cout << "PASS " << name << '\n';
}

lap::AuditReport HealthyAdvisoryFixture() {
    lap::AuditReport r{};
    r.model = L"Precision test fixture";
    r.serviceTag = L"TEST-SESSION";
    r.hardware.cpuName = L"Test CPU";
    r.hardware.installedRamBytes = 16ULL * 1024 * 1024 * 1024;
    lap::MemoryModule m{}; m.capacityBytes = r.hardware.installedRamBytes;
    r.hardware.memoryModules.push_back(m);
    lap::StorageDevice d{}; d.model=L"Test NVMe"; d.capacityBytes=512ULL*1000*1000*1000; d.reliabilityReadable=true; d.reliabilityHealthy=true;
    r.hardware.storage.push_back(d);
    lap::GpuInfo g{}; g.name=L"Intel Iris Xe Graphics"; r.hardware.gpus.push_back(g);
    lap::DisplayInfo display{}; display.friendlyName=L"Internal panel"; r.hardware.displays.push_back(display);

    lap::StressStageResult cpu{}; cpu.name=L"CPU sustained load"; cpu.verdict=lap::TestVerdict::Pass; cpu.telemetrySummary.maxCpuPackageTempC=80;
    lap::StressStageResult ram{}; ram.name=L"RAM online integrity"; ram.verdict=lap::TestVerdict::Pass;
    r.hardware.stress.stages={cpu,ram};

    for (const auto* id : {L"physical_chassis",L"physical_hinge",L"physical_tamper",L"physical_liquid",L"physical_battery",L"physical_charger"})
        r.hardware.stress.functional.items.push_back({id,L"Physical",lap::FunctionalStatus::Pass,L"Passed",L"Operator stimulus",lap::Confidence::High,false});
    r.hardware.stress.functional.overall=L"PASS";
    r.hardware.stress.portPower.overall=L"PASS";
    r.hardware.stress.runtimeValidation.overall=L"PASS";
    r.hardware.stress.chassisProfile.profileId=L"fixture";
    r.hardware.stress.chassisProfile.validationStatus=L"draft";
    r.hardware.stress.chassisProfile.ports.push_back({L"tb4-left-1",L"TB4 left 1",L"Left",L"USB-C",L"TB4",true,true,L"PASS"});
    return r;
}
}

int main() {
    auto r=HealthyAdvisoryFixture();
    const auto d=lap::BuildAuditDecision(r);
    Expect(d.overall==L"BUY WITH NOTES", "healthy advisory chassis is reachable as BUY WITH NOTES");
    return failures==0?0:1;
}
```

- [ ] **Step 2: Register `LapSureDecisionAuthorityTests` using only current scoring/chassis dependencies**

Do not add 5.1A production sources yet.

- [ ] **Step 3: Verify RED**

```powershell
cmake --preset msvc-x64-ci
cmake --build --preset build-msvc-x64-ci --target LapSureDecisionAuthorityTests
ctest --test-dir out/build/msvc-x64-ci -C Release -R LapSureDecisionAuthorityTests --output-on-failure
```

Expected: build succeeds and the test fails because the current scorer returns `INCOMPLETE` specifically due to non-`physical-verified` chassis authority. If another coverage gap causes the failure, correct the fixture until this is the observed reason.

- [ ] **Step 4: Commit RED evidence**

```bash
git add CMakeLists.txt tests/decision_authority_tests.cpp
git commit -m "test: expose Round 5.1 decision deadlock"
```

---

### Task 2: Introduce Core Truth, Attestation, and Authority Types

**Files:**
- Modify: `include/lap/model.h`
- Create: `include/lap/decision_context.h`
- Create: `src/decision_context.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/decision_authority_tests.cpp`

**Interfaces:**

Add to `model.h`:

```cpp
enum class CapabilityTruth { Present, AbsentConfirmed, Unknown };
enum class ProviderCollectionStatus { NotRun, Complete, Failed, Unsupported };

struct SessionPortEvidence {
    std::wstring expectedPortId;
    std::wstring label;
    bool expectedRequired{false};
    CapabilityTruth observedPresence{CapabilityTruth::Unknown};
    bool tested{false};
    std::wstring verdict{L"NOT TESTED"};
    std::wstring discrepancy;
    std::wstring correctionReason;
};

struct SessionPortAttestation {
    std::wstring sessionId;
    std::vector<SessionPortEvidence> ports;
    bool operatorConfirmed{false};
    std::wstring confirmedAt;
};
```

Add `ProviderCollectionStatus gpuInventoryStatus{ProviderCollectionStatus::NotRun};` to `HardwareSnapshot`, `std::wstring expectedPortId;` to `PortProbeResult`, and `SessionPortAttestation portAttestation;` to `StressSession`.

`decision_context.h` produces:

```cpp
enum class ChassisAuthorityLevel { None, Advisory, Certified };
enum class FactoryAuthorityLevel { Unknown, Advisory, Authenticated };
enum class RequirementDisposition { Required, NotApplicable, ConditionalBlocked };

struct PolicyVersions {
    std::wstring decision{L"5.1.0"};
    std::wstring coverage{L"5.1.0"};
    std::wstring authority{L"5.1.0"};
};

struct CapabilityObservation {
    CapabilityTruth state{CapabilityTruth::Unknown};
    std::wstring evidence;
};

struct ObservedCapabilities {
    CapabilityObservation discreteGpu;
};

struct RequirementDomain {
    std::wstring id;
    RequirementDisposition disposition{RequirementDisposition::Required};
    std::wstring reason;
};

struct RequirementSnapshot {
    PolicyVersions versions;
    std::wstring mode;
    std::vector<RequirementDomain> domains;
    RequirementDisposition StateOf(std::wstring_view id) const;
    bool IsRequired(std::wstring_view id) const;
};
```

`ChassisAuthorityEvidence` has a private constructor. Public code can create only `None` or `Advisory`; only `DecisionProfileResolver` can mint `Certified`, and production has no certified source in 5.1A.

- [ ] **Step 1: Write RED authority-boundary assertions**

```cpp
#include "lap/decision_context.h"

void TestAuthorityBoundary() {
    lap::ChassisProfile raw{};
    raw.profileId=L"mutable-profile";
    raw.validationStatus=L"physical-verified";
    const auto resolved=lap::DecisionProfileResolver::ResolvePortable(raw,false,L"disk");
    Expect(resolved.chassisAuthority.Level()==lap::ChassisAuthorityLevel::Advisory,
           "mutable portable chassis cannot mint Certified authority");
#ifdef LAPSURE_ENABLE_TEST_HOOKS
    const auto certified=lap::DecisionProfileResolver::CertifiedForTest(raw,L"test-only protected authority");
    Expect(certified.chassisAuthority.Level()==lap::ChassisAuthorityLevel::Certified,
           "test-only fixture models Certified path");
#endif
}
```

- [ ] **Step 2: Verify RED**

Expected: compile failure only because the new symbols are absent.

- [ ] **Step 3: Implement minimum contracts**

`DecisionProfileResolver::ResolvePortable()` returns:

```text
profileId empty                         -> None
any mutable/static/heuristic profile   -> Advisory
validationStatus="physical-verified"   -> still Advisory
```

`CertifiedForTest()` exists only under `LAPSURE_ENABLE_TEST_HOOKS`. Add that compile definition to `LapSureDecisionAuthorityTests`; do not add it to `LapSure`.

- [ ] **Step 4: Run focused target**

Authority assertions pass; Task 1 happy-path remains RED.

- [ ] **Step 5: Commit**

```bash
git add include/lap/model.h include/lap/decision_context.h src/decision_context.cpp CMakeLists.txt tests/decision_authority_tests.cpp
git commit -m "feat: add typed decision authority contracts"
```

---

### Task 3: Normalize dGPU Capability Truth and Record Enumeration Outcome

**Files:**
- Create: `include/lap/decision_policy.h`
- Create: `src/decision_policy.cpp`
- Modify: `src/inventory.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/decision_authority_tests.cpp`

**Interfaces:**
- Produce `ObservedCapabilities NormalizeObservedCapabilities(const AuditReport&)`.
- 5.1A applies explicit tri-state requiredness to dGPU first; other conditional domains retain existing conservative coverage until separately migrated.

- [ ] **Step 1: Write RED capability tests**

```cpp
void TestDiscreteGpuTruth() {
    lap::AuditReport r{};
    r.hardware.gpuInventoryStatus=lap::ProviderCollectionStatus::Failed;
    auto c=lap::NormalizeObservedCapabilities(r);
    Expect(c.discreteGpu.state==lap::CapabilityTruth::Unknown,"failed GPU enumeration remains Unknown");

    r.hardware.gpuInventoryStatus=lap::ProviderCollectionStatus::Complete;
    r.hardware.gpus.clear();
    lap::GpuInfo integrated{}; integrated.name=L"Intel Iris Xe Graphics";
    r.hardware.gpus.push_back(integrated);
    c=lap::NormalizeObservedCapabilities(r);
    Expect(c.discreteGpu.state==lap::CapabilityTruth::AbsentConfirmed,"integrated-only successful inventory confirms dGPU absence for this policy");

    lap::GpuInfo discrete{}; discrete.name=L"NVIDIA RTX A2000 Laptop GPU";
    r.hardware.gpus.push_back(discrete);
    c=lap::NormalizeObservedCapabilities(r);
    Expect(c.discreteGpu.state==lap::CapabilityTruth::Present,"trusted inventory recognizes dGPU presence");
}
```

- [ ] **Step 2: Verify RED**

Expected: missing normalization API.

- [ ] **Step 3: Record GPU provider collection status in `CollectInventory`**

For the existing `Win32_VideoController` process:

```cpp
const bool complete=gpu.launched&&!gpu.timedOut&&!gpu.cancelled&&gpu.exitCode==0;
r.hardware.gpuInventoryStatus=complete
    ? ProviderCollectionStatus::Complete
    : ProviderCollectionStatus::Failed;
```

When PowerShell/CIM is unavailable and no query runs, set `Unsupported`.

- [ ] **Step 4: Implement conservative classification**

Rules:

```text
provider state != Complete                              -> Unknown
known dedicated token exists                            -> Present
all returned adapters are known integrated adapters     -> AbsentConfirmed
empty completed result or any ambiguous adapter name    -> Unknown
```

Recognized dedicated tokens for current Precision scope: `NVIDIA`, `RTX`, `Quadro`, `Radeon Pro`, `Radeon RX`, `FirePro`. Recognized integrated tokens: `Intel UHD`, `Intel Iris`, `AMD Radeon(TM) Graphics`, `AMD Radeon Graphics`. `Intel Arc` and unrecognized names remain `Unknown`.

- [ ] **Step 5: Run GREEN**

```powershell
cmake --build --preset build-msvc-x64-ci --target LapSureDecisionAuthorityTests LapSureBehaviorTests
ctest --test-dir out/build/msvc-x64-ci -C Release -R "LapSureDecisionAuthorityTests|LapSureBehaviorTests" --output-on-failure
```

The capability tests pass; Task 1 happy path remains RED.

- [ ] **Step 6: Commit**

```bash
git add include/lap/decision_policy.h src/decision_policy.cpp src/inventory.cpp CMakeLists.txt tests/decision_authority_tests.cpp
git commit -m "feat: normalize discrete GPU capability truth"
```

---

### Task 4: Implement Session Port Attestation and Stable Guided-Port Identity

**Files:**
- Create: `include/lap/port_attestation.h`
- Create: `src/port_attestation.cpp`
- Modify: `include/lap/port_selector.h`
- Modify: `src/port_selector.cpp`
- Modify: `src/app_runtime_state.ipp`
- Modify: `src/app_audit.ipp`
- Modify: `CMakeLists.txt`
- Test: `tests/decision_authority_tests.cpp`

**Interfaces:**

```cpp
SessionPortAttestation InitializeSessionPortAttestation(const std::wstring& sessionId,
                                                         const ChassisProfile& profile);
void ApplyPortResultToAttestation(SessionPortAttestation& attestation,
                                  const PortProbeResult& result);
void RecordPortObservation(SessionPortAttestation& attestation,
                           std::wstring_view expectedPortId,
                           CapabilityTruth observed,
                           std::wstring correctionReason);
unsigned RequiredPortsRemaining(const SessionPortAttestation& attestation);
```

Change selector signature to:

```cpp
bool SelectNextChassisPort(HWND hwnd,
                           const ChassisProfile& profile,
                           std::wstring& portId,
                           std::wstring& label,
                           std::wstring& capability);
```

- [ ] **Step 1: Write anti-edit-away RED tests**

```cpp
void TestPortAttestationCannotShrinkCoverage() {
    lap::ChassisProfile p{};
    p.profileId=L"advisory";
    p.ports.push_back({L"left-tb4-1",L"Left TB4 #1",L"Left",L"USB-C",L"TB4",true,false,L"NOT TESTED"});
    p.ports.push_back({L"left-tb4-2",L"Left TB4 #2",L"Left",L"USB-C",L"TB4",true,false,L"NOT TESTED"});
    auto a=lap::InitializeSessionPortAttestation(L"session-1",p);
    lap::RecordPortObservation(a,L"left-tb4-2",lap::CapabilityTruth::AbsentConfirmed,L"Operator reports port absent");
    Expect(a.ports.size()==2,"operator correction preserves expected denominator");
    Expect(lap::RequiredPortsRemaining(a)==2,"missing expected port remains blocking");
    Expect(!a.ports[1].discrepancy.empty(),"expected port absence records discrepancy");
}
```

Add a positive case where both required IDs receive `PASS` probes and remaining becomes zero.

- [ ] **Step 2: Verify RED**

Expected: missing attestation APIs.

- [ ] **Step 3: Implement attestation logic**

Initialization copies expected IDs/labels/required flags into session evidence. `RecordPortObservation(...AbsentConfirmed...)` records `EXPECTED_PORT_NOT_OBSERVED` for an expected required port and never erases the row. `ApplyPortResultToAttestation()` matches `expectedPortId`, records presence/test/verdict, and rejects label-only ambiguity by leaving unmatched results outside expected coverage.

Set `operatorConfirmed=true` and `confirmedAt` only when every expected required port has an explicit observed state and completed test result; completion of the final guided stimulus is the session confirmation event.

- [ ] **Step 4: Wire stable ID through the guided UI path**

```cpp
std::wstring portId,label,capability;
if (!SelectNextChassisPort(hwnd,snapshot.hardware.stress.chassisProfile,portId,label,capability)) return 0;
auto result=RunPhysicalPortProbe(hwnd,label,&gCancel);
result.expectedPortId=portId;
CommitPortResultGuided(result);
```

- [ ] **Step 5: Initialize attestation when chassis guidance is loaded**

In full audit and inventory-only paths:

```cpp
report.hardware.stress.chassisProfile=LoadDecisionChassisProfile(gDir,report.model);
report.hardware.stress.portAttestation=InitializeSessionPortAttestation(
    report.hardware.stress.sessionId,report.hardware.stress.chassisProfile);
```

- [ ] **Step 6: Update guided commit**

`CommitPortResultGuided()` updates `portPower` and `portAttestation`. Chassis `ports[].tested/verdict` may remain only as a UI compatibility mirror until UI cleanup; acceptance stops reading it in Task 5.

- [ ] **Step 7: Run focused GREEN**

Attestation tests pass; Task 1 happy path remains RED until Task 5.

- [ ] **Step 8: Commit**

```bash
git add include/lap/port_attestation.h src/port_attestation.cpp include/lap/port_selector.h src/port_selector.cpp src/app_runtime_state.ipp src/app_audit.ipp CMakeLists.txt tests/decision_authority_tests.cpp
git commit -m "feat: add session port attestation"
```

---

### Task 5: Freeze `RequirementSnapshot` and Make Scoring/Coverage Share It

**Files:**
- Modify: `include/lap/decision_context.h`
- Modify: `src/decision_context.cpp`
- Modify: `include/lap/decision_policy.h`
- Modify: `src/decision_policy.cpp`
- Modify: `include/lap/model.h`
- Modify: `include/lap/scoring.h`
- Modify: `src/scoring.cpp`
- Test: `tests/decision_authority_tests.cpp`
- Test: `tests/behavioral_tests.cpp`

**Interfaces:**

```cpp
RequirementSnapshot BuildRequirementSnapshot(const AuditReport& report,
                                             const ObservedCapabilities& capabilities,
                                             const SessionPortAttestation& attestation);
DecisionContext BuildDecisionContext(const AuditReport& report);
#ifdef LAPSURE_ENABLE_TEST_HOOKS
DecisionContext BuildCertifiedDecisionContextForTest(const AuditReport& report,
                                                      std::wstring source);
#endif
std::vector<CoverageDomain> BuildCoverageContract(const AuditReport& report,
                                                  const DecisionContext& context);
AuditDecision BuildAuditDecision(const AuditReport& report,
                                 const DecisionContext& context);
```

Add to `AuditDecision`:

```cpp
std::wstring decisionPolicyVersion;
std::wstring coveragePolicyVersion;
std::wstring authorityPolicyVersion;
std::wstring chassisAuthority;
std::wstring factoryAuthority;
std::wstring discreteGpuCapability;
std::vector<CoverageDomain> coverageDomains;
```

Move `CoverageDomain` before `AuditDecision` in `model.h` so the vector has a complete type.

- [ ] **Step 1: Write RED requirement/verdict lattice cases**

Required cases:

```text
healthy + Advisory + complete current-session port attestation -> BUY WITH NOTES
healthy + test-only Certified + complete evidence              -> BUY
expected required port untested                                -> INCOMPLETE
dGPU Present + GPU stage unavailable                           -> INCOMPLETE
dGPU Unknown                                                   -> INCOMPLETE
dGPU AbsentConfirmed + no seller dGPU claim                    -> GPU NotApplicable, no artificial blocker
seller dGPU claim not already disproved                        -> GPU Required
CPU load + no trusted CPU thermal sample                       -> INCOMPLETE
critical trusted seller/hardware finding                       -> REJECT
runtimeValidation.failed > 0                                   -> INCOMPLETE, never REJECT
missing authenticated factory truth alone                      -> does not block otherwise complete machine evidence
```

- [ ] **Step 2: Verify RED and record exact mismatches**

The old scorer must fail at least advisory reachability, requirement sharing, and authority metadata cases.

- [ ] **Step 3: Implement `BuildRequirementSnapshot`**

Requiredness rules:

```text
gpu_vram:
  Present                                  -> Required
  Unknown                                  -> ConditionalBlocked
  AbsentConfirmed + no unresolved claim    -> NotApplicable
  seller dGPU claim, absent no proven critical mismatch -> Required

thermals:
  any CPU sustained-load stage executed    -> Required

ports_power:
  any expected required port exists         -> Required

runtime:
  always Required for purchase-grade decision
```

Existing identity/memory/storage/display/functional domains retain their current conservative requiredness. Factory comparison stays optional for machine-health acceptance.

- [ ] **Step 4: Build `DecisionContext` exactly once per decision rebuild**

`BuildDecisionContext(report)` performs:

1. `NormalizeObservedCapabilities(report)`;
2. `DecisionProfileResolver::ResolvePortable(report.hardware.stress.chassisProfile, report.factoryExact, report.profileSource)`;
3. copy current-session `portAttestation`;
4. build exactly one `RequirementSnapshot`.

Test-only certified context replaces only chassis authority; it does not bypass capability/port/runtime requirements.

- [ ] **Step 5: Refactor coverage**

`BuildCoverageContract(report,context)` keeps current evidence-quality checks but takes requiredness only from `context.requirements`.

For GPU/stability:
- CPU/RAM required stages are evaluated independently from GPU.
- A `GPU / VRAM` NotTested stage does not poison generic stability when `gpu_vram` is `NotApplicable`.
- `ConditionalBlocked` is required/incomplete with an explicit “capability state unknown” reason.
- `Required` GPU coverage is complete only when a contract-valid GPU/VRAM stage completed.

For ports: use `SessionPortAttestation`, not `ChassisProfile.validationStatus` or mutable `ports[].tested`.

For thermal: required CPU thermal coverage needs trusted/valid CPU package telemetry after CPU load.

- [ ] **Step 6: Refactor verdict precedence exactly**

```text
1. critical trusted machine / seller-claim failure -> REJECT
2. LapSure runtime/self-integrity failure           -> INCOMPLETE
3. any required or conditional-blocked gap          -> INCOMPLETE
4. complete evidence + Advisory chassis             -> BUY WITH NOTES
5. complete evidence + Certified chassis            -> BUY
6. non-critical warnings downgrade BUY to BUY WITH NOTES
```

Delete the acceptance check on mutable `validationStatus` and stop using `RequiredPortsRemaining(const ChassisProfile&)` for verdict authority.

- [ ] **Step 7: Freeze decision metadata**

```cpp
d.coverageDomains=BuildCoverageContract(report,context);
d.decisionPolicyVersion=context.requirements.versions.decision;
d.coveragePolicyVersion=context.requirements.versions.coverage;
d.authorityPolicyVersion=context.requirements.versions.authority;
```

Also record string forms of chassis/factory authority and dGPU capability for report serialization.

- [ ] **Step 8: Verify GREEN**

```powershell
cmake --build --preset build-msvc-x64-ci --target LapSureDecisionAuthorityTests LapSureBehaviorTests
ctest --test-dir out/build/msvc-x64-ci -C Release -R "LapSureDecisionAuthorityTests|LapSureBehaviorTests" --output-on-failure
```

Migrate old behavioral tests that set `validationStatus=L"physical-verified"` directly to `BuildCertifiedDecisionContextForTest()`.

- [ ] **Step 9: Commit**

```bash
git add include/lap/model.h include/lap/decision_context.h src/decision_context.cpp include/lap/decision_policy.h src/decision_policy.cpp include/lap/scoring.h src/scoring.cpp tests/decision_authority_tests.cpp tests/behavioral_tests.cpp
git commit -m "feat: make verdicts consume a shared requirement snapshot"
```

---

### Task 6: Remove Macro Trust Routing and Wire Typed Production Decisions

**Files:**
- Modify: `src/app_audit.ipp`
- Modify: `src/app_runtime_state.ipp`
- Modify: `src/main_round5.cpp`
- Modify: `include/lap/chassis_profile.h`
- Modify: `CMakeLists.txt`
- Modify: source sanity test(s)
- Test: `tests/profile_provenance_security_tests.cpp`

**Interfaces:**
- Production loader names are explicit: `LoadDecisionFactoryProfile`, `LookupFactoryProfileForDecision`, `LoadDecisionChassisProfile`.
- Every verdict rebuild follows:

```cpp
const auto context=BuildDecisionContext(report);
report.hardware.stress.decision=BuildAuditDecision(report,context);
```

- [ ] **Step 1: Add RED source-contract checks**

Fail while `main_round5.cpp` contains any of:

```text
#define LoadFactoryProfile
#define LookupFactoryProfileOnline
#define LoadChassisProfile
```

Require explicit decision-safe calls in `app_audit.ipp` and `app_runtime_state.ipp`.

- [ ] **Step 2: Verify source RED**

```cmd
run_source_tests.cmd
```

Expected: only the new macro-removal contract fails.

- [ ] **Step 3: Replace all production raw-name calls with safe APIs**

Do not change technician pre-cache/tooling paths that intentionally parse advisory data.

- [ ] **Step 4: Remove `#define/#undef` trust substitution from `main_round5.cpp`**

Include the `.ipp` fragments directly after explicit call-site migration.

- [ ] **Step 5: Update all decision rebuild points**

Migrate:
- full audit post-stress decision;
- inventory-only decision;
- `RebuildDecisionAndReports()` after manual functional/seller/port evidence.

No no-context `BuildAuditDecision(report)` call remains.

- [ ] **Step 6: Preserve provenance attack regression**

`LapSureProfileProvenanceTests` must prove a mutable chassis file self-declaring `physical-verified` still resolves to `Advisory`.

- [ ] **Step 7: Run source + focused compiled regressions**

```cmd
run_source_tests.cmd
```

```powershell
cmake --build --preset build-msvc-x64-ci --target LapSureDecisionAuthorityTests LapSureProfileProvenanceTests LapSureBehaviorTests
ctest --test-dir out/build/msvc-x64-ci -C Release -R "LapSureDecisionAuthorityTests|LapSureProfileProvenanceTests|LapSureBehaviorTests" --output-on-failure
```

- [ ] **Step 8: Commit**

```bash
git add src/app_audit.ipp src/app_runtime_state.ipp src/main_round5.cpp include/lap/chassis_profile.h CMakeLists.txt tests/profile_provenance_security_tests.cpp tests
git commit -m "refactor: use explicit decision trust boundaries"
```

---

### Task 7: Persist Frozen Policy/Authority/Coverage Metadata

**Files:**
- Modify: `src/report.cpp`
- Modify: `include/lap/session_history.h`
- Modify: `src/session_history.cpp`
- Modify: `tests/report_publication_tests.cpp`
- Modify: `tests/session_history_tests.cpp`

**Interfaces:**
- `AuditDecision.coverageDomains` is the report-time coverage source; `report.cpp` no longer recomputes `BuildCoverageContract(report)`.
- Session history schema v2 appends:
  - `decisionPolicyVersion`
  - `coveragePolicyVersion`
  - `authorityPolicyVersion`

- [ ] **Step 1: Write report RED assertions**

Generated JSON must contain exactly:

```json
"decisionPolicyVersion":"5.1.0",
"coveragePolicyVersion":"5.1.0",
"authorityPolicyVersion":"5.1.0"
```

and serialize frozen chassis/factory/dGPU labels plus frozen coverage domains.

- [ ] **Step 2: Write history migration RED cases**

```text
v2 write/read preserves all three policy versions
valid v1 header + 10-field row loads with policy fields="legacy-v1"
malformed v2 row fails closed
unsupported schema version fails closed
```

- [ ] **Step 3: Verify RED**

```powershell
cmake --build --preset build-msvc-x64-ci --target LapSureReportPublicationTests LapSureSessionHistoryTests
ctest --test-dir out/build/msvc-x64-ci -C Release -R "LapSureReportPublicationTests|LapSureSessionHistoryTests" --output-on-failure
```

- [ ] **Step 4: Serialize the frozen snapshot**

In HTML/JSON technical evidence, use `report.hardware.stress.decision.coverageDomains` and decision metadata. Do not call the policy engine during serialization.

- [ ] **Step 5: Implement strict history schema v2**

Set:

```cpp
constexpr int kHistorySchemaVersion=2;
```

Parsing rules:

```text
header version 1 -> exactly 10 fields; policy values become "legacy-v1"
header version 2 -> exactly 13 fields
other version    -> invalid index
```

Writing always emits v2/13 fields. Preserve existing size bounds, transactional publish behavior, duplicate-session rejection, and fault injection.

- [ ] **Step 6: Verify GREEN**

Publication/history tests and transactional fault tests all pass.

- [ ] **Step 7: Commit**

```bash
git add src/report.cpp include/lap/session_history.h src/session_history.cpp tests/report_publication_tests.cpp tests/session_history_tests.cpp
git commit -m "feat: persist Round 5.1 decision policy metadata"
```

---

### Task 8: Full 5.1A Verification and Bounded Security Closure

**Files:**
- Modify only when verification evidence requires a fix.
- Update Issue #7 and PR #2 metadata after evidence is complete.

**Review range:** `df3ab209c4afba21ac42ed7bbbb2dfcb615419b6..HEAD`

- [ ] **Step 1: Run full source regression**

```cmd
run_source_tests.cmd
```

Expected: all source contracts PASS, including no macro trust routing and no decision-authority comparison against mutable `physical-verified` text.

- [ ] **Step 2: Strict-build the complete project**

```powershell
cmake --preset msvc-x64-ci
cmake --build --preset build-msvc-x64-ci
```

Expected: production and all tests build with `/W4 /WX`, zero warnings/errors.

- [ ] **Step 3: Run every CTest**

```powershell
ctest --test-dir out/build/msvc-x64-ci -C Release --output-on-failure
```

Expected: all historical suites plus `LapSureDecisionAuthorityTests` PASS; report the actual count rather than a hard-coded historical number.

- [ ] **Step 4: Run inventory-only transactional preflight**

Verify:

```text
bundle-* published
HTML + JSON siblings exist
no .staging-* remains
session_history.tsv commits
stress.completed == false
stress.stages.Count == 0
overall == INCOMPLETE
policy versions == 5.1.0 in JSON
```

- [ ] **Step 5: Perform bounded security/diff review**

Explicitly answer:

```text
Can mutable profile/operator/report input mint Certified authority? -> must be NO
Can failed/empty enumeration become AbsentConfirmed?                -> must be NO
Can a proven seller mismatch be hidden by missing provider?         -> must be NO
Can expected required ports be edited out of the denominator?       -> must be NO
Can coverage and scoring use different requiredness?                -> must be NO
Can LapSure self-validation failure become machine REJECT?          -> must be NO
Can any old BuildAuditDecision(report) bypass remain?                -> must be NO
Did 5.1A enable/relax third-party provider trust?                    -> must be NO
```

- [ ] **Step 6: Record exact evidence on Issue #7 and PR #2**

Record final 5.1A head SHA, actual CTest count, source result, strict build result, inventory preflight result, bounded compare file list, and every material discrepancy disposition.

- [ ] **Step 7: Close Issue #7 only with evidence**

Issue #8 remains blocked until 5.1A is compiled/security-green. Do not claim physical acceptance or production readiness at 5.1A closure.

---

## Plan Self-Review Result

- Spec coverage: every 5.1A A+ invariant maps to a task.
- Type consistency: `CapabilityTruth`, provider collection status, and session attestation live in `model.h`; authority/requirement types live in `decision_context.h`; requirement construction occurs only after attestation types exist.
- Authority safety: production has no `Certified` minting source in 5.1A; only the test target gets `LAPSURE_ENABLE_TEST_HOOKS`.
- Requiredness safety: empty/failed GPU enumeration is never absence; one frozen snapshot feeds coverage and scoring.
- Port safety: expected rows are never deleted by observation/correction.
- Persistence safety: report serialization consumes frozen decision metadata; history migration is explicit v1→v2 and fail-closed otherwise.
- Scope safety: no provider binary/trust-root implementation or physical/release closure is included.
- Placeholder scan: no `TODO`, `TBD`, “similar to”, or unspecified implementation placeholder is part of the execution steps.

## Execution Strategy

Use subagent-driven development when an execution host supports fresh workers. In this ChatGPT/GitHub environment, use the equivalent inline `executing-plans` workflow: one task/TDD cycle at a time, fresh verification evidence before each completion claim, bounded review at the tranche checkpoint, and no 5.1B work until Issue #7 is proven closed.
