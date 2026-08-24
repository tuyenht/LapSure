from pathlib import Path

R = Path(__file__).resolve().parents[1]


def replace_once(rel, old, new):
    p = R / rel
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{rel}: expected exactly one match, found {count}")
    p.write_text(text.replace(old, new), encoding="utf-8")


def replace_between(rel, start, end, new_block):
    p = R / rel
    text = p.read_text(encoding="utf-8")
    a = text.find(start)
    b = text.find(end, a + len(start))
    if a < 0 or b < 0:
        raise SystemExit(f"{rel}: markers not found: {start!r} -> {end!r}")
    p.write_text(text[:a] + new_block + "\n\n" + text[b:], encoding="utf-8")

# Public hit-test geometry API: renderer and WndProc share the exact same primary-action rectangle.
replace_once(
    "include/lap/ui_screens.h",
    '''// S18 — Đánh giá cuối cùng & Báo cáo
void RenderScreenS18_FinalReport(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                 int focusIndex = 0);

// S19 — Xuất báo cáo & Chia sẻ
void RenderScreenS19_ExportShare(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                 int selectedFormat, int shareFlags, int focusIndex = 0);
''',
    '''// S18 — Đánh giá cuối cùng & Báo cáo
RECT GetScreenS18PrimaryActionRect(const RECT& r, int dpi);
void RenderScreenS18_FinalReport(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                 int focusIndex = 0);

// S19 — Xuất báo cáo & Chia sẻ
RECT GetScreenS19PrimaryActionRect(const RECT& r, int dpi);
void RenderScreenS19_ExportShare(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                 int selectedFormat, int shareFlags, int focusIndex = 0);
''',
)

# S19 reads only real persisted history artifacts; no fake availability state.
replace_once(
    "src/ui_screens_s16_s21_v2.cpp",
    '''#include "lap/ui_screens.h"\n#include "lap/scoring.h"\n#include <algorithm>\n#include <string>\n#include <vector>\n''',
    '''#include "lap/ui_screens.h"\n#include "lap/scoring.h"\n#include "lap/session_history.h"\n#include <algorithm>\n#include <string>\n#include <vector>\n''',
)

