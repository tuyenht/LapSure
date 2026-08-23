from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
ui_theme_h = (ROOT / "include/lap/ui_theme.h").read_text(encoding="utf-8")
ui_state_h = (ROOT / "include/lap/ui_state.h").read_text(encoding="utf-8")
ui_state_cpp = (ROOT / "src/ui_state.cpp").read_text(encoding="utf-8")
ui_comp_h = (ROOT / "include/lap/ui_components.h").read_text(encoding="utf-8")
ui_comp_cpp = (ROOT / "src/ui_components.cpp").read_text(encoding="utf-8")
main_cpp = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
cm = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

checks = [
    # 1. Canonical State Model
    ("CanonicalUiState enum exists with 20 states", all(s in ui_state_h for s in [
        "Idle", "Ready", "Locked", "Running", "Paused", "Pass", "Good", "Warning", "Fail",
        "Changed", "Incomplete", "NotTested", "Unsupported", "ManualRequired",
        "ProviderUnavailable", "PermissionDenied", "Cancelled", "Interrupted", "Empty", "Error"
    ])),
    ("StatePresentation structure with allowCleanPass", "struct StatePresentation" in ui_state_h and "bool allowCleanPass" in ui_state_h),
    ("Canonical state mapping functions declared", all(fn in ui_state_h for fn in [
        "GetStatePresentation", "MapState", "MapFunctionalStatus", "MapTestStageState", "FormatDecisionVi"
    ])),
    ("Canonical Vietnamese status copy matches UI_STATE_MODEL", all(txt in ui_state_cpp for txt in [
        'L"Chưa bắt đầu"', 'L"Sẵn sàng"', 'L"Chưa thể thực hiện"', 'L"Đang kiểm tra"', 'L"Đã tạm dừng"',
        'L"Đạt"', 'L"Tốt"', 'L"Cần lưu ý"', 'L"Không đạt"', 'L"Có thay đổi"', 'L"Chưa đủ dữ liệu"',
        'L"Chưa kiểm tra"', 'L"Không hỗ trợ"', 'L"Cần xác nhận"', 'L"Không có nguồn dữ liệu"',
        'L"Thiếu quyền truy cập"', 'L"Đã hủy"', 'L"Bị gián đoạn"', 'L"Chưa có dữ liệu"', 'L"Có lỗi khi kiểm tra"'
    ])),
    ("Decision labels match Vietnamese UX spec", all(d in ui_state_cpp for d in [
        'L"CÓ THỂ MUA"', 'L"CÓ THỂ MUA — CẦN LƯU Ý"', 'L"KHÔNG NÊN MUA"', 'L"CHƯA ĐỦ DỮ LIỆU ĐỂ KẾT LUẬN"'
    ])),

    # 2. Design Tokens & DPI
    ("Design tokens centralized in UiColors", all(c in ui_theme_h for c in [
        "SidebarBg", "SidebarHover", "SidebarActive", "ContentBg", "CardBg", "CardBorder",
        "PrimaryBlue", "SuccessGreen", "WarnAmber", "FailRed", "InfoBlue", "FocusOutline"
    ])),
    ("UiMetrics defines 4px base spacing scale and DPI scale helper", all(m in ui_theme_h for m in [
        "Spacing4", "Spacing8", "Spacing12", "Spacing16", "Spacing24", "Spacing32", "Spacing48",
        "RadiusSm", "RadiusMd", "RadiusLg", "RadiusPill", "Scale(int val, int dpi)"
    ])),
    ("UiFonts supports DPI-aware initialization", "void Init(int dpi = 96)" in ui_theme_h or "void Init(int dpi)" in ui_theme_h),
    ("GetDpiForHwnd implemented with Win32 fallback", "GetDpiForHwnd" in ui_theme_h and "GetDpiForWindow" in (ROOT / "src/ui_renderer.cpp").read_text(encoding="utf-8")),

    # 3. C01-C12 Reusable Components
    ("C01 App Shell declared and implemented", "ComputeAppShellLayout" in ui_comp_h and "DrawAppShellBackground" in ui_comp_cpp and "DrawAppShellFooter" in ui_comp_cpp),
    ("C02 Sidebar & Grouped Navigation implemented", "GetDefaultSidebarGroups" in ui_comp_h and "DrawSidebar" in ui_comp_cpp and "HitTestSidebar" in ui_comp_cpp),
    ("Sidebar defines 4 canonical groups", all(g in ui_comp_cpp for g in [
        'L"QUY TRÌNH"', 'L"CHI TIẾT THIẾT BỊ"', 'L"ĐÁNH GIÁ & HỒ SƠ"', 'L"HỆ THỐNG"'
    ])),
    ("C03 Page Header implemented", "PageHeaderConfig" in ui_comp_h and "DrawPageHeader" in ui_comp_cpp),
    ("C04 Status Badge implemented with text+icon", "DrawStatusBadge" in ui_comp_h and "displayLabel" in ui_comp_cpp),
    ("C05 Metric Card implemented", "MetricCardConfig" in ui_comp_h and "DrawMetricCard" in ui_comp_cpp),
    ("C06 Progress & Coverage distinguishes progress from evidence", "ProgressCoverageConfig" in ui_comp_h and "isEvidenceCoverage" in ui_comp_h and "DrawProgressCoverage" in ui_comp_cpp),
    ("C07 Guided Stepper implemented", "StepperStep" in ui_comp_h and "DrawGuidedStepper" in ui_comp_cpp),
    ("C08 Evidence Row implemented", "EvidenceRowConfig" in ui_comp_h and "DrawEvidenceRow" in ui_comp_cpp),
    ("C09 Data Table implemented with monospace and status cells", "DataTableConfig" in ui_comp_h and "DrawDataTable" in ui_comp_cpp),
    ("C10 Next Action Panel implemented", "NextActionConfig" in ui_comp_h and "DrawNextActionPanel" in ui_comp_cpp),
    ("C11 Dialog Confirmation pattern implemented", "DialogConfig" in ui_comp_h and "DrawDialogBox" in ui_comp_cpp),
    ("C12 Empty/Error State pattern implemented", "EmptyStateConfig" in ui_comp_h and "DrawEmptyState" in ui_comp_cpp),

    # 4. App Integration & Data Boundary
    ("Main window handles WM_DPICHANGED", "case WM_DPICHANGED:" in main_cpp),
    ("Main window computes App Shell Layout", "ComputeAppShellLayout" in main_cpp),
    ("Main window uses grouped sidebar hit testing", "HitTestSidebar" in main_cpp),
    ("CMakeLists includes ui_state.cpp and ui_components.cpp", "src/ui_state.cpp" in cm and "src/ui_components.cpp" in cm),
    ("No hardcoded fake health score 88/100 in main.cpp", "DrawCircularScoreGauge(dc, (gaugeCard.left + gaugeCard.right) / 2, gaugeCard.top + 95, 45, 88" not in main_cpp),
    ("No hardcoded fake factory match 95% in main.cpp", 'L"Khớp cấu hình nhà máy:  95%"' not in main_cpp),
]

bad = []
for n, ok in checks:
    print(("PASS " if ok else "FAIL ") + n)
    if not ok:
        bad.append(n)

if bad:
    print(f"\n{len(bad)} UI sanity checks failed.", file=sys.stderr)
    raise SystemExit(1)

print(f"\nAll {len(checks)} Phase A UI sanity checks passed successfully.")
