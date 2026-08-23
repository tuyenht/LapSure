#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <exception>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include "lap/inventory.h"
#include "lap/environment.h"
#include "lap/engines.h"
#include "lap/forensics.h"
#include "lap/stress.h"
#include "lap/functional.h"
#include "lap/functional_io.h"
#include "lap/acquisition.h"
#include "lap/port_power.h"
#include "lap/orchestrator.h"
#include "lap/chassis_profile.h"
#include "lap/runtime_validation.h"
#include "lap/port_selector.h"
#include "lap/profile.h"
#include "lap/report.h"
#include "lap/scoring.h"
#include "lap/process.h"
#include "lap/hardware.h"
#include "lap/ui_theme.h"
#include "lap/ui_state.h"
#include "lap/ui_components.h"

#pragma comment(lib,"Comctl32.lib")
using namespace lap;

namespace {
constexpr UINT WM_AUDIT_DONE = WM_APP + 1;
constexpr UINT WM_AUDIT_STATUS = WM_APP + 2;

AuditReport gReport;
std::wstring gDir, gReportPath;
HWND gMainHwnd = nullptr;
HWND gList = nullptr, gStatus = nullptr, gBtn = nullptr, gOpen = nullptr, gMode = nullptr;
HWND gFuncDisplay = nullptr, gFuncKeyboard = nullptr, gFuncTouch = nullptr, gFuncSpeaker = nullptr;
HWND gFuncUsb = nullptr, gFuncIo = nullptr, gPhysical = nullptr, gSeller = nullptr, gPortTest = nullptr, gNext = nullptr, gProgress = nullptr;

std::thread gWorker;
std::atomic_bool gCancel{false}, gRunning{false}, gAuditReady{false}, gCloseRequested{false};
CanonicalUiState gSessionLifecycleState{CanonicalUiState::Idle};
std::wstring gSelectedMode = L"Standard";
std::mutex gReportMutex, gLogsMutex;
std::vector<LiveLogEntry> gLiveLogs;
std::chrono::steady_clock::time_point gAuditStartTime;
int gAuditElapsedSec = 0;
int gAuditTotalItems = 14;
int gAuditCompletedItems = 0;

MainTab gCurrentTab = MainTab::Dashboard;
bool gDeviceGroupExpanded = true;
int gSidebarScrollOffset = 0;
int gTableScrollOffset = 0;
int gFocusIndex = 0; // 0: Sidebar, 1: Mode Pills, 2: Primary CTA, 3: Next CTA
UiFonts gFonts;

std::wstring AppDir() {
    wchar_t p[MAX_PATH]{};
    GetModuleFileNameW(nullptr, p, MAX_PATH);
    return std::filesystem::path(p).parent_path().wstring();
}

const wchar_t* UiState(State s) {
    switch (s) {
    case State::Pass: return L"ĐẠT";
    case State::Good: return L"TỐT";
    case State::Warning: return L"CẦN LƯU Ý";
    case State::Fail: return L"KHÔNG ĐẠT";
    case State::Changed: return L"CÓ THAY ĐỔI";
    case State::NotTested: return L"CHƯA KIỂM TRA";
    case State::Unsupported: return L"KHÔNG HỖ TRỢ";
    default: return L"THÔNG TIN";
    }
}

const wchar_t* UiDimension(Dimension d) {
    switch (d) {
    case Dimension::Identity: return L"Nhận diện";
    case Dimension::Factory: return L"Cấu hình gốc";
    case Dimension::Health: return L"Sức khỏe";
    case Dimension::Usage: return L"Mức sử dụng";
    case Dimension::Performance: return L"Hiệu năng";
    case Dimension::Stability: return L"Độ ổn định";
    case Dimension::Functional: return L"Chức năng";
    default: return L"Bằng chứng";
    }
}

std::wstring Reg(const wchar_t* name) {
    HKEY h{};
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &h) != ERROR_SUCCESS) return L"";
    wchar_t b[512]{}; DWORD sz = sizeof(b), t = 0; std::wstring s;
    if (RegQueryValueExW(h, name, nullptr, &t, (LPBYTE)b, &sz) == ERROR_SUCCESS) s = b;
    RegCloseKey(h);
    return s;
}

std::wstring ServiceTag(const Capabilities& caps, const std::atomic_bool* cancel) {
    auto tag = Reg(L"SystemSerialNumber");
    if (tag.empty() && caps.powershell) {
        auto p = RunProcessCapture(L"powershell.exe -NoProfile -NonInteractive -Command \"(Get-CimInstance Win32_BIOS|Select-Object -First 1).SerialNumber\"", 10000, cancel);
        auto lines = SplitLines(p.output);
        if (p.launched && !p.timedOut && !lines.empty()) tag = lines.front();
    }
    return tag;
}

int GetReadyEngineCount() {
    auto caps = DetectCapabilities(gDir);
    int c = 1; // Native Registry / SetupAPI base
    if (caps.powershell) c++;
    if (caps.wmi) c++;
    if (caps.admin) c++;
    if (caps.smartctl) c++;
    if (caps.nvme) c++;
    if (caps.edidRegistry) c++;
    if (caps.dxgi) c++;
    if (caps.setupApi) c++;
    if (caps.wlanApi) c++;
    if (caps.bluetoothApi) c++;
    if (caps.waveIn) c++;
    if (caps.mediaFoundation) c++;
    if (caps.winEventLog) c++;
    return std::clamp(c, 1, 14);
}

void AddLiveLog(const std::wstring& msg, const std::wstring& src = L"WMI", int state = 0) {
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    struct tm tmNow; localtime_s(&tmNow, &tt);
    wchar_t buf[32]; swprintf_s(buf, L"%02d:%02d:%02d", tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec);
    
    std::lock_guard<std::mutex> lk(gLogsMutex);
    gLiveLogs.push_back({ buf, msg, src, state });
    if (gMainHwnd) InvalidateRect(gMainHwnd, nullptr, FALSE);
}

void PostStatus(HWND h, const std::wstring& s) {
    auto* heap = new std::wstring(s);
    PostMessageW(h, WM_AUDIT_STATUS, 0, (LPARAM)heap);
    AddLiveLog(s, L"LapSure Engine", 0);
}

int RunInventoryOnly(const std::wstring& outputDir) {
  // inventory_only_begin
    std::atomic_bool cancel{false}; auto caps = DetectCapabilities(gDir); auto model = Reg(L"SystemProductName"), tag = ServiceTag(caps, &cancel);
    auto pl = LoadFactoryProfile(gDir + L"\\profiles", model, tag); FactoryProfile profile = pl.loaded ? pl.profile : FactoryProfile{};
    auto report = CollectInventory(profile, caps, gDir, &cancel); report.profileSource = pl.source; report.factoryExact = pl.exact; report.genericMode = !pl.exact;
    CollectNvidia(report, profile, caps, gDir, &cancel);
    CollectWindowsStorageReliability(report, caps, &cancel);
    CollectSmartctl(report, profile, caps, gDir, &cancel);
    CollectPlatformForensics(report, profile, caps, gDir, &cancel);
    CollectFunctionalPresence(report, caps, &cancel);
    CollectPortPowerBaseline(report);
    CollectVolumeIntegrityAudit(report);
    CollectBatteryDischargeAudit(report, caps, &cancel);
    report.hardware.stress.chassisProfile = LoadChassisProfile(gDir,report.model);
    RunRuntimeValidation(report,caps,gDir);
    report.findings.push_back({L"Validation", L"Inventory-only preflight", L"COMPLETED", L"No stress stages executed", State::Warning, Severity::Info, L"Explicit --inventory-only mode; verdict must remain incomplete.", Dimension::Health});
    report.hardware.stress.decision = BuildAuditDecision(report);
    BuildOrchestrator(report, false, false);
    auto out = outputDir.empty() ? ResolveReportDirectory(gDir, caps.winPE) : std::filesystem::absolute(outputDir).wstring();
    auto html = SaveHtmlReport(report, out), json = SaveJsonReport(report, out);
  // inventory_only_end
    return html.empty() || json.empty() ? 2 : 0;
}

void Fill() {
    std::lock_guard<std::mutex> lk(gReportMutex);
    if (!gList) return;
    ListView_DeleteAllItems(gList);
    int i = 0;
    for (auto& x : gReport.findings) {
        LVITEMW it{}; it.mask = LVIF_TEXT; it.iItem = i; it.pszText = (LPWSTR)UiDimension(x.dimension);
        ListView_InsertItem(gList, &it);
        ListView_SetItemText(gList, i, 1, (LPWSTR)x.group.c_str());
        ListView_SetItemText(gList, i, 2, (LPWSTR)x.name.c_str());
        ListView_SetItemText(gList, i, 3, (LPWSTR)x.value.c_str());
        ListView_SetItemText(gList, i, 4, (LPWSTR)UiState(x.state));
        ++i;
    }
}

void SetFunctionalButtonsEnabled(BOOL enabled) {
    if (gFuncDisplay) EnableWindow(gFuncDisplay, enabled);
    if (gFuncKeyboard) EnableWindow(gFuncKeyboard, enabled);
    if (gFuncTouch) EnableWindow(gFuncTouch, enabled);
    if (gFuncSpeaker) EnableWindow(gFuncSpeaker, enabled);
    if (gFuncUsb) EnableWindow(gFuncUsb, enabled);
    if (gFuncIo) EnableWindow(gFuncIo, enabled);
    if (gPhysical) EnableWindow(gPhysical, enabled);
    if (gSeller) EnableWindow(gSeller, enabled);
    if (gPortTest) EnableWindow(gPortTest, enabled);
}

void RebuildDecisionAndReports() {
    std::lock_guard<std::mutex> lk(gReportMutex);
    gReport.hardware.stress.decision = BuildAuditDecision(gReport);
    BuildOrchestrator(gReport, gRunning.load(), gAuditReady.load());
    auto caps = DetectCapabilities(gDir);
    auto out = ResolveReportDirectory(gDir, caps.winPE);
    gReportPath = SaveHtmlReport(gReport, out);
    SaveJsonReport(gReport, out);
}

