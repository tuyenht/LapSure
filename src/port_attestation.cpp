#include "lap/port_attestation.h"

#include <windows.h>
#include <cwchar>

namespace lap {
namespace {

bool IsCompletedPortVerdict(const std::wstring& verdict) {
    return verdict == L"PASS" || verdict == L"FAIL";
}

std::wstring CurrentUtcTimestamp() {
    SYSTEMTIME now{};
    GetSystemTime(&now);
    wchar_t buffer[32]{};
    swprintf_s(buffer,
               L"%04u-%02u-%02uT%02u:%02u:%02uZ",
               static_cast<unsigned>(now.wYear),
               static_cast<unsigned>(now.wMonth),
               static_cast<unsigned>(now.wDay),
               static_cast<unsigned>(now.wHour),
               static_cast<unsigned>(now.wMinute),
               static_cast<unsigned>(now.wSecond));
    return buffer;
}

SessionPortEvidence* FindExpectedPort(
    SessionPortAttestation& attestation,
    std::wstring_view expectedPortId) {
    if (expectedPortId.empty()) return nullptr;
    for (auto& port : attestation.ports) {
        if (port.expectedPortId == expectedPortId) return &port;
    }
    return nullptr;
}

void RefreshConfirmation(SessionPortAttestation& attestation) {
    if (attestation.operatorConfirmed) return;

    bool hasRequiredPort = false;
    for (const auto& port : attestation.ports) {
        if (!port.expectedRequired) continue;
        hasRequiredPort = true;
        if (port.observedPresence != CapabilityTruth::Present ||
            !port.tested ||
            !IsCompletedPortVerdict(port.verdict)) {
            return;
        }
    }

    if (!hasRequiredPort) return;
    attestation.operatorConfirmed = true;
    attestation.confirmedAt = CurrentUtcTimestamp();
}

} // namespace

SessionPortAttestation InitializeSessionPortAttestation(
    const std::wstring& sessionId,
    const ChassisProfile& profile) {
    SessionPortAttestation attestation{};
    attestation.sessionId = sessionId;
    attestation.ports.reserve(profile.ports.size());

    for (const auto& expected : profile.ports) {
        SessionPortEvidence evidence{};
        evidence.expectedPortId = expected.id;
        evidence.label = expected.label;
        evidence.expectedRequired = expected.required;
        attestation.ports.push_back(std::move(evidence));
    }

    return attestation;
}

void ApplyPortResultToAttestation(
    SessionPortAttestation& attestation,
    const PortProbeResult& result) {
    auto* expected = FindExpectedPort(attestation, result.expectedPortId);
    if (!expected) return;

    expected->observedPresence = CapabilityTruth::Present;
    expected->verdict = result.verdict.empty() ? L"NOT TESTED" : result.verdict;
    expected->tested = IsCompletedPortVerdict(expected->verdict);
    RefreshConfirmation(attestation);
}

void RecordPortObservation(
    SessionPortAttestation& attestation,
    std::wstring_view expectedPortId,
    CapabilityTruth observed,
    std::wstring correctionReason) {
    auto* expected = FindExpectedPort(attestation, expectedPortId);
    if (!expected) return;

    expected->observedPresence = observed;
    expected->correctionReason = std::move(correctionReason);
    if (observed == CapabilityTruth::AbsentConfirmed) {
        expected->discrepancy = L"EXPECTED_PORT_NOT_OBSERVED";
        expected->tested = false;
        expected->verdict = L"NOT TESTED";
    }
    RefreshConfirmation(attestation);
}

unsigned RequiredPortsRemaining(const SessionPortAttestation& attestation) {
    unsigned remaining = 0;
    for (const auto& port : attestation.ports) {
        if (!port.expectedRequired) continue;
        if (port.observedPresence != CapabilityTruth::Present ||
            !port.tested ||
            !IsCompletedPortVerdict(port.verdict)) {
            ++remaining;
        }
    }
    return remaining;
}

} // namespace lap
