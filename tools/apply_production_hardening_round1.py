from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


def write(rel, text):
    (ROOT / rel).write_text(text, encoding="utf-8", newline="\n")


def replace_once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match, got {count}")
    return text.replace(old, new, 1)


# ---------------------------------------------------------------------------
# C10: one canonical button geometry function for both painting and hit testing.
# ---------------------------------------------------------------------------
h = read("include/lap/ui_components.h")
h = replace_once(
    h,
    "void DrawNextActionPanel(HDC dc, const RECT& r, const NextActionConfig& config, const UiFonts& fonts, int dpi);",
    "RECT GetNextActionButtonRect(const RECT& panelRect, int dpi);\nvoid DrawNextActionPanel(HDC dc, const RECT& r, const NextActionConfig& config, const UiFonts& fonts, int dpi);",
    "declare C10 button geometry",
)
write("include/lap/ui_components.h", h)

cpp = read("src/ui_components.cpp")
marker = "// C10 — Next Action Panel\n// ============================================================\n\n"
insert = marker + "RECT GetNextActionButtonRect(const RECT& panelRect, int dpi) {\n    const int btnH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);\n    return RECT{\n        panelRect.left + UiMetrics::Scale(14, dpi),\n        panelRect.bottom - btnH - UiMetrics::Scale(12, dpi),\n        panelRect.right - UiMetrics::Scale(14, dpi),\n        panelRect.bottom - UiMetrics::Scale(12, dpi)\n    };\n}\n\n"
cpp = replace_once(cpp, marker, insert, "implement C10 button geometry")
cpp = replace_once(
    cpp,
    "        int btnH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);\n        RECT br{ r.left + UiMetrics::Scale(14, dpi), r.bottom - btnH - UiMetrics::Scale(12, dpi), r.right - UiMetrics::Scale(14, dpi), r.bottom - UiMetrics::Scale(12, dpi) };",
    "        RECT br = GetNextActionButtonRect(r, dpi);",
    "use C10 geometry in painter",
)
write("src/ui_components.cpp", cpp)

# ---------------------------------------------------------------------------
# Session history: never trust persisted artifact paths merely because they are
# local. Canonicalize, constrain to report root, extension allowlist and real file.
# ---------------------------------------------------------------------------
h = read("include/lap/session_history.h")
h = replace_once(
    h,
    "std::vector<SessionHistoryEntry> GetSessionHistorySnapshot();\n",
    "std::vector<SessionHistoryEntry> GetSessionHistorySnapshot();\nbool IsTrustedSessionArtifactPath(const std::wstring& artifactPath);\n",
    "declare trusted history path check",
)
write("include/lap/session_history.h", h)

