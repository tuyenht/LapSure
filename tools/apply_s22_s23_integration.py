from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def replace_one(path: str, old: str, new: str, label: str):
    p = ROOT / path
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {count}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8")

# 1. Model: stable session identity shared by stress, reports and history.
replace_one(
    "include/lap/model.h",
    'struct StressSession {\n    std::wstring mode{L"Quick"};',
    'struct StressSession {\n    std::wstring sessionId;\n    std::wstring mode{L"Quick"};',
    "StressSession sessionId",
)

# 2. Stress engine: persist the real generated ID in the report model.
replace_one(
    "src/stress.cpp",
    'auto sessionId=SessionId();WriteStressJournal(appDir,sessionId,L"SESSION",L"RUNNING");',
    'ss.sessionId=SessionId();const auto& sessionId=ss.sessionId;WriteStressJournal(appDir,sessionId,L"SESSION",L"RUNNING");',
    "stress sessionId persistence",
)

# 3. Report writer: stable filenames, history upsert, and truthful battery wording.
p = ROOT / "src/report.cpp"
text = p.read_text(encoding="utf-8")
if '#include "lap/session_history.h"' not in text:
    text = text.replace('#include "lap/chassis_profile.h"\n', '#include "lap/chassis_profile.h"\n#include "lap/session_history.h"\n', 1)
if 'std::wstring ReportStem(const AuditReport&r)' not in text:
    marker = 'std::wstring Ts(){SYSTEMTIME t;GetLocalTime(&t);wchar_t b[64];swprintf_s(b,L"%04d%02d%02d_%02d%02d%02d",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond);return b;}\n'
    if marker not in text:
        raise SystemExit("report Ts marker not found")
    text = text.replace(marker, marker + 'std::wstring ReportStem(const AuditReport&r){return r.hardware.stress.sessionId.empty()?Ts():r.hardware.stress.sessionId;}\n', 1)
text = text.replace('auto p=std::filesystem::path(dir)/(L"audit_"+Ts()+L".html");', 'auto p=std::filesystem::path(dir)/(L"audit_"+ReportStem(r)+L".html");', 1)
text = text.replace('auto p=std::filesystem::path(dir)/(L"audit_"+Ts()+L".json");', 'auto p=std::filesystem::path(dir)/(L"audit_"+ReportStem(r)+L".json");', 1)
text = text.replace('f<<L"<div class=\'card\'><b>Pin</b><div class=\'metric\'>"<<F(r.hardware.battery.healthPercent)<<L"% sức khỏe</div><div>Dung lượng thiết kế "<<F(r.hardware.battery.designWh)<<L" Wh · Hiện còn "<<F(r.hardware.battery.fullChargeWh)<<L" Wh · Hao mòn "<<F(r.hardware.battery.wearPercent)<<L"%</div></div>";',
                    'f<<L"<div class=\'card\'><b>Pin</b><div class=\'metric\'>"<<(r.hardware.battery.capacityReadable?F(r.hardware.battery.healthPercent)+L"% Full-charge / Design":L"Chưa đọc được tỷ lệ dung lượng")<<L"</div><div>Dung lượng thiết kế "<<F(r.hardware.battery.designWh)<<L" Wh · Full-charge "<<F(r.hardware.battery.fullChargeWh)<<L" Wh · Hao mòn ước tính "<<F(r.hardware.battery.wearPercent)<<L"%</div></div>";', 1)
old_html_return = 'f<<L"</table></details></main></body></html>";return WriteUtf8File(p,f.str())?p.wstring():L"";'
new_html_return = 'f<<L"</table></details></main></body></html>";if(!WriteUtf8File(p,f.str()))return L"";RecordSessionHistoryArtifact(r,p.wstring(),true);return p.wstring();'
if text.count(old_html_return) != 1:
    raise SystemExit(f"HTML return marker count={text.count(old_html_return)}")
text = text.replace(old_html_return, new_html_return, 1)
old_json_return = 'f<<L"]}\\n";return WriteUtf8File(p,f.str())?p.wstring():L"";'
new_json_return = 'f<<L"]}\\n";if(!WriteUtf8File(p,f.str()))return L"";RecordSessionHistoryArtifact(r,p.wstring(),false);return p.wstring();'
if text.count(old_json_return) != 1:
    raise SystemExit(f"JSON return marker count={text.count(old_json_return)}")
text = text.replace(old_json_return, new_json_return, 1)
# Include session ID in JSON top level for provenance.
json_start = 'f<<L"{\\n\\\"model\\\":\\\""<<Json(r.model)'
if json_start not in text:
    raise SystemExit("JSON start marker not found")
text = text.replace(json_start, 'f<<L"{\\n\\\"sessionId\\\":\\\""<<Json(r.hardware.stress.sessionId)<<L"\\\",\\\"model\\\":\\\""<<Json(r.model)', 1)
p.write_text(text, encoding="utf-8")

