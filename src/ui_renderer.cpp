#include "lap/ui_theme.h"
#include <cmath>
#include <vector>

namespace lap {

void UiFonts::Init() {
    if (hTitle) return;
    hTitle = CreateFontW(-22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hSection = CreateFontW(-15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hBodyBold = CreateFontW(-13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hBody = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hSmall = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hMono = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
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
    DrawTextW(dc, text.c_str(), (int)text.size(), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
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

void DrawCircularScoreGauge(HDC dc, int cx, int cy, int radius, int score, const std::wstring& label, HFONT hTitleFont, HFONT hSubFont) {
    HPEN trackPen = CreatePen(PS_SOLID, 8, RGB(226, 232, 240));
    HGDIOBJ oldPen = SelectObject(dc, trackPen);
    HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HGDIOBJ oldBrush = SelectObject(dc, nullBrush);
    
    Ellipse(dc, cx - radius, cy - radius, cx + radius, cy + radius);
    SelectObject(dc, oldPen);
    DeleteObject(trackPen);

    COLORREF arcClr = score >= 80 ? UiColors::SuccessGreen : (score >= 60 ? UiColors::WarnAmber : UiColors::FailRed);
    HPEN arcPen = CreatePen(PS_SOLID, 8, arcClr);
    SelectObject(dc, arcPen);
    
    double startAngle = -3.14159265 / 2.0;
    double sweepAngle = (double)score / 100.0 * 2.0 * 3.14159265;
    double endAngle = startAngle + sweepAngle;
    
    int startX = cx + (int)(radius * std::cos(startAngle));
    int startY = cy + (int)(radius * std::sin(startAngle));
    int endX = cx + (int)(radius * std::cos(endAngle));
    int endY = cy + (int)(radius * std::sin(endAngle));
    
    Arc(dc, cx - radius, cy - radius, cx + radius, cy + radius, startX, startY, endX, endY);
    
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(arcPen);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, UiColors::TextMain);
    SelectObject(dc, hTitleFont);
    std::wstring scoreStr = std::to_wstring(score);
    RECT tr{ cx - radius, cy - 24, cx + radius, cy + 4 };
    DrawTextW(dc, scoreStr.c_str(), (int)scoreStr.size(), &tr, DT_CENTER | DT_SINGLELINE);
    
    SelectObject(dc, hSubFont);
    SetTextColor(dc, UiColors::TextMuted);
    std::wstring denomStr = L"/100";
    RECT dr{ cx + 18, cy - 16, cx + radius, cy + 2 };
    DrawTextW(dc, denomStr.c_str(), (int)denomStr.size(), &dr, DT_LEFT | DT_SINGLELINE);

    SetTextColor(dc, arcClr);
    RECT lr{ cx - radius, cy + 6, cx + radius, cy + 24 };
    DrawTextW(dc, label.c_str(), (int)label.size(), &lr, DT_CENTER | DT_SINGLELINE);
}

} // namespace lap