hist = read("src/session_history.cpp")
anchor = "std::wstring FallbackSessionId(const std::wstring& artifactPath) {"
helpers = r'''std::wstring LowerPath(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    return s;
}

bool IsTrustedArtifactPathLocked(const std::wstring& artifactPath) {
    if (artifactPath.empty() || gHistoryDir.empty()) return false;
    std::error_code ec;
    auto root = std::filesystem::weakly_canonical(std::filesystem::path(gHistoryDir), ec);
    if (ec || root.empty()) return false;
    ec.clear();
    auto candidate = std::filesystem::weakly_canonical(std::filesystem::path(artifactPath), ec);
    if (ec || candidate.empty() || candidate == root) return false;

    ec.clear();
    if (!std::filesystem::is_regular_file(candidate, ec) || ec) return false;

    auto ext = LowerPath(candidate.extension().wstring());
    if (ext != L".html" && ext != L".json" && ext != L".txt") return false;

    std::wstring rootText = LowerPath(root.wstring());
    std::wstring candidateText = LowerPath(candidate.wstring());
    if (!rootText.empty() && rootText.back() != L'\\' && rootText.back() != L'/') rootText.push_back(L'\\');
    return candidateText.size() > rootText.size() && candidateText.compare(0, rootText.size(), rootText) == 0;
}

'''
hist = replace_once(hist, anchor, helpers + anchor, "insert trusted history path helpers")
hist = replace_once(
    hist,
    "std::vector<SessionHistoryEntry> GetSessionHistorySnapshot() {\n    std::lock_guard<std::mutex> lk(gHistoryMutex);\n    return gHistory;\n}\n",
    "std::vector<SessionHistoryEntry> GetSessionHistorySnapshot() {\n    std::lock_guard<std::mutex> lk(gHistoryMutex);\n    return gHistory;\n}\n\nbool IsTrustedSessionArtifactPath(const std::wstring& artifactPath) {\n    std::lock_guard<std::mutex> lk(gHistoryMutex);\n    return IsTrustedArtifactPathLocked(artifactPath);\n}\n",
    "publish trusted history path helper",
)
hist = replace_once(
    hist,
    "    std::lock_guard<std::mutex> lk(gHistoryMutex);\n    std::wstring id = report.hardware.stress.sessionId.empty() ? FallbackSessionId(artifactPath) : report.hardware.stress.sessionId;",
    "    std::lock_guard<std::mutex> lk(gHistoryMutex);\n    if (!IsTrustedArtifactPathLocked(artifactPath)) return;\n    std::wstring id = report.hardware.stress.sessionId.empty() ? FallbackSessionId(artifactPath) : report.hardware.stress.sessionId;",
    "gate recorded history artifact path",
)
old_delete = '''bool DeleteSessionHistoryEntry(const std::wstring& sessionId, bool deleteArtifacts) {
    std::lock_guard<std::mutex> lk(gHistoryMutex);
    auto it = std::find_if(gHistory.begin(), gHistory.end(), [&](const SessionHistoryEntry& e){ return e.sessionId == sessionId; });
    if (it == gHistory.end()) return false;
    if (deleteArtifacts) {
        std::error_code ec;
        if (!it->htmlPath.empty()) std::filesystem::remove(it->htmlPath, ec);
        ec.clear();
        if (!it->jsonPath.empty()) std::filesystem::remove(it->jsonPath, ec);
        ec.clear();
        if (!it->evidencePath.empty()) std::filesystem::remove(it->evidencePath, ec);
    }
    gHistory.erase(it);
    return SaveIndexLocked();
}'''
new_delete = '''bool DeleteSessionHistoryEntry(const std::wstring& sessionId, bool deleteArtifacts) {
    std::lock_guard<std::mutex> lk(gHistoryMutex);
    auto it = std::find_if(gHistory.begin(), gHistory.end(), [&](const SessionHistoryEntry& e){ return e.sessionId == sessionId; });
    if (it == gHistory.end()) return false;
    if (deleteArtifacts) {
        const std::wstring paths[] = {it->htmlPath, it->jsonPath, it->evidencePath};
        for (const auto& path : paths) {
            if (!path.empty() && !IsTrustedArtifactPathLocked(path)) return false;
        }
        for (const auto& path : paths) {
            if (path.empty()) continue;
            std::error_code ec;
            if (!std::filesystem::remove(std::filesystem::path(path), ec) || ec) return false;
        }
    }
    gHistory.erase(it);
    return SaveIndexLocked();
}'''
hist = replace_once(hist, old_delete, new_delete, "harden history artifact deletion")
write("src/session_history.cpp", hist)