# 4. Header: selected history row is runtime state, not fake static selection.
replace_one(
    "include/lap/ui_screens.h",
    'void RenderScreenS22_SessionHistory(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,\n                                    int tableScrollOffset = 0, int focusIndex = 0);',
    'void RenderScreenS22_SessionHistory(HDC dc, const RECT& r, const AuditReport& rep, const UiFonts& fonts, int dpi,\n                                    int tableScrollOffset = 0, int selectedIndex = 0, int focusIndex = 0);',
    "S22 selected index signature",
)

# 5. CMake: compile persistence and S22/S23 production renderers; route legacy symbols.
p = ROOT / "CMakeLists.txt"
text = p.read_text(encoding="utf-8")
text = text.replace('src/ui_screens_s16_s21_v2.cpp src/process.cpp', 'src/ui_screens_s16_s21_v2.cpp src/ui_screens_s22_s23_v2.cpp src/session_history.cpp src/process.cpp', 1)
legacy_tail = 'RenderScreenS21_Settings=RenderScreenS21_Settings_Legacy"'
if legacy_tail not in text:
    raise SystemExit("CMake legacy S21 tail not found")
text = text.replace(legacy_tail, 'RenderScreenS21_Settings=RenderScreenS21_Settings_Legacy;RenderScreenS22_SessionHistory=RenderScreenS22_SessionHistory_Legacy;RenderScreenS23_InterruptedRecovery=RenderScreenS23_InterruptedRecovery_Legacy"', 1)
# Behavioral tests compile report.cpp, which now depends on the history implementation.
text = text.replace('src/journal.cpp src/baseline.cpp src/hardware.cpp)', 'src/journal.cpp src/session_history.cpp src/baseline.cpp src/hardware.cpp)', 1)
p.write_text(text, encoding="utf-8")

# 6. Main: headers, cached report directory/history state, startup recovery snapshot, report-save reuse, S22/S23 actions.
p = ROOT / "src/main.cpp"
text = p.read_text(encoding="utf-8")
if '#include "lap/session_history.h"' not in text:
    text = text.replace('#include "lap/report.h"\n', '#include "lap/report.h"\n#include "lap/session_history.h"\n#include "lap/journal.h"\n', 1)
text = text.replace('std::wstring gDir, gReportPath;', 'std::wstring gDir, gReportPath, gReportOutputDir;', 1)
text = text.replace('int gFocusIndex = 0; // 0: Sidebar, 1: Mode Pills, 2: Primary CTA, 3: Next CTA', 'int gFocusIndex = 0; // 0: Sidebar, 1: Mode Pills, 2: Primary CTA, 3: Next CTA\nint gHistorySelectedIndex = 0;', 1)

# Avoid provider probing on each manual re-save; startup caches the writable report directory.
old_rebuild = '''    auto caps = DetectCapabilities(gDir);\n    auto out = ResolveReportDirectory(gDir, caps.winPE);\n    gReportPath = SaveHtmlReport(gReport, out);\n    SaveJsonReport(gReport, out);'''
new_rebuild = '''    auto out = gReportOutputDir;\n    if (out.empty()) {\n        auto caps = DetectCapabilities(gDir);\n        out = ResolveReportDirectory(gDir, caps.winPE);\n        gReportOutputDir = out;\n        InitializeSessionHistory(out);\n    }\n    gReportPath = SaveHtmlReport(gReport, out);\n    SaveJsonReport(gReport, out);'''
if text.count(old_rebuild) != 1:
    raise SystemExit(f"RebuildDecisionAndReports marker count={text.count(old_rebuild)}")
text = text.replace(old_rebuild, new_rebuild, 1)

# Paint passes real selected row state.
old_s22 = 'RenderScreenS22_SessionHistory(memDC, layout.contentRect, repSnapshot, gFonts, dpi, gTableScrollOffset, gFocusIndex);'
new_s22 = 'RenderScreenS22_SessionHistory(memDC, layout.contentRect, repSnapshot, gFonts, dpi, gTableScrollOffset, gHistorySelectedIndex, gFocusIndex);'
if text.count(old_s22) != 1:
    raise SystemExit(f"S22 paint call count={text.count(old_s22)}")
text = text.replace(old_s22, new_s22, 1)

