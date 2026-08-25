# Round 5.1A — Decision Authority Core Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the current mutable-string/profile-status acceptance path with a typed, capability-aware, session-evidence decision core that can legitimately reach `BUY WITH NOTES` for complete advisory-chassis sessions without weakening fail-closed provenance.

**Architecture:** Add a small typed decision layer beside the existing `AuditReport`: normalize observed capability truth, resolve decision authority, freeze an immutable/versioned `RequirementSnapshot`, preserve session-specific port attestation separately from reusable chassis metadata, then feed the same `DecisionContext` to coverage and scoring. Production call sites move from preprocessor substitution to explicit safe loaders and typed decision construction; reports persist the resulting policy/authority/coverage snapshot rather than recomputing requiredness later.

**Tech Stack:** C++20, Win32 x64, CMake/CTest, MSVC `/W4 /WX`, existing LapSure native providers and report/session-history pipeline.

**Spec:** `docs/superpowers/specs/2026-08-25-decision-authority-happy-path-closure-design.md`

## Global Constraints

- Review baseline: `df3ab209c4afba21ac42ed7bbbb2dfcb615419b6`.
- Parent runtime/security/package evidence: Run #592 / `d7944104b90f6290f8f444de572f06a90d16a676`; do not relabel it as the Round 5.1 acceptance candidate.
- PR #2 remains Draft during 5.1A.
- C++ standard remains C++20; Windows CI remains MSVC x64 strict `/W4 /WX`.
- No third-party GPU/thermal binary is enabled in 5.1A. Missing trusted provider evidence remains fail-closed; 5.1B owns provider execution trust.
- `Unknown` never equals `AbsentConfirmed`.
- Critical trusted machine/seller-claim failures may produce `REJECT`; LapSure self-integrity/runtime-validation/provider-trust failure produces `INCOMPLETE`/invalid evidence, not laptop `REJECT`.
- Mutable portable factory/chassis/profile data remains advisory. No raw parser, UI action, persisted report, or mutable file may mint production `Certified` authority.
- Expected chassis ports and observed/session-attested ports remain separate; operator correction must never silently reduce the required denominator.
- Coverage and scoring consume the same immutable `RequirementSnapshot`.
- Persist exact policy versions: `decision=5.1.0`, `coverage=5.1.0`, `authority=5.1.0`.
- Follow TDD for every behavior change: write RED, run and observe the intended failure, implement the minimum GREEN change, rerun the focused test, then run the affected regression set before committing.
- Do not modify 5.1B provider trust implementation or 5.1C physical/release governance in this plan.

---

## File Structure Locked by This Plan

### New files

- `include/lap/decision_context.h` — typed authority/capability/requirement/decision-context contracts.
- `src/decision_context.cpp` — authority resolution and immutable context construction.
- `include/lap/decision_policy.h` — capability normalization and requirement-policy APIs.
- `src/decision_policy.cpp` — `ObservedCapabilities` normalization and `RequirementSnapshot` construction.
- `include/lap/port_attestation.h` — session-local physical-port evidence APIs.
- `src/port_attestation.cpp` — expected-vs-observed port attestation logic.
- `tests/decision_authority_tests.cpp` — compiled 5.1A reachability/security regression target.

### Existing files modified

- `include/lap/model.h` — collection-state marker, stable guided-port ID, session attestation storage, decision-output metadata/coverage snapshot.
- `include/lap/scoring.h` / `src/scoring.cpp` — context-consuming coverage and verdict APIs; remove profile-string authority semantics.
- `src/inventory.cpp` — record whether native GPU inventory completed successfully instead of inferring absence from an empty vector.
- `include/lap/chassis_profile.h` — retain raw advisory parser helpers; remove inline decision authority shortcut once typed resolver is wired.
- `include/lap/port_selector.h` / `src/port_selector.cpp` — return stable chassis port ID together with label/capability.
- `src/app_runtime_state.ipp` — explicit safe profile loaders, initialize/update port attestation, rebuild decision from typed context.
- `src/app_audit.ipp` — explicit safe profile loaders, initialize typed decision evidence, score through `DecisionContext`.
- `src/main_round5.cpp` — remove `#define LoadFactoryProfile/LoadChassisProfile/LookupFactoryProfileOnline` trust routing.
- `src/report.cpp` — serialize/display the already-frozen decision metadata and coverage snapshot rather than recomputing requiredness.
- `include/lap/session_history.h` / `src/session_history.cpp` — schema v2 policy-version fields with strict backward-compatible v1 read support.
- `tests/behavioral_tests.cpp` — migrate legacy direct scoring fixtures to typed context and retain existing product behaviors.
- `tests/session_history_tests.cpp` — schema migration/version round-trip tests.
- `tests/profile_provenance_security_tests.cpp` — assert mutable profile remains advisory under the new resolver.
- `CMakeLists.txt` — add 5.1A sources to production/tests and add `LapSureDecisionAuthorityTests`.
- `run_source_tests.cmd` and/or the relevant source sanity test — assert macro trust routing does not return and the new contracts remain present.