new_s18 = r'''static RECT ScreenS18ActionPanelRect(const RECT& r, int dpi) {
    const RECT body = ContentBody(r, dpi);
    const int gap = UiMetrics::Scale(10, dpi);
    const int kpiH = UiMetrics::Scale(92, dpi);
    const int tableTop = body.top + kpiH + UiMetrics::Scale(34, dpi);
    const int sideW = UiMetrics::Scale(320, dpi);
    const int actionH = UiMetrics::Scale(188, dpi);
    return RECT{body.right - sideW, body.bottom - actionH, body.right, body.bottom};
}

static RECT ScreenS19ActionPanelRect(const RECT& r, int dpi) {
    const RECT body = ContentBody(r, dpi);
    const int gap = UiMetrics::Scale(12, dpi);
    const int cardH = UiMetrics::Scale(110, dpi);
    const int railW = UiMetrics::Scale(310, dpi);
    return RECT{body.right - railW, body.top + cardH + gap, body.right, body.bottom};
}

RECT GetScreenS18PrimaryActionRect(const RECT& r, int dpi) {
    return GetNextActionButtonRect(ScreenS18ActionPanelRect(r, dpi), dpi);
}

RECT GetScreenS19PrimaryActionRect(const RECT& r, int dpi) {
    return GetNextActionButtonRect(ScreenS19ActionPanelRect(r, dpi), dpi);
}

void RenderScreenS18_FinalReport(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                 int focusIndex) {
    (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Đánh giá cuối cùng & Báo cáo";
    hdr.subtitle = L"Kết luận dựa trên coverage bắt buộc và bằng chứng hiện có; thiếu dữ liệu bắt buộc sẽ giữ trạng thái chưa đủ kết luận.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    const auto& decision = rep.hardware.stress.decision;
    const auto coverage = BuildCoverageContract(rep);
    int requiredTotal = 0, requiredComplete = 0, missingRequired = 0;
    for (const auto& domain : coverage) {
        if (!domain.required) continue;
        ++requiredTotal;
        if (domain.status == L"COMPLETE" || domain.status == L"PASS") ++requiredComplete;
        else ++missingRequired;
    }

    RECT body = ContentBody(r, dpi);
    const int gap = UiMetrics::Scale(10, dpi);
    const int kpiH = UiMetrics::Scale(92, dpi);
    const int kpiW = (ContentMainWidth(r, dpi) - gap * 4) / 5;
    int x = body.left;
    const std::wstring coverageValue = std::to_wstring(requiredComplete) + L" / " + std::to_wstring(requiredTotal);
    const MetricCardConfig cards[5] = {
        {L"KẾT LUẬN", FormatDecisionVi(decision), L"Decision engine", L"Không suy diễn ngoài bằng chứng", DecisionState(decision.overall), true},
        {L"BẰNG CHỨNG BẮT BUỘC", coverageValue, L"domain hoàn thành", missingRequired ? L"Còn thiếu bằng chứng bắt buộc" : L"Đã đủ coverage bắt buộc", missingRequired ? CanonicalUiState::Incomplete : CanonicalUiState::Info, true},
        {L"ĐỘ TIN CẬY", ConfidenceVi(decision.confidence), L"AuditDecision", L"Không phải điểm sức khỏe", CanonicalUiState::Info, false},
        {L"CẢNH BÁO", std::to_wstring(decision.warnings), L"finding", L"Cần đọc chi tiết trước khi mua", decision.warnings ? CanonicalUiState::Warning : CanonicalUiState::Info, false},
        {L"LỖI NGHIÊM TRỌNG", std::to_wstring(decision.criticalFails), L"critical", L"Không được ẩn bởi điểm tổng hợp", decision.criticalFails ? CanonicalUiState::Fail : CanonicalUiState::Info, false}
    };
    for (const auto& card : cards) {
        RECT cr{x, body.top, x + kpiW, body.top + kpiH};
        DrawMetricCard(dc, cr, card, fonts, dpi);
        x += kpiW + gap;
    }

    const int tableTop = body.top + kpiH + UiMetrics::Scale(34, dpi);
    DrawSectionTitle(dc, body.left, tableTop - UiMetrics::Scale(24, dpi), L"Coverage bắt buộc và bằng chứng còn thiếu", fonts);
    DataTableConfig table;
    table.columns = {
        {L"Lĩnh vực", 190, false, false},
        {L"Bắt buộc", 85, false, false},
        {L"Trạng thái coverage", 145, false, true},
        {L"Nguồn", 220, true, false},
        {L"Còn thiếu", 320, false, false}
    };
    for (const auto& domain : coverage) {
        TableRow row;
        row.cells = {domain.name, domain.required ? L"Có" : L"Không", NonEmpty(domain.status), NonEmpty(domain.sources, L"Chưa có nguồn"), NonEmpty(domain.missingEvidence, L"—")};
        row.rowState = CoverageState(domain.status);
        table.rows.push_back(std::move(row));
    }

    const RECT actionPanel = ScreenS18ActionPanelRect(r, dpi);
    const int sideW = actionPanel.right - actionPanel.left;
    RECT tableRect{body.left, tableTop, body.right - sideW - gap, body.bottom};
    DrawDataTable(dc, tableRect, table, fonts, dpi, 0);

    RECT reasonCard{tableRect.right + gap, tableTop, body.right, actionPanel.top - gap};
    DrawRoundedCard(dc, reasonCard, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    DrawSectionTitle(dc, reasonCard.left + 12, reasonCard.top + 12, L"Vì sao có kết luận này?", fonts);
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMain);
    int y = reasonCard.top + UiMetrics::Scale(38, dpi);
    if (decision.reasons.empty()) {
        std::wstring text = L"Decision engine chưa cung cấp lý do chi tiết. Không tự sinh lý do thay thế.";
        RECT tr{reasonCard.left + 12, y, reasonCard.right - 12, reasonCard.bottom - 12};
        DrawTextW(dc, text.c_str(), -1, &tr, DT_LEFT | DT_WORDBREAK);
    } else {
        for (const auto& reason : decision.reasons) {
            RECT tr{reasonCard.left + 12, y, reasonCard.right - 12, std::min<LONG>(reasonCard.bottom - 8, static_cast<LONG>(y + UiMetrics::Scale(44, dpi)))};
            std::wstring line = L"• " + reason;
            DrawTextW(dc, line.c_str(), -1, &tr, DT_LEFT | DT_WORDBREAK);
            y += UiMetrics::Scale(46, dpi);
            if (y >= reasonCard.bottom - UiMetrics::Scale(20, dpi)) break;
        }
    }

    NextActionConfig action;
    action.actionTitle = L"Xuất báo cáo";
    action.reasonText = L"Chuyển sang màn xuất để kiểm tra artifact HTML/JSON thật của phiên hiện tại. Xuất báo cáo không thay đổi kết luận chẩn đoán.";
    action.remainingTasks = {
        missingRequired ? L"Báo cáo sẽ giữ trạng thái INCOMPLETE nếu còn thiếu evidence bắt buộc" : L"Coverage bắt buộc đã đầy đủ",
        decision.warnings ? L"Đọc các cảnh báo trước khi chia sẻ báo cáo" : L"Không có cảnh báo từ decision engine",
        L"Chỉ HTML/JSON được công bố khi backend đã tạo artifact thật"
    };
    action.buttonText = L"XUẤT BÁO CÁO";
    action.isButtonEnabled = true;
    DrawNextActionPanel(dc, actionPanel, action, fonts, dpi);
}'''