# ---------------------------------------------------------------------------
# Runtime hardening: lifecycle follows decision; focused S13/S14 actions; no
# silent TrustedPublisher modification; S22 open is constrained to history root.
# ---------------------------------------------------------------------------
main = read("src/main.cpp")
main = replace_once(
    main,
    "const wchar_t* GetCurrentStageName(int stage) {",
    '''CanonicalUiState LifecycleStateFromDecision(const AuditReport& report) {
    const auto& overall = report.hardware.stress.decision.overall;
    if (overall == L"BUY") return CanonicalUiState::Pass;
    if (overall == L"BUY WITH NOTES") return CanonicalUiState::Warning;
    if (overall == L"REJECT") return CanonicalUiState::Fail;
    return CanonicalUiState::Incomplete;
}

const wchar_t* GetCurrentStageName(int stage) {''',
    "insert decision-derived lifecycle mapping",
)
main = replace_once(
    main,
    "    gAuditReady = !gCancel;\n    gRunning = false;\n    gSessionLifecycleState = gCancel ? CanonicalUiState::Cancelled : CanonicalUiState::Pass;\n    PostMessageW(h, WM_AUDIT_DONE, gCancel ? 1 : 0, 0);",
    "    gAuditReady = !gCancel;\n    gRunning = false;\n    gSessionLifecycleState = gCancel ? CanonicalUiState::Cancelled : LifecycleStateFromDecision(gReport);\n    PostMessageW(h, WM_AUDIT_DONE, gCancel ? 1 : 0, 0);",
    "remove false auto-audit lifecycle PASS",
)
main = replace_once(
    main,
    "        else if (id == 1206) { if (CanRunManualTest(h)) CommitManualResults(RunFunctionalIoWizard(h)); return 0; }",
    "        else if (id == 1206) { if (CanRunManualTest(h)) CommitManualResults(RunFunctionalIoWizard(h)); return 0; }\n        else if (id == 1212) { if (CanRunManualTest(h)) CommitManualResults(RunAudioCameraWizard(h)); return 0; }\n        else if (id == 1213) { if (CanRunManualTest(h)) CommitManualResults(RunNetworkConnectivityWizard(h)); return 0; }",
    "add focused S13/S14 command routes",
)

# Replace S12/S13 legacy-size hit tests and add S14 using the same panel/button
# geometry that the v2 renderers use.
start = main.index("        // 14. Display Screen: Open Wizard Button Hit-Test")
end = main.index("        // 16. Session History:", start)
focused_actions = r'''        // 14–16. S12/S13/S14 primary actions: use the same C10 geometry as rendering.
        if (gCurrentTab == MainTab::Display || gCurrentTab == MainTab::AudioCamera || gCurrentTab == MainTab::Network) {
            const int pad = UiMetrics::Scale(24, dpi);
            const int top = layout.contentRect.top + UiMetrics::Scale(72, dpi);
            const int rightW = UiMetrics::Scale(300, dpi);
            const int leftRight = layout.contentRect.right - rightW - UiMetrics::Scale(34, dpi);
            RECT rail{leftRight + UiMetrics::Scale(10, dpi), top,
                      layout.contentRect.right - pad, layout.contentRect.bottom - UiMetrics::Scale(20, dpi)};
            RECT actionButton = GetNextActionButtonRect(rail, dpi);
            if (x >= actionButton.left && x <= actionButton.right && y >= actionButton.top && y <= actionButton.bottom) {
                if (gCurrentTab == MainTab::Display) PostMessageW(h, WM_COMMAND, 1201, 0);
                else if (gCurrentTab == MainTab::AudioCamera) PostMessageW(h, WM_COMMAND, 1212, 0);
                else PostMessageW(h, WM_COMMAND, 1213, 0);
                return 0;
            }
        }

'''
main = main[:start] + focused_actions + main[end:]

main = replace_once(
    main,
    "                    if (!path.empty()) ShellExecuteW(h, L\"open\", path.c_str(), nullptr, nullptr, SW_SHOW);\n                    else MessageBoxW(h, L\"Phiên này chưa có file report/evidence có thể mở.\", L\"LapSure\", MB_OK | MB_ICONINFORMATION);",
    "                    if (!path.empty() && IsTrustedSessionArtifactPath(path)) ShellExecuteW(h, L\"open\", path.c_str(), nullptr, nullptr, SW_SHOW);\n                    else if (!path.empty()) MessageBoxW(h, L\"Đường dẫn report/evidence không nằm trong thư mục lịch sử tin cậy hoặc không phải loại file được phép.\", L\"LapSure\", MB_OK | MB_ICONERROR);\n                    else MessageBoxW(h, L\"Phiên này chưa có file report/evidence có thể mở.\", L\"LapSure\", MB_OK | MB_ICONINFORMATION);",
    "gate S22 ShellExecute path",
)
main = replace_once(
    main,
    "                        DeleteSessionHistoryEntry(selected.sessionId, answer == IDYES);\n                        auto after = GetSessionHistorySnapshot();",
    "                        if (!DeleteSessionHistoryEntry(selected.sessionId, answer == IDYES)) {\n                            MessageBoxW(h, L\"Không thể xóa phiên hoặc artifact không vượt qua kiểm tra đường dẫn an toàn.\", L\"LapSure\", MB_OK | MB_ICONERROR);\n                            return 0;\n                        }\n                        auto after = GetSessionHistorySnapshot();",
    "surface safe history deletion failure",
)
main = replace_once(
    main,
    "        gRunning = false; gAuditReady = (w == 0);\n        gSessionLifecycleState = (w == 0) ? CanonicalUiState::Pass : CanonicalUiState::Cancelled;",
    "        gRunning = false; gAuditReady = (w == 0);\n        if (w == 0) {\n            std::lock_guard<std::mutex> lk(gReportMutex);\n            gSessionLifecycleState = LifecycleStateFromDecision(gReport);\n        } else if (gSessionLifecycleState != CanonicalUiState::Interrupted) {\n            gSessionLifecycleState = CanonicalUiState::Cancelled;\n        }",
    "preserve decision/interrupted lifecycle in WM_AUDIT_DONE",
)

