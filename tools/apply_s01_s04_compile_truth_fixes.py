from pathlib import Path

UI = Path("src/ui_screens_s01_s04_v2.cpp")
MAIN = Path("src/main.cpp")
ui = UI.read_text(encoding="utf-8")
main = MAIN.read_text(encoding="utf-8")


def once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected 1 match, found {count}")
    return text.replace(old, new, 1)

# Win32 RECT uses LONG while UiMetrics returns int. Make template types explicit for strict MSVC.
ui = once(ui,
'''    const int rightPanelW = std::clamp((r.right - r.left) * 26 / 100,
                                       UiMetrics::Scale(260, dpi), UiMetrics::Scale(340, dpi));
''',
'''    const int rightPanelW = std::clamp<int>(static_cast<int>((r.right - r.left) * 26 / 100),
                                            UiMetrics::Scale(260, dpi), UiMetrics::Scale(340, dpi));
''', "right panel clamp")
ui = once(ui,
'''    const int availableRows = std::max(UiMetrics::Scale(306, dpi), listCard.bottom - listCard.top - UiMetrics::Scale(12, dpi));
''',
'''    const int availableRows = std::max<int>(UiMetrics::Scale(306, dpi),
        static_cast<int>(listCard.bottom - listCard.top) - UiMetrics::Scale(12, dpi));
''', "available rows max")
ui = once(ui,
'''        const int titleW = std::max(UiMetrics::Scale(185, dpi), (row.right - row.left) * 31 / 100);
''',
'''        const int titleW = std::max<int>(UiMetrics::Scale(185, dpi),
            static_cast<int>((row.right - row.left) * 31 / 100));
''', "title width max")

# Stage 2 currently only identifies CPU. The benchmark is actually executed by RunStressSession at stage 9.
old_case2 = '''    case 2:
        out.source = rep.hardware.stress.cpuBenchmark.baselineSource.empty()
            ? L"CPU identity + BuiltIn-FP-Mix-v1" : rep.hardware.stress.cpuBenchmark.baselineSource;
        out.detail = L"Danh tính CPU và microbenchmark có version/baseline";
        if (completed) {
            if (rep.hardware.cpuName.empty()) {
                out.state = CanonicalUiState::Incomplete;
                out.label = L"THIẾU DỮ LIỆU";
            } else if (rep.hardware.stress.cpuBenchmark.verdict == L"BELOW BASELINE") {
                out.state = CanonicalUiState::Warning;
                out.label = L"DƯỚI BASELINE";
            } else {
                out.state = CanonicalUiState::Info;
                out.label = rep.hardware.stress.cpuBenchmark.verdict == L"NOT SCORED"
                    ? L"ĐÃ ĐO — CHƯA CÓ BASELINE" : L"ĐÃ ĐO";
            }
        }
        break;
'''
new_case2 = '''    case 2:
        out.source = L"CPU inventory / CIM / processor identity";
        out.detail = L"Nhận diện CPU; microbenchmark chỉ chạy trong bước Stress & Ổn định";
        if (completed) {
            if (rep.hardware.cpuName.empty()) {
                out.state = CanonicalUiState::Incomplete;
                out.label = L"THIẾU DỮ LIỆU";
            } else {
                out.state = CanonicalUiState::Info;
                out.label = L"ĐÃ NHẬN DIỆN";
            }
        }
        break;
'''
ui = once(ui, old_case2, new_case2, "stage 2 truth")
ui = once(ui,
'''        out.source = L"LapSure stress journal + event delta + telemetry";
        out.detail = L"Stress CPU/RAM/GPU và đánh giá ổn định theo bằng chứng phát sinh";
''',
'''        out.source = rep.hardware.stress.cpuBenchmark.baselineSource.empty()
            ? L"Stress journal + event delta + telemetry + BuiltIn-FP-Mix-v1"
            : L"Stress journal + telemetry + " + rep.hardware.stress.cpuBenchmark.baselineSource;
        out.detail = L"Stress CPU/RAM/GPU; microbenchmark CPU được chạy và ghi bằng chứng tại bước này";
''', "stage 9 benchmark truth")
ui = ui.replace('case 2: return L"CPU & Microbench";', 'case 2: return L"CPU & Nhận diện";')
ui = ui.replace('L"Nhận diện hệ thống", L"CPU & Microbench", L"Bộ nhớ (RAM)"',
                'L"Nhận diện hệ thống", L"CPU & Nhận diện", L"Bộ nhớ (RAM)"')