---

### Task 1: Establish a RED Production-Reachability Test

**Files:**
- Create: `tests/decision_authority_tests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes current `lap::AuditReport`, `lap::BuildAuditDecision(const AuditReport&)`, and existing chassis/functional/port/runtime structures.
- Produces the first intentional RED proving the current architecture cannot issue the approved advisory-chassis result.

- [ ] **Step 1: Add the failing healthy-advisory fixture**

Create a small self-contained fixture using only current APIs. The key assertion is deliberately impossible under the current `validationStatus != physical-verified` gate:

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
    lap::StorageDevice d{}; d.model = L"Test NVMe"; d.capacityBytes = 512ULL * 1000 * 1000 * 1000; d.reliabilityReadable = true; d.reliabilityHealthy = true;
    r.hardware.storage.push_back(d);
    lap::GpuInfo g{}; g.name = L"Intel Iris Xe Graphics";
    r.hardware.gpus.push_back(g);
    lap::DisplayInfo display{}; display.friendlyName = L"Internal panel";
    r.hardware.displays.push_back(display);
    r.hardware.battery.present = false;

    lap::StressStageResult cpu{}; cpu.name = L"CPU sustained load"; cpu.verdict = lap::TestVerdict::Pass; cpu.telemetrySummary.maxCpuPackageTempC = 80;
    lap::StressStageResult ram{}; ram.name = L"RAM online integrity"; ram.verdict = lap::TestVerdict::Pass;
    r.hardware.stress.stages = {cpu, ram};

    for (const auto* id : {L"physical_chassis",L"physical_hinge",L"physical_tamper",L"physical_liquid",L"physical_battery",L"physical_charger"})
        r.hardware.stress.functional.items.push_back({id,L"Physical",lap::FunctionalStatus::Pass,L"Passed",L"Operator stimulus",lap::Confidence::High,false});
    r.hardware.stress.functional.overall = L"PASS";
    r.hardware.stress.portPower.overall = L"PASS";
    r.hardware.stress.runtimeValidation.overall = L"PASS";
    r.hardware.stress.chassisProfile.profileId = L"fixture";
    r.hardware.stress.chassisProfile.validationStatus = L"draft";
    r.hardware.stress.chassisProfile.ports.push_back({L"tb4-left-1",L"TB4 left 1",L"Left",L"USB-C",L"TB4",true,true,L"PASS"});
    return r;
}
}

int main() {
    auto r = HealthyAdvisoryFixture();
    const auto d = lap::BuildAuditDecision(r);
    Expect(d.overall == L"BUY WITH NOTES", "healthy advisory chassis is reachable as BUY WITH NOTES");
    return failures == 0 ? 0 : 1;
}
```

- [ ] **Step 2: Register only the RED test target**

Add a `LapSureDecisionAuthorityTests` executable using the current scoring/chassis sources required by the fixture. Do not add new production sources yet.

- [ ] **Step 3: Run the focused test and observe the intended RED**

Run:

```powershell
cmake --preset msvc-x64-ci
cmake --build --preset build-msvc-x64-ci --target LapSureDecisionAuthorityTests
ctest --test-dir out/build/msvc-x64-ci -C Release -R LapSureDecisionAuthorityTests --output-on-failure
```

Expected: build succeeds, test fails specifically because current scoring returns `INCOMPLETE` with the chassis profile not physical-verified. If it fails for missing fixture evidence instead, fix the fixture until the failure is the architectural deadlock.

- [ ] **Step 4: Commit the RED evidence**

```bash
git add CMakeLists.txt tests/decision_authority_tests.cpp
git commit -m "test: expose Round 5.1 decision deadlock"
```

---

### Task 2: Add Typed Authority and Decision Context Contracts

**Files:**
- Create: `include/lap/decision_context.h`
- Create: `src/decision_context.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/decision_authority_tests.cpp`

**Interfaces:**
- Produces:
  - `enum class CapabilityTruth { Present, AbsentConfirmed, Unknown };`
  - `enum class ChassisAuthorityLevel { None, Advisory, Certified };`
  - `enum class FactoryAuthorityLevel { Unknown, Advisory, Authenticated };`
  - `enum class RequirementDisposition { Required, NotApplicable, ConditionalBlocked };`
  - `PolicyVersions`, `CapabilityObservation`, `ObservedCapabilities`, `RequirementDomain`, `RequirementSnapshot`, `DecisionProfileResolution`, `DecisionContext`.
  - `DecisionProfileResolver::ResolvePortable(...)` and test-only `DecisionProfileResolver::CertifiedForTest(...)`.