# Startup cache/history + interrupted journal snapshot, only for interactive GUI after CLI exits.
startup_marker = '''    if (inventoryOnly) {\n        try { return RunInventoryOnly(outputDir); }\n        catch (...) { return 3; }\n    }\n    \n    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);'''
startup_new = '''    if (inventoryOnly) {\n        try { return RunInventoryOnly(outputDir); }\n        catch (...) { return 3; }\n    }\n\n    {\n        auto startupCaps = DetectCapabilities(gDir);\n        gReportOutputDir = ResolveReportDirectory(gDir, startupCaps.winPE);\n        InitializeSessionHistory(gReportOutputDir);\n        const auto interrupted = ReadInterruptedStressJournal(gDir);\n        if (interrupted.present) {\n            std::lock_guard<std::mutex> lk(gReportMutex);\n            gReport.hardware.stress.previousInterruptedSessionDetected = true;\n            gReport.hardware.stress.journalPath = interrupted.journalPath;\n            gReport.findings.push_back({L"Stability", L"Previous interrupted stress session", interrupted.rawEvidence,\n                L"No abandoned RUNNING journal", State::Warning, Severity::Critical,\n                L"Crash/reboot/interruption evidence; not proof of hardware failure.", Dimension::Health});\n        }\n    }\n    \n    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);'''
if text.count(startup_marker) != 1:
    raise SystemExit(f"GUI startup marker count={text.count(startup_marker)}")
text = text.replace(startup_marker, startup_new, 1)