void UpsertFunctional(const FunctionalItemResult& x) {
    std::lock_guard<std::mutex> lk(gReportMutex);
    auto& items = gReport.hardware.stress.functional.items;
    for (auto& v : items) if (v.id == x.id) { v = x; RecalculateFunctionalSummary(gReport.hardware.stress.functional); return; }
    items.push_back(x);
    RecalculateFunctionalSummary(gReport.hardware.stress.functional);
}

bool CanRunManualTest(HWND h) {
    if(gRunning) {
        MessageBoxW(h, L"LapSure đang kiểm tra tự động. Vui lòng chờ hoàn tất trước khi kiểm tra tương tác.", L"LapSure", MB_OK | MB_ICONINFORMATION);
        return false;
    }
    if (!gAuditReady) {
        MessageBoxW(h, L"Hãy bấm “BẮT ĐẦU KIỂM TRA” trước. Kết quả tương tác sẽ được lưu chung vào báo cáo hiện tại.", L"LapSure", MB_OK | MB_ICONINFORMATION);
        return false;
    }
    return true;
}

void CommitManualResult(const FunctionalItemResult& x) {
    UpsertFunctional(x); RebuildDecisionAndReports(); Fill();
    if (gMainHwnd) InvalidateRect(gMainHwnd, nullptr, FALSE);
}

void CommitManualResults(const std::vector<FunctionalItemResult>& xs) {
    for (auto& x : xs) UpsertFunctional(x);
    RebuildDecisionAndReports(); Fill();
    if (gMainHwnd) InvalidateRect(gMainHwnd, nullptr, FALSE);
}

void CommitSellerClaim(const SellerClaim& claim) {
    {
        std::lock_guard<std::mutex> lk(gReportMutex);
        gReport.sellerClaim = claim;
        ApplySellerClaimComparison(gReport);
    }
    RebuildDecisionAndReports(); Fill();
    if (gMainHwnd) InvalidateRect(gMainHwnd, nullptr, FALSE);
}

void UpsertPortResult(const PortProbeResult& x) {
    auto& ports = gReport.hardware.stress.portPower.ports;
    for (auto& port : ports) if (port.portLabel == x.portLabel) { port = x; return; }
    ports.push_back(x);
}

void CommitPortResultGuided(const PortProbeResult& x) {
    {
        std::lock_guard<std::mutex> lk(gReportMutex);
        UpsertPortResult(x);
        ApplyPortResultToChassisProfile(gReport.hardware.stress.chassisProfile, x);
        RecalculatePortPowerSummary(gReport.hardware.stress.portPower);
    }
    RebuildDecisionAndReports(); Fill();
    if (gMainHwnd) InvalidateRect(gMainHwnd, nullptr, FALSE);
}

void CommitPortResult(const PortProbeResult& x) {
    {
        std::lock_guard<std::mutex> lk(gReportMutex);
        UpsertPortResult(x);
        RecalculatePortPowerSummary(gReport.hardware.stress.portPower);
    }
    RebuildDecisionAndReports(); Fill();
    if (gMainHwnd) InvalidateRect(gMainHwnd, nullptr, FALSE);
}

void AuditWorkerCore(HWND h) {
    gCancel = false; gAuditCompletedItems = 0; gSessionLifecycleState = CanonicalUiState::Running;
    PostStatus(h, L"Đã nhận diện hệ điều hành và cấu hình BIOS"); gAuditCompletedItems = 1;
    auto caps = DetectCapabilities(gDir); auto model = Reg(L"SystemProductName"), tag = ServiceTag(caps, &gCancel);
    auto pl = LoadFactoryProfile(gDir + L"\\profiles", model, tag); FactoryProfile profile = pl.loaded ? pl.profile : FactoryProfile{};
    
    auto report = CollectInventory(profile, caps, gDir, &gCancel);
    report.profileSource = pl.source; report.factoryExact = pl.exact; report.genericMode = (pl.loaded && !pl.exact);
    gAuditCompletedItems = 2;
    
    if (!gCancel) {
        PostStatus(h, L"Đọc thông tin CPU, RAM và bo mạch chủ...");
        gAuditCompletedItems = 3;
    }
    if (!gCancel) {
        PostStatus(h, L"Đọc thông tin GPU và Driver đồ họa...");
        CollectNvidia(report, profile, caps, gDir, &gCancel);
        gAuditCompletedItems = 4;
    }
    if (!gCancel) {
        PostStatus(h, L"Đọc dữ liệu SMART & NVMe cho từng ổ đĩa...");
        CollectWindowsStorageReliability(report, caps, &gCancel);
        CollectSmartctl(report, profile, caps, gDir, &gCancel);
        gAuditCompletedItems = 6;
    }
    if (!gCancel) {
        PostStatus(h, L"Đọc màn hình EDID, TPM, Pin và Sự kiện hệ thống...");
        CollectPlatformForensics(report, profile, caps, gDir, &gCancel);
        CollectFunctionalPresence(report, caps, &gCancel);
        CollectPortPowerBaseline(report);
        CollectVolumeIntegrityAudit(report);
        CollectBatteryDischargeAudit(report, caps, &gCancel);
        report.hardware.stress.chassisProfile = LoadChassisProfile(gDir,report.model);
        gAuditCompletedItems = 9;
    }
    if (!gCancel) {
        PostStatus(h, L"Chạy bài kiểm tra độ ổn định Stress (" + gSelectedMode + L")...");
        RunStressSession(report, caps, gDir, MakeStressPlan(gSelectedMode), &gCancel);
        gAuditCompletedItems = 12;
    }
    if (!gCancel) {
        PostStatus(h, L"Chạy cổng kiểm định Runtime Validation Gate...");
        RunRuntimeValidation(report,caps,gDir);
        gAuditCompletedItems = 14;
    }
    if (!gCancel) {
        report.hardware.stress.decision = BuildAuditDecision(report);
        BuildOrchestrator(report, false, true);
    }
    std::wstring reportPath;
    if (!gCancel) {
        auto out = ResolveReportDirectory(gDir, caps.winPE);
        reportPath = SaveHtmlReport(report, out);
        SaveJsonReport(report, out);
    }
    {
        std::lock_guard<std::mutex> lk(gReportMutex);
        gReport = std::move(report);
        gReportPath = std::move(reportPath);
    }
    gAuditReady = !gCancel; gRunning = false;
    gSessionLifecycleState = gCancel ? CanonicalUiState::Cancelled : CanonicalUiState::Pass;
    PostMessageW(h, WM_AUDIT_DONE, gCancel ? 1 : 0, 0);
}

void AuditWorker(HWND h) {
    try { AuditWorkerCore(h); }
    catch (...) {
        gAuditReady = false; gRunning = false;
        gSessionLifecycleState = CanonicalUiState::Interrupted;
        PostStatus(h, L"Quá trình kiểm tra bị gián đoạn; chưa đủ dữ liệu để kết luận.");
        PostMessageW(h, WM_AUDIT_DONE, 1, 0);
    }
}

void StartAudit(HWND h) {
    if(gRunning) {
        gCancel = true;
        gSessionLifecycleState = CanonicalUiState::Cancelled;
        PostStatus(h, L"Đang dừng kiểm tra...");
        return;
    }
    if (gWorker.joinable()) gWorker.join();
    gRunning = true; gCancel = false; gAuditReady = false;
    gSessionLifecycleState = CanonicalUiState::Running;
    gAuditStartTime = std::chrono::steady_clock::now();
    gAuditCompletedItems = 0;
    PostStatus(h, L"BẮT ĐẦU KIỂM TRA TOÀN DIỆN LAPTOP...");
    gCurrentTab = MainTab::AutoAudit;
    gWorker = std::thread(AuditWorker, h);
}

void ShowAboutDialog(HWND h) {
    MessageBoxW(h,
        L"LapSure — Phần mềm Kiểm định & Pháp y Laptop Chuyên nghiệp\n"
        L"Phiên bản: v0.1.1-beta (Build Native C++20 Win32 x64)\n"
        L"Bản quyền © 2026 LapSure Core Team / TuyenHT\n"
        L"Trang chủ: https://github.com/tuyenht/LapSure\n\n"
        L"Các phân hệ chính:\n"
        L"• Nhận diện 100% phần cứng: CPU, RAM, GPU, NVMe SSD, Màn hình, Pin\n"
        L"• Pháp y Dell: Base36 Express Service Code & 18+ Chassis Profiles\n"
        L"• Pháp y ThinkPad & Cơ sở dữ liệu Silicon CPU Microbenchmark\n"
        L"• Quét mã lỗi ẩn Driver PnP Yellow Bang (Code 43, 10, 28)\n"
        L"• Quét lỗi hệ thống tệp FSCTL_IS_VOLUME_DIRTY & Đo công suất xả pin mW\n"
        L"• Bộ Wizard tương tác: Phím 68 nút, Trackpad 80 ô, Màn hình 6 màu, Loa Stereo\n"
        L"• Đối chiếu Cam kết Người bán & Kiểm tra Ngoại hình 6 điểm vật lý",
        L"Giới thiệu — LapSure v0.1.1-beta",
        MB_OK | MB_ICONINFORMATION);
}

// ----------------------------------------------------
// UI Renderers Composing C01–C12 Reusable Components
// ----------------------------------------------------