- [ ] **Step 1: Extend the test with authority-boundary assertions before implementation**

Add:

```cpp
#include "lap/decision_context.h"

void TestAuthorityBoundary() {
    lap::ChassisProfile raw{};
    raw.profileId = L"mutable-profile";
    raw.validationStatus = L"physical-verified";
    const auto resolution = lap::DecisionProfileResolver::ResolvePortable(raw, false, L"disk");
    Expect(resolution.chassisAuthority.Level() == lap::ChassisAuthorityLevel::Advisory,
           "mutable portable chassis cannot mint Certified authority");
#ifdef LAPSURE_ENABLE_TEST_HOOKS
    const auto certified = lap::DecisionProfileResolver::CertifiedForTest(raw, L"test-only protected authority");
    Expect(certified.chassisAuthority.Level() == lap::ChassisAuthorityLevel::Certified,
           "test-only authority fixture can model Certified path");
#endif
}
```

- [ ] **Step 2: Run RED**

Expected: compile fails only because the new typed API does not exist yet.

- [ ] **Step 3: Implement the minimum typed contracts**

Use this public shape in `include/lap/decision_context.h`:

```cpp
#pragma once
#include "model.h"
#include <string>
#include <string_view>
#include <vector>

namespace lap {

enum class CapabilityTruth { Present, AbsentConfirmed, Unknown };
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

class ChassisAuthorityEvidence {
public:
    ChassisAuthorityLevel Level() const noexcept { return level_; }
    const std::wstring& Source() const noexcept { return source_; }
    static ChassisAuthorityEvidence None();
    static ChassisAuthorityEvidence Advisory(std::wstring source);
private:
    ChassisAuthorityEvidence(ChassisAuthorityLevel level, std::wstring source);
    ChassisAuthorityLevel level_{ChassisAuthorityLevel::None};
    std::wstring source_;
    friend class DecisionProfileResolver;
};

struct DecisionProfileResolution {
    ChassisProfile chassis;
    ChassisAuthorityEvidence chassisAuthority{ChassisAuthorityEvidence::None()};
    FactoryAuthorityLevel factoryAuthority{FactoryAuthorityLevel::Unknown};
    std::wstring factorySource;
};

struct DecisionContext;

class DecisionProfileResolver {
public:
    static DecisionProfileResolution ResolvePortable(const ChassisProfile& chassis,
                                                       bool factoryAuthenticated,
                                                       std::wstring factorySource);
#ifdef LAPSURE_ENABLE_TEST_HOOKS
    static DecisionProfileResolution CertifiedForTest(const ChassisProfile& chassis,
                                                       std::wstring source);
#endif
};

} // namespace lap
```

Implementation rule: `ResolvePortable()` returns `None` when no profile ID exists and at most `Advisory` when any mutable profile exists, regardless of `validationStatus`. `CertifiedForTest()` exists only behind `LAPSURE_ENABLE_TEST_HOOKS` and constructs the private `Certified` value.

- [ ] **Step 4: Build and run the focused authority tests**

The new authority assertions must pass while Task 1’s happy-path assertion remains RED until scoring is migrated.

- [ ] **Step 5: Commit**

```bash
git add include/lap/decision_context.h src/decision_context.cpp CMakeLists.txt tests/decision_authority_tests.cpp
git commit -m "feat: add typed decision authority contracts"
```

---

### Task 3: Normalize Capability Truth and Freeze `RequirementSnapshot`

**Files:**
- Create: `include/lap/decision_policy.h`
- Create: `src/decision_policy.cpp`
- Modify: `include/lap/model.h`
- Modify: `src/inventory.cpp`
- Modify: `CMakeLists.txt`
- Test: `tests/decision_authority_tests.cpp`

**Interfaces:**
- Add `enum class ProviderCollectionStatus { NotRun, Complete, Failed, Unsupported };` to `model.h`.
- Add `ProviderCollectionStatus gpuInventoryStatus{ProviderCollectionStatus::NotRun};` to `HardwareSnapshot`.
- Produce `ObservedCapabilities NormalizeObservedCapabilities(const AuditReport&)`.
- Produce `RequirementSnapshot BuildRequirementSnapshot(const AuditReport&, const ObservedCapabilities&, const SessionPortAttestation&)` after Task 4 provides `SessionPortAttestation`; until then use a forward declaration and a zero-port test fixture.

- [ ] **Step 1: Write RED capability tests**

Add cases that explicitly distinguish unknown from absence:

```cpp
void TestDiscreteGpuTruth() {
    lap::AuditReport r{};
    r.hardware.gpuInventoryStatus = lap::ProviderCollectionStatus::Failed;
    auto caps = lap::NormalizeObservedCapabilities(r);
    Expect(caps.discreteGpu.state == lap::CapabilityTruth::Unknown,
           "failed GPU enumeration remains Unknown");

    r.hardware.gpuInventoryStatus = lap::ProviderCollectionStatus::Complete;
    r.hardware.gpus.clear();
    lap::GpuInfo integrated{}; integrated.name = L"Intel Iris Xe Graphics";
    r.hardware.gpus.push_back(integrated);
    caps = lap::NormalizeObservedCapabilities(r);
    Expect(caps.discreteGpu.state == lap::CapabilityTruth::AbsentConfirmed,
           "successful integrated-only inventory confirms no discrete GPU for this policy");

    lap::GpuInfo discrete{}; discrete.name = L"NVIDIA RTX A2000 Laptop GPU";
    r.hardware.gpus.push_back(discrete);
    caps = lap::NormalizeObservedCapabilities(r);
    Expect(caps.discreteGpu.state == lap::CapabilityTruth::Present,
           "trusted inventory recognizes discrete GPU presence");
}
```

The classifier must remain conservative. Recognize dedicated tokens sufficient for current Precision targets (`NVIDIA`, `RTX`, `Quadro`, `Radeon Pro`, `Radeon RX`, `FirePro`). Treat ambiguous adapters as `Unknown`; do not guess absence.

- [ ] **Step 2: Run RED**

Expected: compile/link failure because `ProviderCollectionStatus` and normalization APIs do not yet exist.

- [ ] **Step 3: Record GPU enumeration outcome in `CollectInventory`**

For the existing `Win32_VideoController` process:

```cpp
const bool gpuQueryComplete = gpu.launched && !gpu.timedOut && !gpu.cancelled && gpu.exitCode == 0;
r.hardware.gpuInventoryStatus = gpuQueryComplete
    ? ProviderCollectionStatus::Complete
    : ProviderCollectionStatus::Failed;
```

When PowerShell/CIM is unavailable before the query can be attempted, set `Unsupported`, not `Complete` with an empty list.

- [ ] **Step 4: Implement conservative capability normalization**

`NormalizeObservedCapabilities()` rules:

```text
Provider status != Complete              -> Unknown
Any known dedicated-adapter name         -> Present
All returned adapters known integrated   -> AbsentConfirmed
No adapters or any ambiguous adapter     -> Unknown
```

Use case-insensitive exact token helpers; do not classify generic `Radeon Graphics` or `Intel Arc` as absent.

- [ ] **Step 5: Add RED requirement-policy cases**

Once Task 4’s attestation type is available, assert:

```text
dGPU Present                       -> gpu_vram Required
dGPU Unknown                       -> gpu_vram ConditionalBlocked
dGPU AbsentConfirmed + no claim    -> gpu_vram NotApplicable
seller claim names a dGPU          -> gpu_vram Required unless already rejected by a critical mismatch
CPU load executed                  -> thermals Required
runtime validation                 -> runtime Required
```

- [ ] **Step 6: Implement `BuildRequirementSnapshot` with exact policy versions**

Do not inspect mutable report state later in scoring to re-decide whether GPU/thermal/port/runtime are required. The snapshot is the only requiredness input.

- [ ] **Step 7: Run focused tests and existing behavioral tests**

```powershell
cmake --build --preset build-msvc-x64-ci --target LapSureDecisionAuthorityTests LapSureBehaviorTests
ctest --test-dir out/build/msvc-x64-ci -C Release -R "LapSureDecisionAuthorityTests|LapSureBehaviorTests" --output-on-failure
```

- [ ] **Step 8: Commit**

```bash
git add include/lap/model.h include/lap/decision_policy.h src/decision_policy.cpp src/inventory.cpp CMakeLists.txt tests/decision_authority_tests.cpp
git commit -m "feat: add capability truth and requirement snapshot"
```

---

### Task 4: Add Session Port Attestation Without Edit-Away Coverage

**Files:**
- Create: `include/lap/port_attestation.h`
- Create: `src/port_attestation.cpp`
- Modify: `include/lap/model.h`
- Modify: `include/lap/port_selector.h`
- Modify: `src/port_selector.cpp`
- Modify: `src/app_runtime_state.ipp`
- Modify: `CMakeLists.txt`
- Test: `tests/decision_authority_tests.cpp`

**Interfaces:**
- Add `std::wstring expectedPortId;` to `PortProbeResult`.
- Add model types:

```cpp
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

If `model.h` cannot include the capability enum without a cycle, keep these two structs in `decision_context.h` and add a lightweight serializable mirror to `StressSession`; do not duplicate behavioral authority.

- Produce:
  - `SessionPortAttestation InitializeSessionPortAttestation(const std::wstring&, const ChassisProfile&)`;
  - `void ApplyPortResultToAttestation(SessionPortAttestation&, const PortProbeResult&)`;
  - `void RecordPortObservation(SessionPortAttestation&, std::wstring_view expectedPortId, CapabilityTruth observed, std::wstring correctionReason)`;
  - `unsigned RequiredPortsRemaining(const SessionPortAttestation&)`.

- [ ] **Step 1: Write anti-edit-away RED tests**

```cpp
void TestPortAttestationCannotShrinkCoverage() {
    lap::ChassisProfile p{};
    p.profileId = L"advisory";
    p.ports.push_back({L"left-tb4-1",L"Left TB4 #1",L"Left",L"USB-C",L"TB4",true,false,L"NOT TESTED"});
    p.ports.push_back({L"left-tb4-2",L"Left TB4 #2",L"Left",L"USB-C",L"TB4",true,false,L"NOT TESTED"});
    auto a = lap::InitializeSessionPortAttestation(L"session-1", p);
    lap::RecordPortObservation(a, L"left-tb4-2", lap::CapabilityTruth::AbsentConfirmed, L"Operator reports port not present");
    Expect(a.ports.size() == 2, "operator correction preserves expected denominator");
    Expect(lap::RequiredPortsRemaining(a) == 2, "missing expected port remains blocking until disposition/test policy resolves it");
    Expect(!a.ports[1].discrepancy.empty(), "expected port absence records discrepancy");
}
```

Add a positive test where both required ports receive `PASS` probe results and `RequiredPortsRemaining()` becomes zero.

- [ ] **Step 2: Run RED**

Expected: compile/link failure for the new attestation API.

- [ ] **Step 3: Implement attestation storage and stable guided-port mapping**

Change `SelectNextChassisPort` to:

```cpp
bool SelectNextChassisPort(HWND hwnd,
                           const ChassisProfile& profile,
                           std::wstring& portId,
                           std::wstring& label,
                           std::wstring& capability);
```

In the guided path, set the stable ID on the probe result before commit:

```cpp
std::wstring portId, label, capability;
if (!SelectNextChassisPort(hwnd, snapshot.hardware.stress.chassisProfile, portId, label, capability)) return 0;
auto result = RunPhysicalPortProbe(hwnd, label, &gCancel);
result.expectedPortId = portId;
CommitPortResultGuided(result);
```

- [ ] **Step 4: Initialize attestation at chassis-profile load**

Immediately after the safe chassis profile is loaded for a new inspection:

```cpp
report.hardware.stress.portAttestation = InitializeSessionPortAttestation(
    report.hardware.stress.sessionId,
    report.hardware.stress.chassisProfile);
```

Do the same in inventory-only mode so the report truth is deterministic even though no manual port test is run.

- [ ] **Step 5: Update guided port commit**

`CommitPortResultGuided()` updates both `portPower` and `portAttestation`. Keep the old chassis `tested/verdict` mutation only as a temporary UI compatibility mirror; scoring must stop consuming it in Task 5.

- [ ] **Step 6: Run focused tests**

The anti-edit-away and complete-attestation cases must pass; Task 1 happy-path may remain RED until Task 5.

- [ ] **Step 7: Commit**

```bash
git add include/lap/model.h include/lap/port_attestation.h src/port_attestation.cpp include/lap/port_selector.h src/port_selector.cpp src/app_runtime_state.ipp CMakeLists.txt tests/decision_authority_tests.cpp
git commit -m "feat: add session port attestation"
```

---

### Task 5: Make Coverage and Scoring Consume One `DecisionContext`

**Files:**
- Modify: `include/lap/decision_context.h`
- Modify: `src/decision_context.cpp`
- Modify: `include/lap/scoring.h`
- Modify: `src/scoring.cpp`
- Modify: `include/lap/model.h`
- Test: `tests/decision_authority_tests.cpp`
- Test: `tests/behavioral_tests.cpp`

**Interfaces:**
- Produce:

```cpp
DecisionContext BuildDecisionContext(const AuditReport& report);
#ifdef LAPSURE_ENABLE_TEST_HOOKS
DecisionContext BuildCertifiedDecisionContextForTest(const AuditReport& report, std::wstring source);
#endif

std::vector<CoverageDomain> BuildCoverageContract(const AuditReport& report,
                                                  const DecisionContext& context);
AuditDecision BuildAuditDecision(const AuditReport& report,
                                 const DecisionContext& context);
