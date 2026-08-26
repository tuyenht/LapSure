#include "lap/ui_components.h"
#include <string>

namespace lap {

// Legacy sidebar implementation is compiled under this name by CMake. The runtime wrapper keeps
// the existing visual/hit-test geometry while replacing stale environment literals.
void DrawSidebar_Legacy(HDC dc, const RECT& r, MainTab activeTab, const UiFonts& fonts, int dpi,
                        bool deviceGroupExpanded, int scrollOffsetY,
                        const std::wstring& sessionStatus, const std::wstring& osVersion);

namespace {

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

bool IsWinPE() {
    HKEY key = nullptr;
    const LONG rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                  L"SYSTEM\\CurrentControlSet\\Control\\MiniNT",
                                  0, KEY_READ, &key);
    if (key) RegCloseKey(key);
    return rc == ERROR_SUCCESS;
}

std::wstring NativeArchitecture() {
    SYSTEM_INFO info{};
    GetNativeSystemInfo(&info);
    switch (info.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64: return L"x64";
    case PROCESSOR_ARCHITECTURE_ARM64: return L"ARM64";
    case PROCESSOR_ARCHITECTURE_INTEL: return L"x86";
    default: return L"kiến trúc chưa xác định";
    }
}

struct RuntimeUiMeta {
    bool elevated{};
    std::wstring environment;
};

RuntimeUiMeta BuildRuntimeMeta() {
    RuntimeUiMeta meta;
    meta.elevated = IsProcessElevated();
    meta.environment = std::wstring(IsWinPE() ? L"Windows PE" : L"Windows") + L" • " + NativeArchitecture();
    return meta;
}

const RuntimeUiMeta& RuntimeMeta() {
    // Process/environment facts are immutable for a running LapSure process. Cache them once so
    // paint/layout never invokes WMI, external engines, hardware providers or other slow work.
    static const RuntimeUiMeta meta = BuildRuntimeMeta();
    return meta;
}

} // namespace

void DrawSidebar(HDC dc, const RECT& r, MainTab activeTab, const UiFonts& fonts, int dpi,
                 bool deviceGroupExpanded, int scrollOffsetY,
                 const std::wstring& sessionStatus, const std::wstring& osVersion) {
    (void)osVersion; // main.cpp may still pass a legacy display string; never render it as runtime fact.
    const auto& meta = RuntimeMeta();
    DrawSidebar_Legacy(dc, r, activeTab, fonts, dpi, deviceGroupExpanded, scrollOffsetY,
                       sessionStatus, meta.environment);
}

void DrawAppShellFooter(HDC dc, const RECT& r, const UiFonts& fonts, int dpi,
                        int readyEngines, int totalEngines) {
    (void)readyEngines;
    (void)totalEngines; // legacy readiness counters are heuristic and therefore intentionally omitted.
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

    const int privilegeW = UiMetrics::Scale(155, dpi);
    const int environmentW = UiMetrics::Scale(190, dpi);
    const int policyW = UiMetrics::Scale(190, dpi);

    RECT privilegeRect{ r.left + UiMetrics::Scale(14, dpi), r.top,
                        r.left + privilegeW, r.bottom };
    SetTextColor(dc, meta.elevated ? UiColors::SuccessGreen : UiColors::WarnAmber);
    const std::wstring privilege = meta.elevated ? L"● Quyền: Admin" : L"● Quyền: Standard";
    DrawTextW(dc, privilege.c_str(), static_cast<int>(privilege.size()), &privilegeRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    RECT environmentRect{ privilegeRect.right + UiMetrics::Scale(8, dpi), r.top,
                          privilegeRect.right + environmentW, r.bottom };
    SetTextColor(dc, UiColors::TextMuted);
    DrawTextW(dc, meta.environment.c_str(), static_cast<int>(meta.environment.size()), &environmentRect,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    RECT policyRect{ r.right - policyW, r.top, r.right - UiMetrics::Scale(14, dpi), r.bottom };
    SetTextColor(dc, UiColors::PrimaryBlue);
    DrawTextW(dc, L"Chính sách: Evidence First", -1, &policyRect,
              DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    RECT middleRect{ environmentRect.right + UiMetrics::Scale(8, dpi), r.top,
                     policyRect.left - UiMetrics::Scale(8, dpi), r.bottom };
    if (middleRect.right > middleRect.left) {
        SetTextColor(dc, UiColors::TextMuted);
        DrawTextW(dc, L"Provider readiness: xem trạng thái kiểm định", -1, &middleRect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }
}

} // namespace lap