void RenderDashboard(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    // 1. C03 Page Header
    PageHeaderConfig hdr;
    hdr.title = L"Tổng quan thiết bị";
    hdr.subtitle = L"Tổng hợp trạng thái phần cứng và mức độ sẵn sàng kiểm định dựa trên bằng chứng kỹ thuật";
    if (gAuditReady) {
        hdr.sessionTag = FormatDecisionVi(rep.hardware.stress.decision.overall);
        if (rep.hardware.stress.decision.overall == L"BUY") hdr.sessionState = CanonicalUiState::Good;
        else if (rep.hardware.stress.decision.overall == L"BUY WITH NOTES") hdr.sessionState = CanonicalUiState::Warning;
        else if (rep.hardware.stress.decision.overall == L"REJECT") hdr.sessionState = CanonicalUiState::Fail;
        else hdr.sessionState = CanonicalUiState::Incomplete;
    } else {
        hdr.sessionTag = (gSessionLifecycleState == CanonicalUiState::Cancelled) ? L"Đã hủy" :
                         ((gSessionLifecycleState == CanonicalUiState::Interrupted) ? L"Bị gián đoạn" : L"Chưa bắt đầu");
        hdr.sessionState = gSessionLifecycleState;
    }
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    // 2. Mode selection pills & Start Audit Button
    int modeX = r.left + UiMetrics::Scale(24, dpi);
    int modeY = r.top + UiMetrics::Scale(70, dpi);
    SelectObject(dc, gFonts.hSmall);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, modeX, modeY + UiMetrics::Scale(6, dpi), L"Chế độ kiểm tra:", 16);
    modeX += UiMetrics::Scale(110, dpi);

    auto drawPill = [&](const wchar_t* label, bool active, int pillIdx) {
        int pw = UiMetrics::Scale(80, dpi);
        int ph = UiMetrics::Scale(28, dpi);
        RECT pr{ modeX, modeY, modeX + pw, modeY + ph };
        DrawRoundedCard(dc, pr, ph / 2, active ? UiColors::PrimaryBlue : UiColors::GrayPillBg, active ? UiColors::PrimaryBlue : UiColors::GrayPillBorder, 1);
        SetTextColor(dc, active ? RGB(255, 255, 255) : UiColors::TextMain);
        SelectObject(dc, active ? gFonts.hBodyBold : gFonts.hBody);
        DrawTextW(dc, label, -1, &pr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (gFocusIndex == 1 && active) DrawFocusRing(dc, pr, ph / 2);
        modeX += pw + UiMetrics::Scale(6, dpi);
    };
    drawPill(L"Nhanh", gSelectedMode == L"Quick", 0);
    drawPill(L"Tiêu chuẩn", gSelectedMode == L"Standard", 1);
    drawPill(L"Chuyên sâu", gSelectedMode == L"Deep", 2);

    // Primary CTA Button
    int btnW = UiMetrics::Scale(230, dpi);
    int btnH = UiMetrics::Scale(40, dpi);
    RECT btnStartRect{ r.right - btnW - UiMetrics::Scale(24, dpi), modeY - UiMetrics::Scale(2, dpi), r.right - UiMetrics::Scale(24, dpi), modeY - UiMetrics::Scale(2, dpi) + btnH };
    COLORREF btnClr = gRunning ? UiColors::FailRed : UiColors::PrimaryBlue;
    DrawRoundedCard(dc, btnStartRect, btnH / 2, btnClr, btnClr, 1);
    SetTextColor(dc, RGB(255, 255, 255));
    SelectObject(dc, gFonts.hBodyBold);
    std::wstring btnText = gRunning ? L"DỪNG KIỂM TRA" : L"BẮT ĐẦU KIỂM TRA";
    DrawTextW(dc, btnText.c_str(), (int)btnText.size(), &btnStartRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (gFocusIndex == 2) DrawFocusRing(dc, btnStartRect, btnH / 2);

    // 3. C05 Metric KPI Cards (4 Cards)
    int kpiY = modeY + UiMetrics::Scale(46, dpi);
    int rightPanelW = UiMetrics::Scale(250, dpi);
    int mainContentW = r.right - r.left - UiMetrics::Scale(48, dpi) - rightPanelW;
    int kpiW = (mainContentW - UiMetrics::Scale(36, dpi)) / 4;
    int kpiH = UiMetrics::Scale(88, dpi);

    // KPI 1: Overall Verdict
    MetricCardConfig mc1;
    mc1.label = L"Trạng thái tổng thể";
    if (gAuditReady) {
        mc1.value = FormatDecisionVi(rep.hardware.stress.decision.overall);
        if (rep.hardware.stress.decision.overall == L"BUY") mc1.state = CanonicalUiState::Good;
        else if (rep.hardware.stress.decision.overall == L"BUY WITH NOTES") mc1.state = CanonicalUiState::Warning;
        else if (rep.hardware.stress.decision.overall == L"REJECT") mc1.state = CanonicalUiState::Fail;
        else mc1.state = CanonicalUiState::Incomplete;
        mc1.note = L"Kết luận kỹ thuật";
        mc1.hasBadge = true;
    } else {
        mc1.value = L"—";
        mc1.note = (gSessionLifecycleState == CanonicalUiState::Cancelled) ? L"Phiên kiểm tra đã hủy" :
                   ((gSessionLifecycleState == CanonicalUiState::Interrupted) ? L"Phiên kiểm tra bị gián đoạn" : L"Chưa kiểm tra");
        mc1.state = gSessionLifecycleState;
    }
    RECT kpi1Rect{ r.left + UiMetrics::Scale(24, dpi), kpiY, r.left + UiMetrics::Scale(24, dpi) + kpiW, kpiY + kpiH };
    DrawMetricCard(dc, kpi1Rect, mc1, gFonts, dpi);

    // KPI 2: Tests Completed
    MetricCardConfig mc2;
    mc2.label = L"Đã kiểm tra";
    mc2.value = std::to_wstring(gAuditCompletedItems) + L" / " + std::to_wstring(gAuditTotalItems);
    mc2.note = L"Hạng mục tự động";
    mc2.state = gAuditReady ? CanonicalUiState::Pass : (gRunning ? CanonicalUiState::Running : CanonicalUiState::Idle);
    RECT kpi2Rect{ kpi1Rect.right + UiMetrics::Scale(12, dpi), kpiY, kpi1Rect.right + UiMetrics::Scale(12, dpi) + kpiW, kpiY + kpiH };
    DrawMetricCard(dc, kpi2Rect, mc2, gFonts, dpi);

    // KPI 3: Warnings Count
    int warnCount = 0;
    int critCount = 0;
    if (gAuditReady) {
        for (const auto& f : rep.findings) {
            if (f.severity == Severity::Critical || f.state == State::Fail) critCount++;
            else if (f.severity == Severity::Warning || f.state == State::Warning) warnCount++;
        }
    }
    MetricCardConfig mc3;
    mc3.label = L"Cảnh báo";
    mc3.value = gAuditReady ? std::to_wstring(warnCount) : L"—";
    mc3.note = gAuditReady ? L"Hạng mục cần lưu ý" : L"Chưa có dữ liệu";
    mc3.state = (warnCount > 0) ? CanonicalUiState::Warning : (gAuditReady ? CanonicalUiState::Good : CanonicalUiState::Idle);
    RECT kpi3Rect{ kpi2Rect.right + UiMetrics::Scale(12, dpi), kpiY, kpi2Rect.right + UiMetrics::Scale(12, dpi) + kpiW, kpiY + kpiH };
    DrawMetricCard(dc, kpi3Rect, mc3, gFonts, dpi);

    // KPI 4: Critical Failures Count
    MetricCardConfig mc4;
    mc4.label = L"Lỗi nghiêm trọng";
    mc4.value = gAuditReady ? std::to_wstring(critCount) : L"—";
    mc4.note = gAuditReady ? L"Hạng mục không đạt" : L"Chưa có dữ liệu";
    mc4.state = (critCount > 0) ? CanonicalUiState::Fail : (gAuditReady ? CanonicalUiState::Good : CanonicalUiState::Idle);
    RECT kpi4Rect{ kpi3Rect.right + UiMetrics::Scale(12, dpi), kpiY, kpi3Rect.right + UiMetrics::Scale(12, dpi) + kpiW, kpiY + kpiH };
    DrawMetricCard(dc, kpi4Rect, mc4, gFonts, dpi);

    // 4. Right Rail Cards: Evidence Coverage (C06) & Factory Profile (C05)
    int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT covCardRect{ rightX, kpiY, r.right - UiMetrics::Scale(24, dpi), kpiY + UiMetrics::Scale(150, dpi) };
    DrawRoundedCard(dc, covCardRect, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    
    int covInnerY = covCardRect.top + UiMetrics::Scale(14, dpi);
    ProgressCoverageConfig pcc;
    pcc.label = L"Độ bao phủ bằng chứng";
    pcc.isEvidenceCoverage = true;
    pcc.total = gAuditTotalItems;
    pcc.completed = gAuditCompletedItems;
    pcc.barColor = UiColors::PrimaryBlue;
    RECT pccRect{ covCardRect.left + UiMetrics::Scale(14, dpi), covInnerY, covCardRect.right - UiMetrics::Scale(14, dpi), covInnerY + UiMetrics::Scale(45, dpi) };
    DrawProgressCoverage(dc, pccRect, pcc, gFonts, dpi);

    SelectObject(dc, gFonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    std::wstring covExpl = gAuditReady ? L"Đã hoàn thành kiểm tra tự động. Cần kiểm tra tương tác trước khi mua." : L"Yêu cầu hoàn thành kiểm tra tự động trước khi kết luận.";
    RECT explRect{ covCardRect.left + UiMetrics::Scale(14, dpi), covCardRect.top + UiMetrics::Scale(70, dpi), covCardRect.right - UiMetrics::Scale(14, dpi), covCardRect.bottom - UiMetrics::Scale(10, dpi) };
    DrawTextW(dc, covExpl.c_str(), (int)covExpl.size(), &explRect, DT_LEFT | DT_WORDBREAK);

    // Factory Profile Card
    RECT factoryCard{ rightX, covCardRect.bottom + UiMetrics::Scale(12, dpi), r.right - UiMetrics::Scale(24, dpi), covCardRect.bottom + UiMetrics::Scale(155, dpi) };
    DrawRoundedCard(dc, factoryCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, factoryCard.left + UiMetrics::Scale(14, dpi), factoryCard.top + UiMetrics::Scale(10, dpi), L"Hồ sơ nhà máy (Factory Profile)", 31);
    
    SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    std::wstring modelDisplay = rep.model.empty() ? (Reg(L"SystemProductName").empty() ? L"—" : Reg(L"SystemProductName")) : rep.model;
    TextOutW(dc, factoryCard.left + UiMetrics::Scale(14, dpi), factoryCard.top + UiMetrics::Scale(30, dpi), modelDisplay.c_str(), (int)modelDisplay.size());
    
    CanonicalUiState profState = rep.factoryExact ? CanonicalUiState::Pass : (rep.genericMode ? CanonicalUiState::Changed : CanonicalUiState::NotTested);
    DrawStatusBadge(dc, factoryCard.right - UiMetrics::Scale(100, dpi), factoryCard.top + UiMetrics::Scale(28, dpi), UiMetrics::Scale(85, dpi), UiMetrics::Scale(22, dpi), profState, gFonts);

    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    std::wstring profStatusText = rep.factoryExact ? L"Khớp hoàn toàn cấu hình gốc nhà máy" : (rep.genericMode ? L"Hồ sơ đối chiếu suy đoán chung" : L"Chưa nạp hồ sơ đối chiếu");
    TextOutW(dc, factoryCard.left + UiMetrics::Scale(14, dpi), factoryCard.top + UiMetrics::Scale(56, dpi), profStatusText.c_str(), (int)profStatusText.size());

    RECT btnProfileDetail{ factoryCard.left + UiMetrics::Scale(14, dpi), factoryCard.bottom - UiMetrics::Scale(36, dpi), factoryCard.right - UiMetrics::Scale(14, dpi), factoryCard.bottom - UiMetrics::Scale(10, dpi) };
    DrawRoundedCard(dc, btnProfileDetail, UiMetrics::RadiusSm, UiColors::GrayPillBg, UiColors::GrayPillBorder, 1);
    SetTextColor(dc, UiColors::PrimaryBlue);
    SelectObject(dc, gFonts.hSmall);
    DrawTextW(dc, L"Xem chi tiết đối chiếu", -1, &btnProfileDetail, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 5. Domain Grid (12 Cards - Bound to real domain evidence)
    int gridY = kpiY + kpiH + UiMetrics::Scale(14, dpi);
    int gridCols = 4;
    int gridW = (mainContentW - UiMetrics::Scale((gridCols - 1) * 8, dpi)) / gridCols;
    int gridH = UiMetrics::Scale(68, dpi);

    // Dynamic domain evaluation (NO presence == functionality)
    CanonicalUiState stSystem = (!rep.model.empty()) ? CanonicalUiState::Pass : CanonicalUiState::NotTested;
    CanonicalUiState stRam = (rep.hardware.installedRamBytes > 0) ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    CanonicalUiState stStorage = (!rep.hardware.storage.empty()) ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    CanonicalUiState stBattery = (rep.hardware.battery.healthPercent > 0) ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    CanonicalUiState stGpu = (!rep.hardware.gpus.empty()) ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    CanonicalUiState stDisplay = (!rep.hardware.displays.empty()) ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    CanonicalUiState stKeyboard = (rep.hardware.stress.functional.passed > 0) ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    CanonicalUiState stAudio = (rep.hardware.stress.functional.passed > 0) ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    CanonicalUiState stNetwork = (rep.hardware.stress.functional.passed > 0) ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    CanonicalUiState stPorts = (rep.hardware.stress.portPower.overall == L"PASS") ? CanonicalUiState::Good : ((rep.hardware.stress.portPower.overall == L"FAIL") ? CanonicalUiState::Fail : CanonicalUiState::NotTested);
    CanonicalUiState stStress = (rep.hardware.stress.stabilityState == L"PASS") ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    CanonicalUiState stEvents = gAuditReady ? ((rep.hardware.forensics.criticalEventsCount > 0) ? CanonicalUiState::Warning : CanonicalUiState::Good) : CanonicalUiState::NotTested;

    struct DomainDef { const wchar_t* icon; const wchar_t* name; const wchar_t* desc; CanonicalUiState state; };
    std::vector<DomainDef> domains = {
        { L"🖥️", L"Nhận diện hệ thống", L"CPU, Mainboard, BIOS, OS", stSystem },
        { L"🧠", L"Bộ nhớ (RAM)", L"Dung lượng, DIMM, Tải", stRam },
        { L"💾", L"Lưu trữ", L"NVMe/SSD, SMART, Độ bền", stStorage },
        { L"🔋", L"Pin & Năng lượng", L"Dung lượng, Chu kỳ, Sạc", stBattery },
        { L"🎮", L"Đồ họa (GPU)", L"iGPU, dGPU, VRAM, Driver", stGpu },
        { L"🖥️", L"Hiển thị (Màn hình)", L"EDID, Độ phân giải, Màu", stDisplay },
        { L"⌨️", L"Bàn phím & Touchpad", L"Phím, Chạm, Cử chỉ", stKeyboard },
        { L"🔊", L"Âm thanh & Camera", L"Loa, Mic, Camera", stAudio },
        { L"📶", L"Mạng & Kết nối", L"Wi-Fi, Bluetooth, LAN", stNetwork },
        { L"🔌", L"Cổng & Nguồn", L"USB, HDMI, DP, LAN, Audio", stPorts },
        { L"📈", L"Stress & Ổn định", L"CPU, RAM, GPU, Nhiệt độ", stStress },
        { L"📜", L"Nhật ký & Sự kiện", L"WHEA, Ổ đĩa, Hệ thống", stEvents }
    };

    for (size_t i = 0; i < domains.size(); ++i) {
        int row = (int)i / gridCols;
        int col = (int)i % gridCols;
        int gx = r.left + UiMetrics::Scale(24, dpi) + col * (gridW + UiMetrics::Scale(8, dpi));
        int gy = gridY + row * (gridH + UiMetrics::Scale(8, dpi));
        RECT gr{ gx, gy, gx + gridW, gy + gridH };
        DrawRoundedCard(dc, gr, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

        SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
        std::wstring nameStr = std::wstring(domains[i].icon) + L" " + domains[i].name;
        TextOutW(dc, gr.left + UiMetrics::Scale(8, dpi), gr.top + UiMetrics::Scale(8, dpi), nameStr.c_str(), (int)nameStr.size());

        SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, gr.left + UiMetrics::Scale(8, dpi), gr.top + UiMetrics::Scale(26, dpi), domains[i].desc, (int)wcslen(domains[i].desc));

        DrawStatusBadge(dc, gr.left + UiMetrics::Scale(8, dpi), gr.top + UiMetrics::Scale(44, dpi), UiMetrics::Scale(85, dpi), UiMetrics::Scale(18, dpi), domains[i].state, gFonts);
    }

    // 6. Quick Hardware Spec Overview Card (Bottom)
    int botY = gridY + 3 * (gridH + UiMetrics::Scale(8, dpi)) + UiMetrics::Scale(8, dpi);
    RECT infoCard{ r.left + UiMetrics::Scale(24, dpi), botY, rightX - UiMetrics::Scale(12, dpi), botY + UiMetrics::Scale(110, dpi) };
    DrawRoundedCard(dc, infoCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, infoCard.left + UiMetrics::Scale(14, dpi), infoCard.top + UiMetrics::Scale(10, dpi), L"Thông tin nhanh phần cứng", 25);

    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, infoCard.left + UiMetrics::Scale(14, dpi), infoCard.top + UiMetrics::Scale(32, dpi), L"Máy:", 4);
    TextOutW(dc, infoCard.left + UiMetrics::Scale(14, dpi), infoCard.top + UiMetrics::Scale(50, dpi), L"Service Tag:", 12);
    TextOutW(dc, infoCard.left + UiMetrics::Scale(14, dpi), infoCard.top + UiMetrics::Scale(68, dpi), L"BIOS:", 5);
    TextOutW(dc, infoCard.left + UiMetrics::Scale(14, dpi), infoCard.top + UiMetrics::Scale(86, dpi), L"Hệ điều hành:", 13);

    SetTextColor(dc, UiColors::TextMain);
    std::wstring sysModel = rep.model.empty() ? (Reg(L"SystemProductName").empty() ? L"—" : Reg(L"SystemProductName")) : rep.model;
    std::wstring sysTag = rep.serviceTag.empty() ? (Reg(L"SystemSerialNumber").empty() ? L"—" : Reg(L"SystemSerialNumber")) : rep.serviceTag;
    std::wstring sysBios = Reg(L"BIOSVersion").empty() ? L"—" : Reg(L"BIOSVersion");
    TextOutW(dc, infoCard.left + UiMetrics::Scale(95, dpi), infoCard.top + UiMetrics::Scale(32, dpi), sysModel.c_str(), (int)sysModel.size());
    TextOutW(dc, infoCard.left + UiMetrics::Scale(95, dpi), infoCard.top + UiMetrics::Scale(50, dpi), sysTag.c_str(), (int)sysTag.size());
    TextOutW(dc, infoCard.left + UiMetrics::Scale(95, dpi), infoCard.top + UiMetrics::Scale(68, dpi), sysBios.c_str(), (int)sysBios.size());
    TextOutW(dc, infoCard.left + UiMetrics::Scale(95, dpi), infoCard.top + UiMetrics::Scale(86, dpi), L"Windows 64-bit (Native Win32)", 30);

    int chipX = infoCard.left + UiMetrics::Scale(340, dpi);
    auto drawChip = [&](const wchar_t* title, const std::wstring& val, int cx, int cy) {
        RECT cr{ cx, cy, cx + UiMetrics::Scale(180, dpi), cy + UiMetrics::Scale(42, dpi) };
        DrawRoundedCard(dc, cr, UiMetrics::RadiusSm, UiColors::GrayPillBg, UiColors::GrayPillBorder, 1);
        SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, cr.left + UiMetrics::Scale(8, dpi), cr.top + UiMetrics::Scale(4, dpi), title, (int)wcslen(title));
        SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMain);
        std::wstring v = val.empty() ? L"—" : val;
        TextOutW(dc, cr.left + UiMetrics::Scale(8, dpi), cr.top + UiMetrics::Scale(20, dpi), v.c_str(), (int)v.size());
    };
    drawChip(L"CPU", rep.hardware.cpuName, chipX, infoCard.top + UiMetrics::Scale(14, dpi));
    std::wstring ramStr = (rep.hardware.installedRamBytes > 0) ? (std::to_wstring(rep.hardware.installedRamBytes / (1024 * 1024 * 1024)) + L" GB") : L"—";
    drawChip(L"RAM", ramStr, chipX + UiMetrics::Scale(190, dpi), infoCard.top + UiMetrics::Scale(14, dpi));
    std::wstring diskStr = rep.hardware.storage.empty() ? L"—" : rep.hardware.storage.front().model;
    drawChip(L"Ổ đĩa chính", diskStr, chipX, infoCard.top + UiMetrics::Scale(60, dpi));
    std::wstring gpuStr = rep.hardware.gpus.empty() ? L"—" : rep.hardware.gpus.front().name;
    drawChip(L"Đồ họa", gpuStr, chipX + UiMetrics::Scale(190, dpi), infoCard.top + UiMetrics::Scale(60, dpi));
}

void RenderAutoAudit(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Kiểm tra Tự động";
    hdr.subtitle = L"Quét cấu hình, sức khỏe và độ ổn định hệ thống một cách tự động và toàn diện.";
    hdr.sessionState = gRunning ? CanonicalUiState::Running : (gAuditReady ? CanonicalUiState::Pass : gSessionLifecycleState);
    hdr.sessionTag = gRunning ? L"Đang chạy" : (gAuditReady ? L"Hoàn tất" : ((gSessionLifecycleState == CanonicalUiState::Cancelled) ? L"Đã hủy" : L"Chưa bắt đầu"));
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    // Progress Bar
    int prW = UiMetrics::Scale(260, dpi);
    RECT prBar{ r.left + UiMetrics::Scale(360, dpi), r.top + UiMetrics::Scale(40, dpi), r.left + UiMetrics::Scale(360, dpi) + prW, r.top + UiMetrics::Scale(50, dpi) };
    int pct = gAuditTotalItems > 0 ? (gAuditCompletedItems * 100 / gAuditTotalItems) : 0;
    DrawModernProgressBar(dc, prBar, pct, UiColors::PrimaryBlue, RGB(226, 232, 240));

    SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    std::wstring progText = L"Đang chạy " + std::to_wstring(gAuditCompletedItems) + L" / " + std::to_wstring(gAuditTotalItems) + L" hạng mục";
    TextOutW(dc, r.left + UiMetrics::Scale(360, dpi), r.top + UiMetrics::Scale(20, dpi), progText.c_str(), (int)progText.size());

    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::PrimaryBlue);
    std::wstring pctStr = std::to_wstring(pct) + L"%";
    TextOutW(dc, prBar.right + UiMetrics::Scale(10, dpi), r.top + UiMetrics::Scale(36, dpi), pctStr.c_str(), (int)pctStr.size());

    // Cancel Button
    RECT btnCancel{ r.right - UiMetrics::Scale(160, dpi), r.top + UiMetrics::Scale(24, dpi), r.right - UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(56, dpi) };
    DrawRoundedCard(dc, btnCancel, UiMetrics::RadiusPill, RGB(254, 242, 242), RGB(254, 202, 202), 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::FailRed);
    DrawTextW(dc, L"🚫 Hủy kiểm tra", -1, &btnCancel, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    int leftW = r.right - r.left - UiMetrics::Scale(48, dpi) - UiMetrics::Scale(250, dpi);
    int curY = r.top + UiMetrics::Scale(80, dpi);

    // Audit Items List (Bound to real stage progression)
    struct AutoItem { int num; const wchar_t* name; const wchar_t* sub; CanonicalUiState state; const wchar_t* src; };
    std::vector<AutoItem> items = {
        { 1, L"Nhận diện hệ thống", L"CPU, Mainboard, BIOS, OS, Thiết bị", (gAuditCompletedItems >= 1) ? CanonicalUiState::Pass : (gRunning ? CanonicalUiState::Running : CanonicalUiState::Idle), L"WMI, SMBIOS, SetupAPI" },
        { 2, L"CPU & Microbench", L"Identity, vi điểm chuẩn, telemetry", (gAuditCompletedItems >= 3) ? CanonicalUiState::Pass : (gAuditCompletedItems >= 1 ? CanonicalUiState::Running : CanonicalUiState::Idle), L"WMI, Telemetry" },
        { 3, L"Bộ nhớ (RAM)", L"Dung lượng, DIMM, Kiểm tra lỗi", (gAuditCompletedItems >= 3) ? CanonicalUiState::Pass : (gAuditCompletedItems >= 2 ? CanonicalUiState::Running : CanonicalUiState::Idle), L"WMI, CIM" },
        { 4, L"Lưu trữ", L"NVMe/SSD, S.M.A.R.T., Độ tin cậy", (gAuditCompletedItems >= 6) ? CanonicalUiState::Good : (gAuditCompletedItems >= 3 ? CanonicalUiState::Running : CanonicalUiState::Idle), L"StorageReliability, SMART" },
        { 5, L"Đồ họa (GPU)", L"iGPU, dGPU, VRAM, Driver", (gAuditCompletedItems >= 4) ? CanonicalUiState::Pass : (gAuditCompletedItems >= 3 ? CanonicalUiState::Running : CanonicalUiState::Idle), L"WMI, DXGI" },
        { 6, L"Pin & Nguồn", L"Dung lượng, Sạc, Công suất xả", (gAuditCompletedItems >= 9) ? CanonicalUiState::Pass : (gAuditCompletedItems >= 6 ? CanonicalUiState::Running : CanonicalUiState::Idle), L"CIM, BatteryDischarge" },
        { 7, L"Mạng & Kết nối", L"Wi-Fi, Bluetooth, LAN", (gAuditCompletedItems >= 9) ? CanonicalUiState::Pass : CanonicalUiState::Idle, L"WlanApi, Bluetooth" },
        { 8, L"Nhật ký & Sự kiện", L"WHEA, Ổ đĩa, Hệ thống", (gAuditCompletedItems >= 9) ? CanonicalUiState::Pass : CanonicalUiState::Idle, L"EventLog, Forensics" },
        { 9, L"Stress & Ổn định", L"CPU, RAM, GPU, Nhiệt độ", (gAuditCompletedItems >= 12) ? CanonicalUiState::Pass : CanonicalUiState::Idle, L"Stress Engine" }
    };

    for (const auto& it : items) {
        EvidenceRowConfig erc;
        erc.parameter = std::to_wstring(it.num) + L". " + it.name;
        erc.actualValue = it.sub;
        erc.providerSource = it.src;
        erc.state = it.state;

        RECT ar{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW, curY + UiMetrics::Scale(42, dpi) };
        DrawEvidenceRow(dc, ar, erc, gFonts, dpi);
        curY += UiMetrics::Scale(46, dpi);
    }

    // Live Logs Card
    RECT logCard{ r.left + UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(4, dpi), r.left + UiMetrics::Scale(24, dpi) + leftW, r.bottom - UiMetrics::Scale(20, dpi) };
    DrawRoundedCard(dc, logCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    
    SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, logCard.left + UiMetrics::Scale(14, dpi), logCard.top + UiMetrics::Scale(10, dpi), L"Nhật ký & Bằng chứng trực tiếp", 30);

    int logY = logCard.top + UiMetrics::Scale(34, dpi);
    std::lock_guard<std::mutex> lk(gLogsMutex);
    int count = 0;
    for (int i = (int)gLiveLogs.size() - 1; i >= 0 && count < 6; --i, ++count) {
        const auto& entry = gLiveLogs[i];
        SelectObject(dc, gFonts.hSmall);
        SetTextColor(dc, UiColors::SuccessGreen);
        TextOutW(dc, logCard.left + UiMetrics::Scale(14, dpi), logY, L"●", 1);
        SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, logCard.left + UiMetrics::Scale(28, dpi), logY, entry.time.c_str(), (int)entry.time.size());
        SetTextColor(dc, UiColors::TextMain);
        TextOutW(dc, logCard.left + UiMetrics::Scale(95, dpi), logY, entry.message.c_str(), (int)entry.message.size());
        logY += UiMetrics::Scale(18, dpi);
    }

    // Right Rail: Next Action Panel (C10)
    int rightX = r.right - UiMetrics::Scale(250, dpi);
    RECT nextCardRect{ rightX, r.top + UiMetrics::Scale(80, dpi), r.right - UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(240, dpi) };
    NextActionConfig nac;
    nac.actionTitle = gRunning ? L"Thu thập bằng chứng tự động" : (gAuditReady ? L"Chuyển sang kiểm tra chức năng" : L"Bắt đầu kiểm tra hệ thống");
    nac.reasonText = gRunning ? L"Đang quét thông số phần cứng..." : (gAuditReady ? L"Đã đủ dữ liệu tự động." : L"Yêu cầu phiên kiểm tra tự động trước.");
    nac.remainingTasks = { L"Kiểm tra bàn phím, touchpad", L"Kiểm tra âm thanh, camera", L"Kiểm tra cổng cắm & sạc" };
    nac.buttonText = gAuditReady ? L"TIẾP TỤC BƯỚC KẾ" : L"BẮT ĐẦU KIỂM TRA";
    nac.isButtonEnabled = true;
    DrawNextActionPanel(dc, nextCardRect, nac, gFonts, dpi);
    if (gFocusIndex == 3) {
        int btnH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
        RECT br{ nextCardRect.left + UiMetrics::Scale(14, dpi), nextCardRect.bottom - btnH - UiMetrics::Scale(12, dpi), nextCardRect.right - UiMetrics::Scale(14, dpi), nextCardRect.bottom - UiMetrics::Scale(12, dpi) };
        DrawFocusRing(dc, br, UiMetrics::RadiusPill);
    }
}

