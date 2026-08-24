#include "lap/ui_screens.h"
#include <algorithm>
#include <cwctype>
#include <iomanip>
#include <sstream>

namespace lap {
namespace {

std::wstring Dash(const std::wstring& value) { return value.empty() ? L"—" : value; }

std::wstring GiB(uint64_t bytes) {
    if (!bytes) return L"—";
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(1) << (static_cast<double>(bytes) / 1073741824.0) << L" GiB";
    return ss.str();
}

std::wstring GB(uint64_t bytes) {
    if (!bytes) return L"—";
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(1) << (static_cast<double>(bytes) / 1000000000.0) << L" GB";
    return ss.str();
}

std::wstring NumberOrUnknown(long long value, const wchar_t* suffix = L"") {
    return value < 0 ? L"Chưa có dữ liệu" : std::to_wstring(value) + suffix;
}

std::wstring DoubleOrUnknown(double value, const wchar_t* suffix = L"") {
    if (value < 0) return L"Chưa có dữ liệu";
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(1) << value << suffix;
    return ss.str();
}

std::wstring Lower(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return value;
}

bool ContainsAny(const std::wstring& text, std::initializer_list<const wchar_t*> words) {
    const auto lower = Lower(text);
    for (const auto* word : words) if (lower.find(Lower(word)) != std::wstring::npos) return true;
    return false;
}

CanonicalUiState StorageProviderState(const StorageDevice& drive) {
    if ((drive.smartReadable && !drive.smartPassed) || (drive.reliabilityReadable && !drive.reliabilityHealthy))
        return CanonicalUiState::Fail;
    if (!drive.smartReadable && !drive.reliabilityReadable) return CanonicalUiState::ProviderUnavailable;
    if (drive.smartReadable && drive.reliabilityReadable && drive.smartPassed && drive.reliabilityHealthy)
        return CanonicalUiState::Good;
    return CanonicalUiState::Incomplete;
}

const FunctionalItemResult* FindFunctional(const FunctionalTestSummary& summary, const wchar_t* id) {
    for (const auto& item : summary.items) if (item.id == id) return &item;
    return nullptr;
}

CanonicalUiState FunctionalState(const FunctionalTestSummary& summary, const wchar_t* id) {
    const auto* item = FindFunctional(summary, id);
    return item ? MapFunctionalStatus(item->status) : CanonicalUiState::NotTested;
}

std::wstring FunctionalEvidence(const FunctionalTestSummary& summary, const wchar_t* id) {
    const auto* item = FindFunctional(summary, id);
    if (!item) return L"Chưa chạy bài kiểm tra";
    if (!item->evidence.empty()) return item->evidence;
    if (!item->detail.empty()) return item->detail;
    return L"Đã có kết quả nhưng provider không trả thêm chi tiết";
}

CanonicalUiState AggregateRequired(std::initializer_list<CanonicalUiState> states) {
    int pass = 0;
    const int total = static_cast<int>(states.size());
    for (auto state : states) {
        if (state == CanonicalUiState::Fail || state == CanonicalUiState::Error) return CanonicalUiState::Fail;
        if (state == CanonicalUiState::Warning) return CanonicalUiState::Warning;
        if (state == CanonicalUiState::PermissionDenied) return CanonicalUiState::PermissionDenied;
        if (state == CanonicalUiState::ProviderUnavailable) return CanonicalUiState::ProviderUnavailable;
        if (state == CanonicalUiState::Unsupported) return CanonicalUiState::Unsupported;
        if (state == CanonicalUiState::Pass || state == CanonicalUiState::Good) ++pass;
    }
    return total > 0 && pass == total ? CanonicalUiState::Good : CanonicalUiState::Incomplete;
}

CanonicalUiState ValidationState(ValidationStatus status) {
    switch (status) {
    case ValidationStatus::Pass: return CanonicalUiState::Good;
    case ValidationStatus::Warning: return CanonicalUiState::Warning;
    case ValidationStatus::Fail: return CanonicalUiState::Fail;
    default: return CanonicalUiState::NotTested;
    }
}

std::wstring StateLabel(CanonicalUiState state) { return GetStatePresentation(state).label; }

} // namespace

void RenderScreenS10_Storage(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                             int selectedDrive, int tableScrollOffset, int focusIndex) {
    (void)focusIndex;
    const bool hasDrive = !rep.hardware.storage.empty();
    const int index = hasDrive ? std::clamp(selectedDrive, 0, static_cast<int>(rep.hardware.storage.size()) - 1) : 0;
    CanonicalUiState overall = CanonicalUiState::NotTested;
    if (hasDrive) {
        overall = CanonicalUiState::Good;
        for (const auto& drive : rep.hardware.storage) {
            const auto state = StorageProviderState(drive);
            if (state == CanonicalUiState::Fail) { overall = state; break; }
            if (state != CanonicalUiState::Good) overall = CanonicalUiState::Incomplete;
        }
    }

    PageHeaderConfig header;
    header.title = L"Lưu trữ";
    header.subtitle = L"Identity, SMART/NVMe và Windows reliability được trình bày riêng; filesystem sạch không chứng minh SSD khỏe.";
    header.sessionTag = hasDrive ? std::to_wstring(rep.hardware.storage.size()) + L" ổ vật lý" : L"Chưa nhận diện ổ";
    header.sessionState = overall;
    DrawPageHeader(dc, r, header, fonts, dpi);

    const int pad = UiMetrics::Scale(24, dpi);
    const int top = r.top + UiMetrics::Scale(72, dpi);
    const int rightW = UiMetrics::Scale(310, dpi);
    const int leftRight = r.right - rightW - UiMetrics::Scale(34, dpi);

    DataTableConfig driveTable;
    driveTable.columns = {
        { L"Ổ vật lý", 170, false, false }, { L"Serial / Firmware", 180, true, false },
        { L"Dung lượng / Giao tiếp", 150, false, false }, { L"Health evidence", 155, false, true }
    };
    if (!hasDrive) {
        driveTable.emptyMessage = L"Chưa nhận diện được ổ lưu trữ vật lý. Không có trạng thái sức khỏe nào được suy đoán.";
    } else {
        for (const auto& drive : rep.hardware.storage) {
            const auto state = StorageProviderState(drive);
            const std::wstring provider = !drive.reliabilityProvider.empty() ? drive.reliabilityProvider
                : (drive.smartReadable ? L"SMART/NVMe" : L"Thiếu provider health");
            driveTable.rows.push_back({ {
                Dash(drive.model),
                Dash(drive.serialNumber) + L" / FW " + Dash(drive.firmware),
                GB(drive.capacityBytes) + L" / " + Dash(drive.interfaceType),
                StateLabel(state) + L" • " + provider
            }, state });
        }
    }
    RECT drivesRect{ r.left + pad, top, leftRight, top + UiMetrics::Scale(150, dpi) };
    DrawDataTable(dc, drivesRect, driveTable, fonts, dpi, tableScrollOffset);

    DataTableConfig detail;
    detail.columns = {
        { L"Bằng chứng", 175, false, false }, { L"Giá trị thực tế", 220, true, false },
        { L"Giới hạn diễn giải", 270, false, false }, { L"Trạng thái", 120, false, true }
    };
    if (hasDrive) {
        const auto& d = rep.hardware.storage[static_cast<size_t>(index)];
        auto add = [&](const std::wstring& name, const std::wstring& value, const std::wstring& note, CanonicalUiState state) {
            detail.rows.push_back({ { name, value, note, StateLabel(state) }, state });
        };
        add(L"SMART / NVMe", d.smartReadable ? (d.smartPassed ? L"Provider báo healthy" : L"Provider báo lỗi") : L"Không đọc được",
            L"Không đọc được SMART không được chuyển thành PASS",
            !d.smartReadable ? CanonicalUiState::ProviderUnavailable : (d.smartPassed ? CanonicalUiState::Good : CanonicalUiState::Fail));
        add(L"Windows Reliability", d.reliabilityReadable ? (d.reliabilityHealthy ? L"Provider báo healthy" : L"Provider báo lỗi") : L"Không đọc được",
            Dash(d.reliabilityProvider), !d.reliabilityReadable ? CanonicalUiState::ProviderUnavailable
                : (d.reliabilityHealthy ? CanonicalUiState::Good : CanonicalUiState::Fail));
        add(L"NVMe Critical Warning", NumberOrUnknown(d.criticalWarning), L"Chỉ có ý nghĩa khi provider thực sự cung cấp trường này",
            d.criticalWarning < 0 ? CanonicalUiState::NotTested : (d.criticalWarning == 0 ? CanonicalUiState::Info : CanonicalUiState::Fail));
        add(L"Percentage Used", NumberOrUnknown(d.percentageUsed, L" %"), L"Chỉ là endurance metric của drive nếu firmware công bố",
            d.percentageUsed < 0 ? CanonicalUiState::NotTested : CanonicalUiState::Info);
        add(L"Endurance Remaining", NumberOrUnknown(d.enduranceRemaining, L" %"), L"Không biến thành health score toàn máy",
            d.enduranceRemaining < 0 ? CanonicalUiState::NotTested : CanonicalUiState::Info);
        add(L"Nhiệt độ", NumberOrUnknown(d.temperatureC, L" °C"), L"Không có telemetry thì giữ Chưa có dữ liệu",
            d.temperatureC < 0 ? CanonicalUiState::NotTested : CanonicalUiState::Info);
        add(L"Power-on hours", NumberOrUnknown(d.powerOnHours, L" h"), L"Usage evidence, không phải lỗi phần cứng",
            d.powerOnHours < 0 ? CanonicalUiState::NotTested : CanonicalUiState::Info);
        add(L"Unsafe shutdowns", NumberOrUnknown(d.unsafeShutdowns), L"Số lần shutdown bất thường đơn lẻ không chứng minh SSD hỏng",
            d.unsafeShutdowns < 0 ? CanonicalUiState::NotTested : (d.unsafeShutdowns > 0 ? CanonicalUiState::Warning : CanonicalUiState::Info));
        const long long media = std::max(d.mediaErrors, std::max(d.readErrorsUncorrected, d.writeErrorsUncorrected));
        add(L"Media / Uncorrectable", media < 0 ? L"Chưa có dữ liệu" : std::to_wstring(media), L"Confirmed non-zero media/uncorrectable error là bằng chứng nghiêm trọng",
            media < 0 ? CanonicalUiState::NotTested : (media > 0 ? CanonicalUiState::Fail : CanonicalUiState::Good));
        add(L"Dữ liệu đã ghi", DoubleOrUnknown(d.approxDataWrittenTB, L" TB"), L"Usage/endurance context khi provider công bố",
            d.approxDataWrittenTB < 0 ? CanonicalUiState::NotTested : CanonicalUiState::Info);
    } else {
        detail.emptyMessage = L"Chưa có ổ vật lý để hiển thị chi tiết.";
    }
    RECT detailRect{ r.left + pad, drivesRect.bottom + UiMetrics::Scale(10, dpi), leftRight, r.bottom - UiMetrics::Scale(20, dpi) };
    DrawDataTable(dc, detailRect, detail, fonts, dpi, tableScrollOffset);

    const int rightX = leftRight + UiMetrics::Scale(10, dpi);
    RECT rail{ rightX, top, r.right - pad, r.bottom - UiMetrics::Scale(20, dpi) };
    DrawRoundedCard(dc, rail, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, rail.left + UiMetrics::Scale(14, dpi), rail.top + UiMetrics::Scale(12, dpi), L"Filesystem / Volume", 19);

    DataTableConfig fs;
    fs.columns = { { L"Kết quả", 210, true, false }, { L"Trạng thái", 90, false, true } };
    for (const auto& finding : rep.findings) {
        const std::wstring haystack = finding.group + L" " + finding.name + L" " + finding.evidence;
        if (!ContainsAny(haystack, { L"filesystem", L"file system", L"volume", L"dirty bit", L"ntfs" })) continue;
        fs.rows.push_back({ { finding.name + L": " + finding.value, StateLabel(MapState(finding.state)) }, MapState(finding.state) });
    }
    if (fs.rows.empty()) fs.emptyMessage = L"Chưa có bằng chứng integrity của volume. SMART healthy cũng không thay thế kiểm tra filesystem.";
    RECT fsRect{ rail.left + UiMetrics::Scale(10, dpi), rail.top + UiMetrics::Scale(38, dpi),
                 rail.right - UiMetrics::Scale(10, dpi), rail.bottom - UiMetrics::Scale(10, dpi) };
    DrawDataTable(dc, fsRect, fs, fonts, dpi, 0);
}

void RenderScreenS11_Memory(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                            int tableScrollOffset, int focusIndex) {
    (void)focusIndex;
    const StressStageResult* ramStage = nullptr;
    for (const auto& stage : rep.hardware.stress.stages) {
        if (stage.ram.bytesAllocated || stage.ram.bytesTested || stage.ram.mismatches) { ramStage = &stage; break; }
    }

    CanonicalUiState testState = CanonicalUiState::NotTested;
    if (ramStage) {
        if (ramStage->ram.mismatches > 0 || ramStage->verdict == TestVerdict::Fail) testState = CanonicalUiState::Fail;
        else if (ramStage->cancelled) testState = CanonicalUiState::Cancelled;
        else if (ramStage->ram.bytesTested > 0) testState = CanonicalUiState::Warning; // online test remains partial coverage
        else testState = CanonicalUiState::Incomplete;
    }

    PageHeaderConfig header;
    header.title = L"Bộ nhớ (RAM)";
    header.subtitle = L"Topology/module và online integrity test được tách riêng; test online sạch không thay thế preboot memory certification.";
    header.sessionTag = rep.hardware.installedRamBytes ? GiB(rep.hardware.installedRamBytes) : L"Chưa nhận diện RAM";
    header.sessionState = rep.hardware.installedRamBytes ? (testState == CanonicalUiState::Fail ? testState : CanonicalUiState::Info)
                                                       : CanonicalUiState::NotTested;
    DrawPageHeader(dc, r, header, fonts, dpi);

    const int pad = UiMetrics::Scale(24, dpi);
    const int top = r.top + UiMetrics::Scale(72, dpi);
    const int rightW = UiMetrics::Scale(300, dpi);
    const int leftRight = r.right - rightW - UiMetrics::Scale(34, dpi);

    DataTableConfig modules;
    modules.columns = {
        { L"Khe / Bank", 130, false, false }, { L"Dung lượng", 105, false, false },
        { L"Configured / Rated", 150, false, false }, { L"Nhà sản xuất", 135, false, false },
        { L"Part / Serial", 220, true, false }, { L"Trạng thái", 100, false, true }
    };
    if (rep.hardware.memoryModules.empty()) {
        modules.emptyMessage = rep.hardware.installedRamBytes
            ? L"Đã biết tổng dung lượng nhưng provider module/DIMM chưa trả dữ liệu. Không suy đoán số khe hoặc hãng RAM."
            : L"Chưa nhận diện được bộ nhớ hệ thống.";
    } else {
        for (const auto& module : rep.hardware.memoryModules) {
            const auto state = module.capacityBytes ? CanonicalUiState::Info : CanonicalUiState::Incomplete;
            const std::wstring speeds = (module.configuredSpeed ? std::to_wstring(module.configuredSpeed) : L"—") +
                L" / " + (module.ratedSpeed ? std::to_wstring(module.ratedSpeed) : L"—") + L" MHz";
            modules.rows.push_back({ {
                !module.deviceLocator.empty() ? module.deviceLocator : Dash(module.bankLabel),
                GiB(module.capacityBytes), speeds, Dash(module.manufacturer),
                Dash(module.partNumber) + L" / " + Dash(module.serialNumber), StateLabel(state)
            }, state });
        }
    }
    RECT moduleRect{ r.left + pad, top, leftRight, r.bottom - UiMetrics::Scale(20, dpi) };
    DrawDataTable(dc, moduleRect, modules, fonts, dpi, tableScrollOffset);

    const int rightX = leftRight + UiMetrics::Scale(10, dpi);
    RECT rail{ rightX, top, r.right - pad, r.bottom - UiMetrics::Scale(20, dpi) };
    NextActionConfig action;
    action.actionTitle = L"Online RAM integrity";
    action.reasonText = L"LapSure chỉ có thể kiểm tra vùng RAM cấp phát khi Windows đang chạy. Kết quả sạch là bằng chứng partial coverage, không phải chứng nhận preboot toàn bộ RAM.";
    if (ramStage) {
        action.remainingTasks = {
            L"Allocated: " + GiB(ramStage->ram.bytesAllocated),
            L"Bytes tested: " + GiB(ramStage->ram.bytesTested),
            L"Passes: " + std::to_wstring(ramStage->ram.passes),
            L"Mismatches: " + std::to_wstring(ramStage->ram.mismatches),
            L"Elapsed: " + std::to_wstring(ramStage->elapsedSeconds) + L" s • " + StateLabel(testState)
        };
    } else {
        action.remainingTasks = { L"Chưa có online RAM test", L"Không hiển thị 0 lỗi khi test chưa chạy", L"Deep/preboot test là bước riêng nếu cần độ tin cậy cao hơn" };
    }
    action.buttonText = ramStage ? L"XEM BẰNG CHỨNG RAM" : L"CHẠY STRESS / RAM TEST";
    action.isButtonEnabled = true;
    DrawNextActionPanel(dc, rail, action, fonts, dpi);
}

void RenderScreenS13_AudioCamera(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                 int activeTest, int focusIndex) {
    (void)activeTest; (void)focusIndex;
    const auto& functional = rep.hardware.stress.functional;
    const auto camera = FunctionalState(functional, L"camera_function");
    const auto mic = FunctionalState(functional, L"mic_function");
    const auto speaker = FunctionalState(functional, L"speaker_function");
    const auto overall = AggregateRequired({ camera, mic, speaker });

    PageHeaderConfig header;
    header.title = L"Âm thanh & Camera";
    header.subtitle = L"PASS camera cần frame Media Foundation thực; mic cần PCM capture; loa cần stimulus L/R và xác nhận người nghe.";
    header.sessionTag = StateLabel(overall);
    header.sessionState = overall;
    DrawPageHeader(dc, r, header, fonts, dpi);

    const int pad = UiMetrics::Scale(24, dpi);
    const int top = r.top + UiMetrics::Scale(72, dpi);
    const int rightW = UiMetrics::Scale(300, dpi);
    const int leftRight = r.right - rightW - UiMetrics::Scale(34, dpi);

    DataTableConfig table;
    table.columns = {
        { L"Bài kiểm tra", 170, false, false }, { L"Điều kiện PASS", 270, false, false },
        { L"Bằng chứng thực tế", 300, true, false }, { L"Trạng thái", 120, false, true }
    };
    auto add = [&](const wchar_t* title, const wchar_t* condition, const wchar_t* id, CanonicalUiState state) {
        table.rows.push_back({ { title, condition, FunctionalEvidence(functional, id), StateLabel(state) }, state });
    };
    add(L"Camera", L"Media Foundation trả actual frame/sample", L"camera_function", camera);
    add(L"Microphone", L"waveIn thu PCM và có measurement tín hiệu", L"mic_function", mic);
    add(L"Loa trái / phải", L"PCM L/R độc lập + người dùng xác nhận nghe đúng kênh", L"speaker_function", speaker);
    RECT tableRect{ r.left + pad, top, leftRight, r.bottom - UiMetrics::Scale(20, dpi) };
    DrawDataTable(dc, tableRect, table, fonts, dpi, 0);

    const int rightX = leftRight + UiMetrics::Scale(10, dpi);
    RECT rail{ rightX, top, r.right - pad, r.bottom - UiMetrics::Scale(20, dpi) };
    NextActionConfig action;
    action.actionTitle = overall == CanonicalUiState::Good ? L"Đã đủ bằng chứng I/O" : L"Hoàn tất stimulus/capture";
    action.reasonText = L"Enumeration/presence của webcam, audio endpoint hoặc radio không đủ để PASS chức năng. WinPE/OS không hỗ trợ capture phải được ghi KHÔNG HỖ TRỢ, không phải lỗi phần cứng.";
    action.remainingTasks = {
        L"Camera: " + StateLabel(camera),
        L"Microphone: " + StateLabel(mic),
        L"Loa L/R: " + StateLabel(speaker)
    };
    action.buttonText = L"CHẠY CAMERA • MIC • LOA";
    action.isButtonEnabled = true;
    DrawNextActionPanel(dc, rail, action, fonts, dpi);
}

void RenderScreenS15_SystemInfo(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,
                                int tableScrollOffset, int focusIndex) {
    (void)focusIndex;
    PageHeaderConfig header;
    header.title = L"Thông tin Hệ thống";
    header.subtitle = L"Identity, firmware/security, PnP và runtime trust là các lớp bằng chứng riêng; không gộp chúng thành một health score.";
    header.sessionTag = rep.model.empty() ? L"Chưa đủ identity" : rep.model;
    header.sessionState = rep.model.empty() ? CanonicalUiState::Incomplete : CanonicalUiState::Info;
    DrawPageHeader(dc, r, header, fonts, dpi);

    const int pad = UiMetrics::Scale(24, dpi);
    const int top = r.top + UiMetrics::Scale(72, dpi);
    const int rightW = UiMetrics::Scale(340, dpi);
    const int leftRight = r.right - rightW - UiMetrics::Scale(34, dpi);

    DataTableConfig identity;
    identity.columns = {
        { L"Hạng mục", 170, false, false }, { L"Giá trị", 300, true, false },
        { L"Nguồn / diễn giải", 260, false, false }, { L"Trạng thái", 110, false, true }
    };
    auto add = [&](const std::wstring& name, const std::wstring& value, const std::wstring& source, CanonicalUiState state) {
        identity.rows.push_back({ { name, value, source, StateLabel(state) }, state });
    };
    add(L"Model", Dash(rep.model), L"SMBIOS / system identity", rep.model.empty() ? CanonicalUiState::Incomplete : CanonicalUiState::Info);
    add(L"Service Tag / Serial", Dash(rep.serviceTag), L"BIOS/SMBIOS identity", rep.serviceTag.empty() ? CanonicalUiState::Incomplete : CanonicalUiState::Info);
    add(L"Mainboard", Dash(rep.hardware.mainboard.manufacturer) + L" • " + Dash(rep.hardware.mainboard.product),
        L"Không dùng board identity để suy ra sức khỏe", CanonicalUiState::Info);
    add(L"Mainboard Serial", Dash(rep.hardware.mainboard.serialNumber), L"Identity evidence", rep.hardware.mainboard.serialNumber.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Info);
    add(L"BIOS", Dash(rep.hardware.bios.vendor) + L" • " + Dash(rep.hardware.bios.version),
        L"Không gắn nhãn 'BIOS cũ' nếu không có nguồn OEM authoritative", rep.hardware.bios.version.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Info);
    add(L"SMBIOS", Dash(rep.hardware.bios.smbiosVersion), L"Firmware metadata", rep.hardware.bios.smbiosVersion.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Info);
    add(L"Môi trường", Dash(rep.environment), L"Windows/WinPE capability context", rep.environment.empty() ? CanonicalUiState::NotTested : CanonicalUiState::Info);
    add(L"TPM", rep.hardware.security.tpmPresent ? (rep.hardware.security.tpmReady ? L"Có • Ready" : L"Có • Chưa Ready") : L"Không phát hiện",
        L"Security posture, không phải hardware-health score", rep.hardware.security.tpmPresent ? CanonicalUiState::Info : CanonicalUiState::Warning);
    const auto sbState = rep.hardware.security.secureBootKnown ? CanonicalUiState::Info : CanonicalUiState::NotTested;
    add(L"Secure Boot", !rep.hardware.security.secureBootKnown ? L"Không xác định" : (rep.hardware.security.secureBootEnabled ? L"Bật" : L"Tắt"),
        L"Security configuration; disabled không tự động đồng nghĩa phần cứng lỗi", sbState);

    RECT identityRect{ r.left + pad, top, leftRight, r.bottom - UiMetrics::Scale(20, dpi) };
    DrawDataTable(dc, identityRect, identity, fonts, dpi, tableScrollOffset);

    const int rightX = leftRight + UiMetrics::Scale(10, dpi);
    RECT rail{ rightX, top, r.right - pad, r.bottom - UiMetrics::Scale(20, dpi) };
    DrawRoundedCard(dc, rail, UiMetrics::RadiusMd, UiColors::CardBg, UiColors::CardBorder, 1);
    SelectObject(dc, fonts.hBodyBold); SetTextColor(dc, UiColors::TextMain);
    TextOutW(dc, rail.left + UiMetrics::Scale(12, dpi), rail.top + UiMetrics::Scale(10, dpi), L"PnP & Runtime Trust", 19);

    DataTableConfig diagnostics;
    diagnostics.columns = { { L"Bằng chứng", 205, true, false }, { L"Trạng thái", 105, false, true } };
    for (const auto& dev : rep.hardware.pnpProblems) {
        diagnostics.rows.push_back({ {
            Dash(dev.friendlyName.empty() ? dev.deviceDesc : dev.friendlyName) + L" • Code " + std::to_wstring(dev.problemCode) +
                (dev.problemDescription.empty() ? L"" : L" • " + dev.problemDescription),
            L"CẦN XEM"
        }, CanonicalUiState::Warning });
    }
    for (const auto& check : rep.hardware.stress.runtimeValidation.checks) {
        const auto state = ValidationState(check.status);
        diagnostics.rows.push_back({ {
            check.name + (check.detail.empty() ? L"" : L": " + check.detail), StateLabel(state)
        }, state });
    }
    if (diagnostics.rows.empty()) {
        diagnostics.emptyMessage = L"Chưa có PnP problem hoặc runtime validation evidence. Code 43 nếu xuất hiện chỉ là triệu chứng PnP, không được gán nguyên nhân chung.";
    }
    RECT diagRect{ rail.left + UiMetrics::Scale(10, dpi), rail.top + UiMetrics::Scale(36, dpi),
                   rail.right - UiMetrics::Scale(10, dpi), rail.bottom - UiMetrics::Scale(10, dpi) };
    DrawDataTable(dc, diagRect, diagnostics, fonts, dpi, tableScrollOffset);
}

} // namespace lap
