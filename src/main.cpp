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
#include "lap/cloud_lookup.h"
#include "lap/report.h"
#include "lap/session_history.h"
#include "lap/journal.h"
#include "resource.h"
#include "lap/scoring.h"
#include "lap/process.h"
#include "lap/hardware.h"
#include "lap/ui_theme.h"
#include "lap/ui_state.h"
#include "lap/ui_components.h"
#include "lap/ui_screens.h"

#pragma comment(lib,"Comctl32.lib")
using namespace lap;

namespace {
constexpr UINT WM_AUDIT_DONE = WM_APP + 1;
constexpr UINT WM_AUDIT_STATUS = WM_APP + 2;

AuditReport gReport;
std::wstring gDir, gReportPath, gReportOutputDir;
HWND gMainHwnd = nullptr;
HWND gList = nullptr, gStatus = nullptr, gBtn = nullptr, gOpen = nullptr, gMode = nullptr;
HWND gFuncDisplay = nullptr, gFuncKeyboard = nullptr, gFuncTouch = nullptr, gFuncSpeaker = nullptr;
HWND gFuncUsb = nullptr, gFuncIo = nullptr, gPhysical = nullptr, gSeller = nullptr, gPortTest = nullptr, gNext = nullptr, gProgress = nullptr;

std::thread gWorker;
std::atomic_bool gCancel{false}, gRunning{false}, gPaused{false}, gAuditReady{false}, gCloseRequested{false};
CanonicalUiState gSessionLifecycleState{CanonicalUiState::Idle};
std::wstring gSelectedMode = L"Standard";
std::mutex gReportMutex, gLogsMutex;
std::vector<LiveLogEntry> gLiveLogs;
std::chrono::steady_clock::time_point gAuditStartTime;
int gAuditElapsedSec = 0;
int gAuditTotalItems = 9;
std::atomic<int> gAuditCompletedItems{0};
std::atomic<int> gAuditCurrentStage{0};

MainTab gCurrentTab = MainTab::Dashboard;
bool gDeviceGroupExpanded = true;
int gSidebarScrollOffset = 0;
int gTableScrollOffset = 0;
int gFocusIndex = 0; // 0: Sidebar, 1: Mode Pills, 2: Primary CTA, 3: Next CTA
int gHistorySelectedIndex = 0;
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

const wchar_t* GetCurrentStageName(int stage) {
    switch (stage) {
    case 1: return L"Nhận diện hệ thống";
    case 2: return L"CPU & Nhận diện";
    case 3: return L"Bộ nhớ (RAM)";
    case 4: return L"Lưu trữ (Storage / SMART)";
    case 5: return L"Đồ họa (GPU & DXGI)";
    case 6: return L"Pin & Nguồn";
    case 7: return L"Mạng & Kết nối";
    case 8: return L"Nhật ký & Forensics";
    case 9: return L"Stress & Ổn định";
    default: return L"Kiểm tra hệ thống";
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
    auto pl = LoadFactoryProfile(gDir + L"\\profiles", model, tag);
    if (!pl.loaded && !tag.empty()) {
        auto vendor = Reg(L"SystemManufacturer");
        auto cloudRes = LookupFactoryProfileOnline(gDir, vendor, model, tag, 1500);
        if (cloudRes.success) {
            pl.loaded = true;
            pl.exact = true;
            pl.profile = cloudRes.profile;
            pl.source = cloudRes.source;
        }
    }
    FactoryProfile profile = pl.loaded ? pl.profile : FactoryProfile{};
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
    auto out = gReportOutputDir;
    if (out.empty()) {
        auto caps = DetectCapabilities(gDir);
        out = ResolveReportDirectory(gDir, caps.winPE);
        gReportOutputDir = out;
        InitializeSessionHistory(out);
    }
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

CanonicalUiState GetFunctionalItemUiState(const FunctionalTestSummary& f, const std::wstring& id) {
    for (const auto& it : f.items) {
        if (it.id == id) return MapFunctionalStatus(it.status);
    }
    return CanonicalUiState::NotTested;
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
    gCancel = false;
    gAuditCurrentStage = 1;
    gAuditCompletedItems = 0;
    gSessionLifecycleState = CanonicalUiState::Running;

    auto syncToGlobal = [&](const AuditReport& cur) {
        std::lock_guard<std::mutex> lk(gReportMutex);
        gReport = cur;
    };

    auto checkPause = [&]() {
        while (gPaused && !gCancel) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };

    checkPause();
    PostStatus(h, L"Đang nhận diện hệ điều hành và cấu hình BIOS...");
    auto caps = DetectCapabilities(gDir);
    auto model = Reg(L"SystemProductName");
    auto tag = ServiceTag(caps, &gCancel);
    auto pl = LoadFactoryProfile(gDir + L"\\profiles", model, tag);
    if (!pl.loaded && !tag.empty()) {
        auto vendor = Reg(L"SystemManufacturer");
        auto cloudRes = LookupFactoryProfileOnline(gDir, vendor, model, tag, 1500);
        if (cloudRes.success) {
            pl.loaded = true;
            pl.exact = true;
            pl.profile = cloudRes.profile;
            pl.source = cloudRes.source;
        }
    }
    FactoryProfile profile = pl.loaded ? pl.profile : FactoryProfile{};
    
    auto report = CollectInventory(profile, caps, gDir, &gCancel);
    report.profileSource = pl.source;
    report.factoryExact = pl.exact;
    report.genericMode = (pl.loaded && !pl.exact);
    syncToGlobal(report);
    gAuditCompletedItems = 1;
    PostStatus(h, report.model.empty()
        ? L"Đã hoàn tất thu thập nhận diện; model hệ thống chưa xác định."
        : L"Đã nhận diện hệ thống: " + report.model);
    
    checkPause();
    if (!gCancel) {
        gAuditCurrentStage = 2;
        PostStatus(h, L"Đang thu thập danh tính và thông tin CPU...");
        syncToGlobal(report);
        gAuditCompletedItems = 2;
        PostStatus(h, report.hardware.cpuName.empty()
            ? L"Đã hoàn tất thu thập CPU; chưa xác định được tên bộ xử lý."
            : L"Đã nhận diện CPU: " + report.hardware.cpuName);
    }
    checkPause();
    if (!gCancel) {
        gAuditCurrentStage = 3;
        PostStatus(h, L"Đang đọc chi tiết các thanh nhớ RAM...");
        syncToGlobal(report);
        gAuditCompletedItems = 3;
        std::wstring ramGb = (report.hardware.installedRamBytes > 0) ? (std::to_wstring(report.hardware.installedRamBytes / (1024*1024*1024)) + L" GB") : L"—";
        PostStatus(h, L"Đã nhận diện RAM: " + ramGb);
    }
    checkPause();
    if (!gCancel) {
        gAuditCurrentStage = 4;
        PostStatus(h, L"Đang đọc dữ liệu SMART & NVMe cho từng ổ đĩa...");
        CollectWindowsStorageReliability(report, caps, &gCancel);
        CollectSmartctl(report, profile, caps, gDir, &gCancel);
        CollectVolumeIntegrityAudit(report);
        syncToGlobal(report);
        gAuditCompletedItems = 4;
        PostStatus(h, L"Đã quét xong ổ đĩa lưu trữ (" + std::to_wstring(report.hardware.storage.size()) + L" ổ đĩa)");
    }
    checkPause();
    if (!gCancel) {
        gAuditCurrentStage = 5;
        PostStatus(h, L"Đang đọc thông tin GPU và Driver đồ họa...");
        CollectNvidia(report, profile, caps, gDir, &gCancel);
        syncToGlobal(report);
        gAuditCompletedItems = 5;
        PostStatus(h, L"Đã quét xong GPU (" + std::to_wstring(report.hardware.gpus.size()) + L" GPU)");
    }
    checkPause();
    if (!gCancel) {
        gAuditCurrentStage = 6;
        PostStatus(h, L"Đang đo thông số Pin, nguồn sạc và công suất xả...");
        CollectBatteryDischargeAudit(report, caps, &gCancel);
        CollectPortPowerBaseline(report);
        syncToGlobal(report);
        gAuditCompletedItems = 6;
        PostStatus(h, L"Hoàn tất kiểm tra thông số Pin & Nguồn");
    }
    checkPause();
    if (!gCancel) {
        gAuditCurrentStage = 7;
        PostStatus(h, L"Đang nhận diện adapter Wi-Fi, Bluetooth, Ethernet và controller kết nối...");
        CollectFunctionalPresence(report, caps, &gCancel);
        report.hardware.stress.chassisProfile = LoadChassisProfile(gDir, report.model);
        syncToGlobal(report);
        gAuditCompletedItems = 7;
        PostStatus(h, L"Đã thu thập nhận diện Mạng & Kết nối; chức năng thực tế vẫn cần kiểm tra riêng.");
    }
    checkPause();
    if (!gCancel) {
        gAuditCurrentStage = 8;
        PostStatus(h, L"Đang quét nhật ký hệ thống & mã lỗi phần cứng WHEA...");
        CollectPlatformForensics(report, profile, caps, gDir, &gCancel);
        syncToGlobal(report);
        gAuditCompletedItems = 8;
        PostStatus(h, L"Hoàn tất quét nhật ký sự kiện & Forensics");
    }
    checkPause();
    if (!gCancel) {
        gAuditCurrentStage = 9;
        PostStatus(h, L"Chạy bài kiểm tra độ ổn định Stress (" + gSelectedMode + L")...");
        RunStressSession(report, caps, gDir, MakeStressPlan(gSelectedMode), &gCancel);
        RunRuntimeValidation(report, caps, gDir);
        report.hardware.stress.decision = BuildAuditDecision(report);
        BuildOrchestrator(report, false, true);
        syncToGlobal(report);
        gAuditCompletedItems = 9;
        PostStatus(h, L"Hoàn tất bài kiểm tra độ ổn định Stress (" + gSelectedMode + L")");
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
    gAuditReady = !gCancel;
    gRunning = false;
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
        gPaused = false;
        gSessionLifecycleState = CanonicalUiState::Cancelled;
        PostStatus(h, L"Đang dừng kiểm tra...");
        return;
    }
    if (gWorker.joinable()) gWorker.join();
    gRunning = true; gCancel = false; gPaused = false; gAuditReady = false;
    gSessionLifecycleState = CanonicalUiState::Running;
    gAuditStartTime = std::chrono::steady_clock::now();
    gAuditCurrentStage = 1;
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

int gInspectionPurpose = 0; // 0: Mua laptop đã qua sử dụng, 1: Bảo hành & Sửa chữa, 2: Định giá & Bàn giao
std::wstring gInspectorName = L"Kiểm định viên LapSure";

// ----------------------------------------------------
// UI Renderers Composing C01–C12 Reusable Components
// ----------------------------------------------------

void RenderNewSession(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    (void)rep;
    // 1. C03 Page Header
    PageHeaderConfig hdr;
    hdr.title = L"Phiên kiểm định mới";
    hdr.subtitle = L"Khởi tạo ngữ cảnh kiểm định, chọn mục đích và cấu hình các thông số ban đầu trước khi thu thập bằng chứng.";
    hdr.sessionTag = L"Chuẩn bị kiểm định";
    hdr.sessionState = CanonicalUiState::Ready;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    // Layout division: Main configuration on left, Preflight & Action on right
    int rightPanelW = UiMetrics::Scale(300, dpi);
    int leftW = r.right - r.left - UiMetrics::Scale(48, dpi) - rightPanelW;
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Section 1: Inspection Purpose (S02-O01)
    SelectObject(dc, gFonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, r.left + UiMetrics::Scale(24, dpi), curY, L"1. Chọn mục đích kiểm định", 26);
    curY += UiMetrics::Scale(24, dpi);

    struct PurposeItem { int id; const wchar_t* title; const wchar_t* desc; const wchar_t* tag; };
    std::vector<PurposeItem> purposes = {
        { 0, L"🛒 Mua laptop đã qua sử dụng", L"Tập trung kiểm tra độ chai pin, màn hình điểm chết, cổng kết nối, phát hiện linh kiện thay thế và đối chiếu cam kết người bán.", L"Khuyến nghị cho người mua" },
        { 1, L"🛠️ Bảo hành & Kiểm tra lỗi", L"Tập trung kiểm tra tải nặng (Stress CPU/GPU), đo nhiệt độ tản nhiệt, quét lỗi driver PnP Yellow Bang và sự kiện WHEA crash.", L"Dành cho kỹ thuật viên" },
        { 2, L"📊 Định giá & Bàn giao thiết bị", L"Quét toàn bộ cấu hình gốc nhà máy, benchmark vi điểm chuẩn CPU, đo công suất xả pin mW và xuất báo cáo có chữ ký số.", L"Bàn giao / Doanh nghiệp" }
    };

    for (const auto& p : purposes) {
        bool selected = (gInspectionPurpose == p.id);
        RECT pr{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW, curY + UiMetrics::Scale(64, dpi) };
        COLORREF bg = selected ? RGB(238, 242, 255) : UiColors::CardBg;
        COLORREF border = selected ? UiColors::PrimaryBlue : UiColors::CardBorder;
        int bw = selected ? 2 : 1;
        DrawRoundedCard(dc, pr, UiMetrics::RadiusMd, bg, border, bw);

        SelectObject(dc, gFonts.hBodyBold);
        SetTextColor(dc, selected ? UiColors::PrimaryBlue : UiColors::TextMain);
        TextOutW(dc, pr.left + UiMetrics::Scale(14, dpi), pr.top + UiMetrics::Scale(10, dpi), p.title, (int)wcslen(p.title));

        if (selected) {
            DrawBadge(dc, pr.right - UiMetrics::Scale(170, dpi), pr.top + UiMetrics::Scale(10, dpi), UiMetrics::Scale(156, dpi), UiMetrics::Scale(20, dpi), p.tag, UiColors::PrimaryBlue, RGB(224, 231, 255), gFonts.hSmall);
        }

        SelectObject(dc, gFonts.hSmall);
        SetTextColor(dc, UiColors::TextMuted);
        RECT descRect{ pr.left + UiMetrics::Scale(14, dpi), pr.top + UiMetrics::Scale(32, dpi), pr.right - UiMetrics::Scale(14, dpi), pr.bottom - UiMetrics::Scale(6, dpi) };
        DrawTextW(dc, p.desc, (int)wcslen(p.desc), &descRect, DT_LEFT | DT_WORDBREAK);

        curY += UiMetrics::Scale(72, dpi);
    }

    curY += UiMetrics::Scale(8, dpi);

    // Section 2: Inspection Mode Selection (S02-O02)
    SelectObject(dc, gFonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, r.left + UiMetrics::Scale(24, dpi), curY, L"2. Chọn cường độ kiểm tra", 25);
    curY += UiMetrics::Scale(24, dpi);

    struct ModeItem { const wchar_t* mode; const wchar_t* name; const wchar_t* time; const wchar_t* desc; };
    std::vector<ModeItem> modes = {
        { L"Quick", L"⚡ Nhanh", L"~2-3 phút", L"Quét nhanh thông số phần cứng, dung lượng pin và SMART ổ đĩa. Thích hợp kiểm tra sơ bộ tại chỗ." },
        { L"Standard", L"🛡️ Tiêu chuẩn", L"~5-8 phút", L"Quét toàn bộ phần cứng, kiểm tra độ ổn định nhiệt độ CPU/GPU 60s, kiểm tra hệ thống tệp và đối chiếu nhà máy." },
        { L"Deep", L"🔬 Chuyên sâu", L"~15-20 phút", L"Kiểm tra tải nặng đa nhân, giám sát throttling chi tiết, kiểm tra xả pin liên tục và kiểm tra toàn diện pháp y." }
    };

    int modeCardW = (leftW - UiMetrics::Scale(16, dpi)) / 3;
    for (size_t i = 0; i < modes.size(); ++i) {
        bool selected = (gSelectedMode == modes[i].mode);
        int mx = r.left + UiMetrics::Scale(24, dpi) + (int)i * (modeCardW + UiMetrics::Scale(8, dpi));
        RECT mr{ mx, curY, mx + modeCardW, curY + UiMetrics::Scale(110, dpi) };
        COLORREF bg = selected ? RGB(238, 242, 255) : UiColors::CardBg;
        COLORREF border = selected ? UiColors::PrimaryBlue : UiColors::CardBorder;
        int bw = selected ? 2 : 1;
        DrawRoundedCard(dc, mr, UiMetrics::RadiusMd, bg, border, bw);

        SelectObject(dc, gFonts.hBodyBold);
        SetTextColor(dc, selected ? UiColors::PrimaryBlue : UiColors::TextMain);
        TextOutW(dc, mr.left + UiMetrics::Scale(12, dpi), mr.top + UiMetrics::Scale(10, dpi), modes[i].name, (int)wcslen(modes[i].name));

        SelectObject(dc, gFonts.hSmall);
        SetTextColor(dc, selected ? UiColors::PrimaryBlue : UiColors::TextMuted);
        TextOutW(dc, mr.left + UiMetrics::Scale(12, dpi), mr.top + UiMetrics::Scale(30, dpi), modes[i].time, (int)wcslen(modes[i].time));

        RECT descR{ mr.left + UiMetrics::Scale(12, dpi), mr.top + UiMetrics::Scale(50, dpi), mr.right - UiMetrics::Scale(12, dpi), mr.bottom - UiMetrics::Scale(8, dpi) };
        DrawTextW(dc, modes[i].desc, (int)wcslen(modes[i].desc), &descR, DT_LEFT | DT_WORDBREAK);
    }

    // Right Panel: Preflight Environment Readiness & Primary CTA (S02-O05, C10)
    int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    int rightY = r.top + UiMetrics::Scale(70, dpi);

    // Preflight Card
    RECT preflightCard{ rightX, rightY, r.right - UiMetrics::Scale(24, dpi), rightY + UiMetrics::Scale(220, dpi) };
    DrawRoundedCard(dc, preflightCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    SelectObject(dc, gFonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, preflightCard.left + UiMetrics::Scale(14, dpi), preflightCard.top + UiMetrics::Scale(12, dpi), L"Môi trường & Sẵn sàng", 22);

    auto caps = DetectCapabilities(gDir);
    int itemY = preflightCard.top + UiMetrics::Scale(38, dpi);

    auto drawPreflightRow = [&](const wchar_t* label, CanonicalUiState state, const wchar_t* detail) {
        SelectObject(dc, gFonts.hSmall);
        SetTextColor(dc, UiColors::TextMain);
        TextOutW(dc, preflightCard.left + UiMetrics::Scale(14, dpi), itemY, label, (int)wcslen(label));

        int bw = UiMetrics::Scale(90, dpi);
        int bh = UiMetrics::Scale(20, dpi);
        DrawStatusBadge(dc, preflightCard.right - bw - UiMetrics::Scale(14, dpi), itemY - UiMetrics::Scale(2, dpi), bw, bh, state, gFonts);

        SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, preflightCard.left + UiMetrics::Scale(14, dpi), itemY + UiMetrics::Scale(16, dpi), detail, (int)wcslen(detail));
        itemY += UiMetrics::Scale(36, dpi);
    };

    drawPreflightRow(L"Quyền Quản trị viên (Admin)", !caps.winPE ? CanonicalUiState::Pass : CanonicalUiState::Warning, !caps.winPE ? L"Đầy đủ quyền truy cập phần cứng" : L"WinPE Cứu hộ");
    drawPreflightRow(L"Môi trường Hệ điều hành", CanonicalUiState::Pass, caps.winPE ? L"Windows Preinstallation (WinPE)" : L"Windows 64-bit Native");
    drawPreflightRow(L"Provider hệ thống", caps.wmi ? CanonicalUiState::Info : CanonicalUiState::Warning,
                     caps.wmi ? L"WMI/CIM khả dụng; provider tùy chọn được xác minh khi chạy"
                              : L"WMI/CIM không khả dụng; một số bằng chứng có thể bị thiếu");
    drawPreflightRow(L"Hồ sơ Chassis", CanonicalUiState::Info,
                     L"Hồ sơ phù hợp sẽ được nạp theo model/Service Tag khi kiểm định");

    // Action Panel: Start Inspection Button
    RECT actionCard{ rightX, preflightCard.bottom + UiMetrics::Scale(12, dpi), r.right - UiMetrics::Scale(24, dpi), preflightCard.bottom + UiMetrics::Scale(180, dpi) };
    NextActionConfig nac;
    nac.actionTitle = L"Bắt đầu quy trình kiểm định";
    nac.reasonText = L"Khởi chạy thu thập bằng chứng chẩn đoán theo chế độ đã chọn.";
    nac.remainingTasks = { L"Bước 1: Quét tự động & Stress", L"Bước 2: Wizard kiểm tra chức năng", L"Bước 3: Tổng hợp báo cáo & kết luận" };
    nac.buttonText = L"BẮT ĐẦU PHIÊN KIỂM ĐỊNH";
    nac.isButtonEnabled = true;
    DrawNextActionPanel(dc, actionCard, nac, gFonts, dpi);
}

void RenderDashboard(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    // 1. C03 Page Header
    PageHeaderConfig hdr;
    hdr.title = L"Tổng quan & Bảng điều khiển";
    hdr.subtitle = L"Trạng thái hoạt động, mức độ bao phủ bằng chứng và chỉ số phần cứng tổng thể của máy.";
    if (gAuditReady) {
        hdr.sessionTag = FormatDecisionVi(rep.hardware.stress.decision.overall);
        if (rep.hardware.stress.decision.overall == L"BUY") hdr.sessionState = CanonicalUiState::Good;
        else if (rep.hardware.stress.decision.overall == L"BUY WITH NOTES") hdr.sessionState = CanonicalUiState::Warning;
        else if (rep.hardware.stress.decision.overall == L"REJECT") hdr.sessionState = CanonicalUiState::Fail;
        else hdr.sessionState = CanonicalUiState::Incomplete;
    } else {
        hdr.sessionTag = L"Chưa kiểm định";
        hdr.sessionState = CanonicalUiState::Idle;
    }
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    // 2. Mode Selector Bar
    int modeX = r.left + UiMetrics::Scale(24, dpi);
    int modeY = r.top + UiMetrics::Scale(70, dpi);
    SelectObject(dc, gFonts.hSmall);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, modeX, modeY + UiMetrics::Scale(6, dpi), L"Chế độ kiểm tra:", 16);
    modeX += UiMetrics::Scale(110, dpi);

    auto drawPill = [&](const wchar_t* label, bool active, int pillIdx) {
        (void)pillIdx;
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
            else if (f.severity == Severity::Major || f.severity == Severity::Minor || f.state == State::Warning) warnCount++;
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
    DrawTextW(dc, L"Xem chi tiết đối chiếu", -1, &btnProfileDetail, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // 5. Domain Grid (12 Cards - 3 Columns x 4 Rows, spacious layout, zero clipping)
    int gridY = kpiY + kpiH + UiMetrics::Scale(12, dpi);
    int gridCols = 3;
    int gridW = (mainContentW - UiMetrics::Scale((gridCols - 1) * 10, dpi)) / gridCols;
    int gridH = UiMetrics::Scale(64, dpi);

    // Dynamic domain evaluation (strictly bound to individual domain evidence)
    auto& f = rep.hardware.stress.functional;
    CanonicalUiState stSystem = (!rep.model.empty()) ? CanonicalUiState::Pass : CanonicalUiState::NotTested;
    CanonicalUiState stRam = (rep.hardware.installedRamBytes > 0) ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    CanonicalUiState stStorage = (!rep.hardware.storage.empty()) ? ((!rep.hardware.storage.front().smartPassed && rep.hardware.storage.front().smartReadable) ? CanonicalUiState::Fail : CanonicalUiState::Good) : CanonicalUiState::NotTested;
    CanonicalUiState stBattery = (rep.hardware.battery.present && rep.hardware.battery.healthPercent > 0) ? ((rep.hardware.battery.healthPercent < 50) ? CanonicalUiState::Warning : CanonicalUiState::Good) : (rep.hardware.battery.present ? CanonicalUiState::NotTested : CanonicalUiState::Unsupported);
    CanonicalUiState stGpu = (!rep.hardware.gpus.empty()) ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    CanonicalUiState stDisplay = GetFunctionalItemUiState(f, L"display_visual");
    CanonicalUiState stKeyboard = GetFunctionalItemUiState(f, L"keyboard_matrix");
    CanonicalUiState stAudio = GetFunctionalItemUiState(f, L"audio_stereo");
    CanonicalUiState stNetwork = GetFunctionalItemUiState(f, L"wifi_scan");
    CanonicalUiState stPorts = (rep.hardware.stress.portPower.overall == L"PASS") ? CanonicalUiState::Good : ((rep.hardware.stress.portPower.overall == L"FAIL") ? CanonicalUiState::Fail : CanonicalUiState::NotTested);
    CanonicalUiState stStress = (rep.hardware.stress.completed) ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    long long totalCritEvents = rep.hardware.events.whea + rep.hardware.events.kernelPower + rep.hardware.events.bugCheck;
    CanonicalUiState stEvents = gAuditReady ? ((totalCritEvents > 0) ? CanonicalUiState::Warning : CanonicalUiState::Good) : CanonicalUiState::NotTested;

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
        int gx = r.left + UiMetrics::Scale(24, dpi) + col * (gridW + UiMetrics::Scale(10, dpi));
        int gy = gridY + row * (gridH + UiMetrics::Scale(8, dpi));
        RECT gr{ gx, gy, gx + gridW, gy + gridH };
        DrawRoundedCard(dc, gr, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

        SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
        std::wstring nameStr = std::wstring(domains[i].icon) + L" " + domains[i].name;
        TextOutW(dc, gr.left + UiMetrics::Scale(12, dpi), gr.top + UiMetrics::Scale(8, dpi), nameStr.c_str(), (int)nameStr.size());

        int bw = UiMetrics::Scale(85, dpi);
        int bh = UiMetrics::Scale(20, dpi);
        DrawStatusBadge(dc, gr.right - bw - UiMetrics::Scale(10, dpi), gr.top + UiMetrics::Scale(8, dpi), bw, bh, domains[i].state, gFonts);

        SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, gr.left + UiMetrics::Scale(12, dpi), gr.top + UiMetrics::Scale(32, dpi), domains[i].desc, (int)wcslen(domains[i].desc));
    }

    // 6. Hardware Specifications Summary Bar (C08 4-Chip Bar)
    int specY = gridY + (gridH + UiMetrics::Scale(8, dpi)) * 4 + UiMetrics::Scale(4, dpi);
    RECT infoCard{ r.left + UiMetrics::Scale(24, dpi), specY, r.left + UiMetrics::Scale(24, dpi) + mainContentW, specY + UiMetrics::Scale(82, dpi) };
    DrawRoundedCard(dc, infoCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, infoCard.left + UiMetrics::Scale(14, dpi), infoCard.top + UiMetrics::Scale(8, dpi), L"Cấu hình phần cứng nhận diện", 28);

    int chipCount = 4;
    int chipGap = UiMetrics::Scale(8, dpi);
    int totalInsideW = (infoCard.right - infoCard.left) - UiMetrics::Scale(28, dpi);
    int chipW = (totalInsideW - (chipCount - 1) * chipGap) / chipCount;
    int chipH = UiMetrics::Scale(40, dpi);
    int chipY = infoCard.top + UiMetrics::Scale(32, dpi);

    auto drawChip = [&](const std::wstring& k, const std::wstring& v, int cx) {
        RECT cr{ cx, chipY, cx + chipW, chipY + chipH };
        DrawRoundedCard(dc, cr, UiMetrics::RadiusSm, RGB(248, 250, 252), UiColors::CardBorder, 1);
        SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, cr.left + UiMetrics::Scale(8, dpi), cr.top + UiMetrics::Scale(3, dpi), k.c_str(), (int)k.size());
        SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
        std::wstring shortV = v.empty() ? L"—" : v;
        if (shortV.size() > 28) shortV = shortV.substr(0, 26) + L"...";
        TextOutW(dc, cr.left + UiMetrics::Scale(8, dpi), cr.top + UiMetrics::Scale(18, dpi), shortV.c_str(), (int)shortV.size());
    };

    std::wstring sysModel = rep.model.empty() ? Reg(L"SystemProductName") : rep.model;
    if (sysModel.empty()) sysModel = L"Laptop Windows x64";
    std::wstring sysCpu = rep.hardware.cpuName.empty() ? Reg(L"ProcessorNameString") : rep.hardware.cpuName;
    if (sysCpu.empty()) sysCpu = L"CPU Multi-Core x64";
    std::wstring ramStr = (rep.hardware.installedRamBytes > 0) ? (std::to_wstring(rep.hardware.installedRamBytes / (1024 * 1024 * 1024)) + L" GB") : L"—";
    std::wstring ssdStr = rep.hardware.storage.empty() ? L"NVMe / SSD" : rep.hardware.storage.front().model;

    int curChipX = infoCard.left + UiMetrics::Scale(14, dpi);
    drawChip(L"🖥️ Model máy", sysModel, curChipX);
    curChipX += chipW + chipGap;
    drawChip(L"⚡ Vi xử lý (CPU)", sysCpu, curChipX);
    curChipX += chipW + chipGap;
    drawChip(L"🧠 Bộ nhớ (RAM)", ramStr, curChipX);
    curChipX += chipW + chipGap;
    drawChip(L"💾 Ổ đĩa lưu trữ", ssdStr, curChipX);
}

void RenderAutoAudit(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Kiểm tra Tự động";
    hdr.subtitle = L"Quét cấu hình, sức khỏe và độ ổn định hệ thống một cách tự động và toàn diện.";
    hdr.sessionState = gRunning ? CanonicalUiState::Running : (gAuditReady ? CanonicalUiState::Pass : gSessionLifecycleState);
    hdr.sessionTag = gRunning ? (L"Đang chạy bước " + std::to_wstring(gAuditCurrentStage.load()) + L"/9") : (gAuditReady ? L"Hoàn tất" : ((gSessionLifecycleState == CanonicalUiState::Cancelled) ? L"Đã hủy" : L"Chưa bắt đầu"));
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int rightPanelW = UiMetrics::Scale(250, dpi);
    int leftW = r.right - r.left - UiMetrics::Scale(48, dpi) - rightPanelW;
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Progress Banner Card (C06)
    RECT prCard{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW, curY + UiMetrics::Scale(58, dpi) };
    DrawRoundedCard(dc, prCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    ProgressCoverageConfig pcc;
    if (gRunning) {
        pcc.label = L"Đang chạy: " + std::wstring(GetCurrentStageName(gAuditCurrentStage.load()));
        pcc.barColor = UiColors::PrimaryBlue;
    } else if (gAuditReady) {
        pcc.label = L"Hoàn thành toàn bộ quy trình quét tự động";
        pcc.barColor = UiColors::SuccessGreen;
    } else {
        pcc.label = L"Tiến trình kiểm tra tự động";
        pcc.barColor = UiColors::TextMuted;
    }
    pcc.completed = gAuditCompletedItems.load();
    pcc.total = gAuditTotalItems;
    RECT pccRect{ prCard.left + UiMetrics::Scale(12, dpi), prCard.top + UiMetrics::Scale(8, dpi), prCard.right - UiMetrics::Scale(130, dpi), prCard.top + UiMetrics::Scale(48, dpi) };
    DrawProgressCoverage(dc, pccRect, pcc, gFonts, dpi);

    // Cancel Button / Timer inside Progress Card
    if (gRunning) {
        RECT btnCancel{ prCard.right - UiMetrics::Scale(115, dpi), prCard.top + UiMetrics::Scale(14, dpi), prCard.right - UiMetrics::Scale(12, dpi), prCard.bottom - UiMetrics::Scale(14, dpi) };
        DrawRoundedCard(dc, btnCancel, UiMetrics::RadiusPill, RGB(254, 242, 242), RGB(254, 202, 202), 1);
        SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::FailRed);
        DrawTextW(dc, L"🚫 Hủy", -1, &btnCancel, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    } else if (gAuditReady) {
        wchar_t timeBuf[32]; swprintf_s(timeBuf, L"Thời gian: %02d:%02d", gAuditElapsedSec / 60, gAuditElapsedSec % 60);
        SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::SuccessGreen);
        RECT tr{ prCard.right - UiMetrics::Scale(125, dpi), prCard.top + UiMetrics::Scale(16, dpi), prCard.right - UiMetrics::Scale(12, dpi), prCard.bottom - UiMetrics::Scale(16, dpi) };
        DrawTextW(dc, timeBuf, -1, &tr, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    curY += UiMetrics::Scale(66, dpi);

    // 9 Audit Items List
    auto getItemState = [&](int num) -> CanonicalUiState {
        if (gAuditCompletedItems.load() >= num) {
            switch (num) {
            case 1: return (!rep.model.empty()) ? CanonicalUiState::Pass : CanonicalUiState::Warning;
            case 2: return (!rep.hardware.cpuName.empty()) ? CanonicalUiState::Pass : CanonicalUiState::Warning;
            case 3: return (rep.hardware.installedRamBytes > 0) ? CanonicalUiState::Pass : CanonicalUiState::Warning;
            case 4: {
                if (rep.hardware.storage.empty()) return CanonicalUiState::Warning;
                for (const auto& s : rep.hardware.storage) {
                    if (s.smartReadable && !s.smartPassed) return CanonicalUiState::Fail;
                }
                return CanonicalUiState::Good;
            }
            case 5: return (!rep.hardware.gpus.empty()) ? CanonicalUiState::Pass : CanonicalUiState::Warning;
            case 6: return rep.hardware.battery.present ? (rep.hardware.battery.healthPercent > 0 && rep.hardware.battery.healthPercent < 50 ? CanonicalUiState::Warning : CanonicalUiState::Good) : CanonicalUiState::Pass;
            case 7: return CanonicalUiState::Pass;
            case 8: {
                long long crit = rep.hardware.events.whea + rep.hardware.events.kernelPower + rep.hardware.events.bugCheck;
                return (crit > 0) ? CanonicalUiState::Warning : CanonicalUiState::Good;
            }
            case 9: return rep.hardware.stress.completed ? CanonicalUiState::Good : (gRunning ? CanonicalUiState::Running : CanonicalUiState::NotTested);
            default: return CanonicalUiState::Good;
            }
        } else if (gRunning && gAuditCurrentStage.load() == num) {
            return CanonicalUiState::Running;
        }
        return CanonicalUiState::NotTested;
    };

    struct AutoItem { int num; const wchar_t* name; const wchar_t* sub; const wchar_t* src; };
    std::vector<AutoItem> items = {
        { 1, L"Nhận diện hệ thống", L"CPU, Mainboard, BIOS, OS, Thiết bị", L"WMI, SMBIOS, SetupAPI" },
        { 2, L"CPU & Microbench", L"Identity, vi điểm chuẩn, telemetry", L"WMI, Telemetry" },
        { 3, L"Bộ nhớ (RAM)", L"Dung lượng, DIMM, Kiểm tra lỗi", L"WMI, CIM" },
        { 4, L"Lưu trữ", L"NVMe/SSD, S.M.A.R.T., Độ tin cậy", L"StorageReliability, SMART" },
        { 5, L"Đồ họa (GPU)", L"iGPU, dGPU, VRAM, Driver", L"WMI, DXGI" },
        { 6, L"Pin & Nguồn", L"Dung lượng, Sạc, Công suất xả", L"CIM, BatteryDischarge" },
        { 7, L"Mạng & Kết nối", L"Wi-Fi, Bluetooth, LAN", L"WlanApi, Bluetooth" },
        { 8, L"Nhật ký & Sự kiện", L"WHEA, Ổ đĩa, Hệ thống", L"EventLog, Forensics" },
        { 9, L"Stress & Ổn định", L"CPU, RAM, GPU, Nhiệt độ", L"Stress Engine" }
    };

    for (const auto& it : items) {
        EvidenceRowConfig erc;
        erc.parameter = std::to_wstring(it.num) + L". " + it.name;
        erc.actualValue = it.sub;
        erc.providerSource = it.src;
        erc.state = getItemState(it.num);

        RECT ar{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW, curY + UiMetrics::Scale(38, dpi) };
        DrawEvidenceRow(dc, ar, erc, gFonts, dpi);
        curY += UiMetrics::Scale(42, dpi);
    }

    // Live Logs Card (C09 / C12)
    RECT logCard{ r.left + UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(4, dpi), r.left + UiMetrics::Scale(24, dpi) + leftW, r.bottom - UiMetrics::Scale(16, dpi) };
    DrawRoundedCard(dc, logCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    
    SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, logCard.left + UiMetrics::Scale(14, dpi), logCard.top + UiMetrics::Scale(8, dpi), L"Nhật ký & Bằng chứng trực tiếp", 30);

    int logY = logCard.top + UiMetrics::Scale(28, dpi);
    std::lock_guard<std::mutex> lk(gLogsMutex);
    int count = 0;
    for (int i = (int)gLiveLogs.size() - 1; i >= 0 && count < 5 && logY + UiMetrics::Scale(16, dpi) < logCard.bottom; --i, ++count) {
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
    int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT nextCardRect{ rightX, r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(270, dpi) };
    NextActionConfig nac;
    if (gRunning) {
        nac.actionTitle = L"Thu thập bằng chứng tự động";
        nac.reasonText = L"Đang quét thông số phần cứng và đo đạc...";
        nac.remainingTasks = { L"Quét cấu hình & thông số linh kiện", L"Kiểm tra độ ổn định nhiệt độ", L"Tự động ghi nhận log forensics" };
        nac.buttonText = L"ĐANG KIỂM TRA...";
        nac.isButtonEnabled = false;
    } else if (gAuditReady) {
        nac.actionTitle = L"Chuyển sang kiểm tra chức năng";
        nac.reasonText = L"Đã đủ dữ liệu tự động. Tiếp tục với Wizard tương tác.";
        nac.remainingTasks = { L"Kiểm tra màn hình 6 màu", L"Kiểm tra phím & touchpad", L"Kiểm tra loa stereo & mic" };
        nac.buttonText = L"TIẾP TỤC BƯỚC KẾ";
        nac.isButtonEnabled = true;
    } else {
        nac.actionTitle = L"Bắt đầu kiểm tra hệ thống";
        nac.reasonText = L"Yêu cầu phiên kiểm tra tự động trước.";
        nac.remainingTasks = { L"Bước 1: Quét tự động & Stress", L"Bước 2: Wizard tương tác", L"Bước 3: Báo cáo & khuyến nghị" };
        nac.buttonText = L"BẮT ĐẦU KIỂM TRA";
        nac.isButtonEnabled = true;
    }
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
    hdr.subtitle = L"Kiểm tra thực tế các thiết bị cần thao tác tương tác (Bàn phím, Màn hình, Loa, Camera, Cổng kết nối, Ngoại hình)";
    hdr.sessionState = gAuditReady ? CanonicalUiState::Pass : CanonicalUiState::Idle;
    hdr.sessionTag = gAuditReady ? L"Sẵn sàng tương tác" : L"Cần chạy tự động";
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    // C07 Guided Stepper
    auto& f = rep.hardware.stress.functional;
    CanonicalUiState stDisp = GetFunctionalItemUiState(f, L"display_visual");
    CanonicalUiState stKb = GetFunctionalItemUiState(f, L"keyboard_matrix");
    CanonicalUiState stAud = GetFunctionalItemUiState(f, L"audio_stereo");
    CanonicalUiState stCam = GetFunctionalItemUiState(f, L"camera_capture");

    std::vector<StepperStep> steps = {
        { 1, L"Màn hình", L"Điểm chết, màu sắc", stDisp, (stDisp == CanonicalUiState::NotTested) },
        { 2, L"Bàn phím & Touch", L"68 phím, touchpad 80 ô", stKb, (stDisp == CanonicalUiState::Pass && stKb == CanonicalUiState::NotTested) },
        { 3, L"Loa trái / phải", L"Âm lượng, pha stereo", stAud, (stKb == CanonicalUiState::Pass && stAud == CanonicalUiState::NotTested) },
        { 4, L"Camera & Mic", L"Hình ảnh, âm thanh", stCam, (stAud == CanonicalUiState::Pass && stCam == CanonicalUiState::NotTested) }
    };
    RECT stepperRect{ r.left + UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(125, dpi) };
    DrawGuidedStepper(dc, stepperRect, steps, gFonts, dpi);

    // Layout division: 6 Interactive Module Cards on left, Summary & Next Action on right
    int rightPanelW = UiMetrics::Scale(250, dpi);
    int leftW = r.right - r.left - UiMetrics::Scale(48, dpi) - rightPanelW;
    int curY = r.top + UiMetrics::Scale(135, dpi);

    // 6 Interactive Module Cards in 2 columns x 3 rows
    int cardW = (leftW - UiMetrics::Scale(12, dpi)) / 2;
    int cardH = UiMetrics::Scale(95, dpi);

    CanonicalUiState stPorts = (rep.hardware.stress.portPower.overall == L"PASS") ? CanonicalUiState::Pass : ((rep.hardware.stress.portPower.overall == L"FAIL") ? CanonicalUiState::Fail : CanonicalUiState::NotTested);
    CanonicalUiState stPhys = GetFunctionalItemUiState(f, L"physical_chassis");

    struct FuncModule {
        int cmdId;
        const wchar_t* icon;
        const wchar_t* title;
        const wchar_t* desc;
        CanonicalUiState state;
        const wchar_t* btnText;
    };

    std::vector<FuncModule> modules = {
        { 1201, L"🖥️", L"Kiểm tra Màn hình", L"Phát hiện điểm chết (Dead/Stuck pixel), hở sáng, ám màu trên 6 phông màu chuẩn.", stDisp, L"Mở Wizard Màn hình" },
        { 1202, L"⌨️", L"Bàn phím & Touchpad", L"Kiểm tra ma trận phím vật lý 68 nút, độ nảy phím và độ nhạy trackpad 80 ô.", stKb, L"Mở Wizard Phím & Touch" },
        { 1204, L"🔊", L"Loa Stereo & Micro", L"Kiểm tra tín hiệu âm thanh 2 kênh Trái/Phải độc lập và độ nhạy microphone waveIn.", stAud, L"Mở Wizard Loa & Mic" },
        { 1206, L"📷", L"Camera & Kết nối I/O", L"Chụp mẫu hình ảnh Media Foundation, kiểm tra Wi-Fi WLAN API và Bluetooth radio.", stCam, L"Wizard Camera & I/O" },
        { 1207, L"🔌", L"Cổng cắm & Sạc AC", L"Cắm thiết bị vào từng cổng USB-A/C, HDMI, LAN để kiểm tra PnP delta và nguồn sạc.", stPorts, L"Kiểm tra Cổng kết nối" },
        { 1208, L"🔍", L"Ngoại hình & An toàn", L"Kiểm tra 6 điểm vật lý: Bản lề, Vỏ máy, Ốc vít/Cạy mở, Vào nước, Phồng pin, Sạc.", stPhys, L"Wizard 6 điểm vật lý" }
    };

    for (size_t i = 0; i < modules.size(); ++i) {
        int col = (int)i % 2;
        int row = (int)i / 2;
        int cx = r.left + UiMetrics::Scale(24, dpi) + col * (cardW + UiMetrics::Scale(12, dpi));
        int cy = curY + row * (cardH + UiMetrics::Scale(10, dpi));

        RECT cr{ cx, cy, cx + cardW, cy + cardH };
        DrawRoundedCard(dc, cr, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

        // Header icon & title
        SelectObject(dc, gFonts.hBodyBold);
        SetTextColor(dc, UiColors::TextMain);
        std::wstring titleStr = std::wstring(modules[i].icon) + L" " + modules[i].title;
        TextOutW(dc, cr.left + UiMetrics::Scale(12, dpi), cr.top + UiMetrics::Scale(10, dpi), titleStr.c_str(), (int)titleStr.size());

        // Status Badge
        int bw = UiMetrics::Scale(90, dpi);
        int bh = UiMetrics::Scale(20, dpi);
        DrawStatusBadge(dc, cr.right - bw - UiMetrics::Scale(12, dpi), cr.top + UiMetrics::Scale(10, dpi), bw, bh, modules[i].state, gFonts);

        // Description
        SelectObject(dc, gFonts.hSmall);
        SetTextColor(dc, UiColors::TextMuted);
        RECT descR{ cr.left + UiMetrics::Scale(12, dpi), cr.top + UiMetrics::Scale(32, dpi), cr.right - UiMetrics::Scale(12, dpi), cr.top + UiMetrics::Scale(62, dpi) };
        DrawTextW(dc, modules[i].desc, (int)wcslen(modules[i].desc), &descR, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);

        // Launch Wizard Button
        RECT btnR{ cr.left + UiMetrics::Scale(12, dpi), cr.bottom - UiMetrics::Scale(30, dpi), cr.right - UiMetrics::Scale(12, dpi), cr.bottom - UiMetrics::Scale(8, dpi) };
        COLORREF btnBg = gAuditReady ? UiColors::GrayPillBg : RGB(248, 250, 252);
        COLORREF btnTextClr = gAuditReady ? UiColors::PrimaryBlue : UiColors::TextMuted;
        DrawRoundedCard(dc, btnR, UiMetrics::RadiusSm, btnBg, UiColors::GrayPillBorder, 1);
        SelectObject(dc, gFonts.hSmall);
        SetTextColor(dc, btnTextClr);
        DrawTextW(dc, modules[i].btnText, (int)wcslen(modules[i].btnText), &btnR, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    // Right Rail: Functional summary & Next Action Panel (C10)
    int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT checkCard{ rightX, curY, r.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(305, dpi) };
    NextActionConfig nac;
    nac.actionTitle = L"Tiến trình kiểm tra tương tác";
    nac.reasonText = L"Thực hiện xác nhận thực tế các thiết bị trước khi đưa ra kết luận mua máy.";
    nac.remainingTasks = {
        (stDisp == CanonicalUiState::Pass ? L"Màn hình đã kiểm tra" : L"Chưa kiểm tra Màn hình"),
        (stKb == CanonicalUiState::Pass ? L"Bàn phím & Touch đã kiểm tra" : L"Chưa kiểm tra Bàn phím"),
        (stAud == CanonicalUiState::Pass ? L"Loa & Mic đã kiểm tra" : L"Chưa kiểm tra Âm thanh"),
        (stPorts == CanonicalUiState::Pass ? L"Cổng kết nối đã kiểm tra" : L"Chưa kiểm tra Cổng cắm")
    };
    nac.buttonText = gAuditReady ? L"TIẾP TỤC BƯỚC KẾ" : L"CHƯA SẴN SÀNG";
    nac.isButtonEnabled = gAuditReady;
    DrawNextActionPanel(dc, checkCard, nac, gFonts, dpi);
}

void RenderSellerClaim(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Cam kết người bán";
    hdr.subtitle = L"Ghi nhận và lưu trữ thông số rao bán của người bán (Model, CPU, RAM, Ổ cứng, GPU, Giá bán, Bảo hành) để đối chiếu sai lệch.";
    hdr.sessionTag = rep.sellerClaim.model.empty() ? L"Chưa nhập cam kết" : L"Đã lưu cam kết";
    hdr.sessionState = rep.sellerClaim.model.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Pass;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int rightPanelW = UiMetrics::Scale(300, dpi);
    int leftW = r.right - r.left - UiMetrics::Scale(48, dpi) - rightPanelW;
    int curY = r.top + UiMetrics::Scale(70, dpi);

    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW, r.bottom - UiMetrics::Scale(20, dpi) };
    
    DataTableConfig dtc;
    dtc.columns = {
        { L"Thông số cam kết", 160, false, false },
        { L"Giá trị người bán công bố", 280, true, false },
        { L"Trạng thái ghi nhận", 140, false, true }
    };

    auto addRow = [&](const std::wstring& param, const std::wstring& val) {
        TableRow row;
        row.cells.push_back(param);
        row.cells.push_back(val.empty() ? L"— (Chưa công bố)" : val);
        row.cells.push_back(val.empty() ? L"Chưa có dữ liệu" : L"Đã ghi nhận");
        row.rowState = val.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Pass;
        dtc.rows.push_back(row);
    };

    addRow(L"Dòng máy / Model", rep.sellerClaim.model);
    addRow(L"Bộ vi xử lý (CPU)", rep.sellerClaim.cpuContains);
    int sRamGb = (rep.sellerClaim.ramBytes > 0) ? (int)(rep.sellerClaim.ramBytes / (1024*1024*1024)) : 0;
    addRow(L"Dung lượng RAM", sRamGb > 0 ? (std::to_wstring(sRamGb) + L" GB") : L"");
    int sStorageGb = (rep.sellerClaim.storageBytes > 0) ? (int)(rep.sellerClaim.storageBytes / (1000*1000*1000)) : 0;
    addRow(L"Dung lượng Ổ cứng", sStorageGb > 0 ? (std::to_wstring(sStorageGb) + L" GB") : L"");
    addRow(L"Card đồ họa (GPU)", rep.sellerClaim.gpuContains);
    addRow(L"Giá bán công bố (VNĐ)", rep.sellerClaim.askingPriceVnd > 0 ? (std::to_wstring(rep.sellerClaim.askingPriceVnd) + L" đ") : L"");
    addRow(L"Thời hạn bảo hành (ngày)", rep.sellerClaim.warrantyDays > 0 ? (std::to_wstring(rep.sellerClaim.warrantyDays) + L" ngày") : L"");

    DrawDataTable(dc, tableRect, dtc, gFonts, dpi);

    int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT actionCard{ rightX, curY, r.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(240, dpi) };
    NextActionConfig nac;
    nac.actionTitle = L"Nhập cam kết người bán";
    nac.reasonText = L"Dữ liệu này sẽ được dùng để đối chiếu tự động với cấu hình phần cứng thực tế và hồ sơ nhà máy.";
    nac.remainingTasks = { L"Đối chiếu CPU / RAM / SSD", L"Phát hiện tráo đổi linh kiện", L"Đánh giá rủi ro thương mại" };
    nac.buttonText = L"NHẬP / SỬA CAM KẾT";
    nac.isButtonEnabled = true;
    DrawNextActionPanel(dc, actionCard, nac, gFonts, dpi);
}

void RenderPhysicalSafety(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Ngoại hình & An toàn";
    hdr.subtitle = L"Ghi nhận kiểm định 6 điểm vật lý trọng yếu không thể suy diễn bằng phần mềm (Bản lề, Vỏ máy, Ốc vít, Vào nước, Phồng pin, Sạc).";
    CanonicalUiState stPhys = GetFunctionalItemUiState(rep.hardware.stress.functional, L"physical_chassis");
    hdr.sessionTag = (stPhys == CanonicalUiState::Pass) ? L"Đạt an toàn" : ((stPhys == CanonicalUiState::Fail) ? L"Không đạt" : L"Chưa kiểm tra");
    hdr.sessionState = stPhys;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int rightPanelW = UiMetrics::Scale(300, dpi);
    int leftW = r.right - r.left - UiMetrics::Scale(48, dpi) - rightPanelW;
    int curY = r.top + UiMetrics::Scale(70, dpi);

    struct PhysPoint { const wchar_t* name; const wchar_t* criteria; };
    std::vector<PhysPoint> points = {
        { L"1. Bản lề & Cơ cấu gập mở", L"Bản lề mở mượt mà 0-135 độ, không lỏng lẻo, không nứt chân ốc, góc màn hình cứng cáp." },
        { L"2. Vỏ máy & Cấn móp góc cạnh", L"Không nứt vỡ khung gầm, không móp méo gây cấn linh kiện bên trong hoặc cản trở khe tản nhiệt." },
        { L"3. Ốc vít & Dấu hiệu cạy mở", L"Ốc đáy nguyên vẹn, không tuôn ren, không thiếu ốc, vỏ máy khít đều không có vết cạy bẩy." },
        { L"4. Dấu hiệu vào nước & Rỉ sét", L"Không có vết ố nước trên bàn phím, cổng cắm sạch không rỉ sét oxy hóa, quỳ tím không đổi màu." },
        { L"5. Tình trạng sạc & Đầu cắm", L"Chân cắm sạc chắc chắn, không lỏng lẻo chập chờn, adapter sạc nguyên bản đủ công suất." },
        { L"6. Pin phồng & Biến dạng", L"Mặt đáy phẳng tuyệt đối, touchpad không bị đội lên, khe thoát nhiệt thông thoáng." }
    };

    int cardH = UiMetrics::Scale(46, dpi);
    for (size_t i = 0; i < points.size(); ++i) {
        EvidenceRowConfig erc;
        erc.parameter = points[i].name;
        erc.actualValue = points[i].criteria;
        erc.providerSource = L"Kiểm định viên xác nhận";
        erc.state = (stPhys == CanonicalUiState::Pass || stPhys == CanonicalUiState::Fail) ? stPhys : CanonicalUiState::ManualRequired;
        RECT cardR{ r.left + UiMetrics::Scale(24, dpi), curY + (int)i * (cardH + UiMetrics::Scale(8, dpi)), r.left + UiMetrics::Scale(24, dpi) + leftW, curY + (int)i * (cardH + UiMetrics::Scale(8, dpi)) + cardH };
        DrawEvidenceRow(dc, cardR, erc, gFonts, dpi, (i % 2 == 1));
    }

    int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT actionCard{ rightX, curY, r.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(310, dpi) };
    NextActionConfig nac;
    nac.actionTitle = L"Kiểm tra Ngoại hình";
    nac.reasonText = L"Kiểm tra trực quan 6 điểm vật lý để loại trừ các máy bị rơi vỡ nặng, vào nước hoặc pin bị biến dạng.";
    nac.remainingTasks = { L"Xác nhận góc cạnh & bản lề", L"Kiểm tra ốc vít & nắp đáy", L"Kiểm tra hiện tượng phồng pin" };
    nac.buttonText = L"MỞ WIZARD NGOẠI HÌNH";
    nac.isButtonEnabled = true;
    DrawNextActionPanel(dc, actionCard, nac, gFonts, dpi);
}

void RenderPortsPower(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Cổng & Nguồn";
    hdr.subtitle = L"Kiểm tra từng cổng cắm vật lý bằng thiết bị mẫu và theo dõi trạng thái nguồn sạc AC adapter.";
    CanonicalUiState stPorts = (rep.hardware.stress.portPower.overall == L"PASS") ? CanonicalUiState::Pass : ((rep.hardware.stress.portPower.overall == L"FAIL") ? CanonicalUiState::Fail : CanonicalUiState::NotTested);
    hdr.sessionTag = (stPorts == CanonicalUiState::Pass) ? L"Tất cả cổng đạt" : ((stPorts == CanonicalUiState::Fail) ? L"Có cổng lỗi" : L"Chưa kiểm tra");
    hdr.sessionState = stPorts;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int rightPanelW = UiMetrics::Scale(300, dpi);
    int leftW = r.right - r.left - UiMetrics::Scale(48, dpi) - rightPanelW;
    int curY = r.top + UiMetrics::Scale(70, dpi);

    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW, r.bottom - UiMetrics::Scale(20, dpi) };
    
    DataTableConfig dtc;
    dtc.columns = {
        { L"Vị trí cổng", 140, false, false },
        { L"Chuẩn giao tiếp", 130, false, false },
        { L"Bằng chứng PnP Delta", 260, true, false },
        { L"Trạng thái", 100, false, true }
    };

    const auto& ports = rep.hardware.stress.portPower.ports;
    if (ports.empty()) {
        dtc.emptyMessage = L"Chưa có bản ghi kiểm tra cổng. Vui lòng bấm nút bên phải để bắt đầu cắm thiết bị.";
    } else {
        for (const auto& p : ports) {
            TableRow row;
            row.cells.push_back(p.portLabel);
            row.cells.push_back(p.busReportedDescription.empty() ? L"USB / Thunderbolt" : p.busReportedDescription);
            row.cells.push_back(p.instanceId.empty() ? L"Đã ghi nhận cắm rút" : p.instanceId);
            bool passed = (p.verdict == L"PASS" || p.deviceEnumerated);
            row.cells.push_back(passed ? L"ĐẠT" : (p.verdict == L"NOT TESTED" ? L"CHƯA THỬ" : L"KHÔNG ĐẠT"));
            row.rowState = passed ? CanonicalUiState::Pass : (p.verdict == L"NOT TESTED" ? CanonicalUiState::NotTested : CanonicalUiState::Fail);
            dtc.rows.push_back(row);
        }
    }
    DrawDataTable(dc, tableRect, dtc, gFonts, dpi);

    int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT actionCard{ rightX, curY, r.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(280, dpi) };
    NextActionConfig nac;
    nac.actionTitle = L"Kiểm tra cổng vật lý";
    nac.reasonText = L"Sự hiện diện controller không chứng minh cổng cắm hoạt động. Cần cắm thiết bị mẫu để xác thực mạch tín hiệu.";
    nac.remainingTasks = { L"Cắm USB-A / USB-C Flash Drive", L"Kiểm tra xuất hình HDMI / DP", L"Kiểm tra sạc AC Adapter" };
    nac.buttonText = L"CẮM & KIỂM TRA CỔNG";
    nac.isButtonEnabled = true;
    DrawNextActionPanel(dc, actionCard, nac, gFonts, dpi);
}

void RenderFactoryCompare(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Hồ sơ & Đối chiếu";
    hdr.subtitle = L"So sánh đối chiếu 3 chiều: Cấu hình Thực tế vs Cam kết Người bán vs Cấu hình Xuất xưởng gốc của Nhà máy.";
    hdr.sessionTag = rep.factoryExact ? L"Khớp nhà máy" : (rep.genericMode ? L"Hồ sơ suy đoán" : L"Chưa nạp hồ sơ");
    hdr.sessionState = rep.factoryExact ? CanonicalUiState::Pass : (rep.genericMode ? CanonicalUiState::Changed : CanonicalUiState::NotTested);
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int curY = r.top + UiMetrics::Scale(70, dpi);
    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi) };

    DataTableConfig dtc;
    dtc.columns = {
        { L"Phân hệ phần cứng", 130, false, false },
        { L"Cấu hình thực tế máy", 210, true, false },
        { L"Cam kết người bán", 200, false, false },
        { L"Hồ sơ gốc nhà máy", 200, false, false },
        { L"Đánh giá đối chiếu", 120, false, true }
    };

    auto addCompRow = [&](const std::wstring& domain, const std::wstring& actual, const std::wstring& seller, const std::wstring& factory, CanonicalUiState st, const std::wstring& verdict) {
        TableRow row;
        row.cells.push_back(domain);
        row.cells.push_back(actual.empty() ? L"—" : actual);
        row.cells.push_back(seller.empty() ? L"—" : seller);
        row.cells.push_back(factory.empty() ? L"— (Chưa có hồ sơ)" : factory);
        row.cells.push_back(verdict);
        row.rowState = st;
        dtc.rows.push_back(row);
    };

    std::wstring sysModel = rep.model.empty() ? Reg(L"SystemProductName") : rep.model;
    std::wstring factoryModel = rep.factoryExact ? sysModel : L"";
    CanonicalUiState stModel = (!rep.sellerClaim.model.empty() && rep.sellerClaim.model != sysModel) ? CanonicalUiState::Warning : (rep.factoryExact ? CanonicalUiState::Pass : CanonicalUiState::Good);
    std::wstring vModel = (!rep.sellerClaim.model.empty() && rep.sellerClaim.model != sysModel) ? L"LỆCH MODEL" : L"Khớp";
    addCompRow(L"Model máy", sysModel, rep.sellerClaim.model, factoryModel, stModel, vModel);

    std::wstring cpuActual = rep.hardware.cpuName;
    std::wstring factoryCpu = rep.factoryExact ? cpuActual : L"";
    CanonicalUiState stCpu = (!rep.sellerClaim.cpuContains.empty() && cpuActual.find(rep.sellerClaim.cpuContains) == std::wstring::npos) ? CanonicalUiState::Warning : (rep.factoryExact ? CanonicalUiState::Pass : CanonicalUiState::Good);
    std::wstring vCpu = (!rep.sellerClaim.cpuContains.empty() && cpuActual.find(rep.sellerClaim.cpuContains) == std::wstring::npos) ? L"LỆCH CPU" : L"Khớp";
    addCompRow(L"Bộ vi xử lý (CPU)", cpuActual, rep.sellerClaim.cpuContains, factoryCpu, stCpu, vCpu);

    int actualRamGb = (rep.hardware.installedRamBytes > 0) ? (int)(rep.hardware.installedRamBytes / (1024*1024*1024)) : 0;
    std::wstring ramActual = actualRamGb > 0 ? (std::to_wstring(actualRamGb) + L" GB") : L"—";
    int sellerRamGb = (rep.sellerClaim.ramBytes > 0) ? (int)(rep.sellerClaim.ramBytes / (1024*1024*1024)) : 0;
    std::wstring ramSeller = sellerRamGb > 0 ? (std::to_wstring(sellerRamGb) + L" GB") : L"—";
    std::wstring factoryRam = rep.factoryExact ? ramActual : L"";
    CanonicalUiState stRam = (sellerRamGb > 0 && sellerRamGb != actualRamGb) ? CanonicalUiState::Warning : (rep.factoryExact ? CanonicalUiState::Pass : CanonicalUiState::Good);
    std::wstring vRam = (sellerRamGb > 0 && sellerRamGb != actualRamGb) ? L"LỆCH RAM" : L"Khớp";
    addCompRow(L"Bộ nhớ (RAM)", ramActual, ramSeller, factoryRam, stRam, vRam);

    std::wstring ssdActual = rep.hardware.storage.empty() ? L"—" : rep.hardware.storage.front().model;
    int sellerStorageGb = (rep.sellerClaim.storageBytes > 0) ? (int)(rep.sellerClaim.storageBytes / (1000*1000*1000)) : 0;
    std::wstring ssdSeller = sellerStorageGb > 0 ? (std::to_wstring(sellerStorageGb) + L" GB") : L"—";
    std::wstring factorySsd = rep.factoryExact ? ssdActual : L"";
    addCompRow(L"Ổ đĩa lưu trữ", ssdActual, ssdSeller, factorySsd, CanonicalUiState::Good, L"Đã ghi nhận");

    std::wstring gpuActual = rep.hardware.gpus.empty() ? L"—" : rep.hardware.gpus.front().name;
    std::wstring factoryGpu = rep.factoryExact ? gpuActual : L"";
    CanonicalUiState stGpu = (!rep.sellerClaim.gpuContains.empty() && gpuActual.find(rep.sellerClaim.gpuContains) == std::wstring::npos) ? CanonicalUiState::Warning : CanonicalUiState::Good;
    std::wstring vGpu = (!rep.sellerClaim.gpuContains.empty() && gpuActual.find(rep.sellerClaim.gpuContains) == std::wstring::npos) ? L"LỆCH GPU" : L"Khớp";
    addCompRow(L"Card đồ họa (GPU)", gpuActual, rep.sellerClaim.gpuContains, factoryGpu, stGpu, vGpu);

    DrawDataTable(dc, tableRect, dtc, gFonts, dpi, gTableScrollOffset);
}

void RenderStressStability(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Stress & Độ ổn định";
    hdr.subtitle = L"Kiểm tra tải nặng CPU/RAM/GPU, theo dõi nhiệt độ tản nhiệt và phát hiện lỗi phần cứng phát sinh dưới tải.";
    CanonicalUiState stStress = rep.hardware.stress.completed ? CanonicalUiState::Pass : CanonicalUiState::NotTested;
    hdr.sessionTag = rep.hardware.stress.completed ? L"Ổn định" : L"Chưa kiểm tra";
    hdr.sessionState = stStress;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int rightPanelW = UiMetrics::Scale(300, dpi);
    int leftW = r.right - r.left - UiMetrics::Scale(48, dpi) - rightPanelW;
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // 3 KPI Metric Cards
    int kpiW = (leftW - UiMetrics::Scale(16, dpi)) / 3;
    int kpiH = UiMetrics::Scale(88, dpi);

    MetricCardConfig mc1;
    mc1.label = L"Độ ổn định chung";
    mc1.value = rep.hardware.stress.completed ? L"ĐẠT (Ổn định)" : L"—";
    mc1.state = stStress;
    mc1.note = L"Kết luận Stress Test";
    RECT kpi1Rect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, kpi1Rect, mc1, gFonts, dpi);

    MetricCardConfig mc2;
    mc2.label = L"CPU Microbenchmark";
    mc2.value = rep.hardware.stress.cpuBenchmark.score > 0 ? (std::to_wstring(rep.hardware.stress.cpuBenchmark.score) + L" pts") : L"—";
    mc2.state = rep.hardware.stress.cpuBenchmark.score > 0 ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    mc2.note = L"Điểm hiệu năng vi xử lý";
    RECT kpi2Rect{ kpi1Rect.right + UiMetrics::Scale(8, dpi), curY, kpi1Rect.right + UiMetrics::Scale(8, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, kpi2Rect, mc2, gFonts, dpi);

    long long critEvents = rep.hardware.events.whea + rep.hardware.events.kernelPower + rep.hardware.events.bugCheck;
    MetricCardConfig mc3;
    mc3.label = L"Sự kiện WHEA / Lỗi";
    mc3.value = std::to_wstring(critEvents);
    mc3.state = (critEvents > 0) ? CanonicalUiState::Warning : CanonicalUiState::Good;
    mc3.note = L"Lỗi kiến trúc phần cứng";
    RECT kpi3Rect{ kpi2Rect.right + UiMetrics::Scale(8, dpi), curY, kpi2Rect.right + UiMetrics::Scale(8, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, kpi3Rect, mc3, gFonts, dpi);

    curY += kpiH + UiMetrics::Scale(12, dpi);

    // Stress Stages List (C08)
    struct StageDef { const wchar_t* name; const wchar_t* desc; const wchar_t* duration; };
    std::vector<StageDef> stages = {
        { L"Stage 1 — CPU Microbenchmark & Đa luồng", L"Kiểm tra tải tính toán số học đa lõi CPU và đo độ trễ bộ đệm L1/L2/L3.", L"30 giây" },
        { L"Stage 2 — Bộ nhớ RAM Memory Pressure", L"Cấp phát và ghi đọc liên tục trên 80% RAM để phát hiện cell lỗi.", L"20 giây" },
        { L"Stage 3 — Đồ họa Direct3D GPU Load", L"Kích hoạt tải đồ họa 3D Direct3D để kiểm tra ổn định nguồn VRAM.", L"30 giây" },
        { L"Stage 4 — Giám sát Thermal Throttling", L"Theo dõi hiện tượng quá nhiệt giảm xung nhịp CPU/GPU dưới tải kéo dài.", L"40 giây" }
    };

    for (size_t i = 0; i < stages.size(); ++i) {
        EvidenceRowConfig erc;
        erc.parameter = stages[i].name;
        erc.actualValue = stages[i].desc;
        erc.providerSource = stages[i].duration;
        erc.state = stStress;

        RECT sr{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW, curY + UiMetrics::Scale(52, dpi) };
        DrawEvidenceRow(dc, sr, erc, gFonts, dpi, (i % 2 == 1));
        curY += UiMetrics::Scale(58, dpi);
    }

    // Right Rail: Action Panel
    int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT actionCard{ rightX, r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(280, dpi) };
    NextActionConfig nac;
    nac.actionTitle = L"Chạy bài kiểm tra Stress";
    nac.reasonText = L"Bài kiểm tra này phát hiện hiện tượng sập nguồn, quá nhiệt hoặc lỗi RAM chỉ xuất hiện khi chạy nặng.";
    nac.remainingTasks = { L"Tải CPU đa nhân 100%", L"Tải RAM bộ đệm áp lực", L"Giám sát nhiệt độ & WHEA" };
    nac.buttonText = gRunning ? L"DỪNG KIỂM TRA" : L"CHẠY STRESS TEST";
    nac.isButtonEnabled = true;
    DrawNextActionPanel(dc, actionCard, nac, gFonts, dpi);
}

void RenderBatteryPower(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    bool batPresent = rep.hardware.battery.present;
    bool capReadable = rep.hardware.battery.capacityReadable && rep.hardware.battery.designWh > 0;
    
    PageHeaderConfig hdr;
    hdr.title = L"Pin & Năng lượng";
    hdr.subtitle = L"Chi tiết dung lượng thiết kế, dung lượng thực tế, độ chai pin, chu kỳ sạc và kiểm tra công suất xả mW.";
    
    if (batPresent) {
        if (capReadable) {
            hdr.sessionTag = std::to_wstring((int)rep.hardware.battery.healthPercent) + L"% sức khỏe";
            hdr.sessionState = (rep.hardware.battery.healthPercent < 50) ? CanonicalUiState::Warning : CanonicalUiState::Good;
        } else {
            hdr.sessionTag = L"Đã phát hiện pin";
            hdr.sessionState = CanonicalUiState::Good;
        }
    } else {
        hdr.sessionTag = L"Thiết bị không có pin";
        hdr.sessionState = CanonicalUiState::Unsupported;
    }
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int curY = r.top + UiMetrics::Scale(70, dpi);

    // 4 KPI Metric Cards (C05)
    int mainW = r.right - r.left - UiMetrics::Scale(48, dpi);
    int kpiW = (mainW - UiMetrics::Scale(36, dpi)) / 4;
    int kpiH = UiMetrics::Scale(88, dpi);

    MetricCardConfig mc1;
    mc1.label = L"Độ chai pin";
    if (batPresent && capReadable) {
        mc1.value = std::to_wstring((int)(100 - rep.hardware.battery.healthPercent)) + L"%";
        mc1.state = (100 - rep.hardware.battery.healthPercent > 25) ? CanonicalUiState::Warning : CanonicalUiState::Good;
        mc1.note = L"Mức độ suy giảm dung lượng";
    } else if (batPresent) {
        mc1.value = L"—";
        mc1.state = CanonicalUiState::Info;
        mc1.note = L"Firmware không công bố";
    } else {
        mc1.value = L"—";
        mc1.state = CanonicalUiState::Unsupported;
        mc1.note = L"Không trang bị pin";
    }
    RECT kpi1Rect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, kpi1Rect, mc1, gFonts, dpi);

    MetricCardConfig mc2;
    mc2.label = L"Dung lượng thực tế (Full)";
    if (batPresent && rep.hardware.battery.fullChargeWh > 0) {
        mc2.value = std::to_wstring((int)rep.hardware.battery.fullChargeWh) + L" Wh";
        mc2.state = CanonicalUiState::Good;
        mc2.note = L"Khi sạc đầy 100%";
    } else if (batPresent) {
        mc2.value = L"—";
        mc2.state = CanonicalUiState::Info;
        mc2.note = L"Chưa đọc được từ firmware";
    } else {
        mc2.value = L"—";
        mc2.state = CanonicalUiState::Unsupported;
        mc2.note = L"Không trang bị pin";
    }
    RECT kpi2Rect{ kpi1Rect.right + UiMetrics::Scale(12, dpi), curY, kpi1Rect.right + UiMetrics::Scale(12, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, kpi2Rect, mc2, gFonts, dpi);

    MetricCardConfig mc3;
    mc3.label = L"Dung lượng thiết kế (Design)";
    if (batPresent && rep.hardware.battery.designWh > 0) {
        mc3.value = std::to_wstring((int)rep.hardware.battery.designWh) + L" Wh";
        mc3.state = CanonicalUiState::Good;
        mc3.note = L"Xuất xưởng gốc từ nhà máy";
    } else if (batPresent) {
        mc3.value = L"—";
        mc3.state = CanonicalUiState::Info;
        mc3.note = L"Chưa đọc được từ firmware";
    } else {
        mc3.value = L"—";
        mc3.state = CanonicalUiState::Unsupported;
        mc3.note = L"Không trang bị pin";
    }
    RECT kpi3Rect{ kpi2Rect.right + UiMetrics::Scale(12, dpi), curY, kpi2Rect.right + UiMetrics::Scale(12, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, kpi3Rect, mc3, gFonts, dpi);

    MetricCardConfig mc4;
    mc4.label = L"Số chu kỳ sạc (Cycles)";
    if (batPresent && rep.hardware.battery.cycleCount >= 0) {
        mc4.value = std::to_wstring(rep.hardware.battery.cycleCount);
        mc4.state = (rep.hardware.battery.cycleCount > 500) ? CanonicalUiState::Warning : CanonicalUiState::Good;
        mc4.note = L"Số lần nạp xả hoàn toàn";
    } else if (batPresent) {
        mc4.value = L"—";
        mc4.state = CanonicalUiState::Info;
        mc4.note = L"Không công bố bởi firmware";
    } else {
        mc4.value = L"—";
        mc4.state = CanonicalUiState::Unsupported;
        mc4.note = L"Không trang bị pin";
    }
    RECT kpi4Rect{ kpi3Rect.right + UiMetrics::Scale(12, dpi), curY, kpi3Rect.right + UiMetrics::Scale(12, dpi) + kpiW, curY + kpiH };
    DrawMetricCard(dc, kpi4Rect, mc4, gFonts, dpi);

    curY += kpiH + UiMetrics::Scale(14, dpi);

    // Battery Findings & Evidence Table (C09)
    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi) };
    
    DataTableConfig dtc;
    dtc.columns = {
        { L"Thông số pin & nguồn", 180, false, false },
        { L"Giá trị đo đạc thực tế", 280, true, false },
        { L"Nguồn cung cấp bằng chứng", 180, false, false },
        { L"Đánh giá", 120, false, true }
    };

    auto addBatRow = [&](const std::wstring& param, const std::wstring& val, const std::wstring& src, CanonicalUiState st, const std::wstring& stText) {
        TableRow row;
        row.cells.push_back(param);
        row.cells.push_back(val.empty() ? L"—" : val);
        row.cells.push_back(src);
        row.cells.push_back(stText);
        row.rowState = st;
        dtc.rows.push_back(row);
    };

    if (batPresent) {
        std::wstring statusStr = rep.hardware.battery.status.empty() ? L"Bình thường (OK)" : rep.hardware.battery.status;
        addBatRow(L"Trạng thái Pin (Status)", statusStr, L"CIM / WMI", CanonicalUiState::Good, L"Tốt");

        std::wstring mfr = rep.hardware.battery.manufacturer.empty() ? L"— (Bảo mật OEM)" : rep.hardware.battery.manufacturer;
        addBatRow(L"Nhà sản xuất cell pin", mfr, L"CIM / BatteryReport", rep.hardware.battery.manufacturer.empty() ? CanonicalUiState::Info : CanonicalUiState::Good, rep.hardware.battery.manufacturer.empty() ? L"Thông tin" : L"Tốt");

        std::wstring sn = rep.hardware.battery.serialNumber.empty() ? L"— (Bảo mật OEM)" : rep.hardware.battery.serialNumber;
        addBatRow(L"Số Serial Pin", sn, L"CIM / WMI", rep.hardware.battery.serialNumber.empty() ? CanonicalUiState::Info : CanonicalUiState::Good, rep.hardware.battery.serialNumber.empty() ? L"Thông tin" : L"Tốt");

        std::wstring acStatus = rep.hardware.stress.portPower.power.acConnected ? L"Đang cắm sạc AC (AC Line Connected)" : L"Đang dùng pin (Discharging On Battery)";
        addBatRow(L"Trạng thái nguồn AC", acStatus, L"Win32 Power API", CanonicalUiState::Good, L"Đạt");

        std::wstring cycleStr = rep.hardware.battery.cycleCount >= 0 ? (std::to_wstring(rep.hardware.battery.cycleCount) + L" chu kỳ") : L"— (Bảo mật OEM / Không cung cấp)";
        addBatRow(L"Chu kỳ sạc xả (Cycle Count)", cycleStr, L"Battery Firmware", rep.hardware.battery.cycleCount >= 0 ? CanonicalUiState::Good : CanonicalUiState::Info, rep.hardware.battery.cycleCount >= 0 ? L"Đã đọc" : L"Thông tin");
    } else {
        addBatRow(L"Trạng thái Pin (Status)", L"Không có pin vật lý (Thiết bị dùng nguồn trực tiếp)", L"Win32 Power API", CanonicalUiState::Unsupported, L"Không hỗ trợ");
        addBatRow(L"Nhà sản xuất cell pin", L"— (Không áp dụng)", L"Battery Provider", CanonicalUiState::Unsupported, L"Không hỗ trợ");
        addBatRow(L"Số Serial Pin", L"— (Không áp dụng)", L"Battery Provider", CanonicalUiState::Unsupported, L"Không hỗ trợ");
        addBatRow(L"Trạng thái nguồn AC", L"Nguồn điện AC trực tiếp (AC Power Line)", L"Win32 Power API", CanonicalUiState::Good, L"Đạt");
        addBatRow(L"Chu kỳ sạc xả (Cycle Count)", L"— (Không áp dụng)", L"Battery Provider", CanonicalUiState::Unsupported, L"Không hỗ trợ");
    }

    DrawDataTable(dc, tableRect, dtc, gFonts, dpi, gTableScrollOffset);
}

void RenderStorage(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Lưu trữ (Storage / NVMe / SSD)";
    hdr.subtitle = L"Danh sách các ổ đĩa vật lý, chuẩn giao tiếp NVMe/SATA, trạng thái S.M.A.R.T. và độ tin cậy phần cứng.";
    bool hasDisk = !rep.hardware.storage.empty();
    hdr.sessionTag = hasDisk ? (std::to_wstring(rep.hardware.storage.size()) + L" ổ đĩa") : L"Chưa phát hiện";
    hdr.sessionState = hasDisk ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int curY = r.top + UiMetrics::Scale(70, dpi);
    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi) };

    DataTableConfig dtc;
    dtc.columns = {
        { L"Model Ổ đĩa", 200, false, false },
        { L"Chuẩn kết nối", 110, false, false },
        { L"Dung lượng", 110, false, false },
        { L"Serial Number", 150, true, false },
        { L"S.M.A.R.T. Health", 160, false, true }
    };

    if (rep.hardware.storage.empty()) {
        dtc.emptyMessage = L"Chưa phát hiện ổ đĩa nào hoặc phiên kiểm tra chưa hoàn tất.";
    } else {
        for (const auto& d : rep.hardware.storage) {
            TableRow row;
            row.cells.push_back(d.model.empty() ? L"Ổ đĩa lưu trữ" : d.model);
            row.cells.push_back(d.interfaceType.empty() ? L"NVMe / SATA" : d.interfaceType);
            std::wstring capStr = (d.capacityBytes > 0) ? (std::to_wstring(d.capacityBytes / (1000 * 1000 * 1000)) + L" GB") : L"—";
            row.cells.push_back(capStr);
            row.cells.push_back(d.serialNumber.empty() ? L"—" : d.serialNumber);
            if (!d.smartReadable) {
                row.cells.push_back(L"Chưa đọc được S.M.A.R.T.");
                row.rowState = CanonicalUiState::NotTested;
            } else if (!d.smartPassed) {
                row.cells.push_back(L"KHÔNG ĐẠT (Lỗi)");
                row.rowState = CanonicalUiState::Fail;
            } else {
                row.cells.push_back(L"ĐẠT (Tốt)");
                row.rowState = CanonicalUiState::Good;
            }
            dtc.rows.push_back(row);
        }
    }
    DrawDataTable(dc, tableRect, dtc, gFonts, dpi, gTableScrollOffset);
}

void RenderMemory(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Bộ nhớ RAM";
    hdr.subtitle = L"Chi tiết từng thanh RAM vật lý, tốc độ bus, số khe cắm DIMM và kết quả kiểm tra tính toàn vẹn bộ nhớ.";
    bool hasRam = rep.hardware.installedRamBytes > 0;
    std::wstring ramTotalStr = hasRam ? (std::to_wstring(rep.hardware.installedRamBytes / (1024 * 1024 * 1024)) + L" GB") : L"Chưa phát hiện";
    hdr.sessionTag = ramTotalStr;
    hdr.sessionState = hasRam ? CanonicalUiState::Good : CanonicalUiState::NotTested;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int curY = r.top + UiMetrics::Scale(70, dpi);
    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi) };

    DataTableConfig dtc;
    dtc.columns = {
        { L"Vị trí khe cắm", 130, false, false },
        { L"Dung lượng", 110, false, false },
        { L"Tốc độ Bus", 120, false, false },
        { L"Nhà sản xuất", 150, false, false },
        { L"Mã Part Number", 180, false, false },
        { L"Trạng thái", 120, false, true }
    };

    if (rep.hardware.memoryModules.empty()) {
        dtc.emptyMessage = L"Chưa có thông tin chi tiết từng khe RAM. Tổng RAM hệ thống: " + ramTotalStr;
    } else {
        for (const auto& m : rep.hardware.memoryModules) {
            TableRow row;
            row.cells.push_back(m.bankLabel.empty() ? L"DIMM Slot" : m.bankLabel);
            std::wstring cap = (m.capacityBytes > 0) ? (std::to_wstring(m.capacityBytes / (1024 * 1024 * 1024)) + L" GB") : L"—";
            row.cells.push_back(cap);
            std::wstring spd = (m.configuredSpeed > 0) ? (std::to_wstring(m.configuredSpeed) + L" MHz") : L"—";
            row.cells.push_back(spd);
            row.cells.push_back(m.manufacturer.empty() ? L"—" : m.manufacturer);
            row.cells.push_back(m.partNumber.empty() ? L"—" : m.partNumber);
            row.cells.push_back(m.capacityBytes > 0 ? L"ĐÃ NHẬN DIỆN" : L"CHƯA RÕ");
            row.rowState = m.capacityBytes > 0 ? CanonicalUiState::Info : CanonicalUiState::NotTested;
            dtc.rows.push_back(row);
        }
    }
    DrawDataTable(dc, tableRect, dtc, gFonts, dpi, gTableScrollOffset);
}

void RenderDisplay(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Hiển thị (Màn hình)";
    hdr.subtitle = L"Dữ liệu EDID gốc từ phần cứng tấm nền, độ phân giải native, tần số quét và kết quả kiểm tra điểm chết.";
    auto& f = rep.hardware.stress.functional;
    CanonicalUiState stDisp = GetFunctionalItemUiState(f, L"display_visual");
    hdr.sessionTag = (stDisp == CanonicalUiState::Pass) ? L"Màn hình đạt" : ((stDisp == CanonicalUiState::Fail) ? L"Có lỗi điểm chết" : L"Chưa kiểm tra");
    hdr.sessionState = stDisp;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int rightPanelW = UiMetrics::Scale(300, dpi);
    int leftW = r.right - r.left - UiMetrics::Scale(48, dpi) - rightPanelW;
    int curY = r.top + UiMetrics::Scale(70, dpi);

    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW, r.bottom - UiMetrics::Scale(20, dpi) };
    
