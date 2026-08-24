from pathlib import Path

R = Path(__file__).resolve().parents[1]


def replace_once(rel, old, new):
    p = R / rel
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{rel}: expected exactly one match, found {count}")
    p.write_text(text.replace(old, new), encoding="utf-8")


# Journal: session status stays RUNNING while stage status advances.
replace_once(
    "include/lap/journal.h",
    "    std::wstring status;\n    std::wstring time;",
    "    std::wstring status;\n    std::wstring stageStatus;\n    std::wstring time;",
)
replace_once(
    "src/journal.cpp",
    '    info.status = ValueOf(info.rawEvidence, L"status");\n    info.time = ValueOf(info.rawEvidence, L"time");',
    '    info.status = ValueOf(info.rawEvidence, L"status");\n    info.stageStatus = ValueOf(info.rawEvidence, L"stage_status");\n    info.time = ValueOf(info.rawEvidence, L"time");',
)
replace_once(
    "src/journal.cpp",
    '    f << L"session=" << id << L"\\nstage=" << stage << L"\\nstatus=" << status\n      << L"\\ntime=" << t.wYear << L"-" << t.wMonth << L"-" << t.wDay << L" "',
    '    f << L"session=" << id << L"\\nstage=" << stage << L"\\nstatus=RUNNING"\n      << L"\\nstage_status=" << status\n      << L"\\ntime=" << t.wYear << L"-" << t.wMonth << L"-" << t.wDay << L" "',
)

# Stress owns stage execution only. A clean cancel discards the active journal;
# successful completion remains RUNNING until report/history persistence commits.
replace_once(
    "src/stress.cpp",
    'ss.completed=!(cancel&&cancel->load());if(ss.completed)CompleteStressJournal(appDir);ss.cpuBenchmark=RunCpuMicroBenchmark(r.hardware.cpuName,appDir,cancel);',
    'ss.completed=!(cancel&&cancel->load());if(!ss.completed)DiscardInterruptedStressJournal(appDir);ss.cpuBenchmark=RunCpuMicroBenchmark(r.hardware.cpuName,appDir,cancel);',
)

# History bundle API.
replace_once(
    "include/lap/session_history.h",
    'void RecordSessionHistoryArtifact(const AuditReport& report, const std::wstring& artifactPath, bool isHtml);',
    'void RecordSessionHistoryArtifact(const AuditReport& report, const std::wstring& artifactPath, bool isHtml);\nbool CommitSessionHistoryBundle(const AuditReport& report, const std::wstring& htmlPath, const std::wstring& jsonPath);',
)
old_record = '''void RecordSessionHistoryArtifact(const AuditReport& report, const std::wstring& artifactPath, bool isHtml) {
    if (artifactPath.empty()) return;
    const auto dir = std::filesystem::path(artifactPath).parent_path().wstring();
    InitializeSessionHistory(dir);
    std::lock_guard<std::mutex> lk(gHistoryMutex);
    if (!IsTrustedArtifactPathLocked(artifactPath)) return;
    std::wstring id = report.hardware.stress.sessionId.empty() ? FallbackSessionId(artifactPath) : report.hardware.stress.sessionId;
    auto* e = FindLocked(id);
    if (!e) {
        SessionHistoryEntry item;
        item.sessionId = id;
        item.timestamp = NowIso();
        gHistory.insert(gHistory.begin(), std::move(item));
        e = &gHistory.front();
    }
    e->model = report.model;
    e->serviceTag = report.serviceTag;
    e->verdict = report.hardware.stress.decision.overall;
    e->status = report.hardware.stress.decision.overall == L"INCOMPLETE" ? L"INCOMPLETE" : L"COMPLETE";
    if (isHtml) e->htmlPath = artifactPath; else e->jsonPath = artifactPath;
    SaveIndexLocked();
}
'''
new_record = '''bool CommitSessionHistoryBundle(const AuditReport& report, const std::wstring& htmlPath, const std::wstring& jsonPath) {
    if (htmlPath.empty() && jsonPath.empty()) return false;
    const std::wstring seed = !htmlPath.empty() ? htmlPath : jsonPath;
    const auto dir = std::filesystem::path(seed).parent_path().wstring();
    std::lock_guard<std::mutex> lk(gHistoryMutex);
    if (gHistoryDir.empty()) {
        gHistoryDir = dir;
        LoadIndexLocked();
    }
    if ((!htmlPath.empty() && !IsTrustedArtifactPathLocked(htmlPath)) ||
        (!jsonPath.empty() && !IsTrustedArtifactPathLocked(jsonPath))) return false;

    std::wstring id = report.hardware.stress.sessionId.empty() ? FallbackSessionId(seed) : report.hardware.stress.sessionId;
    auto* e = FindLocked(id);
    if (!e) {
        SessionHistoryEntry item;
        item.sessionId = id;
        item.timestamp = NowIso();
        gHistory.insert(gHistory.begin(), std::move(item));
        e = &gHistory.front();
    }
    e->model = report.model;
    e->serviceTag = report.serviceTag;
    e->verdict = report.hardware.stress.decision.overall;
    if (!htmlPath.empty()) e->htmlPath = htmlPath;
    if (!jsonPath.empty()) e->jsonPath = jsonPath;
    const bool pairComplete = !e->htmlPath.empty() && !e->jsonPath.empty();
    e->status = pairComplete ? L"COMPLETE" : L"ARTIFACT_PARTIAL";
    e->note = pairComplete ? L"" : L"Report bundle chưa đầy đủ; không được coi là phiên đã persist hoàn chỉnh.";
    return SaveIndexLocked();
}

void RecordSessionHistoryArtifact(const AuditReport& report, const std::wstring& artifactPath, bool isHtml) {
    if (artifactPath.empty()) return;
    if (isHtml) (void)CommitSessionHistoryBundle(report, artifactPath, L"");
    else (void)CommitSessionHistoryBundle(report, L"", artifactPath);
}
'''
replace_once("src/session_history.cpp", old_record, new_record)

