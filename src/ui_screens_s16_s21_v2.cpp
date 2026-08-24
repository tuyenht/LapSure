#include "lap/ui_screens.h"
#include "lap/scoring.h"
#include <algorithm>
#include <string>
#include <vector>

namespace lap {
namespace {

std::wstring NonEmpty(const std::wstring& value, const wchar_t* fallback = L"Không có dữ liệu") {
    return value.empty() ? std::wstring(fallback) : value;
}

std::wstring ConfidenceVi(Confidence c) {
    switch (c) {
    case Confidence::High: return L"Cao";
    case Confidence::Medium: return L"Trung bình";
    default: return L"Thấp";
    }
}

CanonicalUiState DecisionState(const std::wstring& overall) {
    if (overall == L"BUY") return CanonicalUiState::Pass;
    if (overall == L"BUY WITH NOTES") return CanonicalUiState::Warning;
    if (overall == L"REJECT") return CanonicalUiState::Fail;
    return CanonicalUiState::Incomplete;
}

CanonicalUiState CoverageState(const std::wstring& status) {
    if (status == L"COMPLETE" || status == L"PASS") return CanonicalUiState::Info;
    if (status == L"FAIL") return CanonicalUiState::Fail;
    if (status == L"WARNING" || status == L"PARTIAL") return CanonicalUiState::Incomplete;
    if (status == L"UNSUPPORTED") return CanonicalUiState::Unsupported;
    return CanonicalUiState::NotTested;
}

CanonicalUiState TestVerdictState(TestVerdict v) {
    switch (v) {
    case TestVerdict::Pass: return CanonicalUiState::Pass;
    case TestVerdict::Warning: return CanonicalUiState::Warning;
    case TestVerdict::Fail: return CanonicalUiState::Fail;
    case TestVerdict::Cancelled: return CanonicalUiState::Cancelled;
    default: return CanonicalUiState::NotTested;
    }
}

std::wstring TestVerdictVi(TestVerdict v) {
    switch (v) {
    case TestVerdict::Pass: return L"ĐẠT";
    case TestVerdict::Warning: return L"CẦN LƯU Ý";
    case TestVerdict::Fail: return L"KHÔNG ĐẠT";
    case TestVerdict::Cancelled: return L"ĐÃ HỦY";
    default: return L"CHƯA KIỂM TRA";
    }
}

std::wstring FunctionalVi(FunctionalStatus s) {
    switch (s) {
    case FunctionalStatus::Pass: return L"ĐẠT";
    case FunctionalStatus::Warning: return L"CẦN LƯU Ý";
    case FunctionalStatus::Fail: return L"KHÔNG ĐẠT";
    case FunctionalStatus::Unsupported: return L"KHÔNG HỖ TRỢ";
    case FunctionalStatus::ManualRequired: return L"CẦN XÁC NHẬN";
    default: return L"CHƯA KIỂM TRA";
    }
}

int ContentMainWidth(const RECT& r, int dpi) {
    return (r.right - r.left) - UiMetrics::Scale(48, dpi);
}

RECT ContentBody(const RECT& r, int dpi) {
    return RECT{
        r.left + UiMetrics::Scale(24, dpi),
        r.top + UiMetrics::Scale(70, dpi),
        r.right - UiMetrics::Scale(24, dpi),
        r.bottom - UiMetrics::Scale(20, dpi)
    };
}

void DrawSectionTitle(HDC dc, int x, int y, const std::wstring& text, const UiFonts& fonts) {
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    SetBkMode(dc, TRANSPARENT);
    TextOutW(dc, x, y, text.c_str(), static_cast<int>(text.size()));
}

} // namespace

void RenderScreenS16_FactoryCompare(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                    int tableScrollOffset, int focusIndex) {
    (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Hồ sơ & Đối chiếu";
    hdr.subtitle = L"Tách riêng cấu hình thực tế, hồ sơ tham chiếu và cam kết người bán; sai khác không đồng nghĩa lỗi phần cứng.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    RECT body = ContentBody(r, dpi);
    const int heroH = UiMetrics::Scale(92, dpi);
    RECT hero{body.left, body.top, body.right, body.top + heroH};
    DrawRoundedCard(dc, hero, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);

    DrawSectionTitle(dc, hero.left + 14, hero.top + 12, NonEmpty(rep.model, L"Chưa xác định model"), fonts);
    SelectObject(dc, fonts.hSmall);
    SetTextColor(dc, UiColors::TextMuted);
    std::wstring identity = L"Service Tag: " + NonEmpty(rep.serviceTag, L"Chưa đọc được") + L"  |  Nguồn hồ sơ: " + NonEmpty(rep.profileSource, L"Chưa có");
    TextOutW(dc, hero.left + 14, hero.top + 38, identity.c_str(), static_cast<int>(identity.size()));
    std::wstring mode = rep.factoryExact ? L"Có hồ sơ đối chiếu chính xác" : (rep.genericMode ? L"Đang dùng chế độ tham chiếu chung" : L"Chưa xác minh hồ sơ nhà máy chính xác");
    DrawStatusBadge(dc, hero.left + 14, hero.top + 58, UiMetrics::Scale(220, dpi), UiMetrics::Scale(22, dpi),
                    rep.factoryExact ? CanonicalUiState::Info : CanonicalUiState::Incomplete, fonts, mode);

    DataTableConfig table;
    table.columns = {
        {L"Nhóm", 120, false, false},
        {L"Hạng mục", 150, false, false},
        {L"Thực tế", 190, false, false},
        {L"Tham chiếu / khai báo", 210, false, false},
        {L"Bằng chứng", 230, true, false},
        {L"Trạng thái", 105, false, true}
    };
    for (const auto& finding : rep.findings) {
        const bool factory = finding.dimension == Dimension::Factory;
        const bool seller = finding.group == L"Cam kết người bán";
        if (!factory && !seller) continue;
        TableRow row;
        row.cells = {
            seller ? L"Người bán" : L"Hồ sơ",
            NonEmpty(finding.name),
            NonEmpty(finding.value),
            NonEmpty(finding.expected, L"Không có giá trị tham chiếu"),
            NonEmpty(finding.evidence, L"Không có bằng chứng chi tiết"),
            CanonicalStateName(MapState(finding.state))
        };
        row.rowState = MapState(finding.state);
        table.rows.push_back(std::move(row));
    }
    if (rep.sellerClaim.provided && table.rows.empty()) {
        TableRow row;
        row.cells = {L"Người bán", L"Cam kết đã nhập", NonEmpty(rep.sellerClaim.model, L"Không ghi model"), L"Chờ đối chiếu", L"Dữ liệu người bán do người kiểm tra nhập", L"THÔNG TIN"};
        row.rowState = CanonicalUiState::Info;
        table.rows.push_back(std::move(row));
    }
    table.emptyMessage = L"Chưa có bằng chứng đối chiếu. Hãy hoàn tất thu thập cấu hình và/hoặc nhập cam kết người bán.";
    RECT tableRect{body.left, hero.bottom + UiMetrics::Scale(12, dpi), body.right, body.bottom};
    DrawDataTable(dc, tableRect, table, fonts, dpi, tableScrollOffset);
}

void RenderScreenS17_EvidenceLibrary(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                     int activeFilter, int selectedItem, int viewMode, int focusIndex) {
    (void)activeFilter; (void)selectedItem; (void)viewMode; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Thư viện bằng chứng";
    hdr.subtitle = L"Chỉ hiển thị bằng chứng thực đã được thu thập; sự tồn tại của bằng chứng không tự tạo trạng thái ĐẠT.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    DataTableConfig table;
    table.columns = {
        {L"Nguồn", 130, false, false},
        {L"Hạng mục", 180, false, false},
        {L"Chi tiết", 210, false, false},
        {L"Bằng chứng", 330, true, false},
        {L"Trạng thái", 115, false, true}
    };
    for (const auto& f : rep.findings) {
        if (f.evidence.empty()) continue;
        TableRow row;
        row.cells = {NonEmpty(f.group), NonEmpty(f.name), NonEmpty(f.value), f.evidence, CanonicalStateName(MapState(f.state))};
        row.rowState = MapState(f.state);
        table.rows.push_back(std::move(row));
    }
    for (const auto& item : rep.hardware.stress.functional.items) {
        if (item.evidence.empty()) continue;
        TableRow row;
        row.cells = {L"Chức năng", NonEmpty(item.name), NonEmpty(item.detail), item.evidence, FunctionalVi(item.status)};
        row.rowState = MapFunctionalStatus(item.status);
        table.rows.push_back(std::move(row));
    }
    for (const auto& stage : rep.hardware.stress.stages) {
        if (stage.evidence.empty()) continue;
        TableRow row;
        row.cells = {L"Stress", NonEmpty(stage.name), L"Thời gian đo: " + std::to_wstring(stage.elapsedSeconds) + L" giây", stage.evidence, TestVerdictVi(stage.verdict)};
        row.rowState = TestVerdictState(stage.verdict);
        table.rows.push_back(std::move(row));
    }
    for (const auto& p : rep.hardware.stress.portPower.ports) {
        if (p.evidence.empty()) continue;
        TableRow row;
        row.cells = {L"Cổng", NonEmpty(p.portLabel), NonEmpty(p.deviceDescription, L"Không có thiết bị mẫu"), p.evidence, NonEmpty(p.verdict, L"CHƯA KIỂM TRA")};
        row.rowState = CoverageState(p.verdict);
        table.rows.push_back(std::move(row));
    }
    for (const auto& check : rep.hardware.stress.runtimeValidation.checks) {
        if (check.evidence.empty()) continue;
        CanonicalUiState state = CanonicalUiState::NotTested;
        if (check.status == ValidationStatus::Pass) state = CanonicalUiState::Pass;
        else if (check.status == ValidationStatus::Warning) state = CanonicalUiState::Warning;
        else if (check.status == ValidationStatus::Fail) state = CanonicalUiState::Fail;
        TableRow row;
        row.cells = {L"Runtime", NonEmpty(check.name), NonEmpty(check.detail), check.evidence, CanonicalStateName(state)};
        row.rowState = state;
        table.rows.push_back(std::move(row));
    }
    table.emptyMessage = L"Chưa có bằng chứng trong phiên hiện tại. LapSure không tạo ảnh, log hoặc trạng thái mẫu.";
    DrawDataTable(dc, ContentBody(r, dpi), table, fonts, dpi, 0);
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
    const int reasonW = UiMetrics::Scale(300, dpi);
    RECT tableRect{body.left, tableTop, body.right - reasonW - gap, body.bottom};
    DrawDataTable(dc, tableRect, table, fonts, dpi, 0);

    RECT reasonCard{tableRect.right + gap, tableTop, body.right, body.bottom};
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
}

void RenderScreenS19_ExportShare(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                 int selectedFormat, int shareFlags, int focusIndex) {
    (void)selectedFormat; (void)shareFlags; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Xuất báo cáo & Chia sẻ";
    hdr.subtitle = L"Chỉ công bố định dạng backend hiện hỗ trợ thật; PDF/chữ ký số không được giả lập.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    RECT body = ContentBody(r, dpi);
    const int gap = UiMetrics::Scale(12, dpi);
    const int cardW = (body.right - body.left - gap * 2) / 3;
    MetricCardConfig html{L"HTML", L"HỖ TRỢ", L"SaveHtmlReport", L"Báo cáo đọc bằng trình duyệt", CanonicalUiState::Pass, true};
    MetricCardConfig json{L"JSON", L"HỖ TRỢ", L"SaveJsonReport", L"Dữ liệu có cấu trúc", CanonicalUiState::Pass, true};
    MetricCardConfig pdf{L"PDF / Ký số", L"CHƯA HỖ TRỢ", L"Không có backend", L"Không hiển thị như tính năng hoạt động", CanonicalUiState::Unsupported, true};
    RECT a{body.left, body.top, body.left + cardW, body.top + UiMetrics::Scale(110, dpi)};
    RECT b{a.right + gap, body.top, a.right + gap + cardW, a.bottom};
    RECT c{b.right + gap, body.top, body.right, a.bottom};
    DrawMetricCard(dc, a, html, fonts, dpi);
    DrawMetricCard(dc, b, json, fonts, dpi);
    DrawMetricCard(dc, c, pdf, fonts, dpi);

    RECT info{body.left, a.bottom + gap, body.right, body.bottom};
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
    std::wstring note = L"Nút mở báo cáo chỉ hoạt động khi backend đã tạo file HTML thật. JSON được lưu cùng phiên; LapSure không tuyên bố có cloud sharing hoặc PDF khi chưa có implementation.";
    RECT nr{info.left + 14, info.top + 102, info.right - 14, info.bottom - 14};
    DrawTextW(dc, note.c_str(), -1, &nr, DT_LEFT | DT_WORDBREAK);
}

void RenderScreenS20_LogsEvents(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                int activeFilter, int selectedLogIdx, const std::vector<LiveLogEntry>& liveLogs,
                                int tableScrollOffset, int focusIndex) {
    (void)activeFilter; (void)selectedLogIdx; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Nhật ký & Sự kiện";
    hdr.subtitle = L"Hiển thị log runtime và số đếm sự kiện thực; lịch sử sự kiện không tự chứng minh nguyên nhân phần cứng.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    RECT body = ContentBody(r, dpi);
    const int gap = UiMetrics::Scale(10, dpi);
    const int kpiH = UiMetrics::Scale(82, dpi);
    const int kpiW = (body.right - body.left - gap * 4) / 5;
    const auto& ev = rep.hardware.events;
    MetricCardConfig metrics[5] = {
        {L"WHEA", std::to_wstring(ev.whea), L"event count", ev.querySucceeded ? L"Nguồn Event Log" : L"Provider chưa xác nhận", ev.querySucceeded ? CanonicalUiState::Info : CanonicalUiState::ProviderUnavailable, false},
        {L"DISK", std::to_wstring(ev.disk), L"event count", L"Không suy diễn ổ hỏng chỉ từ count", CanonicalUiState::Info, false},
        {L"STORNVME", std::to_wstring(ev.stornvme), L"event count", L"Evidence, không phải verdict", CanonicalUiState::Info, false},
        {L"DISPLAY", std::to_wstring(ev.display), L"event count", L"Evidence, không tự gán GPU hỏng", CanonicalUiState::Info, false},
        {L"BUGCHECK", std::to_wstring(ev.bugCheck), L"event count", L"Cần đối chiếu thời gian/stress", CanonicalUiState::Info, false}
    };
    int x = body.left;
    for (const auto& metric : metrics) {
        RECT cr{x, body.top, x + kpiW, body.top + kpiH};
        DrawMetricCard(dc, cr, metric, fonts, dpi);
        x += kpiW + gap;
    }

    DataTableConfig table;
    table.columns = {
        {L"Thời gian", 100, true, false},
        {L"Nguồn", 150, false, false},
        {L"Thông điệp", 520, false, false},
        {L"Trạng thái", 115, false, true}
    };
    for (const auto& log : liveLogs) {
        CanonicalUiState state = CanonicalUiState::Info;
        if (log.state == 0) state = CanonicalUiState::Good;
        else if (log.state == 1) state = CanonicalUiState::Warning;
        else if (log.state == 2) state = CanonicalUiState::Fail;
        TableRow row;
        row.cells = {NonEmpty(log.time, L"—"), NonEmpty(log.source, L"Runtime"), NonEmpty(log.message), CanonicalStateName(state)};
        row.rowState = state;
        table.rows.push_back(std::move(row));
    }
    table.emptyMessage = L"Chưa có live log cho phiên hiện tại.";
    RECT tr{body.left, body.top + kpiH + gap, body.right, body.bottom};
    DrawDataTable(dc, tr, table, fonts, dpi, tableScrollOffset);
}

void RenderScreenS21_Settings(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                              int selectedCategory, int focusIndex) {
    (void)selectedCategory; (void)focusIndex;
    PageHeaderConfig hdr;
    hdr.title = L"Cài đặt";
    hdr.subtitle = L"Phân biệt chính sách sản phẩm và trạng thái runtime; không hiển thị toggle nếu chưa có persistence thực.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    DataTableConfig table;
    table.columns = {
        {L"Nhóm", 150, false, false},
        {L"Thiết lập / kiểm tra", 220, false, false},
        {L"Giá trị thực", 260, true, false},
        {L"Trạng thái", 120, false, true}
    };
    table.rows.push_back({{L"Giao diện", L"Ngôn ngữ", L"Tiếng Việt", L"THÔNG TIN"}, CanonicalUiState::Info});
    table.rows.push_back({{L"Bảo mật", L"External engine trust", L"SHA-256 allowlist; không tải runtime âm thầm", L"THÔNG TIN"}, CanonicalUiState::Info});
    table.rows.push_back({{L"Báo cáo", L"Định dạng backend", L"HTML + JSON", L"THÔNG TIN"}, CanonicalUiState::Info});
    table.rows.push_back({{L"Môi trường", L"Runtime", NonEmpty(rep.environment, L"Chưa xác định"), L"THÔNG TIN"}, CanonicalUiState::Info});
    for (const auto& check : rep.hardware.stress.runtimeValidation.checks) {
        CanonicalUiState state = CanonicalUiState::NotTested;
        if (check.status == ValidationStatus::Pass) state = CanonicalUiState::Pass;
        else if (check.status == ValidationStatus::Warning) state = CanonicalUiState::Warning;
        else if (check.status == ValidationStatus::Fail) state = CanonicalUiState::Fail;
        TableRow row;
        row.cells = {L"Runtime validation", NonEmpty(check.name), NonEmpty(check.detail, L"Không có chi tiết"), CanonicalStateName(state)};
        row.rowState = state;
        table.rows.push_back(std::move(row));
    }
    table.emptyMessage = L"Không có cấu hình runtime để hiển thị.";
    DrawDataTable(dc, ContentBody(r, dpi), table, fonts, dpi, 0);
}

} // namespace lap
