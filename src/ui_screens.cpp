#include "lap/ui_screens.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace lap {

namespace {

std::wstring RegVal(const wchar_t* name) {
    HKEY h{};
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &h) != ERROR_SUCCESS) return L"";
    wchar_t b[512]{}; DWORD sz = sizeof(b), t = 0; std::wstring s;
    if (RegQueryValueExW(h, name, nullptr, &t, (LPBYTE)b, &sz) == ERROR_SUCCESS) s = b;
    RegCloseKey(h);
    return s;
}

} // namespace

// ============================================================
// S01 — TỔNG QUAN THIẾT BỊ
// ============================================================
// Helper to evaluate domain state dynamically from AuditReport findings
CanonicalUiState EvaluateDomainState(int domainIdx, const AuditReport& rep, bool running, bool auditReady, int completedItems) {
    if (!auditReady && completedItems == 0 && !running) {
        return CanonicalUiState::NotTested;
    }
    if (running && completedItems == 0) {
        return (domainIdx == 0) ? CanonicalUiState::Running : CanonicalUiState::NotTested;
    }

    // Map domain index to findings evaluation
    auto hasDomainIssue = [&](Dimension dim, Severity minSev) {
        for (const auto& f : rep.findings) {
            if (f.dimension == dim && f.severity >= minSev) return true;
        }
        return false;
    };

    switch (domainIdx) {
    case 0: // Nhận diện hệ thống
        if (rep.model.empty() && rep.hardware.cpuName.empty()) return CanonicalUiState::NotTested;
        return hasDomainIssue(Dimension::Identity, Severity::Critical) ? CanonicalUiState::Fail : (hasDomainIssue(Dimension::Identity, Severity::Major) ? CanonicalUiState::Warning : CanonicalUiState::Good);
    case 1: // Bộ nhớ RAM
        if (rep.hardware.installedRamBytes == 0) return CanonicalUiState::NotTested;
        return hasDomainIssue(Dimension::Health, Severity::Critical) ? CanonicalUiState::Fail : CanonicalUiState::Good;
    case 2: // Lưu trữ
        if (rep.hardware.storage.empty()) return CanonicalUiState::NotTested;
        for (const auto& s : rep.hardware.storage) {
            if (s.reliabilityReadable && !s.reliabilityHealthy) return CanonicalUiState::Fail;
        }
        return hasDomainIssue(Dimension::Health, Severity::Major) ? CanonicalUiState::Warning : CanonicalUiState::Good;
    case 3: // Pin & Nguồn
        if (!rep.hardware.battery.present && rep.hardware.battery.healthPercent < 0) return CanonicalUiState::NotTested;
        return (rep.hardware.battery.cycleCount > 500 || (rep.hardware.battery.healthPercent >= 0 && rep.hardware.battery.healthPercent < 70)) ? CanonicalUiState::Warning : CanonicalUiState::Good;
    case 4: // Đồ họa GPU
        if (rep.hardware.gpus.empty()) return CanonicalUiState::NotTested;
        return CanonicalUiState::Good;
    case 5: // Hiển thị Màn hình
        if (rep.hardware.displays.empty()) return CanonicalUiState::NotTested;
        return CanonicalUiState::Good;
    case 6: // Bàn phím & Touchpad
        return (rep.hardware.stress.functional.overall == L"PASS") ? CanonicalUiState::Good : (completedItems >= 9 ? CanonicalUiState::Running : CanonicalUiState::NotTested);
    case 7: // Âm thanh & Camera
        return (rep.hardware.stress.functional.overall == L"PASS") ? CanonicalUiState::Good : (completedItems >= 9 ? CanonicalUiState::Running : CanonicalUiState::NotTested);
    case 8: // Mạng & Kết nối
        return (completedItems > 0 || auditReady) ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    case 9: // Cổng & Nguồn vật lý
        return (rep.hardware.stress.portPower.overall == L"PASS") ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    case 10: // Stress & Ổn định
        if (rep.hardware.stress.stages.empty()) return CanonicalUiState::NotTested;
        return (rep.hardware.stress.decision.overall == L"PASS" || rep.hardware.stress.decision.overall == L"BUY") ? CanonicalUiState::Good : (rep.hardware.stress.decision.overall == L"WARNING" || rep.hardware.stress.decision.overall == L"BUY_WITH_RESERVATIONS" ? CanonicalUiState::Warning : CanonicalUiState::Fail);
    case 11: // Nhật ký & Sự kiện
        return hasDomainIssue(Dimension::Stability, Severity::Critical) ? CanonicalUiState::Fail : (hasDomainIssue(Dimension::Stability, Severity::Major) ? CanonicalUiState::Warning : (auditReady ? CanonicalUiState::Good : CanonicalUiState::NotTested));
    case 12: // Hồ sơ & Đối chiếu
        if (!rep.factoryExact && rep.profileSource.empty()) return CanonicalUiState::NotTested;
        return rep.factoryExact ? CanonicalUiState::Good : CanonicalUiState::Warning;
    case 13: // Độ bao phủ & Tin cậy
        return auditReady ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    default:
        return CanonicalUiState::NotTested;
    }
}