cert_start = main.index("    // Silent automatic certificate trust for current user on launch")
cert_end = main.index("    WNDCLASSEXW wc", cert_start)
main = main[:cert_start] + "    // Trust stores are never modified at runtime. Release signing/trust is an installer/release responsibility.\n\n" + main[cert_end:]
write("src/main.cpp", main)

# ---------------------------------------------------------------------------
# Permanent regression gate.
# ---------------------------------------------------------------------------
test = r'''from pathlib import Path

R = Path(__file__).resolve().parents[1]
MAIN = (R / "src/main.cpp").read_text(encoding="utf-8")
UI_H = (R / "include/lap/ui_components.h").read_text(encoding="utf-8")
UI_CPP = (R / "src/ui_components.cpp").read_text(encoding="utf-8")
HISTORY_H = (R / "include/lap/session_history.h").read_text(encoding="utf-8")
HISTORY = (R / "src/session_history.cpp").read_text(encoding="utf-8")

# Automatic completion must not manufacture a UI PASS.
assert "LifecycleStateFromDecision" in MAIN
assert "gCancel ? CanonicalUiState::Cancelled : CanonicalUiState::Pass" not in MAIN
assert "(w == 0) ? CanonicalUiState::Pass" not in MAIN
assert "gSessionLifecycleState != CanonicalUiState::Interrupted" in MAIN

# S13/S14 must execute their focused real backends.
assert "RunAudioCameraWizard(h)" in MAIN
assert "RunNetworkConnectivityWizard(h)" in MAIN
assert "WM_COMMAND, 1212" in MAIN
assert "WM_COMMAND, 1213" in MAIN

# C10 rendering and hit-testing share one geometry function.
assert "RECT GetNextActionButtonRect" in UI_H
assert "RECT GetNextActionButtonRect" in UI_CPP
assert "GetNextActionButtonRect(r, dpi)" in UI_CPP
assert "GetNextActionButtonRect(rail, dpi)" in MAIN

# Persisted local history is hostile input until canonicalized/allowlisted.
assert "IsTrustedSessionArtifactPath" in HISTORY_H
assert "weakly_canonical" in HISTORY
assert 'ext != L".html"' in HISTORY and 'ext != L".json"' in HISTORY and 'ext != L".txt"' in HISTORY
assert "is_regular_file" in HISTORY
assert "IsTrustedSessionArtifactPath(path)" in MAIN
assert "IsTrustedArtifactPathLocked(path)" in HISTORY

# Application startup must never silently modify the user's trust store.
for forbidden in ["certutil.exe -addstore", "TrustedPublisher", "Silent automatic certificate trust"]:
    assert forbidden not in MAIN, f"silent trust-store mutation returned: {forbidden}"

print("Production hardening round 1 sanity: OK")
'''
write("tests/production_hardening_round1_sanity.py", test)

cmd = read("run_source_tests.cmd")
line = "python tests\\production_hardening_round1_sanity.py\nif errorlevel 1 exit /b 1"
if line not in cmd:
    cmd = cmd.rstrip() + "\n" + line + "\n"
write("run_source_tests.cmd", cmd)

print("Production hardening round 1 migration applied successfully.")