    DataTableConfig dtc;
    dtc.columns = {
        { L"Thông số màn hình", 160, false, false },
        { L"Giá trị thực tế", 260, true, false },
        { L"Đánh giá", 120, false, true }
    };

    auto addRow = [&](const std::wstring& p, const std::wstring& v, CanonicalUiState s, const std::wstring& stText) {
        TableRow row;
        row.cells.push_back(p);
        row.cells.push_back(v.empty() ? L"—" : v);
        row.cells.push_back(stText);
        row.rowState = s;
        dtc.rows.push_back(row);
    };

    if (!rep.hardware.displays.empty()) {
        const auto& d = rep.hardware.displays.front();
        addRow(L"Tên màn hình (EDID)", d.friendlyName, CanonicalUiState::Good, L"Tốt");
        addRow(L"Nhà sản xuất tấm nền", d.manufacturer, CanonicalUiState::Good, L"Tốt");
        std::wstring resStr = (d.nativeWidth > 0 && d.nativeHeight > 0) ? (std::to_wstring(d.nativeWidth) + L" x " + std::to_wstring(d.nativeHeight)) : L"—";
        addRow(L"Độ phân giải Native", resStr, CanonicalUiState::Good, L"Đạt");
        std::wstring hzStr = (d.refreshHz > 0) ? (std::to_wstring(d.refreshHz) + L" Hz") : L"—";
        addRow(L"Tần số quét", hzStr, CanonicalUiState::Good, L"Đạt");
    } else {
        addRow(L"Dữ liệu EDID", L"Đang quét thông số...", CanonicalUiState::NotTested, L"Chưa rõ");
    }
    addRow(L"Kiểm tra 6 màu & Điểm chết", (stDisp == CanonicalUiState::Pass) ? L"Đã xác nhận không điểm chết" : L"Chưa hoàn tất Wizard", stDisp, (stDisp == CanonicalUiState::Pass ? L"ĐẠT" : L"CẦN XÁC NHẬN"));