# Main runtime stage names/logs must say exactly what is executed at that moment.
main = main.replace('case 2: return L"CPU & Microbench";', 'case 2: return L"CPU & Nhận diện";')
main = once(main,
'''        PostStatus(h, L"Đang đọc thông tin CPU và vi điểm chuẩn...");
        syncToGlobal(report);
        gAuditCompletedItems = 2;
        PostStatus(h, L"Đã nhận diện CPU: " + report.hardware.cpuName);
''',
'''        PostStatus(h, L"Đang thu thập danh tính và thông tin CPU...");
        syncToGlobal(report);
        gAuditCompletedItems = 2;
        PostStatus(h, report.hardware.cpuName.empty()
            ? L"Đã hoàn tất thu thập CPU; chưa xác định được tên bộ xử lý."
            : L"Đã nhận diện CPU: " + report.hardware.cpuName);
''', "stage 2 logs")
main = once(main,
'''        PostStatus(h, L"Đang kiểm tra kết nối Wi-Fi, Bluetooth và cổng cắm...");
        CollectFunctionalPresence(report, caps, &gCancel);
        report.hardware.stress.chassisProfile = LoadChassisProfile(gDir, report.model);
        syncToGlobal(report);
        gAuditCompletedItems = 7;
        PostStatus(h, L"Hoàn tất kiểm tra Mạng & Kết nối");
''',
'''        PostStatus(h, L"Đang nhận diện adapter Wi-Fi, Bluetooth, Ethernet và controller kết nối...");
        CollectFunctionalPresence(report, caps, &gCancel);
        report.hardware.stress.chassisProfile = LoadChassisProfile(gDir, report.model);
        syncToGlobal(report);
        gAuditCompletedItems = 7;
        PostStatus(h, L"Đã thu thập nhận diện Mạng & Kết nối; chức năng thực tế vẫn cần kiểm tra riêng.");
''', "stage 7 logs")
main = once(main,
'''    PostStatus(h, L"Đã nhận diện hệ thống: " + (report.model.empty() ? L"Thành công" : report.model));
''',
'''    PostStatus(h, report.model.empty()
        ? L"Đã hoàn tất thu thập nhận diện; model hệ thống chưa xác định."
        : L"Đã nhận diện hệ thống: " + report.model);
''', "stage 1 log")

# S02 preflight was the remaining compile reference to removed heuristic engine count.
# Keep it conservative instead of restoring an invented readiness score/profile count.
main = once(main,
'''    drawPreflightRow(L"Bộ công cụ chẩn đoán", (GetReadyEngineCount() >= 12) ? CanonicalUiState::Pass : CanonicalUiState::Warning, L"WMI, CIM, SetupAPI, DirectX sẵn sàng");
    drawPreflightRow(L"Cơ sở dữ liệu Chassis", CanonicalUiState::Pass, L"18+ Chassis profiles & CPU baselines");
''',
'''    drawPreflightRow(L"Provider hệ thống", caps.wmi ? CanonicalUiState::Info : CanonicalUiState::Warning,
                     caps.wmi ? L"WMI/CIM khả dụng; provider tùy chọn được xác minh khi chạy"
                              : L"WMI/CIM không khả dụng; một số bằng chứng có thể bị thiếu");
    drawPreflightRow(L"Hồ sơ Chassis", CanonicalUiState::Info,
                     L"Hồ sơ phù hợp sẽ được nạp theo model/Service Tag khi kiểm định");
''', "S02 heuristic readiness")

UI.write_text(ui, encoding="utf-8")
MAIN.write_text(main, encoding="utf-8")
print("Applied strict-MSVC and runtime-stage truthfulness fixes")
