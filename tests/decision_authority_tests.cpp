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

    lap::GpuInfo gpu{};
    gpu.name = L"Intel Iris Xe Graphics";
    report.hardware.gpus.push_back(gpu);

    lap::DisplayInfo display{};
    display.friendlyName = L"Internal panel";
    report.hardware.displays.push_back(display);

    lap::StressStageResult cpu{};
    cpu.name = L"CPU sustained load";
    cpu.verdict = lap::TestVerdict::Pass;
    cpu.telemetrySummary.maxCpuPackageTempC = 80.0;

    lap::StressStageResult ram{};
    ram.name = L"RAM online integrity";
    ram.verdict = lap::TestVerdict::Pass;
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
    report.hardware.stress.chassisProfile.ports.push_back(
        {L"tb4-left-1",
         L"TB4 left 1",
         L"Left",
         L"USB-C",
         L"TB4",
         true,
         true,
         L"PASS"});

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
}
} // namespace

int main() {
    TestAuthorityBoundary();
    TestDiscreteGpuTruth();
    TestPortAttestationCannotShrinkCoverage();
    TestPortAttestationRejectsLabelOnlyIdentity();
    TestPortAttestationCompletesByStableId();

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