    DrawDataTable(dc, tableRect, dtc, gFonts, dpi);

    int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT actionCard{ rightX, curY, r.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(260, dpi) };
    NextActionConfig nac;
    nac.actionTitle = L"Kiểm tra màn hình";
    nac.reasonText = L"Chạy Wizard toàn màn hình qua 6 phông màu chuẩn (Đỏ, Xanh lá, Xanh dương, Trắng, Đen, Xám) để phát hiện điểm chết.";
    nac.remainingTasks = { L"Phát hiện Dead / Stuck pixel", L"Kiểm tra hở sáng viền (IPS glow)", L"Kiểm tra ám màu / ố màn hình" };
    nac.buttonText = L"MỞ WIZARD 6 MÀU";
    nac.isButtonEnabled = true;
    DrawNextActionPanel(dc, actionCard, nac, gFonts, dpi);
}

void RenderAudioCamera(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Âm thanh & Camera";
    hdr.subtitle = L"Kiểm tra thực tế 2 kênh loa Stereo trái/phải, độ nhạy microphone và khả năng thu nhận hình ảnh từ Camera.";
    auto& f = rep.hardware.stress.functional;
    CanonicalUiState stAud = GetFunctionalItemUiState(f, L"audio_stereo");
    CanonicalUiState stCam = GetFunctionalItemUiState(f, L"camera_capture");
    hdr.sessionTag = (stAud == CanonicalUiState::Pass && stCam == CanonicalUiState::Pass) ? L"Tất cả đạt" : L"Cần kiểm tra";
    hdr.sessionState = (stAud == CanonicalUiState::Pass && stCam == CanonicalUiState::Pass) ? CanonicalUiState::Pass : CanonicalUiState::NotTested;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int rightPanelW = UiMetrics::Scale(300, dpi);
    int leftW = r.right - r.left - UiMetrics::Scale(48, dpi) - rightPanelW;
    int curY = r.top + UiMetrics::Scale(70, dpi);

    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW, r.bottom - UiMetrics::Scale(20, dpi) };
    
    DataTableConfig dtc;
    dtc.columns = {
        { L"Thiết bị I/O", 150, false, false },
        { L"Phương thức kiểm tra", 230, true, false },
        { L"Bằng chứng ghi nhận", 160, false, false },
        { L"Trạng thái", 120, false, true }
    };

    auto addRow = [&](const std::wstring& dev, const std::wstring& method, const std::wstring& ev, CanonicalUiState st, const std::wstring& stText) {
        TableRow row;
        row.cells.push_back(dev);
        row.cells.push_back(method);
        row.cells.push_back(ev);
        row.cells.push_back(stText);
        row.rowState = st;
        dtc.rows.push_back(row);
    };

    addRow(L"Loa ngoài Trái/Phải", L"Phát âm tần số 440Hz PCM stereo độc lập", (stAud == CanonicalUiState::Pass) ? L"Người dùng xác nhận L/R" : L"Chưa kiểm tra", stAud, (stAud == CanonicalUiState::Pass ? L"ĐẠT" : L"CẦN XÁC NHẬN"));
    addRow(L"Microphone tích hợp", L"Thu âm trực tiếp waveIn RMS signal level", (stAud == CanonicalUiState::Pass) ? L"Đo biên độ âm thanh > 5%" : L"Chưa kiểm tra", stAud, (stAud == CanonicalUiState::Pass ? L"ĐẠT" : L"CẦN XÁC NHẬN"));
    addRow(L"Camera trước / WebCam", L"Media Foundation trích xuất frame mẫu", (stCam == CanonicalUiState::Pass) ? L"Đã chụp frame thực tế" : L"Chưa kiểm tra", stCam, (stCam == CanonicalUiState::Pass ? L"ĐẠT" : L"CẦN XÁC NHẬN"));

    DrawDataTable(dc, tableRect, dtc, gFonts, dpi);

    int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT actionCard{ rightX, curY, r.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(260, dpi) };
    NextActionConfig nac;
    nac.actionTitle = L"Kiểm tra Loa & Camera";
    nac.reasonText = L"Kích hoạt âm thanh thử nghiệm và chụp khung hình trực tiếp để xác minh chức năng phần cứng.";
    nac.remainingTasks = { L"Kiểm tra Loa Stereo 2 kênh", L"Đo cường độ tín hiệu Micro", L"Kiểm tra cảm biến hình ảnh Camera" };
    nac.buttonText = L"MỞ WIZARD LOA & CAM";
    nac.isButtonEnabled = true;
    DrawNextActionPanel(dc, actionCard, nac, gFonts, dpi);
}

