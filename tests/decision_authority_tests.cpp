#include "lap/scoring.h"
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
} // namespace

int main() {
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
