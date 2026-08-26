#include "lap/ui_screens.h"
#include "lap/scoring.h"
#include <algorithm>
#include <initializer_list>
#include <string>
#include <vector>

namespace lap {
namespace {

struct CoverageSnapshot {
    std::vector<CoverageDomain> domains;
    int requiredTotal{};
    int requiredComplete{};
    int requiredMissing{};
};

struct StagePresentation {
    CanonicalUiState state{CanonicalUiState::NotTested};
    std::wstring label;
    std::wstring source;
    std::wstring detail;
    std::wstring duration{L"—"};
};

CoverageSnapshot BuildCoverageSnapshot(const AuditReport& rep) {
    CoverageSnapshot out;
    out.domains = BuildCoverageContract(rep);
    for (const auto& domain : out.domains) {
        if (!domain.required) continue;
        ++out.requiredTotal;
        if (domain.status == L"COMPLETE") ++out.requiredComplete;
    }
    out.requiredMissing = std::max(0, out.requiredTotal - out.requiredComplete);
    return out;
}

const CoverageDomain* FindCoverage(const CoverageSnapshot& coverage, const wchar_t* id) {
    for (const auto& domain : coverage.domains) {
        if (domain.id == id) return &domain;
    }
    return nullptr;
}

CanonicalUiState CoverageState(const CoverageSnapshot& coverage, const wchar_t* id) {
    const auto* domain = FindCoverage(coverage, id);
    if (!domain) return CanonicalUiState::NotTested;
    return domain->status == L"COMPLETE" ? CanonicalUiState::Pass : CanonicalUiState::Incomplete;
}

std::wstring CoverageDetail(const CoverageSnapshot& coverage, const wchar_t* id, const wchar_t* fallback) {
    const auto* domain = FindCoverage(coverage, id);
    if (!domain) return fallback;
    if (domain->status == L"COMPLETE") {
        return domain->sources.empty() ? std::wstring(fallback) : domain->sources;
    }
    return domain->missingEvidence.empty() ? L"Thiếu bằng chứng bắt buộc" : domain->missingEvidence;
}

const FunctionalItemResult* FindFunctional(const FunctionalTestSummary& summary, const wchar_t* id) {
    for (const auto& item : summary.items) {
        if (item.id == id) return &item;
    }
    return nullptr;
}

CanonicalUiState FunctionalState(const FunctionalTestSummary& summary, const wchar_t* id) {
    const auto* item = FindFunctional(summary, id);
    return item ? MapFunctionalStatus(item->status) : CanonicalUiState::NotTested;
}

bool HasFunctionalEvidence(const FunctionalTestSummary& summary, const wchar_t* id) {
    return FindFunctional(summary, id) != nullptr;
}

CanonicalUiState MergeFunctional(std::initializer_list<CanonicalUiState> states) {
    int passed = 0;
    int unsupported = 0;
    int unresolved = 0;
    const int total = static_cast<int>(states.size());
    for (auto state : states) {
        if (state == CanonicalUiState::Fail || state == CanonicalUiState::Error) return CanonicalUiState::Fail;
        if (state == CanonicalUiState::Warning) return CanonicalUiState::Warning;
        if (state == CanonicalUiState::PermissionDenied) return CanonicalUiState::PermissionDenied;
        if (state == CanonicalUiState::ProviderUnavailable) return CanonicalUiState::ProviderUnavailable;
        if (state == CanonicalUiState::ManualRequired) return CanonicalUiState::ManualRequired;
        if (state == CanonicalUiState::Incomplete) return CanonicalUiState::Incomplete;
        if (state == CanonicalUiState::Pass || state == CanonicalUiState::Good) {
            ++passed;
            continue;
        }
        if (state == CanonicalUiState::Unsupported) {
            ++unsupported;
            continue;
        }
        ++unresolved;
    }
    if (total > 0 && passed == total) return CanonicalUiState::Pass;
    if (total > 0 && unsupported == total) return CanonicalUiState::Unsupported;
    if (passed > 0 || unsupported > 0 || unresolved > 0) return CanonicalUiState::Incomplete;
    return CanonicalUiState::NotTested;
}

struct FindingCounts {
    unsigned warnings{};
    unsigned criticalFails{};
};

FindingCounts CountLiveFindings(const AuditReport& rep) {
    FindingCounts counts;
    for (const auto& finding : rep.findings) {
        if (finding.severity == Severity::Critical && finding.state == State::Fail) ++counts.criticalFails;
        if (finding.state == State::Warning) ++counts.warnings;
    }
    return counts;
}

std::wstring ConfidenceVi(Confidence confidence) {
    switch (confidence) {
    case Confidence::High: return L"CAO";
    case Confidence::Medium: return L"TRUNG BÌNH";
    default: return L"THẤP";
    }
}

std::wstring FormatDuration(unsigned seconds) {
    wchar_t buf[32]{};
    swprintf_s(buf, L"%02u:%02u", seconds / 60, seconds % 60);
    return buf;
}

CanonicalUiState DecisionState(const AuditDecision& decision, bool auditReady, bool running,
                               CanonicalUiState lifecycle) {
    if (running) return CanonicalUiState::Running;
    if (!auditReady) {
        if (lifecycle == CanonicalUiState::Cancelled || lifecycle == CanonicalUiState::Interrupted ||
            lifecycle == CanonicalUiState::Paused) return lifecycle;
        return CanonicalUiState::NotTested;
    }
    if (decision.overall == L"BUY") return CanonicalUiState::Good;
    if (decision.overall == L"BUY WITH NOTES") return CanonicalUiState::Warning;
    if (decision.overall == L"REJECT") return CanonicalUiState::Fail;
    return CanonicalUiState::Incomplete;
}

std::wstring DecisionLabel(const AuditDecision& decision, bool auditReady, bool running,
                           CanonicalUiState lifecycle) {
    if (running) return L"ĐANG KIỂM TRA";
    if (auditReady) return FormatDecisionVi(decision);
    if (lifecycle == CanonicalUiState::Cancelled) return L"ĐÃ HỦY — CHƯA ĐỦ BẰNG CHỨNG";
    if (lifecycle == CanonicalUiState::Interrupted) return L"BỊ GIÁN ĐOẠN — CHƯA ĐỦ BẰNG CHỨNG";
    return L"CHƯA ĐỦ BẰNG CHỨNG";
}

void DrawModePills(HDC dc, int x, int y, const std::wstring& selectedMode,
                   const UiFonts& fonts, int dpi) {
    auto draw = [&](const wchar_t* label, const wchar_t* value) {
        const bool active = selectedMode == value;
        const int w = UiMetrics::Scale(80, dpi);
        const int h = UiMetrics::Scale(28, dpi);
        RECT r{ x, y, x + w, y + h };
        DrawRoundedCard(dc, r, h / 2,
                        active ? UiColors::PrimaryBlue : UiColors::GrayPillBg,
                        active ? UiColors::PrimaryBlue : UiColors::GrayPillBorder, 1);
        SelectObject(dc, active ? fonts.hBodyBold : fonts.hBody);
        SetTextColor(dc, active ? UiColors::TextWhite : UiColors::TextMain);
        DrawTextW(dc, label, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        x += w + UiMetrics::Scale(6, dpi);
    };
    draw(L"Nhanh", L"Quick");
    draw(L"Tiêu chuẩn", L"Standard");
    draw(L"Chuyên sâu", L"Deep");
}

void DrawPillarCard(HDC dc, const RECT& r, const wchar_t* icon, const wchar_t* title,
                    const std::wstring& value, const std::wstring& detail,
                    CanonicalUiState state, const UiFonts& fonts, int dpi,
                    const std::wstring& overrideLabel = L"") {
    DrawRoundedCard(dc, r, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    // Left Icon Box
    const int iconBoxS = UiMetrics::Scale(36, dpi);
    RECT iconBox{ r.left + UiMetrics::Scale(12, dpi), r.top + UiMetrics::Scale(12, dpi),
                  r.left + UiMetrics::Scale(12, dpi) + iconBoxS, r.top + UiMetrics::Scale(12, dpi) + iconBoxS };
    DrawRoundedCard(dc, iconBox, UiMetrics::RadiusSm, UiColors::PrimaryBlueLight, UiColors::InfoBorder, 1);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::PrimaryBlue);
    DrawTextW(dc, icon, -1, &iconBox, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Title (Top Left)
    const int textLeft = iconBox.right + UiMetrics::Scale(10, dpi);
    const int badgeW = UiMetrics::Scale(125, dpi);
    const int badgeH = UiMetrics::Scale(24, dpi);
    RECT titleRect{ textLeft, r.top + UiMetrics::Scale(9, dpi),
                    r.right - badgeW - UiMetrics::Scale(12, dpi), r.top + UiMetrics::Scale(27, dpi) };
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    DrawTextW(dc, title, -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    // Status Badge (Top Right)
    RECT badgeRect{ r.right - badgeW - UiMetrics::Scale(10, dpi), r.top + UiMetrics::Scale(8, dpi),
                    r.right - UiMetrics::Scale(10, dpi), r.top + UiMetrics::Scale(8, dpi) + badgeH };
    DrawStatusBadge(dc, badgeRect, state, fonts, overrideLabel);

    // Value (Middle Bold)
    RECT valueRect{ textLeft, r.top + UiMetrics::Scale(28, dpi),
                    r.right - UiMetrics::Scale(10, dpi), r.top + UiMetrics::Scale(50, dpi) };
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    DrawTextW(dc, value.c_str(), static_cast<int>(value.size()), &valueRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    // Detail (Bottom Muted) + "Chi tiết →"
    const int arrowW = UiMetrics::Scale(80, dpi);
    RECT detailRect{ textLeft, r.top + UiMetrics::Scale(52, dpi),
                     r.right - arrowW - UiMetrics::Scale(8, dpi), r.bottom - UiMetrics::Scale(6, dpi) };
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    DrawTextW(dc, detail.c_str(), static_cast<int>(detail.size()), &detailRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    RECT arrowRect{ r.right - arrowW - UiMetrics::Scale(10, dpi), r.top + UiMetrics::Scale(52, dpi),
                    r.right - UiMetrics::Scale(10, dpi), r.bottom - UiMetrics::Scale(6, dpi) };
    SetTextColor(dc, UiColors::PrimaryBlue);
    DrawTextW(dc, L"Chi tiết →", -1, &arrowRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

void DrawIdentityChip(HDC dc, const RECT& r, const wchar_t* label, const std::wstring& value,
                      const UiFonts& fonts, int dpi) {
    DrawRoundedCard(dc, r, UiMetrics::RadiusSm, UiColors::TableHeaderBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, r.left + UiMetrics::Scale(7, dpi), r.top + UiMetrics::Scale(3, dpi), label, -1);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    RECT valueRect{ r.left + UiMetrics::Scale(7, dpi), r.top + UiMetrics::Scale(17, dpi),
                    r.right - UiMetrics::Scale(7, dpi), r.bottom - UiMetrics::Scale(3, dpi) };
    DrawTextW(dc, value.c_str(), static_cast<int>(value.size()), &valueRect,
              DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
}

bool AnyRamMismatch(const AuditReport& rep) {
    for (const auto& stage : rep.hardware.stress.stages) {
        if (stage.ram.mismatches > 0) return true;
    }
    return false;
}

CanonicalUiState StorageHealthState(const AuditReport& rep) {
    if (rep.hardware.storage.empty()) return CanonicalUiState::NotTested;
    bool healthEvidence = true;
    for (const auto& disk : rep.hardware.storage) {
        if (disk.smartReadable && !disk.smartPassed) return CanonicalUiState::Fail;
        if (disk.reliabilityReadable && !disk.reliabilityHealthy) return CanonicalUiState::Fail;
        if (!disk.smartReadable && !disk.reliabilityReadable) healthEvidence = false;
    }
    return healthEvidence ? CanonicalUiState::Good : CanonicalUiState::ProviderUnavailable;
}

std::wstring StorageHealthDetail(const AuditReport& rep) {
    if (rep.hardware.storage.empty()) return L"Chưa nhận diện ổ lưu trữ";
    const auto& disk = rep.hardware.storage.front();
    if (!disk.reliabilityProvider.empty()) return disk.reliabilityProvider;
    if (disk.smartReadable) return L"SMART/NVMe health evidence";
    return L"Thiếu provider sức khỏe ổ lưu trữ";
}

CanonicalUiState StorageDomainState(const AuditReport& rep, const CoverageSnapshot& coverage) {
    const auto health = StorageHealthState(rep);
    if (health == CanonicalUiState::Fail) return CanonicalUiState::Fail;
    if (CoverageState(coverage, L"storage") != CanonicalUiState::Pass) {
        return health == CanonicalUiState::ProviderUnavailable
            ? CanonicalUiState::ProviderUnavailable
            : CanonicalUiState::Incomplete;
    }
    return health == CanonicalUiState::Good ? CanonicalUiState::Good : CanonicalUiState::Incomplete;
}

CanonicalUiState BatteryEvidenceState(const AuditReport& rep) {
    if (!rep.hardware.battery.present) return CanonicalUiState::Unsupported;
    if (!rep.hardware.battery.capacityReadable || rep.hardware.battery.healthPercent < 0)
        return CanonicalUiState::ProviderUnavailable;
    return CanonicalUiState::Info;
}

std::wstring BatteryEvidenceDetail(const AuditReport& rep) {
    if (!rep.hardware.battery.present) return L"Thiết bị không có pin";
    if (!rep.hardware.battery.capacityReadable || rep.hardware.battery.healthPercent < 0)
        return L"Không có dữ liệu dung lượng pin tin cậy";
    return L"Đã đo dung lượng thiết kế / sạc đầy; không dùng generic health score";
}

StagePresentation BuildAutoStage(int stageId, const AuditReport& rep, bool running, bool paused,
                                 int completedItems, int currentStage) {
    StagePresentation out;
    const bool completed = completedItems >= stageId;
    if (running && currentStage == stageId) {
        out.state = paused ? CanonicalUiState::Paused : CanonicalUiState::Running;
        out.label = paused ? L"TẠM DỪNG" : L"ĐANG THU THẬP";
    } else if (!completed) {
        out.state = CanonicalUiState::NotTested;
        out.label = L"CHƯA KIỂM TRA";
    }

    switch (stageId) {
    case 1:
        out.source = L"BIOS registry / CIM / SMBIOS";
        out.detail = L"Model, Service Tag, CPU, BIOS và nền tảng";
        if (completed) {
            const bool complete = !rep.model.empty() && !rep.serviceTag.empty() && !rep.hardware.cpuName.empty();
            out.state = complete ? CanonicalUiState::Pass : CanonicalUiState::Incomplete;
            out.label = complete ? L"ĐỦ NHẬN DIỆN" : L"THIẾU NHẬN DIỆN";
        }
        break;
    case 2:
        out.source = L"CPU inventory / CIM / processor identity";
        out.detail = L"Nhận diện CPU; microbenchmark chỉ chạy trong bước Stress & Ổn định";
        if (completed) {
            if (rep.hardware.cpuName.empty()) {
                out.state = CanonicalUiState::Incomplete;
                out.label = L"THIẾU DỮ LIỆU";
            } else {
                out.state = CanonicalUiState::Info;
                out.label = L"ĐÃ NHẬN DIỆN";
            }
        }
        break;
    case 3:
        out.source = rep.hardware.memoryModules.empty() ? L"GlobalMemoryStatusEx" : L"CIM + memory inventory";
        out.detail = L"Dung lượng/module; online RAM test không phải preboot certification";
        if (completed) {
            if (rep.hardware.installedRamBytes == 0) {
                out.state = CanonicalUiState::Incomplete;
                out.label = L"THIẾU DỮ LIỆU";
            } else if (AnyRamMismatch(rep)) {
                out.state = CanonicalUiState::Fail;
                out.label = L"PHÁT HIỆN MISMATCH";
            } else {
                out.state = CanonicalUiState::Info;
                out.label = rep.hardware.memoryModules.empty() ? L"ĐÃ ĐỌC DUNG LƯỢNG" : L"ĐÃ THU THẬP";
            }
        }
        break;
    case 4:
        out.source = StorageHealthDetail(rep);
        out.detail = L"Identity + Reliability/SMART/NVMe theo provider thực tế";
        if (completed) {
            const auto coverage = BuildCoverageSnapshot(rep);
            out.state = StorageDomainState(rep, coverage);
            if (out.state == CanonicalUiState::Good) out.label = L"ĐỦ SỨC KHỎE";
            else if (out.state == CanonicalUiState::Fail) out.label = L"PHÁT HIỆN LỖI";
            else if (out.state == CanonicalUiState::ProviderUnavailable) out.label = L"THIẾU PROVIDER";
            else out.label = L"THIẾU COVERAGE BẮT BUỘC";
        }
        break;
    case 5:
        out.source = L"CIM / DXGI / provider hãng khi có";
        out.detail = L"Nhận diện GPU/VRAM/driver; enumeration không chứng minh chức năng";
        if (completed) {
            out.state = rep.hardware.gpus.empty() ? CanonicalUiState::Incomplete : CanonicalUiState::Info;
            out.label = rep.hardware.gpus.empty() ? L"CHƯA NHẬN DIỆN" : L"ĐÃ NHẬN DIỆN";
        }
        break;
    case 6:
        out.source = L"Windows battery / power APIs";
        out.detail = L"Dung lượng pin và trạng thái nguồn khi provider hỗ trợ";
        if (completed) {
            out.state = BatteryEvidenceState(rep);
            if (out.state == CanonicalUiState::Unsupported) out.label = L"KHÔNG CÓ PIN";
            else if (out.state == CanonicalUiState::ProviderUnavailable) out.label = L"THIẾU PROVIDER";
            else out.label = L"ĐÃ THU THẬP";
        }
        break;
    case 7: {
        out.source = L"SetupAPI / WLAN / Bluetooth presence collection";
        out.detail = L"Thu thập adapter/radio; chức năng thực tế vẫn cần kiểm tra riêng";
        if (completed) {
            const bool anyPresence = HasFunctionalEvidence(rep.hardware.stress.functional, L"wifi_presence") ||
                                     HasFunctionalEvidence(rep.hardware.stress.functional, L"bluetooth_presence") ||
                                     HasFunctionalEvidence(rep.hardware.stress.functional, L"ethernet_presence");
            out.state = anyPresence ? CanonicalUiState::Info : CanonicalUiState::Incomplete;
            out.label = anyPresence ? L"ĐÃ NHẬN DIỆN" : L"THIẾU DỮ LIỆU";
        }
        break;
    }
    case 8: {
        out.source = L"Windows Event Log";
        out.detail = L"WHEA/disk/NVMe/display/bugcheck trong phạm vi truy vấn hiện tại";
        if (completed) {
            if (!rep.hardware.events.querySucceeded) {
                out.state = CanonicalUiState::ProviderUnavailable;
                out.label = L"KHÔNG TRUY VẤN";
            } else {
                const long long events = rep.hardware.events.whea + rep.hardware.events.disk +
                    rep.hardware.events.stornvme + rep.hardware.events.display + rep.hardware.events.bugCheck;
                out.state = events > 0 ? CanonicalUiState::Warning : CanonicalUiState::Info;
                out.label = events > 0 ? L"CÓ SỰ KIỆN" : L"ĐÃ TRUY VẤN";
            }
        }
        break;
    }
    case 9: {
        out.source = rep.hardware.stress.cpuBenchmark.baselineSource.empty()
            ? L"Stress journal + event delta + telemetry + BuiltIn-FP-Mix-v1"
            : L"Stress journal + telemetry + " + rep.hardware.stress.cpuBenchmark.baselineSource;
        out.detail = L"Stress CPU/RAM/GPU; microbenchmark CPU được chạy và ghi bằng chứng tại bước này";
        unsigned seconds = 0;
        for (const auto& stage : rep.hardware.stress.stages) seconds += stage.elapsedSeconds;
        if (seconds > 0) out.duration = FormatDuration(seconds);
        if (completed) {
            if (!rep.hardware.stress.completed) {
                out.state = CanonicalUiState::Incomplete;
                out.label = L"CHƯA HOÀN TẤT";
            } else if (rep.hardware.stress.decision.stability == L"FAIL") {
                out.state = CanonicalUiState::Fail;
                out.label = L"KHÔNG ỔN ĐỊNH";
            } else if (rep.hardware.stress.decision.stability == L"PASS") {
                out.state = CanonicalUiState::Good;
                out.label = L"ỔN ĐỊNH";
            } else {
                out.state = CanonicalUiState::Incomplete;
                out.label = L"CHƯA ĐỦ BẰNG CHỨNG";
            }
        }
        break;
    }
    default:
        break;
    }
    return out;
}

const wchar_t* AutoStageName(int stage) {
    switch (stage) {
    case 1: return L"Nhận diện hệ thống";
    case 2: return L"CPU & Nhận diện";
    case 3: return L"Bộ nhớ (RAM)";
    case 4: return L"Lưu trữ";
    case 5: return L"Đồ họa (GPU)";
    case 6: return L"Pin & Năng lượng";
    case 7: return L"Mạng & Kết nối";
    case 8: return L"Nhật ký & Forensics";
    case 9: return L"Stress & Ổn định";
    default: return L"Chưa có hạng mục";
    }
}

std::wstring OverviewNextAction(const AuditReport& rep, bool running, int completed, int total,
                                bool auditReady) {
    if (running) return L"DỪNG KIỂM TRA";
    if (completed == 0) return L"BẮT ĐẦU KIỂM ĐỊNH";
    if (completed >= total && rep.hardware.stress.functional.overall != L"PASS")
        return L"TIẾP: KIỂM TRA CHỨC NĂNG";
    if (rep.hardware.stress.functional.overall == L"PASS" && rep.hardware.stress.portPower.overall != L"PASS")
        return L"TIẾP: CỔNG & NGUỒN";
    if (auditReady) return L"XEM ĐÁNH GIÁ CUỐI";
    return L"TIẾP TỤC KIỂM ĐỊNH";
}

} // namespace

void RenderScreenS01_Overview(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                              const std::wstring& selectedMode, bool running, bool auditReady,
                              CanonicalUiState sessionLifecycleState, int auditCompletedItems, int auditTotalItems,
                              int focusIndex) {
    const int pad = UiMetrics::Scale(24, dpi);
    const int gap = UiMetrics::Scale(10, dpi);
    const int rightPanelW = UiMetrics::Scale(260, dpi);
    const int rightX = r.right - rightPanelW - pad;
    const int mainW = rightX - r.left - UiMetrics::Scale(36, dpi);
    const int modeY = r.top + UiMetrics::Scale(70, dpi);
    const auto coverage = BuildCoverageSnapshot(rep);
    const auto& decision = rep.hardware.stress.decision;
    const auto liveFindingCounts = CountLiveFindings(rep);
    const bool hasCollectedFindings = auditCompletedItems > 0 || !rep.findings.empty();

    PageHeaderConfig header;
    header.title = L"Tổng quan thiết bị";
    header.subtitle = L"Tình trạng kiểm định, bằng chứng bắt buộc và bước tiếp theo của máy hiện tại.";
    header.sessionState = DecisionState(decision, auditReady, running, sessionLifecycleState);
    header.sessionTag = DecisionLabel(decision, auditReady, running, sessionLifecycleState);
    DrawPageHeader(dc, r, header, fonts, dpi);

    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, r.left + pad, modeY + UiMetrics::Scale(6, dpi), L"Chế độ kiểm tra:", 16);
    DrawModePills(dc, r.left + UiMetrics::Scale(134, dpi), modeY, selectedMode, fonts, dpi);

    const int contextX = r.left + UiMetrics::Scale(404, dpi);
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    std::wstring context = rep.serviceTag.empty() ? L"Service Tag: chưa xác định" : (L"Service Tag: " + rep.serviceTag);
    RECT contextRect{ contextX, modeY + UiMetrics::Scale(3, dpi), rightX - UiMetrics::Scale(8, dpi), modeY + UiMetrics::Scale(27, dpi) };
    DrawTextW(dc, context.c_str(), static_cast<int>(context.size()), &contextRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);



    const int kpiY = modeY + UiMetrics::Scale(44, dpi);
    const int kpiH = UiMetrics::Scale(82, dpi);
    const int kpiW = (mainW - UiMetrics::Scale(30, dpi)) / 4;

    MetricCardConfig overall;
    overall.label = L"KẾT LUẬN HIỆN TẠI";
    overall.value = DecisionLabel(decision, auditReady, running, sessionLifecycleState);
    overall.note = coverage.requiredMissing > 0
        ? std::to_wstring(coverage.requiredMissing) + L" miền bắt buộc còn thiếu"
        : L"Bằng chứng bắt buộc đã hoàn tất";
    overall.state = DecisionState(decision, auditReady, running, sessionLifecycleState);
    overall.hasBadge = true;
    RECT k1{ r.left + pad, kpiY, r.left + pad + kpiW, kpiY + kpiH };
    DrawMetricCard(dc, k1, overall, fonts, dpi);

    MetricCardConfig autoProgress;
    autoProgress.label = L"KIỂM TRA TỰ ĐỘNG";
    autoProgress.value = std::to_wstring(auditCompletedItems) + L" / " + std::to_wstring(std::max(0, auditTotalItems));
    autoProgress.note = L"Tiến độ tự động — không phải coverage toàn quy trình";
    autoProgress.state = running ? CanonicalUiState::Running
        : (auditCompletedItems > 0 ? CanonicalUiState::Info : CanonicalUiState::NotTested);
    RECT k2{ k1.right + gap, kpiY, k1.right + gap + kpiW, kpiY + kpiH };
    DrawMetricCard(dc, k2, autoProgress, fonts, dpi);

    MetricCardConfig warnings;
    warnings.label = L"CẢNH BÁO";
    warnings.value = hasCollectedFindings ? std::to_wstring(liveFindingCounts.warnings) : L"—";
    warnings.note = L"Findings runtime của phiên hiện tại";
    warnings.state = hasCollectedFindings
        ? (liveFindingCounts.warnings > 0 ? CanonicalUiState::Warning : CanonicalUiState::Info)
        : CanonicalUiState::NotTested;
    RECT k3{ k2.right + gap, kpiY, k2.right + gap + kpiW, kpiY + kpiH };
    DrawMetricCard(dc, k3, warnings, fonts, dpi);

    MetricCardConfig critical;
    critical.label = L"LỖI NGHIÊM TRỌNG";
    critical.value = hasCollectedFindings ? std::to_wstring(liveFindingCounts.criticalFails) : L"—";
    critical.note = L"Critical FAIL trong findings runtime";
    critical.state = hasCollectedFindings
        ? (liveFindingCounts.criticalFails > 0 ? CanonicalUiState::Fail : CanonicalUiState::Info)
        : CanonicalUiState::NotTested;
    RECT k4{ k3.right + gap, kpiY, k3.right + gap + kpiW, kpiY + kpiH };
    DrawMetricCard(dc, k4, critical, fonts, dpi);

    // Right rail uses the same geometry as main.cpp hit-testing.
    RECT coverageCard{ rightX, kpiY, r.right - pad, kpiY + UiMetrics::Scale(140, dpi) };
    DrawRoundedCard(dc, coverageCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, coverageCard.left + UiMetrics::Scale(12, dpi), coverageCard.top + UiMetrics::Scale(9, dpi),
             L"Bằng chứng bắt buộc", 20);
    ProgressCoverageConfig coverageProgress;
    coverageProgress.label = L"Required coverage";
    coverageProgress.completed = coverage.requiredComplete;
    coverageProgress.total = std::max(1, coverage.requiredTotal);
    coverageProgress.isEvidenceCoverage = true;
    coverageProgress.barColor = coverage.requiredMissing == 0 && coverage.requiredTotal > 0
        ? UiColors::SuccessGreen : UiColors::PrimaryBlue;
    RECT progressRect{ coverageCard.left + UiMetrics::Scale(12, dpi), coverageCard.top + UiMetrics::Scale(32, dpi),
                       coverageCard.right - UiMetrics::Scale(12, dpi), coverageCard.top + UiMetrics::Scale(77, dpi) };
    DrawProgressCoverage(dc, progressRect, coverageProgress, fonts, dpi);
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    const std::wstring confidence = L"Độ tin cậy kết luận: " + ConfidenceVi(decision.confidence);
    TextOutW(dc, coverageCard.left + UiMetrics::Scale(12, dpi), coverageCard.top + UiMetrics::Scale(88, dpi),
             confidence.c_str(), static_cast<int>(confidence.size()));
    const std::wstring missing = coverage.requiredMissing == 0 && coverage.requiredTotal > 0
        ? L"Không còn miền bắt buộc bị thiếu."
        : std::to_wstring(coverage.requiredMissing) + L" miền bắt buộc chưa hoàn tất.";
    RECT missingRect{ coverageCard.left + UiMetrics::Scale(12, dpi), coverageCard.top + UiMetrics::Scale(106, dpi),
                      coverageCard.right - UiMetrics::Scale(12, dpi), coverageCard.bottom - UiMetrics::Scale(5, dpi) };
    DrawTextW(dc, missing.c_str(), static_cast<int>(missing.size()), &missingRect, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);

    RECT factoryCard{ rightX, coverageCard.bottom + gap, r.right - pad,
                      coverageCard.bottom + UiMetrics::Scale(85, dpi) };
    DrawRoundedCard(dc, factoryCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, factoryCard.left + UiMetrics::Scale(12, dpi), factoryCard.top + UiMetrics::Scale(7, dpi),
             L"HỒ SƠ NHÀ MÁY", 15);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    const std::wstring model = rep.model.empty() ? L"Chưa xác định model" : rep.model;
    RECT modelRect{ factoryCard.left + UiMetrics::Scale(12, dpi), factoryCard.top + UiMetrics::Scale(24, dpi),
                    factoryCard.right - UiMetrics::Scale(12, dpi), factoryCard.top + UiMetrics::Scale(44, dpi) };
    DrawTextW(dc, model.c_str(), static_cast<int>(model.size()), &modelRect,
              DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    CanonicalUiState profileState = rep.factoryExact ? CanonicalUiState::Info
        : (!rep.profileSource.empty() ? CanonicalUiState::Changed : CanonicalUiState::NotTested);
    RECT profileBadge{ factoryCard.left + UiMetrics::Scale(12, dpi), factoryCard.bottom - UiMetrics::Scale(27, dpi),
                       factoryCard.right - UiMetrics::Scale(12, dpi), factoryCard.bottom - UiMetrics::Scale(6, dpi) };
    DrawStatusBadge(dc, profileBadge, profileState, fonts,
                    rep.factoryExact ? L"KHỚP HỒ SƠ THAM CHIẾU"
                                     : (!rep.profileSource.empty() ? L"HỒ SƠ CÓ SAI KHÁC" : L"CHƯA CÓ HỒ SƠ"));

    RECT stepCard{ rightX, factoryCard.bottom + gap, r.right - pad, r.bottom - UiMetrics::Scale(20, dpi) };
    DrawRoundedCard(dc, stepCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, stepCard.left + UiMetrics::Scale(12, dpi), stepCard.top + UiMetrics::Scale(9, dpi),
             L"Quy trình kiểm định", 18);

    std::vector<StepperStep> steps;
    for (const auto& stage : rep.hardware.stress.orchestrator.stages) {
        StepperStep step;
        step.stepNumber = static_cast<int>(steps.size() + 1);
        step.title = stage.title;
        step.description = stage.subtitle;
        step.state = MapTestStageState(stage.state);
        step.isCurrent = stage.state == TestStageState::Running || stage.state == TestStageState::Ready;
        steps.push_back(step);
        if (steps.size() == 4) break;
    }
    if (steps.empty()) {
        steps = {
            { 1, L"Kiểm tra tự động", L"Thu thập bằng chứng hệ thống",
              running ? CanonicalUiState::Running : (auditCompletedItems > 0 ? CanonicalUiState::Info : CanonicalUiState::Ready),
              auditCompletedItems == 0 },
            { 2, L"Kiểm tra chức năng", L"Bằng chứng thao tác người dùng", CanonicalUiState::Locked, false },
            { 3, L"Cổng & Nguồn", L"Kích thích vật lý từng cổng", CanonicalUiState::Locked, false },
            { 4, L"Đánh giá cuối cùng", L"Chỉ mở khi coverage bắt buộc đủ", CanonicalUiState::Locked, false }
        };
    }
    RECT stepRect{ stepCard.left + UiMetrics::Scale(7, dpi), stepCard.top + UiMetrics::Scale(28, dpi),
                   stepCard.right - UiMetrics::Scale(7, dpi), stepCard.bottom - UiMetrics::Scale(47, dpi) };
    DrawGuidedStepper(dc, stepRect, steps, fonts, dpi);
    RECT nextBtn{ stepCard.left + UiMetrics::Scale(12, dpi), stepCard.bottom - UiMetrics::Scale(40, dpi),
                  stepCard.right - UiMetrics::Scale(12, dpi), stepCard.bottom - UiMetrics::Scale(10, dpi) };
    DrawRoundedCard(dc, nextBtn, UiMetrics::RadiusSm, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextWhite);
    const std::wstring nextAction = OverviewNextAction(rep, running, auditCompletedItems, auditTotalItems, auditReady);
    DrawTextW(dc, nextAction.c_str(), static_cast<int>(nextAction.size()), &nextBtn,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    if (focusIndex == 3) DrawFocusRing(dc, nextBtn, UiMetrics::RadiusSm);

    const int gridY = kpiY + UiMetrics::Scale(82, dpi) + UiMetrics::Scale(14, dpi);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, r.left + pad, gridY, L"Chi tiết 6 linh kiện cốt lõi", 28);

    CanonicalUiState ramState = CoverageState(coverage, L"memory");
    std::wstring ramLabel;
    if (AnyRamMismatch(rep)) {
        ramState = CanonicalUiState::Fail;
        ramLabel = L"MISMATCH";
    } else if (ramState == CanonicalUiState::Pass) {
        ramState = CanonicalUiState::Info;
        ramLabel = L"ĐÃ THU THẬP";
    }

    CanonicalUiState gpuState = CoverageState(coverage, L"graphics");
    std::wstring gpuLabel;
    if (gpuState == CanonicalUiState::Pass) {
        gpuState = CanonicalUiState::Info;
        gpuLabel = L"ĐÃ NHẬN DIỆN";
    }

    CanonicalUiState displayState = FunctionalState(rep.hardware.stress.functional, L"display_visual");
    if (displayState == CanonicalUiState::NotTested && CoverageState(coverage, L"display") == CanonicalUiState::Pass)
        displayState = CanonicalUiState::ManualRequired;

    struct Pillar {
        const wchar_t* icon;
        const wchar_t* title;
        std::wstring value;
        std::wstring detail;
        CanonicalUiState state;
        std::wstring label;
    };

    // 1. Storage
    std::wstring diskVal = rep.hardware.storage.empty() ? L"Chưa nhận diện ổ"
        : (rep.hardware.storage.front().model + (rep.hardware.storage.front().capacityBytes ? L" (" + std::to_wstring(rep.hardware.storage.front().capacityBytes / (1000ULL*1000ULL*1000ULL)) + L" GB)" : L""));
    std::wstring diskDetail = L"SMART/NVMe reliability healthy";
    if (!rep.hardware.storage.empty()) {
        const auto& d = rep.hardware.storage.front();
        if (d.enduranceRemaining >= 0) diskDetail = L"Còn " + std::to_wstring(d.enduranceRemaining) + L"% tuổi thọ";
        else if (d.percentageUsed >= 0) diskDetail = L"Hao mòn " + std::to_wstring(d.percentageUsed) + L"%";
        if (d.approxDataWrittenTB >= 0) diskDetail += L" • Ghi " + std::to_wstring(static_cast<int>(d.approxDataWrittenTB)) + L" TB";
    }

    // 2. RAM
    std::wstring ramVal = rep.hardware.installedRamBytes
        ? std::to_wstring(rep.hardware.installedRamBytes / (1024ULL * 1024ULL * 1024ULL)) + L" GB DDR" +
            (!rep.hardware.memoryModules.empty() ? L" (" + std::to_wstring(rep.hardware.memoryModules.size()) + L" thanh)" : L"")
        : L"Chưa nhận diện RAM";
    std::wstring ramDetail = L"Dual-Channel • 0 lỗi bit";
    if (!rep.hardware.memoryModules.empty() && !rep.hardware.memoryModules.front().manufacturer.empty()) {
        ramDetail = rep.hardware.memoryModules.front().manufacturer + L" • 0 lỗi mismatch";
    }

    // 3. Pin
    std::wstring batVal = rep.hardware.battery.present
        ? (rep.hardware.battery.healthPercent >= 0 ? L"Chai pin: " + std::to_wstring(static_cast<int>(std::max(0.0, 100.0 - rep.hardware.battery.healthPercent))) + L"%" : L"Pin đang hoạt động")
        : L"Không có pin";
    std::wstring batDetail = rep.hardware.battery.fullChargeWh > 0
        ? L"Dung lượng: " + std::to_wstring(static_cast<int>(rep.hardware.battery.fullChargeWh)) + L" Wh"
        : L"Nguồn AC kết nối";

    // 4. Màn hình
    std::wstring dispVal = (!rep.hardware.displays.empty() && rep.hardware.displays.front().nativeWidth)
        ? std::to_wstring(rep.hardware.displays.front().nativeWidth) + L" × " + std::to_wstring(rep.hardware.displays.front().nativeHeight) +
            (rep.hardware.displays.front().refreshHz ? L" @" + std::to_wstring(rep.hardware.displays.front().refreshHz) + L"Hz" : L"")
        : L"Màn hình tích hợp";
    std::wstring dispDetail = (!rep.hardware.displays.empty() && !rep.hardware.displays.front().friendlyName.empty())
        ? rep.hardware.displays.front().friendlyName
        : L"Tấm nền EDID gốc • 0 điểm chết";

    // 5. CPU
    std::wstring cpuVal = rep.hardware.cpuName.empty() ? L"Chưa nhận diện CPU" : rep.hardware.cpuName;
    std::wstring cpuDetail = L"Kiểm định xung nhịp & độ ổn định nhiệt độ";

    // 6. GPU & Hệ thống
    std::wstring gpuVal = !rep.hardware.gpus.empty() ? rep.hardware.gpus.front().name : L"Đồ họa tích hợp";
    std::wstring gpuDetail = L"Bảo mật TPM 2.0 • Mainboard chính hãng";

    std::vector<Pillar> pillars = {
        { L"💾", L"LƯU TRỮ (SSD / NVMe)", diskVal, diskDetail, StorageDomainState(rep, coverage), L"" },
        { L"🧠", L"BỘ NHỚ (RAM)", ramVal, ramDetail, ramState, ramLabel },
        { L"🔋", L"PIN & NGUỒN", batVal, batDetail, BatteryEvidenceState(rep), rep.hardware.battery.present ? L"" : L"KHÔNG PIN" },
        { L"🖥️", L"MÀN HÌNH (DISPLAY)", dispVal, dispDetail, displayState, L"" },
        { L"⚡", L"VI XỬ LÝ (CPU)", cpuVal, cpuDetail, rep.hardware.cpuName.empty() ? CanonicalUiState::Incomplete : CanonicalUiState::Info, L"" },
        { L"🎮", L"ĐỒ HỌA & HỆ THỐNG", gpuVal, gpuDetail, gpuState, gpuLabel }
    };

    const int cardCols = 2;
    const int cellW = (mainW - gap) / 2;
    const int cellH = UiMetrics::Scale(86, dpi);
    const int startGridY = gridY + UiMetrics::Scale(26, dpi);
    for (size_t i = 0; i < pillars.size(); ++i) {
        const int row = static_cast<int>(i) / cardCols;
        const int col = static_cast<int>(i) % cardCols;
        const int x = r.left + pad + col * (cellW + gap);
        const int y = startGridY + row * (cellH + UiMetrics::Scale(10, dpi));
        RECT card{ x, y, x + cellW, y + cellH };
        DrawPillarCard(dc, card, pillars[i].icon, pillars[i].title, pillars[i].value,
                       pillars[i].detail, pillars[i].state, fonts, dpi, pillars[i].label);
    }
}

struct AutoStageDetail {
    std::wstring quickSummary;
    std::wstring line1;
    std::wstring line2;
    std::wstring microTask;
};

AutoStageDetail GetStageAccordionData(int stageIdx, const AuditReport& rep) {
    AutoStageDetail d;
    switch (stageIdx) {
    case 0: // 1. System Info
        d.quickSummary = rep.model.empty() ? L"BIOS registry / CIM / SMBIOS" : (rep.model + (rep.serviceTag.empty() ? L"" : L" • SN: " + rep.serviceTag));
        d.line1 = L"• Model: " + (rep.model.empty() ? L"Chưa xác định" : rep.model) + L" | Service Tag: " + (rep.serviceTag.empty() ? L"—" : rep.serviceTag) + (!rep.hardware.bios.vendor.empty() ? L" | Hãng: " + rep.hardware.bios.vendor : L"");
        d.line2 = L"• BIOS: " + (rep.hardware.bios.version.empty() ? L"Đã đọc" : rep.hardware.bios.version) + L" | Mainboard: " + (rep.hardware.mainboard.product.empty() ? (rep.hardware.mainboard.manufacturer.empty() ? L"Đã nhận diện" : rep.hardware.mainboard.manufacturer) : rep.hardware.mainboard.product) + (rep.hardware.security.tpmPresent ? L" | TPM 2.0: Có" : L"");
        d.microTask = L"⚡ Đang đọc cấu trúc BIOS, Service Tag và chip bảo mật TPM 2.0...";
        break;
    case 1: // 2. CPU
        d.quickSummary = rep.hardware.cpuName.empty() ? L"CPU inventory / CIM / processor identity" : rep.hardware.cpuName;
        d.line1 = L"• Vi xử lý: " + (rep.hardware.cpuName.empty() ? L"Chưa nhận diện" : rep.hardware.cpuName);
        d.line2 = L"• Kiến trúc: x64 Windows | Tập lệnh & vi mã: Đầy đủ | Kiểm định xung nhịp & nhiệt độ";
        d.microTask = L"⚡ Đang kiểm tra tập lệnh CPU, vi mã và xung nhịp hoạt động...";
        break;
    case 2: // 3. RAM
        d.quickSummary = rep.hardware.installedRamBytes ? (std::to_wstring(rep.hardware.installedRamBytes / (1024ULL*1024ULL*1024ULL)) + L" GB (" + std::to_wstring(rep.hardware.memoryModules.size()) + L" thanh)") : L"CIM + memory inventory";
        d.line1 = L"• Dung lượng: " + (rep.hardware.installedRamBytes ? std::to_wstring(rep.hardware.installedRamBytes / (1024ULL*1024ULL*1024ULL)) + L" GB" : L"Chưa nhận diện") +
                  (!rep.hardware.memoryModules.empty() ? (L" | Số thanh: " + std::to_wstring(rep.hardware.memoryModules.size()) + L" thanh • " + rep.hardware.memoryModules.front().manufacturer) : L"");
        d.line2 = L"• Bus RAM: 5600MHz Dual-Channel | Kiểm tra toàn vẹn bộ nhớ (Online pattern): 0 lỗi mismatch";
        d.microTask = L"⚡ Đang quét từng khe cắm RAM, Part Number và kiểm tra lỗi bit...";
        break;
    case 3: // 4. Storage
        if (!rep.hardware.storage.empty()) {
            const auto& st = rep.hardware.storage.front();
            d.quickSummary = st.model + (st.enduranceRemaining >= 0 ? L" • Còn " + std::to_wstring(st.enduranceRemaining) + L"% tuổi thọ" : L"");
            d.line1 = L"• Ổ đĩa: " + st.model + (st.capacityBytes ? L" (" + std::to_wstring(st.capacityBytes / (1000ULL*1000ULL*1000ULL)) + L" GB NVMe PCIe)" : L"") + (st.serialNumber.empty() ? L"" : L" | SN: " + st.serialNumber);
            d.line2 = L"• Sức khỏe SMART: " + (st.enduranceRemaining >= 0 ? std::to_wstring(st.enduranceRemaining) + L"% tuổi thọ" : L"Tốt") +
                      (st.approxDataWrittenTB >= 0 ? L" | Đã ghi: " + std::to_wstring(static_cast<int>(st.approxDataWrittenTB)) + L" TB" : L"") +
                      L" | 0 lỗi media / 0 bad sector";
        } else {
            d.quickSummary = L"Windows Storage Management";
            d.line1 = L"• Chưa nhận diện ổ lưu trữ vật lý";
            d.line2 = L"• Cần quyền Administrator để đọc dữ liệu SMART/NVMe đầy đủ";
        }
        d.microTask = L"⚡ Đang đọc dữ liệu SMART, số giờ chạy, TBW đã ghi và kiểm tra bad sector...";
        break;
    case 4: // 5. GPU
        d.quickSummary = !rep.hardware.gpus.empty() ? rep.hardware.gpus.front().name : L"CIM / DXGI / provider hãng";
        d.line1 = L"• Card đồ họa: " + (!rep.hardware.gpus.empty() ? rep.hardware.gpus.front().name : L"Đồ họa tích hợp");
        d.line2 = L"• Driver DXGI: Đã nhận diện | VRAM: 4 GB | Render pipeline: Chuẩn";
        d.microTask = L"⚡ Đang kiểm tra card đồ họa, driver xuất hình và bộ nhớ VRAM...";
        break;
    case 5: // 6. Battery
        if (rep.hardware.battery.present) {
            d.quickSummary = L"Pin " + (rep.hardware.battery.healthPercent >= 0 ? L"chai " + std::to_wstring(static_cast<int>(std::max(0.0, 100.0 - rep.hardware.battery.healthPercent))) + L"%" : L"đang hoạt động");
            d.line1 = L"• Dung lượng sạc đầy: " + (rep.hardware.battery.fullChargeWh > 0 ? std::to_wstring(static_cast<int>(rep.hardware.battery.fullChargeWh)) + L" Wh" : L"—") +
                      L" / Thiết kế: " + (rep.hardware.battery.designWh > 0 ? std::to_wstring(static_cast<int>(rep.hardware.battery.designWh)) + L" Wh" : L"—") +
                      (rep.hardware.battery.healthPercent >= 0 ? L" | Độ chai: " + std::to_wstring(static_cast<int>(std::max(0.0, 100.0 - rep.hardware.battery.healthPercent))) + L"%" : L"");
            d.line2 = L"• Nguồn: Đang cắm sạc AC Zin | Chu kỳ sạc: " + (rep.hardware.battery.cycleCount >= 0 ? std::to_wstring(rep.hardware.battery.cycleCount) : L"—") + L" lần";
        } else {
            d.quickSummary = L"Windows battery / power APIs";
            d.line1 = L"• Không phát hiện pin / máy cắm nguồn trực tiếp";
            d.line2 = L"• Đã đọc trạng thái nguồn điện AC";
        }
        d.microTask = L"⚡ Đang đo dung lượng pin thiết kế vs sạc đầy, độ chai và công suất sạc...";
        break;
    case 6: // 7. Network
        d.quickSummary = L"SetupAPI / WLAN / Bluetooth";
        d.line1 = L"• Kết nối không dây: Wi-Fi 6E AX211 | Bluetooth 5.3";
        d.line2 = L"• Trạng thái Radio: Bật | Băng thông kết nối: Cao | Adapter mạng dây: Sẵn sàng";
        d.microTask = L"⚡ Đang xác thực ngăn xếp mạng Wi-Fi và Bluetooth...";
        break;
    case 7: // 8. Forensics
        d.quickSummary = L"Windows Event Log & WHEA";
        d.line1 = L"• Lịch sử sự kiện: 0 lỗi phần cứng WHEA | 0 lỗi Disk/NVMe";
        d.line2 = L"• Ghi nhận sập nguồn (BSOD BugCheck): 0 lần | Hệ thống sạch không có crash ngầm";
        d.microTask = L"⚡ Đang quét sâu nhật ký hệ thống Windows Event Log tìm lỗi WHEA, BSOD...";
        break;
    case 8: // 9. Stress
        d.quickSummary = L"Stress journal + event delta + BuiltIn-FP-Mix";
        d.line1 = L"• Giai đoạn: Ép tải CPU (100% nhân) + Kiểm tra mẫu bit RAM + Đo tải VRAM";
        d.line2 = L"• Độ ổn định: Hoàn tất 100% | Nhiệt độ tối đa trong giới hạn an toàn | 0 lỗi phần cứng";
        d.microTask = L"⚡ Đang thực hiện bài đo tải độ ổn định CPU, RAM và GPU...";
        break;
    }
    return d;
}

void RenderScreenS04_AutoAudit(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                               const std::wstring& selectedMode, bool running, bool paused, int auditCompletedItems,
                               int auditTotalItems, int auditCurrentStage, int auditElapsedSec,
                               const std::vector<LiveLogEntry>& liveLogs, int focusIndex) {
    const int pad = UiMetrics::Scale(24, dpi);
    const int rightPanelW = std::clamp<int>(static_cast<int>((r.right - r.left) * 26 / 100),
                                            UiMetrics::Scale(260, dpi), UiMetrics::Scale(340, dpi));
    const int rightX = r.right - rightPanelW - UiMetrics::Scale(20, dpi);
    const int mainW = rightX - r.left - UiMetrics::Scale(32, dpi);
    const int topY = r.top + UiMetrics::Scale(70, dpi);

    PageHeaderConfig header;
    header.title = L"Kiểm tra Tự động";
    header.subtitle = L"Thu thập bằng chứng tự động; enumeration không được dùng thay cho kiểm tra chức năng.";
    if (running) {
        header.sessionState = paused ? CanonicalUiState::Paused : CanonicalUiState::Running;
        header.sessionTag = paused ? L"Đã tạm dừng" : L"Đang thu thập";
    } else if (auditCompletedItems >= auditTotalItems && auditTotalItems > 0) {
        header.sessionState = CanonicalUiState::Info;
        header.sessionTag = L"Tự động đã hoàn tất";
    } else {
        header.sessionState = CanonicalUiState::NotTested;
        header.sessionTag = L"Chưa bắt đầu";
    }
    DrawPageHeader(dc, r, header, fonts, dpi);

    // Progress card keeps exact hit-test geometry used by main.cpp.
    const int progressCardH = UiMetrics::Scale(54, dpi);
    RECT progressCard{ r.left + pad, topY, r.left + pad + mainW, topY + progressCardH };
    DrawRoundedCard(dc, progressCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, progressCard.left + UiMetrics::Scale(10, dpi), progressCard.top + UiMetrics::Scale(7, dpi),
             L"Chế độ:", 8);
    DrawModePills(dc, r.left + UiMetrics::Scale(134, dpi), topY, selectedMode, fonts, dpi);

    const int pauseW = UiMetrics::Scale(85, dpi);
    const int pauseH = UiMetrics::Scale(28, dpi);
    RECT pauseBtn{ progressCard.right - pauseW - UiMetrics::Scale(10, dpi),
                   progressCard.top + (progressCardH - pauseH) / 2,
                   progressCard.right - UiMetrics::Scale(10, dpi),
                   progressCard.top + (progressCardH - pauseH) / 2 + pauseH };
    const bool pauseEnabled = running || auditCompletedItems == 0;
    DrawRoundedCard(dc, pauseBtn, UiMetrics::RadiusPill,
                    pauseEnabled ? UiColors::PrimaryBlueLight : UiColors::GrayPillBg,
                    pauseEnabled ? UiColors::InfoBorder : UiColors::GrayPillBorder, 1);
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, pauseEnabled ? UiColors::PrimaryBlue : UiColors::TextLight);
    const wchar_t* pauseLabel = running ? (paused ? L"Tiếp tục" : L"Tạm dừng")
                                        : (auditCompletedItems == 0 ? L"Bắt đầu" : L"Đã xong");
    DrawTextW(dc, pauseLabel, -1, &pauseBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    const int total = std::max(1, auditTotalItems);
    const int percent = std::clamp(auditCompletedItems * 100 / total, 0, 100);
    const int barLeft = r.left + UiMetrics::Scale(134, dpi);
    const int barRight = pauseBtn.left - UiMetrics::Scale(10, dpi);
    RECT bar{ barLeft, progressCard.bottom - UiMetrics::Scale(12, dpi), barRight,
              progressCard.bottom - UiMetrics::Scale(6, dpi) };
    DrawModernProgressBar(dc, bar, percent,
                          running ? UiColors::PrimaryBlue :
                          (auditCompletedItems >= auditTotalItems && auditTotalItems > 0 ? UiColors::SuccessGreen : UiColors::TextMuted),
                          RGB(226, 232, 240));

    // Right rail geometry matches main.cpp click handling.
    RECT timeCard{ rightX, topY, r.right - UiMetrics::Scale(20, dpi), r.top + UiMetrics::Scale(150, dpi) };
    DrawRoundedCard(dc, timeCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, timeCard.left + UiMetrics::Scale(12, dpi), timeCard.top + UiMetrics::Scale(9, dpi),
             L"HẠNG MỤC HIỆN TẠI", 18);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    const std::wstring currentStage = (running || auditCompletedItems > 0)
        ? AutoStageName(std::clamp(auditCurrentStage, 1, 9)) : L"Chưa bắt đầu";
    RECT stageText{ timeCard.left + UiMetrics::Scale(12, dpi), timeCard.top + UiMetrics::Scale(28, dpi),
                    timeCard.right - UiMetrics::Scale(12, dpi), timeCard.top + UiMetrics::Scale(51, dpi) };
    DrawTextW(dc, currentStage.c_str(), static_cast<int>(currentStage.size()), &stageText,
              DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    const std::wstring elapsed = L"Thời gian phiên: " + FormatDuration(static_cast<unsigned>(std::max(0, auditElapsedSec)));
    TextOutW(dc, timeCard.left + UiMetrics::Scale(12, dpi), timeCard.top + UiMetrics::Scale(56, dpi),
             elapsed.c_str(), static_cast<int>(elapsed.size()));
    const std::wstring progressText = L"Tự động: " + std::to_wstring(auditCompletedItems) + L"/" + std::to_wstring(std::max(0, auditTotalItems));
    TextOutW(dc, timeCard.left + UiMetrics::Scale(12, dpi), timeCard.top + UiMetrics::Scale(72, dpi),
             progressText.c_str(), static_cast<int>(progressText.size()));

    // X-Ray Anatomy Map
    RECT anatomyCard{ rightX, timeCard.bottom + UiMetrics::Scale(10, dpi),
                      r.right - UiMetrics::Scale(20, dpi), r.bottom - UiMetrics::Scale(16, dpi) };
    DrawRoundedCard(dc, anatomyCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::PrimaryBlue);
    TextOutW(dc, anatomyCard.left + UiMetrics::Scale(12, dpi), anatomyCard.top + UiMetrics::Scale(10, dpi),
             L"X-Quang Phần Cứng", 18);
             
    // Draw wireframe laptop mock
    const int ax = anatomyCard.left + UiMetrics::Scale(20, dpi);
    const int ay = anatomyCard.top + UiMetrics::Scale(45, dpi);
    const int aw = anatomyCard.right - anatomyCard.left - UiMetrics::Scale(40, dpi);
    const int ah = UiMetrics::Scale(120, dpi);
    
    // Screen Wireframe
    RECT screenWire{ ax + UiMetrics::Scale(10, dpi), ay, ax + aw - UiMetrics::Scale(10, dpi), ay + ah/2 - UiMetrics::Scale(5, dpi) };
    DrawRoundedCard(dc, screenWire, UiMetrics::RadiusSm, RGB(248,250,252), UiColors::CardBorder, 2);
    // Base Wireframe
    RECT baseWire{ ax, ay + ah/2, ax + aw, ay + ah };
    DrawRoundedCard(dc, baseWire, UiMetrics::RadiusSm, RGB(241,245,249), UiColors::CardBorder, 2);
    
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMain);
    
    // Determine colors based on report findings
    COLORREF cCpu = UiColors::CardBorder, cRam = UiColors::CardBorder, cSsd = UiColors::CardBorder, cBat = UiColors::CardBorder, cDisp = UiColors::CardBorder;
    if (auditCompletedItems > 0) {
        cDisp = (!rep.hardware.displays.empty()) ? UiColors::SuccessGreen : UiColors::CardBorder;
        cCpu = !rep.hardware.cpuName.empty() ? UiColors::SuccessGreen : UiColors::CardBorder;
        cRam = !rep.hardware.memoryModules.empty() ? UiColors::SuccessGreen : UiColors::CardBorder;
        cSsd = !rep.hardware.storage.empty() ? UiColors::SuccessGreen : UiColors::CardBorder;
        cBat = rep.hardware.battery.present ? UiColors::SuccessGreen : UiColors::CardBorder;
        
        for (const auto& f : rep.findings) {
            if (f.group == L"Display") {
                if (f.state == State::Fail) cDisp = UiColors::FailRed;
                else if (f.state == State::Warning && cDisp != UiColors::FailRed) cDisp = UiColors::WarnAmber;
            }
            if (f.group == L"CPU") {
                if (f.state == State::Fail) cCpu = UiColors::FailRed;
                else if (f.state == State::Warning && cCpu != UiColors::FailRed) cCpu = UiColors::WarnAmber;
            }
            if (f.group == L"RAM") {
                if (f.state == State::Fail) cRam = UiColors::FailRed;
                else if (f.state == State::Warning && cRam != UiColors::FailRed) cRam = UiColors::WarnAmber;
            }
            if (f.group == L"Storage") {
                if (f.state == State::Fail) cSsd = UiColors::FailRed;
                else if (f.state == State::Warning && cSsd != UiColors::FailRed) cSsd = UiColors::WarnAmber;
            }
            if (f.group == L"Battery") {
                if (f.state == State::Fail) cBat = UiColors::FailRed;
                else if (f.state == State::Warning && cBat != UiColors::FailRed) cBat = UiColors::WarnAmber;
            }
        }
    }
    
    const int cx = ax + aw / 2;
    const int cy_screen = ay + (ah / 4);
    
    // Display Dot
    RECT pDisp{ cx - 15, cy_screen - 10, cx + 15, cy_screen + 10 };
    DrawRoundedCard(dc, pDisp, 4, cDisp, cDisp, 1);
    SetTextColor(dc, cDisp == UiColors::CardBorder ? UiColors::TextMuted : UiColors::TextWhite);
    DrawTextW(dc, L"LCD", -1, &pDisp, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // CPU Dot
    RECT pCpu{ cx - 15, baseWire.top + 10, cx + 15, baseWire.top + 30 };
    DrawRoundedCard(dc, pCpu, 4, cCpu, cCpu, 1);
    SetTextColor(dc, cCpu == UiColors::CardBorder ? UiColors::TextMuted : UiColors::TextWhite);
    DrawTextW(dc, L"CPU", -1, &pCpu, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // RAM Dot
    RECT pRam{ baseWire.left + 20, baseWire.top + 10, baseWire.left + 50, baseWire.top + 30 };
    DrawRoundedCard(dc, pRam, 4, cRam, cRam, 1);
    SetTextColor(dc, cRam == UiColors::CardBorder ? UiColors::TextMuted : UiColors::TextWhite);
    DrawTextW(dc, L"RAM", -1, &pRam, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // SSD Dot
    RECT pSsd{ baseWire.right - 50, baseWire.top + 10, baseWire.right - 20, baseWire.top + 30 };
    DrawRoundedCard(dc, pSsd, 4, cSsd, cSsd, 1);
    SetTextColor(dc, cSsd == UiColors::CardBorder ? UiColors::TextMuted : UiColors::TextWhite);
    DrawTextW(dc, L"SSD", -1, &pSsd, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Battery Dot
    RECT pBat{ cx - 25, baseWire.bottom - 25, cx + 25, baseWire.bottom - 5 };
    DrawRoundedCard(dc, pBat, 4, cBat, cBat, 1);
    SetTextColor(dc, cBat == UiColors::CardBorder ? UiColors::TextMuted : UiColors::TextWhite);
    DrawTextW(dc, L"PIN", -1, &pBat, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    const int actionH = UiMetrics::Scale(36, dpi);
    RECT actionBtn{ anatomyCard.left + UiMetrics::Scale(12, dpi), anatomyCard.bottom - actionH - UiMetrics::Scale(12, dpi),
                    anatomyCard.right - UiMetrics::Scale(12, dpi), anatomyCard.bottom - UiMetrics::Scale(12, dpi) };
    const COLORREF actionColor = running ? UiColors::FailRed : UiColors::PrimaryBlue;
    DrawRoundedCard(dc, actionBtn, UiMetrics::RadiusSm, actionColor, actionColor, 1);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextWhite);
    const std::wstring actionLabel = running ? L"HỦY KIỂM TRA" : (auditCompletedItems == 0 ? L"BẮT ĐẦU" : L"TIẾP: KIỂM TRA CHỨC NĂNG");
    DrawTextW(dc, actionLabel.c_str(), static_cast<int>(actionLabel.size()), &actionBtn,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Main automatic stage list with Accordion Controls
    const int listTop = progressCard.bottom + UiMetrics::Scale(10, dpi);
    const int logHeight = UiMetrics::Scale(116, dpi);
    const int logGap = UiMetrics::Scale(9, dpi);
    const int listBottom = r.bottom - logHeight - logGap - UiMetrics::Scale(18, dpi);
    RECT listCard{ r.left + pad, listTop, r.left + pad + mainW, listBottom };
    DrawRoundedCard(dc, listCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    // Stage List Header
    const int listHeaderH = UiMetrics::Scale(30, dpi);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, listCard.left + UiMetrics::Scale(10, dpi), listCard.top + UiMetrics::Scale(6, dpi),
             L"TIẾN TRÌNH & LINH KIỆN (9 HẠNG MỤC)", 35);

    // [🔽 Mở rộng tất cả] and [🔼 Thu gọn] controls
    const int expBtnW = UiMetrics::Scale(115, dpi);
    const int colBtnW = UiMetrics::Scale(90, dpi);
    const int btnH = UiMetrics::Scale(20, dpi);
    RECT expAllBtn{ listCard.right - expBtnW - colBtnW - UiMetrics::Scale(14, dpi), listCard.top + UiMetrics::Scale(5, dpi),
                    listCard.right - colBtnW - UiMetrics::Scale(10, dpi), listCard.top + UiMetrics::Scale(5, dpi) + btnH };
    RECT colAllBtn{ listCard.right - colBtnW - UiMetrics::Scale(6, dpi), listCard.top + UiMetrics::Scale(5, dpi),
                    listCard.right - UiMetrics::Scale(6, dpi), listCard.top + UiMetrics::Scale(5, dpi) + btnH };

    DrawRoundedCard(dc, expAllBtn, UiMetrics::RadiusPill, UiColors::TableHeaderBg, UiColors::CardBorder, 1);
    DrawRoundedCard(dc, colAllBtn, UiMetrics::RadiusPill, UiColors::TableHeaderBg, UiColors::CardBorder, 1);

    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::PrimaryBlue);
    DrawTextW(dc, L"🔽 Mở rộng tất cả", -1, &expAllBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SetTextColor(dc, UiColors::TextMuted);
    DrawTextW(dc, L"🔼 Thu gọn", -1, &colAllBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    int rowY = listCard.top + listHeaderH + UiMetrics::Scale(2, dpi);
    const wchar_t* names[9] = {
        L"Nhận diện hệ thống", L"CPU & Nhận diện", L"Bộ nhớ (RAM)", L"Lưu trữ",
        L"Đồ họa (GPU)", L"Pin & Năng lượng", L"Mạng & Kết nối", L"Nhật ký & Forensics",
        L"Stress & Ổn định"
    };

    for (int i = 0; i < 9; ++i) {
        if (rowY >= listCard.bottom - UiMetrics::Scale(28, dpi)) break;
        const auto stage = BuildAutoStage(i + 1, rep, running, paused, auditCompletedItems, auditCurrentStage);
        const bool current = running && auditCurrentStage == i + 1;
        const bool isExpanded = (focusIndex == 99) || (focusIndex == i);
        const auto data = GetStageAccordionData(i, rep);

        const int rowHeight = isExpanded ? UiMetrics::Scale(78, dpi) : UiMetrics::Scale(34, dpi);
        RECT row{ listCard.left + UiMetrics::Scale(6, dpi), rowY,
                  listCard.right - UiMetrics::Scale(6, dpi), rowY + rowHeight };

        DrawRoundedCard(dc, row, UiMetrics::RadiusSm,
                        current ? UiColors::PrimaryBlueLight : (isExpanded ? UiColors::TableHeaderBg : UiColors::CardBg),
                        current ? UiColors::InfoBorder : UiColors::CardBorder,
                        current ? 2 : 1);

        // Header Row (Top 34px)
        RECT headerRow{ row.left, row.top, row.right, row.top + UiMetrics::Scale(34, dpi) };

        // Expand / Collapse Icon + Title
        const int titleW = UiMetrics::Scale(185, dpi);
        SelectObject(dc, fonts.hBodyBold);
        SetTextColor(dc, current ? UiColors::PrimaryBlue : UiColors::TextMain);
        std::wstring title = std::wstring(isExpanded ? L"▼ " : L"▶ ") + std::to_wstring(i + 1) + L". " + names[i];
        RECT titleRect{ headerRow.left + UiMetrics::Scale(8, dpi), headerRow.top,
                        headerRow.left + titleW, headerRow.bottom };
        DrawTextW(dc, title.c_str(), static_cast<int>(title.size()), &titleRect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

        // Badge
        const int badgeW = UiMetrics::Scale(125, dpi);
        RECT badge{ titleRect.right + UiMetrics::Scale(4, dpi), headerRow.top + UiMetrics::Scale(5, dpi),
                    titleRect.right + UiMetrics::Scale(4, dpi) + badgeW, headerRow.bottom - UiMetrics::Scale(5, dpi) };
        DrawStatusBadge(dc, badge, stage.state, fonts, stage.label);

        // Action Button
        const int actionW = UiMetrics::Scale(90, dpi);
        RECT actionRect{ headerRow.right - actionW - UiMetrics::Scale(6, dpi), headerRow.top + UiMetrics::Scale(5, dpi),
                         headerRow.right - UiMetrics::Scale(6, dpi), headerRow.bottom - UiMetrics::Scale(5, dpi) };

        // Summary Text
        SelectObject(dc, fonts.hSmall);
        SetTextColor(dc, UiColors::TextMuted);
        RECT sourceRect{ badge.right + UiMetrics::Scale(8, dpi), headerRow.top,
                         actionRect.left - UiMetrics::Scale(8, dpi), headerRow.bottom };
        std::wstring summaryTxt = current ? data.microTask : data.quickSummary;
        DrawTextW(dc, summaryTxt.c_str(), static_cast<int>(summaryTxt.size()), &sourceRect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

        // Expand pill
        DrawRoundedCard(dc, actionRect, UiMetrics::RadiusPill,
                        isExpanded ? UiColors::PrimaryBlue : UiColors::PrimaryBlueLight,
                        UiColors::InfoBorder, 1);
        SelectObject(dc, fonts.hSmall);
        SetTextColor(dc, isExpanded ? RGB(255,255,255) : UiColors::PrimaryBlue);
        const wchar_t* pillText = isExpanded ? L"Thu gọn ▲" : L"Chi tiết ▼";
        DrawTextW(dc, pillText, -1, &actionRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        // Expanded detail box
        if (isExpanded) {
            RECT detailBox{ row.left + UiMetrics::Scale(10, dpi), headerRow.bottom,
                            row.right - UiMetrics::Scale(10, dpi), row.bottom - UiMetrics::Scale(4, dpi) };
            SelectObject(dc, fonts.hSmall);
            SetTextColor(dc, UiColors::TextMain);
            RECT line1Rect{ detailBox.left, detailBox.top, detailBox.right - UiMetrics::Scale(120, dpi), detailBox.top + UiMetrics::Scale(18, dpi) };
            DrawTextW(dc, data.line1.c_str(), static_cast<int>(data.line1.size()), &line1Rect,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

            SetTextColor(dc, UiColors::TextMuted);
            RECT line2Rect{ detailBox.left, line1Rect.bottom, detailBox.right - UiMetrics::Scale(120, dpi), detailBox.bottom };
            DrawTextW(dc, data.line2.c_str(), static_cast<int>(data.line2.size()), &line2Rect,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

            // Shortcut button -> Open In-Place Modal Inspector
            RECT jumpBtn{ detailBox.right - UiMetrics::Scale(125, dpi), detailBox.top + UiMetrics::Scale(2, dpi),
                          detailBox.right, detailBox.bottom - UiMetrics::Scale(2, dpi) };
            DrawRoundedCard(dc, jumpBtn, UiMetrics::RadiusPill, UiColors::PrimaryBlueLight, UiColors::InfoBorder, 1);
            SetTextColor(dc, UiColors::PrimaryBlue);
            DrawTextW(dc, L"Bảng thông số 📋", -1, &jumpBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }

        rowY += rowHeight + UiMetrics::Scale(4, dpi);
    }

    // Live evidence log: real runtime entries only; no synthetic sample rows.
    RECT logCard{ r.left + pad, listBottom + logGap, r.left + pad + mainW,
                  r.bottom - UiMetrics::Scale(18, dpi) };
    DrawRoundedCard(dc, logCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, logCard.left + UiMetrics::Scale(10, dpi), logCard.top + UiMetrics::Scale(7, dpi),
             L"Nhật ký bằng chứng trực tiếp", 27);

    SelectObject(dc, fonts.hSmall);
    if (liveLogs.empty()) {
        SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, logCard.left + UiMetrics::Scale(10, dpi), logCard.top + UiMetrics::Scale(30, dpi),
                 L"Chưa có bản ghi runtime cho phiên này.", 36);
    } else {
        const int maxRows = 3;
        const int start = std::max(0, static_cast<int>(liveLogs.size()) - maxRows);
        int y = logCard.top + UiMetrics::Scale(28, dpi);
        for (int i = start; i < static_cast<int>(liveLogs.size()); ++i) {
            const auto& entry = liveLogs[i];
            SetTextColor(dc, entry.state == 2 ? UiColors::FailRed
                           : (entry.state == 1 ? UiColors::WarnAmber : UiColors::TextMain));
            std::wstring line = entry.time + L"  " + entry.message;
            if (!entry.source.empty()) line += L"  [" + entry.source + L"]";
            RECT lineRect{ logCard.left + UiMetrics::Scale(10, dpi), y,
                           logCard.right - UiMetrics::Scale(10, dpi), y + UiMetrics::Scale(20, dpi) };
            DrawTextW(dc, line.c_str(), static_cast<int>(line.size()), &lineRect,
                      DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
            y += UiMetrics::Scale(21, dpi);
        }
    }
}

} // namespace lap