replace_between(
    "src/ui_screens_s16_s21_v2.cpp",
    "void RenderScreenS18_FinalReport",
    "void RenderScreenS19_ExportShare",
    new_s18,
)

new_s19 = r'''void RenderScreenS19_ExportShare(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                 int selectedFormat, int shareFlags, int focusIndex) {
    (void)selectedFormat; (void)shareFlags; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Xuất báo cáo & Chia sẻ";
    hdr.subtitle = L"Chỉ công bố artifact backend đã tạo thật; PDF, ký số và cloud sharing không được giả lập.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    std::wstring htmlPath, jsonPath;
    if (!rep.hardware.stress.sessionId.empty()) {
        const auto history = GetSessionHistorySnapshot();
        const auto it = std::find_if(history.begin(), history.end(), [&](const SessionHistoryEntry& e) {
            return e.sessionId == rep.hardware.stress.sessionId;
        });
        if (it != history.end()) {
            htmlPath = it->htmlPath;
            jsonPath = it->jsonPath;
        }
    }
    const bool htmlReady = !htmlPath.empty() && IsTrustedSessionArtifactPath(htmlPath);
    const bool jsonReady = !jsonPath.empty() && IsTrustedSessionArtifactPath(jsonPath);

    RECT body = ContentBody(r, dpi);
    const int gap = UiMetrics::Scale(12, dpi);
    const int cardW = (body.right - body.left - gap * 2) / 3;
    MetricCardConfig html{L"HTML", htmlReady ? L"ĐÃ TẠO" : L"CHƯA TẠO", L"SaveHtmlReport", htmlReady ? L"Artifact tin cậy của phiên hiện tại" : L"Backend hỗ trợ nhưng chưa có artifact", htmlReady ? CanonicalUiState::Info : CanonicalUiState::NotTested, true};
    MetricCardConfig json{L"JSON", jsonReady ? L"ĐÃ TẠO" : L"CHƯA TẠO", L"SaveJsonReport", jsonReady ? L"Artifact tin cậy của phiên hiện tại" : L"Backend hỗ trợ nhưng chưa có artifact", jsonReady ? CanonicalUiState::Info : CanonicalUiState::NotTested, true};
    MetricCardConfig pdf{L"PDF / Ký số", L"CHƯA HỖ TRỢ", L"Không có backend", L"Không hiển thị như tính năng hoạt động", CanonicalUiState::Unsupported, true};
    RECT a{body.left, body.top, body.left + cardW, body.top + UiMetrics::Scale(110, dpi)};
    RECT b{a.right + gap, body.top, a.right + gap + cardW, a.bottom};
    RECT c{b.right + gap, body.top, body.right, a.bottom};
    DrawMetricCard(dc, a, html, fonts, dpi);
    DrawMetricCard(dc, b, json, fonts, dpi);
    DrawMetricCard(dc, c, pdf, fonts, dpi);

    const RECT actionPanel = ScreenS19ActionPanelRect(r, dpi);
    RECT info{body.left, a.bottom + gap, actionPanel.left - gap, body.bottom};
    DrawRoundedCard(dc, info, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    DrawSectionTitle(dc, info.left + 14, info.top + 14, L"Trạng thái báo cáo hiện tại", fonts);
    SelectObject(dc, fonts.hBody);
    SetTextColor(dc, UiColors::TextMain);
    std::wstring decisionLine = L"Kết luận hiện tại: " + FormatDecisionVi(rep.hardware.stress.decision);
    TextOutW(dc, info.left + 14, info.top + 44, decisionLine.c_str(), static_cast<int>(decisionLine.size()));
    std::wstring modelLine = L"Thiết bị: " + NonEmpty(rep.model, L"Chưa xác định") + L"  |  Service Tag: " + NonEmpty(rep.serviceTag, L"Chưa đọc được");
    TextOutW(dc, info.left + 14, info.top + 70, modelLine.c_str(), static_cast<int>(modelLine.size()));
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    const std::wstring sessionLine = L"Session ID: " + NonEmpty(rep.hardware.stress.sessionId, L"Chưa có session ID") +
        L"\nHTML: " + (htmlReady ? htmlPath : L"Chưa có artifact HTML tin cậy") +
        L"\nJSON: " + (jsonReady ? jsonPath : L"Chưa có artifact JSON tin cậy");
    RECT nr{info.left + 14, info.top + 102, info.right - 14, info.bottom - 14};
    DrawTextW(dc, sessionLine.c_str(), -1, &nr, DT_LEFT | DT_WORDBREAK);

    NextActionConfig action;
    action.actionTitle = L"Mở báo cáo HTML";
    action.reasonText = htmlReady
        ? L"Artifact HTML đã tồn tại trong thư mục history tin cậy và có thể mở bằng trình duyệt mặc định."
        : L"Chưa có HTML artifact tin cậy cho session hiện tại. LapSure không tạo nút mở giả.";
    action.remainingTasks = {
        htmlReady ? L"HTML: đã persist" : L"HTML: chưa persist",
        jsonReady ? L"JSON: đã persist" : L"JSON: chưa persist",
        L"PDF / ký số / cloud: không hỗ trợ trong build hiện tại"
    };
    action.buttonText = htmlReady ? L"MỞ BÁO CÁO" : L"CHƯA CÓ HTML";
    action.isButtonEnabled = htmlReady;
    DrawNextActionPanel(dc, actionPanel, action, fonts, dpi);
}'''

