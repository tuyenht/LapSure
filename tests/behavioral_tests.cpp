#include "lap/chassis_profile.h"
#include "lap/port_power.h"
#include "lap/scoring.h"
#include "lap/stress.h"
#include "lap/engines.h"
#include "lap/report.h"
#include "lap/process.h"
#include <filesystem>
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

    auto vram = lap::ParseMemtestVulkanOutput(L"Standard 5-minute test PASSed\ntotal errors: 0\nwritten: 4.0GB checked: 4.0GB 10.0GB/sec");
    Expect(vram.standardFiveMinutePassed && vram.errors == 0 && vram.checkedGB == 4.0, "VRAM parser accepts explicit zero-error completed output");
    vram = lap::ParseMemtestVulkanOutput(L"total errors: 3");
    Expect(vram.errors == 3 && !vram.standardFiveMinutePassed, "VRAM parser preserves numeric error count");

    lap::StorageDevice storage{};std::wstring parseError;
    Expect(!lap::ParseSmartctlHealthJson(L"{\"model_name\":\"Disk\"}",storage,parseError), "SMART without health verdict is not readable PASS");
    Expect(lap::ParseSmartctlHealthJson(L"{\"model_name\":\"Disk\",\"passed\":true,\"critical_warning\":0,\"media_errors\":0}",storage,parseError) && storage.smartPassed, "SMART explicit healthy schema parses");

    auto reportFixture = CompletedAutomaticReport();
    reportFixture.model=L"Precision thử nghiệm";reportFixture.hardware.stress.runtimeValidation.overall=L"PASS";reportFixture.hardware.stress.orchestrator.overall=L"IN PROGRESS";
    const auto reportDir=(std::filesystem::temp_directory_path()/L"lapsure-behavior-tests").wstring();
    const auto jsonPath=lap::SaveJsonReport(reportFixture,reportDir);
    Expect(!jsonPath.empty()&&std::filesystem::file_size(jsonPath)>0,"JSON evidence report writes non-empty UTF-8 file");
    const auto parse=lap::RunProcessCapture(L"powershell.exe -NoProfile -NonInteractive -Command \"Get-Content -Raw -LiteralPath '"+jsonPath+L"' -Encoding UTF8 | ConvertFrom-Json | Out-Null\"",15000,nullptr);
    Expect(parse.launched&&!parse.timedOut&&parse.exitCode==0,"JSON evidence report parses through an independent parser");
    std::error_code cleanupError;std::filesystem::remove_all(reportDir,cleanupError);
    return failures == 0 ? 0 : 1;
}