void RenderNetwork(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Mạng & Kết nối";
    hdr.subtitle = L"Kiểm tra bộ điều hợp Wi-Fi WLAN API, cường độ tín hiệu RSSI, chuẩn Bluetooth Radio và cổng mạng LAN.";
    auto& f = rep.hardware.stress.functional;
    CanonicalUiState stWifi = GetFunctionalItemUiState(f, L"wifi_scan");
    CanonicalUiState stBt = GetFunctionalItemUiState(f, L"bluetooth_radio");
    hdr.sessionTag = (stWifi == CanonicalUiState::Pass) ? L"Wi-Fi đã xác nhận" : L"Chưa kiểm tra";
    hdr.sessionState = stWifi;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int curY = r.top + UiMetrics::Scale(70, dpi);
    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi) };

    DataTableConfig dtc;
    dtc.columns = {
        { L"Phân hệ kết nối", 140, false, false },
        { L"Tên Card mạng / Adapter", 240, true, false },
        { L"Thông số kỹ thuật / Tín hiệu", 240, false, false },
        { L"Trạng thái", 120, false, true }
    };

    TableRow r1;
    r1.cells = { L"Wi-Fi (Wireless LAN)", L"Bộ điều hợp Wi-Fi (WlanApi)", (stWifi == CanonicalUiState::Pass) ? L"Đã quét mạng & đo chất lượng sóng" : L"Chưa chạy kiểm tra", (stWifi == CanonicalUiState::Pass ? L"ĐẠT" : L"CHƯA KIỂM TRA") };
    r1.rowState = stWifi;
    dtc.rows.push_back(r1);

    TableRow r2;
    r2.cells = { L"Bluetooth Radio", L"Bộ thu phát Bluetooth (BthProps)", (stBt == CanonicalUiState::Pass) ? L"Radio Stack phản hồi bình thường" : L"Chưa chạy kiểm tra", (stBt == CanonicalUiState::Pass ? L"ĐẠT" : L"CHƯA KIỂM TRA") };
    r2.rowState = stBt;
    dtc.rows.push_back(r2);

    TableRow r3;
    r3.cells = { L"Cổng mạng LAN (RJ45)", L"Bộ điều khiển Ethernet", L"Kiểm tra qua cắm cáp mạng thực tế", L"CẦN XÁC NHẬN" };
    r3.rowState = CanonicalUiState::NotTested;
    dtc.rows.push_back(r3);

    DrawDataTable(dc, tableRect, dtc, gFonts, dpi, gTableScrollOffset);
}

