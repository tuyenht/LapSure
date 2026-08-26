#pragma once

#include "model.h"
#include <string_view>

namespace lap {

SessionPortAttestation InitializeSessionPortAttestation(
    const std::wstring& sessionId,
    const ChassisProfile& profile);

void ApplyPortResultToAttestation(
    SessionPortAttestation& attestation,
    const PortProbeResult& result);

void RecordPortObservation(
    SessionPortAttestation& attestation,
    std::wstring_view expectedPortId,
    CapabilityTruth observed,
    std::wstring correctionReason);

unsigned RequiredPortsRemaining(
    const SessionPortAttestation& attestation);

} // namespace lap
