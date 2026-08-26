#include "lap/scoring.h"
#include "lap/decision_context.h"
#include "lap/decision_policy.h"
#include "lap/port_attestation.h"
#include <iostream>

namespace {
int failures = 0;

void Expect(bool ok, const char* name) {
    if (!ok) {
        std::cerr << "FAIL " << name << '\n';
        ++failures;
    } else {
        std::cout << "PASS " << name << '\n';
    }
}

void CompleteCurrentSessionPorts(lap::AuditReport& report) {
    report.hardware.stress.sessionId = L"task5-session";
    report.hardware.stress.portAttestation = lap::InitializeSessionPortAttestation(
        report.hardware.stress.sessionId,
        report.hardware.stress.chassisProfile);

    for (const auto& expected : report.hardware.stress.chassisProfile.ports) {
        if (!expected.required) continue;
        lap::PortProbeResult result{};
        result.expectedPortId = expected.id;
        result.portLabel = expected.label;
        result.deviceEnumerated = true;
        result.confidence = lap::Confidence::High;
        result.verdict = L"PASS";
        result.evidence = L"Stable-ID operator stimulus completed";
        lap::ApplyPortResultToAttestation(report.hardware.stress.portAttestation, result);
    }
}

lap::AuditReport HealthyAdvisoryFixture() {
    lap::AuditReport report{};
    report.model = L"Precision test fixture";
    report.serviceTag = L"TEST-SESSION";
    report.hardware.cpuName = L"Test CPU";
    report.hardware.installedRamBytes = 16ULL * 1024ULL * 1024ULL * 1024ULL;

    lap::MemoryModule module{};
    module.capacityBytes = report.hardware.installedRamBytes;
    report.hardware.memoryModules.push_back(module);

    lap::StorageDevice disk{};
    disk.model = L"Test NVMe";
    disk.capacityBytes = 512ULL * 1000ULL * 1000ULL * 1000ULL;
    disk.reliabilityReadable = true;
    disk.reliabilityHealthy = true;
    report.hardware.storage.push_back(disk);

    report.hardware.gpuInventoryStatus = lap::ProviderCollectionStatus::Complete;
    lap::GpuInfo gpu{};
    gpu.name = L"Intel Iris Xe Graphics";
    report.hardware.gpus.push_back(gpu);

    lap::DisplayInfo display{};
    display.friendlyName = L"Internal panel";
    report.hardware.displays.push_back(display);

    lap::StressStageResult cpu{};
    cpu.name = L"CPU sustained load";
    cpu.verdict = lap::TestVerdict::Pass;
    lap::TelemetrySample thermal{};
    thermal.second = 1;
    thermal.cpuUtilPercent = 95.0;
    thermal.cpuPackageTempC = 80.0;
    thermal.cpuThermalConfidence = lap::Confidence::High;
    thermal.cpuThermalSource = L"trusted test telemetry";
    cpu.telemetry.push_back(thermal);
    lap::AssessStressStage(cpu);

    lap::StressStageResult ram{};
    ram.name = L"RAM online integrity";
    ram.verdict = lap::TestVerdict::Pass;
    ram.ram.bytesAllocated = 8ULL * 1024ULL * 1024ULL * 1024ULL;
    ram.ram.bytesTested = ram.ram.bytesAllocated;
    ram.ram.passes = 1;
    report.hardware.stress.stages = {cpu, ram};

    for (const auto* id : {
             L"physical_chassis",
             L"physical_hinge",
             L"physical_tamper",
             L"physical_liquid",
             L"physical_battery",
             L"physical_charger"}) {
        report.hardware.stress.functional.items.push_back(
            {id,
             L"Physical",
             lap::FunctionalStatus::Pass,
             L"Passed",
             L"Operator stimulus",
             lap::Confidence::High,
             false});
    }
    report.hardware.stress.functional.overall = L"PASS";
    report.hardware.stress.portPower.overall = L"PASS";
    report.hardware.stress.runtimeValidation.overall = L"PASS";

    report.hardware.stress.chassisProfile.profileId = L"fixture";
    report.hardware.stress.chassisProfile.validationStatus = L"draft";
    report.hardware.stress.chassisProfile.source = L"portable advisory fixture";
    report.hardware.stress.chassisProfile.ports.push_back(
        {L"tb4-left-1",
         L"TB4 left 1",
         L"Left",
         L"USB-C",
         L"TB4",
         true,
         true,
         L"PASS"});
    CompleteCurrentSessionPorts(report);

    return report;
}

void TestAuthorityBoundary() {
    lap::ChassisProfile raw{};
    raw.profileId = L"mutable-profile";
    raw.validationStatus = L"physical-verified";

    const auto resolved = lap::DecisionProfileResolver::ResolvePortable(raw, false, L"disk");
    Expect(resolved.chassisAuthority.Level() == lap::ChassisAuthorityLevel::Advisory,
           "mutable portable chassis cannot mint Certified authority");

#ifdef LAPSURE_ENABLE_TEST_HOOKS
    const auto certified = lap::DecisionProfileResolver::CertifiedForTest(
        raw, L"test-only protected authority");
    Expect(certified.chassisAuthority.Level() == lap::ChassisAuthorityLevel::Certified,
           "test-only fixture models Certified path");
#endif
}

void TestConditionalBlockedRemainsAcceptanceBlocking() {
    lap::RequirementSnapshot requirements{};
    requirements.domains.push_back(
        {L"gpu_vram", lap::RequirementDisposition::ConditionalBlocked, L"dGPU capability unknown"});
    Expect(requirements.IsRequired(L"gpu_vram"),
           "ConditionalBlocked requirement remains acceptance-blocking");
}

void TestDiscreteGpuTruth() {
    lap::AuditReport report{};
    report.hardware.gpuInventoryStatus = lap::ProviderCollectionStatus::Failed;
    auto capabilities = lap::NormalizeObservedCapabilities(report);
    Expect(capabilities.discreteGpu.state == lap::CapabilityTruth::Unknown,
           "failed GPU enumeration remains Unknown");

    report.hardware.gpuInventoryStatus = lap::ProviderCollectionStatus::Complete;
    report.hardware.gpus.clear();
    capabilities = lap::NormalizeObservedCapabilities(report);
    Expect(capabilities.discreteGpu.state == lap::CapabilityTruth::Unknown,
           "empty successful GPU inventory is not absence proof");

    lap::GpuInfo integrated{};
    integrated.name = L"Intel Iris Xe Graphics";
    report.hardware.gpus.push_back(integrated);
    capabilities = lap::NormalizeObservedCapabilities(report);
    Expect(capabilities.discreteGpu.state == lap::CapabilityTruth::AbsentConfirmed,
           "integrated-only successful inventory confirms dGPU absence");

    lap::GpuInfo discrete{};
    discrete.name = L"NVIDIA RTX A2000 Laptop GPU";
    report.hardware.gpus.push_back(discrete);
    capabilities = lap::NormalizeObservedCapabilities(report);
    Expect(capabilities.discreteGpu.state == lap::CapabilityTruth::Present,
           "trusted inventory recognizes dGPU presence");

    report.hardware.gpus.clear();
    lap::GpuInfo ambiguous{};
    ambiguous.name = L"Intel Arc Graphics";
    report.hardware.gpus.push_back(ambiguous);
    capabilities = lap::NormalizeObservedCapabilities(report);
    Expect(capabilities.discreteGpu.state == lap::CapabilityTruth::Unknown,
           "ambiguous GPU identity remains Unknown");
}

lap::ChassisProfile TwoRequiredPortProfile() {
    lap::ChassisProfile profile{};
    profile.profileId = L"advisory";
    profile.ports.push_back(
        {L"left-tb4-1", L"Left TB4 #1", L"Left", L"USB-C", L"TB4", true, false, L"NOT TESTED"});
    profile.ports.push_back(
        {L"left-tb4-2", L"Left TB4 #2", L"Left", L"USB-C", L"TB4", true, false, L"NOT TESTED"});
    return profile;
}

void TestPortAttestationCannotShrinkCoverage() {
    const auto profile = TwoRequiredPortProfile();
    auto attestation = lap::InitializeSessionPortAttestation(L"session-1", profile);

    lap::RecordPortObservation(attestation,
                               L"left-tb4-2",
                               lap::CapabilityTruth::AbsentConfirmed,
                               L"Operator reports port absent");

    Expect(attestation.ports.size() == 2,
           "operator correction preserves expected denominator");
    Expect(lap::RequiredPortsRemaining(attestation) == 2,
           "missing expected port remains blocking");
    Expect(!attestation.ports[1].discrepancy.empty(),
           "expected port absence records discrepancy");
}

void TestPortAttestationRejectsLabelOnlyIdentity() {
    const auto profile = TwoRequiredPortProfile();
    auto attestation = lap::InitializeSessionPortAttestation(L"session-label", profile);

    lap::PortProbeResult labelOnly{};
    labelOnly.portLabel = L"Left TB4 #1";
    labelOnly.deviceEnumerated = true;
    labelOnly.verdict = L"PASS";
    lap::ApplyPortResultToAttestation(attestation, labelOnly);

    Expect(lap::RequiredPortsRemaining(attestation) == 2,
           "label-only result cannot satisfy expected port coverage");
    Expect(!attestation.ports[0].tested,
           "label-only result leaves expected row untested");
}

void TestPortAttestationCompletesByStableId() {
    const auto profile = TwoRequiredPortProfile();
    auto attestation = lap::InitializeSessionPortAttestation(L"session-2", profile);

    lap::PortProbeResult first{};
    first.expectedPortId = L"left-tb4-1";
    first.portLabel = L"Left TB4 #1";
    first.deviceEnumerated = true;
    first.verdict = L"PASS";
    lap::ApplyPortResultToAttestation(attestation, first);

    lap::PortProbeResult second{};
    second.expectedPortId = L"left-tb4-2";
    second.portLabel = L"Left TB4 #2";
    second.deviceEnumerated = true;
    second.verdict = L"PASS";
    lap::ApplyPortResultToAttestation(attestation, second);

    Expect(lap::RequiredPortsRemaining(attestation) == 0,
           "required ports complete only from stable expected IDs");
    Expect(attestation.operatorConfirmed && !attestation.confirmedAt.empty(),
           "complete required port evidence confirms the session attestation");

    lap::RecordPortObservation(attestation,
                               L"left-tb4-2",
                               lap::CapabilityTruth::AbsentConfirmed,
                               L"Operator correction after probe");
    Expect(lap::RequiredPortsRemaining(attestation) == 1,
           "later expected-port absence reopens required coverage");
    Expect(!attestation.operatorConfirmed && attestation.confirmedAt.empty(),
           "later expected-port absence revokes stale confirmation");
    Expect(attestation.ports.size() == 2,
           "later correction still preserves expected denominator");

    lap::ApplyPortResultToAttestation(attestation, second);
    Expect(attestation.ports[1].discrepancy.empty() &&
               attestation.ports[1].correctionReason.empty(),
           "later stable-ID PASS clears stale absence discrepancy state");

    lap::RecordPortObservation(attestation,
                               L"left-tb4-1",
                               lap::CapabilityTruth::Unknown,
                               L"Operator can no longer establish presence");
    Expect(!attestation.ports[0].tested && attestation.ports[0].verdict == L"NOT TESTED",
           "Unknown observation invalidates stale completed port verdict");
}

void TestCriticalMachineFailureDominatesRuntimeFailure() {
    auto report = HealthyAdvisoryFixture();
    report.hardware.stress.stages.front().verdict = lap::TestVerdict::Fail;
    report.hardware.stress.runtimeValidation.failed = 1;
    report.hardware.stress.runtimeValidation.overall = L"FAIL";

    const auto decision = lap::BuildAuditDecision(report);
    Expect(decision.overall == L"REJECT",
           "trusted critical machine failure remains REJECT when runtime validation also fails");
}

void TestTask5RequirementSnapshotRequiredness() {
    auto report = HealthyAdvisoryFixture();
    auto context = lap::BuildDecisionContext(report);

    Expect(context.capabilities.discreteGpu.state == lap::CapabilityTruth::AbsentConfirmed,
           "Task 5 context freezes normalized dGPU capability truth");
    Expect(context.requirements.StateOf(L"gpu_vram") == lap::RequirementDisposition::NotApplicable,
           "confirmed dGPU absence without seller claim makes GPU coverage NotApplicable");
    Expect(context.requirements.StateOf(L"thermals") == lap::RequirementDisposition::Required,
           "CPU sustained load makes trusted thermal evidence Required");
    Expect(context.requirements.StateOf(L"ports_power") == lap::RequirementDisposition::Required,
           "expected required ports make session port coverage Required");
    Expect(context.requirements.StateOf(L"runtime") == lap::RequirementDisposition::Required,
           "runtime validation remains Required for purchase-grade decisions");

    auto sellerGpuClaim = report;
    sellerGpuClaim.sellerClaim.provided = true;
    sellerGpuClaim.sellerClaim.gpuContains = L"NVIDIA RTX A2000";
    auto sellerContext = lap::BuildDecisionContext(sellerGpuClaim);
    Expect(sellerContext.requirements.StateOf(L"gpu_vram") == lap::RequirementDisposition::Required,
           "unresolved seller dGPU claim keeps GPU evidence Required");

    auto unknownGpu = report;
    unknownGpu.hardware.gpuInventoryStatus = lap::ProviderCollectionStatus::Failed;
    auto unknownContext = lap::BuildDecisionContext(unknownGpu);
    Expect(unknownContext.requirements.StateOf(L"gpu_vram") == lap::RequirementDisposition::ConditionalBlocked,
           "unknown dGPU capability freezes GPU requirement as ConditionalBlocked");
}

void TestTask5VerdictLattice() {
    auto advisoryReport = HealthyAdvisoryFixture();
    const auto advisoryContext = lap::BuildDecisionContext(advisoryReport);
    const auto advisoryDecision = lap::BuildAuditDecision(advisoryReport, advisoryContext);
    Expect(advisoryDecision.overall == L"BUY WITH NOTES",
           "healthy Advisory plus complete current-session evidence reaches BUY WITH NOTES");

#ifdef LAPSURE_ENABLE_TEST_HOOKS
    const auto certifiedContext = lap::BuildCertifiedDecisionContextForTest(
        advisoryReport, L"test-only certified chassis authority");
    const auto certifiedDecision = lap::BuildAuditDecision(advisoryReport, certifiedContext);
    Expect(certifiedDecision.overall == L"BUY",
           "healthy test-only Certified context plus complete evidence reaches BUY");
#endif

    auto missingPort = advisoryReport;
    missingPort.hardware.stress.portAttestation = lap::InitializeSessionPortAttestation(
        missingPort.hardware.stress.sessionId,
        missingPort.hardware.stress.chassisProfile);
    const auto missingPortContext = lap::BuildDecisionContext(missingPort);
    Expect(lap::BuildAuditDecision(missingPort, missingPortContext).overall == L"INCOMPLETE",
           "expected required port left untested remains INCOMPLETE");

    auto discreteGpu = advisoryReport;
    discreteGpu.hardware.gpus.clear();
    lap::GpuInfo discrete{};
    discrete.name = L"NVIDIA RTX A2000 Laptop GPU";
    discreteGpu.hardware.gpus.push_back(discrete);
    const auto discreteContext = lap::BuildDecisionContext(discreteGpu);
    Expect(discreteContext.requirements.StateOf(L"gpu_vram") == lap::RequirementDisposition::Required &&
               lap::BuildAuditDecision(discreteGpu, discreteContext).overall == L"INCOMPLETE",
           "dGPU Present with unavailable GPU stage remains INCOMPLETE");

    auto unknownGpu = advisoryReport;
    unknownGpu.hardware.gpuInventoryStatus = lap::ProviderCollectionStatus::Failed;
    const auto unknownContext = lap::BuildDecisionContext(unknownGpu);
    Expect(lap::BuildAuditDecision(unknownGpu, unknownContext).overall == L"INCOMPLETE",
           "dGPU Unknown remains INCOMPLETE");

    auto missingThermal = advisoryReport;
    auto& cpu = missingThermal.hardware.stress.stages.front();
    cpu.telemetry.clear();
    cpu.telemetrySummary = {};
    const auto thermalContext = lap::BuildDecisionContext(missingThermal);
    Expect(lap::BuildAuditDecision(missingThermal, thermalContext).overall == L"INCOMPLETE",
           "CPU load without trusted CPU thermal sample remains INCOMPLETE");

    auto critical = advisoryReport;
    critical.findings.push_back({L"Seller claim",
                                 L"Critical trusted mismatch",
                                 L"FAIL",
                                 L"match",
                                 lap::State::Fail,
                                 lap::Severity::Critical,
                                 L"trusted machine/seller evidence",
                                 lap::Dimension::Factory});
    const auto criticalContext = lap::BuildDecisionContext(critical);
    Expect(lap::BuildAuditDecision(critical, criticalContext).overall == L"REJECT",
           "critical trusted seller or hardware finding has REJECT precedence");

    auto runtimeFailure = advisoryReport;
    runtimeFailure.hardware.stress.runtimeValidation.failed = 1;
    runtimeFailure.hardware.stress.runtimeValidation.overall = L"FAIL";
    const auto runtimeContext = lap::BuildDecisionContext(runtimeFailure);
    Expect(lap::BuildAuditDecision(runtimeFailure, runtimeContext).overall == L"INCOMPLETE",
           "runtime self-integrity failure yields INCOMPLETE without fabricating machine REJECT");
}

void TestTask5FactoryAuthorityAndFrozenMetadata() {
    auto report = HealthyAdvisoryFixture();
    report.factoryExact = true;
    report.profileSource = L"exact but unauthenticated portable factory metadata";

    const auto context = lap::BuildDecisionContext(report);
    Expect(context.profile.factoryAuthority != lap::FactoryAuthorityLevel::Authenticated,
           "factoryExact alone cannot mint Authenticated factory authority");

    const auto coverage = lap::BuildCoverageContract(report, context);
    const auto decision = lap::BuildAuditDecision(report, context);
    Expect(decision.overall == L"BUY WITH NOTES",
           "missing authenticated factory truth alone does not block complete machine evidence");
    Expect(decision.coverageDomains.size() == coverage.size() && !coverage.empty(),
           "decision freezes the same coverage contract built from its RequirementSnapshot");
    Expect(decision.decisionPolicyVersion == context.requirements.versions.decision &&
               decision.coveragePolicyVersion == context.requirements.versions.coverage &&
               decision.authorityPolicyVersion == context.requirements.versions.authority &&
               decision.decisionPolicyVersion == L"5.1.0",
           "decision persists frozen decision coverage and authority policy versions");
    Expect(!decision.chassisAuthority.empty() &&
               !decision.factoryAuthority.empty() &&
               !decision.discreteGpuCapability.empty(),
           "decision persists authority and capability metadata labels");
}
void TestCrossSessionPortAuthorityIsRevoked() {
    auto report = HealthyAdvisoryFixture();
    report.hardware.stress.sessionId = L"session-A";
    report.hardware.stress.portAttestation = lap::InitializeSessionPortAttestation(
        report.hardware.stress.sessionId,
        report.hardware.stress.chassisProfile);

    lap::PortProbeResult probePass{};
    probePass.expectedPortId = L"tb4-left-1";
    probePass.portLabel = L"TB4 left 1";
    probePass.deviceEnumerated = true;
    probePass.confidence = lap::Confidence::High;
    probePass.verdict = L"PASS";
    probePass.evidence = L"Operator probe PASS in session A";
    lap::ApplyPortResultToAttestation(report.hardware.stress.portAttestation, probePass);

    Expect(report.hardware.stress.portAttestation.operatorConfirmed &&
               !report.hardware.stress.portAttestation.confirmedAt.empty(),
           "attestation is confirmed in matching session A");

    // Switch report stress session to session-B without changing attestation sessionId
    report.hardware.stress.sessionId = L"session-B";

    const auto context = lap::BuildDecisionContext(report);

    Expect(context.portAttestation.ports.size() == report.hardware.stress.chassisProfile.ports.size(),
           "cross-session invalidation preserves expected denominator");
    Expect(!context.portAttestation.ports.empty() &&
               context.portAttestation.ports[0].expectedPortId == L"tb4-left-1",
           "cross-session invalidation preserves expectedPortId values");
    Expect(context.portAttestation.ports[0].expectedRequired == true,
           "cross-session invalidation preserves expectedRequired flags");
    Expect(context.portAttestation.operatorConfirmed == false,
           "cross-session mismatch revokes operatorConfirmed");
    Expect(context.portAttestation.confirmedAt.empty(),
           "cross-session mismatch clears confirmedAt");
    Expect(context.portAttestation.ports[0].observedPresence == lap::CapabilityTruth::Unknown,
           "cross-session mismatch sets observedPresence to Unknown");
    Expect(context.portAttestation.ports[0].tested == false,
           "cross-session mismatch resets tested to false");
    Expect(context.portAttestation.ports[0].verdict == L"NOT TESTED",
           "cross-session mismatch resets verdict to NOT TESTED");
    Expect(lap::RequiredPortsRemaining(context.portAttestation) > 0,
           "cross-session mismatch leaves required ports remaining");

    const auto decision = lap::BuildAuditDecision(report, context);
    Expect(decision.overall == L"INCOMPLETE",
           "cross-session port mismatch produces INCOMPLETE verdict");
    Expect(decision.overall != L"BUY" && decision.overall != L"BUY WITH NOTES",
           "cross-session port mismatch explicitly forbids BUY and BUY WITH NOTES");

    // Stale-failure subcase: Old session A contains a required port FAIL
    auto staleFailReport = HealthyAdvisoryFixture();
    staleFailReport.hardware.stress.sessionId = L"session-A";
    staleFailReport.hardware.stress.portAttestation = lap::InitializeSessionPortAttestation(
        staleFailReport.hardware.stress.sessionId,
        staleFailReport.hardware.stress.chassisProfile);

    lap::PortProbeResult probeFail{};
    probeFail.expectedPortId = L"tb4-left-1";
    probeFail.portLabel = L"TB4 left 1";
    probeFail.deviceEnumerated = false;
    probeFail.confidence = lap::Confidence::High;
    probeFail.verdict = L"FAIL";
    probeFail.evidence = L"Operator probe FAIL in old session";
    lap::ApplyPortResultToAttestation(staleFailReport.hardware.stress.portAttestation, probeFail);

    // Switch report stress session to session-B
    staleFailReport.hardware.stress.sessionId = L"session-B";
    const auto staleFailContext = lap::BuildDecisionContext(staleFailReport);
    const auto staleFailDecision = lap::BuildAuditDecision(staleFailReport, staleFailContext);

    Expect(staleFailDecision.overall == L"INCOMPLETE",
           "stale old-session FAIL is invalidated and does not manufacture REJECT for session B");
    Expect(staleFailDecision.overall != L"REJECT",
           "stale old-session port FAIL must not produce REJECT without independent machine failure");
}
} // namespace

int main() {
    TestAuthorityBoundary();
    TestConditionalBlockedRemainsAcceptanceBlocking();
    TestDiscreteGpuTruth();
    TestPortAttestationCannotShrinkCoverage();
    TestPortAttestationRejectsLabelOnlyIdentity();
    TestPortAttestationCompletesByStableId();
    TestCriticalMachineFailureDominatesRuntimeFailure();
    TestTask5RequirementSnapshotRequiredness();
    TestTask5VerdictLattice();
    TestTask5FactoryAuthorityAndFrozenMetadata();
    TestCrossSessionPortAuthorityIsRevoked();

    auto report = HealthyAdvisoryFixture();
    const auto decision = lap::BuildAuditDecision(report);

    if (decision.overall != L"BUY WITH NOTES") {
        std::wcerr << L"Observed verdict: " << decision.overall << L'\n';
        for (const auto& reason : decision.reasons) {
            std::wcerr << L"Reason: " << reason << L'\n';
        }
    }

    Expect(decision.overall == L"BUY WITH NOTES",
           "healthy advisory chassis is reachable as BUY WITH NOTES");
    return failures == 0 ? 0 : 1;
}