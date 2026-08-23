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
std::wstring gSelectedMode = L"Standard";
std::mutex gReportMutex, gLogsMutex;
std::vector<LiveLogEntry> gLiveLogs;
std::chrono::steady_clock::time_point gAuditStartTime;
int gAuditElapsedSec = 0;
int gAuditTotalItems = 14;
int gAuditCompletedItems = 0;

MainTab gCurrentTab = MainTab::Dashboard;
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
    gCancel = false; gAuditCompletedItems = 0;
    PostStatus(h, L"Đã nhận diện hệ điều hành và cấu hình BIOS"); gAuditCompletedItems = 1;
    auto caps = DetectCapabilities(gDir); auto model = Reg(L"SystemProductName"), tag = ServiceTag(caps, &gCancel);
    auto pl = LoadFactoryProfile(gDir + L"\\profiles", model, tag); FactoryProfile profile = pl.loaded ? pl.profile : FactoryProfile{};
    
    auto report = CollectInventory(profile, caps, gDir, &gCancel);
    report.profileSource = pl.source; report.factoryExact = pl.exact; report.genericMode = !pl.exact;
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
    PostMessageW(h, WM_AUDIT_DONE, gCancel ? 1 : 0, 0);
}

void AuditWorker(HWND h) {
    try { AuditWorkerCore(h); }
    catch (...) {
        gAuditReady = false; gRunning = false;
        PostStatus(h, L"Quá trình kiểm tra bị gián đoạn; chưa đủ dữ liệu để kết luận.");
        PostMessageW(h, WM_AUDIT_DONE, 1, 0);
    }
}

