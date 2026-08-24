#include "lap/ui_components.h"
#include "lap/environment.h"
#include <filesystem>
#include <string>

namespace lap {

// Legacy implementation is compiled under these names by CMake. The runtime wrappers below
// keep all existing sidebar visuals/hit-test geometry while replacing fake environment metadata.
void DrawSidebar_Legacy(HDC dc, const RECT& r, MainTab activeTab, const UiFonts& fonts, int dpi,
                        bool deviceGroupExpanded, int scrollOffsetY,
                        const std::wstring& sessionStatus, const std::wstring& osVersion);

namespace {

std::wstring AppDirectory() {
    wchar_t path[MAX_PATH]{};
    const DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return L".";
    return std::filesystem::path(path).parent_path().wstring();
}

bool IsProcessElevated() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;
    TOKEN_ELEVATION elevation{};
    DWORD size = 0;
    const BOOL ok = GetTokenInformation(token, TokenElevation, &elevation,
                                        sizeof(elevation), &size);
    CloseHandle(token);
    return ok && elevation.TokenIsElevated != 0;
}

std::wstring NativeArchitecture() {
    SYSTEM_INFO info{};
    GetNativeSystemInfo(&info);
    switch (info.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64: return L"x64";
    case PROCESSOR_ARCHITECTURE_ARM64: return L"ARM64";
    case PROCESSOR_ARCHITECTURE_INTEL: return L"x86";
    default: return L"Kiến trúc không xác định";
    }
}

struct RuntimeUiMeta {
    Capabilities capabilities{};
    bool elevated{};
    std::wstring environment;
    std::wstring providerSummary;
};

RuntimeUiMeta BuildRuntimeMeta() {
    RuntimeUiMeta meta;
    meta.capabilities = DetectCapabilities(AppDirectory());
    meta.elevated = IsProcessElevated();
    meta.environment = EnvironmentName(meta.capabilities) + L" • " + NativeArchitecture();
    meta.providerSummary = L"WMI: ";
    meta.providerSummary += meta.capabilities.wmi ? L"OK" : L"không sẵn sàng";
    meta.providerSummary += L"  •  smartctl: ";
    meta.providerSummary += meta.capabilities.smartctl ? L"OK" : L"không có";
    meta.providerSummary += L"  •  NVIDIA CLI: ";
    meta.providerSummary += meta.capabilities.nvidiaSmi ? L"OK" : L"không có";
    return meta;
}

const RuntimeUiMeta& RuntimeMeta() {
    static const RuntimeUiMeta meta = BuildRuntimeMeta();
    return meta;
}

} // namespace

void DrawSidebar(HDC dc, const RECT& r, MainTab activeTab, const UiFonts& fonts, int dpi,
                 bool deviceGroupExpanded, int scrollOffsetY,
                 const std::wstring& sessionStatus, const std::wstring& osVersion) {
    (void)osVersion; // main.cpp still passes the legacy literal; never render it.
    const auto& meta = RuntimeMeta();
    DrawSidebar_Legacy(dc, r, activeTab, fonts, dpi, deviceGroupExpanded, scrollOffsetY,
                       sessionStatus, meta.environment);
}

void DrawAppShellFooter(HDC dc, const RECT& r, const UiFonts& fonts, int dpi,
                        int readyEngines, int totalEngines) {
    (void)readyEngines;
    (void)totalEngines; // legacy x/14 readiness was heuristic, so it is intentionally not rendered.
    const auto& meta = RuntimeMeta();

    HBRUSH background = CreateSolidBrush(UiColors::CardBg);
    FillRect(dc, &r, background);
    DeleteObject(background);

    HPEN border = CreatePen(PS_SOLID, 1, UiColors::CardBorder);
    HGDIOBJ oldPen = SelectObject(dc, border);
    MoveToEx(dc, r.left, r.top, nullptr);
    LineTo(dc, r.right, r.top);
    SelectObject(dc, oldPen);
    DeleteObject(border);

    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, fonts.hSmall);

    const int width = r.right - r.left;
    const int c1w = UiMetrics::Scale(150, dpi);
    const int c2w = UiMetrics::Scale(170, dpi);
    const int c4w = UiMetrics::Scale(185, dpi);

    RECT c1{ r.left + UiMetrics::Scale(14, dpi), r.top,
             r.left + c1w, r.bottom };
    SetTextColor(dc, meta.elevated ? UiColors::SuccessGreen : UiColors::WarnAmber);
    const std::wstring privilege = meta.elevated ? L"● Quyền: Admin" : L"● Quyền: Standard";
    DrawTextW(dc, privilege.c_str(), static_cast<int>(privilege.size()), &c1,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    RECT c2{ c1.right + UiMetrics::Scale(6, dpi), r.top,
             c1.right + c2w, r.bottom };
    SetTextColor(dc, UiColors::TextMuted);
    DrawTextW(dc, meta.environment.c_str(), static_cast<int>(meta.environment.size()), &c2,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    RECT c4{ r.right - c4w, r.top, r.right - UiMetrics::Scale(14, dpi), r.bottom };
    SetTextColor(dc, UiColors::PrimaryBlue);
    DrawTextW(dc, L"Chính sách: Evidence First", -1, &c4,
              DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    RECT c3{ c2.right + UiMetrics::Scale(6, dpi), r.top,
             c4.left - UiMetrics::Scale(8, dpi), r.bottom };
    SetTextColor(dc, UiColors::TextMuted);
    if (c3.right > c3.left) {
        DrawTextW(dc, meta.providerSummary.c_str(), static_cast<int>(meta.providerSummary.size()), &c3,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    // If the window is too narrow, the middle provider column is allowed to collapse; no fake
    // readiness percentage or stale database date is substituted.
    (void)width;
}

} // namespace lap