# Report writes are atomic; history is committed by the bundle owner, not each file writer.
old_write = '''bool WriteUtf8File(const std::filesystem::path&p,const std::wstring&text){if(text.size()>static_cast<size_t>(INT_MAX))return false;int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);if(n<=0)return false;std::string bytes(static_cast<size_t>(n),'\\0');if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),bytes.data(),n,nullptr,nullptr)!=n)return false;std::ofstream out(p,std::ios::binary|std::ios::trunc);if(!out)return false;out.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));out.flush();return out.good();}'''
new_write = '''bool WriteUtf8File(const std::filesystem::path&p,const std::wstring&text){
 if(text.size()>static_cast<size_t>(INT_MAX))return false;
 int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);if(n<=0)return false;
 std::string bytes(static_cast<size_t>(n),'\\0');if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),bytes.data(),n,nullptr,nullptr)!=n)return false;
 const auto target=p.wstring();const auto temp=target+L".tmp";std::ofstream out(std::filesystem::path(temp),std::ios::binary|std::ios::trunc);if(!out)return false;
 out.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));out.flush();if(!out.good()){out.close();DeleteFileW(temp.c_str());return false;}out.close();
 if(!MoveFileExW(temp.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){DeleteFileW(temp.c_str());return false;}return true;
}'''
replace_once("src/report.cpp", old_write, new_write)
replace_once(
    "src/report.cpp",
    'f<<L"</table></details></main></body></html>";if(!WriteUtf8File(p,f.str()))return L"";RecordSessionHistoryArtifact(r,p.wstring(),true);return p.wstring();',
    'f<<L"</table></details></main></body></html>";if(!WriteUtf8File(p,f.str()))return L"";return p.wstring();',
)
replace_once(
    "src/report.cpp",
    'f<<L"]}\\n";if(!WriteUtf8File(p,f.str()))return L"";RecordSessionHistoryArtifact(r,p.wstring(),false);return p.wstring();',
    'f<<L"]}\\n";if(!WriteUtf8File(p,f.str()))return L"";return p.wstring();',
)