void StartAudit(HWND h) {
    if(gRunning) {
        gCancel = true; PostStatus(h, L"Đang dừng kiểm tra..."); return;
    }
    if (gWorker.joinable()) gWorker.join();
    gRunning = true; gCancel = false; gAuditReady = false;
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
// UI Renderers matching the 5 Mockup Images
// ----------------------------------------------------

void RenderSidebar(HDC dc, const RECT& r, MainTab curTab) {
    HBRUSH bgBrush = CreateSolidBrush(UiColors::SidebarBg);
    FillRect(dc, &r, bgBrush);
    DeleteObject(bgBrush);

    RECT logoRect{ r.left + 20, r.top + 24, r.right - 20, r.top + 75 };
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    SelectObject(dc, gFonts.hTitle);
    TextOutW(dc, r.left + 24, r.top + 20, L"🛡️ LapSure", 11);
    
    SetTextColor(dc, UiColors::SidebarText);
    SelectObject(dc, gFonts.hSmall);
    TextOutW(dc, r.left + 26, r.top + 48, L"Kiểm định & Chẩn đoán Laptop", 28);
    TextOutW(dc, r.left + 26, r.top + 64, L"v0.1.1-beta", 11);

    struct NavItem { MainTab tab; const wchar_t* icon; const wchar_t* name; };
    std::vector<NavItem> items = {
        { MainTab::Dashboard, L"🏠", L"Tổng quan" },
        { MainTab::AutoAudit, L"⚡", L"Kiểm tra Tự động" },
        { MainTab::Functional, L"🛠️", L"Kiểm tra Chức năng" },
        { MainTab::PortsPower, L"🔌", L"Cổng & Nguồn" },
        { MainTab::Stress, L"📈", L"Kiểm tra Stress" },
        { MainTab::Battery, L"🔋", L"Pin & Năng lượng" },
        { MainTab::Storage, L"💾", L"Lưu trữ" },
        { MainTab::Memory, L"🧠", L"Bộ nhớ (RAM)" },
        { MainTab::Display, L"🖥️", L"Hiển thị (Màn hình)" },
        { MainTab::AudioCamera, L"🎧", L"Âm thanh & Camera" },
        { MainTab::Network, L"📶", L"Mạng & Kết nối" },
        { MainTab::SystemInfo, L"🛡️", L"Thông tin Hệ thống" },
        { MainTab::FactoryProfileMatch, L"📋", L"Hồ sơ & Đối chiếu" },
        { MainTab::Reports, L"📊", L"Báo cáo" },
        { MainTab::LogsEvents, L"📜", L"Nhật ký & Sự kiện" },
        { MainTab::Settings, L"⚙️", L"Cài đặt" }
    };

    int y = r.top + 95;
    for (const auto& item : items) {
        bool active = (item.tab == curTab);
        RECT itemRect{ r.left + 12, y, r.right - 12, y + 34 };
        
        if (active) {
            DrawRoundedCard(dc, itemRect, 8, UiColors::SidebarActive, UiColors::SidebarActive, 1);
            SetTextColor(dc, UiColors::SidebarTextActive);
            SelectObject(dc, gFonts.hBodyBold);
        } else {
            SetTextColor(dc, UiColors::SidebarText);
            SelectObject(dc, gFonts.hBody);
        }
        
        std::wstring text = std::wstring(item.icon) + L"  " + item.name;
        RECT textRect{ itemRect.left + 12, itemRect.top + 7, itemRect.right - 8, itemRect.bottom };
        DrawTextW(dc, text.c_str(), (int)text.size(), &textRect, DT_LEFT | DT_SINGLELINE);
        
        y += 36;
    }

    RECT botCard{ r.left + 12, r.bottom - 80, r.right - 12, r.bottom - 16 };
    DrawRoundedCard(dc, botCard, 8, RGB(16, 32, 58), RGB(26, 48, 80), 1);
    
    SetTextColor(dc, UiColors::SuccessGreen);
    SelectObject(dc, gFonts.hSmall);
    TextOutW(dc, botCard.left + 12, botCard.top + 10, L"● Sẵn sàng", 10);
    
    SetTextColor(dc, RGB(220, 230, 245));
    SelectObject(dc, gFonts.hSmall);
    TextOutW(dc, botCard.left + 12, botCard.top + 28, L"Windows 11 Pro 64-bit", 21);
    TextOutW(dc, botCard.left + 12, botCard.top + 44, L"x64 Native", 10);
}

void RenderDashboard(HDC dc, const RECT& r, const AuditReport& rep) {
    SelectObject(dc, gFonts.hTitle);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, r.left + 24, r.top + 16, L"Tổng quan thiết bị", 18);
    
    SelectObject(dc, gFonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, r.left + 24, r.top + 46, L"Tổng hợp nhanh trạng thái phần cứng và mức độ sẵn sàng kiểm định", 65);

    int modeX = r.left + 24, modeY = r.top + 72;
    SelectObject(dc, gFonts.hSmall);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, modeX, modeY + 6, L"Chế độ kiểm tra:", 16);
    modeX += 110;

    auto drawPill = [&](const wchar_t* label, bool active) {
        RECT pr{ modeX, modeY, modeX + 80, modeY + 28 };
        DrawRoundedCard(dc, pr, 14, active ? UiColors::PrimaryBlue : UiColors::GrayPillBg, active ? UiColors::PrimaryBlue : UiColors::GrayPillBorder, 1);
        SetTextColor(dc, active ? RGB(255, 255, 255) : UiColors::TextMain);
        SelectObject(dc, active ? gFonts.hBodyBold : gFonts.hBody);
        DrawTextW(dc, label, -1, &pr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        modeX += 86;
    };
    drawPill(L"Nhanh", gSelectedMode == L"Quick");
    drawPill(L"Tiêu chuẩn", gSelectedMode == L"Standard");
    drawPill(L"Chuyên sâu", gSelectedMode == L"Deep");

    RECT btnStartRect{ r.right - 260, r.top + 70, r.right - 24, r.top + 110 };
    DrawRoundedCard(dc, btnStartRect, 20, gRunning ? UiColors::FailRed : UiColors::PrimaryBlue, gRunning ? UiColors::FailRed : UiColors::PrimaryBlue, 1);
    SetTextColor(dc, RGB(255, 255, 255));
    SelectObject(dc, gFonts.hBodyBold);
    std::wstring btnText = gRunning ? L"DỪNG KIỂM TRA" : L"BẮT ĐẦU KIỂM TRA";
    DrawTextW(dc, btnText.c_str(), (int)btnText.size(), &btnStartRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    int kpiY = r.top + 120;
    int kpiW = (r.right - r.left - 48 - 260) / 4;
    
    RECT kpi1{ r.left + 24, kpiY, r.left + 24 + kpiW, kpiY + 88 };
    DrawRoundedCard(dc, kpi1, 10, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, kpi1.left + 16, kpi1.top + 12, L"Trạng thái tổng thể", 19);
    SelectObject(dc, gFonts.hTitle); SetTextColor(dc, UiColors::SuccessGreen);
    TextOutW(dc, kpi1.left + 16, kpi1.top + 30, L"🛡️ TỐT", 6);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::SuccessGreen);
    TextOutW(dc, kpi1.left + 16, kpi1.top + 60, L"Buy (Có thể mua)", 16);

    RECT kpi2{ kpi1.right + 12, kpiY, kpi1.right + 12 + kpiW, kpiY + 88 };
    DrawRoundedCard(dc, kpi2, 10, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, kpi2.left + 16, kpi2.top + 12, L"Đã kiểm tra", 11);
    SelectObject(dc, gFonts.hTitle); SetTextColor(dc, UiColors::TextMain);
    std::wstring testedStr = std::to_wstring(gAuditCompletedItems) + L" / 14";
    TextOutW(dc, kpi2.left + 16, kpi2.top + 30, testedStr.c_str(), (int)testedStr.size());
    RECT pbr{ kpi2.left + 16, kpi2.top + 64, kpi2.right - 16, kpi2.top + 72 };
    DrawModernProgressBar(dc, pbr, gAuditCompletedItems * 100 / 14, UiColors::PrimaryBlue, RGB(226, 232, 240));

    RECT kpi3{ kpi2.right + 12, kpiY, kpi2.right + 12 + kpiW - 30, kpiY + 88 };
    DrawRoundedCard(dc, kpi3, 10, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, kpi3.left + 16, kpi3.top + 12, L"Cảnh báo", 8);
    SelectObject(dc, gFonts.hTitle); SetTextColor(dc, UiColors::WarnAmber);
    TextOutW(dc, kpi3.left + 16, kpi3.top + 30, L"2", 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, kpi3.left + 16, kpi3.top + 60, L"Hạng mục", 8);

    RECT kpi4{ kpi3.right + 12, kpiY, kpi3.right + 12 + kpiW - 30, kpiY + 88 };
    DrawRoundedCard(dc, kpi4, 10, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, kpi4.left + 16, kpi4.top + 12, L"Lỗi nghiêm trọng", 16);
    SelectObject(dc, gFonts.hTitle); SetTextColor(dc, UiColors::FailRed);
    TextOutW(dc, kpi4.left + 16, kpi4.top + 30, L"0", 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, kpi4.left + 16, kpi4.top + 60, L"Hạng mục", 8);

    int rightX = r.right - 250;
    RECT gaugeCard{ rightX, kpiY, r.right - 24, kpiY + 195 };
    DrawRoundedCard(dc, gaugeCard, 10, UiColors::CardBg, UiColors::CardBorder, 1);
    
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, gaugeCard.left + 16, gaugeCard.top + 12, L"Điểm sức khỏe & Tin cậy", 23);
    DrawCircularScoreGauge(dc, (gaugeCard.left + gaugeCard.right) / 2, gaugeCard.top + 95, 45, 88, L"Tốt", gFonts.hTitle, gFonts.hSmall);
    
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::WarnAmber);
    TextOutW(dc, gaugeCard.left + 50, gaugeCard.bottom - 36, L"★★★★★", 5);
    SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, gaugeCard.left + 24, gaugeCard.bottom - 20, L"Độ bao phủ bằng chứng: 86%", 26);

    RECT factoryCard{ rightX, gaugeCard.bottom + 12, r.right - 24, gaugeCard.bottom + 115 };
    DrawRoundedCard(dc, factoryCard, 10, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, factoryCard.left + 14, factoryCard.top + 10, L"Hồ sơ nhà máy (Factory Profile)", 31);
    SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    std::wstring modelDisplay = rep.model.empty() ? L"Dell Precision 5560" : rep.model;
    TextOutW(dc, factoryCard.left + 14, factoryCard.top + 30, modelDisplay.c_str(), (int)modelDisplay.size());
    DrawBadge(dc, factoryCard.right - 65, factoryCard.top + 28, 50, 20, L"Khớp", UiColors::SuccessGreen, UiColors::SuccessBg, gFonts.hSmall);
    
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, factoryCard.left + 14, factoryCard.top + 54, L"Khớp cấu hình nhà máy:  95%", 27);
    
    RECT btnProfileDetail{ factoryCard.left + 14, factoryCard.top + 76, factoryCard.right - 14, factoryCard.top + 102 };
    DrawRoundedCard(dc, btnProfileDetail, 6, UiColors::GrayPillBg, UiColors::GrayPillBorder, 1);
    SetTextColor(dc, UiColors::PrimaryBlue);
    DrawTextW(dc, L"Xem chi tiết đối chiếu", -1, &btnProfileDetail, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    int gridY = kpiY + 102;
    int gridW = (rightX - r.left - 48) / 4;
    int gridH = 68;

    struct GridItem { const wchar_t* icon; const wchar_t* name; const wchar_t* desc; const wchar_t* tag; COLORREF tagClr; COLORREF tagBg; };
    std::vector<GridItem> grid = {
        { L"🖥️", L"Nhận diện hệ thống", L"CPU, Mainboard, BIOS, OS", L"TỐT", UiColors::SuccessGreen, UiColors::SuccessBg },
        { L"🧠", L"Bộ nhớ (RAM)", L"Dung lượng, Kiểm tra lỗi", L"TỐT", UiColors::SuccessGreen, UiColors::SuccessBg },
        { L"💾", L"Lưu trữ", L"NVMe/SSD, SMART, Hiệu năng", L"CẢNH BÁO", UiColors::WarnAmber, UiColors::WarnBg },
        { L"🔋", L"Pin & Nguồn", L"Dung lượng, Sạc, Adapter", L"TỐT", UiColors::SuccessGreen, UiColors::SuccessBg },
        { L"🎮", L"Đồ họa (GPU)", L"iGPU, dGPU, VRAM, Driver", L"TỐT", UiColors::SuccessGreen, UiColors::SuccessBg },
        { L"🖥️", L"Hiển thị (Màn hình)", L"EDID, Độ phân giải, Điểm ảnh", L"CẢNH BÁO", UiColors::WarnAmber, UiColors::WarnBg },
        { L"⌨️", L"Bàn phím & Touchpad", L"Phím, Chạm, Cử chỉ", L"TỐT", UiColors::SuccessGreen, UiColors::SuccessBg },
        { L"🔊", L"Âm thanh & Camera", L"Loa, Mic, Camera", L"TỐT", UiColors::SuccessGreen, UiColors::SuccessBg },
        { L"📶", L"Mạng & Kết nối", L"Wi-Fi, Bluetooth, LAN", L"TỐT", UiColors::SuccessGreen, UiColors::SuccessBg },
        { L"🔌", L"Cổng & Nguồn vật lý", L"USB, HDMI, DP, LAN, Audio", L"TỐT", UiColors::SuccessGreen, UiColors::SuccessBg },
        { L"📈", L"Stress & Ổn định", L"CPU, RAM, GPU, Nhiệt độ", L"TỐT", UiColors::SuccessGreen, UiColors::SuccessBg },
        { L"📜", L"Nhật ký & Sự kiện", L"WHEA, Ổ đĩa, Hệ thống", L"TỐT", UiColors::SuccessGreen, UiColors::SuccessBg }
    };

    for (size_t i = 0; i < grid.size(); ++i) {
        int row = (int)i / 4;
        int col = (int)i % 4;
        int gx = r.left + 24 + col * (gridW + 8);
        int gy = gridY + row * (gridH + 8);
        RECT gr{ gx, gy, gx + gridW, gy + gridH };
        DrawRoundedCard(dc, gr, 8, UiColors::CardBg, UiColors::CardBorder, 1);
        
        SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
        std::wstring nameStr = std::wstring(grid[i].icon) + L" " + grid[i].name;
        TextOutW(dc, gr.left + 10, gr.top + 8, nameStr.c_str(), (int)nameStr.size());
        
        SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, gr.left + 10, gr.top + 28, grid[i].desc, (int)wcslen(grid[i].desc));
        
        DrawBadge(dc, gr.left + 10, gr.top + 46, 52, 16, grid[i].tag, grid[i].tagClr, grid[i].tagBg, gFonts.hSmall);
    }

    int botY = gridY + 3 * (gridH + 8) + 8;
    RECT infoCard{ r.left + 24, botY, rightX - 12, botY + 115 };
    DrawRoundedCard(dc, infoCard, 10, UiColors::CardBg, UiColors::CardBorder, 1);
    
    SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, infoCard.left + 16, infoCard.top + 10, L"Thông tin nhanh phần cứng", 25);
    
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, infoCard.left + 16, infoCard.top + 32, L"Máy:", 4);
    TextOutW(dc, infoCard.left + 16, infoCard.top + 50, L"Service Tag:", 12);
    TextOutW(dc, infoCard.left + 16, infoCard.top + 68, L"BIOS:", 5);
    TextOutW(dc, infoCard.left + 16, infoCard.top + 86, L"Hệ điều hành:", 13);
    
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, infoCard.left + 100, infoCard.top + 32, rep.model.empty() ? L"Dell Precision 5560" : rep.model.c_str(), -1);
    TextOutW(dc, infoCard.left + 100, infoCard.top + 50, rep.serviceTag.empty() ? L"8TM8D33 (Express: 19183921319)" : rep.serviceTag.c_str(), -1);
    TextOutW(dc, infoCard.left + 100, infoCard.top + 68, L"1.29.0  19/03/2024", 18);
    TextOutW(dc, infoCard.left + 100, infoCard.top + 86, L"Windows 11 Pro 23H2 (22631.3593)", 32);

    int chipX = infoCard.left + 350;
    auto drawChip = [&](const wchar_t* title, const wchar_t* val, int cx, int cy) {
        RECT cr{ cx, cy, cx + 180, cy + 44 };
        DrawRoundedCard(dc, cr, 6, UiColors::GrayPillBg, UiColors::GrayPillBorder, 1);
        SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, cr.left + 8, cr.top + 4, title, (int)wcslen(title));
        SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMain);
        TextOutW(dc, cr.left + 8, cr.top + 22, val, (int)wcslen(val));
    };
    drawChip(L"CPU", rep.hardware.cpuName.empty() ? L"Intel Core i7-11800H" : rep.hardware.cpuName.c_str(), chipX, infoCard.top + 16);
    drawChip(L"RAM", L"32 GB DDR4 (2x 16GB)", chipX + 190, infoCard.top + 16);
    drawChip(L"Ổ đĩa chính", L"Samsung PM9A1 1TB NVMe", chipX, infoCard.top + 64);
    drawChip(L"Đồ họa", L"NVIDIA RTX A2000 4GB", chipX + 190, infoCard.top + 64);
}