replace_between(
    "src/ui_screens_s16_s21_v2.cpp",
    "void RenderScreenS19_ExportShare",
    "void RenderScreenS20_LogsEvents",
    new_s19,
)

# Replace invisible legacy hotspots with geometry exported by the actual renderers.
replace_between(
    "src/main.cpp",
    "        // 12. Reports Screen: Export Button Hit-Test",
    "        // 14–16. S12/S13/S14 primary actions: use the same C10 geometry as rendering.",
    r'''        // 12. S18 Final Report: renderer and hit-test share one primary-action rectangle.
        if (gCurrentTab == MainTab::Reports) {
            const RECT actionButton = GetScreenS18PrimaryActionRect(layout.contentRect, dpi);
            if (x >= actionButton.left && x <= actionButton.right && y >= actionButton.top && y <= actionButton.bottom) {
                gCurrentTab = MainTab::ExportShare;
                InvalidateRect(h, nullptr, FALSE);
                return 0;
            }
        }

        // 13. S19 Export: open only the real trusted HTML artifact of the current session.
        if (gCurrentTab == MainTab::ExportShare) {
            const RECT actionButton = GetScreenS19PrimaryActionRect(layout.contentRect, dpi);
            if (x >= actionButton.left && x <= actionButton.right && y >= actionButton.top && y <= actionButton.bottom) {
                if (!gReportPath.empty() && IsTrustedSessionArtifactPath(gReportPath)) {
                    ShellExecuteW(h, L"open", gReportPath.c_str(), nullptr, nullptr, SW_SHOW);
                } else {
                    MessageBoxW(h, L"Phiên hiện tại chưa có HTML report tin cậy để mở.", L"LapSure", MB_OK | MB_ICONINFORMATION);
                }
                return 0;
            }
        }''',
)

