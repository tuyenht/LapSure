#include "lap/ui_screens.h"
#include "lap/session_history.h"
#include <algorithm>
#include <string>

namespace lap {
namespace {
std::wstring Nz(const std::wstring& s, const wchar_t* fallback = L"—") { return s.empty() ? std::wstring(fallback) : s; }

CanonicalUiState HistoryState(const SessionHistoryEntry& e) {
    if (e.status == L"INTERRUPTED") return CanonicalUiState::Interrupted;
    if (e.status == L"INCOMPLETE" || e.verdict == L"INCOMPLETE") return CanonicalUiState::Incomplete;
    if (e.verdict == L"REJECT") return CanonicalUiState::Fail;
    if (e.verdict == L"BUY WITH NOTES") return CanonicalUiState::Warning;
    if (e.verdict == L"BUY") return CanonicalUiState::Pass;
    return CanonicalUiState::Info;
}

std::wstring VerdictVi(const std::wstring& v) {
    if (v == L"BUY") return L"CÓ THỂ MUA";
    if (v == L"BUY WITH NOTES") return L"CÓ THỂ MUA — CẦN LƯU Ý";
    if (v == L"REJECT") return L"KHÔNG NÊN MUA";
    return L"CHƯA ĐỦ DỮ LIỆU ĐỂ KẾT LUẬN";
}

void Text(HDC dc, int x, int y, const std::wstring& s, HFONT font, COLORREF color) {
    SelectObject(dc, font); SetTextColor(dc, color); SetBkMode(dc, TRANSPARENT);
    TextOutW(dc, x, y, s.c_str(), static_cast<int>(s.size()));
}
}

void RenderScreenS22_SessionHistory(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                    int tableScrollOffset, int selectedIndex, int focusIndex) {
    (void)rep; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Lịch sử phiên kiểm định";
    hdr.subtitle = L"Chỉ đọc index phiên/report cục bộ đã được LapSure ghi thật; không có lịch sử cloud hoặc dữ liệu mẫu.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    const auto history = GetSessionHistorySnapshot();
    RECT body{r.left + UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi)};
    if (history.empty()) {
        EmptyStateConfig empty;
        empty.state = CanonicalUiState::Empty;
        empty.title = L"Chưa có phiên đã lưu";
        empty.description = L"Lịch sử sẽ xuất hiện sau khi LapSure tạo report HTML/JSON hoặc lưu một journal gián đoạn.";
        empty.recoveryHint = L"Không tạo phiên mẫu để lấp chỗ trống.";
        DrawEmptyState(dc, body, empty, fonts, dpi);
        return;
    }

    const int rightW = UiMetrics::Scale(300, dpi);
    const int gap = UiMetrics::Scale(12, dpi);
    RECT tableRect{body.left, body.top, body.right - rightW - gap, body.bottom};
    DataTableConfig table;
    table.columns = {
        {L"Thời gian", 150, true, false},
        {L"Model", 175, false, false},
        {L"Service Tag", 120, true, false},
        {L"Kết luận", 190, false, false},
        {L"Trạng thái", 120, false, true}
    };
    for (const auto& e : history) {
        TableRow row;
        row.cells = {Nz(e.timestamp), Nz(e.model, L"Chưa xác định"), Nz(e.serviceTag, L"Chưa đọc được"), VerdictVi(e.verdict), Nz(e.status, L"INCOMPLETE")};
        row.rowState = HistoryState(e);
        table.rows.push_back(std::move(row));
    }
    DrawDataTable(dc, tableRect, table, fonts, dpi, tableScrollOffset);

