#pragma once
#include "model.h"
#include <string>
#include <string_view>
#include <vector>

namespace lap {

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

struct DecisionContext {
    PolicyVersions versions;
    DecisionProfileResolution profile;
    ObservedCapabilities capabilities;
    RequirementSnapshot requirements;
    SessionPortAttestation portAttestation;
};

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

RequirementSnapshot BuildRequirementSnapshot(
    const AuditReport& report,
    const ObservedCapabilities& capabilities,
    const SessionPortAttestation& attestation);

DecisionContext BuildDecisionContext(const AuditReport& report);

#ifdef LAPSURE_ENABLE_TEST_HOOKS
DecisionContext BuildCertifiedDecisionContextForTest(
    const AuditReport& report,
    std::wstring source);
#endif

} // namespace lap