# Explicit cancellation after stress but before persistence must not be misclassified as a crash on next launch.
replace_once(
    "src/main.cpp",
    '''        if (persisted.Complete()) CompleteStressJournal(gDir);
    }
    {
''',
    '''        if (persisted.Complete()) CompleteStressJournal(gDir);
    } else if (!report.hardware.stress.sessionId.empty()) {
        DiscardInterruptedStressJournal(gDir);
    }
    {
''',
)

# Permanent regression gate for visible/actionable S18/S19 behavior.
test = R / "tests" / "production_hardening_round3_interactions_sanity.py"
test.write_text(r'''from pathlib import Path

R = Path(__file__).resolve().parents[1]
h = (R / "include/lap/ui_screens.h").read_text(encoding="utf-8")
ui = (R / "src/ui_screens_s16_s21_v2.cpp").read_text(encoding="utf-8")
main = (R / "src/main.cpp").read_text(encoding="utf-8")

checks = [
    ("S18 primary action geometry exported", "GetScreenS18PrimaryActionRect" in h and "GetScreenS18PrimaryActionRect" in ui),
    ("S19 primary action geometry exported", "GetScreenS19PrimaryActionRect" in h and "GetScreenS19PrimaryActionRect" in ui),
    ("S18 visibly renders export CTA", 'action.buttonText = L"XUẤT BÁO CÁO"' in ui and "DrawNextActionPanel(dc, actionPanel, action" in ui),
    ("S18 click navigates to export screen", "GetScreenS18PrimaryActionRect(layout.contentRect, dpi)" in main and "gCurrentTab = MainTab::ExportShare" in main),
    ("S19 availability comes from persisted history", "GetSessionHistorySnapshot()" in ui and "IsTrustedSessionArtifactPath(htmlPath)" in ui and "IsTrustedSessionArtifactPath(jsonPath)" in ui),
    ("S19 does not fake unavailable formats", 'L"PDF / Ký số", L"CHƯA HỖ TRỢ"' in ui and 'action.isButtonEnabled = htmlReady' in ui),
    ("S19 click uses renderer geometry", "GetScreenS19PrimaryActionRect(layout.contentRect, dpi)" in main),
    ("S19 open is trust-gated", "IsTrustedSessionArtifactPath(gReportPath)" in main),
    ("no legacy invisible S18/S19 action-card hotspots", "Reports Screen: Export Button Hit-Test" not in main and "Export & Share Screen: Open in Browser Button Hit-Test" not in main),
    ("explicit post-stress cancel discards journal", "else if (!report.hardware.stress.sessionId.empty())" in main and "DiscardInterruptedStressJournal(gDir)" in main),
]

bad = []
for name, ok in checks:
    print(("PASS" if ok else "FAIL"), name)
    if not ok:
        bad.append(name)
print(f"{len(checks)-len(bad)}/{len(checks)} PASS")
raise SystemExit(bool(bad))
''', encoding="utf-8")

p = R / "run_source_tests.cmd"
text = p.read_text(encoding="utf-8")
needle = "python tests\\production_hardening_round2_sanity.py\nif errorlevel 1 exit /b 1\n"
if needle not in text:
    raise SystemExit("run_source_tests.cmd: Round 2 gate anchor missing")
if "production_hardening_round3_interactions_sanity.py" not in text:
    text = text.replace(needle, needle + "python tests\\production_hardening_round3_interactions_sanity.py\nif errorlevel 1 exit /b 1\n")
p.write_text(text, encoding="utf-8")

print("Production hardening Round 3 applied")
