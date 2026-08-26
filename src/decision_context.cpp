#include "lap/decision_context.h"
#include "lap/decision_policy.h"
#include <algorithm>
#include <utility>

namespace lap {
namespace {

bool HasCpuSustainedLoad(const AuditReport& report) {
    return std::any_of(report.hardware.stress.stages.begin(),
                       report.hardware.stress.stages.end(),
                       [](const StressStageResult& stage) {
                           return stage.name.find(L"CPU") != std::wstring::npos &&
                                  stage.name.find(L"load") != std::wstring::npos &&
                                  stage.verdict != TestVerdict::NotTested &&
                                  stage.verdict != TestVerdict::Cancelled;
                       });
}

bool HasExpectedRequiredPort(const SessionPortAttestation& attestation) {
    return std::any_of(attestation.ports.begin(), attestation.ports.end(),
                       [](const SessionPortEvidence& port) {
                           return port.expectedRequired;
                       });
}

bool HasUnresolvedSellerDiscreteGpuClaim(const AuditReport& report) {
    if (!report.sellerClaim.provided || report.sellerClaim.gpuContains.empty()) return false;

    const bool alreadyDisproved = std::any_of(
        report.findings.begin(), report.findings.end(),
        [](const Finding& finding) {
            return finding.state == State::Fail &&
                   finding.severity == Severity::Critical &&
                   finding.dimension == Dimension::Factory &&
                   (finding.name.find(L"GPU") != std::wstring::npos ||
                    finding.group.find(L"Seller") != std::wstring::npos ||
                    finding.group.find(L"seller") != std::wstring::npos);
        });
    return !alreadyDisproved;
}

void AddRequirement(RequirementSnapshot& snapshot,
                    std::wstring id,
                    RequirementDisposition disposition,
                    std::wstring reason) {
    snapshot.domains.push_back(
        {std::move(id), disposition, std::move(reason)});
}

void InvalidateCrossSessionPortAuthority(SessionPortAttestation& attestation) {
    attestation.operatorConfirmed = false;
    attestation.confirmedAt.clear();
    for (auto& port : attestation.ports) {
        port.observedPresence = CapabilityTruth::Unknown;
        port.tested = false;
        port.verdict = L"NOT TESTED";
        port.discrepancy = L"ATTESTATION_SESSION_MISMATCH";
        port.correctionReason = L"Port evidence belongs to a different inspection session.";
    }
}

} // namespace

RequirementDisposition RequirementSnapshot::StateOf(std::wstring_view id) const {
    for (const auto& domain : domains) {
        if (domain.id == id) return domain.disposition;
    }
    return RequirementDisposition::ConditionalBlocked;
}

bool RequirementSnapshot::IsRequired(std::wstring_view id) const {
    return StateOf(id) != RequirementDisposition::NotApplicable;
}

ChassisAuthorityEvidence::ChassisAuthorityEvidence(ChassisAuthorityLevel level,
                                                   std::wstring source)
    : level_(level), source_(std::move(source)) {}

ChassisAuthorityEvidence ChassisAuthorityEvidence::None() {
    return ChassisAuthorityEvidence(ChassisAuthorityLevel::None, L"");
}

ChassisAuthorityEvidence ChassisAuthorityEvidence::Advisory(std::wstring source) {
    return ChassisAuthorityEvidence(ChassisAuthorityLevel::Advisory, std::move(source));
}

DecisionProfileResolution DecisionProfileResolver::ResolvePortable(const ChassisProfile& chassis,
                                                                   bool factoryAuthenticated,
                                                                   std::wstring factorySource) {
    DecisionProfileResolution resolution{};
    resolution.chassis = chassis;
    resolution.chassisAuthority = chassis.profileId.empty()
        ? ChassisAuthorityEvidence::None()
        : ChassisAuthorityEvidence::Advisory(
              chassis.source.empty() ? L"portable chassis metadata" : chassis.source);
    resolution.factoryAuthority = factoryAuthenticated
        ? FactoryAuthorityLevel::Authenticated
        : (factorySource.empty() ? FactoryAuthorityLevel::Unknown
                                 : FactoryAuthorityLevel::Advisory);
    resolution.factorySource = std::move(factorySource);
    return resolution;
}

#ifdef LAPSURE_ENABLE_TEST_HOOKS
DecisionProfileResolution DecisionProfileResolver::CertifiedForTest(const ChassisProfile& chassis,
                                                                    std::wstring source) {
    DecisionProfileResolution resolution{};
    resolution.chassis = chassis;
    resolution.chassisAuthority = ChassisAuthorityEvidence(
        ChassisAuthorityLevel::Certified, std::move(source));
    return resolution;
}
#endif

RequirementSnapshot BuildRequirementSnapshot(
    const AuditReport& report,
    const ObservedCapabilities& capabilities,
    const SessionPortAttestation& attestation) {
    RequirementSnapshot snapshot{};
    snapshot.mode = L"purchase-grade";

    AddRequirement(snapshot, L"identity", RequirementDisposition::Required,
                   L"Purchase-grade decisions require stable machine identity.");
    AddRequirement(snapshot, L"memory", RequirementDisposition::Required,
                   L"Installed memory inventory remains a required machine-health domain.");
    AddRequirement(snapshot, L"storage", RequirementDisposition::Required,
                   L"Storage identity and health evidence remain required.");
    AddRequirement(snapshot, L"battery",
                   report.hardware.battery.present
                       ? RequirementDisposition::Required
                       : RequirementDisposition::NotApplicable,
                   report.hardware.battery.present
                       ? L"Detected battery requires readable health evidence."
                       : L"No battery is present in the observed machine inventory.");
    AddRequirement(snapshot, L"graphics", RequirementDisposition::Required,
                   L"Graphics inventory is required to establish discrete-GPU capability truth.");
    AddRequirement(snapshot, L"display", RequirementDisposition::Required,
                   L"Display identity/evidence remains required.");
    AddRequirement(snapshot, L"stability", RequirementDisposition::Required,
                   L"CPU/RAM stability evidence is required independently of optional dGPU coverage.");
    AddRequirement(snapshot, L"functional", RequirementDisposition::Required,
                   L"Functional and physical-condition evidence remains required.");

    RequirementDisposition gpuDisposition = RequirementDisposition::ConditionalBlocked;
    std::wstring gpuReason = L"Discrete-GPU capability state is unknown.";
    if (HasUnresolvedSellerDiscreteGpuClaim(report)) {
        gpuDisposition = RequirementDisposition::Required;
        gpuReason = L"Seller claims a discrete GPU and the claim has not already been disproved.";
    } else if (capabilities.discreteGpu.state == CapabilityTruth::Present) {
        gpuDisposition = RequirementDisposition::Required;
        gpuReason = L"Observed discrete GPU requires a completed GPU/VRAM stress stage.";
    } else if (capabilities.discreteGpu.state == CapabilityTruth::AbsentConfirmed) {
        gpuDisposition = RequirementDisposition::NotApplicable;
        gpuReason = L"Complete GPU inventory confirms no discrete GPU and no unresolved seller dGPU claim exists.";
    }
    AddRequirement(snapshot, L"gpu_vram", gpuDisposition, std::move(gpuReason));

    AddRequirement(snapshot, L"thermals",
                   HasCpuSustainedLoad(report)
                       ? RequirementDisposition::Required
                       : RequirementDisposition::NotApplicable,
                   HasCpuSustainedLoad(report)
                       ? L"CPU sustained-load execution requires trusted CPU package thermal telemetry."
                       : L"No CPU sustained-load stage has executed in this decision snapshot.");

    if (HasExpectedRequiredPort(attestation)) {
        AddRequirement(snapshot, L"ports_power", RequirementDisposition::Required,
                       L"Current-session attestation contains expected required physical ports.");
    } else {
        AddRequirement(snapshot, L"ports_power", RequirementDisposition::ConditionalBlocked,
                       L"No authoritative expected required-port denominator is available for purchase-grade acceptance.");
    }

    AddRequirement(snapshot, L"runtime", RequirementDisposition::Required,
                   L"LapSure runtime/self-integrity validation is always required for an acceptance verdict.");
    return snapshot;
}

DecisionContext BuildDecisionContext(const AuditReport& report) {
    DecisionContext context{};
    context.capabilities = NormalizeObservedCapabilities(report);

    // factoryExact is identity/configuration matching only. Until explicit authenticated
    // provenance is carried as typed evidence, it must not mint Authenticated authority.
    context.profile = DecisionProfileResolver::ResolvePortable(
        report.hardware.stress.chassisProfile,
        false,
        report.profileSource);
    context.portAttestation = report.hardware.stress.portAttestation;
    if (context.portAttestation.sessionId != report.hardware.stress.sessionId) {
        InvalidateCrossSessionPortAuthority(context.portAttestation);
    }
    context.requirements = BuildRequirementSnapshot(
        report, context.capabilities, context.portAttestation);
    context.versions = context.requirements.versions;
    return context;
}

#ifdef LAPSURE_ENABLE_TEST_HOOKS
DecisionContext BuildCertifiedDecisionContextForTest(
    const AuditReport& report,
    std::wstring source) {
    auto context = BuildDecisionContext(report);
    const auto factoryAuthority = context.profile.factoryAuthority;
    const auto factorySource = context.profile.factorySource;
    context.profile = DecisionProfileResolver::CertifiedForTest(
        report.hardware.stress.chassisProfile, std::move(source));
    context.profile.factoryAuthority = factoryAuthority;
    context.profile.factorySource = factorySource;
    return context;
}
#endif

} // namespace lap