# Main owns the transaction boundary.
anchor = '''CanonicalUiState LifecycleStateFromDecision(const AuditReport& report) {
    const auto& overall = report.hardware.stress.decision.overall;
    if (overall == L"BUY") return CanonicalUiState::Pass;
    if (overall == L"BUY WITH NOTES") return CanonicalUiState::Warning;
    if (overall == L"REJECT") return CanonicalUiState::Fail;
    return CanonicalUiState::Incomplete;
}
'''
insert = anchor + '''
struct ReportPersistResult {
    std::wstring htmlPath;
    std::wstring jsonPath;
    bool historyCommitted{false};
    bool Complete() const { return !htmlPath.empty() && !jsonPath.empty() && historyCommitted; }
};

void MarkReportPersistenceIncomplete(AuditReport& report) {
    auto& decision = report.hardware.stress.decision;
    if (decision.overall == L"BUY" || decision.overall == L"BUY WITH NOTES") decision.overall = L"INCOMPLETE";
    decision.coverage = L"PARTIAL";
    decision.confidence = Confidence::Low;
    const std::wstring reason = L"Report bundle/history persistence did not complete; acceptance verdict is withheld.";
    if (std::find(decision.reasons.begin(), decision.reasons.end(), reason) == decision.reasons.end()) decision.reasons.push_back(reason);
}

ReportPersistResult PersistReportBundle(const AuditReport& report, const std::wstring& outputDir) {
    ReportPersistResult result;
    result.htmlPath = SaveHtmlReport(report, outputDir);
    result.jsonPath = SaveJsonReport(report, outputDir);
    result.historyCommitted = CommitSessionHistoryBundle(report, result.htmlPath, result.jsonPath);
    return result;
}
'''
replace_once("src/main.cpp", anchor, insert)

old_rebuild = '''void RebuildDecisionAndReports() {
    std::lock_guard<std::mutex> lk(gReportMutex);
    gReport.hardware.stress.decision = BuildAuditDecision(gReport);
    BuildOrchestrator(gReport, gRunning.load(), gAuditReady.load());
    auto out = gReportOutputDir;
    if (out.empty()) {
        auto caps = DetectCapabilities(gDir);
        out = ResolveReportDirectory(gDir, caps.winPE);
        gReportOutputDir = out;
        InitializeSessionHistory(out);
    }
    gReportPath = SaveHtmlReport(gReport, out);
    SaveJsonReport(gReport, out);
}
'''
new_rebuild = '''void RebuildDecisionAndReports() {
    std::lock_guard<std::mutex> lk(gReportMutex);
    gReport.hardware.stress.decision = BuildAuditDecision(gReport);
    BuildOrchestrator(gReport, gRunning.load(), gAuditReady.load());
    auto out = gReportOutputDir;
    if (out.empty()) {
        auto caps = DetectCapabilities(gDir);
        out = ResolveReportDirectory(gDir, caps.winPE);
        gReportOutputDir = out;
        InitializeSessionHistory(out);
    }
    auto persisted = PersistReportBundle(gReport, out);
    if (!persisted.Complete()) {
        MarkReportPersistenceIncomplete(gReport);
        BuildOrchestrator(gReport, false, false);
        persisted = PersistReportBundle(gReport, out);
        gSessionLifecycleState = CanonicalUiState::Incomplete;
    } else {
        gSessionLifecycleState = LifecycleStateFromDecision(gReport);
    }
    gReportPath = persisted.htmlPath;
}
'''
replace_once("src/main.cpp", old_rebuild, new_rebuild)

