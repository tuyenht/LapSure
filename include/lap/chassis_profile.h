#pragma once
#include "model.h"
namespace lap {
ChassisProfile LoadChassisProfile(const std::wstring&,const std::wstring&);
void ApplyPortResultToChassisProfile(ChassisProfile&,const PortProbeResult&);
unsigned RequiredPortsRemaining(const ChassisProfile&);

// Production decision boundary for mutable portable chassis metadata. Disk-loaded
// profiles may guide operator port checks, but they cannot self-assert the
// physical-verification status that unlocks a clean acceptance verdict.
inline ChassisProfile LoadDecisionChassisProfile(const std::wstring& appDir,
                                                 const std::wstring& model) {
    auto profile = LoadChassisProfile(appDir, model);
    if (profile.validationStatus == L"physical-verified") {
        profile.validationStatus = L"static-unverified";
        if (!profile.reference.empty()) profile.reference += L" | ";
        profile.reference += L"Portable chassis metadata is not authenticated physical-verification evidence.";
    }
    return profile;
}
}