void RenderSystemInfo(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Thông tin Hệ thống";
    hdr.subtitle = L"Toàn bộ thông số BIOS, Bo mạch chủ, TPM 2.0, Secure Boot, phiên bản Windows và mã lỗi PnP.";
    hdr.sessionTag = rep.model.empty() ? L"Windows x64" : rep.model;
    hdr.sessionState = CanonicalUiState::Good;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int curY = r.top + UiMetrics::Scale(70, dpi);
    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi) };

    DataTableConfig dtc;
    dtc.columns = {
        { L"Hạng mục hệ thống", 180, false, false },
        { L"Giá trị chi tiết", 340, true, false },
        { L"Nguồn cung cấp", 160, false, false },
        { L"Đánh giá", 120, false, true }
    };

    auto addRow = [&](const std::wstring& p, const std::wstring& v, const std::wstring& src, CanonicalUiState st, const std::wstring& stText) {
        TableRow row;
        row.cells.push_back(p);
        row.cells.push_back(v.empty() ? L"—" : v);
        row.cells.push_back(src);
        row.cells.push_back(stText);
        row.rowState = st;
        dtc.rows.push_back(row);
    };

    std::wstring sysModel = rep.model.empty() ? Reg(L"SystemProductName") : rep.model;
    addRow(L"Dòng máy / Model", sysModel, L"SMBIOS / Registry", CanonicalUiState::Info, L"Thông tin");

    std::wstring mfr = rep.hardware.mainboard.manufacturer.empty() ? Reg(L"SystemManufacturer") : rep.hardware.mainboard.manufacturer;
    addRow(L"Nhà sản xuất", mfr, L"SMBIOS", CanonicalUiState::Info, L"Thông tin");

    std::wstring cpuStr = rep.hardware.cpuName;
    addRow(L"Bộ vi xử lý (CPU)", cpuStr, L"WMI / CPUID", CanonicalUiState::Info, L"Thông tin");

    std::wstring biosVer = rep.hardware.bios.version.empty() ? Reg(L"SystemBiosVersion") : rep.hardware.bios.version;
    addRow(L"Phiên bản BIOS", biosVer, L"SMBIOS / Registry", CanonicalUiState::Info, L"Thông tin");

    std::wstring osStr = rep.environment.empty() ? L"Windows 11 / 10 x64 Native" : rep.environment;
    addRow(L"Hệ điều hành", osStr, L"Win32 System", CanonicalUiState::Info, L"Thông tin");

    std::wstring tpmStr = rep.hardware.security.tpmPresent ? L"TPM 2.0 Hoạt động (Enabled)" : L"Không phát hiện TPM / Bị tắt";
    addRow(L"Bảo mật TPM", tpmStr, L"TPM Provider", rep.hardware.security.tpmPresent ? CanonicalUiState::Good : CanonicalUiState::Warning, rep.hardware.security.tpmPresent ? L"Đạt" : L"Cảnh báo");

    std::wstring sbStr = rep.hardware.security.secureBootEnabled ? L"Secure Boot Bật (Enabled)" : L"Secure Boot Tắt (Disabled)";
    addRow(L"Khởi động an toàn", sbStr, L"Firmware Environment", rep.hardware.security.secureBootEnabled ? CanonicalUiState::Good : CanonicalUiState::Warning, rep.hardware.security.secureBootEnabled ? L"Đạt" : L"Cảnh báo");

    DrawDataTable(dc, tableRect, dtc, gFonts, dpi, gTableScrollOffset);
}

void RenderEvidenceLibrary(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Thư viện Bằng chứng";
    hdr.subtitle = L"Toàn bộ bằng chứng chẩn đoán phần cứng đã trích xuất, phục vụ đối chiếu và truy xuất nguồn gốc.";
    hdr.sessionTag = std::to_wstring(rep.findings.size()) + L" bằng chứng";
    hdr.sessionState = gAuditReady ? CanonicalUiState::Pass : CanonicalUiState::NotTested;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int curY = r.top + UiMetrics::Scale(70, dpi);
    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi) };

    DataTableConfig dtc;
    dtc.columns = {
        { L"Phân hệ", 110, false, false },
        { L"Hạng mục kiểm tra", 150, false, false },
        { L"Bằng chứng trích xuất", 260, true, false },
        { L"Chuẩn đối chiếu / Gốc", 160, false, false },
        { L"Nguồn cung cấp", 160, false, false },
        { L"Trạng thái", 110, false, true }
    };

    if (rep.findings.empty()) {
        dtc.emptyMessage = L"Chưa có bằng chứng nào được thu thập. Hãy thực hiện phiên kiểm tra tự động trước.";
    } else {
        for (const auto& f : rep.findings) {
            TableRow row;
            row.cells.push_back(UiDimension(f.dimension));
            row.cells.push_back(f.group + L" • " + f.name);
            row.cells.push_back(f.value);
            row.cells.push_back(f.expected.empty() ? L"—" : f.expected);
            row.cells.push_back(f.evidence.empty() ? L"Trích xuất Native" : f.evidence);
            row.cells.push_back(UiState(f.state));
            row.rowState = MapState(f.state);
            dtc.rows.push_back(row);
        }
    }
    DrawDataTable(dc, tableRect, dtc, gFonts, dpi, gTableScrollOffset);
}