old_audit_save = '''    std::wstring reportPath;
    if (!gCancel) {
        auto out = ResolveReportDirectory(gDir, caps.winPE);
        reportPath = SaveHtmlReport(report, out);
        SaveJsonReport(report, out);
    }
    {
        std::lock_guard<std::mutex> lk(gReportMutex);
        gReport = std::move(report);
        gReportPath = std::move(reportPath);
    }
    gAuditReady = !gCancel;
    gRunning = false;
    gSessionLifecycleState = gCancel ? CanonicalUiState::Cancelled : LifecycleStateFromDecision(gReport);
'''
new_audit_save = '''    ReportPersistResult persisted;
    if (!gCancel) {
        auto out = ResolveReportDirectory(gDir, caps.winPE);
        gReportOutputDir = out;
        InitializeSessionHistory(out);
        persisted = PersistReportBundle(report, out);
        if (!persisted.Complete()) {
            MarkReportPersistenceIncomplete(report);
            BuildOrchestrator(report, false, false);
            persisted = PersistReportBundle(report, out);
            PostStatus(h, persisted.Complete()
                ? L"Report đã persist sau retry nhưng verdict acceptance vẫn giữ INCOMPLETE để bảo toàn tính thận trọng."
                : L"Không thể persist đầy đủ HTML/JSON/history; journal được giữ để phục hồi và không phát hành clean verdict.");
        }
        if (persisted.Complete()) CompleteStressJournal(gDir);
    }
    {
        std::lock_guard<std::mutex> lk(gReportMutex);
        gReport = std::move(report);
        gReportPath = persisted.htmlPath;
    }
    gAuditReady = !gCancel && persisted.Complete();
    gRunning = false;
    gSessionLifecycleState = gCancel ? CanonicalUiState::Cancelled
        : (persisted.Complete() ? LifecycleStateFromDecision(gReport) : CanonicalUiState::Incomplete);
'''
replace_once("src/main.cpp", old_audit_save, new_audit_save)

# Update S22/S23 contract gate for bundle-owned history commit.
replace_once(
    "tests/s22_s23_persistence_recovery_sanity.py",
    'assert "RecordSessionHistoryArtifact(r,p.wstring(),true)" in REPORT\nassert "RecordSessionHistoryArtifact(r,p.wstring(),false)" in REPORT',
    'assert "CommitSessionHistoryBundle" in HISTORY_H\nassert "CommitSessionHistoryBundle" in HISTORY\nassert "PersistReportBundle" in MAIN',
)

# Behavioral transaction tests.
replace_once(
    "tests/behavioral_tests.cpp",
    '#include "lap/profile.h"\n#include <filesystem>',
    '#include "lap/profile.h"\n#include "lap/journal.h"\n#include "lap/session_history.h"\n#include <filesystem>\n#include <fstream>',
)
behavior_anchor = '''    std::filesystem::remove_all(providerDir,cleanupError);
    return failures == 0 ? 0 : 1;
}'''
behavior_insert = '''    const auto txRoot = std::filesystem::temp_directory_path() / L"lapsure-transaction-recovery";
    std::filesystem::remove_all(txRoot, cleanupError);
    std::filesystem::create_directories(txRoot / L"reports");
    Expect(lap::WriteStressJournal(txRoot.wstring(), L"tx-session", L"CPU sustained load", L"RUNNING"), "transaction journal starts");
    auto txJournal = lap::ReadInterruptedStressJournal(txRoot.wstring());
    Expect(txJournal.present && txJournal.status == L"RUNNING" && txJournal.stageStatus == L"RUNNING", "running journal is recoverable");
    Expect(lap::WriteStressJournal(txRoot.wstring(), L"tx-session", L"CPU sustained load", L"COMPLETED"), "stage completion updates journal");
    txJournal = lap::ReadInterruptedStressJournal(txRoot.wstring());
    Expect(txJournal.present && txJournal.status == L"RUNNING" && txJournal.stageStatus == L"COMPLETED", "journal remains recoverable after completed stage");
    Expect(lap::DiscardInterruptedStressJournal(txRoot.wstring()) && !lap::ReadInterruptedStressJournal(txRoot.wstring()).present, "orderly cancel/discard is not reported as interruption");

    const auto historyDir = txRoot / L"history";
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
    std::filesystem::remove_all(txRoot, cleanupError);

    std::filesystem::remove_all(providerDir,cleanupError);
    return failures == 0 ? 0 : 1;
}'''
replace_once("tests/behavioral_tests.cpp", behavior_anchor, behavior_insert)

# Activate Round 2 source gate.
replace_once(
    "run_source_tests.cmd",
    'python tests\\production_hardening_round1_sanity.py\nif errorlevel 1 exit /b 1\n',
    'python tests\\production_hardening_round1_sanity.py\nif errorlevel 1 exit /b 1\npython tests\\production_hardening_round2_sanity.py\nif errorlevel 1 exit /b 1\n',
)

print("Production hardening Round 2 migration applied")
