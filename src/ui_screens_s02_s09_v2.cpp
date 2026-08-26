#include "lap/ui_screens.h"
#include <algorithm>
#include <initializer_list>
#include <iomanip>
#include <sstream>

namespace lap {

namespace {

std::wstring DashIfEmpty(const std::wstring& value) {
    return value.empty() ? L"—" : value;
}

std::wstring FormatGiB(uint64_t bytes) {
    if (!bytes) return L"—";
    const double gib = static_cast<double>(bytes) / 1073741824.0;
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(gib >= 100.0 ? 0 : 1) << gib << L" GiB";
    return ss.str();
}

std::wstring FormatGB(uint64_t bytes) {
    if (!bytes) return L"—";
    const double gb = static_cast<double>(bytes) / 1000000000.0;
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(gb >= 100.0 ? 0 : 1) << gb << L" GB";
    return ss.str();
}

std::wstring FormatOneDecimal(double value, const wchar_t* unit) {
    if (value < 0.0) return L"—";
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(1) << value << unit;
    return ss.str();
}

std::wstring FormatPrice(long long value) {
    if (value <= 0) return L"Chưa ghi nhận";
    std::wstring digits = std::to_wstring(value);
    for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3) digits.insert(static_cast<size_t>(i), 1, L'.');
    return digits + L" ₫";
}

const FunctionalItemResult* FindFunctional(const FunctionalTestSummary& summary, const wchar_t* id) {
    for (const auto& item : summary.items) if (item.id == id) return &item;
    return nullptr;
}

CanonicalUiState FunctionalState(const FunctionalTestSummary& summary, const wchar_t* id) {
    const auto* item = FindFunctional(summary, id);
    return item ? MapFunctionalStatus(item->status) : CanonicalUiState::NotTested;
}

std::wstring FunctionalDetail(const FunctionalTestSummary& summary, const wchar_t* id,
                              const wchar_t* fallback = L"Chưa có bằng chứng") {
    const auto* item = FindFunctional(summary, id);
    if (!item) return fallback;
    if (!item->detail.empty()) return item->detail;
    if (!item->evidence.empty()) return item->evidence;
    return fallback;
}

CanonicalUiState AggregateStates(std::initializer_list<CanonicalUiState> states) {
    int pass = 0;
    int unsupported = 0;
    const int total = static_cast<int>(states.size());
    for (auto state : states) {
        if (state == CanonicalUiState::Fail || state == CanonicalUiState::Error) return CanonicalUiState::Fail;
        if (state == CanonicalUiState::Warning) return CanonicalUiState::Warning;
        if (state == CanonicalUiState::PermissionDenied) return CanonicalUiState::PermissionDenied;
        if (state == CanonicalUiState::ProviderUnavailable) return CanonicalUiState::ProviderUnavailable;
        if (state == CanonicalUiState::ManualRequired) return CanonicalUiState::ManualRequired;
        if (state == CanonicalUiState::Interrupted || state == CanonicalUiState::Cancelled) return state;
        if (state == CanonicalUiState::Pass || state == CanonicalUiState::Good) ++pass;
        else if (state == CanonicalUiState::Unsupported) ++unsupported;
    }
    if (total > 0 && pass == total) return CanonicalUiState::Good;
    if (total > 0 && unsupported == total) return CanonicalUiState::Unsupported;
    return CanonicalUiState::Incomplete;
}

CanonicalUiState VerdictState(const std::wstring& verdict) {
    if (verdict == L"PASS") return CanonicalUiState::Good;
    if (verdict == L"FAIL") return CanonicalUiState::Fail;
    if (verdict == L"WARNING" || verdict == L"PARTIAL") return CanonicalUiState::Warning;
    if (verdict == L"UNSUPPORTED") return CanonicalUiState::Unsupported;
    if (verdict == L"CANCELLED") return CanonicalUiState::Cancelled;
    return CanonicalUiState::Incomplete;
}

std::wstring UiLabel(CanonicalUiState state) {
    return GetStatePresentation(state).label;
}

CanonicalUiState PhysicalOverall(const FunctionalTestSummary& summary) {
    return AggregateStates({
        FunctionalState(summary, L"physical_chassis"),
        FunctionalState(summary, L"physical_hinge"),
        FunctionalState(summary, L"physical_tamper"),
        FunctionalState(summary, L"physical_liquid"),
        FunctionalState(summary, L"physical_battery"),
        FunctionalState(summary, L"physical_charger")
    });
}

CanonicalUiState IoBatchOverall(const FunctionalTestSummary& summary) {
    return AggregateStates({
        FunctionalState(summary, L"camera_function"),
        FunctionalState(summary, L"mic_function"),
        FunctionalState(summary, L"wifi_function"),
        FunctionalState(summary, L"bluetooth_function")
    });
}

std::wstring PortEvidence(const PortProbeResult& port) {
    if (!port.evidence.empty()) return port.evidence;
    if (!port.deviceDescription.empty()) return port.deviceDescription;
    return L"Chưa có bằng chứng kích thích vật lý";
}

const PortProbeResult* FindPortResult(const PortPowerSummary& summary, const std::wstring& label) {
    for (const auto& result : summary.ports) if (result.portLabel == label) return &result;
    return nullptr;
}

void DrawSectionTitle(HDC dc, int x, int y, const wchar_t* text, const UiFonts& fonts) {
    SelectObject(dc, fonts.hBodyBold);
    SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, x, y, text, static_cast<int>(wcslen(text)));
}

} // namespace