```

- Remove the old no-context scoring overloads after all call sites are migrated; do not leave an easy bypass.
- Add decision-output metadata to `AuditDecision`:

```cpp
std::wstring decisionPolicyVersion;
std::wstring coveragePolicyVersion;
std::wstring authorityPolicyVersion;
std::wstring chassisAuthority;
std::wstring factoryAuthority;
std::wstring discreteGpuCapability;
std::vector<CoverageDomain> coverageDomains;
```

Move `CoverageDomain` before `AuditDecision` in `model.h` so the vector is a complete type.

- [ ] **Step 1: Add full verdict-lattice RED cases**

Extend `LapSureDecisionAuthorityTests` so the target covers all of these before rewriting scoring:

```text
healthy + Advisory + complete current-session attestation -> BUY WITH NOTES
healthy + test-only Certified + complete evidence         -> BUY
Advisory + one required expected port untested            -> INCOMPLETE
dGPU Unknown                                              -> INCOMPLETE
dGPU Present + GPU evidence unavailable                   -> INCOMPLETE
dGPU AbsentConfirmed + no claim                           -> no artificial GPU blocker
CPU load + trusted thermal sample missing                 -> INCOMPLETE
critical seller/hardware finding                          -> REJECT
runtimeValidation.failed > 0                              -> INCOMPLETE, never REJECT
factory authority unavailable by itself                   -> does not block otherwise complete purchase-safety evidence
```

- [ ] **Step 2: Run RED and capture the exact mismatches**

Expected: the old scorer fails at least the advisory `BUY WITH NOTES`, requirement-sharing, dGPU-requiredness, and authority metadata cases.

- [ ] **Step 3: Build `DecisionContext` once**

`BuildDecisionContext(report)` must:

1. normalize `ObservedCapabilities`;
2. resolve mutable chassis to `Advisory`/`None` only;
3. derive factory authority from the already-safe `report.factoryExact/profileSource` state without making factory authority a purchase-health gate;
4. copy the current-session `portAttestation`;
5. create the immutable `RequirementSnapshot` exactly once.

The test-only builder swaps only the chassis authority evidence to `Certified`; all machine/session requirements still come from the real fixture.

- [ ] **Step 4: Refactor coverage to consume snapshot requiredness**

Keep existing evidence-completeness calculations (identity, memory, storage, functional, runtime, etc.) but set `CoverageDomain.required` and NotApplicable/blocked handling from `context.requirements`.

For `gpu_vram`, explicitly distinguish:

```text
NotApplicable       -> complete/required=false; no blocker
ConditionalBlocked  -> partial/required=true; missingEvidence explains capability is Unknown
Required            -> complete only when required GPU/VRAM evidence is contract-valid
```

For `thermals`, CPU load without trusted CPU package telemetry remains incomplete.

For `ports_power`, completeness must be based on `SessionPortAttestation` plus `PortPowerSummary`, not `ChassisProfile.validationStatus` or mutable `ports[].tested`.

- [ ] **Step 5: Refactor verdict ordering**

Use this exact order:

```text
1. critical trusted machine / seller-claim failure -> REJECT
2. LapSure runtime/self-integrity failure           -> INCOMPLETE
3. any required/conditional-blocked coverage gap    -> INCOMPLETE
4. complete evidence + Advisory chassis             -> BUY WITH NOTES
5. complete evidence + Certified chassis            -> BUY
6. non-critical warnings downgrade BUY to BUY WITH NOTES
```

Remove the old direct check:

```cpp
chassisProfile.validationStatus != L"physical-verified"
```

and stop using `RequiredPortsRemaining(const ChassisProfile&)` for acceptance.

- [ ] **Step 6: Freeze the coverage contract into the decision output**

Set:

```cpp
d.coverageDomains = BuildCoverageContract(report, context);
d.decisionPolicyVersion = context.requirements.versions.decision;
d.coveragePolicyVersion = context.requirements.versions.coverage;
d.authorityPolicyVersion = context.requirements.versions.authority;
```

Also copy string forms of chassis/factory/dGPU state for report serialization. This prevents `report.cpp` from recomputing requiredness after the verdict was made.

- [ ] **Step 7: Run GREEN and migrate behavioral fixtures**

Run:

```powershell
cmake --build --preset build-msvc-x64-ci --target LapSureDecisionAuthorityTests LapSureBehaviorTests
ctest --test-dir out/build/msvc-x64-ci -C Release -R "LapSureDecisionAuthorityTests|LapSureBehaviorTests" --output-on-failure
```

Update old behavioral assertions that directly manufacture `validationStatus=L"physical-verified"` so certified-path testing goes through `BuildCertifiedDecisionContextForTest()` instead.

- [ ] **Step 8: Commit**

```bash
git add include/lap/model.h include/lap/decision_context.h src/decision_context.cpp include/lap/scoring.h src/scoring.cpp tests/decision_authority_tests.cpp tests/behavioral_tests.cpp
git commit -m "feat: make verdicts consume typed decision context"
```

---

### Task 6: Wire Production Explicitly and Remove Macro Trust Routing

**Files:**
- Modify: `src/app_audit.ipp`
- Modify: `src/app_runtime_state.ipp`
- Modify: `src/main_round5.cpp`
- Modify: `include/lap/chassis_profile.h`
- Modify: `CMakeLists.txt`
- Test: `tests/profile_provenance_security_tests.cpp`
- Test: relevant source sanity test invoked by `run_source_tests.cmd`

**Interfaces:**
- Production uses explicit safe functions:
  - `LoadDecisionFactoryProfile(...)`
  - `LookupFactoryProfileForDecision(...)`
  - `LoadDecisionChassisProfile(...)` or the new resolver entry that returns advisory guidance.
- Every verdict call follows:

```cpp
const auto context = BuildDecisionContext(report);
report.hardware.stress.decision = BuildAuditDecision(report, context);
```

- [ ] **Step 1: Add RED source-contract assertions**

The source sanity test must fail while these macros remain:

```text
#define LoadFactoryProfile LoadDecisionFactoryProfile
#define LookupFactoryProfileOnline LookupFactoryProfileForDecision
#define LoadChassisProfile LoadDecisionChassisProfile
```

Also assert `app_audit.ipp` and `app_runtime_state.ipp` contain explicit decision-safe names.

- [ ] **Step 2: Run source suite and observe RED**

```cmd
run_source_tests.cmd
```

Expected: only the new macro-removal contract fails.

- [ ] **Step 3: Replace implicit names in app fragments**

In both inventory-only and full audit paths, replace raw-name calls with the explicit safe APIs. Do not change technician pre-cache/tooling paths that intentionally use advisory raw loaders.

- [ ] **Step 4: Remove the preprocessor substitution block from `main_round5.cpp`**

After explicit call sites compile, include the `.ipp` files directly without `#define/#undef` trust routing.