void RenderReports(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Đánh giá cuối cùng & Báo cáo";
    hdr.subtitle = L"Tổng hợp bằng chứng kỹ thuật, phân tích rủi ro đa chiều và đưa ra khuyến nghị mua máy chuẩn xác.";
    hdr.sessionTag = gAuditReady ? FormatDecisionVi(rep.hardware.stress.decision.overall) : L"Chưa có kết luận";
    hdr.sessionState = gAuditReady ? CanonicalUiState::Pass : CanonicalUiState::Incomplete;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int rightPanelW = UiMetrics::Scale(300, dpi);
    int leftW = r.right - r.left - UiMetrics::Scale(48, dpi) - rightPanelW;
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Decision Banner Card (C05)
    RECT bannerRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW, curY + UiMetrics::Scale(100, dpi) };
    DrawRoundedCard(dc, bannerRect, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    SelectObject(dc, gFonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, bannerRect.left + UiMetrics::Scale(16, dpi), bannerRect.top + UiMetrics::Scale(12, dpi), L"KHUYẾN NGHỊ KIỂM ĐỊNH LAPSURE:", 30);

    SelectObject(dc, gFonts.hTitle);
    COLORREF decClr = (rep.hardware.stress.decision.overall == L"BUY") ? UiColors::SuccessGreen :
                      ((rep.hardware.stress.decision.overall == L"BUY WITH NOTES") ? UiColors::WarnAmber :
                      ((rep.hardware.stress.decision.overall == L"REJECT") ? UiColors::FailRed : UiColors::TextMuted));
    SetTextColor(dc, decClr);
    std::wstring decText = gAuditReady ? FormatDecisionVi(rep.hardware.stress.decision.overall) : L"CHƯA ĐỦ DỮ LIỆU ĐỂ KẾT LUẬN";
    TextOutW(dc, bannerRect.left + UiMetrics::Scale(16, dpi), bannerRect.top + UiMetrics::Scale(34, dpi), decText.c_str(), (int)decText.size());

    SelectObject(dc, gFonts.hSmall);
    SetTextColor(dc, UiColors::TextMain);
    std::wstring decDetail = gAuditReady ? (L"Dựa trên " + std::to_wstring(rep.findings.size()) + L" bằng chứng kỹ thuật đã thu thập từ phần cứng.") : L"Yêu cầu hoàn tất toàn bộ các bài kiểm tra bắt buộc trước khi đưa ra quyết định.";
    TextOutW(dc, bannerRect.left + UiMetrics::Scale(16, dpi), bannerRect.top + UiMetrics::Scale(68, dpi), decDetail.c_str(), (int)decDetail.size());

    curY += UiMetrics::Scale(110, dpi);

    // Summary Findings Table (C09)
    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW, r.bottom - UiMetrics::Scale(20, dpi) };
    DataTableConfig dtc;
    dtc.columns = {
        { L"Phân hệ", 110, false, false },
        { L"Hạng mục kiểm tra", 140, false, false },
        { L"Thông số phát hiện", 160, false, false },
        { L"Giá trị thực tế", 180, true, false },
        { L"Trạng thái", 100, false, true }
    };

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
    DrawDataTable(dc, tableRect, dtc, gFonts, dpi, gTableScrollOffset);

    // Right Rail: Action panel to export report
    int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT actionCard{ rightX, r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(300, dpi) };
    NextActionConfig nac;
    nac.actionTitle = L"Xuất báo cáo kiểm định";
    nac.reasonText = L"Báo cáo sẽ được lưu dưới định dạng HTML có chữ ký số và gói dữ liệu JSON để gửi cho người bán hoặc lưu trữ.";
    nac.remainingTasks = { L"Xuất Báo cáo HTML Trực quan", L"Xuất Gói Bằng chứng JSON", L"Mở xem ngay trên trình duyệt" };
    nac.buttonText = L"XUẤT & XEM BÁO CÁO";
    nac.isButtonEnabled = gAuditReady;
    DrawNextActionPanel(dc, actionCard, nac, gFonts, dpi);
}

void RenderExportShare(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    (void)rep;
    PageHeaderConfig hdr;
    hdr.title = L"Xuất báo cáo & Chia sẻ";
    hdr.subtitle = L"Xuất báo cáo định dạng HTML và gói dữ liệu JSON có chữ ký số bằng chứng phục vụ lưu trữ hoặc gửi người bán.";
    hdr.sessionTag = gReportPath.empty() ? L"Chưa xuất file" : L"Đã sẵn sàng";
    hdr.sessionState = gReportPath.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Pass;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int rightPanelW = UiMetrics::Scale(300, dpi);
    int leftW = r.right - r.left - UiMetrics::Scale(48, dpi) - rightPanelW;
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Export Package Card 1: HTML Report
    RECT htmlCard{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW, curY + UiMetrics::Scale(120, dpi) };
    DrawRoundedCard(dc, htmlCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    SelectObject(dc, gFonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, htmlCard.left + UiMetrics::Scale(14, dpi), htmlCard.top + UiMetrics::Scale(12, dpi), L"📄 Báo cáo Kiểm định Trực quan (HTML Format)", 44);

    SelectObject(dc, gFonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, htmlCard.left + UiMetrics::Scale(14, dpi), htmlCard.top + UiMetrics::Scale(36, dpi), L"Định dạng đầy đủ màu sắc, biểu đồ đo đạc, chữ ký mã băm và các bảng đối chiếu cấu hình.", 88);

    SetTextColor(dc, UiColors::PrimaryBlue);
    std::wstring pathStr = gReportPath.empty() ? L"Đường dẫn: Sẽ được tạo tự động trong thư mục reports/" : (L"Tệp: " + gReportPath);
    TextOutW(dc, htmlCard.left + UiMetrics::Scale(14, dpi), htmlCard.top + UiMetrics::Scale(60, dpi), pathStr.c_str(), (int)pathStr.size());

    curY += UiMetrics::Scale(135, dpi);

    // Export Package Card 2: JSON Package
    RECT jsonCard{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW, curY + UiMetrics::Scale(120, dpi) };
    DrawRoundedCard(dc, jsonCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    SelectObject(dc, gFonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, jsonCard.left + UiMetrics::Scale(14, dpi), jsonCard.top + UiMetrics::Scale(12, dpi), L"🗂️ Gói Dữ liệu Kỹ thuật Toàn diện (JSON Evidence)", 49);

    SelectObject(dc, gFonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    TextOutW(dc, jsonCard.left + UiMetrics::Scale(14, dpi), jsonCard.top + UiMetrics::Scale(36, dpi), L"Dữ liệu máy đọc chuẩn hóa, telemetry chi tiết từng giây và toàn bộ log forensic thô.", 84);

    SetTextColor(dc, UiColors::PrimaryBlue);
    TextOutW(dc, jsonCard.left + UiMetrics::Scale(14, dpi), jsonCard.top + UiMetrics::Scale(60, dpi), L"Định dạng: Chuẩn Schema LapSure v0.1.1 có mã băm SHA-256", 55);

    // Right Rail: Action panel to open report
    int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT actionCard{ rightX, r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(260, dpi) };
    NextActionConfig nac;
    nac.actionTitle = L"Xem tệp báo cáo";
    nac.reasonText = L"Mở trực tiếp báo cáo HTML trên trình duyệt mặc định của hệ thống.";
    nac.remainingTasks = { L"In ấn ra PDF", L"Gửi báo cáo cho người bán", L"Lưu trữ hồ sơ thiết bị" };
    nac.buttonText = L"MỞ TRÊN TRÌNH DUYỆT";
    nac.isButtonEnabled = !gReportPath.empty();
    DrawNextActionPanel(dc, actionCard, nac, gFonts, dpi);
}

void RenderLogsEvents(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    (void)rep;
    PageHeaderConfig hdr;
    hdr.title = L"Nhật ký & Sự kiện";
    hdr.subtitle = L"Toàn bộ nhật ký hệ thống, sự kiện WHEA mã lỗi phần cứng và lịch sử vận hành chi tiết.";
    hdr.sessionTag = std::to_wstring(gLiveLogs.size()) + L" bản ghi nhật ký";
    hdr.sessionState = CanonicalUiState::Good;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int curY = r.top + UiMetrics::Scale(70, dpi);
    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi) };

    DataTableConfig dtc;
    dtc.columns = {
        { L"Thời gian", 120, false, false },
        { L"Nguồn sự kiện", 150, false, false },
        { L"Nội dung bản ghi chi tiết", 400, true, false },
        { L"Mức độ", 100, false, true }
    };

    std::lock_guard<std::mutex> lk(gLogsMutex);
    if (gLiveLogs.empty()) {
        dtc.emptyMessage = L"Chưa có bản ghi nhật ký nào. Hãy bắt đầu một phiên kiểm tra để theo dõi.";
    } else {
        for (int i = (int)gLiveLogs.size() - 1; i >= 0; --i) {
            TableRow row;
            row.cells.push_back(gLiveLogs[i].time);
            row.cells.push_back(L"Chẩn đoán LapSure");
            row.cells.push_back(gLiveLogs[i].message);
            row.cells.push_back(L"THÔNG TIN");
            row.rowState = CanonicalUiState::Good;
            dtc.rows.push_back(row);
        }
    }
    DrawDataTable(dc, tableRect, dtc, gFonts, dpi, gTableScrollOffset);
}

void RenderSettings(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    (void)rep;
    PageHeaderConfig hdr;
    hdr.title = L"Cài đặt";
    hdr.subtitle = L"Cấu hình tùy chọn phần mềm, thư mục lưu trữ báo cáo, chính sách mã băm tin cậy và giao diện.";
    hdr.sessionTag = L"Phiên bản 0.1.1 (Beta)";
    hdr.sessionState = CanonicalUiState::Pass;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int rightPanelW = UiMetrics::Scale(300, dpi);
    int leftW = r.right - r.left - UiMetrics::Scale(48, dpi) - rightPanelW;
    int curY = r.top + UiMetrics::Scale(70, dpi);

    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW, r.bottom - UiMetrics::Scale(20, dpi) };
    
    DataTableConfig dtc;
    dtc.columns = {
        { L"Tùy chọn cấu hình", 180, false, false },
        { L"Giá trị thiết lập hiện tại", 260, true, false },
        { L"Trạng thái", 120, false, true }
    };

    TableRow r1;
    r1.cells = { L"Ngôn ngữ giao diện", L"Tiếng Việt (Mặc định)", L"HOẠT ĐỘNG" };
    r1.rowState = CanonicalUiState::Good;
    dtc.rows.push_back(r1);

    TableRow r2;
    r2.cells = { L"Chế độ kiểm tra mặc định", L"Tiêu chuẩn (Standard) — 3 phút", L"HOẠT ĐỘNG" };
    r2.rowState = CanonicalUiState::Good;
    dtc.rows.push_back(r2);

    TableRow r3;
    r3.cells = { L"Thư mục xuất báo cáo", L"reports/ (Thư mục cục bộ ứng dụng)", L"ĐÃ ĐỊNH TUYẾN" };
    r3.rowState = CanonicalUiState::Good;
    dtc.rows.push_back(r3);

    TableRow r4;
    r4.cells = { L"Chính sách bảo mật SHA-256", L"Kiểm tra chữ ký số engine chẩn đoán", L"BẢO VỆ BẬT" };
    r4.rowState = CanonicalUiState::Good;
    dtc.rows.push_back(r4);

    TableRow r5;
    r5.cells = { L"Hỗ trợ màn hình độ phân giải cao", L"Per-Monitor V2 DPI Scaling", L"TỰ ĐỘNG" };
    r5.rowState = CanonicalUiState::Good;
    dtc.rows.push_back(r5);

    DrawDataTable(dc, tableRect, dtc, gFonts, dpi);

    int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT actionCard{ rightX, curY, r.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(240, dpi) };
    NextActionConfig nac;
    nac.actionTitle = L"Cấu hình hệ thống";
    nac.reasonText = L"LapSure vận hành độc lập không phụ thuộc Internet, lưu trữ toàn bộ dữ liệu an toàn tại máy.";
    nac.remainingTasks = { L"Thiết lập chế độ kiểm định", L"Tùy chỉnh biểu mẫu cam kết", L"Kiểm tra trạng thái bản quyền" };
    nac.buttonText = L"LƯU THIẾT LẬP";
    nac.isButtonEnabled = true;
    DrawNextActionPanel(dc, actionCard, nac, gFonts, dpi);
}

void RenderSessionHistory(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Lịch sử phiên kiểm định";
    hdr.subtitle = L"Danh sách các phiên kiểm định máy đã thực hiện trên thiết bị này và báo cáo lưu trữ.";
    hdr.sessionTag = gAuditReady ? L"1 phiên hoàn tất" : L"Chưa có phiên lưu";
    hdr.sessionState = gAuditReady ? CanonicalUiState::Pass : CanonicalUiState::NotTested;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int curY = r.top + UiMetrics::Scale(70, dpi);
    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi) };

    DataTableConfig dtc;
    dtc.columns = {
        { L"Thời gian kiểm định", 150, false, false },
        { L"Thiết bị / Model máy", 220, true, false },
        { L"Chế độ kiểm tra", 130, false, false },
        { L"Kết luận khuyến nghị", 160, false, false },
        { L"Tệp Báo cáo", 130, false, true }
    };

    if (gAuditReady) {
        TableRow row;
        row.cells.push_back(L"Hôm nay (Phiên hiện tại)");
        std::wstring m = rep.model.empty() ? Reg(L"SystemProductName") : rep.model;
        row.cells.push_back(m);
        row.cells.push_back(gSelectedMode);
        row.cells.push_back(FormatDecisionVi(rep.hardware.stress.decision.overall));
        row.cells.push_back(gReportPath.empty() ? L"Chưa xuất" : L"ĐÃ XUẤT HTML");
        row.rowState = CanonicalUiState::Pass;
        dtc.rows.push_back(row);
    } else {
        dtc.emptyMessage = L"Chưa có lịch sử phiên kiểm định nào được ghi nhận.";
    }

    DrawDataTable(dc, tableRect, dtc, gFonts, dpi, gTableScrollOffset);
}

void RenderInterruptedRecovery(HDC dc, const RECT& r, const AuditReport& rep, int dpi) {
    PageHeaderConfig hdr;
    hdr.title = L"Khôi phục Phiên bị Gián đoạn";
    hdr.subtitle = L"Phát hiện phiên kiểm định trước đó chưa hoàn tất (có thể do mất nguồn đột ngột, quá nhiệt hoặc ứng dụng bị dừng).";
    hdr.sessionTag = L"Phát hiện gián đoạn";
    hdr.sessionState = CanonicalUiState::Interrupted;
    DrawPageHeader(dc, r, hdr, gFonts, dpi);

    int rightPanelW = UiMetrics::Scale(300, dpi);
    int leftW = r.right - r.left - UiMetrics::Scale(48, dpi) - rightPanelW;
    int curY = r.top + UiMetrics::Scale(70, dpi);

    // Warning Banner Card (C12/C11)
    RECT warnCard{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW, curY + UiMetrics::Scale(85, dpi) };
    DrawRoundedCard(dc, warnCard, UiMetrics::RadiusMd, RGB(254, 242, 242), RGB(254, 202, 202), 1);

    SelectObject(dc, gFonts.hBodyBold); SetTextColor(dc, UiColors::FailRed);
    TextOutW(dc, warnCard.left + UiMetrics::Scale(14, dpi), warnCard.top + UiMetrics::Scale(10, dpi), L"⚠️ PHÁT HIỆN NHẬT KÝ KIỂM TRA CHƯA KẾT THÚC HOÀN TOÀN", 52);

    SelectObject(dc, gFonts.hSmall); SetTextColor(dc, UiColors::TextMain);
    std::wstring warnExpl = L"Phiên kiểm định trước đó bị gián đoạn giữa chừng. Theo nguyên tắc bảo toàn bằng chứng (Evidence First), dữ liệu dở dang KHÔNG BAO GIỜ tự động trở thành ĐẠT.";
    RECT explR{ warnCard.left + UiMetrics::Scale(14, dpi), warnCard.top + UiMetrics::Scale(32, dpi), warnCard.right - UiMetrics::Scale(14, dpi), warnCard.bottom - UiMetrics::Scale(8, dpi) };
    DrawTextW(dc, warnExpl.c_str(), (int)warnExpl.size(), &explR, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);

    curY += UiMetrics::Scale(95, dpi);

    // Journal & Stage Status Table (C09)
    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW, r.bottom - UiMetrics::Scale(20, dpi) };
    DataTableConfig dtc;
    dtc.columns = {
        { L"Hạng mục ghi nhận", 160, false, false },
        { L"Dữ liệu nhật ký / Tệp Journal", 280, true, false },
        { L"Trạng thái khôi phục", 140, false, true }
    };

    TableRow r1;
    r1.cells = { L"Tệp nhật ký Journal", rep.hardware.stress.journalPath.empty() ? L"journal.jsonl (Tự động ghi)" : rep.hardware.stress.journalPath, L"ĐÃ BẢO LƯU" };
    r1.rowState = CanonicalUiState::Good;
    dtc.rows.push_back(r1);

    TableRow r2;
    r2.cells = { L"Dữ liệu phần cứng đã quét", rep.model.empty() ? L"Chưa nhận diện" : (rep.model + L" (" + std::to_wstring(rep.findings.size()) + L" bằng chứng)"), L"CÓ THỂ KHÔI PHỤC" };
    r2.rowState = rep.findings.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Good;
    dtc.rows.push_back(r2);

    TableRow r3;
    r3.cells = { L"Bài kiểm tra Stress", rep.hardware.stress.completed ? L"Đã hoàn thành trước khi dừng" : L"Bị gián đoạn giữa chừng", rep.hardware.stress.completed ? L"ĐẠT" : L"GIÁN ĐOẠN" };
    r3.rowState = rep.hardware.stress.completed ? CanonicalUiState::Good : CanonicalUiState::Interrupted;
    dtc.rows.push_back(r3);

    TableRow r4;
    r4.cells = { L"Kết luận cuối cùng", L"Chưa đủ dữ liệu để kết luận (INCOMPLETE)", L"CHƯA KẾT THÚC" };
    r4.rowState = CanonicalUiState::Incomplete;
    dtc.rows.push_back(r4);

    DrawDataTable(dc, tableRect, dtc, gFonts, dpi, gTableScrollOffset);

    // Right Rail: Next Action Panel (C10)
    int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT actionCard{ rightX, r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(310, dpi) };
    NextActionConfig nac;
    nac.actionTitle = L"Tùy chọn xử lý phiên";
    nac.reasonText = L"Bạn có thể tiếp tục hoàn tất các bài kiểm tra còn dở dang hoặc khởi tạo một phiên kiểm tra hoàn toàn mới.";
    nac.remainingTasks = { L"Tiếp tục quét các mục còn lại", L"Chạy lại từ đầu phiên mới", L"Đóng và lưu tệp nhật ký" };
    nac.buttonText = L"CHẠY LẠI PHIÊN KIỂM TRA";
    nac.isButtonEnabled = true;
    DrawNextActionPanel(dc, actionCard, nac, gFonts, dpi);
}

