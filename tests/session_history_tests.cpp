#include "lap/session_history.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {
int failures = 0;

void Expect(bool condition, const char* behavior) {
    if (!condition) {
        std::cerr << "FAIL " << behavior << '\n';
        ++failures;
    } else {
        std::cout << "PASS " << behavior << '\n';
    }
}

lap::AuditReport Fixture(const std::wstring& sessionId) {
    lap::AuditReport report{};
    report.model = L"Transactional History Test Laptop";
    report.serviceTag = L"TEST-HISTORY";
    report.hardware.stress.sessionId = sessionId;
    report.hardware.stress.decision.overall = L"BUY";
    report.hardware.stress.decision.coverage = L"COMPLETE";
    report.hardware.stress.decision.confidence = lap::Confidence::High;
    return report;
}

void WriteText(const std::filesystem::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
}

void Touch(const std::filesystem::path& path, const char* text) {
    WriteText(path, text);
}

bool ContainsSession(const std::vector<lap::SessionHistoryEntry>& history, const std::wstring& id) {
    for (const auto& entry : history) if (entry.sessionId == id) return true;
    return false;
}

std::string ReadText(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void ResetRoot(const std::filesystem::path& root) {
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    lap::SetSessionHistoryFaultForTesting(lap::SessionHistoryFault::None);
    lap::InitializeSessionHistory(root.wstring());
}
} // namespace

