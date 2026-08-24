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
    bool hasPass = false;
    bool hasUnsupported = false;
    for (auto state : states) {
        if (state == CanonicalUiState::Fail || state == CanonicalUiState::Error) return CanonicalUiState::Fail;
        if (state == CanonicalUiState::Warning) return CanonicalUiState::Warning;
        if (state == CanonicalUiState::PermissionDenied) return CanonicalUiState::PermissionDenied;
        if (state == CanonicalUiState::ProviderUnavailable) return CanonicalUiState::ProviderUnavailable;
        if (state == CanonicalUiState::ManualRequired) return CanonicalUiState::ManualRequired;
        if (state == CanonicalUiState::Incomplete) return CanonicalUiState::Incomplete;
        if (state == CanonicalUiState::Pass || state == CanonicalUiState::Good) hasPass = true;
        if (state == CanonicalUiState::Unsupported) hasUnsupported = true;
    }
    if (hasPass) return CanonicalUiState::Pass;
    if (hasUnsupported) return CanonicalUiState::Unsupported;
    return CanonicalUiState::NotTested;
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

void DrawDomainCard(HDC dc, const RECT& r, const wchar_t* code, const wchar_t* title,
                    const std::wstring& detail, CanonicalUiState state,
                    const UiFonts& fonts, int dpi, const std::wstring& overrideLabel = L"") {
    DrawRoundedCard(dc, r, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    RECT codeRect{ r.left + UiMetrics::Scale(8, dpi), r.top + UiMetrics::Scale(8, dpi),
                   r.left + UiMetrics::Scale(38, dpi), r.top + UiMetrics::Scale(31, dpi) };
    DrawRoundedCard(dc, codeRect, UiMetrics::RadiusSm, UiColors::PrimaryBlueLight, UiColors::InfoBorder, 1);
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::PrimaryBlue);
    DrawTextW(dc, code, -1, &codeRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    RECT titleRect{ codeRect.right + UiMetrics::Scale(7, dpi), r.top + UiMetrics::Scale(6, dpi),
                    r.right - UiMetrics::Scale(83, dpi), r.top + UiMetrics::Scale(27, dpi) };
    DrawTextW(dc, title, -1, &titleRect, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    RECT badgeRect{ r.right - UiMetrics::Scale(80, dpi), r.top + UiMetrics::Scale(7, dpi),
                    r.right - UiMetrics::Scale(7, dpi), r.top + UiMetrics::Scale(28, dpi) };
    DrawStatusBadge(dc, badgeRect, state, fonts, overrideLabel);

    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    RECT detailRect{ codeRect.right + UiMetrics::Scale(7, dpi), r.top + UiMetrics::Scale(30, dpi),
                     r.right - UiMetrics::Scale(7, dpi), r.bottom - UiMetrics::Scale(4, dpi) };
    DrawTextW(dc, detail.c_str(), static_cast<int>(detail.size()), &detailRect,
              DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
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

CanonicalUiState PhysicalSafetyState(const FunctionalTestSummary& summary) {
    bool any = false;
    bool allResolved = true;
    for (const auto& item : summary.items) {
        if (item.id.rfind(L"physical_", 0) != 0) continue;
        any = true;
        auto state = MapFunctionalStatus(item.status);
        if (state == CanonicalUiState::Fail) return CanonicalUiState::Fail;
        if (state == CanonicalUiState::Warning) return CanonicalUiState::Warning;
        if (state == CanonicalUiState::ManualRequired || state == CanonicalUiState::NotTested)
            allResolved = false;
    }
    if (!any) return CanonicalUiState::NotTested;
    return allResolved ? CanonicalUiState::Pass : CanonicalUiState::Incomplete;
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
        out.source = rep.hardware.stress.cpuBenchmark.baselineSource.empty()
            ? L"CPU identity + BuiltIn-FP-Mix-v1" : rep.hardware.stress.cpuBenchmark.baselineSource;
        out.detail = L"Danh tính CPU và microbenchmark có version/baseline";
        if (completed) {
            if (rep.hardware.cpuName.empty()) {
                out.state = CanonicalUiState::Incomplete;
                out.label = L"THIẾU DỮ LIỆU";
            } else if (rep.hardware.stress.cpuBenchmark.verdict == L"BELOW BASELINE") {
                out.state = CanonicalUiState::Warning;
                out.label = L"DƯỚI BASELINE";
            } else {
                out.state = CanonicalUiState::Info;
                out.label = rep.hardware.stress.cpuBenchmark.verdict == L"NOT SCORED"
                    ? L"ĐÃ ĐO — CHƯA CÓ BASELINE" : L"ĐÃ ĐO";
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
            out.state = StorageHealthState(rep);
            if (out.state == CanonicalUiState::Good) out.label = L"BẰNG CHỨNG SỨC KHỎE HỢP LỆ";
            else if (out.state == CanonicalUiState::Fail) out.label = L"PHÁT HIỆN LỖI SỨC KHỎE";
            else if (out.state == CanonicalUiState::ProviderUnavailable) out.label = L"THIẾU PROVIDER";
            else out.label = L"THIẾU DỮ LIỆU";
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
                out.label = L"KHÔNG TRUY VẤN ĐƯỢC";
            } else {
                const long long events = rep.hardware.events.whea + rep.hardware.events.disk +
                    rep.hardware.events.stornvme + rep.hardware.events.display + rep.hardware.events.bugCheck;
                out.state = events > 0 ? CanonicalUiState::Warning : CanonicalUiState::Info;
                out.label = events > 0 ? L"CÓ SỰ KIỆN CẦN XEM" : L"ĐÃ TRUY VẤN";
            }
        }
        break;
    }
    case 9: {
        out.source = L"LapSure stress journal + event delta + telemetry";
        out.detail = L"Stress CPU/RAM/GPU và đánh giá ổn định theo bằng chứng phát sinh";
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
                out.label = L"ỔN ĐỊNH TRONG PHẠM VI TEST";
            } else {
                out.state = CanonicalUiState::Incomplete;
                out.label = L"BẰNG CHỨNG CHƯA ĐỦ";
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
    case 2: return L"CPU & Microbench";
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

    const int btnW = UiMetrics::Scale(230, dpi);
    const int btnH = UiMetrics::Scale(40, dpi);
    RECT startBtn{ r.right - btnW - pad, modeY - UiMetrics::Scale(2, dpi), r.right - pad,
                   modeY - UiMetrics::Scale(2, dpi) + btnH };
    const COLORREF buttonColor = running ? UiColors::FailRed : UiColors::PrimaryBlue;
    DrawRoundedCard(dc, startBtn, btnH / 2, buttonColor, buttonColor, 1);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextWhite);
    const std::wstring startText = running ? L"DỪNG KIỂM TRA" : (auditReady ? L"KIỂM TRA LẠI" : L"BẮT ĐẦU KIỂM TRA TỰ ĐỘNG");
    DrawTextW(dc, startText.c_str(), static_cast<int>(startText.size()), &startBtn,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    if (focusIndex == 2) DrawFocusRing(dc, startBtn, btnH / 2);

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
    warnings.value = auditReady ? std::to_wstring(decision.warnings) : L"—";
    warnings.note = L"Từ findings/decision engine";
    warnings.state = auditReady ? (decision.warnings > 0 ? CanonicalUiState::Warning : CanonicalUiState::Info)
                                : CanonicalUiState::NotTested;
    RECT k3{ k2.right + gap, kpiY, k2.right + gap + kpiW, kpiY + kpiH };
    DrawMetricCard(dc, k3, warnings, fonts, dpi);

    MetricCardConfig critical;
    critical.label = L"LỖI NGHIÊM TRỌNG";
    critical.value = auditReady ? std::to_wstring(decision.criticalFails) : L"—";
    critical.note = L"Lỗi có thể chặn quyết định mua";
    critical.state = auditReady ? (decision.criticalFails > 0 ? CanonicalUiState::Fail : CanonicalUiState::Info)
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
    CanonicalUiState profileState = rep.factoryExact ? CanonicalUiState::Pass
        : (!rep.profileSource.empty() ? CanonicalUiState::Changed : CanonicalUiState::NotTested);
    RECT profileBadge{ factoryCard.left + UiMetrics::Scale(12, dpi), factoryCard.bottom - UiMetrics::Scale(27, dpi),
                       factoryCard.right - UiMetrics::Scale(12, dpi), factoryCard.bottom - UiMetrics::Scale(6, dpi) };
    DrawStatusBadge(dc, profileBadge, profileState, fonts,
                    rep.factoryExact ? L"HỒ SƠ CHÍNH XÁC"
                                     : (!rep.profileSource.empty() ? L"HỒ SƠ THAM CHIẾU" : L"CHƯA CÓ HỒ SƠ"));

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
    TextOutW(dc, r.left + pad, gridY, L"Tình trạng theo nhóm", 19);

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

    CanonicalUiState keyboardState = FunctionalState(rep.hardware.stress.functional, L"keyboard_function");
    if (keyboardState == CanonicalUiState::Pass &&
        HasFunctionalEvidence(rep.hardware.stress.functional, L"touchpad_presence")) {
        keyboardState = CanonicalUiState::ManualRequired;
    }

    CanonicalUiState audioCameraState = MergeFunctional({
        FunctionalState(rep.hardware.stress.functional, L"speaker_function"),
        FunctionalState(rep.hardware.stress.functional, L"camera_function"),
        FunctionalState(rep.hardware.stress.functional, L"mic_function")
    });

    CanonicalUiState networkState = MergeFunctional({
        FunctionalState(rep.hardware.stress.functional, L"wifi_function"),
        FunctionalState(rep.hardware.stress.functional, L"bluetooth_function")
    });
    std::wstring networkLabel;
    if (networkState == CanonicalUiState::NotTested &&
        (HasFunctionalEvidence(rep.hardware.stress.functional, L"wifi_presence") ||
         HasFunctionalEvidence(rep.hardware.stress.functional, L"bluetooth_presence"))) {
        networkState = CanonicalUiState::Info;
        networkLabel = L"ĐÃ NHẬN DIỆN";
    }

    CanonicalUiState portsState = rep.hardware.stress.portPower.overall == L"PASS"
        ? CanonicalUiState::Pass
        : (rep.hardware.stress.portPower.overall == L"FAIL" ? CanonicalUiState::Fail : CanonicalUiState::NotTested);

    CanonicalUiState stabilityState = decision.stability == L"PASS" ? CanonicalUiState::Good
        : (decision.stability == L"FAIL" ? CanonicalUiState::Fail
        : (decision.stability == L"PARTIAL" ? CanonicalUiState::Incomplete : CanonicalUiState::NotTested));

    CanonicalUiState eventState = CanonicalUiState::NotTested;
    if (rep.hardware.events.querySucceeded) {
        const long long eventCount = rep.hardware.events.whea + rep.hardware.events.disk +
            rep.hardware.events.stornvme + rep.hardware.events.display + rep.hardware.events.bugCheck;
        eventState = eventCount > 0 ? CanonicalUiState::Warning : CanonicalUiState::Info;
    } else if (auditCompletedItems >= 8) {
        eventState = CanonicalUiState::ProviderUnavailable;
    }

    CanonicalUiState profileDomainState = rep.factoryExact ? CanonicalUiState::Pass
        : (!rep.profileSource.empty() ? CanonicalUiState::Changed : CanonicalUiState::NotTested);
    CanonicalUiState coverageDomainState = coverage.requiredTotal > 0 && coverage.requiredMissing == 0
        ? CanonicalUiState::Pass : CanonicalUiState::Incomplete;

    struct Domain {
        const wchar_t* code;
        const wchar_t* title;
        std::wstring detail;
        CanonicalUiState state;
        std::wstring label;
    };

    std::vector<Domain> domains = {
        { L"ID", L"Nhận diện hệ thống", CoverageDetail(coverage, L"identity", L"Model, Service Tag, CPU, BIOS"), CoverageState(coverage, L"identity"), L"" },
        { L"RAM", L"Bộ nhớ (RAM)", CoverageDetail(coverage, L"memory", L"Dung lượng và module"), ramState, ramLabel },
        { L"SSD", L"Lưu trữ", StorageHealthDetail(rep), StorageHealthState(rep), L"" },
        { L"BAT", L"Pin & Năng lượng", BatteryEvidenceDetail(rep), BatteryEvidenceState(rep), L"" },
        { L"GPU", L"Đồ họa (GPU)", L"Identity/VRAM/driver; chức năng cần bằng chứng riêng", gpuState, gpuLabel },
        { L"LCD", L"Hiển thị", L"EDID + kiểm tra màu/khuyết tật thực tế", displayState, L"" },
        { L"KEY", L"Bàn phím & Touchpad", L"Không suy chức năng touchpad từ presence", keyboardState, L"" },
        { L"AV", L"Âm thanh & Camera", L"Loa trái/phải, camera frame, microphone PCM", audioCameraState, L"" },
        { L"NET", L"Mạng & Kết nối", L"Wi-Fi/Bluetooth function tách khỏi radio presence", networkState, networkLabel },
        { L"PORT", L"Cổng & Nguồn", L"Cắm/rút thiết bị thật + nguồn sạc", portsState, L"" },
        { L"STB", L"Stress & Ổn định", L"Stress + event delta; thermal là dimension riêng", stabilityState, L"" },
        { L"LOG", L"Nhật ký & Sự kiện", L"Lịch sử là bằng chứng; không tự suy lỗi hiện tại", eventState, L"" },
        { L"OEM", L"Hồ sơ & Đối chiếu", L"Factory mismatch tách khỏi health", profileDomainState, L"" },
        { L"COV", L"Bằng chứng & Tin cậy", std::to_wstring(coverage.requiredComplete) + L"/" + std::to_wstring(coverage.requiredTotal) + L" miền bắt buộc hoàn tất", coverageDomainState, L"" }
    };

    const int cardCols = 4;
    const int cellW = (mainW - (cardCols - 1) * gap) / cardCols;
    const int cellH = UiMetrics::Scale(54, dpi);
    const int startGridY = gridY + UiMetrics::Scale(26, dpi);
    for (size_t i = 0; i < domains.size(); ++i) {
        const int row = static_cast<int>(i) / cardCols;
        const int col = static_cast<int>(i) % cardCols;
        const int x = r.left + pad + col * (cellW + gap);
        const int y = startGridY + row * (cellH + UiMetrics::Scale(8, dpi));
        RECT card{ x, y, x + cellW, y + cellH };
        DrawDomainCard(dc, card, domains[i].code, domains[i].title, domains[i].detail,
                       domains[i].state, fonts, dpi, domains[i].label);
    }

    const int infoY = startGridY + 4 * (cellH + UiMetrics::Scale(8, dpi)) + UiMetrics::Scale(4, dpi);
    if (infoY + UiMetrics::Scale(54, dpi) < r.bottom - UiMetrics::Scale(8, dpi)) {
        RECT info{ r.left + pad, infoY, r.left + pad + mainW, infoY + UiMetrics::Scale(54, dpi) };
        DrawRoundedCard(dc, info, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
        const int chipGap = UiMetrics::Scale(6, dpi);
        const int chipW = ((info.right - info.left) - UiMetrics::Scale(24, dpi) - 3 * chipGap) / 4;
        const std::wstring modelValue = rep.model.empty() ? L"Không xác định" : rep.model;
        const std::wstring cpuValue = rep.hardware.cpuName.empty() ? L"Không xác định" : rep.hardware.cpuName;
        const std::wstring ramValue = rep.hardware.installedRamBytes > 0
            ? std::to_wstring(rep.hardware.installedRamBytes / (1024ULL * 1024ULL * 1024ULL)) + L" GB"
            : L"Không xác định";
        const std::wstring storageValue = rep.hardware.storage.empty() ? L"Không xác định" : rep.hardware.storage.front().model;
        int x = info.left + UiMetrics::Scale(6, dpi);
        const int chipY = info.top + UiMetrics::Scale(6, dpi);
        DrawIdentityChip(dc, { x, chipY, x + chipW, chipY + UiMetrics::Scale(42, dpi) }, L"MODEL", modelValue, fonts, dpi);
        x += chipW + chipGap;
        DrawIdentityChip(dc, { x, chipY, x + chipW, chipY + UiMetrics::Scale(42, dpi) }, L"CPU", cpuValue, fonts, dpi);
        x += chipW + chipGap;
        DrawIdentityChip(dc, { x, chipY, x + chipW, chipY + UiMetrics::Scale(42, dpi) }, L"RAM", ramValue, fonts, dpi);
        x += chipW + chipGap;
        DrawIdentityChip(dc, { x, chipY, x + chipW, chipY + UiMetrics::Scale(42, dpi) }, L"LƯU TRỮ", storageValue, fonts, dpi);
    }
}

void RenderScreenS04_AutoAudit(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                               const std::wstring& selectedMode, bool running, bool paused, int auditCompletedItems,
                               int auditTotalItems, int auditCurrentStage, int auditElapsedSec,
                               const std::vector<LiveLogEntry>& liveLogs, int focusIndex) {
    (void)focusIndex;
    const int pad = UiMetrics::Scale(24, dpi);
    const int rightPanelW = std::clamp((r.right - r.left) * 26 / 100,
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
    RECT timeCard{ rightX, topY, r.right - UiMetrics::Scale(20, dpi), r.top + UiMetrics::Scale(160, dpi) };
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

    RECT guideCard{ rightX, timeCard.bottom + UiMetrics::Scale(10, dpi),
                    r.right - UiMetrics::Scale(20, dpi), r.bottom - UiMetrics::Scale(16, dpi) };
    DrawRoundedCard(dc, guideCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, guideCard.left + UiMetrics::Scale(12, dpi), guideCard.top + UiMetrics::Scale(10, dpi),
             L"Bước tiếp theo", 13);
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    std::wstring nextText;
    if (running) nextText = paused ? L"Tiếp tục khi sẵn sàng hoặc hủy phiên." : L"Theo dõi bằng chứng; có thể tạm dừng hoặc hủy.";
    else if (auditCompletedItems >= auditTotalItems && auditTotalItems > 0)
        nextText = L"Kiểm tra tự động đã xong. Chưa được kết luận mua nếu chức năng/cổng/an toàn còn thiếu.";
    else nextText = L"Chọn chế độ rồi bắt đầu thu thập bằng chứng tự động.";
    RECT nextTextRect{ guideCard.left + UiMetrics::Scale(12, dpi), guideCard.top + UiMetrics::Scale(34, dpi),
                       guideCard.right - UiMetrics::Scale(12, dpi), guideCard.top + UiMetrics::Scale(90, dpi) };
    DrawTextW(dc, nextText.c_str(), static_cast<int>(nextText.size()), &nextTextRect,
              DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);

    const auto coverage = BuildCoverageSnapshot(rep);
    const std::wstring coverageText = L"Required coverage toàn quy trình: " +
        std::to_wstring(coverage.requiredComplete) + L"/" + std::to_wstring(coverage.requiredTotal);
    TextOutW(dc, guideCard.left + UiMetrics::Scale(12, dpi), guideCard.top + UiMetrics::Scale(98, dpi),
             coverageText.c_str(), static_cast<int>(coverageText.size()));

    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::WarnAmber);
    RECT invariantRect{ guideCard.left + UiMetrics::Scale(12, dpi), guideCard.top + UiMetrics::Scale(120, dpi),
                        guideCard.right - UiMetrics::Scale(12, dpi), guideCard.top + UiMetrics::Scale(176, dpi) };
    DrawTextW(dc,
              L"Presence ≠ functionality. Unknown / unsupported / provider unavailable không được chuyển thành PASS.",
              -1, &invariantRect, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);

    const int actionH = UiMetrics::Scale(36, dpi);
    RECT actionBtn{ guideCard.left + UiMetrics::Scale(12, dpi), guideCard.bottom - actionH - UiMetrics::Scale(12, dpi),
                    guideCard.right - UiMetrics::Scale(12, dpi), guideCard.bottom - UiMetrics::Scale(12, dpi) };
    const COLORREF actionColor = running ? UiColors::FailRed : UiColors::PrimaryBlue;
    DrawRoundedCard(dc, actionBtn, UiMetrics::RadiusSm, actionColor, actionColor, 1);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextWhite);
    const std::wstring actionLabel = running ? L"HỦY KIỂM TRA"
        : (auditCompletedItems == 0 ? L"BẮT ĐẦU KIỂM TRA" : L"CHẠY LẠI KIỂM TRA");
    DrawTextW(dc, actionLabel.c_str(), static_cast<int>(actionLabel.size()), &actionBtn,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Main automatic stage list.
    const int listTop = progressCard.bottom + UiMetrics::Scale(10, dpi);
    const int logHeight = UiMetrics::Scale(116, dpi);
    const int logGap = UiMetrics::Scale(9, dpi);
    const int listBottom = r.bottom - logHeight - logGap - UiMetrics::Scale(18, dpi);
    RECT listCard{ r.left + pad, listTop, r.left + pad + mainW, listBottom };
    DrawRoundedCard(dc, listCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    const int rowGap = UiMetrics::Scale(4, dpi);
    const int availableRows = std::max(UiMetrics::Scale(306, dpi), listCard.bottom - listCard.top - UiMetrics::Scale(12, dpi));
    const int rowHeight = std::clamp((availableRows - rowGap * 8) / 9,
                                     UiMetrics::Scale(32, dpi), UiMetrics::Scale(44, dpi));
    int rowY = listCard.top + UiMetrics::Scale(6, dpi);
    const wchar_t* names[9] = {
        L"Nhận diện hệ thống", L"CPU & Microbench", L"Bộ nhớ (RAM)", L"Lưu trữ",
        L"Đồ họa (GPU)", L"Pin & Năng lượng", L"Mạng & Kết nối", L"Nhật ký & Forensics",
        L"Stress & Ổn định"
    };

    for (int i = 0; i < 9; ++i) {
        const auto stage = BuildAutoStage(i + 1, rep, running, paused, auditCompletedItems, auditCurrentStage);
        const bool current = running && auditCurrentStage == i + 1;
        RECT row{ listCard.left + UiMetrics::Scale(6, dpi), rowY,
                  listCard.right - UiMetrics::Scale(6, dpi), rowY + rowHeight };
        DrawRoundedCard(dc, row, UiMetrics::RadiusSm,
                        current ? UiColors::PrimaryBlueLight : UiColors::CardBg,
                        current ? UiColors::InfoBorder : UiColors::CardBorder,
                        current ? 2 : 1);

        const int titleW = std::max(UiMetrics::Scale(185, dpi), (row.right - row.left) * 31 / 100);
        SelectObject(dc, fonts.hBodyBold);
        SetTextColor(dc, current ? UiColors::PrimaryBlue : UiColors::TextMain);
        std::wstring title = std::to_wstring(i + 1) + L". " + names[i];
        RECT titleRect{ row.left + UiMetrics::Scale(8, dpi), row.top,
                        row.left + titleW, row.bottom };
        DrawTextW(dc, title.c_str(), static_cast<int>(title.size()), &titleRect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

        const int badgeW = UiMetrics::Scale(125, dpi);
        RECT badge{ titleRect.right + UiMetrics::Scale(4, dpi), row.top + UiMetrics::Scale(6, dpi),
                    titleRect.right + UiMetrics::Scale(4, dpi) + badgeW, row.bottom - UiMetrics::Scale(6, dpi) };
        DrawStatusBadge(dc, badge, stage.state, fonts, stage.label);

        SelectObject(dc, fonts.hSmall);
        SetTextColor(dc, UiColors::TextMuted);
        RECT sourceRect{ badge.right + UiMetrics::Scale(7, dpi), row.top,
                         row.right - UiMetrics::Scale(70, dpi), row.bottom };
        DrawTextW(dc, stage.source.c_str(), static_cast<int>(stage.source.size()), &sourceRect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

        RECT durationRect{ row.right - UiMetrics::Scale(65, dpi), row.top,
                           row.right - UiMetrics::Scale(6, dpi), row.bottom };
        DrawTextW(dc, stage.duration.c_str(), static_cast<int>(stage.duration.size()), &durationRect,
                  DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        rowY += rowHeight + rowGap;
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