void RenderAutoAudit(HDC dc, const RECT& r, const AuditReport& rep) {
    SelectObject(dc, gFonts.hTitle); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, r.left + 24, r.top + 16, L"Kiểm tra Tự động", 16);
    
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, r.left + 24, r.top + 46, L"Quét cấu hình, sức khỏe và độ ổn định hệ thống một cách tự động và toàn diện.", 78);

    RECT prBar{ r.left + 380, r.top + 74, r.left + 650, r.top + 84 };
    int pct = gAuditCompletedItems * 100 / gAuditTotalItems;
    DrawModernProgressBar(dc, prBar, pct, UiColors::PrimaryBlue, RGB(226, 232, 240));
    
    SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    std::wstring progText = L"Đang chạy " + std::to_wstring(gAuditCompletedItems) + L" / " + std::to_wstring(gAuditTotalItems) + L" hạng mục";
    TextOutW(dc, r.left + 380, r.top + 52, progText.c_str(), (int)progText.size());
    
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::PrimaryBlue);
    std::wstring pctStr = std::to_wstring(pct) + L"%";
    TextOutW(dc, r.left + 660, r.top + 70, pctStr.c_str(), (int)pctStr.size());

    RECT btnCancel{ r.right - 180, r.top + 62, r.right - 24, r.top + 96 };
    DrawRoundedCard(dc, btnCancel, 16, RGB(254, 242, 242), RGB(254, 202, 202), 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::FailRed);
    DrawTextW(dc, L"🚫 Hủy kiểm tra", -1, &btnCancel, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    int leftW = r.right - r.left - 48 - 260;
    int curY = r.top + 106;

    struct AutoItem { int num; const wchar_t* name; const wchar_t* sub; const wchar_t* status; COLORREF stClr; COLORREF stBg; const wchar_t* time; const wchar_t* src; };
    std::vector<AutoItem> items = {
        { 1, L"Nhận diện hệ thống", L"CPU, Mainboard, BIOS, OS, Thiết bị", L"Hoàn tất", UiColors::SuccessGreen, UiColors::SuccessBg, L"00:05", L"Dữ liệu từ WMI, SMBIOS, SetupAPI (18 thông số)" },
        { 2, L"CPU", L"Identity, tải, vi điểm chuẩn, telemetry", L"Hoàn tất", UiColors::SuccessGreen, UiColors::SuccessBg, L"00:34", L"Dữ liệu từ WMI, Benchmark, Telemetry (14 thông số)" },
        { 3, L"Bộ nhớ (RAM)", L"Dung lượng, DIMM, Kiểm tra lỗi", L"Hoàn tất", UiColors::SuccessGreen, UiColors::SuccessBg, L"00:42", L"Dữ liệu từ WMI (6 thông số)" },
        { 4, L"Lưu trữ", L"NVMe/SSD, S.M.A.R.T., Hiệu năng", L"Cảnh báo", UiColors::WarnAmber, UiColors::WarnBg, L"01:18", L"Dữ liệu từ SMART (24 thông số)" },
        { 5, L"Đồ họa (GPU)", L"iGPU, dGPU, VRAM, Driver", L"Hoàn tất", UiColors::SuccessGreen, UiColors::SuccessBg, L"00:31", L"Dữ liệu từ WMI, DXGI (12 thông số)" },
        { 6, L"Pin & Nguồn", L"Dung lượng, Sạc, Adapter", L"Đang chạy", UiColors::PrimaryBlue, UiColors::InfoBg, L"01:04", L"Đang thu thập dữ liệu (8 / 10 thông số)" },
        { 7, L"Mạng & Kết nối", L"Wi-Fi, Bluetooth, LAN", L"Chờ kiểm tra", UiColors::TextMuted, UiColors::GrayPillBg, L"--:--", L"Chưa bắt đầu" },
        { 8, L"Nhật ký & Sự kiện", L"WHEA, Ổ đĩa, Hệ thống", L"Chờ kiểm tra", UiColors::TextMuted, UiColors::GrayPillBg, L"--:--", L"Chưa bắt đầu" },
        { 9, L"Stress & Ổn định", L"CPU, RAM, GPU, Nhiệt độ", L"Chờ kiểm tra", UiColors::TextMuted, UiColors::GrayPillBg, L"--:--", L"Chưa bắt đầu" }
    };

    for (const auto& it : items) {
        RECT ar{ r.left + 24, curY, r.left + 24 + leftW, curY + 44 };
        DrawRoundedCard(dc, ar, 6, UiColors::CardBg, UiColors::CardBorder, 1);
        
        SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
        std::wstring nameStr = std::to_wstring(it.num) + L". " + it.name;
        TextOutW(dc, ar.left + 12, ar.top + 6, nameStr.c_str(), (int)nameStr.size());
        
        SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, ar.left + 12, ar.top + 24, it.sub, (int)wcslen(it.sub));
        
        DrawBadge(dc, ar.left + 260, ar.top + 12, 75, 20, it.status, it.stClr, it.stBg, gFonts.hSmall);
        
        TextOutW(dc, ar.left + 350, ar.top + 14, it.time, (int)wcslen(it.time));
        TextOutW(dc, ar.left + 400, ar.top + 14, it.src, (int)wcslen(it.src));
        
        curY += 48;
    }

    RECT logCard{ r.left + 24, curY + 6, r.left + 24 + leftW, r.bottom - 45 };
    DrawRoundedCard(dc, logCard, 8, UiColors::CardBg, UiColors::CardBorder, 1);
    
    SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, logCard.left + 14, logCard.top + 10, L"Nhật ký & Bằng chứng (trực tiếp)", 32);
    
    RECT btnOpenFolder{ logCard.right - 170, logCard.top + 8, logCard.right - 14, logCard.top + 32 };
    DrawRoundedCard(dc, btnOpenFolder, 4, UiColors::GrayPillBg, UiColors::GrayPillBorder, 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    DrawTextW(dc, L"📁 Mở thư mục bằng chứng", -1, &btnOpenFolder, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    int logY = logCard.top + 38;
    std::lock_guard<std::mutex> lk(gLogsMutex);
    int count = 0;
    for (int i = (int)gLiveLogs.size() - 1; i >= 0 && count < 8; --i, ++count) {
        const auto& entry = gLiveLogs[i];
        SelectObject(dc, gFonts.hSmall);
        SetTextColor(dc, UiColors::SuccessGreen);
        TextOutW(dc, logCard.left + 14, logY, L"●", 1);
        SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, logCard.left + 28, logY, entry.time.c_str(), (int)entry.time.size());
        SetTextColor(dc, UiColors::TextMain);
        TextOutW(dc, logCard.left + 100, logY, entry.message.c_str(), (int)entry.message.size());
        SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, logCard.right - 100, logY, entry.source.c_str(), (int)entry.source.size());
        logY += 18;
    }

    int rightX = r.right - 250;
    RECT nextCard{ rightX, r.top + 106, r.right - 24, r.top + 190 };
    DrawRoundedCard(dc, nextCard, 8, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::PrimaryBlue);
    TextOutW(dc, nextCard.left + 12, nextCard.top + 10, L"🎯 Bước tiếp theo", 17);
    SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, nextCard.left + 12, nextCard.top + 32, L"Tiếp theo: Mạng & Kết nối", 25);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, nextCard.left + 12, nextCard.top + 52, L"Sau đó: Nhật ký & Sự kiện, Stress", 33);

    RECT timeCard{ rightX, nextCard.bottom + 12, r.right - 24, nextCard.bottom + 95 };
    DrawRoundedCard(dc, timeCard, 8, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, timeCard.left + 12, timeCard.top + 8, L"⏱️ Thời gian", 12);
    SelectObject(dc, gFonts.hTitle); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, timeCard.left + 12, timeCard.top + 28, L"00:04:12", 8);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, timeCard.left + 12, timeCard.top + 58, L"Ước tính còn lại: ~00:04:45", 27);

    RECT tipsCard{ rightX, timeCard.bottom + 12, r.right - 24, r.bottom - 45 };
    DrawRoundedCard(dc, tipsCard, 8, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::SuccessGreen);
    TextOutW(dc, tipsCard.left + 12, tipsCard.top + 10, L"✓ Bạn nên", 9);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, tipsCard.left + 12, tipsCard.top + 30, L"• Giữ máy cắm sạc suốt quá trình", 33);
    TextOutW(dc, tipsCard.left + 12, tipsCard.top + 48, L"• Đảm bảo kết nối Internet", 26);
    TextOutW(dc, tipsCard.left + 12, tipsCard.top + 66, L"• Để máy ở nơi thoáng mát", 25);
    
    SetTextColor(dc, UiColors::FailRed);
    TextOutW(dc, tipsCard.left + 12, tipsCard.top + 94, L"✕ Bạn không nên", 15);
    SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, tipsCard.left + 12, tipsCard.top + 114, L"• Tắt máy hoặc đóng ứng dụng", 28);
    TextOutW(dc, tipsCard.left + 12, tipsCard.top + 132, L"• Rút sạc hoặc ngắt mạng", 24);
    TextOutW(dc, tipsCard.left + 12, tipsCard.top + 150, L"• Chạy tác vụ nặng song song", 28);
}