void RenderFunctional(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Kiểm tra Chức năng";
    hdr.subtitle = L"Kiểm tra thực tế các thiết bị cần thao tác tương tác (Bàn phím, Màn hình, Loa, Camera, Cổng kết nối)";
    hdr.sessionState = gAuditReady ? CanonicalUiState::Pass : CanonicalUiState::Idle;
    hdr.sessionTag = gAuditReady ? L"Sẵn sàng tương tác" : L"Cần chạy tự động";
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    // C07 Guided Stepper
    std::vector<StepperStep> steps = {
        { 1, L"Màn hình", L"Điểm chết, màu", (rep.hardware.stress.functional.passed > 0) ? CanonicalUiState::Pass : CanonicalUiState::NotTested, false },
        { 2, L"Bàn phím & Touch", L"Phím, touchpad", gAuditReady ? CanonicalUiState::Ready : CanonicalUiState::Locked, true },
        { 3, L"Loa trái / phải", L"Âm lượng, pha", CanonicalUiState::NotTested, false },
        { 4, L"Camera & Mic", L"Hình ảnh, âm thanh", CanonicalUiState::NotTested, false }
    };
    RECT stepperRect{ r.left + UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(125, dpi) };
    DrawGuidedStepper(dc, stepperRect, steps, gFonts, dpi);

    // Main Test Area
    int mainW = r.right - r.left - UiMetrics::Scale(48, dpi) - UiMetrics::Scale(250, dpi);
    RECT testArea{ r.left + UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(135, dpi), r.left + UiMetrics::Scale(24, dpi) + mainW, r.bottom - UiMetrics::Scale(60, dpi) };
    DrawRoundedCard(dc, testArea, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, testArea.left + UiMetrics::Scale(16, dpi), testArea.top + UiMetrics::Scale(12, dpi), L"Đang kiểm tra: Bàn phím & Touchpad (Wizard tương tác)", 52);

    RECT kbRect{ testArea.left + UiMetrics::Scale(16, dpi), testArea.top + UiMetrics::Scale(40, dpi), testArea.left + UiMetrics::Scale(380, dpi), testArea.top + UiMetrics::Scale(230, dpi) };
    DrawRoundedCard(dc, kbRect, UiMetrics::RadiusSm, RGB(248, 250, 252), UiColors::CardBorder, 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, kbRect.left + UiMetrics::Scale(10, dpi), kbRect.top + UiMetrics::Scale(8, dpi), L"1. Bấm nút bên dưới để mở Wizard kiểm tra phím vật lý:", 54);

    const wchar_t* rows[] = {
        L"Esc  F1  F2  F3  F4  F5  F6  F7  F8  F9  F10 F11 F12 Del",
        L"`  1  2  3  4  5  6  7  8  9  0  -  =  Backspace",
        L"Tab   Q   W   E   R   T   Y   U   I   O   P   [   ]   \\",
        L"Caps   A   S   D   F   G   H   J   K   L   ;   '   Enter",
        L"Shift    Z   X   C   V   B   N   M   ,   .   /    Shift",
        L"Ctrl   Fn   Win   Alt      Space      Alt   Ctrl  ◀  ▲  ▼  ▶"
    };
    int ky = kbRect.top + UiMetrics::Scale(32, dpi);
    SelectObject(dc, gFonts.hMono);
    SetTextColor(dc, UiColors::TextMuted);
    for (int i = 0; i < 6; ++i) {
        TextOutW(dc, kbRect.left + UiMetrics::Scale(12, dpi), ky, rows[i], (int)wcslen(rows[i]));
        ky += UiMetrics::Scale(22, dpi);
    }

    // Right Rail: Functional summary
    int rightX = r.right - UiMetrics::Scale(250, dpi);
    RECT checkCard{ rightX, r.top + UiMetrics::Scale(135, dpi), r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(60, dpi) };
    NextActionConfig nac;
    nac.actionTitle = L"Tiến trình tương tác";
    nac.reasonText = L"Hoàn tất các bước kiểm tra phần cứng vật lý.";
    nac.remainingTasks = { L"Kiểm tra Màn hình", L"Kiểm tra Bàn phím & Touch", L"Kiểm tra Loa & Microphone", L"Kiểm tra Cổng kết nối" };
    nac.buttonText = L"TIẾP TỤC BƯỚC KẾ";
    nac.isButtonEnabled = gAuditReady;
    DrawNextActionPanel(dc, checkCard, nac, gFonts, dpi);
}

