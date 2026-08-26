#include "lap/scoring.h"
#include "lap/port_attestation.h"
#include <algorithm>

namespace lap {
namespace {

bool IsCompletedVerdict(TestVerdict verdict) {
    return verdict != TestVerdict::NotTested && verdict != TestVerdict::Cancelled;
}

bool IsCpuLoadStage(const StressStageResult& stage) {
    return stage.name.find(L"CPU") != std::wstring::npos &&
           stage.name.find(L"load") != std::wstring::npos;
}

bool IsRamStage(const StressStageResult& stage) {
    return stage.name.find(L"RAM") != std::wstring::npos;
}

bool IsGpuStage(const StressStageResult& stage) {
    return stage.name.find(L"GPU") != std::wstring::npos ||
           stage.name.find(L"VRAM") != std::wstring::npos;
}

const RequirementDomain* FindRequirement(const RequirementSnapshot& snapshot,
                                         std::wstring_view id) {
    for (const auto& domain : snapshot.domains) {
        if (domain.id == id) return &domain;
    }
    return nullptr;
}

std::wstring ChassisAuthorityText(ChassisAuthorityLevel level) {
    switch (level) {
        case ChassisAuthorityLevel::Certified: return L"CERTIFIED";
        case ChassisAuthorityLevel::Advisory: return L"ADVISORY";
        default: return L"NONE";
    }
}

std::wstring FactoryAuthorityText(FactoryAuthorityLevel level) {
    switch (level) {
        case FactoryAuthorityLevel::Authenticated: return L"AUTHENTICATED";
        case FactoryAuthorityLevel::Advisory: return L"ADVISORY";
        default: return L"UNKNOWN";
    }
}

std::wstring CapabilityText(CapabilityTruth state) {
    switch (state) {
        case CapabilityTruth::Present: return L"PRESENT";
        case CapabilityTruth::AbsentConfirmed: return L"ABSENT_CONFIRMED";
        default: return L"UNKNOWN";
    }
}

bool HasTrustedCpuThermalSample(const AuditReport& report) {
    for (const auto& stage : report.hardware.stress.stages) {
        if (!IsCpuLoadStage(stage) || !IsCompletedVerdict(stage.verdict)) continue;
        for (const auto& sample : stage.telemetry) {
            if (sample.cpuPackageTempC >= 0.0 &&
                sample.cpuThermalConfidence != Confidence::Low &&
                !sample.cpuThermalSource.empty()) {
                return true;
            }
        }
    }
    return false;
}

bool FunctionalCoverageComplete(const AuditReport& report) {
    unsigned physicalCompleted = 0;
    bool incomplete = false;
    for (const auto& item : report.hardware.stress.functional.items) {
        if (item.status == FunctionalStatus::NotTested ||
            item.status == FunctionalStatus::ManualRequired) {
            incomplete = true;
        }
        if (item.id.rfind(L"physical_", 0) == 0 &&
            item.status != FunctionalStatus::NotTested &&
            item.status != FunctionalStatus::ManualRequired) {
            ++physicalCompleted;
        }
    }
    return !report.hardware.stress.functional.items.empty() &&
           !incomplete && physicalCompleted >= 6;
}

bool StableCoreCoverageComplete(const AuditReport& report) {
    bool cpuComplete = false;
    bool ramComplete = false;
    for (const auto& stage : report.hardware.stress.stages) {
        if (IsCpuLoadStage(stage) && IsCompletedVerdict(stage.verdict)) cpuComplete = true;
        if (IsRamStage(stage) && IsCompletedVerdict(stage.verdict)) ramComplete = true;
    }
    return cpuComplete && ramComplete;
}

bool GpuCoverageComplete(const AuditReport& report) {
    return std::any_of(report.hardware.stress.stages.begin(),
                       report.hardware.stress.stages.end(),
                       [](const StressStageResult& stage) {
                           return IsGpuStage(stage) && IsCompletedVerdict(stage.verdict);
                       });
}

bool PortsCoverageComplete(const DecisionContext& context) {
    bool anyRequired = false;
    for (const auto& port : context.portAttestation.ports) {
        if (!port.expectedRequired) continue;
        anyRequired = true;
        if (port.observedPresence != CapabilityTruth::Present ||
            !port.tested ||
            (port.verdict != L"PASS" && port.verdict != L"FAIL")) {
            return false;
        }
    }
    return anyRequired && context.portAttestation.operatorConfirmed &&
           RequiredPortsRemaining(context.portAttestation) == 0;
}

bool HasCriticalMachineFailure(const AuditReport& report) {
    if (std::any_of(report.findings.begin(), report.findings.end(),
                    [](const Finding& finding) {
                        return finding.severity == Severity::Critical &&
                               finding.state == State::Fail;
                    })) {
        return true;
    }

    if (std::any_of(report.hardware.stress.stages.begin(),
                    report.hardware.stress.stages.end(),
                    [](const StressStageResult& stage) {
                        return stage.verdict == TestVerdict::Fail;
                    })) {
        return true;
    }

    if (report.hardware.stress.functional.failed > 0 ||
        std::any_of(report.hardware.stress.functional.items.begin(),
                    report.hardware.stress.functional.items.end(),
                    [](const FunctionalItemResult& item) {
                        return item.status == FunctionalStatus::Fail;
                    })) {
        return true;
    }

    return std::any_of(report.hardware.stress.portAttestation.ports.begin(),
                       report.hardware.stress.portAttestation.ports.end(),
                       [](const SessionPortEvidence& port) {
                           return port.expectedRequired &&
                                  (port.observedPresence == CapabilityTruth::AbsentConfirmed ||
                                   (port.tested && port.verdict == L"FAIL"));
                       });
}

unsigned CountWarnings(const AuditReport& report) {
    unsigned count = 0;
    for (const auto& finding : report.findings) {
        if (finding.state == State::Warning) ++count;
    }
    for (const auto& stage : report.hardware.stress.stages) {
        if (stage.verdict == TestVerdict::Warning) ++count;
    }
    for (const auto& item : report.hardware.stress.functional.items) {
        if (item.status == FunctionalStatus::Warning) ++count;
    }
    return count;
}

unsigned CountCriticalNotTested(const AuditReport& report) {
    return static_cast<unsigned>(std::count_if(
        report.findings.begin(), report.findings.end(),
        [](const Finding& finding) {
            return finding.severity == Severity::Critical &&
                   finding.state == State::NotTested;
        }));
}

void AddCoverageDomain(std::vector<CoverageDomain>& out,
                       const DecisionContext& context,
                       const wchar_t* id,
                       const wchar_t* name,
                       bool complete,
                       const wchar_t* sources,
                       const wchar_t* missing) {
    const auto disposition = context.requirements.StateOf(id);
    if (disposition == RequirementDisposition::NotApplicable) {
        out.push_back({id, name, L"NOT APPLICABLE", false, sources, L""});
        return;
    }

    if (disposition == RequirementDisposition::ConditionalBlocked) {
        const auto* requirement = FindRequirement(context.requirements, id);
        out.push_back({id,
                       name,
                       L"PARTIAL",
                       true,
                       sources,
                       requirement && !requirement->reason.empty()
                           ? requirement->reason
                           : L"Requirement is blocked because capability/authority state is unresolved."});
        return;
    }

    out.push_back({id,
                   name,
                   complete ? L"COMPLETE" : L"PARTIAL",
                   true,
                   sources,
                   complete ? L"" : missing});
}

} // namespace

const wchar_t* ConfidenceText(Confidence c) {
    switch (c) {
        case Confidence::High: return L"HIGH";
        case Confidence::Medium: return L"MEDIUM";
        default: return L"LOW";
    }
}

const wchar_t* VerdictText(TestVerdict v) {
    switch (v) {
        case TestVerdict::Pass: return L"PASS";
        case TestVerdict::Warning: return L"WARNING";
        case TestVerdict::Fail: return L"FAIL";
        case TestVerdict::Cancelled: return L"CANCELLED";
        default: return L"NOT TESTED";
    }
}

TelemetrySummary SummarizeTelemetry(const std::vector<TelemetrySample>& samples) {
    TelemetrySummary summary{};
    summary.sampleCount = static_cast<unsigned>(samples.size());
    if (samples.empty()) return summary;

    double cpu = 0, gpuTemp = 0, gpuPower = 0, gpuUtil = 0, cpuTemp = 0, cpuPower = 0;
    unsigned cpuCount = 0, gpuTempCount = 0, gpuPowerCount = 0, gpuUtilCount = 0;
    unsigned cpuTempCount = 0, cpuPowerCount = 0;
    for (const auto& sample : samples) {
        if (sample.cpuUtilPercent >= 0) {
            cpu += sample.cpuUtilPercent;
            ++cpuCount;
            summary.maxCpuUtil = std::max(summary.maxCpuUtil, sample.cpuUtilPercent);
        }
        if (sample.gpuTempC >= 0) {
            gpuTemp += sample.gpuTempC;
            ++gpuTempCount;
            summary.maxGpuTempC = std::max(summary.maxGpuTempC, sample.gpuTempC);
        }
        if (sample.gpuPowerW >= 0) {
            gpuPower += sample.gpuPowerW;
            ++gpuPowerCount;
            summary.maxGpuPowerW = std::max(summary.maxGpuPowerW, sample.gpuPowerW);
        }
        if (sample.gpuUtilPercent >= 0) {
            gpuUtil += sample.gpuUtilPercent;
            ++gpuUtilCount;
            summary.maxGpuUtil = std::max(summary.maxGpuUtil, sample.gpuUtilPercent);
        }
        if (sample.cpuPackageTempC >= 0) {
            cpuTemp += sample.cpuPackageTempC;
            ++cpuTempCount;
            summary.maxCpuPackageTempC = std::max(summary.maxCpuPackageTempC, sample.cpuPackageTempC);
            summary.cpuThermalConfidence = sample.cpuThermalConfidence;
        }
        if (sample.cpuPackagePowerW >= 0) {
            cpuPower += sample.cpuPackagePowerW;
            ++cpuPowerCount;
            summary.maxCpuPackagePowerW = std::max(summary.maxCpuPackagePowerW, sample.cpuPackagePowerW);
        }
        if (sample.cpuThermalThrottle > 0) summary.cpuThrottleObserved = true;
    }

    summary.avgCpuUtil = cpuCount ? cpu / cpuCount : -1;
    summary.avgGpuTempC = gpuTempCount ? gpuTemp / gpuTempCount : -1;
    summary.avgGpuPowerW = gpuPowerCount ? gpuPower / gpuPowerCount : -1;
    summary.avgGpuUtil = gpuUtilCount ? gpuUtil / gpuUtilCount : -1;
    summary.avgCpuPackageTempC = cpuTempCount ? cpuTemp / cpuTempCount : -1;
    summary.avgCpuPackagePowerW = cpuPowerCount ? cpuPower / cpuPowerCount : -1;
    return summary;
}

void AssessStressStage(StressStageResult& stage) {
    stage.telemetrySummary = SummarizeTelemetry(stage.telemetry);
    if (IsGpuStage(stage) && stage.telemetrySummary.maxGpuTempC >= 0) {
        stage.evidence += L"; GPU max temperature=" +
                          std::to_wstring(static_cast<int>(stage.telemetrySummary.maxGpuTempC)) + L"C";
        if (stage.telemetrySummary.maxGpuTempC >= 90 && stage.verdict == TestVerdict::Pass) {
            stage.verdict = TestVerdict::Warning;
        }
    }
}

std::vector<CoverageDomain> BuildCoverageContract(const AuditReport& report,
                                                  const DecisionContext& context) {
    std::vector<CoverageDomain> out;

    const auto& claim = report.sellerClaim;
    const bool claimComplete = claim.provided && !claim.model.empty() &&
                               !claim.cpuContains.empty() && claim.ramBytes > 0 &&
                               claim.storageBytes > 0;
    out.push_back({L"seller_claim",
                   L"Cấu hình người bán cam kết",
                   claimComplete ? L"COMPLETE" : L"PARTIAL",
                   false,
                   L"Biểu mẫu người bán + đối chiếu bằng chứng LapSure",
                   claimComplete ? L"" : L"Thiếu model, CPU, RAM hoặc dung lượng ổ do người bán cam kết"});

    AddCoverageDomain(out,
                      context,
                      L"identity",
                      L"Thông tin nhận diện máy",
                      !report.model.empty() && !report.serviceTag.empty() && !report.hardware.cpuName.empty(),
                      L"BIOS registry + CIM",
                      L"Thiếu tên máy, Service Tag hoặc thông tin bộ xử lý");

    AddCoverageDomain(out,
                      context,
                      L"memory",
                      L"Bộ nhớ RAM",
                      report.hardware.installedRamBytes > 0 && !report.hardware.memoryModules.empty(),
                      L"GlobalMemoryStatusEx + CIM",
                      L"Thiếu dung lượng RAM hoặc thông tin thanh RAM vật lý");

    const bool storageComplete = !report.hardware.storage.empty() &&
        std::all_of(report.hardware.storage.begin(), report.hardware.storage.end(),
                    [](const StorageDevice& disk) {
                        return !disk.model.empty() && disk.capacityBytes > 0 && disk.reliabilityReadable;
                    });
    AddCoverageDomain(out,
                      context,
                      L"storage",
                      L"Ổ lưu trữ và sức khỏe ổ",
                      storageComplete,
                      L"CIM + Windows Storage Reliability; SMART là dữ liệu bổ sung",
                      L"Có ổ đĩa thiếu nhận diện, dung lượng hoặc bằng chứng sức khỏe");

    const bool batteryComplete = !report.hardware.battery.present ||
        (report.hardware.battery.capacityReadable && report.hardware.battery.healthPercent >= 0);
    AddCoverageDomain(out,
                      context,
                      L"battery",
                      L"Pin",
                      batteryComplete,
                      L"CIM + báo cáo pin Windows",
                      L"Thiếu dung lượng hoặc sức khỏe pin");

    const bool graphicsComplete = report.hardware.gpuInventoryStatus == ProviderCollectionStatus::Complete &&
                                  !report.hardware.gpus.empty();
    AddCoverageDomain(out,
                      context,
                      L"graphics",
                      L"Bộ xử lý đồ họa",
                      graphicsComplete,
                      L"CIM; dữ liệu hãng là phần bổ sung",
                      L"Chưa có inventory GPU hoàn chỉnh để xác lập capability truth");

    AddCoverageDomain(out,
                      context,
                      L"display",
                      L"Màn hình",
                      !report.hardware.displays.empty(),
                      L"EDID gốc + cấu hình hiển thị",
                      L"Chưa có bằng chứng EDID/màn hình hợp lệ");

    AddCoverageDomain(out,
                      context,
                      L"stability",
                      L"Độ ổn định CPU và RAM",
                      StableCoreCoverageComplete(report),
                      L"Các bài tải LapSure + sự kiện phát sinh",
                      L"CPU hoặc RAM load stage bắt buộc chưa hoàn tất");

    AddCoverageDomain(out,
                      context,
                      L"gpu_vram",
                      L"GPU rời / VRAM",
                      GpuCoverageComplete(report),
                      L"GPU/VRAM stress provider + capability inventory",
                      L"dGPU hiện diện/được cam kết nhưng chưa có GPU/VRAM stage hoàn tất");

    AddCoverageDomain(out,
                      context,
                      L"thermals",
                      L"Nhiệt độ và giảm hiệu năng do nóng",
                      HasTrustedCpuThermalSample(report),
                      L"Cảm biến CPU package tin cậy trong khi chạy tải",
                      L"CPU load đã chạy nhưng chưa có mẫu CPU package thermal tin cậy");

    AddCoverageDomain(out,
                      context,
                      L"functional",
                      L"Các thiết bị, ngoại hình và chức năng",
                      FunctionalCoverageComplete(report),
                      L"Kiểm tra tự động + hướng dẫn người dùng",
                      L"Còn chức năng hoặc kiểm tra ngoại hình/an toàn vật lý chưa hoàn tất");

    AddCoverageDomain(out,
                      context,
                      L"ports_power",
                      L"Cổng kết nối và nguồn sạc",
                      PortsCoverageComplete(context),
                      L"SessionPortAttestation stable-ID + cắm thử thực tế",
                      L"Còn expected required port chưa có attestation hoàn chỉnh trong session hiện tại");

    const bool runtimeComplete = report.hardware.stress.runtimeValidation.failed == 0 &&
                                 report.hardware.stress.runtimeValidation.notRun == 0 &&
                                 report.hardware.stress.runtimeValidation.overall == L"PASS";
    AddCoverageDomain(out,
                      context,
                      L"runtime",
                      L"Tính toàn vẹn chương trình và báo cáo",
                      runtimeComplete,
                      L"Cổng xác thực nội bộ LapSure",
                      L"Xác thực runtime/self-integrity chưa đạt");

    return out;
}

std::vector<CoverageDomain> BuildCoverageContract(const AuditReport& report) {
    const auto context = BuildDecisionContext(report);
    return BuildCoverageContract(report, context);
}

AuditDecision BuildAuditDecision(const AuditReport& report,
                                 const DecisionContext& context) {
    AuditDecision decision{};
    decision.decisionPolicyVersion = context.requirements.versions.decision;
    decision.coveragePolicyVersion = context.requirements.versions.coverage;
    decision.authorityPolicyVersion = context.requirements.versions.authority;
    decision.chassisAuthority = ChassisAuthorityText(context.profile.chassisAuthority.Level());
    decision.factoryAuthority = FactoryAuthorityText(context.profile.factoryAuthority);
    decision.discreteGpuCapability = CapabilityText(context.capabilities.discreteGpu.state);
    decision.coverageDomains = BuildCoverageContract(report, context);
    decision.factory = decision.factoryAuthority;
    decision.performance = report.hardware.stress.cpuBenchmark.verdict;
    decision.warnings = CountWarnings(report);
    decision.criticalNotTested = CountCriticalNotTested(report);
    decision.criticalFails = static_cast<unsigned>(std::count_if(
        report.findings.begin(), report.findings.end(),
        [](const Finding& finding) {
            return finding.severity == Severity::Critical && finding.state == State::Fail;
        }));

    bool stageFailure = false;
    bool anyCoreStage = false;
    bool coreComplete = true;
    double maxCpu = -1;
    double maxGpu = -1;
    bool cpuThrottle = false;
    for (const auto& stage : report.hardware.stress.stages) {
        if (IsCpuLoadStage(stage) || IsRamStage(stage)) {
            anyCoreStage = true;
            if (!IsCompletedVerdict(stage.verdict)) coreComplete = false;
        }
        if (stage.verdict == TestVerdict::Fail) stageFailure = true;
        maxCpu = std::max(maxCpu, stage.telemetrySummary.maxCpuPackageTempC);
        maxGpu = std::max(maxGpu, stage.telemetrySummary.maxGpuTempC);
        cpuThrottle = cpuThrottle || stage.telemetrySummary.cpuThrottleObserved;
    }
    decision.stability = stageFailure ? L"FAIL" :
                         (anyCoreStage && coreComplete ? L"PASS" :
                          (anyCoreStage ? L"PARTIAL" : L"NOT TESTED"));

    if (maxCpu >= 0) {
        if (cpuThrottle || maxCpu >= 100) {
            decision.thermal = L"CPU THERMAL / THROTTLE REVIEW";
            decision.reasons.push_back(L"Trusted CPU telemetry reported severe temperature or throttling.");
        } else if (maxCpu >= 95) {
            decision.thermal = L"CPU HOT / REVIEW";
            decision.reasons.push_back(L"Trusted CPU package telemetry reached >=95C.");
        } else {
            decision.thermal = L"CPU TELEMETRY OK";
        }
    } else if (maxGpu >= 90) {
        decision.thermal = L"GPU HOT / REVIEW; CPU THERMAL UNKNOWN";
    } else if (maxGpu >= 0) {
        decision.thermal = L"GPU TELEMETRY OK; CPU THERMAL UNKNOWN";
    } else {
        decision.thermal = L"UNKNOWN (NO RELIABLE THERMAL PROVIDER)";
    }

    const auto missingRequired = static_cast<unsigned>(std::count_if(
        decision.coverageDomains.begin(), decision.coverageDomains.end(),
        [](const CoverageDomain& domain) {
            return domain.required && domain.status != L"COMPLETE";
        }));
    decision.coverage = missingRequired == 0 ? L"HIGH" : L"PARTIAL";

    if (HasCriticalMachineFailure(report)) {
        decision.overall = L"REJECT";
        decision.stability = stageFailure ? L"FAIL" : decision.stability;
        decision.confidence = Confidence::High;
        decision.reasons.push_back(L"Critical trusted machine or seller-claim failure detected.");
        return decision;
    }

    if (report.hardware.stress.runtimeValidation.failed > 0) {
        decision.overall = L"INCOMPLETE";
        decision.confidence = Confidence::Low;
        decision.reasons.push_back(L"Runtime validation failed; this build cannot issue an acceptance verdict.");
        return decision;
    }

    if (decision.criticalNotTested > 0 || missingRequired > 0) {
        decision.overall = L"INCOMPLETE";
        decision.confidence = Confidence::Medium;
        if (decision.criticalNotTested > 0) {
            decision.reasons.push_back(L"Critical evidence remains untested.");
        }
        if (missingRequired > 0) {
            decision.reasons.push_back(
                std::to_wstring(missingRequired) +
                L" required or conditional-blocked coverage domain(s) remain incomplete; see the frozen coverage contract.");
        }
        return decision;
    }

    switch (context.profile.chassisAuthority.Level()) {
        case ChassisAuthorityLevel::Certified:
            decision.overall = L"BUY";
            decision.confidence = Confidence::High;
            break;
        case ChassisAuthorityLevel::Advisory:
            decision.overall = L"BUY WITH NOTES";
            decision.confidence = Confidence::High;
            decision.reasons.push_back(L"Chassis authority is advisory rather than protected Certified authority.");
            break;
        default:
            decision.overall = L"INCOMPLETE";
            decision.confidence = Confidence::Medium;
            decision.reasons.push_back(L"No decision-grade chassis authority is available.");
            return decision;
    }

    if (decision.warnings > 0 && decision.overall == L"BUY") {
        decision.overall = L"BUY WITH NOTES";
        decision.reasons.push_back(L"Non-critical warnings require review before purchase.");
    }
    if (report.hardware.stress.cpuBenchmark.verdict == L"BELOW BASELINE") {
        decision.reasons.push_back(L"CPU microbenchmark is below the validated local baseline for this benchmark version.");
        if (decision.overall == L"BUY") decision.overall = L"BUY WITH NOTES";
    }
    return decision;
}

AuditDecision BuildAuditDecision(const AuditReport& report) {
    const auto context = BuildDecisionContext(report);
    return BuildAuditDecision(report, context);
}

} // namespace lap