- [ ] **Step 5: Route all rebuild points through typed context**

Update:

- full audit post-stress decision;
- inventory-only decision;
- `RebuildDecisionAndReports()` after manual functional/seller/port evidence.

No call to `BuildAuditDecision(report)` without a context may remain.

- [ ] **Step 6: Preserve profile-provenance fail-closed regression**

Update `LapSureProfileProvenanceTests` so mutable chassis claiming `physical-verified` still resolves only to advisory authority under the new path.

- [ ] **Step 7: Run source + compiled focused regressions**

```cmd
run_source_tests.cmd
```

```powershell
cmake --build --preset build-msvc-x64-ci --target LapSureDecisionAuthorityTests LapSureProfileProvenanceTests LapSureBehaviorTests
ctest --test-dir out/build/msvc-x64-ci -C Release -R "LapSureDecisionAuthorityTests|LapSureProfileProvenanceTests|LapSureBehaviorTests" --output-on-failure
```

- [ ] **Step 8: Commit**

```bash
git add src/app_audit.ipp src/app_runtime_state.ipp src/main_round5.cpp include/lap/chassis_profile.h CMakeLists.txt tests/profile_provenance_security_tests.cpp tests/*.py
git commit -m "refactor: use explicit decision trust boundaries"
```

---

### Task 7: Persist Policy/Authority/Coverage Snapshot Without Recomputing It

**Files:**
- Modify: `src/report.cpp`
- Modify: `include/lap/session_history.h`
- Modify: `src/session_history.cpp`
- Modify: `tests/report_publication_tests.cpp`
- Modify: `tests/session_history_tests.cpp`

**Interfaces:**
- `AuditDecision` is the canonical serialized output for policy versions, authority labels, dGPU capability truth, and frozen coverage domains.
- Upgrade session-history schema from v1 to v2 with three appended fields:
  - `decisionPolicyVersion`
  - `coveragePolicyVersion`
  - `authorityPolicyVersion`

- [ ] **Step 1: Add report RED assertions**

Extend report publication/behavioral tests to require generated JSON contains exactly:

```json
"decisionPolicyVersion":"5.1.0",
"coveragePolicyVersion":"5.1.0",
"authorityPolicyVersion":"5.1.0"
```

and includes frozen authority/capability labels plus coverage domains from `AuditDecision`.

- [ ] **Step 2: Add session-history migration RED tests**

Required cases:

```text
v2 write/read round trip preserves all three policy versions
existing valid v1 row loads successfully with empty/"legacy-v1" policy fields
malformed v2 row or unsupported schema still fails closed
```

Do not silently reinterpret corrupt v2 data as v1.

- [ ] **Step 3: Run RED**

```powershell
cmake --build --preset build-msvc-x64-ci --target LapSureReportPublicationTests LapSureSessionHistoryTests
ctest --test-dir out/build/msvc-x64-ci -C Release -R "LapSureReportPublicationTests|LapSureSessionHistoryTests" --output-on-failure
```

- [ ] **Step 4: Serialize the frozen decision snapshot**

In `report.cpp`, replace calls that recompute `BuildCoverageContract(report)` with iteration over:

```cpp
report.hardware.stress.decision.coverageDomains
```

Emit policy/authority/capability fields from `AuditDecision` in both JSON and HTML technical evidence sections.

- [ ] **Step 5: Implement strict history schema v2 with explicit v1 compatibility**

Change:

```cpp
constexpr int kHistorySchemaVersion = 2;
```

Write 13 fields for v2. On load:

- header `#LapSureSessionHistory\t1` → parse exactly 10 fields and populate policy fields with `legacy-v1`;
- header `...\t2` → parse exactly 13 fields;
- any other version → invalid index, preserving existing fail-closed behavior.

- [ ] **Step 6: Run GREEN plus history/report regressions**

All publication and history transactional/fault-injection tests must remain green.

- [ ] **Step 7: Commit**

```bash
git add src/report.cpp include/lap/session_history.h src/session_history.cpp tests/report_publication_tests.cpp tests/session_history_tests.cpp
git commit -m "feat: persist Round 5.1 decision policy metadata"
```

---

### Task 8: Full 5.1A Verification, Bounded Security Review, and Tracker Closure

**Files:**
- Modify only if evidence requires: source sanity tests / `SECURITY.md` / Round 5.1 status docs.
- Do not touch 5.1B provider execution implementation.

**Interfaces:**
- Review range starts at `df3ab209c4afba21ac42ed7bbbb2dfcb615419b6` and ends at the final 5.1A head.

- [ ] **Step 1: Run full source regression locally/CI-capable environment**

```cmd
run_source_tests.cmd
```

Expected: all source contracts PASS, including no macro trust routing and no stale `physical-verified` decision authority checks.

- [ ] **Step 2: Configure and strict-build the complete project**

```powershell
cmake --preset msvc-x64-ci
cmake --build --preset build-msvc-x64-ci
```

Expected: Release production target and every test target build with `/W4 /WX`, zero warnings/errors.

- [ ] **Step 3: Run every CTest suite**

```powershell
ctest --test-dir out/build/msvc-x64-ci -C Release --output-on-failure
```

Expected: all existing six suites plus `LapSureDecisionAuthorityTests` PASS; if additional focused targets were added during implementation, the total count must reflect them explicitly rather than retaining a hard-coded historical count.

- [ ] **Step 4: Run inventory-only transactional preflight**

Use the same command/contract as `.github/workflows/windows-msvc-build.yml`. Verify:

```text
published bundle-* exists
HTML + JSON siblings exist
no .staging-* remains
session_history.tsv commits
stress.completed == false
stress.stages.Count == 0
final decision == INCOMPLETE
policy versions in JSON == 5.1.0
```

- [ ] **Step 5: Perform bounded security/diff review**

Review only:

```text
df3ab209c4afba21ac42ed7bbbb2dfcb615419b6..HEAD
```

Required questions:

```text
Can any mutable profile/operator/report input mint Certified authority?
Can Unknown become AbsentConfirmed by default/empty-vector behavior?
Can seller mismatch be hidden by provider incompleteness?
Can expected required ports be deleted from the denominator?
Can coverage and scoring observe different requirement states?
Can runtime/self-validation failure become machine REJECT?
Can any old BuildAuditDecision(report) bypass remain?
Did 5.1A accidentally enable third-party providers or weaken engine trust?
```

- [ ] **Step 6: Record evidence on Issue #7 and PR #2**

Record exact 5.1A head SHA, test counts, strict build result, inventory preflight result, bounded compare file list, and any discrepancy disposition. Do not claim provider closure or physical acceptance.

- [ ] **Step 7: Mark Issue #7 closed only when all exit criteria are proven**

Issue #8 remains blocked until this evidence is complete. No formal acceptance package is created at the end of 5.1A alone unless needed as an internal smoke artifact; the formal replacement candidate waits for 5.1B provider closure.

---

## Self-Review Checklist for This Plan

- Every approved 5.1A invariant maps to a task.
- Provider trust root/dependency/TOCTOU implementation is explicitly excluded and remains #8.
- Physical/DPI/accessibility/main-protection release closure remains #9.
- The first behavior change starts from a real RED that compiles against the existing product and demonstrates the happy-path deadlock.
- `Certified` has no production caller-settable constructor/path in 5.1A; only a test hook can create it until a protected authority source exists.
- Capability absence requires completed evidence; empty/failed enumeration is never absence.
- Session port attestation preserves the expected denominator.
- Coverage and scoring share exactly one `RequirementSnapshot`.
- Reports serialize the frozen decision snapshot instead of recomputing policy.
- Session-history schema migration is explicit and fail-closed.
- No task requires a third-party network download or binary.
- No `TODO`, `TBD`, “similar to”, or unspecified error-handling placeholder is part of the implementation instructions.

## Execution Strategy

Use **subagent-driven development** when available, with one fresh worker per task and review between tasks. In this ChatGPT/GitHub environment, if subagent execution is unavailable, use the equivalent inline `executing-plans` workflow: execute one task/TDD cycle at a time, verify evidence before each completion claim, and record the tranche checkpoint before moving to the next task.