void RenderGenericScreen(HDC dc, const RECT& r, MainTab tab, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    switch (tab) {
    case MainTab::NewSession:
        hdr.title = L"Phiên kiểm định mới";
        hdr.subtitle = L"Khởi tạo phiên kiểm định máy cũ, nạp cấu hình và chuẩn bị quy trình kiểm tra.";
        break;
    case MainTab::SellerClaim:
        hdr.title = L"Cam kết người bán";
        hdr.subtitle = L"Ghi nhận thông tin rao bán (Model, CPU, RAM, Ổ cứng, Giá) để đối chiếu sai lệch.";
        break;
    case MainTab::PhysicalSafety:
        hdr.title = L"Ngoại hình & An toàn";
        hdr.subtitle = L"Kiểm tra 6 điểm vật lý trọng yếu: Bản lề, Vỏ máy, Ốc vít/Cạy mở, Vào nước, Phồng pin, Sạc.";
        break;
    case MainTab::PortsPower:
        hdr.title = L"Cổng & Nguồn";
        hdr.subtitle = L"Kiểm tra từng cổng cắm vật lý, chuẩn giao tiếp USB/Thunderbolt và nguồn sạc AC adapter.";
        break;
    case MainTab::Stress:
        hdr.title = L"Stress & Ổn định";
        hdr.subtitle = L"Kiểm tra tải nặng CPU/RAM/GPU, theo dõi nhiệt độ, công suất và phát hiện quá nhiệt giảm xung.";
        break;
    case MainTab::Battery:
        hdr.title = L"Pin & Năng lượng";
        hdr.subtitle = L"Chi tiết dung lượng thiết kế, dung lượng thực tế, độ chai pin, chu kỳ sạc và công suất xả mW.";
        break;
    case MainTab::Storage:
        hdr.title = L"Lưu trữ";
        hdr.subtitle = L"Thông tin chi tiết NVMe/SATA SSD, dữ liệu S.M.A.R.T., số giờ bật máy và kiểm tra hệ thống tệp.";
        break;
    case MainTab::Memory:
        hdr.title = L"Bộ nhớ (RAM)";
        hdr.subtitle = L"Chi tiết thanh RAM, tốc độ bus, số khe cắm DIMM và kiểm tra tính toàn vẹn bộ nhớ.";
        break;
    case MainTab::Display:
        hdr.title = L"Hiển thị (Màn hình)";
        hdr.subtitle = L"Dữ liệu EDID gốc, nhà sản xuất tấm nền, độ phân giải native, tần số quét và kiểm tra điểm chết.";
        break;
    case MainTab::AudioCamera:
        hdr.title = L"Âm thanh & Camera";
        hdr.subtitle = L"Kiểm tra thiết bị âm thanh waveIn/waveOut stereo và camera Media Foundation.";
        break;
    case MainTab::Network:
        hdr.title = L"Mạng & Kết nối";
        hdr.subtitle = L"Kiểm tra kết nối Wi-Fi WLAN API, chất lượng sóng RSSI và Bluetooth radio.";
        break;
    case MainTab::SystemInfo:
        hdr.title = L"Thông tin Hệ thống";
        hdr.subtitle = L"Toàn bộ thông số BIOS, Bo mạch chủ, TPM, Secure Boot và hệ điều hành.";
        break;
    case MainTab::FactoryProfileMatch:
        hdr.title = L"Hồ sơ & Đối chiếu";
        hdr.subtitle = L"So sánh đối chiếu phần cứng hiện tại với cấu hình xuất xưởng gốc của nhà máy.";
        break;
    case MainTab::Reports:
        hdr.title = L"Đánh giá cuối cùng & Báo cáo";
        hdr.subtitle = L"Tổng hợp bằng chứng, phân tích rủi ro và đưa ra kết luận khuyến nghị mua máy.";
        break;
    case MainTab::ExportShare:
        hdr.title = L"Xuất báo cáo & Chia sẻ";
        hdr.subtitle = L"Xuất báo cáo định dạng HTML/JSON có chữ ký bằng chứng để lưu trữ hoặc gửi người bán.";
        break;
    case MainTab::LogsEvents:
        hdr.title = L"Nhật ký & Sự kiện";
        hdr.subtitle = L"Nhật ký hệ thống, sự kiện WHEA mã lỗi phần cứng và lịch sử vận hành.";
        break;
    case MainTab::Settings:
        hdr.title = L"Cài đặt";
        hdr.subtitle = L"Cấu hình công cụ, chính sách tin cậy mã băm SHA-256 và ngôn ngữ.";
        break;
    case MainTab::SessionHistory:
        hdr.title = L"Lịch sử phiên kiểm định";
        hdr.subtitle = L"Danh sách các phiên kiểm tra laptop đã thực hiện trên thiết bị này.";
        break;
    default:
        hdr.title = L"LapSure";
        hdr.subtitle = L"Phần mềm kiểm định và chẩn đoán laptop chuyên nghiệp.";
        break;
    }
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    // Render Clean State Content
    RECT contentArea{ r.left + UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi) };
    if (!gAuditReady && !gRunning) {
        EmptyStateConfig esc;
        esc.state = CanonicalUiState::NotTested;
        esc.title = L"Chưa có dữ liệu kiểm tra cho phân hệ này";
        esc.description = L"Vui lòng thực hiện phiên kiểm tra tự động toàn diện để thu thập bằng chứng chẩn đoán.";
        esc.recoveryHint = L"Bấm “BẮT ĐẦU KIỂM TRA” từ màn hình Tổng quan để khởi động.";
        DrawEmptyState(dc, contentArea, esc, gFonts, dpi);
    } else {
        // Render Structured Findings Data Table (C09)
        DataTableConfig dtc;
        dtc.columns = {
            { L"Phân hệ", 110, false, false },
            { L"Hạng mục", 130, false, false },
            { L"Tên thông số", 160, false, false },
            { L"Giá trị thực tế", 220, true, false },
            { L"Trạng thái", 100, false, true }
        };

        std::lock_guard<std::mutex> lk(gReportMutex);
        for (const auto& f : rep.findings) {
            TableRow row;
            row.cells.push_back(UiDimension(f.dimension));
            row.cells.push_back(f.group);
            row.cells.push_back(f.name);
            row.cells.push_back(f.value);
            row.cells.push_back(UiState(f.state));
            row.rowState = MapState(f.state);
            dtc.rows.push_back(row);
        }
        DrawDataTable(dc, contentArea, dtc, gFonts, dpi, gTableScrollOffset);
    }
}

} // namespace

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        gMainHwnd = h;
        int dpi = GetDpiForHwnd(h);
        gFonts.Init(dpi);
        SetTimer(h, 1, 1000, nullptr);
        
        // Hidden controls for command routing and test hooks
        gMode = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | CBS_DROPDOWNLIST, -100, -100, 10, 10, h, (HMENU)3, nullptr, nullptr);
        gBtn = CreateWindowW(L"BUTTON", L"BẮT ĐẦU KIỂM TRA", WS_CHILD, -100, -100, 10, 10, h, (HMENU)1, nullptr, nullptr);
        gOpen = CreateWindowW(L"BUTTON", L"XEM KẾT QUẢ", WS_CHILD, -100, -100, 10, 10, h, (HMENU)2, nullptr, nullptr); EnableWindow(gOpen, FALSE);
        gFuncDisplay = CreateWindowW(L"BUTTON", L"Màn hình", WS_CHILD, -100, -100, 10, 10, h, (HMENU)1201, nullptr, nullptr); EnableWindow(gFuncDisplay,FALSE);
        gFuncKeyboard = CreateWindowW(L"BUTTON", L"Bàn phím", WS_CHILD, -100, -100, 10, 10, h, (HMENU)1202, nullptr, nullptr); EnableWindow(gFuncKeyboard, FALSE);
        gFuncTouch = CreateWindowW(L"BUTTON", L"Cảm ứng", WS_CHILD, -100, -100, 10, 10, h, (HMENU)1203, nullptr, nullptr); EnableWindow(gFuncTouch, FALSE);
        gFuncSpeaker = CreateWindowW(L"BUTTON", L"Loa", WS_CHILD, -100, -100, 10, 10, h, (HMENU)1204, nullptr, nullptr); EnableWindow(gFuncSpeaker, FALSE);
        gFuncUsb = CreateWindowW(L"BUTTON", L"Cổng USB", WS_CHILD, -100, -100, 10, 10, h, (HMENU)1205, nullptr, nullptr); EnableWindow(gFuncUsb, FALSE);
        gPhysical = CreateWindowW(L"BUTTON", L"Ngoại hình", WS_CHILD, -100, -100, 10, 10, h, (HMENU)1208, nullptr, nullptr); EnableWindow(gPhysical, FALSE);
        gFuncIo = CreateWindowW(L"BUTTON", L"Thiết bị tự động", WS_CHILD, -100, -100, 10, 10, h, (HMENU)1206, nullptr, nullptr); EnableWindow(gFuncIo, FALSE);
        gPortTest = CreateWindowW(L"BUTTON", L"Kiểm tra cổng", WS_CHILD, -100, -100, 10, 10, h, (HMENU)1207, nullptr, nullptr); EnableWindow(gPortTest, FALSE);
        gNext = CreateWindowW(L"BUTTON", L"TIẾP TỤC BƯỚC KẾ", WS_CHILD, -100, -100, 10, 10, h, (HMENU)1300, nullptr, nullptr); EnableWindow(gNext, FALSE);
        gSeller = CreateWindowW(L"BUTTON", L"Cấu hình bán", WS_CHILD, -100, -100, 10, 10, h, (HMENU)1209, nullptr, nullptr); EnableWindow(gSeller, FALSE);
        gProgress = CreateWindowW(L"STATIC", L"Quy trình: chưa bắt đầu", WS_CHILD, -100, -100, 10, 10, h, (HMENU)1301, nullptr, nullptr);
        gStatus = CreateWindowW(L"STATIC", L"Sẵn sàng", WS_CHILD, -100, -100, 10, 10, h, nullptr, nullptr, nullptr);
        gList = CreateWindowW(WC_LISTVIEWW, L"", WS_CHILD | LVS_REPORT | LVS_SINGLESEL, -100, -100, 10, 10, h, nullptr, nullptr, nullptr);
        return 0;
    }
    case WM_DPICHANGED: {
        int newDpi = HIWORD(w);
        gFonts.Init(newDpi);
        RECT* const prcNewWindow = (RECT*)l;
        SetWindowPos(h, nullptr,
            prcNewWindow->left, prcNewWindow->top,
            prcNewWindow->right - prcNewWindow->left,
            prcNewWindow->bottom - prcNewWindow->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(h, nullptr, FALSE);
        return 0;
    }
    case WM_TIMER: {
        if(gRunning) {
            auto now = std::chrono::steady_clock::now();
            gAuditElapsedSec = (int)std::chrono::duration_cast<std::chrono::seconds>(now - gAuditStartTime).count();
            InvalidateRect(h, nullptr, FALSE);
        }
        return 0;
    }
    case WM_KEYDOWN: {
        switch (w) {
        case VK_TAB:
            if (GetKeyState(VK_SHIFT) & 0x8000) gFocusIndex = (gFocusIndex + 3) % 4;
            else gFocusIndex = (gFocusIndex + 1) % 4;
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        case VK_UP:
            if (gFocusIndex == 0) {
                int cur = (int)gCurrentTab;
                if (cur > 0) gCurrentTab = (MainTab)(cur - 1);
                InvalidateRect(h, nullptr, FALSE);
            }
            return 0;
        case VK_DOWN:
            if (gFocusIndex == 0) {
                int cur = (int)gCurrentTab;
                if (cur < (int)MainTab::InterruptedRecovery) gCurrentTab = (MainTab)(cur + 1);
                InvalidateRect(h, nullptr, FALSE);
            }
            return 0;
        case VK_LEFT:
            if (gFocusIndex == 1) {
                if (gSelectedMode == L"Deep") gSelectedMode = L"Standard";
                else if (gSelectedMode == L"Standard") gSelectedMode = L"Quick";
                InvalidateRect(h, nullptr, FALSE);
            }
            return 0;
        case VK_RIGHT:
            if (gFocusIndex == 1) {
                if (gSelectedMode == L"Quick") gSelectedMode = L"Standard";
                else if (gSelectedMode == L"Standard") gSelectedMode = L"Deep";
                InvalidateRect(h, nullptr, FALSE);
            }
            return 0;
        case VK_RETURN:
        case VK_SPACE:
            if (gFocusIndex == 2) StartAudit(h);
            else if (gFocusIndex == 3) PostMessageW(h, WM_COMMAND, 1300, 0);
            return 0;
        }
        break;
    }
    case WM_MOUSEWHEEL: {
        short delta = GET_WHEEL_DELTA_WPARAM(w);
        if (delta > 0) {
            if (gTableScrollOffset > 0) gTableScrollOffset--;
            if (gSidebarScrollOffset > 0) gSidebarScrollOffset = std::max(0, gSidebarScrollOffset - 30);
        } else {
            gTableScrollOffset++;
            gSidebarScrollOffset += 30;
        }
        InvalidateRect(h, nullptr, FALSE);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(w);
        if (id == 1) StartAudit(h);
        else if (id == 2 && !gReportPath.empty()) ShellExecuteW(h, L"open", gReportPath.c_str(), nullptr, nullptr, SW_SHOW);
        else if (id == 1201) { if (CanRunManualTest(h)) CommitManualResult(RunDisplayColorWizard(h)); return 0; }
        else if (id == 1202) { if (CanRunManualTest(h)) CommitManualResult(RunKeyboardWizard(h)); return 0; }
        else if (id == 1203) { if (CanRunManualTest(h)) { auto caps = DetectCapabilities(gDir); auto fc = DetectFunctionalCapabilities(caps, &gCancel); CommitManualResult(RunTouchGridWizard(h, fc.touchPresent)); } return 0; }
        else if (id == 1204) { if (CanRunManualTest(h)) { auto caps = DetectCapabilities(gDir); auto fc = DetectFunctionalCapabilities(caps, &gCancel); CommitManualResult(RunSpeakerWizard(h, fc.audioPresent)); } return 0; }
        else if (id == 1205) { if (CanRunManualTest(h)) { auto caps = DetectCapabilities(gDir); CommitManualResult(RunUsbPortWizard(h, caps, &gCancel)); } return 0; }
        else if (id == 1206) { if (CanRunManualTest(h)) CommitManualResults(RunFunctionalIoWizard(h)); return 0; }
        else if (id == 1207) { if (CanRunManualTest(h)) { wchar_t label[64] = L"USB-C / USB-A port"; CommitPortResult(RunPhysicalPortProbe(h, label, &gCancel)); } return 0; }
        else if (id == 1208) { if (CanRunManualTest(h)) CommitManualResults(RunPhysicalConditionWizard(h)); return 0; }
        else if (id == 1209) { if (CanRunManualTest(h)) { SellerClaim claim; if (RunSellerClaimWizard(h, claim)) CommitSellerClaim(claim); } return 0; }
        else if (id == 1210) { ShowAboutDialog(h); return 0; }
        else if (id==1300) {
            if (!CanRunManualTest(h)) return 0;
            BuildOrchestrator(gReport, false, true);
            auto& f = gReport.hardware.stress.functional;
            if (f.manualRequired || f.notTested) CommitManualResults(RunFunctionalIoWizard(h));
            else if (gReport.hardware.stress.portPower.overall != L"PASS") {
                std::wstring label = L"USB-C / USB-A port", cap;
                auto prof = gReport.hardware.stress.chassisProfile;
                if (!prof.ports.empty() && !SelectNextChassisPort(h, prof, label, cap)) return 0;
                CommitPortResultGuided(RunPhysicalPortProbe(h, label, &gCancel));
            }
            else if (!gReportPath.empty()) ShellExecuteW(h, L"open", gReportPath.c_str(), nullptr, nullptr, SW_SHOW);
            return 0;
        }
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int x = LOWORD(l), y = HIWORD(l);
        RECT cr; GetClientRect(h, &cr);
        int dpi = GetDpiForHwnd(h);
        auto layout = ComputeAppShellLayout(cr, dpi);
        
        // 1. Sidebar Grouped Navigation Click
        MainTab clickedTab;
        bool toggleDevice = false;
        int hitRes = HitTestSidebar(x, y, layout.sidebarRect, dpi, gDeviceGroupExpanded, gSidebarScrollOffset, clickedTab, toggleDevice);
        if (hitRes == 2 && toggleDevice) {
            gDeviceGroupExpanded = !gDeviceGroupExpanded;
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        }
        if (hitRes == 1) {
            gCurrentTab = clickedTab;
            gTableScrollOffset = 0;
            InvalidateRect(h, nullptr, FALSE);
            if (clickedTab == MainTab::Settings) ShowAboutDialog(h);
            return 0;
        }

        // 2. Start / Stop Button Click
        int btnW = UiMetrics::Scale(230, dpi);
        int btnH = UiMetrics::Scale(40, dpi);
        int modeY = layout.contentRect.top + UiMetrics::Scale(70, dpi);
        RECT btnRect{ cr.right - btnW - UiMetrics::Scale(24, dpi), modeY - UiMetrics::Scale(2, dpi), cr.right - UiMetrics::Scale(24, dpi), modeY - UiMetrics::Scale(2, dpi) + btnH };
        if (x >= btnRect.left && x <= btnRect.right && y >= btnRect.top && y <= btnRect.bottom) {
            StartAudit(h);
            return 0;
        }

        // 3. Mode Pills Click
        int mX = layout.contentRect.left + UiMetrics::Scale(134, dpi);
        int pillW = UiMetrics::Scale(80, dpi);
        int pillH = UiMetrics::Scale(28, dpi);
        if (y >= modeY && y <= modeY + pillH) {
            if (x >= mX && x <= mX + pillW) { gSelectedMode = L"Quick"; InvalidateRect(h, nullptr, FALSE); }
            else if (x >= mX + pillW + 6 && x <= mX + (pillW + 6) * 2) { gSelectedMode = L"Standard"; InvalidateRect(h, nullptr, FALSE); }
            else if (x >= mX + (pillW + 6) * 2 && x <= mX + (pillW + 6) * 3) { gSelectedMode = L"Deep"; InvalidateRect(h, nullptr, FALSE); }
        }

        // 4. AutoAudit Screen: Cancel Button & Next Action Button Hit-Test
        if (gCurrentTab == MainTab::AutoAudit) {
            RECT btnCancel{ cr.right - UiMetrics::Scale(160, dpi), layout.contentRect.top + UiMetrics::Scale(24, dpi), cr.right - UiMetrics::Scale(24, dpi), layout.contentRect.top + UiMetrics::Scale(56, dpi) };
            if (x >= btnCancel.left && x <= btnCancel.right && y >= btnCancel.top && y <= btnCancel.bottom) {
                if (gRunning) {
                    gCancel = true;
                    gSessionLifecycleState = CanonicalUiState::Cancelled;
                    PostStatus(h, L"Đã yêu cầu hủy kiểm tra...");
                }
                return 0;
            }
            int rightX = cr.right - UiMetrics::Scale(250, dpi);
            RECT nextCardRect{ rightX, layout.contentRect.top + UiMetrics::Scale(80, dpi), cr.right - UiMetrics::Scale(24, dpi), layout.contentRect.top + UiMetrics::Scale(240, dpi) };
            if (x >= nextCardRect.left && x <= nextCardRect.right && y >= nextCardRect.top && y <= nextCardRect.bottom) {
                if (gAuditReady) PostMessageW(h, WM_COMMAND, 1300, 0);
                else StartAudit(h);
                return 0;
            }
        }

        // 5. Functional Screen: Next Action Button Hit-Test
        if (gCurrentTab == MainTab::Functional) {
            int rightX = cr.right - UiMetrics::Scale(250, dpi);
            RECT checkCard{ rightX, layout.contentRect.top + UiMetrics::Scale(135, dpi), cr.right - UiMetrics::Scale(24, dpi), layout.contentRect.bottom - UiMetrics::Scale(60, dpi) };
            if (x >= checkCard.left && x <= checkCard.right && y >= checkCard.top && y <= checkCard.bottom) {
                PostMessageW(h, WM_COMMAND, 1300, 0);
                return 0;
            }
        }

        // 6. Dashboard Screen: Factory Compare Button Hit-Test
        if (gCurrentTab == MainTab::Dashboard) {
            int rightPanelW = UiMetrics::Scale(250, dpi);
            int rightX = cr.right - rightPanelW - UiMetrics::Scale(24, dpi);
            int kpiY = modeY + UiMetrics::Scale(46, dpi);
            RECT covCardRect{ rightX, kpiY, cr.right - UiMetrics::Scale(24, dpi), kpiY + UiMetrics::Scale(150, dpi) };
            RECT factoryCard{ rightX, covCardRect.bottom + UiMetrics::Scale(12, dpi), cr.right - UiMetrics::Scale(24, dpi), covCardRect.bottom + UiMetrics::Scale(155, dpi) };
            RECT btnProfileDetail{ factoryCard.left + UiMetrics::Scale(14, dpi), factoryCard.bottom - UiMetrics::Scale(36, dpi), factoryCard.right - UiMetrics::Scale(14, dpi), factoryCard.bottom - UiMetrics::Scale(10, dpi) };
            if (x >= btnProfileDetail.left && x <= btnProfileDetail.right && y >= btnProfileDetail.top && y <= btnProfileDetail.bottom) {
                gCurrentTab = MainTab::FactoryProfileMatch;
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
        }

        return 0;
    }
    case WM_AUDIT_STATUS: {
        auto* s = (std::wstring*)l;
        if (s) { delete s; }
        InvalidateRect(h, nullptr, FALSE);
        return 0;
    }
    case WM_AUDIT_DONE: {
        Fill();
        SetFunctionalButtonsEnabled(w?FALSE:TRUE);
        if (gNext) EnableWindow(gNext,w?FALSE:TRUE);
        gRunning = false; gAuditReady = (w == 0);
        gSessionLifecycleState = (w == 0) ? CanonicalUiState::Pass : CanonicalUiState::Cancelled;
        InvalidateRect(h, nullptr, FALSE);
        if (gCloseRequested) DestroyWindow(h);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(h, &ps);
        RECT cr; GetClientRect(h, &cr);
        int dpi = GetDpiForHwnd(h);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, cr.right - cr.left, cr.bottom - cr.top);
        HGDIOBJ oldBM = SelectObject(memDC, memBM);

        // C01 App Shell Background
        DrawAppShellBackground(memDC, cr);

        auto layout = ComputeAppShellLayout(cr, dpi);

        // C02 Grouped Navigation Sidebar
        std::wstring stStr = gRunning ? L"Đang kiểm tra..." : (gAuditReady ? L"Sẵn sàng" : ((gSessionLifecycleState == CanonicalUiState::Cancelled) ? L"Đã hủy" : L"Chưa bắt đầu"));
        DrawSidebar(memDC, layout.sidebarRect, gCurrentTab, gFonts, dpi, gDeviceGroupExpanded, gSidebarScrollOffset, stStr, L"Windows x64 Native");

        // Content Area Screen Rendering
        if (gCurrentTab == MainTab::Dashboard) {
            RenderDashboard(memDC, layout.contentRect, gReport, dpi);
        } else if (gCurrentTab == MainTab::AutoAudit) {
            RenderAutoAudit(memDC, layout.contentRect, gReport, dpi);
        } else if (gCurrentTab == MainTab::Functional) {
            RenderFunctional(memDC, layout.contentRect, gReport, dpi);
        } else {
            RenderGenericScreen(memDC, layout.contentRect, gCurrentTab, gReport, dpi);
        }

        // C01 App Shell Footer (with dynamic engine count)
        int readyEngines = GetReadyEngineCount();
        DrawAppShellFooter(memDC, layout.footerRect, gFonts, readyEngines, 14);

        BitBlt(hdc, 0, 0, cr.right - cr.left, cr.bottom - cr.top, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBM);
        DeleteObject(memBM);
        DeleteDC(memDC);
        EndPaint(h, &ps);
        return 0;
    }
    case WM_CLOSE:
        if(gRunning) {
            gCloseRequested = true; gCancel = true; return 0;
        }
        DestroyWindow(h);
        return 0;
    case WM_DESTROY:
        gCancel = true;
        if (gWorker.joinable()) gWorker.join();
        gFonts.Cleanup();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, LPWSTR, int) {
    gDir = AppDir();
    int argc = 0;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool inventoryOnly = false;
    std::wstring outputDir;
    for (int i = 1; i < argc; i++) {
        if (std::wstring(argv[i]) == L"--inventory-only") inventoryOnly = true;
        else if (std::wstring(argv[i]) == L"--output" && i + 1 < argc) outputDir = argv[++i];
    }
    if (argv) LocalFree(argv);
    if (inventoryOnly) {
        try { return RunInventoryOnly(outputDir); }
        catch (...) { return 3; }
    }
    
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    INITCOMMONCONTROLSEX ic{ sizeof(ic), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&ic);
    
    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"LapSure";
    RegisterClassExW(&wc);
    
    CreateWindowExW(0, wc.lpszClassName, L"LapSure — Kiểm tra laptop toàn diện",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 820,
        nullptr, nullptr, hi, nullptr);
        
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