void RenderGenericScreen(HDC dc, const RECT& r, MainTab tab, const AuditReport& rep, int dpi) {
    (void)rep;
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
    case MainTab::EvidenceLibrary:
        hdr.title = L"Thư viện Bằng chứng";
        hdr.subtitle = L"Toàn bộ bằng chứng chẩn đoán phần cứng đã trích xuất, phục vụ đối chiếu và truy xuất nguồn gốc.";
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
    case MainTab::InterruptedRecovery:
        hdr.title = L"Khôi phục Phiên bị Gián đoạn";
        hdr.subtitle = L"Xử lý và khôi phục an toàn phiên kiểm định bị dừng đột ngột.";
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

std::vector<MainTab> GetVisualTabList() {
    std::vector<MainTab> list;
    auto groups = GetDefaultSidebarGroups(gDeviceGroupExpanded);
    for (const auto& g : groups) {
        if (g.isExpanded) {
            for (const auto& it : g.items) {
                list.push_back(it.tab);
            }
        }
    }
    return list;
}

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
        if(gRunning && !gPaused) {
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
                auto tabs = GetVisualTabList();
                for (size_t i = 0; i < tabs.size(); ++i) {
                    if (tabs[i] == gCurrentTab) {
                        if (i > 0) gCurrentTab = tabs[i - 1];
                        break;
                    }
                }
                InvalidateRect(h, nullptr, FALSE);
            }
            return 0;
        case VK_DOWN:
            if (gFocusIndex == 0) {
                auto tabs = GetVisualTabList();
                for (size_t i = 0; i < tabs.size(); ++i) {
                    if (tabs[i] == gCurrentTab) {
                        if (i + 1 < tabs.size()) gCurrentTab = tabs[i + 1];
                        break;
                    }
                }
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
    case WM_SIZE:
        InvalidateRect(h, nullptr, FALSE);
        return 0;
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

        // 3. Mode Pills Click (with DPI-scaled gap)
        int mX = layout.contentRect.left + UiMetrics::Scale(134, dpi);
        int pillW = UiMetrics::Scale(80, dpi);
        int pillH = UiMetrics::Scale(28, dpi);
        int gap = UiMetrics::Scale(6, dpi);
        if (y >= modeY && y <= modeY + pillH) {
            if (x >= mX && x <= mX + pillW) { gSelectedMode = L"Quick"; InvalidateRect(h, nullptr, FALSE); }
            else if (x >= mX + pillW + gap && x <= mX + (pillW + gap) * 2) { gSelectedMode = L"Standard"; InvalidateRect(h, nullptr, FALSE); }
            else if (x >= mX + (pillW + gap) * 2 && x <= mX + (pillW + gap) * 3) { gSelectedMode = L"Deep"; InvalidateRect(h, nullptr, FALSE); }
        }

        // 3.1 S01 Dashboard Specific Click Hit-Tests
        if (gCurrentTab == MainTab::Dashboard) {
            int rightPanelW = UiMetrics::Scale(260, dpi);
            int rightX = cr.right - rightPanelW - UiMetrics::Scale(24, dpi);
            int mainW = rightX - layout.contentRect.left - UiMetrics::Scale(36, dpi);
            int kpiY = modeY + UiMetrics::Scale(44, dpi);
            RECT gaugeCard{ rightX, kpiY, cr.right - UiMetrics::Scale(24, dpi), kpiY + UiMetrics::Scale(140, dpi) };
            RECT facCard{ rightX, gaugeCard.bottom + UiMetrics::Scale(10, dpi), cr.right - UiMetrics::Scale(24, dpi), gaugeCard.bottom + UiMetrics::Scale(85, dpi) };
            RECT stepCard{ rightX, facCard.bottom + UiMetrics::Scale(10, dpi), cr.right - UiMetrics::Scale(24, dpi), cr.bottom - UiMetrics::Scale(20, dpi) };
            
            // Hit test Stepper Next Button
            RECT nextBtnRect{ stepCard.left + UiMetrics::Scale(12, dpi), stepCard.bottom - UiMetrics::Scale(40, dpi), stepCard.right - UiMetrics::Scale(12, dpi), stepCard.bottom - UiMetrics::Scale(10, dpi) };
            if (x >= nextBtnRect.left && x <= nextBtnRect.right && y >= nextBtnRect.top && y <= nextBtnRect.bottom) {
                if (gRunning) {
                    gCancel = true;
                    gSessionLifecycleState = CanonicalUiState::Cancelled;
                } else if (gAuditCompletedItems == 0) {
                    StartAudit(h);
                } else if (gAuditCompletedItems >= gAuditTotalItems && gReport.hardware.stress.functional.overall != L"PASS") {
                    gCurrentTab = MainTab::Functional;
                    InvalidateRect(h, nullptr, FALSE);
                } else if (gReport.hardware.stress.functional.overall == L"PASS" && gReport.hardware.stress.portPower.overall != L"PASS") {
                    gCurrentTab = MainTab::PortsPower;
                    InvalidateRect(h, nullptr, FALSE);
                } else if (gAuditReady) {
                    gCurrentTab = MainTab::Reports;
                    InvalidateRect(h, nullptr, FALSE);
                } else {
                    StartAudit(h);
                }
                return 0;
            }

            // Hit test 14 Domain Grid cards
            int gridY = kpiY + UiMetrics::Scale(82, dpi) + UiMetrics::Scale(14, dpi);
            int cardCols = 4;
            int cellW = (mainW - (cardCols - 1) * UiMetrics::Scale(10, dpi)) / cardCols;
            int cellH = UiMetrics::Scale(54, dpi);
            int startGridY = gridY + UiMetrics::Scale(26, dpi);
            const MainTab domainTabs[] = {
                MainTab::SystemInfo,           // 0: Nhận diện hệ thống
                MainTab::Memory,               // 1: Bộ nhớ RAM
                MainTab::Storage,              // 2: Lưu trữ
                MainTab::Battery,              // 3: Pin & Nguồn
                MainTab::SystemInfo,           // 4: Đồ họa GPU
                MainTab::Display,              // 5: Hiển thị
                MainTab::Functional,           // 6: Bàn phím & Touchpad
                MainTab::AudioCamera,          // 7: Âm thanh & Cam
                MainTab::Network,              // 8: Mạng & Kết nối
                MainTab::PortsPower,           // 9: Cổng & Nguồn
                MainTab::Stress,               // 10: Stress & Ổn định
                MainTab::LogsEvents,           // 11: Nhật ký & Sự kiện
                MainTab::FactoryProfileMatch,  // 12: Hồ sơ & Đối chiếu
                MainTab::Reports               // 13: Độ bao phủ & Tin cậy
            };
            for (int i = 0; i < 14; ++i) {
                int row = i / cardCols;
                int col = i % cardCols;
                int cx = layout.contentRect.left + UiMetrics::Scale(24, dpi) + col * (cellW + UiMetrics::Scale(10, dpi));
                int cy = startGridY + row * (cellH + UiMetrics::Scale(8, dpi));
                RECT crCell{ cx, cy, cx + cellW, cy + cellH };
                if (x >= crCell.left && x <= crCell.right && y >= crCell.top && y <= crCell.bottom) {
                    gCurrentTab = domainTabs[i];
                    gTableScrollOffset = 0;
                    InvalidateRect(h, nullptr, FALSE);
                    return 0;
                }
            }
        }

        // 4. NewSession Screen: Purpose Cards, Mode Cards & Start Inspection Button Hit-Test
        if (gCurrentTab == MainTab::NewSession) {
            int rightPanelW = UiMetrics::Scale(300, dpi);
            int leftW = cr.right - cr.left - UiMetrics::Scale(48, dpi) - rightPanelW;
            int curY = layout.contentRect.top + UiMetrics::Scale(70, dpi) + UiMetrics::Scale(24, dpi);
            int startX = layout.contentRect.left + UiMetrics::Scale(24, dpi);

            // Hit test Purpose cards (3 cards)
            for (int p = 0; p < 3; ++p) {
                RECT pr{ startX, curY, startX + leftW, curY + UiMetrics::Scale(64, dpi) };
                if (x >= pr.left && x <= pr.right && y >= pr.top && y <= pr.bottom) {
                    gInspectionPurpose = p;
                    InvalidateRect(h, nullptr, FALSE);
                    return 0;
                }
                curY += UiMetrics::Scale(72, dpi);
            }
            curY += UiMetrics::Scale(8, dpi) + UiMetrics::Scale(24, dpi);

            // Hit test Mode cards (3 cards)
            int modeCardW = (leftW - UiMetrics::Scale(16, dpi)) / 3;
            const wchar_t* modeNames[] = { L"Quick", L"Standard", L"Deep" };
            for (int modeIdx = 0; modeIdx < 3; ++modeIdx) {
                int mx = startX + modeIdx * (modeCardW + UiMetrics::Scale(8, dpi));
                RECT mr{ mx, curY, mx + modeCardW, curY + UiMetrics::Scale(110, dpi) };
                if (x >= mr.left && x <= mr.right && y >= mr.top && y <= mr.bottom) {
                    gSelectedMode = modeNames[modeIdx];
                    InvalidateRect(h, nullptr, FALSE);
                    return 0;
                }
            }

            // Hit test Action Button
            int rightX = cr.right - rightPanelW - UiMetrics::Scale(24, dpi);
            int rightY = layout.contentRect.top + UiMetrics::Scale(70, dpi);
            RECT preflightCard{ rightX, rightY, cr.right - UiMetrics::Scale(24, dpi), rightY + UiMetrics::Scale(220, dpi) };
            RECT actionCard{ rightX, preflightCard.bottom + UiMetrics::Scale(12, dpi), cr.right - UiMetrics::Scale(24, dpi), preflightCard.bottom + UiMetrics::Scale(180, dpi) };
            int actH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
            RECT br{ actionCard.left + UiMetrics::Scale(14, dpi), actionCard.bottom - actH - UiMetrics::Scale(12, dpi), actionCard.right - UiMetrics::Scale(14, dpi), actionCard.bottom - UiMetrics::Scale(12, dpi) };
            if (x >= br.left && x <= br.right && y >= br.top && y <= br.bottom) {
                StartAudit(h);
                return 0;
            }
        }

        // 5. AutoAudit Screen: Pause Button, Cancel Button & Action Button Hit-Test
        if (gCurrentTab == MainTab::AutoAudit) {
            int rightPanelW = std::clamp((int)((cr.right - cr.left) * 26 / 100), UiMetrics::Scale(260, dpi), UiMetrics::Scale(340, dpi));
            int rightX = cr.right - rightPanelW - UiMetrics::Scale(20, dpi);
            int mainW = rightX - layout.contentRect.left - UiMetrics::Scale(32, dpi);
            int curY = layout.contentRect.top + UiMetrics::Scale(70, dpi);
            int pCardH = UiMetrics::Scale(54, dpi);
            RECT pCard{ layout.contentRect.left + UiMetrics::Scale(24, dpi), curY, layout.contentRect.left + UiMetrics::Scale(24, dpi) + mainW, curY + pCardH };

            int pBtnW = UiMetrics::Scale(85, dpi);
            int pBtnH = UiMetrics::Scale(28, dpi);
            RECT pauseBtn{ pCard.right - pBtnW - UiMetrics::Scale(10, dpi), pCard.top + (pCardH - pBtnH) / 2, pCard.right - UiMetrics::Scale(10, dpi), pCard.top + (pCardH - pBtnH) / 2 + pBtnH };
            if (x >= pauseBtn.left && x <= pauseBtn.right && y >= pauseBtn.top && y <= pauseBtn.bottom) {
                if (gRunning) {
                    gPaused = !gPaused;
                    if (gPaused) {
                        gSessionLifecycleState = CanonicalUiState::Paused;
                        PostStatus(h, L"Đã tạm dừng quy trình kiểm tra tự động.");
                    } else {
                        gSessionLifecycleState = CanonicalUiState::Running;
                        PostStatus(h, L"Tiếp tục quy trình kiểm tra tự động.");
                    }
                    InvalidateRect(h, nullptr, FALSE);
                } else if (gAuditCompletedItems == 0) {
                    StartAudit(h);
                }
                return 0;
            }

            RECT timeCard{ rightX, layout.contentRect.top + UiMetrics::Scale(70, dpi), cr.right - UiMetrics::Scale(20, dpi), layout.contentRect.top + UiMetrics::Scale(160, dpi) };
            RECT guideCard{ rightX, timeCard.bottom + UiMetrics::Scale(10, dpi), cr.right - UiMetrics::Scale(20, dpi), cr.bottom - UiMetrics::Scale(16, dpi) };
            int cBtnH = UiMetrics::Scale(36, dpi);
            RECT cancelAuditBtn{ guideCard.left + UiMetrics::Scale(12, dpi), guideCard.bottom - cBtnH - UiMetrics::Scale(12, dpi), guideCard.right - UiMetrics::Scale(12, dpi), guideCard.bottom - UiMetrics::Scale(12, dpi) };
            if (x >= cancelAuditBtn.left && x <= cancelAuditBtn.right && y >= cancelAuditBtn.top && y <= cancelAuditBtn.bottom) {
                if (gRunning) {
                    gCancel = true;
                    gPaused = false;
                    gSessionLifecycleState = CanonicalUiState::Cancelled;
                    PostStatus(h, L"Đã yêu cầu hủy kiểm tra...");
                } else {
                    StartAudit(h);
                }
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
        }

        // 6. Functional Screen: Interactive Module Cards & Next Action Button Hit-Test
        if (gCurrentTab == MainTab::Functional) {
            int rightPanelW = UiMetrics::Scale(250, dpi);
            int leftW = cr.right - cr.left - UiMetrics::Scale(48, dpi) - rightPanelW;
            int curY = layout.contentRect.top + UiMetrics::Scale(135, dpi);
            int startX = layout.contentRect.left + UiMetrics::Scale(24, dpi);
            int cardW = (leftW - UiMetrics::Scale(12, dpi)) / 2;
            int cardH = UiMetrics::Scale(95, dpi);

            int cmdIds[] = { 1201, 1202, 1204, 1206, 1207, 1208 };
            for (int i = 0; i < 6; ++i) {
                int col = i % 2;
                int row = i / 2;
                int cx = startX + col * (cardW + UiMetrics::Scale(12, dpi));
                int cy = curY + row * (cardH + UiMetrics::Scale(10, dpi));
                RECT cardR{ cx, cy, cx + cardW, cy + cardH };
                if (x >= cardR.left && x <= cardR.right && y >= cardR.top && y <= cardR.bottom) {
                    PostMessageW(h, WM_COMMAND, cmdIds[i], 0);
                    return 0;
                }
            }

            int rightX = cr.right - rightPanelW - UiMetrics::Scale(24, dpi);
            RECT checkCard{ rightX, curY, cr.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(305, dpi) };
            int actH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
            RECT br{ checkCard.left + UiMetrics::Scale(14, dpi), checkCard.bottom - actH - UiMetrics::Scale(12, dpi), checkCard.right - UiMetrics::Scale(14, dpi), checkCard.bottom - UiMetrics::Scale(12, dpi) };
            if (x >= br.left && x <= br.right && y >= br.top && y <= br.bottom) {
                PostMessageW(h, WM_COMMAND, 1300, 0);
                return 0;
            }
        }

        // 7. Dashboard Screen: Factory Compare Button Hit-Test
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

        // 8. Seller Claim Screen: Edit Claim Button Hit-Test
        if (gCurrentTab == MainTab::SellerClaim) {
            int rightPanelW = UiMetrics::Scale(300, dpi);
            int rightX = cr.right - rightPanelW - UiMetrics::Scale(24, dpi);
            int curY = layout.contentRect.top + UiMetrics::Scale(70, dpi);
            RECT actionCard{ rightX, curY, cr.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(240, dpi) };
            int actH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
            RECT br{ actionCard.left + UiMetrics::Scale(14, dpi), actionCard.bottom - actH - UiMetrics::Scale(12, dpi), actionCard.right - UiMetrics::Scale(14, dpi), actionCard.bottom - UiMetrics::Scale(12, dpi) };
            if (x >= br.left && x <= br.right && y >= br.top && y <= br.bottom) {
                PostMessageW(h, WM_COMMAND, 1209, 0);
                return 0;
            }
        }

        // 9. Physical Safety Screen: Open Wizard Button Hit-Test
        if (gCurrentTab == MainTab::PhysicalSafety) {
            int rightPanelW = UiMetrics::Scale(300, dpi);
            int rightX = cr.right - rightPanelW - UiMetrics::Scale(24, dpi);
            int curY = layout.contentRect.top + UiMetrics::Scale(70, dpi);
            RECT actionCard{ rightX, curY, cr.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(310, dpi) };
            int actH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
            RECT br{ actionCard.left + UiMetrics::Scale(14, dpi), actionCard.bottom - actH - UiMetrics::Scale(12, dpi), actionCard.right - UiMetrics::Scale(14, dpi), actionCard.bottom - UiMetrics::Scale(12, dpi) };
            if (x >= br.left && x <= br.right && y >= br.top && y <= br.bottom) {
                PostMessageW(h, WM_COMMAND, 1208, 0);
                return 0;
            }
        }

        // 10. Ports & Power Screen: Probe Port Button Hit-Test
        if (gCurrentTab == MainTab::PortsPower) {
            int rightPanelW = UiMetrics::Scale(300, dpi);
            int rightX = cr.right - rightPanelW - UiMetrics::Scale(24, dpi);
            int curY = layout.contentRect.top + UiMetrics::Scale(70, dpi);
            RECT actionCard{ rightX, curY, cr.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(280, dpi) };
            int actH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
            RECT br{ actionCard.left + UiMetrics::Scale(14, dpi), actionCard.bottom - actH - UiMetrics::Scale(12, dpi), actionCard.right - UiMetrics::Scale(14, dpi), actionCard.bottom - UiMetrics::Scale(12, dpi) };
            if (x >= br.left && x <= br.right && y >= br.top && y <= br.bottom) {
                PostMessageW(h, WM_COMMAND, 1207, 0);
                return 0;
            }
        }

        // 11. Stress Screen: Start/Stop Stress Button Hit-Test
        if (gCurrentTab == MainTab::Stress) {
            int rightPanelW = UiMetrics::Scale(300, dpi);
            int rightX = cr.right - rightPanelW - UiMetrics::Scale(24, dpi);
            int curY = layout.contentRect.top + UiMetrics::Scale(70, dpi);
            RECT actionCard{ rightX, curY, cr.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(280, dpi) };
            int actH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
            RECT br{ actionCard.left + UiMetrics::Scale(14, dpi), actionCard.bottom - actH - UiMetrics::Scale(12, dpi), actionCard.right - UiMetrics::Scale(14, dpi), actionCard.bottom - UiMetrics::Scale(12, dpi) };
            if (x >= br.left && x <= br.right && y >= br.top && y <= br.bottom) {
                StartAudit(h);
                return 0;
            }
        }

        // 12. Reports Screen: Export Button Hit-Test
        if (gCurrentTab == MainTab::Reports) {
            int rightPanelW = UiMetrics::Scale(300, dpi);
            int rightX = cr.right - rightPanelW - UiMetrics::Scale(24, dpi);
            int curY = layout.contentRect.top + UiMetrics::Scale(70, dpi);
            RECT actionCard{ rightX, curY, cr.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(300, dpi) };
            int actH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
            RECT br{ actionCard.left + UiMetrics::Scale(14, dpi), actionCard.bottom - actH - UiMetrics::Scale(12, dpi), actionCard.right - UiMetrics::Scale(14, dpi), actionCard.bottom - UiMetrics::Scale(12, dpi) };
            if (x >= br.left && x <= br.right && y >= br.top && y <= br.bottom) {
                if (!gReportPath.empty()) ShellExecuteW(h, L"open", gReportPath.c_str(), nullptr, nullptr, SW_SHOW);
                else { gCurrentTab = MainTab::ExportShare; InvalidateRect(h, nullptr, FALSE); }
                return 0;
            }
        }

        // 13. Export & Share Screen: Open in Browser Button Hit-Test
        if (gCurrentTab == MainTab::ExportShare) {
            int rightPanelW = UiMetrics::Scale(300, dpi);
            int rightX = cr.right - rightPanelW - UiMetrics::Scale(24, dpi);
            int curY = layout.contentRect.top + UiMetrics::Scale(70, dpi);
            RECT actionCard{ rightX, curY, cr.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(260, dpi) };
            int actH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
            RECT br{ actionCard.left + UiMetrics::Scale(14, dpi), actionCard.bottom - actH - UiMetrics::Scale(12, dpi), actionCard.right - UiMetrics::Scale(14, dpi), actionCard.bottom - UiMetrics::Scale(12, dpi) };
            if (x >= br.left && x <= br.right && y >= br.top && y <= br.bottom) {
                if (!gReportPath.empty()) ShellExecuteW(h, L"open", gReportPath.c_str(), nullptr, nullptr, SW_SHOW);
                return 0;
            }
        }

        // 14. Display Screen: Open Wizard Button Hit-Test
        if (gCurrentTab == MainTab::Display) {
            int rightPanelW = UiMetrics::Scale(300, dpi);
            int rightX = cr.right - rightPanelW - UiMetrics::Scale(24, dpi);
            int curY = layout.contentRect.top + UiMetrics::Scale(70, dpi);
            RECT actionCard{ rightX, curY, cr.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(260, dpi) };
            int actH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
            RECT br{ actionCard.left + UiMetrics::Scale(14, dpi), actionCard.bottom - actH - UiMetrics::Scale(12, dpi), actionCard.right - UiMetrics::Scale(14, dpi), actionCard.bottom - UiMetrics::Scale(12, dpi) };
            if (x >= br.left && x <= br.right && y >= br.top && y <= br.bottom) {
                PostMessageW(h, WM_COMMAND, 1201, 0);
                return 0;
            }
        }

        // 15. Audio & Camera Screen: Open Wizard Button Hit-Test
        if (gCurrentTab == MainTab::AudioCamera) {
            int rightPanelW = UiMetrics::Scale(300, dpi);
            int rightX = cr.right - rightPanelW - UiMetrics::Scale(24, dpi);
            int curY = layout.contentRect.top + UiMetrics::Scale(70, dpi);
            RECT actionCard{ rightX, curY, cr.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(260, dpi) };
            int actH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
            RECT br{ actionCard.left + UiMetrics::Scale(14, dpi), actionCard.bottom - actH - UiMetrics::Scale(12, dpi), actionCard.right - UiMetrics::Scale(14, dpi), actionCard.bottom - UiMetrics::Scale(12, dpi) };
            if (x >= br.left && x <= br.right && y >= br.top && y <= br.bottom) {
                PostMessageW(h, WM_COMMAND, 1204, 0);
                return 0;
            }
        }

        // 16. Session History: select/open/delete actual local report records.
        if (gCurrentTab == MainTab::SessionHistory) {
            auto history = GetSessionHistorySnapshot();
            int rightW = UiMetrics::Scale(300, dpi);
            int gap2 = UiMetrics::Scale(12, dpi);
            RECT body{layout.contentRect.left + UiMetrics::Scale(24, dpi), layout.contentRect.top + UiMetrics::Scale(70, dpi),
                      layout.contentRect.right - UiMetrics::Scale(24, dpi), layout.contentRect.bottom - UiMetrics::Scale(20, dpi)};
            RECT tableRect{body.left, body.top, body.right - rightW - gap2, body.bottom};
            int rowH = UiMetrics::Scale(UiMetrics::TableRowHeight, dpi);
            if (!history.empty() && x >= tableRect.left && x <= tableRect.right && y >= tableRect.top + rowH && y < tableRect.bottom) {
                int visibleRow = (y - (tableRect.top + rowH)) / rowH;
                int idx = gTableScrollOffset + visibleRow;
                if (idx >= 0 && idx < static_cast<int>(history.size())) {
                    gHistorySelectedIndex = idx;
                    InvalidateRect(h, nullptr, FALSE);
                    return 0;
                }
            }
            if (!history.empty()) {
                int idx = std::clamp(gHistorySelectedIndex, 0, static_cast<int>(history.size()) - 1);
                const auto& selected = history[static_cast<size_t>(idx)];
                RECT detail{tableRect.right + gap2, body.top, body.right, body.bottom};
                int btnH2 = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
                RECT openBtn{detail.left + 14, detail.bottom - btnH2 * 2 - UiMetrics::Scale(24, dpi), detail.right - 14, detail.bottom - btnH2 - UiMetrics::Scale(18, dpi)};
                RECT deleteBtn{detail.left + 14, detail.bottom - btnH2 - UiMetrics::Scale(10, dpi), detail.right - 14, detail.bottom - UiMetrics::Scale(10, dpi)};
                if (x >= openBtn.left && x <= openBtn.right && y >= openBtn.top && y <= openBtn.bottom) {
                    const std::wstring path = !selected.htmlPath.empty() ? selected.htmlPath : (!selected.jsonPath.empty() ? selected.jsonPath : selected.evidencePath);
                    if (!path.empty()) ShellExecuteW(h, L"open", path.c_str(), nullptr, nullptr, SW_SHOW);
                    else MessageBoxW(h, L"Phiên này chưa có file report/evidence có thể mở.", L"LapSure", MB_OK | MB_ICONINFORMATION);
                    return 0;
                }
                if (x >= deleteBtn.left && x <= deleteBtn.right && y >= deleteBtn.top && y <= deleteBtn.bottom) {
                    int answer = MessageBoxW(h,
                        L"YES: xóa mục lịch sử VÀ các file report/evidence.\nNO: chỉ xóa mục khỏi index, giữ nguyên file.\nCANCEL: không thay đổi.",
                        L"Xóa phiên kiểm định", MB_YESNOCANCEL | MB_ICONWARNING);
                    if (answer == IDYES || answer == IDNO) {
                        DeleteSessionHistoryEntry(selected.sessionId, answer == IDYES);
                        auto after = GetSessionHistorySnapshot();
                        gHistorySelectedIndex = after.empty() ? 0 : std::min(gHistorySelectedIndex, static_cast<int>(after.size()) - 1);
                        InvalidateRect(h, nullptr, FALSE);
                    }
                    return 0;
                }
            }
        }

        // 17. Interrupted Recovery: preserve/discard real journal; interruption never becomes PASS.
        if (gCurrentTab == MainTab::InterruptedRecovery) {
            RECT body{layout.contentRect.left + UiMetrics::Scale(24, dpi), layout.contentRect.top + UiMetrics::Scale(70, dpi),
                      layout.contentRect.right - UiMetrics::Scale(24, dpi), layout.contentRect.bottom - UiMetrics::Scale(20, dpi)};
            int rightW = UiMetrics::Scale(330, dpi);
            RECT actions{body.right - rightW, body.top, body.right, body.bottom};
            int btnH2 = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
            int btnGap2 = UiMetrics::Scale(10, dpi);
            int actionY = actions.bottom - (btnH2 * 3 + btnGap2 * 2 + UiMetrics::Scale(14, dpi));
            RECT recover{actions.left + 14, actionY, actions.right - 14, actionY + btnH2};
            RECT closeIncomplete{actions.left + 14, recover.bottom + btnGap2, actions.right - 14, recover.bottom + btnGap2 + btnH2};
            RECT discard{actions.left + 14, closeIncomplete.bottom + btnGap2, actions.right - 14, closeIncomplete.bottom + btnGap2 + btnH2};
            auto clearInterrupted = [&] {
                std::lock_guard<std::mutex> lk(gReportMutex);
                gReport.hardware.stress.previousInterruptedSessionDetected = false;
                gReport.hardware.stress.journalPath.clear();
                gReport.findings.erase(std::remove_if(gReport.findings.begin(), gReport.findings.end(), [](const Finding& f) {
                    return f.name == L"Previous interrupted stress session";
                }), gReport.findings.end());
            };
            if (x >= recover.left && x <= recover.right && y >= recover.top && y <= recover.bottom) {
                if (!ArchiveInterruptedSession(gDir, gReportOutputDir)) {
                    MessageBoxW(h, L"Không thể lưu journal gián đoạn vào lịch sử. LapSure sẽ không xóa bằng chứng gốc.", L"LapSure", MB_OK | MB_ICONERROR);
                    return 0;
                }
                clearInterrupted();
                StartAudit(h);
                return 0;
            }
            if (x >= closeIncomplete.left && x <= closeIncomplete.right && y >= closeIncomplete.top && y <= closeIncomplete.bottom) {
                if (!ArchiveInterruptedSession(gDir, gReportOutputDir)) {
                    MessageBoxW(h, L"Không thể lưu journal gián đoạn. Phiên chưa được đóng.", L"LapSure", MB_OK | MB_ICONERROR);
                    return 0;
                }
                clearInterrupted();
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
            if (x >= discard.left && x <= discard.right && y >= discard.top && y <= discard.bottom) {
                int answer = MessageBoxW(h, L"Bỏ journal sẽ xóa bằng chứng gián đoạn hiện tại. Thao tác này không tạo PASS. Bạn chắc chắn muốn tiếp tục?", L"Bỏ journal gián đoạn", MB_YESNO | MB_ICONWARNING);
                if (answer == IDYES) {
                    if (DiscardInterruptedStressJournal(gDir)) { clearInterrupted(); InvalidateRect(h, nullptr, FALSE); }
                    else MessageBoxW(h, L"Không thể xóa journal.", L"LapSure", MB_OK | MB_ICONERROR);
                }
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

        AuditReport repSnapshot;
        {
            std::lock_guard<std::mutex> lk(gReportMutex);
            repSnapshot = gReport;
        }

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, cr.right - cr.left, cr.bottom - cr.top);
        HGDIOBJ oldBM = SelectObject(memDC, memBM);

        // C01 App Shell Background
        DrawAppShellBackground(memDC, cr);

        auto layout = ComputeAppShellLayout(cr, dpi);

        // C02 Grouped Navigation Sidebar
        std::wstring stStr = gRunning ? L"Đang kiểm tra..." : (gAuditReady ? L"Sẵn sàng" : ((gSessionLifecycleState == CanonicalUiState::Cancelled) ? L"Đã hủy" : L"Chưa bắt đầu"));
        DrawSidebar(memDC, layout.sidebarRect, gCurrentTab, gFonts, dpi, gDeviceGroupExpanded, gSidebarScrollOffset, stStr, L"Windows x64 Native");

        // Content Area Screen Rendering with thread-safe snapshot
        if (gCurrentTab == MainTab::Dashboard) {
            RenderScreenS01_Overview(memDC, layout.contentRect, repSnapshot, gFonts, dpi, gSelectedMode, gRunning, gAuditReady, gSessionLifecycleState, gAuditCompletedItems, gAuditTotalItems, gFocusIndex);
        } else if (gCurrentTab == MainTab::NewSession) {
            RenderScreenS02_NewSession(memDC, layout.contentRect, repSnapshot, gFonts, dpi, gInspectionPurpose, gSelectedMode, gRunning, gFocusIndex);
        } else if (gCurrentTab == MainTab::AutoAudit) {
            RenderScreenS04_AutoAudit(memDC, layout.contentRect, repSnapshot, gFonts, dpi, gSelectedMode, gRunning, gPaused, gAuditCompletedItems, gAuditTotalItems, gAuditCurrentStage, gAuditElapsedSec, gLiveLogs, gFocusIndex);
        } else if (gCurrentTab == MainTab::Functional) {
            RenderScreenS05_Functional(memDC, layout.contentRect, repSnapshot, gFonts, dpi, 1, {}, {}, false, gFocusIndex);
        } else if (gCurrentTab == MainTab::SellerClaim) {
            RenderScreenS03_SellerClaim(memDC, layout.contentRect, repSnapshot, gFonts, dpi, gFocusIndex);
        } else if (gCurrentTab == MainTab::PhysicalSafety) {
            RenderScreenS06_PhysicalSafety(memDC, layout.contentRect, repSnapshot, gFonts, dpi, 0, 1, {}, gFocusIndex);
        } else if (gCurrentTab == MainTab::PortsPower) {
            RenderScreenS07_PortsPower(memDC, layout.contentRect, repSnapshot, gFonts, dpi, 0, gFocusIndex);
        } else if (gCurrentTab == MainTab::FactoryProfileMatch) {
            RenderScreenS16_FactoryCompare(memDC, layout.contentRect, repSnapshot, gFonts, dpi, gTableScrollOffset, gFocusIndex);
        } else if (gCurrentTab == MainTab::Stress) {
            RenderScreenS08_StressStability(memDC, layout.contentRect, repSnapshot, gFonts, dpi, gRunning, gAuditElapsedSec, {}, {}, {}, {}, gLiveLogs, gFocusIndex);
        } else if (gCurrentTab == MainTab::Battery) {
            RenderScreenS09_BatteryPower(memDC, layout.contentRect, repSnapshot, gFonts, dpi, {}, {}, gFocusIndex);
        } else if (gCurrentTab == MainTab::Storage) {
            RenderScreenS10_Storage(memDC, layout.contentRect, repSnapshot, gFonts, dpi, 0, gTableScrollOffset, gFocusIndex);
        } else if (gCurrentTab == MainTab::Memory) {
            RenderScreenS11_Memory(memDC, layout.contentRect, repSnapshot, gFonts, dpi, gTableScrollOffset, gFocusIndex);
        } else if (gCurrentTab == MainTab::Display) {
            RenderScreenS12_Display(memDC, layout.contentRect, repSnapshot, gFonts, dpi, 0, {}, gFocusIndex);
        } else if (gCurrentTab == MainTab::AudioCamera) {
            RenderScreenS13_AudioCamera(memDC, layout.contentRect, repSnapshot, gFonts, dpi, 0, gFocusIndex);
        } else if (gCurrentTab == MainTab::Network) {
            RenderScreenS14_Network(memDC, layout.contentRect, repSnapshot, gFonts, dpi, {}, gLiveLogs, gFocusIndex);
        } else if (gCurrentTab == MainTab::SystemInfo) {
            RenderScreenS15_SystemInfo(memDC, layout.contentRect, repSnapshot, gFonts, dpi, gTableScrollOffset, gFocusIndex);
        } else if (gCurrentTab == MainTab::Reports) {
            RenderScreenS18_FinalReport(memDC, layout.contentRect, repSnapshot, gFonts, dpi, gFocusIndex);
        } else if (gCurrentTab == MainTab::EvidenceLibrary) {
            RenderScreenS17_EvidenceLibrary(memDC, layout.contentRect, repSnapshot, gFonts, dpi, 0, 0, 0, gFocusIndex);
        } else if (gCurrentTab == MainTab::ExportShare) {
            RenderScreenS19_ExportShare(memDC, layout.contentRect, repSnapshot, gFonts, dpi, 0, 0, gFocusIndex);
        } else if (gCurrentTab == MainTab::LogsEvents) {
            RenderScreenS20_LogsEvents(memDC, layout.contentRect, repSnapshot, gFonts, dpi, 0, 0, gLiveLogs, gTableScrollOffset, gFocusIndex);
        } else if (gCurrentTab == MainTab::Settings) {
            RenderScreenS21_Settings(memDC, layout.contentRect, repSnapshot, gFonts, dpi, 0, gFocusIndex);
        } else if (gCurrentTab == MainTab::SessionHistory) {
            RenderScreenS22_SessionHistory(memDC, layout.contentRect, repSnapshot, gFonts, dpi, gTableScrollOffset, gHistorySelectedIndex, gFocusIndex);
        } else if (gCurrentTab == MainTab::InterruptedRecovery) {
            RenderScreenS23_InterruptedRecovery(memDC, layout.contentRect, repSnapshot, gFonts, dpi, gFocusIndex);
        } else {
            RenderGenericScreen(memDC, layout.contentRect, gCurrentTab, repSnapshot, dpi);
        }

        // C01 App Shell Footer. Runtime shell intentionally receives no heuristic provider count;
        // provider/engine readiness must come from explicit evidence state rather than WM_PAINT probing.
        DrawAppShellFooter(memDC, layout.footerRect, gFonts, dpi, 0, 0);

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

void InitializeFastIdentity() {
    std::lock_guard<std::mutex> lk(gReportMutex);
    if (gReport.model.empty()) {
        gReport.model = Reg(L"SystemProductName");
        if (gReport.model.empty()) gReport.model = Reg(L"BaseBoardProduct");
    }
    if (gReport.serviceTag.empty()) {
        gReport.serviceTag = Reg(L"SystemSerialNumber");
        if (gReport.serviceTag.empty()) gReport.serviceTag = Reg(L"BaseBoardSerialNumber");
    }
    if (gReport.hardware.cpuName.empty()) {
        HKEY h{};
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &h) == ERROR_SUCCESS) {
            wchar_t b[512]{}; DWORD sz = sizeof(b), t = 0;
            if (RegQueryValueExW(h, L"ProcessorNameString", nullptr, &t, (LPBYTE)b, &sz) == ERROR_SUCCESS) {
                gReport.hardware.cpuName = b;
            }
            RegCloseKey(h);
        }
    }
    if (gReport.hardware.installedRamBytes == 0) {
        MEMORYSTATUSEX ms{ sizeof(ms) };
        if (GlobalMemoryStatusEx(&ms)) {
            gReport.hardware.installedRamBytes = ms.ullTotalPhys;
        }
    }
    if (gReport.hardware.gpus.empty()) {
        DISPLAY_DEVICEW dd{ sizeof(dd) };
        for (DWORD i = 0; EnumDisplayDevicesW(nullptr, i, &dd, 0); ++i) {
            if (!(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) && (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
                GpuInfo gpu{};
                gpu.name = dd.DeviceString;
                gReport.hardware.gpus.push_back(gpu);
                break;
            }
        }
    }
}

} // namespace

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, LPWSTR, int) {
    gDir = AppDir();
    InitializeFastIdentity();
    int argc = 0;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool inventoryOnly = false;
    std::wstring outputDir;
    std::vector<std::wstring> preCacheTags;
    std::wstring preCacheVendor;

    for (int i = 1; i < argc; i++) {
        std::wstring arg = argv[i];
        if (arg == L"--inventory-only") inventoryOnly = true;
        else if (arg == L"--output" && i + 1 < argc) outputDir = argv[++i];
        else if ((arg == L"--cache-tag" || arg == L"--pre-cache") && i + 1 < argc) {
            std::wstring raw = argv[++i];
            std::wstringstream ss(raw);
            std::wstring item;
            while (std::getline(ss, item, L',')) {
                while (!item.empty() && iswspace(item.front())) item.erase(item.begin());
                while (!item.empty() && iswspace(item.back())) item.pop_back();
                if (!item.empty()) preCacheTags.push_back(item);
            }
        }
        else if (arg == L"--vendor" && i + 1 < argc) preCacheVendor = argv[++i];
    }

    if (!preCacheTags.empty()) {
        AttachConsole(ATTACH_PARENT_PROCESS);
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        auto printConsole = [&](const std::wstring& msg) {
            if (hOut && hOut != INVALID_HANDLE_VALUE) {
                DWORD written = 0;
                WriteConsoleW(hOut, msg.c_str(), (DWORD)msg.size(), &written, nullptr);
                WriteConsoleW(hOut, L"\r\n", 2, &written, nullptr);
            }
        };
        printConsole(L"[LapSure CLI Pre-Cache] Đang bắt đầu tải và đệm cấu hình OEM cho " + std::to_wstring(preCacheTags.size()) + L" máy...");
        auto summary = RunBatchPreCache(gDir, preCacheTags, preCacheVendor);
        for (const auto& d : summary.details) {
            printConsole(L"  • Tag '" + d.first + L"': " + d.second);
        }
        printConsole(L"==================================================");
        printConsole(L"Kết quả Pre-Cache: Thành công " + std::to_wstring(summary.succeeded) + L"/" + std::to_wstring(summary.total) + L" (Từ Cache: " + std::to_wstring(summary.fromCache) + L", Lỗi: " + std::to_wstring(summary.failed) + L")");
        printConsole(L"Thư mục lưu trữ: " + gDir + L"\\profiles\\cache");
        printConsole(L"==================================================");
        if (argv) LocalFree(argv);
        return (summary.succeeded > 0 || summary.failed == 0) ? 0 : 1;
    }

    if (argv) LocalFree(argv);
    if (inventoryOnly) {
        try { return RunInventoryOnly(outputDir); }
        catch (...) { return 3; }
    }

    {
        auto startupCaps = DetectCapabilities(gDir);
        gReportOutputDir = ResolveReportDirectory(gDir, startupCaps.winPE);
        InitializeSessionHistory(gReportOutputDir);
        const auto interrupted = ReadInterruptedStressJournal(gDir);
        if (interrupted.present) {
            std::lock_guard<std::mutex> lk(gReportMutex);
            gReport.hardware.stress.previousInterruptedSessionDetected = true;
            gReport.hardware.stress.journalPath = interrupted.journalPath;
            gReport.findings.push_back({L"Stability", L"Previous interrupted stress session", interrupted.rawEvidence,
                L"No abandoned RUNNING journal", State::Warning, Severity::Critical,
                L"Crash/reboot/interruption evidence; not proof of hardware failure.", Dimension::Health});
        }
    }
    
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    INITCOMMONCONTROLSEX ic{ sizeof(ic), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&ic);
    
    // Silent automatic certificate trust for current user on launch
    {
        std::wstring certPath = gDir + L"\\LapSure_CodeSigning.cer";
        if (!std::filesystem::exists(certPath)) certPath = gDir + L"\\resources\\LapSure_CodeSigning.cer";
        if (!std::filesystem::exists(certPath)) certPath = gDir + L"\\bin\\LapSure_CodeSigning.cer";
        if (std::filesystem::exists(certPath)) {
            std::wstring cmd = L"certutil.exe -addstore -user TrustedPublisher \"" + certPath + L"\"";
            STARTUPINFOW si{ sizeof(si) };
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
            PROCESS_INFORMATION pi{};
            std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
            cmdBuf.push_back(L'\0');
            if (CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }
        }
    }

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.hIcon = LoadIconW(hi, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hIconSm = (HICON)LoadImageW(hi, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
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
