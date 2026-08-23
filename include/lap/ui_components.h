#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "lap/ui_theme.h"
#include "lap/ui_state.h"

namespace lap {

// ============================================================
// C01 — App Shell
// ============================================================
struct AppShellLayout {
    RECT sidebarRect{};
    RECT headerRect{};
    RECT contentRect{};
    RECT footerRect{};
    int dpi{96};
};

AppShellLayout ComputeAppShellLayout(const RECT& clientRect, int dpi);
void DrawAppShellBackground(HDC dc, const RECT& clientRect);
void DrawAppShellFooter(HDC dc, const RECT& r, const UiFonts& fonts, int dpi = 96, int readyEngines = 14, int totalEngines = 14);

// ============================================================
// C02 — Sidebar & Grouped Navigation
// ============================================================
struct SidebarItem {
    MainTab tab;
    const wchar_t* icon;
    const wchar_t* title;
    bool isContextual{false};
};

struct SidebarGroup {
    NavGroup group;
    const wchar_t* name;
    std::vector<SidebarItem> items;
    bool isCollapsible{false};
    bool isExpanded{true};
};

std::vector<SidebarGroup> GetDefaultSidebarGroups(bool deviceGroupExpanded = true);
void DrawSidebar(HDC dc, const RECT& r, MainTab activeTab, const UiFonts& fonts, int dpi, bool deviceGroupExpanded = true, int scrollOffsetY = 0, const std::wstring& sessionStatus = L"Sẵn sàng", const std::wstring& osVersion = L"Windows x64 Native");
int HitTestSidebar(int x, int y, const RECT& sidebarRect, int dpi, bool deviceGroupExpanded, int scrollOffsetY, MainTab& outTab, bool& outToggleDeviceGroup);

// ============================================================
// C03 — Page Header
// ============================================================
struct PageHeaderConfig {
    std::wstring title;
    std::wstring subtitle;
    std::wstring sessionTag;
    CanonicalUiState sessionState{CanonicalUiState::NotTested};
    std::wstring actionButtonText;
    bool actionButtonEnabled{true};
    RECT actionButtonRect{};
};

void DrawPageHeader(HDC dc, const RECT& r, const PageHeaderConfig& config, const UiFonts& fonts, int dpi);

// ============================================================
// C04 — Status Badge
// ============================================================
void DrawStatusBadge(HDC dc, const RECT& r, CanonicalUiState state, const UiFonts& fonts, const std::wstring& overrideLabel = L"");
void DrawStatusBadge(HDC dc, int x, int y, int w, int h, CanonicalUiState state, const UiFonts& fonts, const std::wstring& overrideLabel = L"");

// ============================================================
// C05 — Metric Card
// ============================================================
struct MetricCardConfig {
    std::wstring label;
    std::wstring value;
    std::wstring unitOrSource;
    std::wstring note;
    CanonicalUiState state{CanonicalUiState::NotTested};
    bool hasBadge{false};
};

void DrawMetricCard(HDC dc, const RECT& r, const MetricCardConfig& config, const UiFonts& fonts, int dpi);

// ============================================================
// C06 — Progress & Coverage
// ============================================================
struct ProgressCoverageConfig {
    std::wstring label;
    int completed{0};
    int total{100};
    bool isEvidenceCoverage{false}; // true = "Độ bao phủ bằng chứng", false = "Tiến trình kiểm tra"
    COLORREF barColor{UiColors::PrimaryBlue};
    COLORREF bgColor{RGB(226, 232, 240)};
};

void DrawProgressCoverage(HDC dc, const RECT& r, const ProgressCoverageConfig& config, const UiFonts& fonts, int dpi);

// ============================================================
// C07 — Guided Stepper
// ============================================================
struct StepperStep {
    int stepNumber{1};
    std::wstring title;
    std::wstring description;
    CanonicalUiState state{CanonicalUiState::NotTested};
    bool isCurrent{false};
};

void DrawGuidedStepper(HDC dc, const RECT& r, const std::vector<StepperStep>& steps, const UiFonts& fonts, int dpi);

// ============================================================
// C08 — Evidence Row
// ============================================================
struct EvidenceRowConfig {
    std::wstring domain;
    std::wstring parameter;
    std::wstring actualValue;
    std::wstring expectedValue;
    std::wstring providerSource;
    CanonicalUiState state{CanonicalUiState::NotTested};
    bool isOperatorConfirmed{false};
    bool isMissingEvidence{false};
};

void DrawEvidenceRow(HDC dc, const RECT& r, const EvidenceRowConfig& config, const UiFonts& fonts, int dpi, bool alternateBg = false);

// ============================================================
// C09 — Data Table
// ============================================================
struct TableColumn {
    std::wstring header;
    int widthPx{120};
    bool isMonospace{false};
    bool isStatusBadge{false};
};

struct TableRow {
    std::vector<std::wstring> cells;
    CanonicalUiState rowState{CanonicalUiState::NotTested};
};

struct DataTableConfig {
    std::vector<TableColumn> columns;
    std::vector<TableRow> rows;
    std::wstring emptyMessage{L"Không có bản ghi dữ liệu."};
};

void DrawDataTable(HDC dc, const RECT& r, const DataTableConfig& config, const UiFonts& fonts, int dpi, int scrollRowOffset = 0);

// ============================================================
// C10 — Next Action Panel
// ============================================================
struct NextActionConfig {
    std::wstring actionTitle;
    std::wstring reasonText;
    std::vector<std::wstring> remainingTasks;
    std::wstring buttonText;
    bool isButtonEnabled{true};
    RECT outButtonRect{};
};

void DrawNextActionPanel(HDC dc, const RECT& r, const NextActionConfig& config, const UiFonts& fonts, int dpi);

// ============================================================
// C11 — Dialog Confirmation Pattern
// ============================================================
struct DialogConfig {
    std::wstring title;
    std::wstring message;
    std::wstring consequenceWarning;
    std::wstring confirmButtonText{L"Xác nhận"};
    std::wstring cancelButtonText{L"Hủy bỏ"};
    bool isDestructive{false};
};

void DrawDialogBox(HDC dc, const RECT& dialogRect, const DialogConfig& config, const UiFonts& fonts, int dpi);

// ============================================================
// C12 — Empty / Error / Unsupported State
// ============================================================
struct EmptyStateConfig {
    CanonicalUiState state{CanonicalUiState::Empty};
    std::wstring title;
    std::wstring description;
    std::wstring recoveryHint;
    std::wstring actionButtonText;
};

void DrawEmptyState(HDC dc, const RECT& r, const EmptyStateConfig& config, const UiFonts& fonts, int dpi);

} // namespace lap
