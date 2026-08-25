#include "lap/ui_screens.h"
#include <string>
#include <utility>
#include <vector>

namespace lap {
namespace {

std::wstring EventCountText(bool available, unsigned long long count) {
    return available ? std::to_wstring(count) : L"KHÔNG KHẢ DỤNG";
}

std::wstring NonEmptyS20(const std::wstring& value, const wchar_t* fallback) {
    return value.empty() ? std::wstring(fallback) : value;
}

} // namespace

void RenderScreenS20_LogsEvents(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                int activeFilter, int selectedLogIdx, const std::vector<LiveLogEntry>& liveLogs,
                                int tableScrollOffset, int focusIndex) {
    (void)activeFilter;
    (void)selectedLogIdx;
    (void)focusIndex;

    PageHeaderConfig hdr;
    hdr.title = L"Nhật ký & Sự kiện";
    hdr.subtitle = L"Hiển thị log runtime và số đếm sự kiện thực; provider không khả dụng không được hiển thị như số 0 đã xác nhận.";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    RECT body{
        r.left + UiMetrics::Scale(24, dpi),
        r.top + UiMetrics::Scale(70, dpi),
        r.right - UiMetrics::Scale(24, dpi),
        r.bottom - UiMetrics::Scale(20, dpi)
    };
    const int gap = UiMetrics::Scale(10, dpi);
    const int kpiH = UiMetrics::Scale(82, dpi);
    const int kpiW = (body.right - body.left - gap * 4) / 5;
    const auto& ev = rep.hardware.events;
    const CanonicalUiState eventState = ev.querySucceeded ? CanonicalUiState::Info : CanonicalUiState::ProviderUnavailable;
    const std::wstring eventDetail = ev.querySucceeded ? L"Nguồn Event Log" : L"Provider Event Log không khả dụng";

    MetricCardConfig metrics[5] = {
        {L"WHEA", EventCountText(ev.querySucceeded, ev.whea), L"event count", eventDetail, eventState, false},
        {L"DISK", EventCountText(ev.querySucceeded, ev.disk), L"event count", ev.querySucceeded ? L"Không suy diễn ổ hỏng chỉ từ count" : eventDetail, eventState, false},
        {L"STORNVME", EventCountText(ev.querySucceeded, ev.stornvme), L"event count", ev.querySucceeded ? L"Evidence, không phải verdict" : eventDetail, eventState, false},
        {L"DISPLAY", EventCountText(ev.querySucceeded, ev.display), L"event count", ev.querySucceeded ? L"Evidence, không tự gán GPU hỏng" : eventDetail, eventState, false},
        {L"BUGCHECK", EventCountText(ev.querySucceeded, ev.bugCheck), L"event count", ev.querySucceeded ? L"Cần đối chiếu thời gian/stress" : eventDetail, eventState, false}
    };

    int x = body.left;
    for (const auto& metric : metrics) {
        RECT card{x, body.top, x + kpiW, body.top + kpiH};
        DrawMetricCard(dc, card, metric, fonts, dpi);
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
        if (log.state == 1) state = CanonicalUiState::Warning;
        else if (log.state == 2) state = CanonicalUiState::Fail;
        TableRow row;
        row.cells = {
            NonEmptyS20(log.time, L"—"),
            NonEmptyS20(log.source, L"Runtime"),
            NonEmptyS20(log.message, L"Không có thông điệp"),
            CanonicalStateName(state)
        };
        row.rowState = state;
        table.rows.push_back(std::move(row));
    }
    table.emptyMessage = L"Chưa có live log cho phiên hiện tại.";
    RECT tableRect{body.left, body.top + kpiH + gap, body.right, body.bottom};
    DrawDataTable(dc, tableRect, table, fonts, dpi, tableScrollOffset);
}

} // namespace lap
