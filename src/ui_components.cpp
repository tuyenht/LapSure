#include "lap/ui_components.h"
#include <algorithm>

namespace lap {

bool gReturnToAutoAudit = false;

// ============================================================
// C01 — App Shell
// ============================================================

AppShellLayout ComputeAppShellLayout(const RECT& clientRect, int dpi) {
    AppShellLayout layout;
    layout.dpi = dpi;
    int sidebarW = UiMetrics::Scale(UiMetrics::SidebarWidth, dpi);
    int footerH = UiMetrics::Scale(32, dpi);

    layout.sidebarRect = { clientRect.left, clientRect.top, clientRect.left + sidebarW, clientRect.bottom };
    layout.footerRect = { clientRect.left + sidebarW, clientRect.bottom - footerH, clientRect.right, clientRect.bottom };
    layout.headerRect = { clientRect.left + sidebarW, clientRect.top, clientRect.right, clientRect.top + UiMetrics::Scale(60, dpi) };
    layout.contentRect = { clientRect.left + sidebarW, clientRect.top, clientRect.right, clientRect.bottom - footerH };
    return layout;
}

void DrawAppShellBackground(HDC dc, const RECT& clientRect) {
    HBRUSH bgBrush = CreateSolidBrush(UiColors::ContentBg);
    FillRect(dc, &clientRect, bgBrush);
    DeleteObject(bgBrush);
}

void DrawAppShellFooter(HDC dc, const RECT& r, const UiFonts& fonts, int dpi, int readyEngines, int totalEngines) {
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
    SelectObject(dc, fonts.hSmall);
    
    // Column 1: Engine status
    SetTextColor(dc, readyEngines >= totalEngines ? UiColors::SuccessGreen : UiColors::WarnAmber);
    std::wstring engineText = L"● Engine: " + std::to_wstring(readyEngines) + L"/" + std::to_wstring(totalEngines) + L" sẵn sàng";
    RECT c1{ r.left + UiMetrics::Scale(16, dpi), r.top, r.left + UiMetrics::Scale(200, dpi), r.bottom };
    DrawTextW(dc, engineText.c_str(), (int)engineText.size(), &c1, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Column 2: DB Date
    SetTextColor(dc, UiColors::TextMuted);
    RECT c2{ c1.right + UiMetrics::Scale(10, dpi), r.top, c1.right + UiMetrics::Scale(180, dpi), r.bottom };
    DrawTextW(dc, L"Cơ sở dữ liệu: 2026.08.23", 25, &c2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Column 3: Mode
    RECT c3{ c2.right + UiMetrics::Scale(10, dpi), r.top, r.right - UiMetrics::Scale(200, dpi), r.bottom };
    DrawTextW(dc, L"Chế độ: Win32 Native C++20", 27, &c3, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Column 4: Policy
    RECT c4{ r.right - UiMetrics::Scale(190, dpi), r.top, r.right - UiMetrics::Scale(16, dpi), r.bottom };
    DrawTextW(dc, L"Chính sách: Evidence First", 26, &c4, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
}

// ============================================================
// C02 — Sidebar & Grouped Navigation
// ============================================================

std::vector<SidebarGroup> GetDefaultSidebarGroups(bool deviceGroupExpanded) {
    std::vector<SidebarGroup> groups;

    // Group 1: QUY TRÌNH
    SidebarGroup g1;
    g1.group = NavGroup::Workflow;
    g1.name = L"QUY TRÌNH";
    g1.isCollapsible = false;
    g1.isExpanded = true;
    g1.items = {
        { MainTab::Dashboard, L"🏠", L"Tổng quan" },
        { MainTab::NewSession, L"➕", L"Phiên kiểm định mới" },
        { MainTab::AutoAudit, L"⚡", L"Kiểm tra Tự động" },
        { MainTab::Functional, L"🛠️", L"Kiểm tra Chức năng" },
        { MainTab::PhysicalSafety, L"🔍", L"Ngoại hình & An toàn" },
        { MainTab::PortsPower, L"🔌", L"Cổng & Nguồn" },
        { MainTab::Stress, L"📈", L"Stress & Ổn định" }
    };
    groups.push_back(g1);

    // Group 2: CHI TIẾT THIẾT BỊ (Collapsible) - Bỏ khỏi menu trái theo thiết kế mới, truy cập qua S04
    SidebarGroup g2;
    g2.group = NavGroup::DeviceDetails;
    g2.name = L"CHI TIẾT THIẾT BỊ";
    g2.isCollapsible = true;
    g2.isExpanded = deviceGroupExpanded;
    g2.items = {
        { MainTab::Battery, L"🔋", L"Pin & Nguồn" },
        { MainTab::Storage, L"💾", L"Lưu trữ" },
        { MainTab::Memory, L"🧠", L"Bộ nhớ (RAM)" },
        { MainTab::Display, L"🖥️", L"Hiển thị" },
        { MainTab::AudioCamera, L"🔊", L"Âm thanh & Cam" },
        { MainTab::Network, L"📶", L"Mạng & Kết nối" },
        { MainTab::SystemInfo, L"ℹ️", L"Thông tin Hệ thống" }
    };
    // groups.push_back(g2); // LAPSURE_UI: Ẩn khỏi sidebar, chỉ truy cập qua nút bảng thông số ở S04

    // Group 3: ĐÁNH GIÁ & HỒ SƠ
    SidebarGroup g3;
    g3.group = NavGroup::AuditAndRecords;
    g3.name = L"ĐÁNH GIÁ & HỒ SƠ";
    g3.isCollapsible = false;
    g3.isExpanded = true;
    g3.items = {
        { MainTab::SellerClaim, L"📋", L"Cam kết người bán" },
        { MainTab::FactoryProfileMatch, L"🏭", L"Hồ sơ Nhà máy" },
        { MainTab::EvidenceLibrary, L"📚", L"Thư viện Bằng chứng" },
        { MainTab::Reports, L"📊", L"Báo cáo & Đánh giá" },
        { MainTab::ExportShare, L"📤", L"Xuất báo cáo" }
    };
    groups.push_back(g3);

    // Group 4: HỆ THỐNG
    SidebarGroup g4;
    g4.group = NavGroup::SystemSettings;
    g4.name = L"HỆ THỐNG";
    g4.isCollapsible = false;
    g4.isExpanded = true;
    g4.items = {
        { MainTab::LogsEvents, L"📜", L"Nhật ký & Sự kiện" },
        { MainTab::Settings, L"⚙️", L"Cài đặt" },
        { MainTab::SessionHistory, L"🕒", L"Lịch sử kiểm định" }
    };
    groups.push_back(g4);

    return groups;
}

void DrawSidebar(HDC dc, const RECT& r, MainTab activeTab, const UiFonts& fonts, int dpi, bool deviceGroupExpanded, int scrollOffsetY, const std::wstring& sessionStatus, const std::wstring& osVersion) {
    HBRUSH b = CreateSolidBrush(UiColors::SidebarBg);
    FillRect(dc, &r, b);
    DeleteObject(b);

    // Right border
    HPEN p = CreatePen(PS_SOLID, 1, UiColors::SidebarBorder);
    HGDIOBJ op = SelectObject(dc, p);
    MoveToEx(dc, r.right - 1, r.top, nullptr);
    LineTo(dc, r.right - 1, r.bottom);
    SelectObject(dc, op);
    DeleteObject(p);

    SetBkMode(dc, TRANSPARENT);

    // Brand Logo & Title with Emblem
    int logoS = UiMetrics::Scale(36, dpi);
    int logoX = r.left + UiMetrics::Scale(16, dpi);
    int logoY = r.top + UiMetrics::Scale(16, dpi);
    RECT logoRect{ logoX, logoY, logoX + logoS, logoY + logoS };
    DrawRoundedCard(dc, logoRect, UiMetrics::Scale(8, dpi), UiColors::PrimaryBlue, RGB(96, 165, 250), 1);

    // Inner Laptop Base & Checkmark in emblem
    int pad = UiMetrics::Scale(6, dpi);
    int deckY = logoRect.bottom - pad - UiMetrics::Scale(4, dpi);
    RECT deckRect{ logoRect.left + pad, deckY, logoRect.right - pad, deckY + UiMetrics::Scale(3, dpi) };
    DrawRoundedCard(dc, deckRect, 1, RGB(226, 232, 240), RGB(226, 232, 240), 1);

    // Emerald Checkmark circle
    int chkR = UiMetrics::Scale(7, dpi);
    int chkCX = (logoRect.left + logoRect.right) / 2;
    int chkCY = logoRect.top + UiMetrics::Scale(15, dpi);
    HBRUSH gBrush = CreateSolidBrush(RGB(16, 185, 129));
    HPEN gPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    HGDIOBJ oldGb = SelectObject(dc, gBrush);
    HGDIOBJ oldGp = SelectObject(dc, gPen);
    Ellipse(dc, chkCX - chkR, chkCY - chkR, chkCX + chkR, chkCY + chkR);
    SelectObject(dc, oldGb); SelectObject(dc, oldGp);
    DeleteObject(gBrush); DeleteObject(gPen);

    // White Checkmark lines
    HPEN wPen = CreatePen(PS_SOLID, UiMetrics::Scale(2, dpi), RGB(255, 255, 255));
    HGDIOBJ oldWp = SelectObject(dc, wPen);
    MoveToEx(dc, chkCX - UiMetrics::Scale(4, dpi), chkCY, nullptr);
    LineTo(dc, chkCX - UiMetrics::Scale(1, dpi), chkCY + UiMetrics::Scale(3, dpi));
    LineTo(dc, chkCX + UiMetrics::Scale(4, dpi), chkCY - UiMetrics::Scale(3, dpi));
    SelectObject(dc, oldWp);
    DeleteObject(wPen);

    int textX = logoRect.right + UiMetrics::Scale(10, dpi);
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextWhite);
    std::wstring brand = L"LapSure";
    TextOutW(dc, textX, r.top + UiMetrics::Scale(16, dpi), brand.c_str(), (int)brand.size());

    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, RGB(147, 197, 253));
    std::wstring tag = L"PRO DASHBOARD";
    TextOutW(dc, textX, r.top + UiMetrics::Scale(34, dpi), tag.c_str(), (int)tag.size());

    // Navigation Groups
    auto groups = GetDefaultSidebarGroups(deviceGroupExpanded);
    int y = r.top + UiMetrics::Scale(75, dpi) - scrollOffsetY;
    int botH = UiMetrics::Scale(70, dpi);
    int maxContentY = r.bottom - botH - UiMetrics::Scale(12, dpi);

    for (const auto& grp : groups) {
        if (y > maxContentY) break;

        if (y >= r.top + UiMetrics::Scale(55, dpi)) {
            SetTextColor(dc, UiColors::SidebarGroupHeader);
            SelectObject(dc, fonts.hSmall);
            std::wstring grpTitle = grp.name;
            if (grp.isCollapsible) {
                grpTitle += grp.isExpanded ? L"  ▲" : L"  ▼ (thu gọn)";
            }
            TextOutW(dc, r.left + UiMetrics::Scale(16, dpi), y, grpTitle.c_str(), (int)grpTitle.size());
        }
        y += UiMetrics::Scale(18, dpi);

        if (grp.isExpanded) {
            for (const auto& item : grp.items) {
                bool active = (item.tab == activeTab);
                int itemH = UiMetrics::Scale(26, dpi);
                RECT itemRect{ r.left + UiMetrics::Scale(10, dpi), y, r.right - UiMetrics::Scale(10, dpi), y + itemH };

                if (y + itemH > r.top + UiMetrics::Scale(60, dpi) && y < maxContentY) {
                    if (active) {
                        DrawRoundedCard(dc, itemRect, UiMetrics::RadiusSm, UiColors::SidebarActive, UiColors::SidebarActive, 1);
                        SetTextColor(dc, UiColors::SidebarTextActive);
                        SelectObject(dc, fonts.hBodyBold);
                        DrawFocusRing(dc, itemRect, UiMetrics::RadiusSm);
                    } else {
                        SetTextColor(dc, UiColors::SidebarText);
                        SelectObject(dc, fonts.hBody);
                    }

                    std::wstring text = std::wstring(item.icon) + L"  " + item.title;
                    RECT textRect{ itemRect.left + UiMetrics::Scale(10, dpi), itemRect.top + UiMetrics::Scale(4, dpi), itemRect.right - UiMetrics::Scale(6, dpi), itemRect.bottom };
                    DrawTextW(dc, text.c_str(), (int)text.size(), &textRect, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
                }

                y += itemH + UiMetrics::Scale(2, dpi);
            }
        }
        y += UiMetrics::Scale(4, dpi);
    }

    // Bottom Session Status Card
    RECT botCard{ r.left + UiMetrics::Scale(10, dpi), r.bottom - botH - UiMetrics::Scale(8, dpi), r.right - UiMetrics::Scale(10, dpi), r.bottom - UiMetrics::Scale(8, dpi) };
    DrawRoundedCard(dc, botCard, UiMetrics::RadiusMd, RGB(16, 32, 58), RGB(26, 48, 80), 1);

    SetTextColor(dc, UiColors::SuccessGreen);
    SelectObject(dc, fonts.hSmall);
    std::wstring stText = L"● " + sessionStatus;
    TextOutW(dc, botCard.left + UiMetrics::Scale(10, dpi), botCard.top + UiMetrics::Scale(6, dpi), stText.c_str(), (int)stText.size());

    SetTextColor(dc, RGB(220, 230, 245));
    TextOutW(dc, botCard.left + UiMetrics::Scale(10, dpi), botCard.top + UiMetrics::Scale(22, dpi), osVersion.c_str(), (int)osVersion.size());
    SetTextColor(dc, UiColors::SidebarText);
    TextOutW(dc, botCard.left + UiMetrics::Scale(10, dpi), botCard.top + UiMetrics::Scale(38, dpi), L"C++20 Win32 Native", 19);
}

int HitTestSidebar(int x, int y, const RECT& sidebarRect, int dpi, bool deviceGroupExpanded, int scrollOffsetY, MainTab& outTab, bool& outToggleDeviceGroup) {
    outToggleDeviceGroup = false;
    if (x < sidebarRect.left || x > sidebarRect.right) return 0;
    int curY = sidebarRect.top + UiMetrics::Scale(70, dpi) - scrollOffsetY;
    auto groups = GetDefaultSidebarGroups(deviceGroupExpanded);

    int botH = UiMetrics::Scale(60, dpi);
    int maxContentY = sidebarRect.bottom - botH - UiMetrics::Scale(10, dpi);

    for (const auto& grp : groups) {
        // Group Header Hit Test (for collapsible)
        if (grp.isCollapsible && y >= curY && y < curY + UiMetrics::Scale(18, dpi) && y < maxContentY) {
            outToggleDeviceGroup = true;
            return 2; // Toggled group
        }
        curY += UiMetrics::Scale(18, dpi);

        if (grp.isExpanded) {
            for (const auto& item : grp.items) {
                int itemH = UiMetrics::Scale(25, dpi);
                if (y >= curY && y < curY + itemH && y < maxContentY) {
                    outTab = item.tab;
                    return 1;
                }
                curY += itemH + UiMetrics::Scale(2, dpi);
            }
        }
        curY += UiMetrics::Scale(4, dpi);
    }
    return 0;
}

// ============================================================
// C03 — Page Header
// ============================================================

void DrawPageHeader(HDC dc, const RECT& r, const PageHeaderConfig& config, const UiFonts& fonts, int dpi) {
    SetBkMode(dc, TRANSPARENT);
    
    // Title
    SelectObject(dc, fonts.hTitle);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, r.left + UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(14, dpi), config.title.c_str(), (int)config.title.size());

    // Subtitle
    if (!config.subtitle.empty()) {
        SelectObject(dc, fonts.hSmall);
        SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, r.left + UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(42, dpi), config.subtitle.c_str(), (int)config.subtitle.size());
    }

    // Session Status Tag
    if (!config.sessionTag.empty()) {
        SelectObject(dc, fonts.hSmall);
        SIZE sz{};
        GetTextExtentPoint32W(dc, config.sessionTag.c_str(), static_cast<int>(config.sessionTag.size()), &sz);
        const int badgeW = sz.cx + UiMetrics::Scale(32, dpi);
        const int badgeH = UiMetrics::Scale(26, dpi);
        int rightMargin = config.actionButtonText.empty() ? UiMetrics::Scale(24, dpi) : UiMetrics::Scale(160, dpi);
        int tagX = r.right - rightMargin - badgeW;
        int tagY = r.top + UiMetrics::Scale(14, dpi);
        DrawStatusBadge(dc, tagX, tagY, badgeW, badgeH, config.sessionState, fonts, config.sessionTag);
    }

    // Action Button
    if (!config.actionButtonText.empty()) {
        RECT btnRect{ r.right - UiMetrics::Scale(140, dpi), r.top + UiMetrics::Scale(14, dpi), r.right - UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(44, dpi) };
        COLORREF btnBg = config.actionButtonEnabled ? UiColors::PrimaryBlue : UiColors::GrayPillBg;
        COLORREF btnBorder = config.actionButtonEnabled ? UiColors::PrimaryBlue : UiColors::GrayPillBorder;
        DrawRoundedCard(dc, btnRect, UiMetrics::RadiusPill, btnBg, btnBorder, 1);
        SetTextColor(dc, config.actionButtonEnabled ? RGB(255, 255, 255) : UiColors::TextMuted);
        SelectObject(dc, fonts.hBodyBold);
        DrawTextW(dc, config.actionButtonText.c_str(), (int)config.actionButtonText.size(), &btnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
}

// ============================================================
// C04 — Status Badge
// ============================================================

void DrawStatusBadge(HDC dc, const RECT& r, CanonicalUiState state, const UiFonts& fonts, const std::wstring& overrideLabel) {
    StatePresentation p = GetStatePresentation(state);
    std::wstring displayLabel = overrideLabel.empty() ? p.label : overrideLabel;
    std::wstring fullText = std::wstring(p.icon) + L" " + displayLabel;

    DrawRoundedCard(dc, r, (r.bottom - r.top) / 2, p.bgColor, p.borderColor, 1);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, p.textColor);
    HGDIOBJ oldFont = SelectObject(dc, fonts.hSmall);
    RECT tr = r;
    DrawTextW(dc, fullText.c_str(), (int)fullText.size(), &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, oldFont);
}

void DrawStatusBadge(HDC dc, int x, int y, int w, int h, CanonicalUiState state, const UiFonts& fonts, const std::wstring& overrideLabel) {
    RECT r{ x, y, x + w, y + h };
    DrawStatusBadge(dc, r, state, fonts, overrideLabel);
}

// ============================================================
// C05 — Metric Card
// ============================================================

void DrawMetricCard(HDC dc, const RECT& r, const MetricCardConfig& config, const UiFonts& fonts, int dpi) {
    DrawRoundedCard(dc, r, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    SetBkMode(dc, TRANSPARENT);
    
    // Label
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    RECT lr{ r.left + UiMetrics::Scale(12, dpi), r.top + UiMetrics::Scale(8, dpi), r.right - UiMetrics::Scale(12, dpi), r.top + UiMetrics::Scale(24, dpi) };
    DrawTextW(dc, config.label.c_str(), (int)config.label.size(), &lr, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Value
    StatePresentation sp = GetStatePresentation(config.state);
    COLORREF valClr = config.hasBadge ? sp.textColor : UiColors::TextMain;
    SetTextColor(dc, valClr);
    
    if (config.value.size() > 8) {
        SelectObject(dc, fonts.hBodyBold);
    } else {
        SelectObject(dc, fonts.hTitle);
    }
    
    RECT vr{ r.left + UiMetrics::Scale(12, dpi), r.top + UiMetrics::Scale(24, dpi), r.right - UiMetrics::Scale(12, dpi), r.top + UiMetrics::Scale(54, dpi) };
    DrawTextW(dc, config.value.c_str(), (int)config.value.size(), &vr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Subtitle Note
    if (!config.unitOrSource.empty() || !config.note.empty()) {
        SelectObject(dc, fonts.hSmall);
        SetTextColor(dc, UiColors::TextLight);
        std::wstring sub = config.unitOrSource.empty() ? config.note : (config.unitOrSource + (config.note.empty() ? L"" : (L" • " + config.note)));
        RECT nr{ r.left + UiMetrics::Scale(12, dpi), r.bottom - UiMetrics::Scale(22, dpi), r.right - UiMetrics::Scale(12, dpi), r.bottom - UiMetrics::Scale(4, dpi) };
        DrawTextW(dc, sub.c_str(), (int)sub.size(), &nr, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
}

// ============================================================
// C06 — Progress & Coverage
// ============================================================

void DrawProgressCoverage(HDC dc, const RECT& r, const ProgressCoverageConfig& config, const UiFonts& fonts, int dpi) {
    SetBkMode(dc, TRANSPARENT);
    
    int pct = config.total > 0 ? (config.completed * 100 / config.total) : 0;
    pct = std::clamp(pct, 0, 100);

    // Label & Ratio
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    std::wstring title = config.label + L" (" + std::to_wstring(config.completed) + L" / " + std::to_wstring(config.total) + L")";
    TextOutW(dc, r.left, r.top, title.c_str(), (int)title.size());

    // Percentage
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, config.barColor);
    std::wstring pctStr = std::to_wstring(pct) + L"%";
    RECT pctRect{ r.right - UiMetrics::Scale(50, dpi), r.top, r.right, r.top + UiMetrics::Scale(20, dpi) };
    DrawTextW(dc, pctStr.c_str(), (int)pctStr.size(), &pctRect, DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX);

    // Bar
    int barH = UiMetrics::Scale(10, dpi);
    RECT barRect{ r.left, r.top + UiMetrics::Scale(22, dpi), r.right, r.top + UiMetrics::Scale(22, dpi) + barH };
    DrawModernProgressBar(dc, barRect, pct, config.barColor, config.bgColor);
}

// ============================================================
// C07 — Guided Stepper
// ============================================================

void DrawGuidedStepper(HDC dc, const RECT& r, const std::vector<StepperStep>& steps, const UiFonts& fonts, int dpi) {
    if (steps.empty()) return;
    int stepCount = (int)steps.size();
    int totalW = r.right - r.left;
    int totalH = r.bottom - r.top;

    bool isVertical = (totalW < UiMetrics::Scale(550, dpi)) || (totalH > totalW * 0.5);

    if (isVertical) {
        int stepH = totalH / stepCount;
        for (int i = 0; i < stepCount; ++i) {
            const auto& s = steps[i];
            int sy = r.top + i * stepH;
            RECT stepRect{ r.left, sy + UiMetrics::Scale(2, dpi), r.right, sy + stepH - UiMetrics::Scale(4, dpi) };

            COLORREF cardBg = s.isCurrent ? RGB(239, 246, 255) : UiColors::CardBg;
            COLORREF borderClr = s.isCurrent ? UiColors::PrimaryBlue : UiColors::CardBorder;
            int borderW = s.isCurrent ? 2 : 1;
            DrawRoundedCard(dc, stepRect, UiMetrics::RadiusSm, cardBg, borderClr, borderW);

            SetBkMode(dc, TRANSPARENT);
            
            // Step Number Circle
            int circleD = UiMetrics::Scale(20, dpi);
            int circleX = stepRect.left + UiMetrics::Scale(8, dpi);
            int circleY = stepRect.top + UiMetrics::Scale(8, dpi);
            COLORREF circleBg = s.isCurrent ? UiColors::PrimaryBlue : (s.state == CanonicalUiState::Good || s.state == CanonicalUiState::Pass ? UiColors::SuccessGreen : RGB(226, 232, 240));
            COLORREF circleText = (s.isCurrent || s.state == CanonicalUiState::Good || s.state == CanonicalUiState::Pass) ? RGB(255, 255, 255) : UiColors::TextMain;

            HBRUSH cBrush = CreateSolidBrush(circleBg);
            HPEN cPen = CreatePen(PS_SOLID, 1, circleBg);
            HGDIOBJ op = SelectObject(dc, cPen);
            HGDIOBJ ob = SelectObject(dc, cBrush);
            Ellipse(dc, circleX, circleY, circleX + circleD, circleY + circleD);
            SelectObject(dc, op); SelectObject(dc, ob);
            DeleteObject(cPen); DeleteObject(cBrush);

            SetTextColor(dc, circleText);
            SelectObject(dc, fonts.hBodyBold);
            std::wstring numStr = std::to_wstring(s.stepNumber);
            RECT numRect{ circleX, circleY, circleX + circleD, circleY + circleD };
            DrawTextW(dc, numStr.c_str(), (int)numStr.size(), &numRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

            // Step Title
            int textX = circleX + circleD + UiMetrics::Scale(8, dpi);
            SelectObject(dc, fonts.hBodyBold);
            SetTextColor(dc, s.isCurrent ? UiColors::PrimaryBlue : UiColors::TextMain);
            RECT titleRect{ textX, stepRect.top + UiMetrics::Scale(4, dpi), stepRect.right - UiMetrics::Scale(6, dpi), stepRect.top + UiMetrics::Scale(20, dpi) };
            DrawTextW(dc, s.title.c_str(), (int)s.title.size(), &titleRect, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

            // Step Description
            SelectObject(dc, fonts.hSmall);
            SetTextColor(dc, UiColors::TextMuted);
            RECT descRect{ textX, stepRect.top + UiMetrics::Scale(20, dpi), stepRect.right - UiMetrics::Scale(6, dpi), stepRect.bottom - UiMetrics::Scale(2, dpi) };
            DrawTextW(dc, s.description.c_str(), (int)s.description.size(), &descRect, DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX);
        }
    } else {
        int stepW = totalW / stepCount;
        for (int i = 0; i < stepCount; ++i) {
            const auto& s = steps[i];
            int sx = r.left + i * stepW;
            RECT stepRect{ sx + UiMetrics::Scale(4, dpi), r.top, sx + stepW - UiMetrics::Scale(4, dpi), r.bottom };

            COLORREF borderClr = s.isCurrent ? UiColors::PrimaryBlue : UiColors::CardBorder;
            int borderW = s.isCurrent ? 2 : 1;
            DrawRoundedCard(dc, stepRect, UiMetrics::RadiusMd, UiColors::CardBg, borderClr, borderW);

            SetBkMode(dc, TRANSPARENT);
            SelectObject(dc, fonts.hBodyBold);
            SetTextColor(dc, s.isCurrent ? UiColors::PrimaryBlue : UiColors::TextMain);
            std::wstring numTitle = std::to_wstring(s.stepNumber) + L". " + s.title;
            RECT tr{ stepRect.left + UiMetrics::Scale(8, dpi), stepRect.top + UiMetrics::Scale(8, dpi), stepRect.right - UiMetrics::Scale(8, dpi), stepRect.top + UiMetrics::Scale(26, dpi) };
            DrawTextW(dc, numTitle.c_str(), (int)numTitle.size(), &tr, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

            SelectObject(dc, fonts.hSmall);
            SetTextColor(dc, UiColors::TextMuted);
            RECT dr{ stepRect.left + UiMetrics::Scale(8, dpi), stepRect.top + UiMetrics::Scale(28, dpi), stepRect.right - UiMetrics::Scale(8, dpi), stepRect.bottom - UiMetrics::Scale(6, dpi) };
            DrawTextW(dc, s.description.c_str(), (int)s.description.size(), &dr, DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);
        }
    }
}

// ============================================================
// C08 — Evidence Row
// ============================================================

void DrawEvidenceRow(HDC dc, const RECT& r, const EvidenceRowConfig& config, const UiFonts& fonts, int dpi, bool alternateBg) {
    COLORREF bg = alternateBg ? UiColors::TableRowAlt : UiColors::CardBg;
    DrawRoundedCard(dc, r, UiMetrics::RadiusSm, bg, UiColors::CardBorder, 1);

    SetBkMode(dc, TRANSPARENT);
    // Domain & Parameter
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    std::wstring nameStr = config.domain.empty() ? config.parameter : (config.domain + L" • " + config.parameter);
    TextOutW(dc, r.left + UiMetrics::Scale(12, dpi), r.top + UiMetrics::Scale(6, dpi), nameStr.c_str(), (int)nameStr.size());

    // Actual vs Expected
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    std::wstring valStr = L"Giá trị: " + config.actualValue;
    if (!config.expectedValue.empty()) valStr += L" (Chuẩn: " + config.expectedValue + L")";
    TextOutW(dc, r.left + UiMetrics::Scale(12, dpi), r.top + UiMetrics::Scale(22, dpi), valStr.c_str(), (int)valStr.size());

    // Provider Source
    if (!config.providerSource.empty()) {
        TextOutW(dc, r.left + UiMetrics::Scale(360, dpi), r.top + UiMetrics::Scale(14, dpi), config.providerSource.c_str(), (int)config.providerSource.size());
    }

    // Status Badge
    int bw = UiMetrics::Scale(90, dpi);
    int bh = UiMetrics::Scale(22, dpi);
    DrawStatusBadge(dc, r.right - bw - UiMetrics::Scale(12, dpi), r.top + UiMetrics::Scale(9, dpi), bw, bh, config.state, fonts);
}

// ============================================================
// C09 — Data Table
// ============================================================

void DrawDataTable(HDC dc, const RECT& r, const DataTableConfig& config, const UiFonts& fonts, int dpi, int scrollRowOffset) {
    DrawRoundedCard(dc, r, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    if (config.columns.empty()) return;

    int headerH = UiMetrics::Scale(32, dpi);
    RECT hr{ r.left, r.top, r.right, r.top + headerH };
    DrawRoundedCard(dc, hr, UiMetrics::RadiusMd, UiColors::TableHeaderBg, UiColors::TableBorder, 1);

    // Draw Column Headers
    int curX = r.left + UiMetrics::Scale(12, dpi);
    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);

    for (const auto& col : config.columns) {
        int w = UiMetrics::Scale(col.widthPx, dpi);
        RECT chRect{ curX, r.top + UiMetrics::Scale(8, dpi), curX + w, r.top + headerH };
        DrawTextW(dc, col.header.c_str(), (int)col.header.size(), &chRect, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
        curX += w + UiMetrics::Scale(8, dpi);
    }

    // Rows
    if (config.rows.empty()) {
        SelectObject(dc, fonts.hBody);
        SetTextColor(dc, UiColors::TextMuted);
        RECT emRect{ r.left, r.top + headerH, r.right, r.bottom };
        DrawTextW(dc, config.emptyMessage.c_str(), (int)config.emptyMessage.size(), &emRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        return;
    }

    int rowY = r.top + headerH;
    int rowH = UiMetrics::Scale(UiMetrics::TableRowHeight, dpi);
    size_t startRow = (size_t)std::max(0, scrollRowOffset);

    for (size_t i = startRow; i < config.rows.size() && rowY + rowH <= r.bottom; ++i) {
        const auto& row = config.rows[i];
        RECT rr{ r.left, rowY, r.right, rowY + rowH };
        if (i % 2 == 1) {
            HBRUSH rb = CreateSolidBrush(UiColors::TableRowAlt);
            FillRect(dc, &rr, rb);
            DeleteObject(rb);
        }

        // Bottom row divider
        HPEN divPen = CreatePen(PS_SOLID, 1, UiColors::TableBorder);
        HGDIOBJ op = SelectObject(dc, divPen);
        MoveToEx(dc, r.left, rowY + rowH, nullptr);
        LineTo(dc, r.right, rowY + rowH);
        SelectObject(dc, op);
        DeleteObject(divPen);

        int rx = r.left + UiMetrics::Scale(12, dpi);
        for (size_t c = 0; c < config.columns.size() && c < row.cells.size(); ++c) {
            const auto& col = config.columns[c];
            int cw = UiMetrics::Scale(col.widthPx, dpi);
            RECT cr{ rx, rowY + UiMetrics::Scale(8, dpi), rx + cw, rowY + rowH };

            if (col.isStatusBadge) {
                DrawStatusBadge(dc, rx, rowY + UiMetrics::Scale(6, dpi), cw, UiMetrics::Scale(22, dpi), row.rowState, fonts, row.cells[c]);
            } else {
                SelectObject(dc, col.isMonospace ? fonts.hMono : fonts.hBody);
                SetTextColor(dc, UiColors::TextMain);
                DrawTextW(dc, row.cells[c].c_str(), (int)row.cells[c].size(), &cr, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
            }
            rx += cw + UiMetrics::Scale(8, dpi);
        }
        rowY += rowH;
    }
}

// ============================================================
// C10 — Next Action Panel
// ============================================================

RECT GetNextActionButtonRect(const RECT& panelRect, int dpi) {
    const int btnH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
    return RECT{
        panelRect.left + UiMetrics::Scale(14, dpi),
        panelRect.bottom - btnH - UiMetrics::Scale(12, dpi),
        panelRect.right - UiMetrics::Scale(14, dpi),
        panelRect.bottom - UiMetrics::Scale(12, dpi)
    };
}

void DrawNextActionPanel(HDC dc, const RECT& r, const NextActionConfig& config, const UiFonts& fonts, int dpi) {
    DrawRoundedCard(dc, r, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::PrimaryBlue);
    TextOutW(dc, r.left + UiMetrics::Scale(14, dpi), r.top + UiMetrics::Scale(10, dpi), L"🎯 Bước tiếp theo", 17);

    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, r.left + UiMetrics::Scale(14, dpi), r.top + UiMetrics::Scale(30, dpi), config.actionTitle.c_str(), (int)config.actionTitle.size());

    if (!config.reasonText.empty()) {
        SelectObject(dc, fonts.hSmall);
        SetTextColor(dc, UiColors::TextMuted);
        TextOutW(dc, r.left + UiMetrics::Scale(14, dpi), r.top + UiMetrics::Scale(50, dpi), config.reasonText.c_str(), (int)config.reasonText.size());
    }

    int taskY = r.top + UiMetrics::Scale(70, dpi);
    for (const auto& t : config.remainingTasks) {
        SelectObject(dc, fonts.hSmall);
        SetTextColor(dc, UiColors::TextMuted);
        std::wstring itemStr = t;
        if (itemStr.rfind(L"•", 0) != 0 && itemStr.rfind(L"-", 0) != 0 && itemStr.rfind(L"*", 0) != 0) {
            itemStr = L"• " + itemStr;
        }
        TextOutW(dc, r.left + UiMetrics::Scale(14, dpi), taskY, itemStr.c_str(), (int)itemStr.size());
        taskY += UiMetrics::Scale(18, dpi);
    }

    if (!config.buttonText.empty()) {
        RECT br = GetNextActionButtonRect(r, dpi);
        COLORREF bg = config.isButtonEnabled ? UiColors::PrimaryBlue : UiColors::GrayPillBg;
        COLORREF border = config.isButtonEnabled ? UiColors::PrimaryBlue : UiColors::GrayPillBorder;
        DrawRoundedCard(dc, br, UiMetrics::RadiusPill, bg, border, 1);
        SetTextColor(dc, config.isButtonEnabled ? RGB(255, 255, 255) : UiColors::TextMuted);
        SelectObject(dc, fonts.hBodyBold);
        DrawTextW(dc, config.buttonText.c_str(), (int)config.buttonText.size(), &br, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
}

// ============================================================
// C11 — Dialog Confirmation Pattern
// ============================================================

void DrawDialogBox(HDC dc, const RECT& dialogRect, const DialogConfig& config, const UiFonts& fonts, int dpi) {
    DrawRoundedCard(dc, dialogRect, UiMetrics::RadiusLg, UiColors::CardBg, UiColors::CardBorder, 2);

    SetBkMode(dc, TRANSPARENT);
    // Title
    SelectObject(dc, fonts.hTitle);
    SetTextColor(dc, config.isDestructive ? UiColors::FailRed : UiColors::TextMain);
    TextOutW(dc, dialogRect.left + UiMetrics::Scale(20, dpi), dialogRect.top + UiMetrics::Scale(18, dpi), config.title.c_str(), (int)config.title.size());

    // Message
    SelectObject(dc, fonts.hBody);
    SetTextColor(dc, UiColors::TextMain);
    RECT msgRect{ dialogRect.left + UiMetrics::Scale(20, dpi), dialogRect.top + UiMetrics::Scale(50, dpi), dialogRect.right - UiMetrics::Scale(20, dpi), dialogRect.top + UiMetrics::Scale(110, dpi) };
    DrawTextW(dc, config.message.c_str(), (int)config.message.size(), &msgRect, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);

    // Consequence
    if (!config.consequenceWarning.empty()) {
        RECT warnRect{ dialogRect.left + UiMetrics::Scale(20, dpi), dialogRect.top + UiMetrics::Scale(115, dpi), dialogRect.right - UiMetrics::Scale(20, dpi), dialogRect.top + UiMetrics::Scale(155, dpi) };
        DrawRoundedCard(dc, warnRect, UiMetrics::RadiusSm, UiColors::WarnBg, UiColors::WarnBorder, 1);
        SelectObject(dc, fonts.hSmall);
        SetTextColor(dc, UiColors::WarnAmber);
        RECT wtRect = { warnRect.left + UiMetrics::Scale(8, dpi), warnRect.top + UiMetrics::Scale(6, dpi), warnRect.right - UiMetrics::Scale(8, dpi), warnRect.bottom - UiMetrics::Scale(6, dpi) };
        DrawTextW(dc, config.consequenceWarning.c_str(), (int)config.consequenceWarning.size(), &wtRect, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
    }

    // Cancel Button
    int btnH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
    int btnW = UiMetrics::Scale(100, dpi);
    int btnY = dialogRect.bottom - btnH - UiMetrics::Scale(16, dpi);

    RECT cancelBtn{ dialogRect.right - btnW * 2 - UiMetrics::Scale(24, dpi), btnY, dialogRect.right - btnW - UiMetrics::Scale(24, dpi), btnY + btnH };
    DrawRoundedCard(dc, cancelBtn, UiMetrics::RadiusSm, UiColors::GrayPillBg, UiColors::GrayPillBorder, 1);
    SelectObject(dc, fonts.hBody);
    SetTextColor(dc, UiColors::TextMain);
    DrawTextW(dc, config.cancelButtonText.c_str(), (int)config.cancelButtonText.size(), &cancelBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Confirm Button
    RECT confirmBtn{ dialogRect.right - btnW - UiMetrics::Scale(16, dpi), btnY, dialogRect.right - UiMetrics::Scale(16, dpi), btnY + btnH };
    COLORREF cBg = config.isDestructive ? UiColors::FailRed : UiColors::PrimaryBlue;
    DrawRoundedCard(dc, confirmBtn, UiMetrics::RadiusSm, cBg, cBg, 1);
    SetTextColor(dc, RGB(255, 255, 255));
    SelectObject(dc, fonts.hBodyBold);
    DrawTextW(dc, config.confirmButtonText.c_str(), (int)config.confirmButtonText.size(), &confirmBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

// ============================================================
// C12 — Empty / Error / Unsupported State
// ============================================================

void DrawEmptyState(HDC dc, const RECT& r, const EmptyStateConfig& config, const UiFonts& fonts, int dpi) {
    DrawRoundedCard(dc, r, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    StatePresentation sp = GetStatePresentation(config.state);
    int cy = (r.top + r.bottom) / 2;

    SetBkMode(dc, TRANSPARENT);
    // Large Icon
    SelectObject(dc, fonts.hTitle);
    SetTextColor(dc, sp.textColor);
    RECT iconRect{ r.left, cy - UiMetrics::Scale(60, dpi), r.right, cy - UiMetrics::Scale(25, dpi) };
    DrawTextW(dc, sp.icon, (int)wcslen(sp.icon), &iconRect, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Title
    SelectObject(dc, fonts.hSection);
    SetTextColor(dc, UiColors::TextMain);
    std::wstring t = config.title.empty() ? sp.label : config.title;
    RECT titleRect{ r.left, cy - UiMetrics::Scale(20, dpi), r.right, cy + UiMetrics::Scale(5, dpi) };
    DrawTextW(dc, t.c_str(), (int)t.size(), &titleRect, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Description
    if (!config.description.empty()) {
        SelectObject(dc, fonts.hSmall);
        SetTextColor(dc, UiColors::TextMuted);
        RECT descRect{ r.left + UiMetrics::Scale(40, dpi), cy + UiMetrics::Scale(10, dpi), r.right - UiMetrics::Scale(40, dpi), cy + UiMetrics::Scale(40, dpi) };
        DrawTextW(dc, config.description.c_str(), (int)config.description.size(), &descRect, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
    }

    // Recovery Hint
    if (!config.recoveryHint.empty()) {
        SelectObject(dc, fonts.hSmall);
        SetTextColor(dc, UiColors::PrimaryBlue);
        RECT hintRect{ r.left + UiMetrics::Scale(40, dpi), cy + UiMetrics::Scale(45, dpi), r.right - UiMetrics::Scale(40, dpi), cy + UiMetrics::Scale(65, dpi) };
        DrawTextW(dc, config.recoveryHint.c_str(), (int)config.recoveryHint.size(), &hintRect, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
}

} // namespace lap