# Replace old one-button S23 hit test with production S22 history + S23 three-action semantics.
old_actions = '''        // 16. Interrupted Recovery Screen: Restart Button Hit-Test\n        if (gCurrentTab == MainTab::InterruptedRecovery) {\n            int rightPanelW = UiMetrics::Scale(300, dpi);\n            int rightX = cr.right - rightPanelW - UiMetrics::Scale(24, dpi);\n            int curY = layout.contentRect.top + UiMetrics::Scale(70, dpi);\n            RECT actionCard{ rightX, curY, cr.right - UiMetrics::Scale(24, dpi), curY + UiMetrics::Scale(310, dpi) };\n            int actH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);\n            RECT br{ actionCard.left + UiMetrics::Scale(14, dpi), actionCard.bottom - actH - UiMetrics::Scale(12, dpi), actionCard.right - UiMetrics::Scale(14, dpi), actionCard.bottom - UiMetrics::Scale(12, dpi) };\n            if (x >= br.left && x <= br.right && y >= br.top && y <= br.bottom) {\n                StartAudit(h);\n                return 0;\n            }\n        }'''
new_actions = '''        // 16. Session History: select/open/delete actual local report records.\n        if (gCurrentTab == MainTab::SessionHistory) {\n            auto history = GetSessionHistorySnapshot();\n            int rightW = UiMetrics::Scale(300, dpi);\n            int gap2 = UiMetrics::Scale(12, dpi);\n            RECT body{layout.contentRect.left + UiMetrics::Scale(24, dpi), layout.contentRect.top + UiMetrics::Scale(70, dpi),\n                      layout.contentRect.right - UiMetrics::Scale(24, dpi), layout.contentRect.bottom - UiMetrics::Scale(20, dpi)};\n            RECT tableRect{body.left, body.top, body.right - rightW - gap2, body.bottom};\n            int rowH = UiMetrics::Scale(UiMetrics::TableRowHeight, dpi);\n            if (!history.empty() && x >= tableRect.left && x <= tableRect.right && y >= tableRect.top + rowH && y < tableRect.bottom) {\n                int visibleRow = (y - (tableRect.top + rowH)) / rowH;\n                int idx = gTableScrollOffset + visibleRow;\n                if (idx >= 0 && idx < static_cast<int>(history.size())) {\n                    gHistorySelectedIndex = idx;\n                    InvalidateRect(h, nullptr, FALSE);\n                    return 0;\n                }\n            }\n            if (!history.empty()) {\n                int idx = std::clamp(gHistorySelectedIndex, 0, static_cast<int>(history.size()) - 1);\n                const auto& selected = history[static_cast<size_t>(idx)];\n                RECT detail{tableRect.right + gap2, body.top, body.right, body.bottom};\n                int btnH2 = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);\n                RECT openBtn{detail.left + 14, detail.bottom - btnH2 * 2 - UiMetrics::Scale(24, dpi), detail.right - 14, detail.bottom - btnH2 - UiMetrics::Scale(18, dpi)};\n                RECT deleteBtn{detail.left + 14, detail.bottom - btnH2 - UiMetrics::Scale(10, dpi), detail.right - 14, detail.bottom - UiMetrics::Scale(10, dpi)};\n                if (x >= openBtn.left && x <= openBtn.right && y >= openBtn.top && y <= openBtn.bottom) {\n                    const std::wstring path = !selected.htmlPath.empty() ? selected.htmlPath : (!selected.jsonPath.empty() ? selected.jsonPath : selected.evidencePath);\n                    if (!path.empty()) ShellExecuteW(h, L"open", path.c_str(), nullptr, nullptr, SW_SHOW);\n                    else MessageBoxW(h, L"Phiên này chưa có file report/evidence có thể mở.", L"LapSure", MB_OK | MB_ICONINFORMATION);\n                    return 0;\n                }\n                if (x >= deleteBtn.left && x <= deleteBtn.right && y >= deleteBtn.top && y <= deleteBtn.bottom) {\n                    int answer = MessageBoxW(h,\n                        L"YES: xóa mục lịch sử VÀ các file report/evidence.\\nNO: chỉ xóa mục khỏi index, giữ nguyên file.\\nCANCEL: không thay đổi.",\n                        L"Xóa phiên kiểm định", MB_YESNOCANCEL | MB_ICONWARNING);\n                    if (answer == IDYES || answer == IDNO) {\n                        DeleteSessionHistoryEntry(selected.sessionId, answer == IDYES);\n                        auto after = GetSessionHistorySnapshot();\n                        gHistorySelectedIndex = after.empty() ? 0 : std::min(gHistorySelectedIndex, static_cast<int>(after.size()) - 1);\n                        InvalidateRect(h, nullptr, FALSE);\n                    }\n                    return 0;\n                }\n            }\n        }\n\n        // 17. Interrupted Recovery: preserve/discard real journal; interruption never becomes PASS.\n        if (gCurrentTab == MainTab::InterruptedRecovery) {\n            RECT body{layout.contentRect.left + UiMetrics::Scale(24, dpi), layout.contentRect.top + UiMetrics::Scale(70, dpi),\n                      layout.contentRect.right - UiMetrics::Scale(24, dpi), layout.contentRect.bottom - UiMetrics::Scale(20, dpi)};\n            int rightW = UiMetrics::Scale(330, dpi);\n            RECT actions{body.right - rightW, body.top, body.right, body.bottom};\n            int btnH2 = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);\n            int btnGap2 = UiMetrics::Scale(10, dpi);\n            int actionY = actions.bottom - (btnH2 * 3 + btnGap2 * 2 + UiMetrics::Scale(14, dpi));\n            RECT recover{actions.left + 14, actionY, actions.right - 14, actionY + btnH2};\n            RECT closeIncomplete{actions.left + 14, recover.bottom + btnGap2, actions.right - 14, recover.bottom + btnGap2 + btnH2};\n            RECT discard{actions.left + 14, closeIncomplete.bottom + btnGap2, actions.right - 14, closeIncomplete.bottom + btnGap2 + btnH2};\n            auto clearInterrupted = [&] {\n                std::lock_guard<std::mutex> lk(gReportMutex);\n                gReport.hardware.stress.previousInterruptedSessionDetected = false;\n                gReport.hardware.stress.journalPath.clear();\n                gReport.findings.erase(std::remove_if(gReport.findings.begin(), gReport.findings.end(), [](const Finding& f) {\n                    return f.name == L"Previous interrupted stress session";\n                }), gReport.findings.end());\n            };\n            if (x >= recover.left && x <= recover.right && y >= recover.top && y <= recover.bottom) {\n                if (!ArchiveInterruptedSession(gDir, gReportOutputDir)) {\n                    MessageBoxW(h, L"Không thể lưu journal gián đoạn vào lịch sử. LapSure sẽ không xóa bằng chứng gốc.", L"LapSure", MB_OK | MB_ICONERROR);\n                    return 0;\n                }\n                clearInterrupted();\n                StartAudit(h);\n                return 0;\n            }\n            if (x >= closeIncomplete.left && x <= closeIncomplete.right && y >= closeIncomplete.top && y <= closeIncomplete.bottom) {\n                if (!ArchiveInterruptedSession(gDir, gReportOutputDir)) {\n                    MessageBoxW(h, L"Không thể lưu journal gián đoạn. Phiên chưa được đóng.", L"LapSure", MB_OK | MB_ICONERROR);\n                    return 0;\n                }\n                clearInterrupted();\n                InvalidateRect(h, nullptr, FALSE);\n                return 0;\n            }\n            if (x >= discard.left && x <= discard.right && y >= discard.top && y <= discard.bottom) {\n                int answer = MessageBoxW(h, L"Bỏ journal sẽ xóa bằng chứng gián đoạn hiện tại. Thao tác này không tạo PASS. Bạn chắc chắn muốn tiếp tục?", L"Bỏ journal gián đoạn", MB_YESNO | MB_ICONWARNING);\n                if (answer == IDYES) {\n                    if (DiscardInterruptedStressJournal(gDir)) { clearInterrupted(); InvalidateRect(h, nullptr, FALSE); }\n                    else MessageBoxW(h, L"Không thể xóa journal.", L"LapSure", MB_OK | MB_ICONERROR);\n                }\n                return 0;\n            }\n        }'''
if text.count(old_actions) != 1:
    raise SystemExit(f"S23 old hit-test block count={text.count(old_actions)}")
text = text.replace(old_actions, new_actions, 1)
p.write_text(text, encoding="utf-8")

print("Applied S22/S23 model/report/runtime integration")