void RenderScreenS02_NewSession(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                int inspectionPurpose, const std::wstring& selectedMode, bool running,
                                int focusIndex) {
    PageHeaderConfig hdr;
    hdr.title = L"Phiên kiểm định mới";
    hdr.subtitle = L"Chọn mục đích và chế độ trước khi LapSure bắt đầu thu thập bằng chứng.";
    hdr.sessionTag = running ? L"Đang kiểm định" : L"Chuẩn bị";
    hdr.sessionState = running ? CanonicalUiState::Running : CanonicalUiState::Ready;
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    const int rightPanelW = UiMetrics::Scale(300, dpi);
    const int leftW = static_cast<int>(r.right - r.left) - UiMetrics::Scale(48, dpi) - rightPanelW;
    int curY = r.top + UiMetrics::Scale(94, dpi); // matches main.cpp purpose-card hit testing
    const int startX = r.left + UiMetrics::Scale(24, dpi);

    struct Purpose { const wchar_t* title; const wchar_t* description; };
    const Purpose purposes[3] = {
        { L"Mua laptop đã qua sử dụng", L"Ưu tiên bằng chứng sức khỏe, chức năng, an toàn vật lý và đối chiếu người bán." },
        { L"Bảo hành & kiểm tra lỗi", L"Ưu tiên tái hiện lỗi, stress, nhiệt, PnP và sự kiện hệ thống." },
        { L"Định giá & bàn giao", L"Ưu tiên nhận diện cấu hình, hồ sơ thiết bị, chức năng và báo cáo bằng chứng." }
    };

    for (int i = 0; i < 3; ++i) {
        const bool selected = inspectionPurpose == i;
        RECT card{ startX, curY, startX + leftW, curY + UiMetrics::Scale(64, dpi) };
        DrawRoundedCard(dc, card, UiMetrics::RadiusMd,
                        selected ? UiColors::PrimaryBlueLight : UiColors::CardBg,
                        selected ? UiColors::PrimaryBlue : UiColors::CardBorder,
                        selected ? 2 : 1);
        SelectObject(dc, fonts.hBodyBold);
        SetTextColor(dc, selected ? UiColors::PrimaryBlue : UiColors::TextMain);
        TextOutW(dc, card.left + UiMetrics::Scale(14, dpi), card.top + UiMetrics::Scale(9, dpi),
                 purposes[i].title, static_cast<int>(wcslen(purposes[i].title)));
        SelectObject(dc, fonts.hSmall);
        SetTextColor(dc, UiColors::TextMuted);
        RECT description{ card.left + UiMetrics::Scale(14, dpi), card.top + UiMetrics::Scale(30, dpi),
                          card.right - UiMetrics::Scale(14, dpi), card.bottom - UiMetrics::Scale(6, dpi) };
        DrawTextW(dc, purposes[i].description, -1, &description, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
        curY += UiMetrics::Scale(72, dpi);
    }

    DrawSectionTitle(dc, startX, curY + UiMetrics::Scale(8, dpi), L"Chế độ kiểm tra", fonts);
    curY += UiMetrics::Scale(32, dpi); // matches main.cpp mode-card hit testing

    struct Mode { const wchar_t* id; const wchar_t* title; const wchar_t* description; };
    const Mode modes[3] = {
        { L"Quick", L"Nhanh", L"Thu thập nhanh; các mục chưa đủ bằng chứng vẫn giữ trạng thái chưa hoàn tất." },
        { L"Standard", L"Tiêu chuẩn", L"Cân bằng thời gian và độ bao phủ; phù hợp phần lớn ca kiểm định máy cũ." },
        { L"Deep", L"Chuyên sâu", L"Tăng thời lượng stress/kiểm tra; không thay thế các xác nhận vật lý bắt buộc." }
    };
    const int modeCardW = (leftW - UiMetrics::Scale(16, dpi)) / 3;
    for (int i = 0; i < 3; ++i) {
        const bool selected = selectedMode == modes[i].id;
        const int x = startX + i * (modeCardW + UiMetrics::Scale(8, dpi));
        RECT card{ x, curY, x + modeCardW, curY + UiMetrics::Scale(110, dpi) };
        DrawRoundedCard(dc, card, UiMetrics::RadiusMd,
                        selected ? UiColors::PrimaryBlueLight : UiColors::CardBg,
                        selected ? UiColors::PrimaryBlue : UiColors::CardBorder,
                        selected ? 2 : 1);
        SelectObject(dc, fonts.hBodyBold);
        SetTextColor(dc, selected ? UiColors::PrimaryBlue : UiColors::TextMain);
        TextOutW(dc, card.left + UiMetrics::Scale(12, dpi), card.top + UiMetrics::Scale(10, dpi),
                 modes[i].title, static_cast<int>(wcslen(modes[i].title)));
        SelectObject(dc, fonts.hSmall);
        SetTextColor(dc, UiColors::TextMuted);
        RECT description{ card.left + UiMetrics::Scale(12, dpi), card.top + UiMetrics::Scale(34, dpi),
                          card.right - UiMetrics::Scale(12, dpi), card.bottom - UiMetrics::Scale(8, dpi) };
        DrawTextW(dc, modes[i].description, -1, &description, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
        if (focusIndex == 1 && selected) DrawFocusRing(dc, card, UiMetrics::RadiusMd);
    }

    const int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    const int rightY = r.top + UiMetrics::Scale(70, dpi);
    RECT preflight{ rightX, rightY, r.right - UiMetrics::Scale(24, dpi), rightY + UiMetrics::Scale(220, dpi) };
    DrawRoundedCard(dc, preflight, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    DrawSectionTitle(dc, preflight.left + UiMetrics::Scale(14, dpi), preflight.top + UiMetrics::Scale(12, dpi),
                     L"Ngữ cảnh bằng chứng", fonts);

    DataTableConfig preflightTable;
    preflightTable.columns = {
        { L"Hạng mục", 120, false, false }, { L"Trạng thái", 125, false, true }
    };
    preflightTable.rows.push_back({ { L"Model hiện có", rep.model.empty() ? L"Sẽ nhận diện khi chạy" : rep.model },
                                    rep.model.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Info });
    preflightTable.rows.push_back({ { L"Service Tag", rep.serviceTag.empty() ? L"Sẽ nhận diện khi chạy" : rep.serviceTag },
                                    rep.serviceTag.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Info });
    preflightTable.rows.push_back({ { L"Hồ sơ chassis", rep.hardware.stress.chassisProfile.profileId.empty()
                                        ? L"Nạp theo model/Service Tag" : rep.hardware.stress.chassisProfile.validationStatus },
                                    CanonicalUiState::Info });
    RECT preflightRows{ preflight.left + UiMetrics::Scale(10, dpi), preflight.top + UiMetrics::Scale(36, dpi),
                        preflight.right - UiMetrics::Scale(10, dpi), preflight.bottom - UiMetrics::Scale(10, dpi) };
    DrawDataTable(dc, preflightRows, preflightTable, fonts, dpi, 0);

    RECT action{ rightX, preflight.bottom + UiMetrics::Scale(12, dpi), r.right - UiMetrics::Scale(24, dpi),
                 preflight.bottom + UiMetrics::Scale(180, dpi) };
    NextActionConfig next;
    next.actionTitle = running ? L"Phiên đang chạy" : L"Bắt đầu phiên kiểm định";
    next.reasonText = L"LapSure chỉ kết luận khi các domain bắt buộc có đủ bằng chứng; thiếu provider hoặc xác nhận thủ công sẽ không tự biến thành ĐẠT.";
    next.remainingTasks = { L"Quét tự động", L"Kiểm tra chức năng", L"Ngoại hình / Cổng / Nguồn", L"Đánh giá cuối cùng" };
    next.buttonText = running ? L"ĐANG KIỂM ĐỊNH" : L"BẮT ĐẦU PHIÊN KIỂM ĐỊNH";
    next.isButtonEnabled = !running;
    DrawNextActionPanel(dc, action, next, fonts, dpi);
}

void RenderScreenS03_SellerClaim(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                 int focusIndex) {
    (void)focusIndex;
    const auto& claim = rep.sellerClaim;
    PageHeaderConfig hdr;
    hdr.title = L"Cam kết người bán";
    hdr.subtitle = L"Ghi lại nội dung rao bán để đối chiếu với bằng chứng phần cứng; cam kết không thay thế kiểm tra kỹ thuật.";
    hdr.sessionTag = claim.provided ? L"Đã ghi nhận" : L"Chưa ghi nhận";
    hdr.sessionState = claim.provided ? CanonicalUiState::Info : CanonicalUiState::NotTested;
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    const int rightPanelW = UiMetrics::Scale(300, dpi);
    const int leftW = static_cast<int>(r.right - r.left) - UiMetrics::Scale(48, dpi) - rightPanelW;
    const int curY = r.top + UiMetrics::Scale(70, dpi);

    DataTableConfig table;
    table.columns = {
        { L"Thông tin người bán", 170, false, false },
        { L"Giá trị đã ghi nhận", 260, true, false },
        { L"Đối chiếu", 120, false, true }
    };
    if (!claim.provided) {
        table.emptyMessage = L"Chưa nhập cam kết người bán. Không có dữ liệu nào được giả định.";
    } else {
        auto comparisonState = [&](const wchar_t* name) {
            for (const auto& finding : rep.findings) {
                if (finding.group == L"Cam kết người bán" && finding.name == name) return MapState(finding.state);
            }
            return CanonicalUiState::NotTested;
        };
        auto add = [&](const wchar_t* label, const std::wstring& value, CanonicalUiState state) {
            table.rows.push_back({ { label, value, UiLabel(state) }, state });
        };
        add(L"Model", DashIfEmpty(claim.model), claim.model.empty() ? CanonicalUiState::NotTested : comparisonState(L"Model"));
        add(L"CPU", DashIfEmpty(claim.cpuContains), claim.cpuContains.empty() ? CanonicalUiState::NotTested : comparisonState(L"CPU"));
        add(L"RAM", claim.ramBytes ? FormatGiB(claim.ramBytes) : L"Chưa ghi nhận",
            claim.ramBytes ? comparisonState(L"RAM") : CanonicalUiState::NotTested);
        add(L"GPU", DashIfEmpty(claim.gpuContains), claim.gpuContains.empty() ? CanonicalUiState::NotTested : comparisonState(L"GPU"));
        add(L"Ổ lưu trữ", claim.storageBytes ? FormatGB(claim.storageBytes) : L"Chưa ghi nhận",
            claim.storageBytes ? comparisonState(L"Ổ lưu trữ") : CanonicalUiState::NotTested);
        const std::wstring display = (claim.displayWidth && claim.displayHeight)
            ? std::to_wstring(claim.displayWidth) + L" × " + std::to_wstring(claim.displayHeight)
            : L"Chưa ghi nhận";
        add(L"Màn hình", display, (claim.displayWidth && claim.displayHeight) ? comparisonState(L"Màn hình") : CanonicalUiState::NotTested);
        add(L"Giá chào bán", FormatPrice(claim.askingPriceVnd), CanonicalUiState::Info);
        add(L"Bảo hành", claim.warrantyDays ? std::to_wstring(claim.warrantyDays) + L" ngày" : L"Chưa ghi nhận", CanonicalUiState::Info);
    }
    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW,
                    r.bottom - UiMetrics::Scale(20, dpi) };
    DrawDataTable(dc, tableRect, table, fonts, dpi, 0);

    const int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT action{ rightX, curY, r.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(240, dpi) };
    NextActionConfig next;
    next.actionTitle = claim.provided ? L"Cập nhật cam kết" : L"Ghi nhận cam kết";
    next.reasonText = claim.provided
        ? L"Chỉ các trường người bán thực sự cung cấp mới được đối chiếu. Sai khác cấu hình là evidence thương mại, tách biệt với sức khỏe phần cứng."
        : L"Nhập đúng nội dung rao bán/người bán xác nhận. Trường bỏ trống sẽ giữ nguyên là chưa có dữ liệu.";
    next.remainingTasks = { L"Model / CPU / RAM / GPU", L"Ổ lưu trữ / màn hình", L"Giá / bảo hành (tùy chọn)" };
    next.buttonText = claim.provided ? L"CHỈNH SỬA CAM KẾT" : L"NHẬP CAM KẾT NGƯỜI BÁN";
    next.isButtonEnabled = true;
    DrawNextActionPanel(dc, action, next, fonts, dpi);
}

void RenderScreenS05_Functional(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                int subTab, const std::vector<int>& keyStates,
                                const std::vector<POINT>& touchpadTrail, bool touchpadDone,
                                int focusIndex) {
    (void)subTab; (void)keyStates; (void)touchpadTrail; (void)touchpadDone; (void)focusIndex;
    const auto& summary = rep.hardware.stress.functional;
    PageHeaderConfig hdr;
    hdr.title = L"Kiểm tra Chức năng";
    hdr.subtitle = L"Các bài test tương tác phải tạo bằng chứng thực tế; phát hiện thiết bị không đồng nghĩa thiết bị hoạt động.";
    hdr.sessionTag = summary.items.empty() ? L"Chưa kiểm tra" : summary.overall;
    hdr.sessionState = summary.overall == L"PASS" ? CanonicalUiState::Good
        : (summary.failed ? CanonicalUiState::Fail
                          : (summary.warning ? CanonicalUiState::Warning
                                             : (summary.items.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Incomplete)));
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    const int rightPanelW = UiMetrics::Scale(250, dpi);
    const int leftW = static_cast<int>(r.right - r.left) - UiMetrics::Scale(48, dpi) - rightPanelW;
    const int startX = r.left + UiMetrics::Scale(24, dpi);

    ProgressCoverageConfig progress;
    progress.label = L"Bằng chứng chức năng";
    progress.completed = static_cast<int>(summary.passed + summary.failed + summary.warning);
    progress.total = std::max(1, static_cast<int>(summary.items.size()));
    progress.isEvidenceCoverage = true;
    RECT progressRect{ startX, r.top + UiMetrics::Scale(76, dpi), startX + leftW, r.top + UiMetrics::Scale(120, dpi) };
    DrawProgressCoverage(dc, progressRect, progress, fonts, dpi);

    const int curY = r.top + UiMetrics::Scale(135, dpi); // exact geometry used by main.cpp hit tests
    const int cardW = (leftW - UiMetrics::Scale(12, dpi)) / 2;
    const int cardH = UiMetrics::Scale(95, dpi);

    struct Module { const wchar_t* title; CanonicalUiState state; std::wstring detail; };
    const auto displayState = FunctionalState(summary, L"display_visual");
    const auto keyboardState = FunctionalState(summary, L"keyboard_function");
    const auto speakerState = FunctionalState(summary, L"speaker_function");
    const auto ioState = IoBatchOverall(summary);
    const auto portState = VerdictState(rep.hardware.stress.portPower.overall);
    const auto physicalState = PhysicalOverall(summary);
    const Module modules[6] = {
        { L"Màn hình trực quan", displayState, FunctionalDetail(summary, L"display_visual") },
        { L"Bàn phím", keyboardState, FunctionalDetail(summary, L"keyboard_function") },
        { L"Loa trái / phải", speakerState, FunctionalDetail(summary, L"speaker_function") },
        { L"Camera • Mic • Wi-Fi • Bluetooth", ioState, L"Chạy probe chức năng thực tế; không dùng presence làm PASS." },
        { L"Cổng vật lý mẫu", portState, rep.hardware.stress.portPower.ports.empty() ? L"Chưa có phép kích thích cổng" : rep.hardware.stress.portPower.overall },
        { L"Ngoại hình & an toàn", physicalState, L"6 xác nhận vật lý do người kiểm tra thực hiện" }
    };

    for (int i = 0; i < 6; ++i) {
        const int col = i % 2;
        const int row = i / 2;
        const int x = startX + col * (cardW + UiMetrics::Scale(12, dpi));
        const int y = curY + row * (cardH + UiMetrics::Scale(10, dpi));
        RECT card{ x, y, x + cardW, y + cardH };
        DrawRoundedCard(dc, card, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
        SelectObject(dc, fonts.hBodyBold);
        SetTextColor(dc, UiColors::TextMain);
        TextOutW(dc, card.left + UiMetrics::Scale(12, dpi), card.top + UiMetrics::Scale(10, dpi),
                 modules[i].title, static_cast<int>(wcslen(modules[i].title)));
        RECT badge{ card.right - UiMetrics::Scale(126, dpi), card.top + UiMetrics::Scale(8, dpi),
                    card.right - UiMetrics::Scale(10, dpi), card.top + UiMetrics::Scale(30, dpi) };
        DrawStatusBadge(dc, badge, modules[i].state, fonts);
        SelectObject(dc, fonts.hSmall);
        SetTextColor(dc, UiColors::TextMuted);
        RECT detail{ card.left + UiMetrics::Scale(12, dpi), card.top + UiMetrics::Scale(38, dpi),
                     card.right - UiMetrics::Scale(12, dpi), card.bottom - UiMetrics::Scale(8, dpi) };
        DrawTextW(dc, modules[i].detail.c_str(), static_cast<int>(modules[i].detail.size()), &detail,
                  DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    const int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT action{ rightX, curY, r.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(305, dpi) };
    NextActionConfig next;
    next.actionTitle = summary.overall == L"PASS" ? L"Chức năng đã đủ bằng chứng" : L"Hoàn tất các mục còn thiếu";
    next.reasonText = L"Mỗi bài test chỉ được PASS sau khi đúng stimulus/measurement hoặc xác nhận người dùng đã hoàn thành.";
    if (summary.manualRequired) next.remainingTasks.push_back(std::to_wstring(summary.manualRequired) + L" mục cần xác nhận");
    if (summary.notTested) next.remainingTasks.push_back(std::to_wstring(summary.notTested) + L" mục chưa kiểm tra");
    if (summary.failed) next.remainingTasks.push_back(std::to_wstring(summary.failed) + L" mục không đạt");
    if (next.remainingTasks.empty()) next.remainingTasks.push_back(L"Không còn mục chức năng bắt buộc đang mở");
    next.buttonText = L"TIẾP TỤC HẠNG MỤC CÒN THIẾU";
    next.isButtonEnabled = true;
    DrawNextActionPanel(dc, action, next, fonts, dpi);
}

void RenderScreenS06_PhysicalSafety(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                    int activeStep, int activeHotspot, const std::vector<int>& checkState,
                                    int focusIndex) {
    (void)activeStep; (void)activeHotspot; (void)checkState; (void)focusIndex;
    const auto& functional = rep.hardware.stress.functional;
    const auto overall = PhysicalOverall(functional);
    PageHeaderConfig hdr;
    hdr.title = L"Ngoại hình & An toàn";
    hdr.subtitle = L"6 quan sát vật lý bắt buộc; phần mềm không suy đoán tình trạng từ hình dáng hoặc presence.";
    hdr.sessionTag = UiLabel(overall);
    hdr.sessionState = overall;
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    const int rightPanelW = UiMetrics::Scale(300, dpi);
    const int leftW = static_cast<int>(r.right - r.left) - UiMetrics::Scale(48, dpi) - rightPanelW;
    const int curY = r.top + UiMetrics::Scale(70, dpi);

    struct Check { const wchar_t* id; const wchar_t* title; const wchar_t* expectation; };
    const Check checks[6] = {
        { L"physical_chassis", L"Vỏ và kết cấu", L"Không nứt/móp mạnh/hở bất thường" },
        { L"physical_hinge", L"Bản lề", L"Không lỏng/cứng/kêu/tách vỏ bất thường" },
        { L"physical_tamper", L"Dấu hiệu tháo sửa", L"Ghi nhận cảnh báo; không đồng nghĩa phần cứng hỏng" },
        { L"physical_liquid", L"Chất lỏng / ăn mòn / mùi khét", L"Không có dấu hiệu an toàn nghiêm trọng" },
        { L"physical_battery", L"Dấu hiệu pin phồng", L"Không đội touchpad/bàn phím/đáy máy" },
        { L"physical_charger", L"An toàn bộ sạc", L"Không hở lõi/nứt/chân lỏng/mùi khét" }
    };
    DataTableConfig table;
    table.columns = {
        { L"Hạng mục", 170, false, false }, { L"Kỳ vọng", 250, false, false },
        { L"Bằng chứng thực tế", 250, true, false }, { L"Trạng thái", 120, false, true }
    };
    for (const auto& check : checks) {
        const auto state = FunctionalState(functional, check.id);
        table.rows.push_back({ { check.title, check.expectation, FunctionalDetail(functional, check.id), UiLabel(state) }, state });
    }
    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW,
                    r.bottom - UiMetrics::Scale(20, dpi) };
    DrawDataTable(dc, tableRect, table, fonts, dpi, 0);

    const int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT action{ rightX, curY, r.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(310, dpi) };
    NextActionConfig next;
    next.actionTitle = overall == CanonicalUiState::Good ? L"Đã hoàn tất quan sát" : L"Hướng dẫn quan sát an toàn";
    next.reasonText = L"Nếu thấy pin phồng, dấu chất lỏng/ăn mòn, mùi khét hoặc sạc hở lõi: ghi nhận rủi ro và dừng thao tác có thể gây nguy hiểm.";
    next.remainingTasks = { L"CÓ = thấy vấn đề", L"KHÔNG = không thấy vấn đề", L"HỦY = chưa kiểm tra" };
    next.buttonText = overall == CanonicalUiState::Good ? L"TIẾP TỤC: KIỂM TRA CỔNG" : L"MỞ HƯỚNG DẪN 6 ĐIỂM";
    next.isButtonEnabled = true;
    DrawNextActionPanel(dc, action, next, fonts, dpi);
}

void RenderScreenS07_PortsPower(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                int activePort, int focusIndex) {
    (void)activePort; (void)focusIndex;
    const auto& pp = rep.hardware.stress.portPower;
    const auto overall = VerdictState(pp.overall);
    PageHeaderConfig hdr;
    hdr.title = L"Cổng & Nguồn";
    hdr.subtitle = L"PASS chỉ xuất hiện sau phép kích thích vật lý/thiết bị mẫu; controller presence không chứng minh từng cổng hoạt động.";
    hdr.sessionTag = UiLabel(overall);
    hdr.sessionState = overall;
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    const int rightPanelW = UiMetrics::Scale(300, dpi);
    const int leftW = static_cast<int>(r.right - r.left) - UiMetrics::Scale(48, dpi) - rightPanelW;
    const int curY = r.top + UiMetrics::Scale(70, dpi);

    DataTableConfig table;
    table.columns = {
        { L"Cổng", 170, false, false }, { L"Khả năng", 145, false, false },
        { L"Thiết bị / đường dẫn", 210, true, false }, { L"Tốc độ đã chứng minh", 150, false, false },
        { L"Trạng thái", 115, false, true }
    };

    const auto& chassis = rep.hardware.stress.chassisProfile;
    if (!chassis.ports.empty()) {
        for (const auto& definition : chassis.ports) {
            const auto* result = FindPortResult(pp, definition.label);
            CanonicalUiState state = CanonicalUiState::NotTested;
            std::wstring device = L"Chưa kích thích / chưa ghi nhận";
            std::wstring speed = L"Chưa có bằng chứng";
            if (result) {
                state = VerdictState(result->verdict);
                device = PortEvidence(*result);
                speed = result->negotiatedSpeed.empty() ? L"Không suy đoán" : result->negotiatedSpeed;
            }
            table.rows.push_back({ { definition.label,
                                     definition.capability.empty() ? definition.connector : definition.capability,
                                     device, speed, UiLabel(state) }, state });
        }
    } else if (!pp.ports.empty()) {
        for (const auto& result : pp.ports) {
            const auto state = VerdictState(result.verdict);
            table.rows.push_back({ { result.portLabel, DashIfEmpty(result.connectorHint), PortEvidence(result),
                                     result.negotiatedSpeed.empty() ? L"Không suy đoán" : result.negotiatedSpeed,
                                     UiLabel(state) }, state });
        }
    } else {
        table.emptyMessage = L"Chưa có kết quả kích thích cổng và chưa nạp được sơ đồ chassis.";
    }
    RECT tableRect{ r.left + UiMetrics::Scale(24, dpi), curY, r.left + UiMetrics::Scale(24, dpi) + leftW,
                    r.bottom - UiMetrics::Scale(20, dpi) };
    DrawDataTable(dc, tableRect, table, fonts, dpi, 0);

    const int rightX = r.right - rightPanelW - UiMetrics::Scale(24, dpi);
    RECT action{ rightX, curY, r.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(310, dpi) };
    NextActionConfig next;
    next.actionTitle = overall == CanonicalUiState::Good ? L"Đã hoàn tất kiểm tra cổng" : L"Kiểm tra cổng tiếp theo";
    next.reasonText = L"LapSure so sánh baseline/delta PnP và location path. USB4/Thunderbolt/DP Alt Mode chỉ được ghi khi có bằng chứng tương ứng; không tự gán tốc độ.";
    next.remainingTasks = {
        pp.power.acConnected ? L"Nguồn AC: đang kết nối" : L"Nguồn AC: chưa xác nhận đang kết nối",
        pp.power.adapterWatts >= 0.0 ? L"Công suất adapter: có bằng chứng" : L"Công suất adapter: chưa xác định",
        chassis.validationStatus.empty() ? L"Chassis profile: chưa xác định" : L"Chassis profile: " + chassis.validationStatus
    };
    next.buttonText = overall == CanonicalUiState::Good ? L"TIẾP TỤC: XEM BÁO CÁO" : L"KIỂM TRA CỔNG / NGUỒN";
    next.isButtonEnabled = true;
    DrawNextActionPanel(dc, action, next, fonts, dpi);
}

void RenderScreenS08_StressStability(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                     bool running, int elapsedSec, const std::vector<float>& cpuTemps,
                                     const std::vector<float>& gpuTemps, const std::vector<float>& freqs,
                                     const std::vector<float>& powers, const std::vector<LiveLogEntry>& liveLogs,
                                     int focusIndex) {
    (void)cpuTemps; (void)gpuTemps; (void)freqs; (void)powers; (void)liveLogs; (void)focusIndex;
    const auto& stress = rep.hardware.stress;
    CanonicalUiState overall = CanonicalUiState::NotTested;
    if (running) overall = CanonicalUiState::Running;
    else if (stress.completed) overall = VerdictState(stress.decision.stability);
    else if (!stress.stages.empty()) overall = CanonicalUiState::Incomplete;

    PageHeaderConfig hdr;
    hdr.title = L"Stress & Ổn định";
    hdr.subtitle = L"Kết luận ổn định chỉ dựa trên stage đã chạy, event delta và telemetry thực tế của phiên.";
    hdr.sessionTag = running ? L"Đang chạy" : UiLabel(overall);
    hdr.sessionState = overall;
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    const int pad = UiMetrics::Scale(24, dpi);
    const int width = static_cast<int>(r.right - r.left) - pad * 2;
    const int cardGap = UiMetrics::Scale(10, dpi);
    const int cardW = (width - cardGap * 3) / 4;
    const int topY = r.top + UiMetrics::Scale(72, dpi);

    double maxCpuTemp = -1.0, maxGpuTemp = -1.0, maxGpuPower = -1.0;
    unsigned samples = 0;
    for (const auto& stage : stress.stages) {
        maxCpuTemp = std::max(maxCpuTemp, stage.telemetrySummary.maxCpuPackageTempC);
        maxGpuTemp = std::max(maxGpuTemp, stage.telemetrySummary.maxGpuTempC);
        maxGpuPower = std::max(maxGpuPower, stage.telemetrySummary.maxGpuPowerW);
        samples += stage.telemetrySummary.sampleCount;
    }

    MetricCardConfig cards[4];
    cards[0].label = L"Thời gian phiên";
    cards[0].value = elapsedSec > 0 ? std::to_wstring(elapsedSec) + L" s" : L"—";
    cards[0].note = running ? L"Đang đo" : L"Thời gian UI phiên hiện tại";
    cards[0].state = running ? CanonicalUiState::Running : (stress.stages.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Info);
    cards[1].label = L"Nhiệt CPU tối đa";
    cards[1].value = FormatOneDecimal(maxCpuTemp, L" °C");
    cards[1].note = samples ? L"Telemetry đã lấy mẫu" : L"Chưa có mẫu CPU hợp lệ";
    cards[1].state = maxCpuTemp >= 0.0 ? CanonicalUiState::Info : CanonicalUiState::NotTested;
    cards[2].label = L"Nhiệt GPU tối đa";
    cards[2].value = FormatOneDecimal(maxGpuTemp, L" °C");
    cards[2].note = samples ? L"Telemetry đã lấy mẫu" : L"Chưa có mẫu GPU hợp lệ";
    cards[2].state = maxGpuTemp >= 0.0 ? CanonicalUiState::Info : CanonicalUiState::NotTested;
    cards[3].label = L"CPU microbenchmark";
    cards[3].value = stress.cpuBenchmark.score >= 0.0 ? FormatOneDecimal(stress.cpuBenchmark.score, L"") : L"—";
    cards[3].note = stress.cpuBenchmark.verdict;
    cards[3].state = stress.cpuBenchmark.verdict == L"BELOW BASELINE" ? CanonicalUiState::Warning
        : (stress.cpuBenchmark.score >= 0.0 ? CanonicalUiState::Info : CanonicalUiState::NotTested);
    for (int i = 0; i < 4; ++i) {
        const int x = r.left + pad + i * (cardW + cardGap);
        RECT card{ x, topY, x + cardW, topY + UiMetrics::Scale(82, dpi) };
        DrawMetricCard(dc, card, cards[i], fonts, dpi);
    }

    DataTableConfig table;
    table.columns = {
        { L"Stage", 180, false, false }, { L"Thời lượng", 115, false, false },
        { L"Event delta", 230, true, false }, { L"Bằng chứng", 300, true, false },
        { L"Kết quả", 120, false, true }
    };
    for (const auto& stage : stress.stages) {
        CanonicalUiState state = CanonicalUiState::NotTested;
        switch (stage.verdict) {
        case TestVerdict::Pass: state = CanonicalUiState::Good; break;
        case TestVerdict::Warning: state = CanonicalUiState::Warning; break;
        case TestVerdict::Fail: state = CanonicalUiState::Fail; break;
        case TestVerdict::Cancelled: state = CanonicalUiState::Cancelled; break;
        default: state = CanonicalUiState::NotTested; break;
        }
        const std::wstring duration = std::to_wstring(stage.elapsedSeconds) + L" / " + std::to_wstring(stage.plannedSeconds) + L" s";
        const std::wstring events = L"WHEA " + std::to_wstring(stage.newWhea) + L" • Disk " + std::to_wstring(stage.newDisk) +
            L" • NVMe " + std::to_wstring(stage.newNvme) + L" • Display " + std::to_wstring(stage.newDisplay) +
            L" • BugCheck " + std::to_wstring(stage.newBugCheck);
        table.rows.push_back({ { stage.name, duration, events, DashIfEmpty(stage.evidence), UiLabel(state) }, state });
    }
    if (stress.stages.empty()) table.emptyMessage = L"Chưa chạy stage stress nào. Không có kết luận ổn định để hiển thị.";
    RECT tableRect{ r.left + pad, topY + UiMetrics::Scale(94, dpi), r.right - pad, r.bottom - UiMetrics::Scale(20, dpi) };
    DrawDataTable(dc, tableRect, table, fonts, dpi, 0);
}

void RenderScreenS09_BatteryPower(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                  const std::vector<float>& chargeHistory, const std::vector<float>& powerHistory,
                                  int focusIndex) {
    (void)chargeHistory; (void)powerHistory; (void)focusIndex;
    const auto& battery = rep.hardware.battery;
    const auto& power = rep.hardware.stress.portPower.power;
    PageHeaderConfig hdr;
    hdr.title = L"Pin & Năng lượng";
    hdr.subtitle = L"Hiển thị dung lượng đo được và bằng chứng nguồn; không dùng một 'điểm sức khỏe máy' tổng hợp.";
    if (!battery.present) {
        hdr.sessionTag = L"Không có pin / chưa phát hiện";
    } else if (battery.capacityReadable && battery.healthPercent >= 0.0) {
        hdr.sessionTag = L"TÌNH TRẠNG: TỐT (Dung lượng " + FormatOneDecimal(battery.healthPercent, L"%") +
            L" • Chai " + FormatOneDecimal(std::max(0.0, 100.0 - battery.healthPercent), L"%") + L")";
    } else {
        hdr.sessionTag = L"Đã phát hiện pin";
    }
    hdr.sessionState = !battery.present ? CanonicalUiState::Unsupported
        : (battery.capacityReadable ? CanonicalUiState::Info : CanonicalUiState::ProviderUnavailable);
    if (gReturnToAutoAudit) hdr.actionButtonText = L"< Quay lại Bảng";
    DrawPageHeader(dc, r, hdr, fonts, dpi);

    const int pad = UiMetrics::Scale(24, dpi);
    const int width = static_cast<int>(r.right - r.left) - pad * 2;
    const int gap = UiMetrics::Scale(10, dpi);
    const int cardW = (width - gap * 4) / 5;
    const int topY = r.top + UiMetrics::Scale(72, dpi);

    MetricCardConfig cards[5];
    cards[0].label = L"Dung lượng thiết kế";
    cards[0].value = FormatOneDecimal(battery.designWh, L" Wh");
    cards[0].state = battery.capacityReadable ? CanonicalUiState::Info : CanonicalUiState::NotTested;
    cards[1].label = L"Sạc đầy hiện tại";
    cards[1].value = FormatOneDecimal(battery.fullChargeWh, L" Wh");
    cards[1].state = battery.capacityReadable ? CanonicalUiState::Info : CanonicalUiState::NotTested;
    cards[2].label = L"Tỷ lệ dung lượng";
    cards[2].value = battery.healthPercent >= 0.0 ? FormatOneDecimal(battery.healthPercent, L" %") : L"—";
    cards[2].note = L"Full-charge / design; không phải health score toàn máy";
    cards[2].state = battery.healthPercent >= 0.0 ? CanonicalUiState::Info : CanonicalUiState::NotTested;
    cards[3].label = L"Chu kỳ sạc";
    cards[3].value = battery.cycleCount >= 0 ? std::to_wstring(battery.cycleCount) : L"—";
    cards[3].state = battery.cycleCount >= 0 ? CanonicalUiState::Info : CanonicalUiState::NotTested;
    cards[4].label = L"Mức sạc hiện tại";
    cards[4].value = battery.currentChargePercent >= 0 ? std::to_wstring(battery.currentChargePercent) + L" %" : L"—";
    cards[4].state = battery.currentChargePercent >= 0 ? CanonicalUiState::Info : CanonicalUiState::NotTested;
    for (int i = 0; i < 5; ++i) {
        const int x = r.left + pad + i * (cardW + gap);
        RECT card{ x, topY, x + cardW, topY + UiMetrics::Scale(86, dpi) };
        DrawMetricCard(dc, card, cards[i], fonts, dpi);
    }

    DataTableConfig table;
    table.columns = {
        { L"Bằng chứng", 190, false, false }, { L"Giá trị", 260, true, false },
        { L"Nguồn / giới hạn", 360, false, false }, { L"Trạng thái", 130, false, true }
    };
    auto add = [&](const wchar_t* name, const std::wstring& value, const std::wstring& source, CanonicalUiState state) {
        table.rows.push_back({ { name, value, source, UiLabel(state) }, state });
    };
    add(L"Pin hiện diện", battery.present ? L"Có" : L"Không phát hiện",
        L"Windows battery providers", battery.present ? CanonicalUiState::Info : CanonicalUiState::Unsupported);
    add(L"Nhà sản xuất / Serial", DashIfEmpty(battery.manufacturer) + L" / " + DashIfEmpty(battery.serialNumber),
        L"Chỉ hiển thị dữ liệu provider trả về", CanonicalUiState::Info);
    add(L"Trạng thái pin", DashIfEmpty(battery.status), L"Không suy diễn thời lượng sử dụng từ một mẫu ngắn",
        battery.status.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Info);
    add(L"Nguồn AC", power.acConnected ? L"Đang kết nối" : L"Chưa xác nhận đang kết nối",
        DashIfEmpty(power.evidence), power.acConnected ? CanonicalUiState::Info : CanonicalUiState::Incomplete);
    add(L"Nhận diện adapter", DashIfEmpty(power.adapterIdentity), L"Không tự suy đoán công suất từ model",
        power.adapterIdentity.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Info);
    add(L"Công suất adapter", power.adapterWatts >= 0.0 ? FormatOneDecimal(power.adapterWatts, L" W") : L"Chưa xác định",
        power.adapterWatts >= 0.0 ? L"Có bằng chứng đo/nhận diện" : L"Không có provider đủ tin cậy",
        power.adapterWatts >= 0.0 ? CanonicalUiState::Info : CanonicalUiState::NotTested);

    RECT tableRect{ r.left + pad, topY + UiMetrics::Scale(100, dpi), r.right - pad, r.bottom - UiMetrics::Scale(20, dpi) };
    DrawDataTable(dc, tableRect, table, fonts, dpi, 0);
}

} // namespace lap
