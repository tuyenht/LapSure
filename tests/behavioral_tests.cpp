#include "lap/chassis_profile.h"
#include "lap/port_power.h"
#include "lap/scoring.h"
#include "lap/stress.h"
#include "lap/engines.h"
#include "lap/report.h"
#include "lap/process.h"
#include "lap/environment.h"
#include "lap/inventory.h"
#include "lap/forensics.h"
#include "lap/functional.h"
#include "lap/port_power.h"
#include "lap/runtime_validation.h"
#include "lap/orchestrator.h"
#include "lap/acquisition.h"
#include <filesystem>
#include <iostream>
#include <atomic>

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
    report.model=L"Test Laptop";report.serviceTag=L"TEST123";report.hardware.cpuName=L"Test CPU";report.hardware.cpuThreads=8;
    report.sellerClaim.provided=true;report.sellerClaim.model=L"Test Laptop";report.sellerClaim.cpuContains=L"Test CPU";report.sellerClaim.ramBytes=16ULL*1024*1024*1024;report.sellerClaim.storageBytes=512ULL*1000*1000*1000;
    report.hardware.installedRamBytes=16ULL*1024*1024*1024;lap::MemoryModule module{};module.capacityBytes=report.hardware.installedRamBytes;report.hardware.memoryModules.push_back(module);
    lap::StorageDevice disk{};disk.model=L"Test NVMe";disk.capacityBytes=512ULL*1000*1000*1000;disk.reliabilityReadable=true;disk.reliabilityHealthy=true;report.hardware.storage.push_back(disk);
    lap::GpuInfo gpu{};gpu.name=L"Test GPU";report.hardware.gpus.push_back(gpu);lap::DisplayInfo display{};display.friendlyName=L"Internal panel";report.hardware.displays.push_back(display);
    lap::StressStageResult cpu{};
    cpu.name = L"CPU sustained load";
    cpu.verdict = lap::TestVerdict::Pass;
    cpu.telemetrySummary.maxCpuPackageTempC=80;
    lap::StressStageResult ram{};
    ram.name = L"RAM online integrity";
    ram.verdict = lap::TestVerdict::Warning;
    report.hardware.stress.stages = {cpu, ram};
    report.hardware.stress.functional.overall = L"PASS";
    report.hardware.stress.functional.items.push_back({L"camera",L"Camera",lap::FunctionalStatus::Pass,L"Captured",L"Native sample",lap::Confidence::High,true});
    for(const auto*id:{L"physical_chassis",L"physical_hinge",L"physical_tamper",L"physical_liquid",L"physical_battery",L"physical_charger"})report.hardware.stress.functional.items.push_back({id,L"Physical inspection",lap::FunctionalStatus::Pass,L"No risk observed",L"Guided operator confirmation",lap::Confidence::Medium,false});
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
    report.hardware.stress.runtimeValidation.failed=0;report.hardware.stress.runtimeValidation.overall=L"PASS";report.hardware.stress.chassisProfile.validationStatus=L"draft";
    decision=lap::BuildAuditDecision(report);Expect(decision.overall==L"INCOMPLETE","draft chassis profile cannot issue BUY");
    report.hardware.stress.chassisProfile.validationStatus=L"physical-verified";decision=lap::BuildAuditDecision(report);Expect(decision.overall==L"BUY","verified complete evidence can issue BUY");
    auto missingPhysical=report;missingPhysical.hardware.stress.functional.items.erase(missingPhysical.hardware.stress.functional.items.begin()+1,missingPhysical.hardware.stress.functional.items.end());decision=lap::BuildAuditDecision(missingPhysical);Expect(decision.overall==L"INCOMPLETE","missing physical-condition evidence blocks BUY");

    const auto quick = lap::MakeStressPlan(L"Quick");
    const auto deep = lap::MakeStressPlan(L"Deep");
    Expect(quick.gpuSeconds == 30 && deep.gpuSeconds == 600, "stress plans expose mode-specific GPU duration");
    Expect(lap::HasNewEventRecord(100,101)&&!lap::HasNewEventRecord(100,100)&&!lap::HasNewEventRecord(100,0),"event attribution uses monotonic record checkpoints");

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

    const auto appDir=std::filesystem::current_path().wstring();
    const auto caps=lap::DetectCapabilities(appDir);
    std::atomic_bool cancel{false};
    lap::FactoryProfile genericProfile{};
    auto providerReport=lap::CollectInventory(genericProfile,caps,appDir,&cancel);
    lap::CollectWindowsStorageReliability(providerReport,caps,&cancel);
    lap::CollectPlatformForensics(providerReport,genericProfile,caps,appDir,&cancel);
    lap::CollectFunctionalPresence(providerReport,caps,&cancel);
    lap::CollectPortPowerBaseline(providerReport);
    lap::RunRuntimeValidation(providerReport,caps,appDir);
    lap::BuildOrchestrator(providerReport,false,true);
    providerReport.hardware.stress.decision=lap::BuildAuditDecision(providerReport);
    Expect(!providerReport.environment.empty(),"native provider smoke identifies the Windows environment");
    Expect(!providerReport.findings.empty(),"native provider smoke emits hardware evidence without stress");
    Expect(!providerReport.hardware.stress.runtimeValidation.overall.empty(),"native provider smoke completes the runtime gate");
    const auto providerDir=(std::filesystem::temp_directory_path()/L"lapsure-provider-smoke").wstring();
    const auto providerJson=lap::SaveJsonReport(providerReport,providerDir);
    const auto providerParse=lap::RunProcessCapture(L"powershell.exe -NoProfile -NonInteractive -Command \"Get-Content -Raw -LiteralPath '"+providerJson+L"' -Encoding UTF8 | ConvertFrom-Json | Out-Null\"",15000,nullptr);
    Expect(providerParse.launched&&!providerParse.timedOut&&providerParse.exitCode==0,"native provider evidence serializes as valid JSON");
    lap::MemoryModule memoryFixture{};
    Expect(lap::ParseMemoryModuleLine(L"17179869184|5600|5600|Micron|MTC8C1084S1SC48BA1|12345678",memoryFixture)&&memoryFixture.capacityBytes==17179869184ULL&&memoryFixture.deviceLocator.empty(),"RAM parser accepts optional empty locator fields");
    lap::BatteryInfo batteryFixture{};
    Expect(lap::ParseBatteryLine(L"54002|42112|225|BYD|2656|DELL KDM9P4C3",batteryFixture)&&batteryFixture.capacityReadable&&batteryFixture.designWh>54.0&&batteryFixture.fullChargeWh>42.1&&batteryFixture.cycleCount==225,"battery report capacity schema parses explicit evidence");
    lap::StorageDevice reliabilityFixture{};
    Expect(lap::ParseWindowsStorageReliabilityLine(L"PM9C1a Samsung 512GB|002538A741BB3C4E|Healthy|OK|45|83|0|-1|-1|-1",reliabilityFixture)&&reliabilityFixture.reliabilityReadable&&reliabilityFixture.reliabilityHealthy&&reliabilityFixture.temperatureC==45&&reliabilityFixture.percentageUsed==0,"Windows native storage reliability schema parses health evidence");
    lap::AuditReport claimMismatch{};claimMismatch.model=L"Actual Model";claimMismatch.hardware.cpuName=L"Core i5";claimMismatch.hardware.installedRamBytes=8ULL*1024*1024*1024;lap::StorageDevice actualDisk{};actualDisk.capacityBytes=256ULL*1000*1000*1000;claimMismatch.hardware.storage.push_back(actualDisk);claimMismatch.sellerClaim={true,L"Advertised Model",L"Core i7",16ULL*1024*1024*1024,L"",512ULL*1000*1000*1000,0,0,0,0};lap::ApplySellerClaimComparison(claimMismatch);
    Expect(claimMismatch.findings.size()==4&&lap::BuildAuditDecision(claimMismatch).overall==L"REJECT","seller claim mismatch creates critical evidence and rejects purchase");
    providerReport.hardware.stress.decision=lap::BuildAuditDecision(providerReport);
    const auto coverage=lap::BuildCoverageContract(providerReport);
    Expect(coverage.size()>=10&&std::any_of(coverage.begin(),coverage.end(),[](const auto&x){return x.id==L"storage";})&&providerReport.hardware.stress.decision.coverage==L"PARTIAL","coverage contract exposes required domains and gates incomplete evidence");
    std::filesystem::remove_all(providerDir,cleanupError);
    return failures == 0 ? 0 : 1;
}
