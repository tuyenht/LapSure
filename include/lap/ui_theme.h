#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "lap/model.h"

namespace lap {

enum class MainTab {
    Dashboard = 0,
    AutoAudit,
    Functional,
    PortsPower,
    Stress,
    Battery,
    Storage,
    Memory,
    Display,
    AudioCamera,
    Network,
    SystemInfo,
    FactoryProfileMatch,
    Reports,
    LogsEvents,
    Settings,
    NewSession,
    PhysicalSafety,
    SellerClaim,
    EvidenceLibrary,
    ExportShare,
    SessionHistory,
    InterruptedRecovery
};

enum class NavGroup {
    Workflow = 0,      // QUY TRÌNH
    DeviceDetails,     // CHI TIẾT THIẾT BỊ
    AuditAndRecords,   // ĐÁNH GIÁ & HỒ SƠ
    SystemSettings     // HỆ THỐNG / CÀI ĐẶT
};

struct UiColors {
    // Surfaces
    static constexpr COLORREF SidebarBg = RGB(11, 25, 46);
    static constexpr COLORREF SidebarHover = RGB(20, 38, 66);
    static constexpr COLORREF SidebarActive = RGB(29, 100, 242);
    static constexpr COLORREF SidebarText = RGB(160, 175, 200);
    static constexpr COLORREF SidebarTextActive = RGB(255, 255, 255);
    static constexpr COLORREF SidebarGroupHeader = RGB(100, 116, 139);
    static constexpr COLORREF ContentBg = RGB(244, 247, 251);
    static constexpr COLORREF CardBg = RGB(255, 255, 255);
    static constexpr COLORREF CardBorder = RGB(226, 232, 240);
    static constexpr COLORREF CardBorderHover = RGB(186, 205, 235);
    
    // Text
    static constexpr COLORREF TextMain = RGB(15, 23, 42);
    static constexpr COLORREF TextMuted = RGB(100, 116, 139);
    static constexpr COLORREF TextLight = RGB(148, 163, 184);
    
    // Brand
    static constexpr COLORREF PrimaryBlue = RGB(29, 100, 242);
    static constexpr COLORREF PrimaryBlueHover = RGB(21, 85, 215);
    static constexpr COLORREF PrimaryBlueLight = RGB(239, 246, 255);
    
    // Status - Success / Good / Pass
    static constexpr COLORREF SuccessGreen = RGB(22, 163, 74);
    static constexpr COLORREF SuccessBg = RGB(236, 253, 245);
    static constexpr COLORREF SuccessBorder = RGB(187, 247, 208);
    
    // Status - Warning / Note
    static constexpr COLORREF WarnAmber = RGB(217, 119, 6);
    static constexpr COLORREF WarnBg = RGB(254, 243, 199);
    static constexpr COLORREF WarnBorder = RGB(253, 230, 138);
    
    // Status - Fail / Critical
    static constexpr COLORREF FailRed = RGB(220, 38, 38);
    static constexpr COLORREF FailBg = RGB(254, 242, 242);
    static constexpr COLORREF FailBorder = RGB(254, 202, 202);
    
    // Status - Info / Running
    static constexpr COLORREF InfoBlue = RGB(37, 99, 235);
    static constexpr COLORREF InfoBg = RGB(239, 246, 255);
    static constexpr COLORREF InfoBorder = RGB(191, 219, 254);
    
    // Neutral / Pills / Table
    static constexpr COLORREF GrayPillBg = RGB(241, 245, 249);
    static constexpr COLORREF GrayPillBorder = RGB(203, 213, 225);
    static constexpr COLORREF TableHeaderBg = RGB(248, 250, 252);
    static constexpr COLORREF TableRowAlt = RGB(250, 252, 255);
    static constexpr COLORREF TableBorder = RGB(226, 232, 240);
    static constexpr COLORREF FocusOutline = RGB(29, 100, 242);
};

struct UiMetrics {
    static constexpr int Spacing4 = 4;
    static constexpr int Spacing8 = 8;
    static constexpr int Spacing12 = 12;
    static constexpr int Spacing16 = 16;
    static constexpr int Spacing24 = 24;
    static constexpr int Spacing32 = 32;
    static constexpr int Spacing48 = 48;

    static constexpr int RadiusSm = 4;
    static constexpr int RadiusMd = 8;
    static constexpr int RadiusLg = 12;
    static constexpr int RadiusPill = 20;

    static constexpr int ButtonHeight = 36;
    static constexpr int ButtonHeightSm = 28;
    static constexpr int BadgeHeight = 22;
    static constexpr int TableRowHeight = 36;
    static constexpr int SidebarWidth = 240;

    static int Scale(int val, int dpi) {
        if (dpi <= 0 || dpi == 96) return val;
        return MulDiv(val, dpi, 96);
    }
};

int GetDpiForHwnd(HWND hwnd);

struct UiFonts {
    int currentDpi{96};
    HFONT hTitle{nullptr};      // Segoe UI 18pt Bold (24px)
    HFONT hSection{nullptr};    // Segoe UI 13pt SemiBold (17px)
    HFONT hBodyBold{nullptr};   // Segoe UI 10pt SemiBold (13px)
    HFONT hBody{nullptr};       // Segoe UI 9.5pt Regular (13px)
    HFONT hSmall{nullptr};      // Segoe UI 8.5pt Regular (11px)
    HFONT hMono{nullptr};       // Consolas 9pt Regular (12px)

    void Init(int dpi = 96);
    void Cleanup();
};

struct LiveLogEntry {
    std::wstring time;
    std::wstring message;
    std::wstring source;
    int state{0}; // 0=good, 1=warn, 2=fail, 3=info
};

// UI Helper drawing functions
void DrawRoundedCard(HDC dc, const RECT& r, int radius, COLORREF fill, COLORREF border, int borderWidth = 1);
void DrawBadge(HDC dc, int x, int y, int w, int h, const std::wstring& text, COLORREF textClr, COLORREF bgClr, HFONT font);
void DrawCircularScoreGauge(HDC dc, int cx, int cy, int radius, int score, const std::wstring& label, HFONT hTitleFont, HFONT hSubFont);
void DrawModernProgressBar(HDC dc, const RECT& r, int percent, COLORREF barClr, COLORREF bgClr);
void DrawFocusRing(HDC dc, const RECT& r, int radius);

} // namespace lap
