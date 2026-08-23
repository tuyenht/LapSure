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
    Settings
};

struct UiColors {
    static constexpr COLORREF SidebarBg = RGB(11, 25, 46);
    static constexpr COLORREF SidebarHover = RGB(20, 38, 66);
    static constexpr COLORREF SidebarActive = RGB(29, 100, 242);
    static constexpr COLORREF SidebarText = RGB(160, 175, 200);
    static constexpr COLORREF SidebarTextActive = RGB(255, 255, 255);
    static constexpr COLORREF ContentBg = RGB(244, 247, 251);
    static constexpr COLORREF CardBg = RGB(255, 255, 255);
    static constexpr COLORREF CardBorder = RGB(226, 232, 240);
    static constexpr COLORREF CardBorderHover = RGB(186, 205, 235);
    static constexpr COLORREF TextMain = RGB(15, 23, 42);
    static constexpr COLORREF TextMuted = RGB(100, 116, 139);
    static constexpr COLORREF TextLight = RGB(148, 163, 184);
    static constexpr COLORREF PrimaryBlue = RGB(29, 100, 242);
    static constexpr COLORREF PrimaryBlueHover = RGB(21, 85, 215);
    static constexpr COLORREF SuccessGreen = RGB(22, 163, 74);
    static constexpr COLORREF SuccessBg = RGB(236, 253, 245);
    static constexpr COLORREF WarnAmber = RGB(217, 119, 6);
    static constexpr COLORREF WarnBg = RGB(254, 243, 199);
    static constexpr COLORREF FailRed = RGB(220, 38, 38);
    static constexpr COLORREF FailBg = RGB(254, 242, 242);
    static constexpr COLORREF InfoBlue = RGB(37, 99, 235);
    static constexpr COLORREF InfoBg = RGB(239, 246, 255);
    static constexpr COLORREF GrayPillBg = RGB(241, 245, 249);
    static constexpr COLORREF GrayPillBorder = RGB(203, 213, 225);
};

struct UiFonts {
    HFONT hTitle{nullptr};      // Segoe UI 18pt Bold
    HFONT hSection{nullptr};    // Segoe UI 13pt SemiBold
    HFONT hBodyBold{nullptr};   // Segoe UI 10pt SemiBold
    HFONT hBody{nullptr};       // Segoe UI 9.5pt Regular
    HFONT hSmall{nullptr};      // Segoe UI 8.5pt Regular
    HFONT hMono{nullptr};       // Consolas 9pt Regular

    void Init();
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

} // namespace lap
