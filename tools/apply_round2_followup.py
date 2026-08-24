from pathlib import Path

R = Path(__file__).resolve().parents[1]


def replace_once(rel, old, new):
    p = R / rel
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{rel}: expected exactly one match, found {count}")
    p.write_text(text.replace(old, new), encoding="utf-8")

# Behavioral test variable names must not collide with the earlier report fixture.
old_block = '''    const auto historyDir = txRoot / L"history";
    std::filesystem::create_directories(historyDir);
    const auto htmlPath = historyDir / L"audit_tx-session.html";
    const auto jsonPath = historyDir / L"audit_tx-session.json";
    { std::ofstream f(htmlPath, std::ios::binary | std::ios::trunc); f << "<html></html>"; }
    lap::InitializeSessionHistory(historyDir.wstring());
    lap::AuditReport txReport = CompletedAutomaticReport();
    txReport.hardware.stress.sessionId = L"tx-session";
    txReport.hardware.stress.decision.overall = L"BUY";
    Expect(lap::CommitSessionHistoryBundle(txReport, htmlPath.wstring(), L""), "history accepts partial HTML artifact");
    auto txHistory = lap::GetSessionHistorySnapshot();
    auto txIt = std::find_if(txHistory.begin(), txHistory.end(), [](const auto& e){ return e.sessionId == L"tx-session"; });
    Expect(txIt != txHistory.end() && txIt->status == L"ARTIFACT_PARTIAL", "history marks single artifact as ARTIFACT_PARTIAL");
    { std::ofstream f(jsonPath, std::ios::binary | std::ios::trunc); f << "{}"; }
    Expect(lap::CommitSessionHistoryBundle(txReport, htmlPath.wstring(), jsonPath.wstring()), "history commits complete report pair");
    txHistory = lap::GetSessionHistorySnapshot();
    txIt = std::find_if(txHistory.begin(), txHistory.end(), [](const auto& e){ return e.sessionId == L"tx-session"; });
    Expect(txIt != txHistory.end() && txIt->status == L"COMPLETE" && !txIt->htmlPath.empty() && !txIt->jsonPath.empty(), "history bundle becomes COMPLETE only with HTML and JSON");
    const auto outsidePath = txRoot / L"outside.json";
    { std::ofstream f(outsidePath, std::ios::binary | std::ios::trunc); f << "{}"; }
    Expect(!lap::CommitSessionHistoryBundle(txReport, htmlPath.wstring(), outsidePath.wstring()), "history rejects bundle artifact outside trusted root");
'''
new_block = '''    const auto historyDir = txRoot / L"history";
    std::filesystem::create_directories(historyDir);
    const auto txHtmlPath = historyDir / L"audit_tx-session.html";
    const auto txJsonPath = historyDir / L"audit_tx-session.json";
    { std::ofstream f(txHtmlPath, std::ios::binary | std::ios::trunc); f << "<html></html>"; }
    lap::InitializeSessionHistory(historyDir.wstring());
    lap::AuditReport txReport = CompletedAutomaticReport();
    txReport.hardware.stress.sessionId = L"tx-session";
    txReport.hardware.stress.decision.overall = L"BUY";
    Expect(lap::CommitSessionHistoryBundle(txReport, txHtmlPath.wstring(), L""), "history accepts partial HTML artifact");
    auto txHistory = lap::GetSessionHistorySnapshot();
    auto txIt = std::find_if(txHistory.begin(), txHistory.end(), [](const auto& e){ return e.sessionId == L"tx-session"; });
    Expect(txIt != txHistory.end() && txIt->status == L"ARTIFACT_PARTIAL", "history marks single artifact as ARTIFACT_PARTIAL");
    { std::ofstream f(txJsonPath, std::ios::binary | std::ios::trunc); f << "{}"; }
    Expect(lap::CommitSessionHistoryBundle(txReport, txHtmlPath.wstring(), txJsonPath.wstring()), "history commits complete report pair");
    txHistory = lap::GetSessionHistorySnapshot();
    txIt = std::find_if(txHistory.begin(), txHistory.end(), [](const auto& e){ return e.sessionId == L"tx-session"; });
    Expect(txIt != txHistory.end() && txIt->status == L"COMPLETE" && !txIt->htmlPath.empty() && !txIt->jsonPath.empty(), "history bundle becomes COMPLETE only with HTML and JSON");
    const auto outsidePath = txRoot / L"outside.json";
    { std::ofstream f(outsidePath, std::ios::binary | std::ios::trunc); f << "{}"; }
    Expect(!lap::CommitSessionHistoryBundle(txReport, txHtmlPath.wstring(), outsidePath.wstring()), "history rejects bundle artifact outside trusted root");
'''
replace_once("tests/behavioral_tests.cpp", old_block, new_block)

# Explicit user cancellation after stress but before report persistence is not a crash.
replace_once(
    "src/main.cpp",
    '''        if (persisted.Complete()) CompleteStressJournal(gDir);
    }
    {
''',
    '''        if (persisted.Complete()) CompleteStressJournal(gDir);
    } else if (!report.hardware.stress.sessionId.empty()) {
        // Explicit user cancellation is terminal-by-choice, not crash evidence.
        DiscardInterruptedStressJournal(gDir);
    }
    {
''',
)

# Remove source-tree binary copy side effect. Packaging consumes the build output directly.
replace_once(
    "CMakeLists.txt",
    '''add_custom_command(TARGET LapSure POST_BUILD
    COMMAND powershell -Command "if (Test-Path '$<TARGET_FILE:LapSure>') { Copy-Item '$<TARGET_FILE:LapSure>' '${CMAKE_SOURCE_DIR}/bin/LapSure.exe' -Force -ErrorAction SilentlyContinue; Copy-Item '$<TARGET_FILE:LapSure>' '${CMAKE_SOURCE_DIR}/LapSure.exe' -Force -ErrorAction SilentlyContinue }"
    COMMENT "Syncing LapSure.exe to bin/ and root directory"
)
''',
    '',
)

# Strengthen Round 2 gate around the cancellation edge and clean build output.
p = R / "tests/production_hardening_round2_sanity.py"
text = p.read_text(encoding="utf-8")
text = text.replace(
    'behavior = (R / "tests/behavioral_tests.cpp").read_text(encoding="utf-8")\n',
    'behavior = (R / "tests/behavioral_tests.cpp").read_text(encoding="utf-8")\ncmake = (R / "CMakeLists.txt").read_text(encoding="utf-8")\n',
)
text = text.replace(
    '("orderly cancellation discards active journal", "DiscardInterruptedStressJournal(appDir)" in stress),',
    '("orderly cancellation discards active journal", "DiscardInterruptedStressJournal(appDir)" in stress and "else if (!report.hardware.stress.sessionId.empty())" in main),',
)
text = text.replace(
    '("behavioral recovery transaction coverage exists", "journal remains recoverable after completed stage" in behavior and "history bundle becomes COMPLETE only with HTML and JSON" in behavior),',
    '("behavioral recovery transaction coverage exists", "journal remains recoverable after completed stage" in behavior and "history bundle becomes COMPLETE only with HTML and JSON" in behavior),\n    ("build output stays outside source tree", "Syncing LapSure.exe to bin/ and root directory" not in cmake and "Copy-Item '$<TARGET_FILE:LapSure>'" not in cmake),',
)
p.write_text(text, encoding="utf-8")

print("Round 2 follow-up applied")
