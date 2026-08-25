#include "lap/decision_context.h"
#include <utility>

namespace lap {

RequirementDisposition RequirementSnapshot::StateOf(std::wstring_view id) const {
    for (const auto& domain : domains) {
        if (domain.id == id) return domain.disposition;
    }
    return RequirementDisposition::ConditionalBlocked;
}

bool RequirementSnapshot::IsRequired(std::wstring_view id) const {
    return StateOf(id) == RequirementDisposition::Required;
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

} // namespace lap