int main() {
    const auto temp = std::filesystem::temp_directory_path();
    const auto root = temp / L"lapsure-session-history-tests";
    ResetRoot(root);

    const auto html1 = root / L"audit_history_1.html";
    const auto json1 = root / L"audit_history_1.json";
    Touch(html1, "html-1");
    Touch(json1, "json-1");
    auto report1 = Fixture(L"history-test-1");
    Expect(lap::CommitSessionHistoryBundle(report1, html1.wstring(), json1.wstring()),
           "complete report pair commits to history");
    auto history = lap::GetSessionHistorySnapshot();
    Expect(history.size() == 1 && history[0].status == L"COMPLETE",
           "successful bundle commit publishes one COMPLETE live record");
    Expect(ReadText(root / L"session_history.tsv").rfind("#LapSureSessionHistory\t2\n", 0) == 0,
           "persisted history carries schema version 2 header");

    const auto html2 = root / L"audit_history_2.html";
    const auto json2 = root / L"audit_history_2.json";
    Touch(html2, "html-2");
    Touch(json2, "json-2");
    auto report2 = Fixture(L"history-test-2");
    lap::SetSessionHistoryFaultForTesting(lap::SessionHistoryFault::FailIndexWrite);
    Expect(!lap::CommitSessionHistoryBundle(report2, html2.wstring(), json2.wstring()),
           "injected index write failure is reported");
    history = lap::GetSessionHistorySnapshot();
    Expect(history.size() == 1 && !ContainsSession(history, report2.hardware.stress.sessionId),
           "index write failure leaves live history unchanged");

    lap::SetSessionHistoryFaultForTesting(lap::SessionHistoryFault::FailIndexPublish);
    Expect(!lap::CommitSessionHistoryBundle(report2, html2.wstring(), json2.wstring()),
           "injected index publish failure is reported");
    history = lap::GetSessionHistorySnapshot();
    Expect(history.size() == 1 && !ContainsSession(history, report2.hardware.stress.sessionId),
           "index publish failure leaves live history unchanged");

    lap::SetSessionHistoryFaultForTesting(lap::SessionHistoryFault::None);
    auto oversized = Fixture(L"history-test-oversized");
    oversized.model.assign(33000, L'X');
    Expect(!lap::CommitSessionHistoryBundle(oversized, html2.wstring(), json2.wstring()),
           "oversized persisted field is rejected");
    Expect(lap::GetSessionHistorySnapshot().size() == 1,
           "oversized candidate cannot mutate live history");

    const auto corruptRoot = temp / L"lapsure-session-history-corrupt-tests";
    ResetRoot(corruptRoot);
    const auto corruptIndex = corruptRoot / L"session_history.tsv";
    WriteText(corruptIndex,
        "#LapSureSessionHistory\t1\n"
        "bad-session\t2026-08-25 12:00:00\tModel\tTAG\tBUY\tBOGUS\ta.html\ta.json\t\tnote\n");
    lap::InitializeSessionHistory(corruptRoot.wstring());
    Expect(lap::GetSessionHistorySnapshot().empty(),
           "invalid persisted status is not loaded into live history");
    const auto corruptHtml = corruptRoot / L"fresh.html";
    const auto corruptJson = corruptRoot / L"fresh.json";
    Touch(corruptHtml, "html");
    Touch(corruptJson, "json");
    auto fresh = Fixture(L"history-test-fresh");
    Expect(!lap::CommitSessionHistoryBundle(fresh, corruptHtml.wstring(), corruptJson.wstring()),
           "invalid persisted index blocks mutation instead of being overwritten");
    Expect(ReadText(corruptIndex).find("BOGUS") != std::string::npos,
           "invalid persisted evidence remains available for diagnosis");

    const auto duplicateRoot = temp / L"lapsure-session-history-duplicate-tests";
    ResetRoot(duplicateRoot);
    WriteText(duplicateRoot / L"session_history.tsv",
        "#LapSureSessionHistory\t1\n"
        "dup-session\t2026-08-25 12:00:00\tModel\tTAG\tBUY\tCOMPLETE\ta.html\ta.json\t\tnote\n"
        "dup-session\t2026-08-25 12:01:00\tModel\tTAG\tBUY\tCOMPLETE\tb.html\tb.json\t\tnote\n");
    lap::InitializeSessionHistory(duplicateRoot.wstring());
    Expect(lap::GetSessionHistorySnapshot().empty(),
           "duplicate persisted session identities are rejected");

    const auto deleteRoot = temp / L"lapsure-session-history-delete-tests";
    ResetRoot(deleteRoot);
    const auto deleteHtml = deleteRoot / L"delete.html";
    const auto deleteJson = deleteRoot / L"delete.json";
    Touch(deleteHtml, "html-delete");
    Touch(deleteJson, "json-delete");
    auto deleteReport = Fixture(L"history-test-delete");
    Expect(lap::CommitSessionHistoryBundle(deleteReport, deleteHtml.wstring(), deleteJson.wstring()),
           "delete fixture commits successfully");

    lap::SetSessionHistoryFaultForTesting(lap::SessionHistoryFault::FailArtifactMove);
    Expect(!lap::DeleteSessionHistoryEntry(deleteReport.hardware.stress.sessionId, true),
           "artifact staging failure aborts delete");
    Expect(std::filesystem::exists(deleteHtml) && std::filesystem::exists(deleteJson) &&
           ContainsSession(lap::GetSessionHistorySnapshot(), deleteReport.hardware.stress.sessionId),
           "artifact staging failure preserves files and live record");

    lap::SetSessionHistoryFaultForTesting(lap::SessionHistoryFault::FailIndexPublish);
    Expect(!lap::DeleteSessionHistoryEntry(deleteReport.hardware.stress.sessionId, true),
           "index publication failure aborts delete");
    Expect(std::filesystem::exists(deleteHtml) && std::filesystem::exists(deleteJson) &&
           ContainsSession(lap::GetSessionHistorySnapshot(), deleteReport.hardware.stress.sessionId),
           "failed delete index publication rolls quarantined files back");

    lap::SetSessionHistoryFaultForTesting(lap::SessionHistoryFault::None);
    Expect(lap::DeleteSessionHistoryEntry(deleteReport.hardware.stress.sessionId, true),
           "successful delete commits index transition");
    Expect(!std::filesystem::exists(deleteHtml) && !std::filesystem::exists(deleteJson) &&
           !ContainsSession(lap::GetSessionHistorySnapshot(), deleteReport.hardware.stress.sessionId),
           "successful delete removes original artifacts and live record");

    // Task 7B: Schema v1 migration test (10 fields -> legacy-v1 policy versions)
    const auto v1Root = temp / L"lapsure-session-history-v1-migration-tests";
    ResetRoot(v1Root);
    WriteText(v1Root / L"session_history.tsv",
        "#LapSureSessionHistory\t1\n"
        "v1-session\t2026-08-25 12:00:00\tModel V1\tTAG-V1\tBUY\tCOMPLETE\tv1.html\tv1.json\t\tnote v1\n");
    lap::InitializeSessionHistory(v1Root.wstring());
    auto v1History = lap::GetSessionHistorySnapshot();
    Expect(v1History.size() == 1, "schema v1 index is loaded successfully");
    if (!v1History.empty()) {
        Expect(v1History[0].sessionId == L"v1-session", "v1 sessionId matches");
        Expect(v1History[0].decisionPolicyVersion == L"legacy-v1", "v1 decisionPolicyVersion is legacy-v1");
        Expect(v1History[0].coveragePolicyVersion == L"legacy-v1", "v1 coveragePolicyVersion is legacy-v1");
        Expect(v1History[0].authorityPolicyVersion == L"legacy-v1", "v1 authorityPolicyVersion is legacy-v1");
    }

    // Task 7B: Schema v2 parsing test (13 fields -> preserved policy versions)
    const auto v2Root = temp / L"lapsure-session-history-v2-tests";
    ResetRoot(v2Root);
    WriteText(v2Root / L"session_history.tsv",
        "#LapSureSessionHistory\t2\n"
        "v2-session\t2026-08-26 12:00:00\tModel V2\tTAG-V2\tBUY WITH NOTES\tCOMPLETE\tv2.html\tv2.json\t\tnote v2\t5.1.0\t5.1.0\t5.1.0\n");
    lap::InitializeSessionHistory(v2Root.wstring());
    auto v2History = lap::GetSessionHistorySnapshot();
    Expect(v2History.size() == 1, "schema v2 index is loaded successfully");
    if (!v2History.empty()) {
        Expect(v2History[0].sessionId == L"v2-session", "v2 sessionId matches");
        Expect(v2History[0].decisionPolicyVersion == L"5.1.0", "v2 decisionPolicyVersion preserved");
        Expect(v2History[0].coveragePolicyVersion == L"5.1.0", "v2 coveragePolicyVersion preserved");
        Expect(v2History[0].authorityPolicyVersion == L"5.1.0", "v2 authorityPolicyVersion preserved");
    }

    // Task 7B: Unsupported schema version fail-closed
    const auto unsupportedRoot = temp / L"lapsure-session-history-unsupported-schema-tests";
    ResetRoot(unsupportedRoot);
    WriteText(unsupportedRoot / L"session_history.tsv",
        "#LapSureSessionHistory\t3\n"
        "v3-session\t2026-08-26 12:00:00\tModel V3\tTAG-V3\tBUY\tCOMPLETE\tv3.html\tv3.json\t\tnote v3\t5.1.0\t5.1.0\t5.1.0\n");
    lap::InitializeSessionHistory(unsupportedRoot.wstring());
    Expect(lap::GetSessionHistorySnapshot().empty(),
           "unsupported schema version 3 fails closed");

    // Task 7B: Malformed schema v2 line (wrong field count) fails closed
    const auto malformedV2Root = temp / L"lapsure-session-history-malformed-v2-tests";
    ResetRoot(malformedV2Root);
    WriteText(malformedV2Root / L"session_history.tsv",
        "#LapSureSessionHistory\t2\n"
        "v2-bad-session\t2026-08-26 12:00:00\tModel V2\tTAG-V2\tBUY\tCOMPLETE\tv2.html\tv2.json\t\tnote v2\t5.1.0\n");
    lap::InitializeSessionHistory(malformedV2Root.wstring());
    Expect(lap::GetSessionHistorySnapshot().empty(),
           "malformed schema v2 line with missing fields fails closed");

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::remove_all(corruptRoot, ec);
    std::filesystem::remove_all(duplicateRoot, ec);
    std::filesystem::remove_all(deleteRoot, ec);
    std::filesystem::remove_all(v1Root, ec);
    std::filesystem::remove_all(v2Root, ec);
    std::filesystem::remove_all(unsupportedRoot, ec);
    std::filesystem::remove_all(malformedV2Root, ec);
    return failures == 0 ? 0 : 1;
}
