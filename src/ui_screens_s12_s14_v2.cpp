#include "lap/ui_screens.h"
#include <algorithm>
#include <sstream>

namespace lap {
namespace {

const FunctionalItemResult* FindItem(const FunctionalTestSummary& summary, const wchar_t* id) {
    for (const auto& item : summary.items) if (item.id == id) return &item;
    return nullptr;
}

CanonicalUiState ItemState(const FunctionalTestSummary& summary, const wchar_t* id) {
    const auto* item = FindItem(summary, id);
    return item ? MapFunctionalStatus(item->status) : CanonicalUiState::NotTested;
}

std::wstring ItemEvidence(const FunctionalTestSummary& summary, const wchar_t* id) {
    const auto* item = FindItem(summary, id);
    if (!item) return L"Chưa có bằng chứng chức năng";
    if (!item->evidence.empty()) return item->evidence;
    if (!item->detail.empty()) return item->detail;
    return L"Provider không trả chi tiết bổ sung";
}

std::wstring Label(CanonicalUiState state) { return GetStatePresentation(state).label; }

CanonicalUiState DisplayIdentityState(const DisplayInfo& display) {
    if (display.edidHex.empty()) return CanonicalUiState::Incomplete;
    if (!display.nativeWidth || !display.nativeHeight) return CanonicalUiState::Incomplete;
    return CanonicalUiState::Info;
}

CanonicalUiState PortState(const PortPowerSummary& summary) {
    if (summary.overall == L"PASS") return CanonicalUiState::Good;
    if (summary.overall == L"FAIL") return CanonicalUiState::Fail;
    if (summary.overall == L"WARNING" || summary.overall == L"PARTIAL") return CanonicalUiState::Warning;
    return CanonicalUiState::NotTested;
}

} // namespace

void RenderScreenS12_Display(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                             int colorIndex, const std::vector<int>& defectCheckStates, int focusIndex) {
    (void)colorIndex; (void)defectCheckStates; (void)focusIndex;
    const auto& functional = rep.hardware.stress.functional;
    const auto visualState = ItemState(functional, L"display_visual");
    const bool hasDisplay = !rep.hardware.displays.empty();
    const auto identityState = hasDisplay ? DisplayIdentityState(rep.hardware.displays.front()) : CanonicalUiState::NotTested;

    CanonicalUiState overall = CanonicalUiState::Incomplete;
    if (visualState == CanonicalUiState::Fail) overall = CanonicalUiState::Fail;
    else if (visualState == CanonicalUiState::Good || visualState == CanonicalUiState::Pass)
        overall = identityState == CanonicalUiState::Info ? CanonicalUiState::Good : CanonicalUiState::Incomplete;
    else if (!hasDisplay) overall = CanonicalUiState::NotTested;

    PageHeaderConfig header;
    header.title = L"Hiển thị";
    header.subtitle = L"EDID/native timing là identity evidence; điểm chết, hở sáng và ám màu chỉ được kết luận sau kiểm tra trực quan.";
    header.sessionTag = Label(overall);
    header.sessionState = overall;
    DrawPageHeader(dc, r, header, fonts, dpi);

    const int pad = UiMetrics::Scale(24, dpi);
    const int top = r.top + UiMetrics::Scale(72, dpi);
    const int rightW = UiMetrics::Scale(300, dpi);
    const int leftRight = r.right - rightW - UiMetrics::Scale(34, dpi);

    DataTableConfig table;
    table.columns = {
        { L"Bằng chứng", 165, false, false }, { L"Giá trị thực tế", 270, true, false },
        { L"Giới hạn diễn giải", 270, false, false }, { L"Trạng thái", 115, false, true }
    };
    if (hasDisplay) {
        const auto& d = rep.hardware.displays.front();
        auto add = [&](const std::wstring& name, const std::wstring& value, const std::wstring& note, CanonicalUiState state) {
            table.rows.push_back({ { name, value, note, Label(state) }, state });
        };
        add(L"Panel / Friendly Name", d.friendlyName.empty() ? L"Chưa có tên EDID" : d.friendlyName,
            L"Identity only", d.friendlyName.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Info);
        add(L"Manufacturer", d.manufacturer.empty() ? L"Chưa xác định" : d.manufacturer,
            L"Không dùng vendor code để suy diễn panel 'zin'", d.manufacturer.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Info);
        add(L"Serial EDID", d.serialNumber.empty() ? L"Chưa có" : d.serialNumber,
            L"Thiếu serial không đồng nghĩa panel lỗi", d.serialNumber.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Info);
        const std::wstring native = (d.nativeWidth && d.nativeHeight)
            ? std::to_wstring(d.nativeWidth) + L" × " + std::to_wstring(d.nativeHeight) : L"Chưa xác định";
        add(L"Native resolution", native, L"Phải đến từ EDID/native timing; current mode không phải native proof",
            (d.nativeWidth && d.nativeHeight) ? CanonicalUiState::Info : CanonicalUiState::Incomplete);
        const std::wstring current = (d.currentWidth && d.currentHeight)
            ? std::to_wstring(d.currentWidth) + L" × " + std::to_wstring(d.currentHeight) : L"Chưa xác định";
        add(L"Current mode", current, L"Chỉ là chế độ Windows đang xuất", (d.currentWidth && d.currentHeight) ? CanonicalUiState::Info : CanonicalUiState::NotTested);
        add(L"Refresh rate", d.refreshHz ? std::to_wstring(d.refreshHz) + L" Hz" : L"Chưa xác định",
            L"Không suy đoán từ model panel", d.refreshHz ? CanonicalUiState::Info : CanonicalUiState::NotTested);
        add(L"Touch presence", d.touchDetected ? L"Có thiết bị touch" : L"Không phát hiện touch",
            L"Presence tách biệt với kiểm tra touch functionality", CanonicalUiState::Info);
        add(L"EDID payload", d.edidHex.empty() ? L"Không đọc được" : L"Đã thu thập EDID",
            L"EDID phải được provider validate; không tự gán checksum OK", d.edidHex.empty() ? CanonicalUiState::ProviderUnavailable : CanonicalUiState::Info);
    } else {
        table.emptyMessage = L"Chưa có DisplayInfo/EDID. Không hiển thị panel model, độ phân giải native hoặc trạng thái giả định.";
    }
    RECT tableRect{ r.left + pad, top, leftRight, r.bottom - UiMetrics::Scale(20, dpi) };
    DrawDataTable(dc, tableRect, table, fonts, dpi, 0);

    const int rightX = leftRight + UiMetrics::Scale(10, dpi);
    RECT rail{ rightX, top, r.right - pad, r.bottom - UiMetrics::Scale(20, dpi) };
    NextActionConfig action;
    action.actionTitle = L"Kiểm tra trực quan màn hình";
    action.reasonText = L"Dead/stuck pixel, bleed, ám màu và lỗi bề mặt không được suy ra từ EDID. Chỉ wizard trực quan/người kiểm tra mới tạo bằng chứng này.";
    action.remainingTasks = {
        L"Visual test: " + Label(visualState),
        L"Evidence: " + ItemEvidence(functional, L"display_visual"),
        L"Brightness / eDP link rate: không hiển thị nếu chưa có provider"
    };
    action.buttonText = L"MỞ WIZARD HIỂN THỊ";
    action.isButtonEnabled = true;
    DrawNextActionPanel(dc, rail, action, fonts, dpi);
}

void RenderScreenS14_Network(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                             const std::vector<float>& rssiHistory, const std::vector<LiveLogEntry>& netLogs,
                             int focusIndex) {
    (void)rssiHistory; (void)netLogs; (void)focusIndex;
    const auto& functional = rep.hardware.stress.functional;
    const auto wifi = ItemState(functional, L"wifi_function");
    const auto bluetooth = ItemState(functional, L"bluetooth_function");
    const auto ethernetPresence = ItemState(functional, L"ethernet_presence");
    const auto ports = PortState(rep.hardware.stress.portPower);

    CanonicalUiState overall = CanonicalUiState::Incomplete;
    if (wifi == CanonicalUiState::Fail || bluetooth == CanonicalUiState::Fail || ports == CanonicalUiState::Fail)
        overall = CanonicalUiState::Fail;
    else if ((wifi == CanonicalUiState::Good || wifi == CanonicalUiState::Pass) &&
             (bluetooth == CanonicalUiState::Good || bluetooth == CanonicalUiState::Pass))
        overall = CanonicalUiState::Good;

    PageHeaderConfig header;
    header.title = L"Mạng & Kết nối";
    header.subtitle = L"Wi-Fi association, Bluetooth interaction và LAN/port evidence được tách riêng; throughput Internet phụ thuộc hạ tầng mạng.";
    header.sessionTag = Label(overall);
    header.sessionState = overall;
    DrawPageHeader(dc, r, header, fonts, dpi);

    const int pad = UiMetrics::Scale(24, dpi);
    const int top = r.top + UiMetrics::Scale(72, dpi);
    const int rightW = UiMetrics::Scale(300, dpi);
    const int leftRight = r.right - rightW - UiMetrics::Scale(34, dpi);

    DataTableConfig table;
    table.columns = {
        { L"Phân hệ", 150, false, false }, { L"Bằng chứng thực tế", 330, true, false },
        { L"Điều được chứng minh", 260, false, false }, { L"Trạng thái", 115, false, true }
    };
    table.rows.push_back({ {
        L"Wi-Fi", ItemEvidence(functional, L"wifi_function"),
        L"WLAN adapter/association/signal quality khi API trả về; không phải Internet benchmark", Label(wifi)
    }, wifi });
    table.rows.push_back({ {
        L"Bluetooth", ItemEvidence(functional, L"bluetooth_function"),
        L"Radio access/pairing interaction phải phân biệt với chỉ enumeration", Label(bluetooth)
    }, bluetooth });
    table.rows.push_back({ {
        L"Ethernet presence", ItemEvidence(functional, L"ethernet_presence"),
        L"Presence chỉ nhận diện adapter; chưa chứng minh jack/cáp/link vật lý", Label(ethernetPresence)
    }, ethernetPresence });
    table.rows.push_back({ {
        L"LAN / physical ports", rep.hardware.stress.portPower.ports.empty() ? L"Chưa có port stimulus evidence" : rep.hardware.stress.portPower.overall,
        L"Negotiated link/PNP delta là evidence cổng; không suy ra chất lượng Internet", Label(ports)
    }, ports });
    RECT tableRect{ r.left + pad, top, leftRight, r.bottom - UiMetrics::Scale(20, dpi) };
    DrawDataTable(dc, tableRect, table, fonts, dpi, 0);

    const int rightX = leftRight + UiMetrics::Scale(10, dpi);
    RECT rail{ rightX, top, r.right - pad, r.bottom - UiMetrics::Scale(20, dpi) };
    NextActionConfig action;
    action.actionTitle = L"Xác minh kết nối thực tế";
    action.reasonText = L"Không chấm điểm card mạng từ ping/throughput Internet vì router, ISP và nhiễu RF nằm ngoài laptop. Bluetooth cần thiết bị mẫu/interaction, không chỉ radio enumeration.";
    action.remainingTasks = {
        L"Wi-Fi: " + Label(wifi),
        L"Bluetooth: " + Label(bluetooth),
        L"LAN/ports: " + Label(ports)
    };
    action.buttonText = L"CHẠY KIỂM TRA KẾT NỐI";
    action.isButtonEnabled = true;
    DrawNextActionPanel(dc, rail, action, fonts, dpi);
}

} // namespace lap