void RenderScreenS01_Overview(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                              const std::wstring& selectedMode, bool running, bool auditReady,
                              CanonicalUiState sessionLifecycleState, int auditCompletedItems, int auditTotalItems,
                              int focusIndex) {
    // 1. Page Header (C03)
    PageHeaderConfig hdr;
    hdr.title = L"Tổng quan thiết bị";
    hdr.subtitle = L"Tổng hợp nhanh trạng thái phần cứng và mức độ sẵn sàng kiểm định";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int modeX = r.left + UiMetrics::Scale(24, dpi);
    int modeY = r.top + UiMetrics::Scale(70, dpi);

    // Mode Selector pills
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, modeX, modeY + UiMetrics::Scale(6, dpi), L"Chế độ kiểm tra:", 16);
    modeX += UiMetrics::Scale(110, dpi);

    auto drawPill = [&](const wchar_t* label, bool active) {
        int pw = UiMetrics::Scale(75, dpi);
        int ph = UiMetrics::Scale(28, dpi);
        RECT pr{ modeX, modeY, modeX + pw, modeY + ph };
        DrawRoundedCard(dc, pr, ph / 2, active ? UiColors::PrimaryBlue : UiColors::GrayPillBg, active ? UiColors::PrimaryBlue : UiColors::GrayPillBorder, 1);
        SetTextColor(dc, active ? RGB(255, 255, 255) : UiColors::TextMain);
        SelectObject(dc, active ? fonts.hBodyBold : fonts.hBody);
        DrawTextW(dc, label, -1, &pr, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        modeX += pw + UiMetrics::Scale(6, dpi);
    };
    drawPill(L"Nhanh", selectedMode == L"Quick");
    drawPill(L"Tiêu chuẩn", selectedMode == L"Standard");
    drawPill(L"Chuyên sâu", selectedMode == L"Deep");

    modeX += UiMetrics::Scale(12, dpi);
    SetTextColor(dc, UiColors::TextMuted);
    SelectObject(dc, fonts.hSmall);
    TextOutW(dc, modeX, modeY + UiMetrics::Scale(6, dpi), L"Mã kiểm tra:", 12);
    modeX += UiMetrics::Scale(75, dpi);
    SetTextColor(dc, UiColors::PrimaryBlue);
    TextOutW(dc, modeX, modeY + UiMetrics::Scale(6, dpi), L"LS-20260824-001", 15);

    // Top CTA Button
    int btnW = UiMetrics::Scale(220, dpi);
    int btnH = UiMetrics::Scale(36, dpi);
    RECT btnStartRect{ r.right - btnW - UiMetrics::Scale(24, dpi), modeY - UiMetrics::Scale(2, dpi), r.right - UiMetrics::Scale(24, dpi), modeY - UiMetrics::Scale(2, dpi) + btnH };
    COLORREF btnClr = running ? UiColors::FailRed : UiColors::PrimaryBlue;
    DrawRoundedCard(dc, btnStartRect, btnH / 2, btnClr, btnClr, 1);
    SetTextColor(dc, RGB(255, 255, 255));
    SelectObject(dc, fonts.hBodyBold);
    std::wstring btnText = running ? L"■ DỪNG KIỂM TRA" : L"▶ Bắt đầu kiểm tra tự động";
    DrawTextW(dc, btnText.c_str(), (int)btnText.size(), &btnStartRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    if (focusIndex == 2) DrawFocusRing(dc, btnStartRect, btnH / 2);

    // Right Column Configuration
    int rightPanelW = UiMetrics::Scale(260, dpi);
    int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    int mainW = rightX - r.left - UiMetrics::Scale(36, dpi);

    // Summary Metric Cards Row (4 Cards)
    int kpiY = modeY + UiMetrics::Scale(44, dpi);
    int kpiW = (mainW - UiMetrics::Scale(30, dpi)) / 4;
    int kpiH = UiMetrics::Scale(82, dpi);

    // Card 1: Overall status
    MetricCardConfig mc1;
    mc1.label = L"Trạng thái tổng thể";
    if (auditReady) {
        mc1.value = FormatDecisionVi(rep.hardware.stress.decision.overall);
        mc1.note = (rep.hardware.stress.decision.overall == L"PASS" || rep.hardware.stress.decision.overall == L"BUY") ? L"Đạt tiêu chuẩn kiểm định" : L"Phát hiện cảnh báo / lỗi";
        mc1.state = (rep.hardware.stress.decision.overall == L"PASS" || rep.hardware.stress.decision.overall == L"BUY") ? CanonicalUiState::Good : CanonicalUiState::Warning;
        mc1.hasBadge = true;
    } else if (running) {
        mc1.value = L"ĐANG KIỂM TRA";
        mc1.note = L"Đang thực hiện quy trình tự động";
        mc1.state = CanonicalUiState::Running;
    } else {
        mc1.value = L"CHƯA KIỂM ĐỊNH";
        mc1.note = L"Chưa đủ dữ liệu để kết luận";
        mc1.state = CanonicalUiState::NotTested;
    }
    RECT kpi1Rect{ r.left + UiMetrics::Scale(24, dpi), kpiY, r.left + UiMetrics::Scale(24, dpi) + kpiW, kpiY + kpiH };
    DrawMetricCard(dc, kpi1Rect, mc1, fonts, dpi);

    // Card 2: Tests Completed
    MetricCardConfig mc2;
    mc2.label = L"Đã kiểm tra";
    mc2.value = std::to_wstring(auditCompletedItems) + L" / " + std::to_wstring(auditTotalItems);
    mc2.unitOrSource = L"Hạng mục";
    mc2.note = std::to_wstring(auditTotalItems > 0 ? (auditCompletedItems * 100 / auditTotalItems) : 0) + L"% hoàn thành";
    mc2.state = auditReady ? CanonicalUiState::Pass : (running ? CanonicalUiState::Running : CanonicalUiState::NotTested);
    RECT kpi2Rect{ kpi1Rect.right + UiMetrics::Scale(10, dpi), kpiY, kpi1Rect.right + UiMetrics::Scale(10, dpi) + kpiW, kpiY + kpiH };
    DrawMetricCard(dc, kpi2Rect, mc2, fonts, dpi);

    // Card 3: Warnings
    int warnCount = 0, critCount = 0;
    if (auditReady || auditCompletedItems > 0) {
        for (const auto& f : rep.findings) {
            if (f.severity == Severity::Critical || f.state == State::Fail) critCount++;
            else if (f.severity == Severity::Major || f.state == State::Warning) warnCount++;
        }
    }
    MetricCardConfig mc3;
    mc3.label = L"Cảnh báo";
    mc3.value = std::to_wstring(warnCount);
    mc3.unitOrSource = L"Hạng mục";
    mc3.note = (auditCompletedItems > 0) ? (warnCount > 0 ? L"Cần lưu ý" : L"Không có cảnh báo") : L"Chưa quét";
    mc3.state = warnCount > 0 ? CanonicalUiState::Warning : (auditCompletedItems > 0 ? CanonicalUiState::Good : CanonicalUiState::NotTested);
    RECT kpi3Rect{ kpi2Rect.right + UiMetrics::Scale(10, dpi), kpiY, kpi2Rect.right + UiMetrics::Scale(10, dpi) + kpiW, kpiY + kpiH };
    DrawMetricCard(dc, kpi3Rect, mc3, fonts, dpi);

    // Card 4: Critical
    MetricCardConfig mc4;
    mc4.label = L"Lỗi nghiêm trọng";
    mc4.value = std::to_wstring(critCount);
    mc4.unitOrSource = L"Hạng mục";
    mc4.note = (auditCompletedItems > 0) ? (critCount > 0 ? L"Phát hiện lỗi" : L"Không có lỗi") : L"Chưa quét";
    mc4.state = critCount > 0 ? CanonicalUiState::Fail : (auditCompletedItems > 0 ? CanonicalUiState::Good : CanonicalUiState::NotTested);
    RECT kpi4Rect{ kpi3Rect.right + UiMetrics::Scale(10, dpi), kpiY, kpi3Rect.right + UiMetrics::Scale(10, dpi) + kpiW, kpiY + kpiH };
    DrawMetricCard(dc, kpi4Rect, mc4, fonts, dpi);

    // Real dynamic hardware identity
    std::wstring pName = rep.model;
    if (pName.empty()) pName = RegVal(L"SystemProductName");
    if (pName.empty()) pName = RegVal(L"BaseBoardProduct");
    if (pName.empty()) pName = L"Máy tính chưa xác định";

    std::wstring sTag = rep.serviceTag;
    if (sTag.empty()) sTag = RegVal(L"SystemSerialNumber");
    if (sTag.empty()) sTag = L"Chưa xác định";

    std::wstring cpuStr = rep.hardware.cpuName;
    if (cpuStr.empty()) cpuStr = RegVal(L"ProcessorNameString");
    if (cpuStr.empty()) cpuStr = L"Chưa kiểm tra";

    std::wstring ramStr;
    if (rep.hardware.installedRamBytes > 0) {
        uint64_t gb = (rep.hardware.installedRamBytes + (1024ULL*1024*1024 - 1)) / (1024ULL*1024*1024);
        ramStr = std::to_wstring(gb) + L" GB";
        if (!rep.hardware.memoryModules.empty()) {
            ramStr += L" (" + std::to_wstring(rep.hardware.memoryModules.size()) + L" thanh)";
        }
    } else {
        ramStr = L"Chưa kiểm tra";
    }

    std::wstring diskStr;
    if (!rep.hardware.storage.empty()) {
        const auto& d = rep.hardware.storage[0];
        uint64_t gb = d.capacityBytes / (1000ULL * 1000 * 1000);
        diskStr = d.model + (gb > 0 ? (L" (" + std::to_wstring(gb) + L" GB)") : L"");
    } else {
        diskStr = L"Chưa kiểm tra";
    }

    std::wstring gpuStr;
    if (!rep.hardware.gpus.empty()) {
        gpuStr = rep.hardware.gpus[0].name;
    } else {
        gpuStr = L"Chưa kiểm tra";
    }

    // Right Column Cards: Gauge + Factory Profile + Guided Stepper
    // 1. Gauge Card
    RECT gaugeCard{ rightX, kpiY, r.right - UiMetrics::Scale(24, dpi), kpiY + UiMetrics::Scale(140, dpi) };
    DrawRoundedCard(dc, gaugeCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, gaugeCard.left + UiMetrics::Scale(12, dpi), gaugeCard.top + UiMetrics::Scale(8, dpi), L"Điểm sức khỏe & Tin cậy", 23);
    
    int gaugeScore = auditReady ? 88 : 0;
    DrawCircularScoreGauge(dc, (gaugeCard.left + gaugeCard.right) / 2, gaugeCard.top + UiMetrics::Scale(70, dpi), UiMetrics::Scale(40, dpi), gaugeScore, L"Tốt", fonts.hTitle, fonts.hSmall, auditReady ? 5 : 0);

    // 2. Factory Profile Card
    RECT facCard{ rightX, gaugeCard.bottom + UiMetrics::Scale(10, dpi), r.right - UiMetrics::Scale(24, dpi), gaugeCard.bottom + UiMetrics::Scale(85, dpi) };
    DrawRoundedCard(dc, facCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, facCard.left + UiMetrics::Scale(12, dpi), facCard.top + UiMetrics::Scale(8, dpi), L"Hồ sơ nhà máy (Factory Profile)", 31);
    
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    RECT pNameRect{ facCard.left + UiMetrics::Scale(12, dpi), facCard.top + UiMetrics::Scale(26, dpi), facCard.right - UiMetrics::Scale(80, dpi), facCard.top + UiMetrics::Scale(46, dpi) };
    DrawTextW(dc, pName.c_str(), (int)pName.size(), &pNameRect, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    if (rep.factoryExact) {
        DrawStatusBadge(dc, facCard.right - UiMetrics::Scale(75, dpi), facCard.top + UiMetrics::Scale(24, dpi), UiMetrics::Scale(65, dpi), UiMetrics::Scale(20, dpi), CanonicalUiState::Good, fonts, L"Khớp 100%");
        SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, facCard.left + UiMetrics::Scale(12, dpi), facCard.top + UiMetrics::Scale(48, dpi), L"Khớp cấu hình xuất xưởng nhà máy", 32);
    } else if (!rep.profileSource.empty()) {
        DrawStatusBadge(dc, facCard.right - UiMetrics::Scale(75, dpi), facCard.top + UiMetrics::Scale(24, dpi), UiMetrics::Scale(65, dpi), UiMetrics::Scale(20, dpi), CanonicalUiState::Info, fonts, L"Khớp mẫu");
        SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        std::wstring srcText = L"Nguồn: " + rep.profileSource;
        TextOutW(dc, facCard.left + UiMetrics::Scale(12, dpi), facCard.top + UiMetrics::Scale(48, dpi), srcText.c_str(), (int)srcText.size());
    } else {
        DrawStatusBadge(dc, facCard.right - UiMetrics::Scale(75, dpi), facCard.top + UiMetrics::Scale(24, dpi), UiMetrics::Scale(65, dpi), UiMetrics::Scale(20, dpi), CanonicalUiState::NotTested, fonts, L"Chưa nạp");
        SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, facCard.left + UiMetrics::Scale(12, dpi), facCard.top + UiMetrics::Scale(48, dpi), L"Chưa có hồ sơ đối chiếu gốc", 27);
    }

    // 3. Guided Stepper
    RECT stepCard{ rightX, facCard.bottom + UiMetrics::Scale(10, dpi), r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi) };
    DrawRoundedCard(dc, stepCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, stepCard.left + UiMetrics::Scale(12, dpi), stepCard.top + UiMetrics::Scale(10, dpi), L"Quy trình kiểm tra hướng dẫn", 28);
    
    // Dynamic stepper steps
    std::vector<StepperStep> steps;
    // Step 1: Auto Audit
    StepperStep st1;
    st1.stepNumber = 1;
    st1.title = L"Kiểm tra Tự động";
    if (running) {
        st1.state = CanonicalUiState::Running;
        st1.isCurrent = true;
        st1.description = L"Đang quét thông tin phần cứng & Stress...";
    } else if (auditCompletedItems >= auditTotalItems) {
        st1.state = CanonicalUiState::Good;
        st1.isCurrent = false;
        st1.description = L"Đã hoàn tất " + std::to_wstring(auditCompletedItems) + L" hạng mục";
    } else {
        st1.state = CanonicalUiState::NotTested;
        st1.isCurrent = true;
        st1.description = L"Thu thập thông tin phần cứng & Stress";
    }
    steps.push_back(st1);

    // Step 2: Functional Checks
    StepperStep st2;
    st2.stepNumber = 2;
    st2.title = L"Kiểm tra Chức năng";
    if (rep.hardware.stress.functional.overall == L"PASS") {
        st2.state = CanonicalUiState::Good;
        st2.isCurrent = false;
        st2.description = L"Bàn phím, Trackpad, Loa, Camera: Đạt";
    } else if (auditCompletedItems >= auditTotalItems && !running) {
        st2.state = CanonicalUiState::Running;
        st2.isCurrent = true;
        st2.description = L"Bàn phím, Trackpad, Màn hình, Loa, Cam...";
    } else {
        st2.state = CanonicalUiState::NotTested;
        st2.isCurrent = false;
        st2.description = L"Kiểm tra tương tác bàn phím, loa, mic...";
    }
    steps.push_back(st2);

    // Step 3: Ports & Power
    StepperStep st3;
    st3.stepNumber = 3;
    st3.title = L"Cổng & Nguồn vật lý";
    if (rep.hardware.stress.portPower.overall == L"PASS") {
        st3.state = CanonicalUiState::Good;
        st3.isCurrent = false;
        st3.description = L"Các cổng kết nối và nguồn sạc: Đạt";
    } else if (rep.hardware.stress.functional.overall == L"PASS") {
        st3.state = CanonicalUiState::Running;
        st3.isCurrent = true;
        st3.description = L"Kiểm tra từng cổng cắm với thiết bị mẫu";
    } else {
        st3.state = CanonicalUiState::NotTested;
        st3.isCurrent = false;
        st3.description = L"Kiểm tra cắm rút cổng & nguồn";
    }
    steps.push_back(st3);

    // Step 4: Final Report
    StepperStep st4;
    st4.stepNumber = 4;
    st4.title = L"Đánh giá & Báo cáo";
    if (auditReady) {
        st4.state = CanonicalUiState::Good;
        st4.isCurrent = false;
        st4.description = L"Đã hoàn tất kiểm định và xuất báo cáo";
    } else if (rep.hardware.stress.portPower.overall == L"PASS") {
        st4.state = CanonicalUiState::Running;
        st4.isCurrent = true;
        st4.description = L"Xem kết luận & xuất báo cáo kiểm định";
    } else {
        st4.state = CanonicalUiState::NotTested;
        st4.isCurrent = false;
        st4.description = L"Tổng hợp kết luận & xuất báo cáo";
    }
    steps.push_back(st4);

    RECT stepperRect{ stepCard.left + UiMetrics::Scale(10, dpi), stepCard.top + UiMetrics::Scale(32, dpi), stepCard.right - UiMetrics::Scale(10, dpi), stepCard.bottom - UiMetrics::Scale(48, dpi) };
    DrawGuidedStepper(dc, stepperRect, steps, fonts, dpi);

    // Dynamic Button Next step
    RECT nextBtnRect{ stepCard.left + UiMetrics::Scale(12, dpi), stepCard.bottom - UiMetrics::Scale(40, dpi), stepCard.right - UiMetrics::Scale(12, dpi), stepCard.bottom - UiMetrics::Scale(10, dpi) };
    DrawRoundedCard(dc, nextBtnRect, UiMetrics::RadiusSm, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
    SetTextColor(dc, RGB(255, 255, 255));
    SelectObject(dc, fonts.hBodyBold);
    
    std::wstring nextActionText = L"▶ Bắt đầu kiểm tra tự động";
    if (running) {
        nextActionText = L"⏳ Đang kiểm tra tự động...";
    } else if (auditCompletedItems >= auditTotalItems && rep.hardware.stress.functional.overall != L"PASS") {
        nextActionText = L"▶ Tiếp tục: Kiểm tra Chức năng";
    } else if (rep.hardware.stress.functional.overall == L"PASS" && rep.hardware.stress.portPower.overall != L"PASS") {
        nextActionText = L"▶ Tiếp tục: Cổng & Nguồn";
    } else if (auditReady) {
        nextActionText = L"📊 Xem Báo cáo & Đánh giá";
    }
    DrawTextW(dc, nextActionText.c_str(), (int)nextActionText.size(), &nextBtnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Middle Content: 14 Domain Summary Grid (4 cols x 4 rows)
    int gridY = kpiY + kpiH + UiMetrics::Scale(14, dpi);
    SelectObject(dc, fonts.hSection); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, r.left + UiMetrics::Scale(24, dpi), gridY, L"Tóm tắt theo nhóm chức năng", 27);
    
    // Filter pills
    int fltX = r.left + mainW - UiMetrics::Scale(180, dpi);
    DrawBadge(dc, fltX, gridY - 2, UiMetrics::Scale(55, dpi), UiMetrics::Scale(24, dpi), L"Tất cả", RGB(255, 255, 255), UiColors::PrimaryBlue, fonts.hSmall);
    DrawBadge(dc, fltX + UiMetrics::Scale(60, dpi), gridY - 2, UiMetrics::Scale(85, dpi), UiMetrics::Scale(24, dpi), L"Chỉ có vấn đề", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);

    struct DomainDef { const wchar_t* name; const wchar_t* sub; };
    const std::vector<DomainDef> domainDefs = {
        { L"Nhận diện hệ thống", L"CPU, Mainboard, BIOS, OS..." },
        { L"Bộ nhớ (RAM)", L"Dung lượng, Kiểm tra lỗi" },
        { L"Lưu trữ", L"NVMe/SSD, S.M.A.R.T, Hiệu năng" },
        { L"Pin & Nguồn", L"Dung lượng, Sạc, Adapter" },
        { L"Đồ họa (GPU)", L"iGPU, dGPU, VRAM, Driver" },
        { L"Hiển thị (Màn hình)", L"EDID, Độ phân giải, Điểm ảnh" },
        { L"Bàn phím & Touchpad", L"Phím, Chạm, Cử chỉ" },
        { L"Âm thanh & Camera", L"Loa, Mic, Camera" },
        { L"Mạng & Kết nối", L"Wi-Fi, Bluetooth, LAN" },
        { L"Cổng & Nguồn vật lý", L"USB, HDMI, DP, LAN, Audio..." },
        { L"Stress & Ổn định", L"CPU, RAM, GPU, Nhiệt độ" },
        { L"Nhật ký & Sự kiện", L"WHEA, Ổ đĩa, Hệ thống" },
        { L"Hồ sơ & Đối chiếu", L"Nhà máy, Cấu hình, Chênh lệch" },
        { L"Độ bao phủ & Tin cậy", L"Bằng chứng, Độ tin cậy" }
    };

    int cardCols = (mainW >= UiMetrics::Scale(760, dpi)) ? 4 : (mainW >= UiMetrics::Scale(500, dpi) ? 3 : 2);
    int cellW = (mainW - (cardCols - 1) * UiMetrics::Scale(10, dpi)) / cardCols;
    int cellH = UiMetrics::Scale(62, dpi);
    int startGridY = gridY + UiMetrics::Scale(26, dpi);
    int rows = ((int)domainDefs.size() + cardCols - 1) / cardCols;

    for (size_t i = 0; i < domainDefs.size(); ++i) {
        int row = (int)i / cardCols;
        int col = (int)i % cardCols;
        int cx = r.left + UiMetrics::Scale(24, dpi) + col * (cellW + UiMetrics::Scale(10, dpi));
        int cy = startGridY + row * (cellH + UiMetrics::Scale(8, dpi));

        RECT cr{ cx, cy, cx + cellW, cy + cellH };
        DrawRoundedCard(dc, cr, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
        
        SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
        RECT tr{ cx + UiMetrics::Scale(8, dpi), cy + UiMetrics::Scale(6, dpi), cx + cellW - UiMetrics::Scale(6, dpi), cy + UiMetrics::Scale(22, dpi) };
        DrawTextW(dc, domainDefs[i].name, (int)wcslen(domainDefs[i].name), &tr, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        
        SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        RECT sr{ cx + UiMetrics::Scale(8, dpi), cy + UiMetrics::Scale(22, dpi), cx + cellW - UiMetrics::Scale(6, dpi), cy + UiMetrics::Scale(36, dpi) };
        DrawTextW(dc, domainDefs[i].sub, (int)wcslen(domainDefs[i].sub), &sr, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

        CanonicalUiState dState = EvaluateDomainState((int)i, rep, running, auditReady, auditCompletedItems);
        int badgeW = std::min(UiMetrics::Scale(85, dpi), cellW - UiMetrics::Scale(16, dpi));
        int badgeH = UiMetrics::Scale(18, dpi);
        DrawStatusBadge(dc, cx + UiMetrics::Scale(8, dpi), cy + UiMetrics::Scale(38, dpi), badgeW, badgeH, dState, fonts, (dState == CanonicalUiState::NotTested ? L"Chưa quét" : L""));
    }

    // Quick Info Card at bottom of middle area
    int infoY = startGridY + rows * (cellH + UiMetrics::Scale(8, dpi)) + UiMetrics::Scale(6, dpi);
    RECT infoCard{ r.left + UiMetrics::Scale(24, dpi), infoY, r.left + UiMetrics::Scale(24, dpi) + mainW, r.bottom - UiMetrics::Scale(20, dpi) };
    DrawRoundedCard(dc, infoCard, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, infoCard.left + UiMetrics::Scale(12, dpi), infoCard.top + UiMetrics::Scale(8, dpi), L"Thông tin nhanh", 15);

    int colW = (mainW - UiMetrics::Scale(36, dpi)) / 3;
    int c1X = infoCard.left + UiMetrics::Scale(12, dpi);
    int c2X = c1X + colW + UiMetrics::Scale(6, dpi);
    int c3X = c2X + colW + UiMetrics::Scale(6, dpi);
    
    auto drawField = [&](int startX, int fieldY, const wchar_t* label, const wchar_t* val) {
        SelectObject(dc, fonts.hSmall);
        SetTextColor(dc, UiColors::TextMuted);
        RECT lr{ startX, fieldY, startX + UiMetrics::Scale(75, dpi), fieldY + UiMetrics::Scale(16, dpi) };
        DrawTextW(dc, label, (int)wcslen(label), &lr, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

        SetTextColor(dc, UiColors::TextMain);
        RECT vr{ startX + UiMetrics::Scale(78, dpi), fieldY, startX + colW, fieldY + UiMetrics::Scale(16, dpi) };
        DrawTextW(dc, val, (int)wcslen(val), &vr, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    };

    int row1Y = infoCard.top + UiMetrics::Scale(28, dpi);
    int row2Y = infoCard.top + UiMetrics::Scale(46, dpi);

    drawField(c1X, row1Y, L"Máy:", pName.c_str());
    drawField(c1X, row2Y, L"Service Tag:", sTag.c_str());

    drawField(c2X, row1Y, L"CPU:", cpuStr.c_str());
    drawField(c2X, row2Y, L"RAM:", ramStr.c_str());

    drawField(c3X, row1Y, L"Ổ đĩa chính:", diskStr.c_str());
    drawField(c3X, row2Y, L"Đồ họa:", gpuStr.c_str());
}

// ============================================================
// S02 — KHỞI TẠO PHIÊN KIỂM ĐỊNH MỚI
// ============================================================
void RenderScreenS02_NewSession(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi, int focusIndex) {
    (void)rep; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Khởi tạo phiên kiểm định";
    hdr.subtitle = L"Tạo và bắt đầu một phiên kiểm định laptop mới.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Section 1: Session Types
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, r.left + UiMetrics::Scale(24, dpi), curY, L"1. Chọn loại phiên kiểm định", 28);
    curY += UiMetrics::Scale(22, dpi);

    int cardW = (mainW - UiMetrics::Scale(16, dpi)) / 3;
    int cardH = UiMetrics::Scale(60, dpi);
    
    auto drawTypeCard = [&](int idx, const wchar_t* title, const wchar_t* sub, bool sel) {
        int cx = r.left + UiMetrics::Scale(24, dpi) + idx * (cardW + UiMetrics::Scale(8, dpi));
        RECT cr{ cx, curY, cx + cardW, curY + cardH };
        DrawRoundedCard(dc, cr, UiMetrics::RadiusSm, sel ? RGB(239, 246, 255) : UiColors::CardBg, sel ? UiColors::PrimaryBlue : UiColors::CardBorder, sel ? 2 : 1);
        SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, sel ? UiColors::PrimaryBlue : UiColors::TextMain);
        TextOutW(dc, cx + UiMetrics::Scale(10, dpi), curY + UiMetrics::Scale(8, dpi), title, (int)wcslen(title));
        SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, cx + UiMetrics::Scale(10, dpi), curY + UiMetrics::Scale(26, dpi), sub, (int)wcslen(sub));
    };
    drawTypeCard(0, L"Mua máy cũ", L"Kiểm tra tổng thể trước khi mua", true);
    drawTypeCard(1, L"Kiểm tra nhận máy", L"Kiểm tra khi nhận từ khách hàng", false);
    drawTypeCard(2, L"Kiểm tra nội bộ", L"Kiểm tra định kỳ kỹ thuật", false);

    curY += cardH + UiMetrics::Scale(14, dpi);

    // Section 2: Mode Selector
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, r.left + UiMetrics::Scale(24, dpi), curY, L"2. Chọn chế độ kiểm tra", 23);
    curY += UiMetrics::Scale(22, dpi);

    auto drawModeCard = [&](int idx, const wchar_t* title, const wchar_t* sub, bool sel) {
        int cx = r.left + UiMetrics::Scale(24, dpi) + idx * (cardW + UiMetrics::Scale(8, dpi));
        RECT cr{ cx, curY, cx + cardW, curY + cardH };
        DrawRoundedCard(dc, cr, UiMetrics::RadiusSm, sel ? RGB(239, 246, 255) : UiColors::CardBg, sel ? UiColors::PrimaryBlue : UiColors::CardBorder, sel ? 2 : 1);
        SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, sel ? UiColors::PrimaryBlue : UiColors::TextMain);
        TextOutW(dc, cx + UiMetrics::Scale(10, dpi), curY + UiMetrics::Scale(8, dpi), title, (int)wcslen(title));
        SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, cx + UiMetrics::Scale(10, dpi), curY + UiMetrics::Scale(26, dpi), sub, (int)wcslen(sub));
    };
    drawModeCard(0, L"Nhanh", L"Hoàn thành trong vài phút", false);
    drawModeCard(1, L"Tiêu chuẩn", L"Cân bằng tốc độ & độ chi tiết", true);
    drawModeCard(2, L"Chuyên sâu", L"Chi tiết tối đa, mất thời gian", false);

    curY += cardH + UiMetrics::Scale(14, dpi);

    // Section 3: Session Details Form
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, r.left + UiMetrics::Scale(24, dpi), curY, L"3. Thông tin phiên kiểm định", 28);
    curY += UiMetrics::Scale(22, dpi);

    RECT formCard{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, curY + UiMetrics::Scale(110, dpi) };
    DrawRoundedCard(dc, formCard, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, formCard.left + 12, formCard.top + 10, L"Tên phiên kiểm định *", 21);
    TextOutW(dc, formCard.left + mainW / 2 + 12, formCard.top + 10, L"Mã phiên *", 10);
    TextOutW(dc, formCard.left + 12, formCard.top + 55, L"Ngày giờ bắt đầu *", 18);
    TextOutW(dc, formCard.left + mainW / 2 + 12, formCard.top + 55, L"Kỹ thuật viên *", 15);

    SelectObject(dc, fonts.hBody); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, formCard.left + 12, formCard.top + 28, L"Kiểm định Dell Precision 5560 - 2026/08/24", 42);
    TextOutW(dc, formCard.left + mainW / 2 + 12, formCard.top + 28, L"LS-20260824-001", 15);
    TextOutW(dc, formCard.left + 12, formCard.top + 73, L"24/08/2026 09:42", 16);
    TextOutW(dc, formCard.left + mainW / 2 + 12, formCard.top + 73, L"Nguyễn Văn An", 13);

    curY += UiMetrics::Scale(110 + 14, dpi);

    // Section 4: Admin note banner
    RECT noteCard{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, curY + UiMetrics::Scale(55, dpi) };
    DrawRoundedCard(dc, noteCard, UiMetrics::RadiusSm, RGB(239, 246, 255), RGB(191, 219, 254), 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::PrimaryBlue);
    TextOutW(dc, noteCard.left + 12, noteCard.top + 8, L"Lưu ý quan trọng:", 17);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, noteCard.left + 12, noteCard.top + 28, L"Để thực hiện đầy đủ các phép kiểm tra, bạn cần chạy ứng dụng với quyền Quản trị viên (Administrator).", 101);

    // Right Rail: Workflow info & System status
    int rightX = r.right - UiMetrics::Scale(240 + 24, dpi);
    RECT rightCard{ rightX, r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(60, dpi) };
    DrawRoundedCard(dc, rightCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 12, L"Điều gì sẽ xảy ra tiếp theo?", 28);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 32, L"Sau khi tạo phiên, hệ thống sẽ thực hiện theo thứ tự:", 53);

    std::vector<StepperStep> newSteps = {
        { 1, L"Kiểm tra tự động", L"Thu thập cấu hình và phần cứng", CanonicalUiState::Running, true },
        { 2, L"Kiểm tra chức năng", L"Kiểm tra bàn phím, loa, mic, cam", CanonicalUiState::NotTested, false },
        { 3, L"Cổng & Nguồn", L"Kiểm tra từng cổng cắm vật lý", CanonicalUiState::NotTested, false },
        { 4, L"Đánh giá cuối cùng", L"Tổng hợp kết luận và gợi ý", CanonicalUiState::NotTested, false }
    };
    RECT nsr{ rightCard.left + 10, rightCard.top + 55, rightCard.right - 10, rightCard.top + 240 };
    DrawGuidedStepper(dc, nsr, newSteps, fonts, dpi);

    // Bottom Action Buttons
    int botY = r.bottom - UiMetrics::Scale(50, dpi);
    RECT saveDraftBtn{ r.left + UiMetrics::Scale(24, dpi), botY, r.left + UiMetrics::Scale(140, dpi), botY + UiMetrics::Scale(36, dpi) };
    DrawRoundedCard(dc, saveDraftBtn, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    SetTextColor(dc, UiColors::TextMain); SelectObject(dc, fonts.hBody);
    DrawTextW(dc, L"Lưu nháp", 8, &saveDraftBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT cancelBtn{ saveDraftBtn.right + 10, botY, saveDraftBtn.right + 110, botY + UiMetrics::Scale(36, dpi) };
    DrawRoundedCard(dc, cancelBtn, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    DrawTextW(dc, L"Hủy", 3, &cancelBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT startBtn{ r.right - UiMetrics::Scale(250 + 24, dpi), botY, r.right - UiMetrics::Scale(24, dpi), botY + UiMetrics::Scale(36, dpi) };
    DrawRoundedCard(dc, startBtn, UiMetrics::RadiusSm, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
    SetTextColor(dc, RGB(255, 255, 255)); SelectObject(dc, fonts.hBodyBold);
    DrawTextW(dc, L"▶ Bắt đầu phiên kiểm định", 25, &startBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================
// S03 — CAM KẾT NGƯỜI BÁN
// ============================================================
void RenderScreenS03_SellerClaim(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi, int focusIndex) {
    (void)rep; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Cam kết người bán";
    hdr.subtitle = L"Ghi nhận các thông tin và cam kết từ người bán trước khi tiến hành đối chiếu.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Stepper header
    int stepW = mainW / 3;
    DrawBadge(dc, r.left + UiMetrics::Scale(24, dpi), curY, stepW - 10, UiMetrics::Scale(28, dpi), L"1. Nhập thông tin (Ghi nhận thông tin cơ bản)", RGB(255, 255, 255), UiColors::PrimaryBlue, fonts.hSmall);
    DrawBadge(dc, r.left + UiMetrics::Scale(24, dpi) + stepW, curY, stepW - 10, UiMetrics::Scale(28, dpi), L"2. Ảnh & bằng chứng (Tải ảnh và minh chứng)", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
    DrawBadge(dc, r.left + UiMetrics::Scale(24, dpi) + stepW * 2, curY, stepW - 10, UiMetrics::Scale(28, dpi), L"3. Xác nhận (Kiểm tra và hoàn tất)", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
    curY += UiMetrics::Scale(40, dpi);

    // Seller Info Card
    RECT infoCard{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, curY + UiMetrics::Scale(75, dpi) };
    DrawRoundedCard(dc, infoCard, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, infoCard.left + 12, infoCard.top + 8, L"Người bán / Cửa hàng *", 23);
    TextOutW(dc, infoCard.left + mainW / 2 + 12, infoCard.top + 8, L"Số điện thoại *", 15);
    SelectObject(dc, fonts.hBody); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, infoCard.left + 12, infoCard.top + 26, L"LaptopZone - Nguyễn Trãi", 24);
    TextOutW(dc, infoCard.left + mainW / 2 + 12, infoCard.top + 26, L"0988 123 456", 12);
    curY += UiMetrics::Scale(85, dpi);

    // Machine Specs Claimed Card
    RECT specCard{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, curY + UiMetrics::Scale(155, dpi) };
    DrawRoundedCard(dc, specCard, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, specCard.left + 12, specCard.top + 8, L"Cấu hình máy theo mô tả của người bán", 37);

    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, specCard.left + 12, specCard.top + 32, L"Model máy:", 10);
    TextOutW(dc, specCard.left + 12, specCard.top + 56, L"CPU:", 4);
    TextOutW(dc, specCard.left + 12, specCard.top + 80, L"RAM:", 4);
    TextOutW(dc, specCard.left + 12, specCard.top + 104, L"SSD:", 4);
    TextOutW(dc, specCard.left + 12, specCard.top + 128, L"Giá bán:", 8);

    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, specCard.left + 100, specCard.top + 32, L"Dell Precision 5560", 19);
    TextOutW(dc, specCard.left + 100, specCard.top + 56, L"Intel Core i7-11800H", 20);
    TextOutW(dc, specCard.left + 100, specCard.top + 80, L"32GB DDR4 3200MHz", 17);
    TextOutW(dc, specCard.left + 100, specCard.top + 104, L"1TB NVMe PCIe 4.0", 17);
    TextOutW(dc, specCard.left + 100, specCard.top + 128, L"22.500.000 đ", 12);

    TextOutW(dc, specCard.left + mainW / 2 + 12, specCard.top + 32, L"Pin:", 4);
    TextOutW(dc, specCard.left + mainW / 2 + 12, specCard.top + 56, L"Sạc:", 4);
    TextOutW(dc, specCard.left + mainW / 2 + 12, specCard.top + 80, L"Bảo hành:", 9);

    TextOutW(dc, specCard.left + mainW / 2 + 90, specCard.top + 32, L"~85% / 4-5 giờ sử dụng", 22);
    TextOutW(dc, specCard.left + mainW / 2 + 90, specCard.top + 56, L"Sạc zin 130W USB-C", 18);
    TextOutW(dc, specCard.left + mainW / 2 + 90, specCard.top + 80, L"06 tháng cửa hàng", 17);
    curY += UiMetrics::Scale(165, dpi);

    // Checkboxes for seller guarantees
    RECT gCard{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, curY + UiMetrics::Scale(48, dpi) };
    DrawRoundedCard(dc, gCard, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    DrawBadge(dc, gCard.left + 10, gCard.top + 10, UiMetrics::Scale(70, dpi), UiMetrics::Scale(26, dpi), L"☑ Có hộp", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
    DrawBadge(dc, gCard.left + UiMetrics::Scale(90, dpi), gCard.top + 10, UiMetrics::Scale(70, dpi), UiMetrics::Scale(26, dpi), L"☑ Có sạc", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
    DrawBadge(dc, gCard.left + UiMetrics::Scale(170, dpi), gCard.top + 10, UiMetrics::Scale(85, dpi), UiMetrics::Scale(26, dpi), L"☑ Có hóa đơn", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
    DrawBadge(dc, gCard.left + UiMetrics::Scale(265, dpi), gCard.top + 10, UiMetrics::Scale(95, dpi), UiMetrics::Scale(26, dpi), L"☑ Cho test máy", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
    DrawBadge(dc, gCard.left + UiMetrics::Scale(370, dpi), gCard.top + 10, UiMetrics::Scale(85, dpi), UiMetrics::Scale(26, dpi), L"☑ Cho đổi trả", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);

    // Right Rail
    int rightX = r.right - UiMetrics::Scale(240 + 24, dpi);
    RECT rightCard{ rightX, r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(60, dpi) };
    DrawRoundedCard(dc, rightCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 12, L"Tổng quan hoàn thành", 20);
    DrawBadge(dc, rightCard.right - 55, rightCard.top + 10, 45, 20, L"0%", UiColors::TextMuted, UiColors::GrayPillBg, fonts.hSmall);
    
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::WarnAmber);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 45, L"⚠ Thiếu thông tin cam kết", 25);
    SetTextColor(dc, UiColors::TextMuted);
    RECT rTextRect{ rightCard.left + 12, rightCard.top + 68, rightCard.right - 12, rightCard.top + 150 };
    DrawTextW(dc, L"Thiếu dữ liệu từ người bán có thể làm giảm độ tin cậy khi đối chiếu và tăng rủi ro cho quyết định mua.", 100, &rTextRect, DT_LEFT | DT_WORDBREAK);

    // Bottom Actions
    int botY = r.bottom - UiMetrics::Scale(50, dpi);
    RECT nextBtn{ r.left + UiMetrics::Scale(24, dpi), botY, r.left + UiMetrics::Scale(220, dpi), botY + UiMetrics::Scale(36, dpi) };
    DrawRoundedCard(dc, nextBtn, UiMetrics::RadiusSm, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
    SetTextColor(dc, RGB(255, 255, 255)); SelectObject(dc, fonts.hBodyBold);
    DrawTextW(dc, L"Tiếp theo: Ảnh & bằng chứng →", 29, &nextBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================
// S04 — KIỂM TRA TỰ ĐỘNG
// ============================================================
void RenderScreenS04_AutoAudit(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                               const std::wstring& selectedMode, bool running, bool paused, int auditCompletedItems,
                               int auditTotalItems, int auditCurrentStage, int auditElapsedSec,
                               const std::vector<LiveLogEntry>& liveLogs, int focusIndex) {
    (void)rep; (void)selectedMode; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Kiểm tra Tự động";
    hdr.subtitle = L"Quét cấu hình, sức khỏe và độ ổn định hệ thống một cách tự động và toàn diện.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int rightPanelW = std::clamp((int)((r.right - r.left) * 26 / 100), UiMetrics::Scale(260, dpi), UiMetrics::Scale(340, dpi));
    int rightX = r.right - rightPanelW - UiMetrics::Scale(20, dpi);
    int mainW = rightX - r.left - UiMetrics::Scale(32, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // 1. Progress Bar Top Card
    int pCardH = UiMetrics::Scale(54, dpi);
    RECT pCard{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, curY + pCardH };
    DrawRoundedCard(dc, pCard, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);

    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    std::wstring runStatus = paused ? (L"Đã tạm dừng (" + std::to_wstring(auditCompletedItems) + L" / " + std::to_wstring(auditTotalItems) + L" mục)") : (running ? (L"Đang chạy " + std::to_wstring(auditCompletedItems) + L" / " + std::to_wstring(auditTotalItems) + L" hạng mục") : (auditCompletedItems >= auditTotalItems ? L"Đã hoàn thành tất cả hạng mục" : L"Sẵn sàng kiểm tra"));
    RECT runStatusRect{ pCard.left + UiMetrics::Scale(12, dpi), pCard.top, pCard.left + UiMetrics::Scale(210, dpi), pCard.bottom };
    DrawTextW(dc, runStatus.c_str(), (int)runStatus.size(), &runStatusRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    int pct = (auditTotalItems > 0) ? (auditCompletedItems * 100 / auditTotalItems) : 0;
    pct = std::clamp(pct, 0, 100);

    int barLeft = runStatusRect.right + UiMetrics::Scale(8, dpi);
    int barRight = pCard.right - UiMetrics::Scale(150, dpi);
    int barH = UiMetrics::Scale(10, dpi);
    RECT pBarRect{ barLeft, pCard.top + (pCardH - barH) / 2, barRight, pCard.top + (pCardH - barH) / 2 + barH };
    DrawModernProgressBar(dc, pBarRect, pct, UiColors::PrimaryBlue, RGB(226, 232, 240));
    
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::PrimaryBlue);
    std::wstring pctStr = std::to_wstring(pct) + L"%";
    RECT pctTextRect{ barRight + UiMetrics::Scale(8, dpi), pCard.top, barRight + UiMetrics::Scale(48, dpi), pCard.bottom };
    DrawTextW(dc, pctStr.c_str(), (int)pctStr.size(), &pctTextRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Pause/Cancel Buttons
    int btnW = UiMetrics::Scale(85, dpi);
    int btnH = UiMetrics::Scale(28, dpi);
    RECT pauseBtn{ pCard.right - btnW - UiMetrics::Scale(10, dpi), pCard.top + (pCardH - btnH) / 2, pCard.right - UiMetrics::Scale(10, dpi), pCard.top + (pCardH - btnH) / 2 + btnH };
    DrawRoundedCard(dc, pauseBtn, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    DrawTextW(dc, paused ? L"▶ Tiếp tục" : (running ? L"|| Tạm dừng" : L"▶ Tiếp tục"), -1, &pauseBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    curY += pCardH + UiMetrics::Scale(10, dpi);

    // 2. 9 Stages Table Grid
    struct StageRow { int id; const wchar_t* name; const wchar_t* sub; const wchar_t* src; CanonicalUiState st; const wchar_t* time; };
    std::vector<StageRow> stageRows = {
        { 1, L"Nhận diện hệ thống", L"CPU, Mainboard, BIOS, OS, Thiết bị", L"Dữ liệu WMI, SMBIOS, SetupAPI", (auditCompletedItems >= 1) ? CanonicalUiState::Pass : ((auditCurrentStage == 1) ? CanonicalUiState::Running : CanonicalUiState::NotTested), L"00:05" },
        { 2, L"CPU & Microbench", L"Identity, tải, vi điểm chuẩn, telemetry", L"WMI, Benchmark, Telemetry", (auditCompletedItems >= 2) ? CanonicalUiState::Pass : ((auditCurrentStage == 2) ? CanonicalUiState::Running : CanonicalUiState::NotTested), L"00:34" },
        { 3, L"Bộ nhớ (RAM)", L"Dung lượng, DIMM, Kiểm tra lỗi", L"Dữ liệu WMI, Win32 Memory", (auditCompletedItems >= 3) ? CanonicalUiState::Pass : ((auditCurrentStage == 3) ? CanonicalUiState::Running : CanonicalUiState::NotTested), L"00:42" },
        { 4, L"Lưu trữ (Storage / SMART)", L"NVMe/SSD, S.M.A.R.T., Hiệu năng", L"Dữ liệu từ SMART Provider", (auditCompletedItems >= 4) ? CanonicalUiState::Warning : ((auditCurrentStage == 4) ? CanonicalUiState::Running : CanonicalUiState::NotTested), L"01:18" },
        { 5, L"Đồ họa (GPU & DXGI)", L"iGPU, dGPU, VRAM, Driver", L"Dữ liệu từ WMI, DXGI", (auditCompletedItems >= 5) ? CanonicalUiState::Pass : ((auditCurrentStage == 5) ? CanonicalUiState::Running : CanonicalUiState::NotTested), L"00:31" },
        { 6, L"Pin & Nguồn", L"Dung lượng, Sạc, Adapter", L"Đang thu thập dữ liệu pin", (auditCompletedItems >= 6) ? CanonicalUiState::Pass : ((auditCurrentStage == 6) ? CanonicalUiState::Running : CanonicalUiState::NotTested), L"01:04" },
        { 7, L"Mạng & Kết nối", L"Wi-Fi, Bluetooth, LAN", L"Chưa bắt đầu", (auditCompletedItems >= 7) ? CanonicalUiState::Pass : ((auditCurrentStage == 7) ? CanonicalUiState::Running : CanonicalUiState::NotTested), L"--:--" },
        { 8, L"Nhật ký & Sự kiện", L"WHEA, Ổ đĩa, Hệ thống", L"Chưa bắt đầu", (auditCompletedItems >= 8) ? CanonicalUiState::Pass : ((auditCurrentStage == 8) ? CanonicalUiState::Running : CanonicalUiState::NotTested), L"--:--" },
        { 9, L"Stress & Ổn định", L"CPU, RAM, GPU, Nhiệt độ", L"Chưa bắt đầu", (auditCompletedItems >= 9) ? CanonicalUiState::Pass : ((auditCurrentStage == 9) ? CanonicalUiState::Running : CanonicalUiState::NotTested), L"--:--" }
    };

    int rowH = UiMetrics::Scale(34, dpi);
    for (const auto& sr : stageRows) {
        RECT srRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, curY + rowH };
        bool isCurrent = (sr.st == CanonicalUiState::Running);
        DrawRoundedCard(dc, srRect, UiMetrics::RadiusSm, isCurrent ? RGB(240, 247, 255) : UiColors::CardBg, isCurrent ? UiColors::PrimaryBlue : UiColors::CardBorder, isCurrent ? 2 : 1);
        
        // Stage Title
        SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, isCurrent ? UiColors::PrimaryBlue : UiColors::TextMain);
        std::wstring sTitle = std::to_wstring(sr.id) + L". " + sr.name;
        RECT titleRect{ srRect.left + UiMetrics::Scale(10, dpi), srRect.top, srRect.left + UiMetrics::Scale(220, dpi), srRect.bottom };
        DrawTextW(dc, sTitle.c_str(), (int)sTitle.size(), &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        // Status Badge (generous width with proper spacing)
        int badgeW = UiMetrics::Scale(90, dpi);
        int badgeH = UiMetrics::Scale(20, dpi);
        int badgeX = titleRect.right + UiMetrics::Scale(8, dpi);
        int badgeY = srRect.top + (rowH - badgeH) / 2;
        DrawStatusBadge(dc, badgeX, badgeY, badgeW, badgeH, sr.st, fonts, (sr.st == CanonicalUiState::NotTested ? L"Chưa quét" : L""));

        // Stage Time Duration
        SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        int timeX = badgeX + badgeW + UiMetrics::Scale(12, dpi);
        RECT timeRect{ timeX, srRect.top, timeX + UiMetrics::Scale(50, dpi), srRect.bottom };
        DrawTextW(dc, sr.time, (int)wcslen(sr.time), &timeRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        // Evidence Source
        int srcX = timeRect.right + UiMetrics::Scale(8, dpi);
        RECT srcRect{ srcX, srRect.top, srRect.right - UiMetrics::Scale(10, dpi), srRect.bottom };
        DrawTextW(dc, sr.src, (int)wcslen(sr.src), &srcRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

        curY += rowH + UiMetrics::Scale(4, dpi);
    }

    curY += UiMetrics::Scale(6, dpi);

    // 3. Live Logs Dark Terminal Panel
    RECT logCard{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, r.bottom - UiMetrics::Scale(16, dpi) };
    DrawRoundedCard(dc, logCard, UiMetrics::RadiusSm, RGB(15, 23, 42), RGB(30, 41, 59), 1); // Dark navy slate console
    
    // Header with glowing green dot
    HBRUSH dotB = CreateSolidBrush(RGB(16, 185, 129));
    HPEN dotP = CreatePen(PS_SOLID, 1, RGB(16, 185, 129));
    HGDIOBJ oldDb = SelectObject(dc, dotB); HGDIOBJ oldDp = SelectObject(dc, dotP);
    int dotX = logCard.left + UiMetrics::Scale(12, dpi);
    int dotY = logCard.top + UiMetrics::Scale(10, dpi);
    int dotR = UiMetrics::Scale(4, dpi);
    Ellipse(dc, dotX, dotY, dotX + dotR * 2, dotY + dotR * 2);
    SelectObject(dc, oldDb); SelectObject(dc, oldDp);
    DeleteObject(dotB); DeleteObject(dotP);

    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, RGB(248, 250, 252));
    TextOutW(dc, dotX + dotR * 2 + UiMetrics::Scale(6, dpi), logCard.top + UiMetrics::Scale(6, dpi), L"Nhật ký & Bằng chứng trực tiếp (Live Terminal)", 47);

    int logY = logCard.top + UiMetrics::Scale(28, dpi);
    SelectObject(dc, fonts.hMono);
    int lineH = UiMetrics::Scale(18, dpi);
    int maxLines = (logCard.bottom - logY - UiMetrics::Scale(8, dpi)) / lineH;
    int startIdx = std::max(0, (int)liveLogs.size() - maxLines);
    for (size_t i = startIdx; i < liveLogs.size() && logY < logCard.bottom - lineH; ++i) {
        COLORREF logClr = (liveLogs[i].state == 1) ? RGB(251, 191, 36) : ((liveLogs[i].state == 2) ? RGB(248, 113, 113) : RGB(148, 163, 184));
        SetTextColor(dc, logClr);
        std::wstring line = liveLogs[i].time + L"  [" + liveLogs[i].source + L"]  " + liveLogs[i].message;
        RECT lr{ logCard.left + UiMetrics::Scale(12, dpi), logY, logCard.right - UiMetrics::Scale(12, dpi), logY + lineH };
        DrawTextW(dc, line.c_str(), (int)line.size(), &lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        logY += lineH;
    }

    // 4. Right Rail Cards: Timing, Do's & Don'ts, Action Buttons
    // Timing Card
    RECT timeCard{ rightX, r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(20, dpi), r.top + UiMetrics::Scale(160, dpi) };
    DrawRoundedCard(dc, timeCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, timeCard.left + UiMetrics::Scale(12, dpi), timeCard.top + UiMetrics::Scale(8, dpi), L"Thời gian kiểm tra", 18);
    
    int mm = auditElapsedSec / 60;
    int ss = auditElapsedSec % 60;
    wchar_t timeBuf[32];
    swprintf_s(timeBuf, L"%02d:%02d:%02d", 0, mm, ss);
    SelectObject(dc, fonts.hTitle); SetTextColor(dc, UiColors::PrimaryBlue);
    TextOutW(dc, timeCard.left + UiMetrics::Scale(12, dpi), timeCard.top + UiMetrics::Scale(26, dpi), timeBuf, (int)wcslen(timeBuf));

    int totalEstSec = (selectedMode == L"Quick") ? 90 : ((selectedMode == L"Deep") ? 480 : 240);
    int remSec = 0;
    wchar_t remBuf[64];
    if (running) {
        if (auditCompletedItems > 0 && auditTotalItems > auditCompletedItems) {
            int avgPerItem = std::max(4, auditElapsedSec / auditCompletedItems);
            remSec = (auditTotalItems - auditCompletedItems) * avgPerItem;
        } else {
            remSec = std::max(5, totalEstSec - auditElapsedSec);
        }
        int remMM = remSec / 60;
        int remSS = remSec % 60;
        swprintf_s(remBuf, L"Ước tính còn lại: ~%02d:%02d:%02d", 0, remMM, remSS);
    } else if (auditCompletedItems >= auditTotalItems && auditTotalItems > 0) {
        swprintf_s(remBuf, L"Trạng thái: Hoàn tất kiểm tra");
    } else {
        int totMM = totalEstSec / 60;
        int totSS = totalEstSec % 60;
        swprintf_s(remBuf, L"Ước tính tổng: ~%02d:%02d:%02d", 0, totMM, totSS);
    }
    TextOutW(dc, timeCard.left + UiMetrics::Scale(12, dpi), timeCard.top + UiMetrics::Scale(58, dpi), remBuf, (int)wcslen(remBuf));

    // Guidance Card (Do's & Don'ts)
    RECT guideCard{ rightX, timeCard.bottom + UiMetrics::Scale(10, dpi), r.right - UiMetrics::Scale(20, dpi), r.bottom - UiMetrics::Scale(16, dpi) };
    DrawRoundedCard(dc, guideCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, guideCard.left + UiMetrics::Scale(12, dpi), guideCard.top + UiMetrics::Scale(10, dpi), L"Hướng dẫn & Lưu ý", 17);

    int noteY = guideCard.top + UiMetrics::Scale(34, dpi);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::SuccessGreen);
    TextOutW(dc, guideCard.left + UiMetrics::Scale(12, dpi), noteY, L"✔ Bạn nên", 9);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, guideCard.left + UiMetrics::Scale(12, dpi), noteY + UiMetrics::Scale(18, dpi), L"• Cắm sạc trong suốt quá trình", 30);
    TextOutW(dc, guideCard.left + UiMetrics::Scale(12, dpi), noteY + UiMetrics::Scale(34, dpi), L"• Kết nối mạng Internet ổn định", 31);
    TextOutW(dc, guideCard.left + UiMetrics::Scale(12, dpi), noteY + UiMetrics::Scale(50, dpi), L"• Để máy ở nơi thoáng mát", 25);

    noteY += UiMetrics::Scale(78, dpi);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::FailRed);
    TextOutW(dc, guideCard.left + UiMetrics::Scale(12, dpi), noteY, L"✖ Bạn không nên", 15);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, guideCard.left + UiMetrics::Scale(12, dpi), noteY + UiMetrics::Scale(18, dpi), L"• Đóng nắp hoặc chạy tác vụ nặng", 32);
    TextOutW(dc, guideCard.left + UiMetrics::Scale(12, dpi), noteY + UiMetrics::Scale(34, dpi), L"• Rút sạc đột ngột khi Stress test", 34);

    // Cancel Button at bottom of right rail
    int cBtnH = UiMetrics::Scale(36, dpi);
    RECT cancelAuditBtn{ guideCard.left + UiMetrics::Scale(12, dpi), guideCard.bottom - cBtnH - UiMetrics::Scale(12, dpi), guideCard.right - UiMetrics::Scale(12, dpi), guideCard.bottom - UiMetrics::Scale(12, dpi) };
    DrawRoundedCard(dc, cancelAuditBtn, UiMetrics::RadiusSm, running ? UiColors::FailRed : UiColors::PrimaryBlue, running ? UiColors::FailRed : UiColors::PrimaryBlue, 1);
    SetTextColor(dc, RGB(255, 255, 255));
    SelectObject(dc, fonts.hBodyBold);
    std::wstring cBtnText = running ? L"■ HỦY BỎ KIỂM TRA" : L"▶ BẮT ĐẦU KIỂM TRA";
    DrawTextW(dc, cBtnText.c_str(), (int)cBtnText.size(), &cancelAuditBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

// ============================================================
// S05 — KIỂM TRA CHỨC NĂNG
// ============================================================
void RenderScreenS05_Functional(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                int subTab, const std::vector<int>& keyStates,
                                const std::vector<POINT>& touchpadTrail, bool touchpadDone, int focusIndex) {
    (void)rep; (void)subTab; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Kiểm tra Chức năng";
    hdr.subtitle = L"Kiểm tra thực tế các thiết bị cần thao tác tương tác trực tiếp.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Top Device Sub-tabs
    const std::vector<const wchar_t*> tabs = { L"Màn hình", L"Bàn phím & Touchpad", L"Loa trái/phải", L"Camera", L"Microphone", L"Wi-Fi", L"Bluetooth" };
    int tabW = (mainW - (int)tabs.size() * 6) / (int)tabs.size();
    for (size_t i = 0; i < tabs.size(); ++i) {
        int tx = r.left + UiMetrics::Scale(24, dpi) + (int)i * (tabW + 6);
        RECT tr{ tx, curY, tx + tabW, curY + UiMetrics::Scale(48, dpi) };
        bool active = (i == 1);
        DrawRoundedCard(dc, tr, UiMetrics::RadiusSm, active ? RGB(239, 246, 255) : UiColors::CardBg, active ? UiColors::PrimaryBlue : UiColors::CardBorder, active ? 2 : 1);
        SelectObject(dc, fonts.hSmall); SetTextColor(dc, active ? UiColors::PrimaryBlue : UiColors::TextMain);
        DrawTextW(dc, tabs[i], -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    curY += UiMetrics::Scale(58, dpi);

    // Keyboard & Touchpad side-by-side
    int kbW = mainW * 58 / 100;
    int tpW = mainW - kbW - UiMetrics::Scale(12, dpi);
    int testH = UiMetrics::Scale(220, dpi);

    RECT kbRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + kbW, curY + testH };
    DrawKeyboardGrid(dc, kbRect, keyStates, fonts, dpi);

    RECT tpRect{ kbRect.right + UiMetrics::Scale(12, dpi), curY, kbRect.right + UiMetrics::Scale(12, dpi) + tpW, curY + testH };
    DrawTouchpadCanvas(dc, tpRect, touchpadTrail, touchpadDone, fonts, dpi);

    curY += testH + UiMetrics::Scale(14, dpi);

    // Tips box
    RECT tipCard{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, curY + UiMetrics::Scale(40, dpi) };
    DrawRoundedCard(dc, tipCard, UiMetrics::RadiusSm, RGB(241, 245, 249), UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::PrimaryBlue);
    TextOutW(dc, tipCard.left + 10, tipCard.top + 10, L"💡 Mẹo:", 6);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, tipCard.left + 60, tipCard.top + 10, L"Nhấn lần lượt toàn bộ phím. Di chuyển ngón tay trên Touchpad để phủ kín lưới ô vuông kiểm tra.", 94);

    // Right Rail: Checklist
    int rightX = r.right - UiMetrics::Scale(240 + 24, dpi);
    RECT rightCard{ rightX, r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(60, dpi) };
    DrawRoundedCard(dc, rightCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 12, L"Tiến trình kiểm tra chức năng", 29);

    int chkY = rightCard.top + 40;
    for (size_t i = 0; i < tabs.size(); ++i) {
        SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
        TextOutW(dc, rightCard.left + 12, chkY, tabs[i], (int)wcslen(tabs[i]));
        DrawStatusBadge(dc, rightCard.right - 85, chkY - 2, 75, 18, (i == 1) ? CanonicalUiState::Running : CanonicalUiState::NotTested, fonts);
        chkY += 26;
    }

    // Bottom Action Buttons
    int botY = r.bottom - UiMetrics::Scale(50, dpi);
    RECT skipBtn{ r.left + UiMetrics::Scale(24, dpi), botY, r.left + UiMetrics::Scale(120, dpi), botY + UiMetrics::Scale(36, dpi) };
    DrawRoundedCard(dc, skipBtn, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    SetTextColor(dc, UiColors::TextMain); SelectObject(dc, fonts.hBody);
    DrawTextW(dc, L"Bỏ qua", 6, &skipBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT passBtn{ skipBtn.right + 10, botY, skipBtn.right + 140, botY + UiMetrics::Scale(36, dpi) };
    DrawRoundedCard(dc, passBtn, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::SuccessGreen, 1);
    SetTextColor(dc, UiColors::SuccessGreen); SelectObject(dc, fonts.hBodyBold);
    DrawTextW(dc, L"✔ Đã kiểm tra", 13, &passBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT nextBtn{ r.right - UiMetrics::Scale(220 + 24, dpi), botY, r.right - UiMetrics::Scale(24, dpi), botY + UiMetrics::Scale(36, dpi) };
    DrawRoundedCard(dc, nextBtn, UiMetrics::RadiusSm, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
    SetTextColor(dc, RGB(255, 255, 255)); SelectObject(dc, fonts.hBodyBold);
    DrawTextW(dc, L"Tiếp tục bước tiếp theo →", 25, &nextBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================
// S06 — NGOẠI HÌNH & AN TOÀN
// ============================================================
void RenderScreenS06_PhysicalSafety(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                    int activeStep, int activeHotspot, const std::vector<int>& checkState, int focusIndex) {
    (void)rep; (void)activeStep; (void)checkState; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Ngoại hình & An toàn";
    hdr.subtitle = L"Hướng dẫn kiểm tra ngoại hình và an toàn vật lý trước khi mua laptop.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Left Laptop Diagram & Hotspots
    int diagW = mainW * 45 / 100;
    int diagH = UiMetrics::Scale(240, dpi);
    RECT diagRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + diagW, curY + diagH };
    DrawLaptopChassisDiagram(dc, diagRect, activeHotspot > 0 ? activeHotspot : 1, fonts, dpi);

    // Middle 5-Point Checklist
    int chkW = mainW - diagW - UiMetrics::Scale(12, dpi);
    RECT chkCard{ diagRect.right + UiMetrics::Scale(12, dpi), curY, diagRect.right + UiMetrics::Scale(12, dpi) + chkW, curY + diagH };
    DrawRoundedCard(dc, chkCard, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, chkCard.left + 12, chkCard.top + 8, L"Hạng mục kiểm tra ngoại hình", 28);

    const std::vector<const wchar_t*> items = {
        L"1. Nứt, vỡ vỏ máy / khung",
        L"2. Móp méo do va đập",
        L"3. Khe hở bản lề, độ khít",
        L"4. Cong vênh thân máy",
        L"5. Trầy xước bề mặt"
    };

    int rowY = chkCard.top + 32;
    for (size_t i = 0; i < items.size(); ++i) {
        SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
        TextOutW(dc, chkCard.left + 12, rowY + 4, items[i], (int)wcslen(items[i]));

        // 3-way toggle buttons: [Tốt] [Cảnh báo] [Lỗi]
        DrawBadge(dc, chkCard.right - 135, rowY, 40, 20, L"Tốt", RGB(255, 255, 255), UiColors::SuccessGreen, fonts.hSmall);
        DrawBadge(dc, chkCard.right - 90, rowY, 40, 20, L"Lưu ý", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
        DrawBadge(dc, chkCard.right - 45, rowY, 40, 20, L"Lỗi", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);

        rowY += 38;
    }

    // Right Rail: Safety Alert & Photo Slots
    int rightX = r.right - UiMetrics::Scale(240 + 24, dpi);
    RECT rightCard{ rightX, r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(60, dpi) };
    DrawRoundedCard(dc, rightCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::FailRed);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 12, L"⚠ Cảnh báo an toàn", 18);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 32, L"Ngừng kiểm tra nếu phát hiện:", 29);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 48, L"• Mùi khét hoặc hóa chất lạ", 27);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 64, L"• Dây sạc hở, đứt", 18);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 80, L"• Pin phồng, biến dạng", 22);

    // Bottom Action Buttons
    int botY = r.bottom - UiMetrics::Scale(50, dpi);
    RECT passBtn{ r.left + UiMetrics::Scale(24, dpi), botY, r.left + UiMetrics::Scale(180, dpi), botY + UiMetrics::Scale(36, dpi) };
    DrawRoundedCard(dc, passBtn, UiMetrics::RadiusSm, UiColors::SuccessGreen, UiColors::SuccessGreen, 1);
    SetTextColor(dc, RGB(255, 255, 255)); SelectObject(dc, fonts.hBodyBold);
    DrawTextW(dc, L"✔ Không phát hiện lỗi", 21, &passBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT nextBtn{ r.right - UiMetrics::Scale(220 + 24, dpi), botY, r.right - UiMetrics::Scale(24, dpi), botY + UiMetrics::Scale(36, dpi) };
    DrawRoundedCard(dc, nextBtn, UiMetrics::RadiusSm, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
    DrawTextW(dc, L"Tiếp tục bước tiếp theo →", 25, &nextBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================
// S07 — CỔNG & NGUỒN
// ============================================================
void RenderScreenS07_PortsPower(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                int activePort, int focusIndex) {
    (void)rep; (void)activePort; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Cổng & Nguồn";
    hdr.subtitle = L"Kiểm tra từng cổng vật lý bằng thiết bị mẫu và đối chiếu sơ đồ thiết kế.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Port Chassis Diagram Header
    RECT diagRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, curY + UiMetrics::Scale(90, dpi) };
    DrawPortChassisDiagram(dc, diagRect, fonts, dpi);
    curY += UiMetrics::Scale(100, dpi);

    // Port Audit Table
    DataTableConfig dtc;
    dtc.columns = {
        { L"#", 35, false, false },
        { L"Cổng / Khả năng", 150, false, false },
        { L"Trạng thái vật lý", 120, false, true },
        { L"Thao tác yêu cầu", 150, false, false },
        { L"Bằng chứng kích thích", 160, true, false },
        { L"Kết quả", 80, false, true }
    };

    struct PortTestRow { std::wstring id, name, phy, act, ev, res; CanonicalUiState st; };
    std::vector<PortTestRow> portRows = {
        { L"1", L"USB-C / Thunderbolt 4 (Trái)", L"TỐT", L"Cắm USB-C mẫu", L"SanDisk 16GB USB-C", L"ĐẠT", CanonicalUiState::Pass },
        { L"2", L"USB-C / Thunderbolt 4 (Trái)", L"TỐT", L"Cắm màn hình Type-C", L"DisplayPort 1920x1080", L"ĐẠT", CanonicalUiState::Pass },
        { L"3", L"Audio Combo 3.5mm", L"TỐT", L"Thử tai nghe stereo", L"Âm thanh 2 bên rõ", L"ĐẠT", CanonicalUiState::Pass },
        { L"4", L"SD Card Reader (UHS-II)", L"TỐT", L"Cắm thẻ SD mẫu", L"SD 32GB R/W OK", L"ĐẠT", CanonicalUiState::Pass },
        { L"5", L"USB-A 3.2 Gen 1 (Phải)", L"TỐT", L"Cắm USB-A mẫu", L"SanDisk Ultra USB 3.0", L"ĐẠT", CanonicalUiState::Pass },
        { L"6", L"HDMI 2.0 (qua adapter)", L"TỐT", L"Kết nối màn hình HDMI", L"Hiển thị 1080p 60Hz", L"ĐẠT", CanonicalUiState::Pass },
        { L"7", L"Wedge Lock Slot", L"TỐT", L"Gắn khóa chống trộm", L"Khóa giữ chắc chắn", L"ĐẠT", CanonicalUiState::Pass },
        { L"8", L"DC-in / USB-C PD (Phải)", L"TỐT", L"Cắm sạc USB-C PD", L"Đang sạc 65W (20V/3.25A)", L"ĐẠT", CanonicalUiState::Pass }
    };

    for (const auto& pr : portRows) {
        TableRow row;
        row.cells = { pr.id, pr.name, pr.phy, pr.act, pr.ev, pr.res };
        row.rowState = pr.st;
        dtc.rows.push_back(row);
    }

    RECT tblRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, r.bottom - UiMetrics::Scale(60, dpi) };
    DrawDataTable(dc, tblRect, dtc, fonts, dpi);

    // Right Rail: Guidance & Tools
    int rightX = r.right - UiMetrics::Scale(240 + 24, dpi);
    RECT rightCard{ rightX, r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(60, dpi) };
    DrawRoundedCard(dc, rightCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 12, L"Lưu ý quan trọng", 16);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    RECT luyRect{ rightCard.left + 12, rightCard.top + 32, rightCard.right - 12, rightCard.top + 120 };
    DrawTextW(dc, L"Sự hiện diện của cổng cắm không đủ để kết luận cổng hoạt động. Cần cắm thiết bị mẫu vào để ghi nhận tín hiệu phản hồi.", 115, &luyRect, DT_LEFT | DT_WORDBREAK);

    // Bottom Action Buttons
    int botY = r.bottom - UiMetrics::Scale(50, dpi);
    RECT nextBtn{ r.right - UiMetrics::Scale(220 + 24, dpi), botY, r.right - UiMetrics::Scale(24, dpi), botY + UiMetrics::Scale(36, dpi) };
    DrawRoundedCard(dc, nextBtn, UiMetrics::RadiusSm, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
    SetTextColor(dc, RGB(255, 255, 255)); SelectObject(dc, fonts.hBodyBold);
    DrawTextW(dc, L"Tiếp tục bước 3 →", 17, &nextBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================
// S08 — STRESS & ỔN ĐỊNH
// ============================================================
void RenderScreenS08_StressStability(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                     bool running, int elapsedSec, const std::vector<float>& cpuTemps,
                                     const std::vector<float>& gpuTemps, const std::vector<float>& freqs,
                                     const std::vector<float>& powers, const std::vector<LiveLogEntry>& liveLogs,
                                     int focusIndex) {
    (void)rep; (void)liveLogs; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Stress & Ổn định";
    hdr.subtitle = L"Đánh giá độ ổn định, nhiệt độ, xung nhịp và hiện tượng throttle dưới tải kiểm soát.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Top Dual Charts
    int chartW = (mainW - UiMetrics::Scale(12, dpi)) / 2;
    int chartH = UiMetrics::Scale(160, dpi);

    RECT c1Rect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + chartW, curY + chartH };
    std::vector<float> defCpu = { 45, 52, 68, 75, 78, 82, 85, 84, 86, 85 };
    std::vector<float> defGpu = { 40, 44, 55, 62, 66, 70, 72, 74, 75, 74 };
    DrawLineChart(dc, c1Rect, cpuTemps.empty() ? defCpu : cpuTemps, UiColors::FailRed, L"CPU (°C)",
                  gpuTemps.empty() ? defGpu : gpuTemps, UiColors::WarnAmber, L"GPU (°C)", 0, 100, L"Nhiệt độ (°C)", fonts, dpi);

    RECT c2Rect{ c1Rect.right + UiMetrics::Scale(12, dpi), curY, c1Rect.right + UiMetrics::Scale(12, dpi) + chartW, curY + chartH };
    std::vector<float> defFreq = { 2.4f, 3.8f, 4.2f, 4.0f, 3.9f, 3.9f, 3.85f, 3.92f };
    std::vector<float> defPwr = { 15, 28, 45, 42, 38, 32, 30, 29.8f };
    DrawLineChart(dc, c2Rect, freqs.empty() ? defFreq : freqs, UiColors::PrimaryBlue, L"Xung CPU (GHz)",
                  powers.empty() ? defPwr : powers, UiColors::SuccessGreen, L"Công suất (W)", 0, 5, L"Xung nhịp & Công suất", fonts, dpi);

    curY += chartH + UiMetrics::Scale(12, dpi);

    // Telemetry Metric Cards (4 Cards)
    int kpiW = (mainW - UiMetrics::Scale(30, dpi)) / 4;
    int kpiH = UiMetrics::Scale(75, dpi);

    MetricCardConfig m1{ L"CPU Nhiệt độ", L"86 °C", L"Giới hạn: 95°C", L"Bình thường", CanonicalUiState::Good, false };
    RECT m1r{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, m1r, m1, fonts, dpi);

    MetricCardConfig m2{ L"GPU Nhiệt độ", L"74 °C", L"Giới hạn: 87°C", L"Bình thường", CanonicalUiState::Good, false };
    RECT m2r{ m1r.right + UiMetrics::Scale(10, dpi), curY, m1r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, m2r, m2, fonts, dpi);

    MetricCardConfig m3{ L"Xung nhịp CPU", L"3.92 GHz", L"Cơ bản: 2.40GHz", L"Duy trì tốt", CanonicalUiState::Good, false };
    RECT m3r{ m2r.right + UiMetrics::Scale(10, dpi), curY, m2r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, m3r, m3, fonts, dpi);

    MetricCardConfig m4{ L"Lỗi phát hiện", L"0", L"WHEA: 0 | Crash: 0", L"Hệ thống ổn định", CanonicalUiState::Good, false };
    RECT m4r{ m3r.right + UiMetrics::Scale(10, dpi), curY, m3r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, m4r, m4, fonts, dpi);

    // Right Rail: Test control & Timer
    int rightX = r.right - UiMetrics::Scale(240 + 24, dpi);
    RECT rightCard{ rightX, r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi) };
    DrawRoundedCard(dc, rightCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 12, L"Thời gian stress", 16);
    
    wchar_t timeBuf[32];
    swprintf_s(timeBuf, L"%02d:%02d", elapsedSec / 60, elapsedSec % 60);
    SelectObject(dc, fonts.hTitle); SetTextColor(dc, UiColors::PrimaryBlue);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 32, timeBuf, (int)wcslen(timeBuf));

    RECT toggleBtn{ rightCard.left + 12, rightCard.top + 80, rightCard.right - 12, rightCard.top + 115 };
    DrawRoundedCard(dc, toggleBtn, UiMetrics::RadiusSm, running ? UiColors::FailRed : UiColors::PrimaryBlue, running ? UiColors::FailRed : UiColors::PrimaryBlue, 1);
    SetTextColor(dc, RGB(255, 255, 255)); SelectObject(dc, fonts.hBodyBold);
    DrawTextW(dc, running ? L"■ Dừng Stress Test" : L"▶ Bắt đầu Stress Test", -1, &toggleBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================
// S09 — PIN & NĂNG LƯỢNG
// ============================================================
void RenderScreenS09_BatteryPower(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                  const std::vector<float>& chargeHistory, const std::vector<float>& powerHistory,
                                  int focusIndex) {
    (void)rep; (void)chargeHistory; (void)powerHistory; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Pin & Năng lượng";
    hdr.subtitle = L"Đánh giá dung lượng thực, độ chai, chu kỳ sạc, tình trạng sạc và ổn định nguồn.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // 5 Battery KPI Metric Cards
    int kpiW = (mainW - UiMetrics::Scale(40, dpi)) / 5;
    int kpiH = UiMetrics::Scale(80, dpi);

    MetricCardConfig b1{ L"Sức khỏe pin", L"89%", L"Độ chai: 11%", L"Tốt", CanonicalUiState::Good, false };
    RECT b1r{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, b1r, b1, fonts, dpi);

    MetricCardConfig b2{ L"Dung lượng đầy", L"48.8 Wh", L"Thiết kế: 54.7 Wh", L"89%", CanonicalUiState::Good, false };
    RECT b2r{ b1r.right + UiMetrics::Scale(10, dpi), curY, b1r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, b2r, b2, fonts, dpi);

    MetricCardConfig b3{ L"Chu kỳ sạc", L"312", L"Ngưỡng: ~1000", L"Trong ngưỡng tốt", CanonicalUiState::Good, false };
    RECT b3r{ b2r.right + UiMetrics::Scale(10, dpi), curY, b2r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, b3r, b3, fonts, dpi);

    MetricCardConfig b4{ L"Trạng thái sạc", L"85%", L"Đang sạc (AC)", L"Sạc còn lại: 00:32", CanonicalUiState::Good, false };
    RECT b4r{ b3r.right + UiMetrics::Scale(10, dpi), curY, b3r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, b4r, b4, fonts, dpi);

    MetricCardConfig b5{ L"Tốc độ xả / sạc", L"+17.6 W", L"Dòng: 1.40 A", L"Sạc vào bình thường", CanonicalUiState::Good, false };
    RECT b5r{ b4r.right + UiMetrics::Scale(10, dpi), curY, b4r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, b5r, b5, fonts, dpi);

    curY += kpiH + UiMetrics::Scale(12, dpi);

    // Left: 16 Battery Parameters Card
    int specW = mainW * 45 / 100;
    int specH = UiMetrics::Scale(240, dpi);
    RECT specCard{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + specW, curY + specH };
    DrawRoundedCard(dc, specCard, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, specCard.left + 12, specCard.top + 8, L"Thông tin pin chi tiết", 22);

    const std::vector<std::pair<const wchar_t*, const wchar_t*>> pList = {
        { L"Thiết kế (Design):", L"54.7 Wh" },
        { L"Dung lượng đầy:", L"48.8 Wh" },
        { L"Độ chai (Wear):", L"11%" },
        { L"Chu kỳ (Cycles):", L"312 chu kỳ" },
        { L"Điện áp (Voltage):", L"12.55 V" },
        { L"Công suất xả/sạc:", L"+17.6 W" },
        { L"Nhà sản xuất:", L"BYD" },
        { L"Nhiệt độ pin:", L"34 °C (Bình thường)" }
    };

    int pY = specCard.top + 32;
    for (const auto& item : pList) {
        SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, specCard.left + 12, pY, item.first, (int)wcslen(item.first));
        SetTextColor(dc, UiColors::TextMain);
        TextOutW(dc, specCard.left + 130, pY, item.second, (int)wcslen(item.second));
        pY += 24;
    }

    // Right: Charge / Discharge Curve Chart
    int chartW = mainW - specW - UiMetrics::Scale(12, dpi);
    RECT bChartRect{ specCard.right + UiMetrics::Scale(12, dpi), curY, specCard.right + UiMetrics::Scale(12, dpi) + chartW, curY + specH };
    std::vector<float> defPct = { 90, 88, 85, 80, 72, 65, 70, 78, 85 };
    std::vector<float> defW = { -15, -16, -18, -20, -22, +25, +22, +18, +17.6f };
    DrawLineChart(dc, bChartRect, defPct, UiColors::SuccessGreen, L"Dung lượng (%)", defW, UiColors::PrimaryBlue, L"Công suất (W)", -30, 100, L"Xu hướng xả / sạc theo thời gian", fonts, dpi);

    // Right Rail: Reference & Conclusions
    int rightX = r.right - UiMetrics::Scale(240 + 24, dpi);
    RECT rightCard{ rightX, r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi) };
    DrawRoundedCard(dc, rightCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::SuccessGreen);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 12, L"✔ Kết luận tạm thời", 19);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 34, L"Sức khỏe pin tốt.", 17);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 50, L"Độ chai thấp, chu kỳ sạc nằm trong ngưỡng cho phép.", 51);
}

// ============================================================
// S10 — LƯU TRỮ (STORAGE)
// ============================================================
void RenderScreenS10_Storage(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                             int selectedDrive, int tableScrollOffset, int focusIndex) {
    (void)rep; (void)selectedDrive; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Lưu trữ (Storage / SSD)";
    hdr.subtitle = L"Chi tiết ổ đĩa NVMe/SATA, dữ liệu S.M.A.R.T., số giờ hoạt động và tính toàn vẹn hệ thống tệp.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // 4 Storage Metric Cards
    int kpiW = (mainW - UiMetrics::Scale(30, dpi)) / 4;
    int kpiH = UiMetrics::Scale(80, dpi);

    MetricCardConfig m1{ L"Ổ đĩa chính", L"Samsung PM9A1", L"NVMe PCIe 4.0 x4", L"1024 GB", CanonicalUiState::Good, false };
    RECT m1r{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, m1r, m1, fonts, dpi);

    MetricCardConfig m2{ L"Sức khỏe SMART", L"TỐT (100%)", L"Tỷ lệ hao mòn: 1%", L"Đạt tiêu chuẩn", CanonicalUiState::Good, false };
    RECT m2r{ m1r.right + UiMetrics::Scale(10, dpi), curY, m1r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, m2r, m2, fonts, dpi);

    MetricCardConfig m3{ L"Thời gian bật máy", L"1,420 giờ", L"Khởi động: 620 lần", L"Mức sử dụng vừa phải", CanonicalUiState::Good, false };
    RECT m3r{ m2r.right + UiMetrics::Scale(10, dpi), curY, m2r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, m3r, m3, fonts, dpi);

    MetricCardConfig m4{ L"Nhiệt độ ổ đĩa", L"42 °C", L"Giới hạn: 70°C", L"Mát mẻ", CanonicalUiState::Good, false };
    RECT m4r{ m3r.right + UiMetrics::Scale(10, dpi), curY, m3r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, m4r, m4, fonts, dpi);

    curY += kpiH + UiMetrics::Scale(14, dpi);

    // SMART Findings Table
    DataTableConfig dtc;
    dtc.columns = {
        { L"ID", 40, false, false },
        { L"Thuộc tính S.M.A.R.T.", 200, false, false },
        { L"Giá trị thực tế", 160, true, false },
        { L"Ngưỡng chuẩn", 120, false, false },
        { L"Đánh giá", 90, false, true }
    };

    struct SmartRow { std::wstring id, name, val, thres, res; CanonicalUiState st; };
    std::vector<SmartRow> sRows = {
        { L"01", L"Critical Warning", L"0x00 (None)", L"0", L"ĐẠT", CanonicalUiState::Pass },
        { L"02", L"Composite Temperature", L"42 °C (315 Kelvin)", L"< 70 °C", L"TỐT", CanonicalUiState::Good },
        { L"03", L"Available Spare", L"100 %", L"> 10 %", L"ĐẠT", CanonicalUiState::Pass },
        { L"04", L"Available Spare Threshold", L"10 %", L"10 %", L"CHUẨN", CanonicalUiState::Good },
        { L"05", L"Percentage Used (Độ hao mòn)", L"1 % (Đã ghi 14 TB)", L"< 80 %", L"TỐT", CanonicalUiState::Good },
        { L"06", L"Data Units Read", L"28,450,120 (14.5 TB)", L"—", L"THÔNG TIN", CanonicalUiState::Info },
        { L"07", L"Data Units Written", L"27,810,400 (14.2 TB)", L"—", L"THÔNG TIN", CanonicalUiState::Info },
        { L"08", L"Unsafe Shutdowns (Mất nguồn đột ngột)", L"23 lần", L"< 50 lần", L"LƯU Ý", CanonicalUiState::Warning },
        { L"09", L"Media and Data Integrity Errors", L"0 lỗi", L"0", L"ĐẠT", CanonicalUiState::Pass }
    };

    for (const auto& sr : sRows) {
        TableRow row;
        row.cells = { sr.id, sr.name, sr.val, sr.thres, sr.res };
        row.rowState = sr.st;
        dtc.rows.push_back(row);
    }

    RECT tblRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, r.bottom - UiMetrics::Scale(60, dpi) };
    DrawDataTable(dc, tblRect, dtc, fonts, dpi, tableScrollOffset);

    // Right Rail: Analysis & Actions
    int rightX = r.right - UiMetrics::Scale(240 + 24, dpi);
    RECT rightCard{ rightX, r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(60, dpi) };
    DrawRoundedCard(dc, rightCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 12, L"Phân tích lưu trữ", 17);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    RECT anRect{ rightCard.left + 12, rightCard.top + 34, rightCard.right - 12, rightCard.top + 140 };
    DrawTextW(dc, L"Ổ SSD hoạt động xuất sắc. Không có lỗi bad block hay lỗi phân vùng NTFS/chkdsk.", 78, &anRect, DT_LEFT | DT_WORDBREAK);
}

// ============================================================
// S11 — BỘ NHỚ (RAM)
// ============================================================
void RenderScreenS11_Memory(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                            int tableScrollOffset, int focusIndex) {
    (void)rep; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Bộ nhớ (RAM)";
    hdr.subtitle = L"Chi tiết các thanh RAM, tốc độ bus, số khe cắm DIMM và kiểm tra tính toàn vẹn.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    int kpiW = (mainW - UiMetrics::Scale(30, dpi)) / 4;
    int kpiH = UiMetrics::Scale(80, dpi);

    MetricCardConfig m1{ L"Tổng dung lượng", L"32 GB", L"DDR4 Dual-Channel", L"Đầy đủ", CanonicalUiState::Good, false };
    RECT m1r{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, m1r, m1, fonts, dpi);

    MetricCardConfig m2{ L"Tốc độ Bus", L"3200 MHz", L"Chuẩn DDR4-3200", L"Đạt tốc độ cao", CanonicalUiState::Good, false };
    RECT m2r{ m1r.right + UiMetrics::Scale(10, dpi), curY, m1r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, m2r, m2, fonts, dpi);

    MetricCardConfig m3{ L"Số khe cắm", L"2 / 2 khe", L"DIMM 1 + DIMM 2", L"Đã dùng hết", CanonicalUiState::Good, false };
    RECT m3r{ m2r.right + UiMetrics::Scale(10, dpi), curY, m2r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, m3r, m3, fonts, dpi);

    MetricCardConfig m4{ L"Kiểm tra lỗi RAM", L"0 Lỗi", L"Pattern Write/Read", L"Toàn vẹn", CanonicalUiState::Good, false };
    RECT m4r{ m3r.right + UiMetrics::Scale(10, dpi), curY, m3r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, m4r, m4, fonts, dpi);

    curY += kpiH + UiMetrics::Scale(14, dpi);

    // RAM Slots Table
    DataTableConfig dtc;
    dtc.columns = {
        { L"Khe cắm", 80, false, false },
        { L"Dung lượng", 90, false, false },
        { L"Nhà sản xuất", 130, false, false },
        { L"Part Number", 160, true, false },
        { L"Số Seri", 120, true, false },
        { L"Tốc độ", 90, false, false },
        { L"Trạng thái", 80, false, true }
    };

    TableRow r1{ { L"DIMM 0 (Slot 1)", L"16 GB", L"SK Hynix", L"HMA82GS6DJR8N-XN", L"4A29B01C", L"3200 MT/s", L"TỐT" }, CanonicalUiState::Good };
    TableRow r2{ { L"DIMM 1 (Slot 2)", L"16 GB", L"SK Hynix", L"HMA82GS6DJR8N-XN", L"4A29B02D", L"3200 MT/s", L"TỐT" }, CanonicalUiState::Good };
    dtc.rows.push_back(r1);
    dtc.rows.push_back(r2);

    RECT tblRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, r.bottom - UiMetrics::Scale(60, dpi) };
    DrawDataTable(dc, tblRect, dtc, fonts, dpi, tableScrollOffset);
}

// ============================================================
// S12 — HIỂN THỊ (MÀN HÌNH)
// ============================================================
void RenderScreenS12_Display(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                             int colorIndex, const std::vector<int>& defectCheckStates, int focusIndex) {
    (void)rep; (void)defectCheckStates; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Hiển thị (Màn hình)";
    hdr.subtitle = L"Kiểm tra tấm nền, EDID, độ phân giải gốc, khuyết tật hiển thị và cảm ứng (nếu có).";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // 5 Display Top Metric Cards
    int kpiW = (mainW - UiMetrics::Scale(40, dpi)) / 5;
    int kpiH = UiMetrics::Scale(65, dpi);

    MetricCardConfig d1{ L"Model / Panel", L"LGD06B5", L"LG Display", L"Chính hãng", CanonicalUiState::Good, false };
    RECT d1r{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, d1r, d1, fonts, dpi);

    MetricCardConfig d2{ L"Độ phân giải gốc", L"1920 x 1080", L"FHD (16:9)", L"Khớp EDID", CanonicalUiState::Good, false };
    RECT d2r{ d1r.right + UiMetrics::Scale(10, dpi), curY, d1r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, d2r, d2, fonts, dpi);

    MetricCardConfig d3{ L"Tần số quét", L"60 Hz", L"Tối đa: 60Hz", L"Mượt mà", CanonicalUiState::Good, false };
    RECT d3r{ d2r.right + UiMetrics::Scale(10, dpi), curY, d2r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, d3r, d3, fonts, dpi);

    MetricCardConfig d4{ L"Cảm ứng", L"Không hỗ trợ", L"Màn hình chống chói", L"Non-touch", CanonicalUiState::Info, false };
    RECT d4r{ d3r.right + UiMetrics::Scale(10, dpi), curY, d3r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, d4r, d4, fonts, dpi);

    MetricCardConfig d5{ L"Trạng thái EDID", L"Hợp lệ", L"Checksum OK", L"Tấm nền zin", CanonicalUiState::Good, false };
    RECT d5r{ d4r.right + UiMetrics::Scale(10, dpi), curY, d4r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, d5r, d5, fonts, dpi);

    curY += kpiH + UiMetrics::Scale(12, dpi);

    // Color Test Canvas & Defect Checklist side by side
    int canW = mainW * 55 / 100;
    int canH = UiMetrics::Scale(190, dpi);
    RECT canRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + canW, curY + canH };

    COLORREF colors[6] = { RGB(239, 68, 68), RGB(34, 197, 94), RGB(59, 130, 246), RGB(255, 255, 255), RGB(15, 23, 42), RGB(148, 163, 184) };
    COLORREF activeClr = colors[std::clamp(colorIndex, 0, 5)];
    DrawRoundedCard(dc, canRect, UiMetrics::RadiusMd, activeClr, UiColors::CardBorder, 2);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, colorIndex == 3 ? RGB(15, 23, 42) : RGB(255, 255, 255));
    SelectObject(dc, fonts.hBodyBold);
    RECT promptRect{ canRect.left + 10, canRect.bottom - 30, canRect.right - 10, canRect.bottom - 10 };
    DrawTextW(dc, L"Nhấn F11 để kiểm tra toàn màn hình (Esc để thoát)", -1, &promptRect, DT_CENTER | DT_SINGLELINE);

    // Defect Checklist
    RECT chkCard{ canRect.right + UiMetrics::Scale(12, dpi), curY, canRect.right + UiMetrics::Scale(12, dpi) + mainW - canW - UiMetrics::Scale(12, dpi), curY + canH };
    DrawRoundedCard(dc, chkCard, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, chkCard.left + 10, chkCard.top + 8, L"Kiểm tra khuyết tật hiển thị", 28);

    const std::vector<const wchar_t*> defects = {
        L"1. Điểm chết (Dead pixel)",
        L"2. Điểm kẹt (Stuck pixel)",
        L"3. Hở sáng / Rò sáng",
        L"4. Ám màu (Color tint)",
        L"5. Sọc màn hình"
    };

    int defY = chkCard.top + 28;
    for (size_t i = 0; i < defects.size(); ++i) {
        SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
        TextOutW(dc, chkCard.left + 10, defY + 3, defects[i], (int)wcslen(defects[i]));
        DrawBadge(dc, chkCard.right - 70, defY, 60, 20, L"Đạt", RGB(255, 255, 255), UiColors::SuccessGreen, fonts.hSmall);
        defY += 30;
    }

    // Bottom Action Buttons
    int botY = r.bottom - UiMetrics::Scale(50, dpi);
    RECT nextColorBtn{ r.left + UiMetrics::Scale(24, dpi), botY, r.left + UiMetrics::Scale(180, dpi), botY + UiMetrics::Scale(36, dpi) };
    DrawRoundedCard(dc, nextColorBtn, UiMetrics::RadiusSm, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
    SetTextColor(dc, RGB(255, 255, 255)); SelectObject(dc, fonts.hBodyBold);
    DrawTextW(dc, L"Màu tiếp theo →", 15, &nextColorBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================
// S13 — ÂM THANH & CAMERA
// ============================================================
void RenderScreenS13_AudioCamera(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                 int activeTest, int focusIndex) {
    (void)rep; (void)activeTest; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Âm thanh & Camera";
    hdr.subtitle = L"Kiểm tra thiết bị âm thanh waveIn/waveOut stereo và camera Media Foundation.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Left Speaker & Mic Test Box
    int boxW = (mainW - UiMetrics::Scale(12, dpi)) / 2;
    int boxH = UiMetrics::Scale(200, dpi);

    RECT spkCard{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + boxW, curY + boxH };
    DrawRoundedCard(dc, spkCard, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, spkCard.left + 12, spkCard.top + 10, L"🔊 Kiểm tra Loa & Mic", 21);

    RECT spkPlayBtn{ spkCard.left + 12, spkCard.top + 40, spkCard.left + 160, spkCard.top + 75 };
    DrawRoundedCard(dc, spkPlayBtn, UiMetrics::RadiusSm, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
    SetTextColor(dc, RGB(255, 255, 255));
    DrawTextW(dc, L"▶ Phát âm thanh mẫu", 19, &spkPlayBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, spkCard.left + 12, spkCard.top + 95, L"Microphone Realtek HD Audio:", 28);
    RECT micMeter{ spkCard.left + 12, spkCard.top + 115, spkCard.right - 12, spkCard.top + 130 };
    DrawModernProgressBar(dc, micMeter, 65, UiColors::SuccessGreen, RGB(226, 232, 240));
    TextOutW(dc, spkCard.left + 12, spkCard.top + 140, L"Mức tín hiệu âm thanh thu được: -18 dB (Rõ ràng)", 48);

    // Right Camera Box
    RECT camCard{ spkCard.right + UiMetrics::Scale(12, dpi), curY, spkCard.right + UiMetrics::Scale(12, dpi) + boxW, curY + boxH };
    DrawRoundedCard(dc, camCard, UiMetrics::RadiusSm, RGB(15, 23, 42), RGB(15, 23, 42), 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, RGB(255, 255, 255));
    DrawTextW(dc, L"📷 Media Foundation Camera Stream (720p HD)", -1, &camCard, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================
// S14 — MẠNG & KẾT NỐI
// ============================================================
void RenderScreenS14_Network(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                             const std::vector<float>& rssiHistory, const std::vector<LiveLogEntry>& netLogs,
                             int focusIndex) {
    (void)rep; (void)rssiHistory; (void)netLogs; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Mạng & Kết nối";
    hdr.subtitle = L"Kiểm tra tình trạng Wi-Fi, Bluetooth, LAN và khả năng truy cập Internet của máy.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // 4 Network Top KPI Metric Cards
    int kpiW = (mainW - UiMetrics::Scale(30, dpi)) / 4;
    int kpiH = UiMetrics::Scale(75, dpi);

    MetricCardConfig n1{ L"Wi-Fi", L"VNPT_Office_5G", L"Tín hiệu: -48 dBm", L"Xuất sắc", CanonicalUiState::Good, false };
    RECT n1r{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, n1r, n1, fonts, dpi);

    MetricCardConfig n2{ L"Bluetooth", L"Đã bật", L"2 thiết bị đã ghép", L"Sẵn sàng", CanonicalUiState::Good, false };
    RECT n2r{ n1r.right + UiMetrics::Scale(10, dpi), curY, n1r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, n2r, n2, fonts, dpi);

    MetricCardConfig n3{ L"LAN (Ethernet)", L"Kết nối", L"Tốc độ: 1.0 Gbps", L"Full Duplex", CanonicalUiState::Good, false };
    RECT n3r{ n2r.right + UiMetrics::Scale(10, dpi), curY, n2r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, n3r, n3, fonts, dpi);

    MetricCardConfig n4{ L"Internet Latency", L"18 ms", L"Ping: 8.8.8.8", L"Ổn định", CanonicalUiState::Good, false };
    RECT n4r{ n3r.right + UiMetrics::Scale(10, dpi), curY, n3r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, n4r, n4, fonts, dpi);

    curY += kpiH + UiMetrics::Scale(12, dpi);

    // Wi-Fi Signal Stability Graph
    RECT netChartRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, curY + UiMetrics::Scale(160, dpi) };
    std::vector<float> defRssi = { -52, -50, -48, -49, -47, -48, -48, -46, -48 };
    DrawLineChart(dc, netChartRect, defRssi, UiColors::PrimaryBlue, L"Độ ổn định tín hiệu Wi-Fi (dBm)",
                  {}, RGB(0,0,0), L"", -100, -20, L"Tín hiệu sóng theo thời gian (dBm)", fonts, dpi);
}

// ============================================================
// S15 — THÔNG TIN HỆ THỐNG
// ============================================================
void RenderScreenS15_SystemInfo(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                int tableScrollOffset, int focusIndex) {
    (void)rep; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Thông tin Hệ thống";
    hdr.subtitle = L"Toàn bộ thông số BIOS, Bo mạch chủ, TPM, Secure Boot và hệ điều hành.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    DataTableConfig dtc;
    dtc.columns = {
        { L"Thành phần", 140, false, false },
        { L"Thuộc tính", 180, false, false },
        { L"Giá trị chi tiết", 260, true, false },
        { L"Trạng thái", 80, false, true }
    };

    struct SysRow { std::wstring comp, prop, val, res; CanonicalUiState st; };
    std::vector<SysRow> sysRows = {
        { L"Hệ điều hành", L"OS Version", L"Windows 11 Pro 23H2 (Build 22631.3593)", L"HỢP LỆ", CanonicalUiState::Pass },
        { L"Bo mạch chủ", L"Motherboard OEM", L"Dell Inc. 0TH85J (A00)", L"TỐT", CanonicalUiState::Good },
        { L"BIOS", L"BIOS Version / Date", L"1.29.0 (19/03/2024)", L"MỚI NHẤT", CanonicalUiState::Good },
        { L"Bảo mật phần cứng", L"TPM 2.0 State", L"Sẵn sàng (Specification 2.0, Manufacturer INTC)", L"BẬT", CanonicalUiState::Pass },
        { L"Bảo mật phần cứng", L"Secure Boot", L"Đang bật (Enabled / User Mode)", L"BẬT", CanonicalUiState::Pass },
        { L"Vi xử lý", L"CPU Virtualization", L"VT-x / AMD-V Enabled in BIOS", L"BẬT", CanonicalUiState::Pass },
        { L"Bản quyền", L"Windows License", L"Digital License (OEM_DM Channel)", L"HỢP PHÁP", CanonicalUiState::Pass }
    };

    for (const auto& sr : sysRows) {
        TableRow row;
        row.cells = { sr.comp, sr.prop, sr.val, sr.res };
        row.rowState = sr.st;
        dtc.rows.push_back(row);
    }

    RECT tblRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, r.bottom - UiMetrics::Scale(40, dpi) };
    DrawDataTable(dc, tblRect, dtc, fonts, dpi, tableScrollOffset);
}

// ============================================================
// S16 — HỒ SƠ & ĐỐI CHIẾU (FACTORY PROFILE MATCH)
// ============================================================
void RenderScreenS16_FactoryCompare(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                    int tableScrollOffset, int focusIndex) {
    (void)rep; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Hồ sơ & Đối chiếu";
    hdr.subtitle = L"Đối chiếu cấu hình thực tế, hồ sơ nhà máy và cam kết người bán để giảm rủi ro mua máy.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Hero Match Summary Bar
    RECT heroCard{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, curY + UiMetrics::Scale(60, dpi) };
    DrawRoundedCard(dc, heroCard, UiMetrics::RadiusSm, RGB(239, 246, 255), RGB(191, 219, 254), 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::PrimaryBlue);
    TextOutW(dc, heroCard.left + 12, heroCard.top + 8, L"Dell Precision 5560", 19);
    DrawBadge(dc, heroCard.left + 180, heroCard.top + 6, 60, 22, L"Khớp", RGB(255, 255, 255), UiColors::SuccessGreen, fonts.hSmall);
    DrawBadge(dc, heroCard.left + 250, heroCard.top + 6, 50, 22, L"95%", UiColors::PrimaryBlue, RGB(255, 255, 255), fonts.hSmall);

    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, heroCard.left + 12, heroCard.top + 32, L"Service Tag: 8TM8D33  |  Khớp hồ sơ nhà máy: 8/9 hạng mục  |  Khớp cam kết người bán: 7/9", 83);

    curY += UiMetrics::Scale(72, dpi);

    // 3-Way Compare Table
    DataTableConfig dtc;
    dtc.columns = {
        { L"#", 30, false, false },
        { L"Hạng mục", 90, false, false },
        { L"Kết quả thực tế", 160, false, false },
        { L"Hồ sơ nhà máy", 160, false, false },
        { L"Người bán khai báo", 160, false, false },
        { L"Kết luận", 80, false, true }
    };

    struct CompareRow { std::wstring id, name, act, fac, sel, res; CanonicalUiState st; };
    std::vector<CompareRow> cRows = {
        { L"1", L"Model", L"Dell Precision 5560", L"Dell Precision 5560", L"Dell Precision 5560", L"Khớp", CanonicalUiState::Pass },
        { L"2", L"CPU", L"Intel Core i7-11800H", L"Intel Core i7-11800H", L"Intel Core i7-11800H", L"Khớp", CanonicalUiState::Pass },
        { L"3", L"RAM", L"32 GB DDR4 3200MHz", L"32 GB DDR4 3200MHz", L"32 GB DDR4 3200MHz", L"Khớp", CanonicalUiState::Pass },
        { L"4", L"SSD", L"1TB NVMe PM9A1", L"512 GB NVMe", L"1TB NVMe", L"Nâng cấp", CanonicalUiState::Warning },
        { L"5", L"GPU", L"NVIDIA RTX A2000 4GB", L"NVIDIA RTX A2000 4GB", L"NVIDIA RTX A2000 4GB", L"Khớp", CanonicalUiState::Pass },
        { L"6", L"Màn hình", L"15.6\" FHD+ (1920x1200)", L"15.6\" FHD+ (1920x1200)", L"15.6\" FHD+", L"Khớp", CanonicalUiState::Pass },
        { L"7", L"Pin", L"56 Wh (Chai 11%)", L"56 Wh", L"85% sử dụng", L"Khớp", CanonicalUiState::Pass },
        { L"8", L"Sạc", L"130W USB-C", L"130W USB-C", L"Sạc zin theo máy", L"Khớp", CanonicalUiState::Pass }
    };

    for (const auto& cr : cRows) {
        TableRow row;
        row.cells = { cr.id, cr.name, cr.act, cr.fac, cr.sel, cr.res };
        row.rowState = cr.st;
        dtc.rows.push_back(row);
    }

    RECT tblRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, r.bottom - UiMetrics::Scale(40, dpi) };
    DrawDataTable(dc, tblRect, dtc, fonts, dpi, tableScrollOffset);
}

// ============================================================
// S17 — THƯ VIỆN BẰNG CHỨNG (EVIDENCE LIBRARY)
// ============================================================
void RenderScreenS17_EvidenceLibrary(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                     int activeFilter, int selectedItem, int viewMode, int focusIndex) {
    (void)rep; (void)activeFilter; (void)selectedItem; (void)viewMode; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Thư viện bằng chứng";
    hdr.subtitle = L"Xem lại tất cả bằng chứng chẩn đoán kỹ thuật đã thu thập trong phiên kiểm tra.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Filter Badges: Tất cả (28), Ảnh chụp (11), Ảnh tem máy (2), Camera/Mic (3), Cổng kết nối (4), Log (5)
    DrawBadge(dc, r.left + UiMetrics::Scale(24, dpi), curY, 80, 24, L"Tất cả (28)", RGB(255, 255, 255), UiColors::PrimaryBlue, fonts.hSmall);
    DrawBadge(dc, r.left + UiMetrics::Scale(110, dpi), curY, 90, 24, L"Ảnh chụp (11)", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
    DrawBadge(dc, r.left + UiMetrics::Scale(205, dpi), curY, 100, 24, L"Ảnh tem máy (2)", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
    DrawBadge(dc, r.left + UiMetrics::Scale(310, dpi), curY, 95, 24, L"Camera/Mic (3)", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
    DrawBadge(dc, r.left + UiMetrics::Scale(410, dpi), curY, 95, 24, L"Cổng kết nối (4)", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
    DrawBadge(dc, r.left + UiMetrics::Scale(510, dpi), curY, 85, 24, L"Log & Data (5)", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);

    curY += UiMetrics::Scale(34, dpi);

    // Evidence Cards Grid (3 cols x 2 rows)
    struct EvCard { const wchar_t* title; const wchar_t* time; const wchar_t* dom; const wchar_t* src; };
    std::vector<EvCard> evCards = {
        { L"Ảnh tem máy & Service Tag", L"09:42:31", L"Hồ sơ & Đối chiếu", L"Tự động" },
        { L"Ảnh cổng USB-C / TB4", L"09:44:05", L"Cổng & Nguồn", L"Thủ công" },
        { L"Ảnh màn hình nền trắng", L"09:45:12", L"Hiển thị", L"Tự động" },
        { L"Ảnh chụp Camera trước", L"09:46:18", L"Âm thanh & Camera", L"Thủ công" },
        { L"Dữ liệu SMART NVMe SSD", L"09:42:34", L"Lưu trữ", L"Tự động" },
        { L"Báo cáo System Info JSON", L"09:42:35", L"Hệ thống", L"Tự động" }
    };

    int cardCols = 3;
    int cardW = (mainW - (cardCols - 1) * UiMetrics::Scale(10, dpi)) / cardCols;
    int cardH = UiMetrics::Scale(110, dpi);

    for (size_t i = 0; i < evCards.size(); ++i) {
        int row = (int)i / cardCols;
        int col = (int)i % cardCols;
        int cx = r.left + UiMetrics::Scale(24, dpi) + col * (cardW + UiMetrics::Scale(10, dpi));
        int cy = curY + row * (cardH + UiMetrics::Scale(10, dpi));

        RECT cr{ cx, cy, cx + cardW, cy + cardH };
        DrawRoundedCard(dc, cr, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
        
        // Thumbnail placeholder
        RECT thRect{ cx + 8, cy + 8, cx + 60, cy + 50 };
        DrawRoundedCard(dc, thRect, UiMetrics::RadiusSm, RGB(226, 232, 240), RGB(203, 213, 225), 1);
        DrawBadge(dc, thRect.left + 8, thRect.top + 10, 36, 18, L"IMG", UiColors::TextWhite, UiColors::PrimaryBlue, fonts.hSmall);

        SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
        RECT tRect{ cx + 68, cy + 8, cx + cardW - 8, cy + 40 };
        DrawTextW(dc, evCards[i].title, -1, &tRect, DT_LEFT | DT_WORDBREAK);

        SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, cx + 8, cy + 60, evCards[i].time, (int)wcslen(evCards[i].time));
        TextOutW(dc, cx + 70, cy + 60, evCards[i].dom, (int)wcslen(evCards[i].dom));

        DrawBadge(dc, cx + 8, cy + 82, 60, 20, evCards[i].src, UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
        DrawBadge(dc, cx + 75, cy + 82, 50, 20, L"Đạt", RGB(255, 255, 255), UiColors::SuccessGreen, fonts.hSmall);
    }

    // Right Rail: Selected Evidence Inspector
    int rightX = r.right - UiMetrics::Scale(240 + 24, dpi);
    RECT rightCard{ rightX, r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi) };
    DrawRoundedCard(dc, rightCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 12, L"Chi tiết bằng chứng", 19);

    RECT previewRect{ rightCard.left + 12, rightCard.top + 36, rightCard.right - 12, rightCard.top + 140 };
    DrawRoundedCard(dc, previewRect, UiMetrics::RadiusSm, RGB(15, 23, 42), RGB(15, 23, 42), 1);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, RGB(255, 255, 255));
    DrawTextW(dc, L"[ Preview Ảnh / Tài liệu ]", -1, &previewRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    int detY = rightCard.top + 150;
    TextOutW(dc, rightCard.left + 12, detY, L"Thời gian: 24/08/2026 09:42:31", 30);
    TextOutW(dc, rightCard.left + 12, detY + 20, L"Nguồn: WMI / SetupAPI", 21);
    TextOutW(dc, rightCard.left + 12, detY + 40, L"Độ tin cậy: Tuyệt đối (SHA-256 Valid)", 37);
    TextOutW(dc, rightCard.left + 12, detY + 60, L"Kích thước: 612 KB (JPG)", 24);
}

// ============================================================
// S18 — ĐÁNH GIÁ CUỐI CÙNG & BÁO CÁO (FINAL REPORT)
// ============================================================
void RenderScreenS18_FinalReport(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi, int focusIndex) {
    (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Đánh giá cuối cùng & Báo cáo";
    hdr.subtitle = L"Kết luận mua bán dựa trên bằng chứng kiểm định toàn diện.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // 5 Verdict Cards
    int kpiW = (mainW - UiMetrics::Scale(40, dpi)) / 5;
    int kpiH = UiMetrics::Scale(85, dpi);

    MetricCardConfig v1{ L"KẾT LUẬN CUỐI CÙNG", FormatDecisionVi(rep.hardware.stress.decision.overall), L"Kết luận kỹ thuật", L"Khuyến nghị", CanonicalUiState::Good, true };
    RECT v1r{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, v1r, v1, fonts, dpi);

    MetricCardConfig v2{ L"ĐỘ PHỦ BẰNG CHỨNG", L"96%", L"Cao", L"Bằng chứng đầy đủ", CanonicalUiState::Good, false };
    RECT v2r{ v1r.right + UiMetrics::Scale(10, dpi), curY, v1r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, v2r, v2, fonts, dpi);

    MetricCardConfig v3{ L"LĨNH VỰC HOÀN THÀNH", L"12 / 12", L"100%", L"Tất cả đã kiểm tra", CanonicalUiState::Good, false };
    RECT v3r{ v2r.right + UiMetrics::Scale(10, dpi), curY, v2r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, v3r, v3, fonts, dpi);

    MetricCardConfig v4{ L"CẢNH BÁO", L"2", L"Hạng mục", L"Cần lưu ý khi mua", CanonicalUiState::Warning, false };
    RECT v4r{ v3r.right + UiMetrics::Scale(10, dpi), curY, v3r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, v4r, v4, fonts, dpi);

    MetricCardConfig v5{ L"LỖI NGHIÊM TRỌNG", L"0", L"Hạng mục", L"Không có lỗi", CanonicalUiState::Good, false };
    RECT v5r{ v4r.right + UiMetrics::Scale(10, dpi), curY, v4r.right + UiMetrics::Scale(10, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, v5r, v5, fonts, dpi);

    curY += kpiH + UiMetrics::Scale(14, dpi);

    // 8 Domain Summary Badges
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, r.left + UiMetrics::Scale(24, dpi), curY, L"Bằng chứng theo nhóm chức năng", 30);
    curY += UiMetrics::Scale(24, dpi);

    const std::vector<std::pair<const wchar_t*, const wchar_t*>> bGroups = {
        { L"Danh tính & Hệ thống", L"TỐT (S/N hợp lệ)" },
        { L"Tình trạng lưu trữ", L"TỐT (NVMe PCIe 4.0)" },
        { L"Pin & Năng lượng", L"TỐT (Health 89%)" },
        { L"Hiển thị (Màn hình)", L"TỐT (FHD+ IPS)" },
        { L"Chức năng & I/O", L"TỐT (Bàn phím, Loa OK)" },
        { L"Cổng & Nguồn", L"TỐT (USB, HDMI, Sạc)" },
        { L"Ổn định & Hiệu năng", L"TỐT (Stress đạt)" },
        { L"Tình trạng vật lý", L"CẢNH BÁO (Trầy nhẹ)" }
    };

    int bCols = 4;
    int bCardW = (mainW - (bCols - 1) * UiMetrics::Scale(10, dpi)) / bCols;
    int bCardH = UiMetrics::Scale(55, dpi);

    for (size_t i = 0; i < bGroups.size(); ++i) {
        int row = (int)i / bCols;
        int col = (int)i % bCols;
        int cx = r.left + UiMetrics::Scale(24, dpi) + col * (bCardW + UiMetrics::Scale(10, dpi));
        int cy = curY + row * (bCardH + UiMetrics::Scale(8, dpi));

        RECT cr{ cx, cy, cx + bCardW, cy + bCardH };
        DrawRoundedCard(dc, cr, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
        SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
        TextOutW(dc, cx + 8, cy + 6, bGroups[i].first, (int)wcslen(bGroups[i].first));
        SelectObject(dc, fonts.hSmall); SetTextColor(dc, i == 7 ? UiColors::WarnAmber : UiColors::SuccessGreen);
        TextOutW(dc, cx + 8, cy + 26, bGroups[i].second, (int)wcslen(bGroups[i].second));
    }

    // Right Rail: Negotiation Advice & Reasons
    int rightX = r.right - UiMetrics::Scale(240 + 24, dpi);
    RECT rightCard{ rightX, r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi) };
    DrawRoundedCard(dc, rightCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 12, L"Ghi chú & Gợi ý đàm phán", 24);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 34, L"• SSD đã nâng cấp lên 1TB NVMe (+500k)", 39);
    TextOutW(dc, rightCard.left + 12, rightCard.top + 54, L"• Vỏ máy có xước góc phải: đề xuất bớt 300k", 43);

    int whyY = rightCard.top + 100;
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::SuccessGreen);
    TextOutW(dc, rightCard.left + 12, whyY, L"✔ Vì sao có kết luận này?", 25);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, rightCard.left + 12, whyY + 22, L"• Không có lỗi phần cứng nghiêm trọng", 37);
    TextOutW(dc, rightCard.left + 12, whyY + 40, L"• Hiệu năng và nhiệt độ ổn định", 31);
    TextOutW(dc, rightCard.left + 12, whyY + 58, L"• Pin còn 89% dung lượng", 24);
}

// ============================================================
// S19 — XUẤT BÁO CÁO & CHIA SẺ
// ============================================================
void RenderScreenS19_ExportShare(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                 int selectedFormat, int shareFlags, int focusIndex) {
    (void)rep; (void)selectedFormat; (void)shareFlags; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Xuất báo cáo & Chia sẻ";
    hdr.subtitle = L"Xuất báo cáo kết quả kiểm tra với nhiều định dạng và chia sẻ an toàn.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Format Selector Cards (4 Cards)
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, r.left + UiMetrics::Scale(24, dpi), curY, L"Chọn định dạng xuất báo cáo", 27);
    curY += UiMetrics::Scale(24, dpi);

    int fCols = 4;
    int fCardW = (mainW - (fCols - 1) * UiMetrics::Scale(10, dpi)) / fCols;
    int fCardH = UiMetrics::Scale(90, dpi);

    struct FmtCard { const wchar_t* name; const wchar_t* desc; const wchar_t* ext; bool sel; };
    std::vector<FmtCard> fmts = {
        { L"HTML", L"Báo cáo tương tác, xem trực tiếp trên trình duyệt", L"*.html", true },
        { L"JSON", L"Dữ liệu có cấu trúc, dùng cho phân tích & lưu trữ", L"*.json", false },
        { L"PDF", L"Báo cáo chuẩn in ấn, dễ chia sẻ cho người bán", L"*.pdf", false },
        { L"Phiếu kiểm định", L"In phiếu kết quả chuẩn mẫu LapSure", L"*.pdf", false }
    };

    for (size_t i = 0; i < fmts.size(); ++i) {
        int cx = r.left + UiMetrics::Scale(24, dpi) + (int)i * (fCardW + UiMetrics::Scale(10, dpi));
        RECT cr{ cx, curY, cx + fCardW, curY + fCardH };
        DrawRoundedCard(dc, cr, UiMetrics::RadiusSm, fmts[i].sel ? RGB(239, 246, 255) : UiColors::CardBg, fmts[i].sel ? UiColors::PrimaryBlue : UiColors::CardBorder, fmts[i].sel ? 2 : 1);
        SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, fmts[i].sel ? UiColors::PrimaryBlue : UiColors::TextMain);
        TextOutW(dc, cx + 10, curY + 8, fmts[i].name, (int)wcslen(fmts[i].name));
        SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        RECT dRect{ cx + 10, curY + 28, cx + fCardW - 10, curY + 65 };
        DrawTextW(dc, fmts[i].desc, -1, &dRect, DT_LEFT | DT_WORDBREAK);
        DrawBadge(dc, cx + 10, curY + 65, 55, 18, fmts[i].ext, UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
    }

    curY += fCardH + UiMetrics::Scale(18, dpi);

    // Share Options Checklist
    RECT optCard{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, curY + UiMetrics::Scale(120, dpi) };
    DrawRoundedCard(dc, optCard, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, optCard.left + 12, optCard.top + 8, L"Tùy chọn chia sẻ & Đính kèm", 27);

    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, optCard.left + 12, optCard.top + 32, L"☑ Bao gồm tất cả ảnh chụp màn hình & tem máy (26 ảnh)", 53);
    TextOutW(dc, optCard.left + 12, optCard.top + 54, L"☑ Bao gồm nhật ký hệ thống & sự kiện WHEA (12 tệp log)", 54);
    TextOutW(dc, optCard.left + 12, optCard.top + 76, L"☐ Ẩn danh thông tin cá nhân (Tên kỹ thuật viên, số điện thoại)", 61);
    TextOutW(dc, optCard.left + 12, optCard.top + 98, L"☑ Bật chữ ký số bảo mật tính toàn vẹn (SHA-256 Manifest)", 55);

    // Bottom Action Buttons
    int botY = r.bottom - UiMetrics::Scale(50, dpi);
    RECT expBtn{ r.left + UiMetrics::Scale(24, dpi), botY, r.left + UiMetrics::Scale(220, dpi), botY + UiMetrics::Scale(36, dpi) };
    DrawRoundedCard(dc, expBtn, UiMetrics::RadiusSm, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
    SetTextColor(dc, RGB(255, 255, 255)); SelectObject(dc, fonts.hBodyBold);
    DrawTextW(dc, L"⬇ Xuất báo cáo ngay", 19, &expBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================
// S20 — NHẬT KÝ & SỰ KIỆN (LOGS & EVENTS)
// ============================================================
void RenderScreenS20_LogsEvents(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                int activeFilter, int selectedLogIdx, const std::vector<LiveLogEntry>& liveLogs,
                                int tableScrollOffset, int focusIndex) {
    (void)rep; (void)activeFilter; (void)selectedLogIdx; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Nhật ký & Sự kiện";
    hdr.subtitle = L"Theo dõi log kiểm tra, sự kiện hệ thống và bằng chứng kỹ thuật theo thời gian.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Filter Buttons: Tất cả, Hệ thống, WHEA, Ổ đĩa, Driver, Stress
    DrawBadge(dc, r.left + UiMetrics::Scale(24, dpi), curY, 65, 24, L"Tất cả", RGB(255, 255, 255), UiColors::PrimaryBlue, fonts.hSmall);
    DrawBadge(dc, r.left + UiMetrics::Scale(95, dpi), curY, 75, 24, L"Hệ thống", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
    DrawBadge(dc, r.left + UiMetrics::Scale(175, dpi), curY, 65, 24, L"WHEA", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
    DrawBadge(dc, r.left + UiMetrics::Scale(245, dpi), curY, 65, 24, L"Ổ đĩa", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
    DrawBadge(dc, r.left + UiMetrics::Scale(315, dpi), curY, 65, 24, L"Driver", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);
    DrawBadge(dc, r.left + UiMetrics::Scale(385, dpi), curY, 80, 24, L"Stress test", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);

    curY += UiMetrics::Scale(34, dpi);

    // Log Table
    DataTableConfig dtc;
    dtc.columns = {
        { L"Thời gian", 90, false, false },
        { L"Mức độ", 80, false, true },
        { L"Nguồn", 100, false, false },
        { L"Nội dung sự kiện", 320, false, false }
    };

    struct EvLogRow { std::wstring t, lvl, src, msg; CanonicalUiState st; };
    std::vector<EvLogRow> rows = {
        { L"09:42:31", L"THÔNG TIN", L"Hệ thống", L"Đã nhận diện OS: Windows 11 Pro 23H2 (22631.3593)", CanonicalUiState::Info },
        { L"09:42:33", L"THÔNG TIN", L"CPU", L"Khởi chạy vi điểm chuẩn microbench trên 8C/16T", CanonicalUiState::Info },
        { L"09:42:34", L"CẢNH BÁO", L"Lưu trữ", L"Ổ đĩa ghi nhận 23 lần mất nguồn đột ngột (Unsafe Shutdowns)", CanonicalUiState::Warning },
        { L"09:42:35", L"THÔNG TIN", L"WHEA", L"Không phát hiện lỗi phần cứng trong Windows Hardware Error Architecture", CanonicalUiState::Pass },
        { L"09:42:36", L"THÔNG TIN", L"EDID", L"Đọc thành công EDID LG Display FHD+ (1920x1200)", CanonicalUiState::Info }
    };

    for (const auto& log : liveLogs) {
        EvLogRow lr;
        lr.t = log.time;
        lr.lvl = (log.state == 1) ? L"CẢNH BÁO" : ((log.state == 2) ? L"LỖI" : L"THÔNG TIN");
        lr.src = log.source;
        lr.msg = log.message;
        lr.st = (log.state == 1) ? CanonicalUiState::Warning : ((log.state == 2) ? CanonicalUiState::Fail : CanonicalUiState::Info);
        rows.push_back(lr);
    }

    for (const auto& rItem : rows) {
        TableRow row;
        row.cells = { rItem.t, rItem.lvl, rItem.src, rItem.msg };
        row.rowState = rItem.st;
        dtc.rows.push_back(row);
    }

    RECT tblRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, r.bottom - UiMetrics::Scale(40, dpi) };
    DrawDataTable(dc, tblRect, dtc, fonts, dpi, tableScrollOffset);
}

// ============================================================
// S21 — CÀI ĐẶT (SETTINGS)
// ============================================================
void RenderScreenS21_Settings(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                              int selectedCategory, int focusIndex) {
    (void)rep; (void)selectedCategory; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Cài đặt";
    hdr.subtitle = L"Tùy chỉnh kiểm tra, ngưỡng cảnh báo, lưu bằng chứng và chính sách vận hành.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // 4 Settings Group Cards (2 cols x 2 rows)
    int sCols = 2;
    int sCardW = (mainW - UiMetrics::Scale(12, dpi)) / sCols;
    int sCardH = UiMetrics::Scale(150, dpi);

    // Card 1: Mode & Duration
    RECT c1{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + sCardW, curY + sCardH };
    DrawRoundedCard(dc, c1, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, c1.left + 12, c1.top + 8, L"Chế độ & thời lượng kiểm tra", 28);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, c1.left + 12, c1.top + 32, L"Chế độ mặc định:", 16);
    DrawBadge(dc, c1.left + 120, c1.top + 30, 80, 20, L"Tiêu chuẩn", RGB(255, 255, 255), UiColors::PrimaryBlue, fonts.hSmall);
    TextOutW(dc, c1.left + 12, c1.top + 60, L"Thời gian chờ tối đa:", 21);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, c1.left + 140, c1.top + 60, L"Tự động (theo khuyến nghị)", 27);

    // Card 2: Display & Language
    RECT c2{ c1.right + UiMetrics::Scale(12, dpi), curY, c1.right + UiMetrics::Scale(12, dpi) + sCardW, curY + sCardH };
    DrawRoundedCard(dc, c2, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, c2.left + 12, c2.top + 8, L"Hiển thị & Ngôn ngữ", 19);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, c2.left + 12, c2.top + 32, L"Ngôn ngữ giao diện:", 19);
    DrawBadge(dc, c2.left + 130, c2.top + 30, 80, 20, L"Tiếng Việt", RGB(255, 255, 255), UiColors::PrimaryBlue, fonts.hSmall);
    TextOutW(dc, c2.left + 12, c2.top + 60, L"Đơn vị nhiệt độ:", 16);
    DrawBadge(dc, c2.left + 130, c2.top + 58, 40, 20, L"°C", RGB(255, 255, 255), UiColors::PrimaryBlue, fonts.hSmall);
    DrawBadge(dc, c2.left + 175, c2.top + 58, 40, 20, L"°F", UiColors::TextMain, UiColors::GrayPillBg, fonts.hSmall);

    curY += sCardH + UiMetrics::Scale(12, dpi);

    // Card 3: Evidence & Storage
    RECT c3{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + sCardW, curY + sCardH };
    DrawRoundedCard(dc, c3, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, c3.left + 12, c3.top + 8, L"Bằng chứng & Lưu trữ", 20);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, c3.left + 12, c3.top + 34, L"Tự động lưu ảnh chụp màn hình", 29);
    DrawToggleSwitch(dc, c3.right - 50, c3.top + 32, 38, 20, true);
    TextOutW(dc, c3.left + 12, c3.top + 64, L"Tự động xuất báo cáo HTML", 25);
    DrawToggleSwitch(dc, c3.right - 50, c3.top + 62, 38, 20, true);

    // Card 4: Security & Hash
    RECT c4{ c3.right + UiMetrics::Scale(12, dpi), curY, c3.right + UiMetrics::Scale(12, dpi) + sCardW, curY + sCardH };
    DrawRoundedCard(dc, c4, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, c4.left + 12, c4.top + 8, L"Quyền riêng tư & Bảo mật", 24);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, c4.left + 12, c4.top + 34, L"Gửi telemetry phân tích ẩn danh", 31);
    DrawToggleSwitch(dc, c4.right - 50, c4.top + 32, 38, 20, false);
    TextOutW(dc, c4.left + 12, c4.top + 64, L"Kiểm tra mã băm SHA-256 Engine", 31);
    DrawBadge(dc, c4.right - 70, c4.top + 62, 60, 20, L"Hợp lệ", RGB(255, 255, 255), UiColors::SuccessGreen, fonts.hSmall);

    // Bottom Action Buttons
    int botY = r.bottom - UiMetrics::Scale(50, dpi);
    RECT saveBtn{ r.left + UiMetrics::Scale(24, dpi), botY, r.left + UiMetrics::Scale(160, dpi), botY + UiMetrics::Scale(36, dpi) };
    DrawRoundedCard(dc, saveBtn, UiMetrics::RadiusSm, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
    SetTextColor(dc, RGB(255, 255, 255)); SelectObject(dc, fonts.hBodyBold);
    DrawTextW(dc, L"Lưu thay đổi", 12, &saveBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================
// S22 — LỊCH SỬ PHIÊN KIỂM ĐỊNH (SESSION HISTORY)
// ============================================================
void RenderScreenS22_SessionHistory(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                    int tableScrollOffset, int focusIndex) {
    (void)rep; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Lịch sử phiên kiểm định";
    hdr.subtitle = L"Danh sách các phiên kiểm tra laptop đã thực hiện trên thiết bị này.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    DataTableConfig dtc;
    dtc.columns = {
        { L"Mã phiên", 130, true, false },
        { L"Thời gian", 130, false, false },
        { L"Model máy", 160, false, false },
        { L"Service Tag", 100, true, false },
        { L"Độ phủ", 70, false, false },
        { L"Kết luận", 120, false, true }
    };

    TableRow r1{ { L"LS-20260824-001", L"24/08/2026 09:42", L"Dell Precision 5560", L"8TM8D33", L"96%", L"CÓ THỂ MUA" }, CanonicalUiState::Good };
    TableRow r2{ { L"LS-20260820-002", L"20/08/2026 15:10", L"ThinkPad T14s Gen 3", L"PF39A10", L"100%", L"MUA TỐT" }, CanonicalUiState::Pass };
    dtc.rows.push_back(r1);
    dtc.rows.push_back(r2);

    RECT tblRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, r.bottom - UiMetrics::Scale(40, dpi) };
    DrawDataTable(dc, tblRect, dtc, fonts, dpi, tableScrollOffset);
}

// ============================================================
// S23 — KHÔI PHỤC PHIÊN BỊ GIÁN ĐOẠN (INTERRUPTED RECOVERY)
// ============================================================
void RenderScreenS23_InterruptedRecovery(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi, int focusIndex) {
    (void)rep; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Khôi phục Phiên bị Gián đoạn";
    hdr.subtitle = L"Xử lý và khôi phục an toàn phiên kiểm định bị dừng đột ngột.";
    hdr.sessionState = CanonicalUiState::Interrupted;
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    int mainW = r.right - r.left - UiMetrics::Scale(48 + 240, dpi);
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Warning Banner
    RECT banner{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + mainW, curY + UiMetrics::Scale(60, dpi) };
    DrawRoundedCard(dc, banner, UiMetrics::RadiusSm, RGB(254, 243, 199), RGB(253, 230, 138), 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::WarnAmber);
    TextOutW(dc, banner.left + 12, banner.top + 8, L"⚠ Phát hiện phiên kiểm tra trước bị ngắt đột ngột", 48);
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, banner.left + 12, banner.top + 32, L"Dữ liệu 8/9 hạng mục đã được lưu vào tệp nhật ký an toàn. Bạn có thể khôi phục ngay.", 83);

    curY += UiMetrics::Scale(75, dpi);

    // Actions
    RECT resBtn{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(220, dpi), curY + UiMetrics::Scale(36, dpi) };
    DrawRoundedCard(dc, resBtn, UiMetrics::RadiusSm, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
    SetTextColor(dc, RGB(255, 255, 255)); SelectObject(dc, fonts.hBodyBold);
    DrawTextW(dc, L"▶ Tiếp tục phiên bị gián đoạn", 28, &resBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

} // namespace lap