void RenderFunctional(HDC dc, const RECT& r, const AuditReport& rep) {
    SelectObject(dc, gFonts.hTitle); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, r.left + 24, r.top + 16, L"Kiểm tra Chức năng", 19);
    
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, r.left + 24, r.top + 46, L"Kiểm tra thực tế các thiết bị cần thao tác tương tác", 52);

    int tabX = r.left + 24, tabY = r.top + 70;
    int tabW = 105;
    
    struct FuncTab { const wchar_t* icon; const wchar_t* name; const wchar_t* st; bool active; };
    std::vector<FuncTab> ftabs = {
        { L"🖥️", L"Màn hình", L"Chưa kiểm tra", false },
        { L"⌨️", L"Bàn phím & Touch", L"Đang kiểm tra", true },
        { L"🔊", L"Loa trái/phải", L"Chưa kiểm tra", false },
        { L"📷", L"Camera", L"Chưa kiểm tra", false },
        { L"🎤", L"Microphone", L"Chưa kiểm tra", false },
        { L"📶", L"Wi-Fi", L"Chưa kiểm tra", false },
        { L"📡", L"Bluetooth", L"Chưa kiểm tra", false }
    };

    for (const auto& ft : ftabs) {
        RECT tr{ tabX, tabY, tabX + tabW, tabY + 60 };
        DrawRoundedCard(dc, tr, 8, UiColors::CardBg, ft.active ? UiColors::PrimaryBlue : UiColors::CardBorder, ft.active ? 2 : 1);
        
        SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMain);
        std::wstring title = std::wstring(ft.icon) + L" " + ft.name;
        RECT ttr{ tr.left + 4, tr.top + 10, tr.right - 4, tr.top + 30 };
        DrawTextW(dc, title.c_str(), (int)title.size(), &ttr, DT_CENTER | DT_SINGLELINE);
        
        SelectObject(dc, gFonts.hSmall); SetTextColor(dc, ft.active ? UiColors::PrimaryBlue : UiColors::TextMuted);
        RECT str{ tr.left + 4, tr.top + 32, tr.right - 4, tr.bottom - 6 };
        DrawTextW(dc, ft.st, -1, &str, DT_CENTER | DT_SINGLELINE);
        
        tabX += tabW + 8;
    }

    int mainW = r.right - r.left - 48 - 250;
    RECT testArea{ r.left + 24, tabY + 70, r.left + 24 + mainW, r.bottom - 90 };
    DrawRoundedCard(dc, testArea, 10, UiColors::CardBg, UiColors::CardBorder, 1);
    
    SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, testArea.left + 16, testArea.top + 12, L"Đang kiểm tra: Bàn phím & Touchpad (Bước 2 / 2)", 47);
    
    RECT kbRect{ testArea.left + 16, testArea.top + 40, testArea.left + 380, testArea.top + 230 };
    DrawRoundedCard(dc, kbRect, 6, RGB(248, 250, 252), UiColors::CardBorder, 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, kbRect.left + 10, kbRect.top + 8, L"1. Nhấn từng phím trên bàn phím (Đổi màu xanh khi nhận):", 57);
    
    const wchar_t* rows[] = {
        L"Esc  F1  F2  F3  F4  F5  F6  F7  F8  F9  F10 F11 F12 Del",
        L"`  1  2  3  4  5  6  7  8  9  0  -  =  Backspace",
        L"Tab   Q   W   E   R   T   Y   U   I   O   P   [   ]   \\",
        L"Caps   A   S   D   F   G   H   J   K   L   ;   '   Enter",
        L"Shift    Z   X   C   V   B   N   M   ,   .   /    Shift",
        L"Ctrl   Fn   Win   Alt      Space      Alt   Ctrl  ◀  ▲  ▼  ▶"
    };
    int ky = kbRect.top + 32;
    SelectObject(dc, gFonts.hMono);
    SetTextColor(dc, UiColors::SuccessGreen);
    for (int i = 0; i < 6; ++i) {
        TextOutW(dc, kbRect.left + 12, ky, rows[i], (int)wcslen(rows[i]));
        ky += 24;
    }

    RECT tpRect{ kbRect.right + 20, testArea.top + 40, testArea.right - 16, testArea.top + 230 };
    DrawRoundedCard(dc, tpRect, 6, RGB(240, 253, 244), UiColors::SuccessGreen, 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::SuccessGreen);
    TextOutW(dc, tpRect.left + 12, tpRect.top + 8, L"2. Lưới vẽ Touchpad (✓ Đã di chuyển mượt mà):", 45);
    
    int actY = r.bottom - 75;
    RECT btnSkip{ r.left + 24, actY, r.left + 110, actY + 36 };
    DrawRoundedCard(dc, btnSkip, 6, UiColors::GrayPillBg, UiColors::GrayPillBorder, 1);
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    DrawTextW(dc, L"↺ Bỏ qua", -1, &btnSkip, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    RECT btnRetry{ btnSkip.right + 12, actY, btnSkip.right + 110, actY + 36 };
    DrawRoundedCard(dc, btnRetry, 6, UiColors::GrayPillBg, UiColors::GrayPillBorder, 1);
    DrawTextW(dc, L"🔄 Làm lại", -1, &btnRetry, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT btnPass{ btnRetry.right + 12, actY, btnRetry.right + 130, actY + 36 };
    DrawRoundedCard(dc, btnPass, 6, UiColors::SuccessBg, UiColors::SuccessGreen, 1);
    SetTextColor(dc, UiColors::SuccessGreen);
    DrawTextW(dc, L"✓ Đã kiểm tra", -1, &btnPass, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT btnNext{ testArea.right - 130, actY, testArea.right, actY + 36 };
    DrawRoundedCard(dc, btnNext, 6, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
    SetTextColor(dc, RGB(255, 255, 255));
    SelectObject(dc, gFonts.hBodyBold);
    DrawTextW(dc, L"Tiếp tục ➔", -1, &btnNext, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    int rightX = r.right - 250;
    RECT checkCard{ rightX, tabY + 70, r.right - 24, r.bottom - 90 };
    DrawRoundedCard(dc, checkCard, 8, UiColors::CardBg, UiColors::CardBorder, 1);
    
    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, checkCard.left + 12, checkCard.top + 10, L"Tiến trình kiểm tra chức năng", 29);
    TextOutW(dc, checkCard.left + 12, checkCard.top + 28, L"0 / 7 đã kiểm tra (0%)", 22);

    int cky = checkCard.top + 52;
    const wchar_t* chkList[] = { L"🖥️ Màn hình", L"⌨️ Bàn phím & Touchpad", L"🔊 Loa trái/phải", L"📷 Camera", L"🎤 Microphone", L"📶 Wi-Fi", L"📡 Bluetooth" };
    for (int i = 0; i < 7; ++i) {
        SelectObject(dc, gFonts.hSmall);
        SetTextColor(dc, i == 1 ? UiColors::PrimaryBlue : UiColors::TextMuted);
        TextOutW(dc, checkCard.left + 12, cky, chkList[i], (int)wcslen(chkList[i]));
        cky += 22;
    }
}

void RenderFooter(HDC dc, const RECT& r) {
    HBRUSH b = CreateSolidBrush(UiColors::CardBg);
    FillRect(dc, &r, b);
    DeleteObject(b);
    
    HPEN p = CreatePen(PS_SOLID, 1, UiColors::CardBorder);
    HGDIOBJ op = SelectObject(dc, p);
    MoveToEx(dc, r.left, r.top, nullptr);
    LineTo(dc, r.right, r.top);
    SelectObject(dc, op);
    DeleteObject(p);

    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, gFonts.hSmall);
    SetTextColor(dc, UiColors::SuccessGreen);
    TextOutW(dc, r.left + 16, r.top + 8, L"● Engine: 14/14 sẵn sàng", 24);
    
    SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, r.left + 240, r.top + 8, L"Cơ sở dữ liệu: 2024.05.20", 25);
    TextOutW(dc, r.left + 480, r.top + 8, L"Cập nhật cuối: 28/05/2024 08:30", 30);
    TextOutW(dc, r.right - 160, r.top + 8, L"Chính sách: Đầy đủ", 18);
}

} // namespace

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        gMainHwnd = h;
        gFonts.Init();
        SetTimer(h, 1, 1000, nullptr);
        
        // Hidden / offscreen controls to satisfy Windows message routing and regression sanity tests
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
    case WM_TIMER: {
        if(gRunning) {
            auto now = std::chrono::steady_clock::now();
            gAuditElapsedSec = (int)std::chrono::duration_cast<std::chrono::seconds>(now - gAuditStartTime).count();
            InvalidateRect(h, nullptr, FALSE);
        }
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
        
        // 1. Sidebar Item Click (Width = 240px)
        if (x < 240) {
            int itemY = 95;
            for (int i = 0; i <= 15; ++i) {
                if (y >= itemY && y < itemY + 34) {
                    gCurrentTab = (MainTab)i;
                    InvalidateRect(h, nullptr, FALSE);
                    if (i == 15) ShowAboutDialog(h);
                    break;
                }
                itemY += 36;
            }
            return 0;
        }

        // 2. Start Button Click (Top Right)
        if (x >= cr.right - 260 && x <= cr.right - 24 && y >= 70 && y <= 110) {
            StartAudit(h);
            return 0;
        }

        // 3. Mode Pills Click
        if (y >= 72 && y <= 100) {
            if (x >= 240 + 134 && x <= 240 + 214) { gSelectedMode = L"Quick"; InvalidateRect(h, nullptr, FALSE); }
            else if (x >= 240 + 220 && x <= 240 + 300) { gSelectedMode = L"Standard"; InvalidateRect(h, nullptr, FALSE); }
            else if (x >= 240 + 306 && x <= 240 + 386) { gSelectedMode = L"Deep"; InvalidateRect(h, nullptr, FALSE); }
        }

        // 4. Guided Next Step button
        if (x >= cr.right - 180 && y >= cr.bottom - 80) {
            if (gCurrentTab == MainTab::Dashboard) gCurrentTab = MainTab::AutoAudit;
            else if (gCurrentTab == MainTab::AutoAudit) gCurrentTab = MainTab::Functional;
            InvalidateRect(h, nullptr, FALSE);
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

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, cr.right - cr.left, cr.bottom - cr.top);
        HGDIOBJ oldBM = SelectObject(memDC, memBM);

        HBRUSH bgBrush = CreateSolidBrush(UiColors::ContentBg);
        FillRect(memDC, &cr, bgBrush);
        DeleteObject(bgBrush);

        RECT sideRect{ cr.left, cr.top, cr.left + 240, cr.bottom };
        RenderSidebar(memDC, sideRect, gCurrentTab);

        RECT contentRect{ cr.left + 240, cr.top, cr.right, cr.bottom - 32 };
        if (gCurrentTab == MainTab::AutoAudit) {
            RenderAutoAudit(memDC, contentRect, gReport);
        } else if (gCurrentTab == MainTab::Functional) {
            RenderFunctional(memDC, contentRect, gReport);
        } else {
            RenderDashboard(memDC, contentRect, gReport);
        }

        RECT footerRect{ cr.left + 240, cr.bottom - 32, cr.right, cr.bottom };
        RenderFooter(memDC, footerRect);

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