    const int idx = std::clamp(selectedIndex, 0, static_cast<int>(history.size()) - 1);
    const auto& selected = history[static_cast<size_t>(idx)];
    RECT detail{tableRect.right + gap, body.top, body.right, body.bottom};
    DrawRoundedCard(dc, detail, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    Text(dc, detail.left + 14, detail.top + 14, L"Phiên đang chọn", fonts.hBodyBold, UiColors::TextMain);
    DrawStatusBadge(dc, detail.left + 14, detail.top + UiMetrics::Scale(40, dpi), UiMetrics::Scale(190, dpi), UiMetrics::Scale(22, dpi), HistoryState(selected), fonts, VerdictVi(selected.verdict));
    Text(dc, detail.left + 14, detail.top + UiMetrics::Scale(76, dpi), L"ID: " + Nz(selected.sessionId), fonts.hSmall, UiColors::TextMuted);
    Text(dc, detail.left + 14, detail.top + UiMetrics::Scale(98, dpi), L"HTML: " + (selected.htmlPath.empty() ? L"Không có" : L"Có"), fonts.hSmall, UiColors::TextMain);
    Text(dc, detail.left + 14, detail.top + UiMetrics::Scale(118, dpi), L"JSON: " + (selected.jsonPath.empty() ? L"Không có" : L"Có"), fonts.hSmall, UiColors::TextMain);
    Text(dc, detail.left + 14, detail.top + UiMetrics::Scale(138, dpi), L"Journal evidence: " + (selected.evidencePath.empty() ? L"Không có" : L"Có"), fonts.hSmall, UiColors::TextMain);

    const int btnH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
    RECT openBtn{detail.left + 14, detail.bottom - btnH * 2 - UiMetrics::Scale(24, dpi), detail.right - 14, detail.bottom - btnH - UiMetrics::Scale(18, dpi)};
    RECT deleteBtn{detail.left + 14, detail.bottom - btnH - UiMetrics::Scale(10, dpi), detail.right - 14, detail.bottom - UiMetrics::Scale(10, dpi)};
    DrawRoundedCard(dc, openBtn, UiMetrics::RadiusSm, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
    DrawTextW(dc, L"MỞ REPORT / BẰNG CHỨNG", -1, &openBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SetTextColor(dc, UiColors::FailRed); SelectObject(dc, fonts.hBodyBold);
    DrawRoundedCard(dc, deleteBtn, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::FailRed, 1);
    DrawTextW(dc, L"XÓA PHIÊN...", -1, &deleteBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void RenderScreenS23_InterruptedRecovery(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                         int focusIndex) {
    (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Khôi phục phiên bị gián đoạn";
    hdr.subtitle = L"Crash/reboot/interruption là bằng chứng gián đoạn, không phải bằng chứng phần cứng hỏng và tuyệt đối không tạo PASS.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    RECT body{r.left + UiMetrics::Scale(24, dpi), r.top + UiMetrics::Scale(70, dpi), r.right - UiMetrics::Scale(24, dpi), r.bottom - UiMetrics::Scale(20, dpi)};
    if (!rep.hardware.stress.previousInterruptedSessionDetected) {
        EmptyStateConfig empty;
        empty.state = CanonicalUiState::Empty;
        empty.title = L"Không có phiên bị gián đoạn cần xử lý";
        empty.description = L"LapSure không phát hiện journal RUNNING còn sót lại từ phiên trước.";
        empty.recoveryHint = L"Khi có crash/reboot giữa stress test, journal sẽ được giữ để xử lý ở đây.";
        DrawEmptyState(dc, body, empty, fonts, dpi);
        return;
    }

    const int rightW = UiMetrics::Scale(330, dpi);
    const int gap = UiMetrics::Scale(14, dpi);
    RECT evidence{body.left, body.top, body.right - rightW - gap, body.bottom};
    DrawRoundedCard(dc, evidence, UiMetrics::RadiusMd, UiColors::WarnBg, UiColors::WarnBorder, 1);
    Text(dc, evidence.left + 16, evidence.top + 14, L"PHIÊN BỊ GIÁN ĐOẠN", fonts.hBodyBold, UiColors::WarnAmber);
    Text(dc, evidence.left + 16, evidence.top + UiMetrics::Scale(46, dpi), L"Journal: " + Nz(rep.hardware.stress.journalPath, L"Đường dẫn chưa xác định"), fonts.hSmall, UiColors::TextMain);

    std::wstring journalEvidence;
    for (const auto& f : rep.findings) {
        if (f.name == L"Previous interrupted stress session") { journalEvidence = f.value.empty() ? f.evidence : f.value; break; }
    }
    RECT raw{evidence.left + 16, evidence.top + UiMetrics::Scale(78, dpi), evidence.right - 16, evidence.bottom - 16};
    SelectObject(dc, fonts.hMono); SetTextColor(dc, UiColors::TextMain); SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, journalEvidence.empty() ? L"Journal được phát hiện nhưng chưa có nội dung parse trong report snapshot." : journalEvidence.c_str(), -1, &raw, DT_LEFT | DT_WORDBREAK);

    RECT actions{evidence.right + gap, body.top, body.right, body.bottom};
    DrawRoundedCard(dc, actions, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    Text(dc, actions.left + 14, actions.top + 14, L"Chọn cách xử lý", fonts.hBodyBold, UiColors::TextMain);
    RECT desc{actions.left + 14, actions.top + UiMetrics::Scale(42, dpi), actions.right - 14, actions.top + UiMetrics::Scale(138, dpi)};
    SelectObject(dc, fonts.hSmall); SetTextColor(dc, UiColors::TextMuted);
    DrawTextW(dc, L"Khôi phục sẽ lưu journal cũ vào lịch sử rồi chạy lại kiểm tra tự động để thu thập bằng chứng mới từ đầu. Đóng INCOMPLETE chỉ lưu journal. Bỏ journal sẽ xóa bằng chứng gián đoạn sau xác nhận.", -1, &desc, DT_LEFT | DT_WORDBREAK);

    const int btnH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
    const int btnGap = UiMetrics::Scale(10, dpi);
    int y = actions.bottom - (btnH * 3 + btnGap * 2 + UiMetrics::Scale(14, dpi));
    RECT recover{actions.left + 14, y, actions.right - 14, y + btnH};
    RECT closeIncomplete{actions.left + 14, recover.bottom + btnGap, actions.right - 14, recover.bottom + btnGap + btnH};
    RECT discard{actions.left + 14, closeIncomplete.bottom + btnGap, actions.right - 14, closeIncomplete.bottom + btnGap + btnH};
    DrawRoundedCard(dc, recover, UiMetrics::RadiusSm, UiColors::PrimaryBlue, UiColors::PrimaryBlue, 1);
    SetTextColor(dc, UiColors::TextWhite); SelectObject(dc, fonts.hBodyBold);
    DrawTextW(dc, L"LƯU JOURNAL & CHẠY LẠI", -1, &recover, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DrawRoundedCard(dc, closeIncomplete, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::WarnAmber, 1);
    SetTextColor(dc, UiColors::WarnAmber);
    DrawTextW(dc, L"ĐÓNG PHIÊN INCOMPLETE", -1, &closeIncomplete, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DrawRoundedCard(dc, discard, UiMetrics::RadiusSm, UiColors::CardBg, UiColors::FailRed, 1);
    SetTextColor(dc, UiColors::FailRed);
    DrawTextW(dc, L"BỎ JOURNAL...", -1, &discard, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

} // namespace lap
