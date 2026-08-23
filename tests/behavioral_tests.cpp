#include "lap/chassis_profile.h"
#include "lap/port_power.h"
#include "lap/scoring.h"
#include "lap/stress.h"
#include <iostream>

namespace {
int failures = 0;

void Expect(bool condition, const char* behavior) {
    if (!condition) {
        std::cerr << "FAIL " << behavior << '\n';
        ++failures;
    } else {
        std::cout << "PASS " << behavior << '\n';
    }
}

lap::AuditReport CompletedAutomaticReport() {
    lap::AuditReport report{};
    lap::StressStageResult cpu{};
    cpu.name = L"CPU sustained load";
    cpu.verdict = lap::TestVerdict::Pass;
    lap::StressStageResult ram{};
    ram.name = L"RAM online integrity";
    ram.verdict = lap::TestVerdict::Warning;
    report.hardware.stress.stages = {cpu, ram};
    report.hardware.stress.functional.overall = L"PASS";
    report.hardware.stress.portPower.overall = L"PASS";
    return report;
}
}

int main() {
    lap::PortPowerSummary emptyPorts{};
    lap::RecalculatePortPowerSummary(emptyPorts);
    Expect(emptyPorts.overall == L"INCOMPLETE", "zero port probes remain incomplete");

    auto report = CompletedAutomaticReport();
    report.hardware.stress.chassisProfile.profileId = L"test-profile";
    report.hardware.stress.chassisProfile.ports.push_back({L"left", L"Left USB-C", L"Left", L"USB-C", L"data", true, false, L"NOT TESTED"});
    auto decision = lap::BuildAuditDecision(report);
    Expect(decision.overall == L"INCOMPLETE", "required untested port blocks BUY");

    report.hardware.stress.chassisProfile.ports.front().tested = true;
    report.hardware.stress.chassisProfile.ports.front().verdict = L"PASS";
    report.hardware.stress.runtimeValidation.failed = 1;
    report.hardware.stress.runtimeValidation.overall = L"FAIL";
    decision = lap::BuildAuditDecision(report);
    Expect(decision.overall == L"INCOMPLETE", "runtime validation failure blocks BUY");

    const auto quick = lap::MakeStressPlan(L"Quick");
    const auto deep = lap::MakeStressPlan(L"Deep");
    Expect(quick.gpuSeconds == 30 && deep.gpuSeconds == 600, "stress plans expose mode-specific GPU duration");
    return failures == 0 ? 0 : 1;
}
