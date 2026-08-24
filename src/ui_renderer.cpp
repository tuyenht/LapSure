#include "lap/ui_theme.h"
#include <cmath>
#include <vector>
#include <algorithm>

namespace lap {

int GetDpiForHwnd(HWND hwnd) {
    if (!hwnd) return 96;
    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    if (hUser) {
        typedef UINT(WINAPI* GetDpiForWindowProc)(HWND);
        auto pGetDpi = (GetDpiForWindowProc)GetProcAddress(hUser, "GetDpiForWindow");
        if (pGetDpi) {
            UINT dpi = pGetDpi(hwnd);
            if (dpi > 0) return (int)dpi;
        }
    }
    HDC hdc = GetDC(hwnd);
    int dpi = 96;
    if (hdc) {
        dpi = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(hwnd, hdc);
    }
    return dpi > 0 ? dpi : 96;
}

void UiFonts::Init(int dpi) {
    if (dpi <= 0) dpi = 96;
    if (hTitle && currentDpi == dpi) return;
    Cleanup();
    currentDpi = dpi;

    int szTitle = MulDiv(-22, dpi, 96);
    int szSection = MulDiv(-16, dpi, 96);
    int szBodyBold = MulDiv(-13, dpi, 96);
    int szBody = MulDiv(-13, dpi, 96);
    int szSmall = MulDiv(-12, dpi, 96);
    int szMono = MulDiv(-12, dpi, 96);

    hTitle = CreateFontW(szTitle, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hSection = CreateFontW(szSection, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hBodyBold = CreateFontW(szBodyBold, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hBody = CreateFontW(szBody, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hSmall = CreateFontW(szSmall, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hMono = CreateFontW(szMono, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
}

void UiFonts::Cleanup() {
    if (hTitle) { DeleteObject(hTitle); hTitle = nullptr; }
    if (hSection) { DeleteObject(hSection); hSection = nullptr; }
    if (hBodyBold) { DeleteObject(hBodyBold); hBodyBold = nullptr; }
    if (hBody) { DeleteObject(hBody); hBody = nullptr; }
    if (hSmall) { DeleteObject(hSmall); hSmall = nullptr; }
    if (hMono) { DeleteObject(hMono); hMono = nullptr; }
}

void DrawRoundedCard(HDC dc, const RECT& r, int radius, COLORREF fill, COLORREF border, int borderWidth) {
    HPEN hPen = CreatePen(PS_SOLID, borderWidth, border);
    HBRUSH hBrush = CreateSolidBrush(fill);
    HGDIOBJ oldPen = SelectObject(dc, hPen);
    HGDIOBJ oldBrush = SelectObject(dc, hBrush);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius * 2, radius * 2);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(hPen);
    DeleteObject(hBrush);
}

void DrawBadge(HDC dc, int x, int y, int w, int h, const std::wstring& text, COLORREF textClr, COLORREF bgClr, HFONT font) {
    RECT r{ x, y, x + w, y + h };
    DrawRoundedCard(dc, r, h / 2, bgClr, bgClr, 1);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, textClr);
    HGDIOBJ oldFont = SelectObject(dc, font);
    DrawTextW(dc, text.c_str(), (int)text.size(), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, oldFont);
}

void DrawModernProgressBar(HDC dc, const RECT& r, int percent, COLORREF barClr, COLORREF bgClr) {
    DrawRoundedCard(dc, r, (r.bottom - r.top) / 2, bgClr, bgClr, 1);
    if (percent > 0) {
        int w = (r.right - r.left) * percent / 100;
        if (w < (r.bottom - r.top)) w = (r.bottom - r.top);
        if (w > (r.right - r.left)) w = (r.right - r.left);
        RECT fr{ r.left, r.top, r.left + w, r.bottom };
        DrawRoundedCard(dc, fr, (r.bottom - r.top) / 2, barClr, barClr, 1);
    }
}

void DrawStarRating(HDC dc, int x, int y, int starCount, int maxStars) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(245, 158, 11)); // Amber gold
    for (int i = 0; i < maxStars; ++i) {
        const wchar_t* st = (i < starCount) ? L"★" : L"☆";
        TextOutW(dc, x + i * 14, y, st, 1);
    }
}

void DrawCircularScoreGauge(HDC dc, int cx, int cy, int radius, int score, const std::wstring& label, HFONT hTitleFont, HFONT hSubFont, int starCount) {
    HPEN trackPen = CreatePen(PS_SOLID, 8, RGB(226, 232, 240));
    HGDIOBJ oldPen = SelectObject(dc, trackPen);
    HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HGDIOBJ oldBrush = SelectObject(dc, nullBrush);
    
    Ellipse(dc, cx - radius, cy - radius, cx + radius, cy + radius);
    SelectObject(dc, oldPen);
    DeleteObject(trackPen);

    if (score > 0) {
        COLORREF arcClr = score >= 80 ? UiColors::SuccessGreen : (score >= 60 ? UiColors::WarnAmber : UiColors::FailRed);
        HPEN arcPen = CreatePen(PS_SOLID, 8, arcClr);
        SelectObject(dc, arcPen);
        
        double startAngle = -3.14159265 / 2.0;
        double sweepAngle = (double)std::clamp(score, 1, 100) / 100.0 * 2.0 * 3.14159265;
        double endAngle = startAngle + sweepAngle;
        
        int startX = cx + (int)(radius * std::cos(startAngle));
        int startY = cy + (int)(radius * std::sin(startAngle));
        int endX = cx + (int)(radius * std::cos(endAngle));
        int endY = cy + (int)(radius * std::sin(endAngle));
        
        Arc(dc, cx - radius, cy - radius, cx + radius, cy + radius, startX, startY, endX, endY);
        
        SelectObject(dc, oldPen);
        DeleteObject(arcPen);
    }
    SelectObject(dc, oldBrush);

    SetBkMode(dc, TRANSPARENT);
    
    if (score > 0) {
        SetTextColor(dc, UiColors::TextMain);
        SelectObject(dc, hTitleFont);
        std::wstring scoreStr = std::to_wstring(score);
        RECT tr{ cx - radius, cy - 22, cx + radius, cy + 4 };
        DrawTextW(dc, scoreStr.c_str(), (int)scoreStr.size(), &tr, DT_CENTER | DT_SINGLELINE);
        
        SelectObject(dc, hSubFont);
        SetTextColor(dc, UiColors::TextMuted);
        RECT dr{ cx + 18, cy - 14, cx + radius, cy + 4 };
        DrawTextW(dc, L"/100", 4, &dr, DT_LEFT | DT_SINGLELINE);

        COLORREF lblClr = score >= 80 ? UiColors::SuccessGreen : (score >= 60 ? UiColors::WarnAmber : UiColors::FailRed);
        SetTextColor(dc, lblClr);
        RECT lr{ cx - radius, cy + 4, cx + radius, cy + 20 };
        DrawTextW(dc, label.c_str(), (int)label.size(), &lr, DT_CENTER | DT_SINGLELINE);

        if (starCount > 0) {
            DrawStarRating(dc, cx - 35, cy + 22, starCount, 5);
        }
    } else {
        SetTextColor(dc, UiColors::TextMuted);
        SelectObject(dc, hTitleFont);
        RECT tr{ cx - radius, cy - 20, cx + radius, cy + 6 };
        DrawTextW(dc, L"—", 1, &tr, DT_CENTER | DT_SINGLELINE);

        SelectObject(dc, hSubFont);
        RECT lr{ cx - radius - 10, cy + 6, cx + radius + 10, cy + 24 };
        DrawTextW(dc, L"Chưa kiểm định", 14, &lr, DT_CENTER | DT_SINGLELINE);
    }
}

void DrawFocusRing(HDC dc, const RECT& r, int radius) {
    HPEN hPen = CreatePen(PS_SOLID, 2, UiColors::FocusOutline);
    HGDIOBJ oldPen = SelectObject(dc, hPen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    RoundRect(dc, r.left - 2, r.top - 2, r.right + 2, r.bottom + 2, radius * 2 + 4, radius * 2 + 4);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(hPen);
}

void DrawToggleSwitch(HDC dc, int x, int y, int w, int h, bool isChecked) {
    COLORREF bgClr = isChecked ? UiColors::PrimaryBlue : RGB(203, 213, 225);
    RECT r{ x, y, x + w, y + h };
    DrawRoundedCard(dc, r, h / 2, bgClr, bgClr, 1);
    
    int thumbRadius = (h - 4) / 2;
    int thumbX = isChecked ? (x + w - 2 - thumbRadius * 2) : (x + 2);
    int thumbY = y + 2;
    RECT tr{ thumbX, thumbY, thumbX + thumbRadius * 2, thumbY + thumbRadius * 2 };
    DrawRoundedCard(dc, tr, thumbRadius, RGB(255, 255, 255), RGB(255, 255, 255), 1);
}

void DrawLineChart(HDC dc, const RECT& r, const std::vector<float>& s1, COLORREF c1, const std::wstring& l1,
                   const std::vector<float>& s2, COLORREF c2, const std::wstring& l2,
                   float minVal, float maxVal, const std::wstring& unit, const UiFonts& fonts, int dpi) {
    DrawRoundedCard(dc, r, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    
    int padding = UiMetrics::Scale(12, dpi);
    int headerH = UiMetrics::Scale(24, dpi);
    
    // Header & Legend
    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, r.left + padding, r.top + padding, unit.c_str(), (int)unit.size());

    int legendX = r.right - padding;
    if (!l2.empty()) {
        SelectObject(dc, fonts.hSmall);
        SetTextColor(dc, c2);
        std::wstring leg2 = L"● " + l2;
        SIZE sz; GetTextExtentPoint32W(dc, leg2.c_str(), (int)leg2.size(), &sz);
        legendX -= sz.cx;
        TextOutW(dc, legendX, r.top + padding, leg2.c_str(), (int)leg2.size());
        legendX -= UiMetrics::Scale(16, dpi);
    }
    if (!l1.empty()) {
        SelectObject(dc, fonts.hSmall);
        SetTextColor(dc, c1);
        std::wstring leg1 = L"● " + l1;
        SIZE sz; GetTextExtentPoint32W(dc, leg1.c_str(), (int)leg1.size(), &sz);
        legendX -= sz.cx;
        TextOutW(dc, legendX, r.top + padding, leg1.c_str(), (int)leg1.size());
    }

    // Chart grid bounds
    RECT plotArea{ r.left + padding + UiMetrics::Scale(28, dpi), r.top + padding + headerH, r.right - padding, r.bottom - padding - UiMetrics::Scale(20, dpi) };
    
    // Draw horizontal grid lines (4 levels)
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(241, 245, 249));
    HGDIOBJ oldPen = SelectObject(dc, gridPen);
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextLight);

    for (int i = 0; i <= 3; ++i) {
        int y = plotArea.top + (plotArea.bottom - plotArea.top) * i / 3;
        MoveToEx(dc, plotArea.left, y, nullptr);
        LineTo(dc, plotArea.right, y);
        float val = maxVal - (maxVal - minVal) * (float)i / 3.0f;
        std::wstring valStr = std::to_wstring((int)val);
        RECT yr{ r.left + padding, y - 8, plotArea.left - 4, y + 8 };
        DrawTextW(dc, valStr.c_str(), (int)valStr.size(), &yr, DT_RIGHT | DT_SINGLELINE);
    }
    SelectObject(dc, oldPen);
    DeleteObject(gridPen);

    // Plot series helper
    auto plotSeries = [&](const std::vector<float>& series, COLORREF clr) {
        if (series.size() < 2) return;
        HPEN sp = CreatePen(PS_SOLID, 2, clr);
        HGDIOBJ op = SelectObject(dc, sp);
        int n = (int)series.size();
        for (int i = 0; i < n; ++i) {
            int x = plotArea.left + (plotArea.right - plotArea.left) * i / std::max(1, n - 1);
            float norm = (series[i] - minVal) / std::max(1.0f, maxVal - minVal);
            norm = std::clamp(norm, 0.0f, 1.0f);
            int y = plotArea.bottom - (int)((plotArea.bottom - plotArea.top) * norm);
            if (i == 0) MoveToEx(dc, x, y, nullptr);
            else LineTo(dc, x, y);
        }
        SelectObject(dc, op);
        DeleteObject(sp);
    };

    plotSeries(s1, c1);
    plotSeries(s2, c2);

    // Time axis labels
    SetTextColor(dc, UiColors::TextLight);
    TextOutW(dc, plotArea.left, plotArea.bottom + 4, L"00:00", 5);
    TextOutW(dc, (plotArea.left + plotArea.right) / 2 - 15, plotArea.bottom + 4, L"06:00", 5);
    TextOutW(dc, plotArea.right - 30, plotArea.bottom + 4, L"12:00", 5);
}

void DrawKeyboardGrid(HDC dc, const RECT& r, const std::vector<int>& keyStates, const UiFonts& fonts, int dpi) {
    DrawRoundedCard(dc, r, UiMetrics::RadiusMd, RGB(250, 252, 255), UiColors::CardBorder, 1);
    
    // Standard keyboard layout rows
    const std::vector<std::vector<const wchar_t*>> layout = {
        { L"Esc", L"F1", L"F2", L"F3", L"F4", L"F5", L"F6", L"F7", L"F8", L"F9", L"F10", L"F11", L"F12", L"PrtSc", L"Del" },
        { L"`", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"0", L"-", L"=", L"Backspace" },
        { L"Tab", L"Q", L"W", L"E", L"R", L"T", L"Y", L"U", L"I", L"O", L"P", L"[", L"]", L"\\" },
        { L"Caps", L"A", L"S", L"D", L"F", L"G", L"H", L"J", L"K", L"L", L";", L"'", L"Enter" },
        { L"Shift", L"Z", L"X", L"C", L"V", L"B", L"N", L"M", L",", L".", L"/", L"▲", L"Shift" },
        { L"Ctrl", L"Fn", L"Win", L"Alt", L"Space", L"Alt", L"Menu", L"Ctrl", L"◄", L"▼", L"►" }
    };

    int startY = r.top + UiMetrics::Scale(12, dpi);
    int keyH = UiMetrics::Scale(26, dpi);
    int gap = UiMetrics::Scale(4, dpi);
    int keyIdx = 0;

    SelectObject(dc, fonts.hSmall);
    SetBkMode(dc, TRANSPARENT);

    for (size_t row = 0; row < layout.size(); ++row) {
        int startX = r.left + UiMetrics::Scale(12, dpi);
        int totalCols = (int)layout[row].size();
        int availW = r.right - r.left - UiMetrics::Scale(24, dpi) - (totalCols - 1) * gap;
        int defaultKeyW = availW / 15;

        for (size_t col = 0; col < layout[row].size(); ++col) {
            const wchar_t* kName = layout[row][col];
            int kw = defaultKeyW;
            if (wcscmp(kName, L"Backspace") == 0 || wcscmp(kName, L"Tab") == 0 || wcscmp(kName, L"Caps") == 0) kw = defaultKeyW * 3 / 2;
            else if (wcscmp(kName, L"Enter") == 0 || wcscmp(kName, L"Shift") == 0) kw = defaultKeyW * 7 / 4;
            else if (wcscmp(kName, L"Space") == 0) kw = defaultKeyW * 4;

            RECT kr{ startX, startY, startX + kw, startY + keyH };
            bool pressed = (keyIdx < (int)keyStates.size()) ? (keyStates[keyIdx] > 0) : false;

            COLORREF bg = pressed ? RGB(220, 252, 231) : RGB(255, 255, 255);
            COLORREF bd = pressed ? UiColors::SuccessGreen : UiColors::CardBorder;
            COLORREF tc = pressed ? UiColors::SuccessGreen : UiColors::TextMain;

            DrawRoundedCard(dc, kr, UiMetrics::RadiusSm, bg, bd, 1);
            SetTextColor(dc, tc);
            DrawTextW(dc, kName, -1, &kr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            startX += kw + gap;
            keyIdx++;
        }
        startY += keyH + gap;
    }
}

void DrawTouchpadCanvas(HDC dc, const RECT& r, const std::vector<POINT>& trail, bool completed, const UiFonts& fonts, int dpi) {
    DrawRoundedCard(dc, r, UiMetrics::RadiusMd, RGB(250, 252, 255), UiColors::CardBorder, 1);
    
    // Grid lines inside touchpad
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(235, 240, 248));
    HGDIOBJ oldPen = SelectObject(dc, gridPen);
    for (int x = r.left + 20; x < r.right - 20; x += 30) {
        MoveToEx(dc, x, r.top + 10, nullptr);
        LineTo(dc, x, r.bottom - 10);
    }
    for (int y = r.top + 20; y < r.bottom - 20; y += 30) {
        MoveToEx(dc, r.left + 10, y, nullptr);
        LineTo(dc, r.right - 10, y);
    }
    SelectObject(dc, oldPen);
    DeleteObject(gridPen);

    // Draw trail
    if (trail.size() > 1) {
        HPEN trailPen = CreatePen(PS_SOLID, 6, RGB(187, 247, 208));
        HGDIOBJ op = SelectObject(dc, trailPen);
        for (size_t i = 0; i < trail.size(); ++i) {
            int px = r.left + trail[i].x * (r.right - r.left) / 1000;
            int py = r.top + trail[i].y * (r.bottom - r.top) / 1000;
            if (i == 0) MoveToEx(dc, px, py, nullptr);
            else LineTo(dc, px, py);
        }
        SelectObject(dc, op);
        DeleteObject(trailPen);
    }

    if (completed) {
        int iconW = UiMetrics::Scale(36, dpi);
        RECT ir{ r.right - iconW - UiMetrics::Scale(12, dpi), r.top + UiMetrics::Scale(12, dpi), r.right - UiMetrics::Scale(12, dpi), r.top + UiMetrics::Scale(12, dpi) + iconW };
        DrawRoundedCard(dc, ir, iconW / 2, UiColors::SuccessGreen, UiColors::SuccessGreen, 1);
        SetTextColor(dc, RGB(255, 255, 255));
        SelectObject(dc, fonts.hBodyBold);
        SetBkMode(dc, TRANSPARENT);
        DrawTextW(dc, L"✔", 1, &ir, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

void DrawLaptopChassisDiagram(HDC dc, const RECT& r, int activeHotspot, const UiFonts& fonts, int dpi) {
    DrawRoundedCard(dc, r, UiMetrics::RadiusMd, RGB(255, 255, 255), UiColors::CardBorder, 1);
    
    // Draw laptop perspective wireframe
    int cx = (r.left + r.right) / 2;
    int cy = (r.top + r.bottom) / 2;
    
    HPEN bodyPen = CreatePen(PS_SOLID, 2, RGB(148, 163, 184));
    HBRUSH bodyBrush = CreateSolidBrush(RGB(241, 245, 249));
    HGDIOBJ op = SelectObject(dc, bodyPen);
    HGDIOBJ ob = SelectObject(dc, bodyBrush);

    // Screen lid
    POINT screenPts[4] = {
        { cx - UiMetrics::Scale(80, dpi), cy - UiMetrics::Scale(70, dpi) },
        { cx + UiMetrics::Scale(70, dpi), cy - UiMetrics::Scale(60, dpi) },
        { cx + UiMetrics::Scale(60, dpi), cy + UiMetrics::Scale(10, dpi) },
        { cx - UiMetrics::Scale(70, dpi), cy + UiMetrics::Scale(5, dpi) }
    };
    Polygon(dc, screenPts, 4);

    // Base body
    POINT basePts[4] = {
        { cx - UiMetrics::Scale(70, dpi), cy + UiMetrics::Scale(5, dpi) },
        { cx + UiMetrics::Scale(60, dpi), cy + UiMetrics::Scale(10, dpi) },
        { cx + UiMetrics::Scale(80, dpi), cy + UiMetrics::Scale(65, dpi) },
        { cx - UiMetrics::Scale(90, dpi), cy + UiMetrics::Scale(60, dpi) }
    };
    Polygon(dc, basePts, 4);

    SelectObject(dc, op);
    SelectObject(dc, ob);
    DeleteObject(bodyPen);
    DeleteObject(bodyBrush);

    // Hotspot badges (1: Screen, 2: Hinge, 3: Palmrest, 4: Front edge, 5: Base bottom)
    struct Hotspot { int id; int x; int y; };
    std::vector<Hotspot> spots = {
        { 1, cx, cy - UiMetrics::Scale(35, dpi) },
        { 2, cx - UiMetrics::Scale(5, dpi), cy + UiMetrics::Scale(8, dpi) },
        { 3, cx - UiMetrics::Scale(40, dpi), cy + UiMetrics::Scale(45, dpi) },
        { 4, cx, cy + UiMetrics::Scale(62, dpi) },
        { 5, cx + UiMetrics::Scale(30, dpi), cy + UiMetrics::Scale(48, dpi) }
    };

    SelectObject(dc, fonts.hSmall);
    SetBkMode(dc, TRANSPARENT);
    for (const auto& sp : spots) {
        int sz = UiMetrics::Scale(20, dpi);
        RECT sr{ sp.x - sz / 2, sp.y - sz / 2, sp.x + sz / 2, sp.y + sz / 2 };
        bool isActive = (sp.id == activeHotspot);
        COLORREF bg = isActive ? UiColors::PrimaryBlue : RGB(100, 116, 139);
        DrawRoundedCard(dc, sr, sz / 2, bg, bg, 1);
        SetTextColor(dc, RGB(255, 255, 255));
        std::wstring idStr = std::to_wstring(sp.id);
        DrawTextW(dc, idStr.c_str(), (int)idStr.size(), &sr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

void DrawPortChassisDiagram(HDC dc, const RECT& r, const UiFonts& fonts, int dpi) {
    DrawRoundedCard(dc, r, UiMetrics::RadiusMd, RGB(255, 255, 255), UiColors::CardBorder, 1);
    
    int halfW = (r.right - r.left - UiMetrics::Scale(36, dpi)) / 2;
    int sideH = UiMetrics::Scale(32, dpi);
    
    // Left Edge
    RECT leftSide{ r.left + UiMetrics::Scale(16, dpi), r.top + UiMetrics::Scale(36, dpi), r.left + UiMetrics::Scale(16, dpi) + halfW, r.top + UiMetrics::Scale(36, dpi) + sideH };
    DrawRoundedCard(dc, leftSide, UiMetrics::RadiusSm, RGB(226, 232, 240), RGB(148, 163, 184), 1);
    
    // Right Edge
    RECT rightSide{ r.right - UiMetrics::Scale(16, dpi) - halfW, r.top + UiMetrics::Scale(36, dpi), r.right - UiMetrics::Scale(16, dpi), r.top + UiMetrics::Scale(36, dpi) + sideH };
    DrawRoundedCard(dc, rightSide, UiMetrics::RadiusSm, RGB(226, 232, 240), RGB(148, 163, 184), 1);

    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, leftSide.left, r.top + UiMetrics::Scale(14, dpi), L"Cạnh trái (Left)", 16);
    TextOutW(dc, rightSide.left, r.top + UiMetrics::Scale(14, dpi), L"Cạnh phải (Right)", 17);

    // Numbered port pins on left (1 to 4)
    for (int i = 0; i < 4; ++i) {
        int px = leftSide.left + UiMetrics::Scale(20 + i * 35, dpi);
        int py = leftSide.bottom + UiMetrics::Scale(8, dpi);
        int sz = UiMetrics::Scale(18, dpi);
        RECT pr{ px - sz / 2, py, px + sz / 2, py + sz };
        DrawRoundedCard(dc, pr, sz / 2, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
        SetTextColor(dc, RGB(255, 255, 255));
        std::wstring idStr = std::to_wstring(i + 1);
        DrawTextW(dc, idStr.c_str(), (int)idStr.size(), &pr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // Numbered port pins on right (5 to 8)
    for (int i = 0; i < 4; ++i) {
        int px = rightSide.left + UiMetrics::Scale(20 + i * 35, dpi);
        int py = rightSide.bottom + UiMetrics::Scale(8, dpi);
        int sz = UiMetrics::Scale(18, dpi);
        RECT pr{ px - sz / 2, py, px + sz / 2, py + sz };
        DrawRoundedCard(dc, pr, sz / 2, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
        SetTextColor(dc, RGB(255, 255, 255));
        std::wstring idStr = std::to_wstring(i + 5);
        DrawTextW(dc, idStr.c_str(), (int)idStr.size(), &pr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
}

} // namespace lap
