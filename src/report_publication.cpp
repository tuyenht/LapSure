#include "lap/report.h"
#include "lap/session_history.h"
#include <windows.h>
#include <atomic>
#include <filesystem>
#include <string>

namespace lap {
namespace {
#ifdef LAPSURE_ENABLE_TEST_HOOKS
std::atomic<ReportPublicationFault> gPublicationFault{ReportPublicationFault::None};
bool HasFault(ReportPublicationFault fault) { return gPublicationFault.load(std::memory_order_relaxed) == fault; }
#else
bool FailHtmlStage() { return false; }
bool FailJsonStage() { return false; }
bool FailFinalPublish() { return false; }
bool FailHistoryCommit() { return false; }
#endif

std::wstring SafeToken(const std::wstring& value) {
    std::wstring out;
    out.reserve(value.size());
    for (wchar_t c : value) {
        const bool asciiAlphaNum = (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9');
        out.push_back(asciiAlphaNum || c == L'-' || c == L'_' ? c : L'_');
    }
    return out;
}

std::wstring PublicationToken(const AuditReport& report) {
    const auto id = SafeToken(report.hardware.stress.sessionId);
    if (id.empty()) return {};
    return id + L"-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
}

void CleanupTree(const std::filesystem::path& path) {
    if (path.empty()) return;
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
}

ReportPublicationResult Failure(ReportPublicationStatus status, std::wstring detail,
                                const std::filesystem::path& cleanup = {}) {
    CleanupTree(cleanup);
    ReportPublicationResult result;
    result.status = status;
    result.detail = std::move(detail);
    return result;
}

#ifdef LAPSURE_ENABLE_TEST_HOOKS
bool FailHtmlStage() { return HasFault(ReportPublicationFault::FailHtmlStage); }
bool FailJsonStage() { return HasFault(ReportPublicationFault::FailJsonStage); }
bool FailFinalPublish() { return HasFault(ReportPublicationFault::FailFinalPublish); }
bool FailHistoryCommit() { return HasFault(ReportPublicationFault::FailHistoryCommit); }
#endif
} // namespace

#ifdef LAPSURE_ENABLE_TEST_HOOKS
void SetReportPublicationFaultForTesting(ReportPublicationFault fault) {
    gPublicationFault.store(fault, std::memory_order_relaxed);
}
#endif

ReportPublicationResult PublishReportBundle(const AuditReport& report, const std::wstring& outputDir) {
    if (outputDir.empty() || report.hardware.stress.sessionId.empty()) {
        return Failure(ReportPublicationStatus::InvalidInput,
                       L"Report publication requires a writable output root and stable inspection identity.");
    }

    const std::filesystem::path root(outputDir);
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    if (ec) {
        return Failure(ReportPublicationStatus::StageFailed, L"Unable to create report publication root.");
    }

    InitializeSessionHistory(root.wstring());
    const auto token = PublicationToken(report);
    if (token.empty()) {
        return Failure(ReportPublicationStatus::InvalidInput, L"Inspection identity cannot form a publication token.");
    }

    const auto staging = root / (L".staging-" + token);
    const auto published = root / (L"bundle-" + token);
    CleanupTree(staging);
    CleanupTree(published);
    std::filesystem::create_directories(staging, ec);
    if (ec) {
        return Failure(ReportPublicationStatus::StageFailed, L"Unable to create report staging directory.", staging);
    }

    if (FailHtmlStage()) {
        return Failure(ReportPublicationStatus::StageFailed, L"Injected HTML staging failure.", staging);
    }
    const auto stagedHtml = SaveHtmlReport(report, staging.wstring());
    if (stagedHtml.empty()) {
        return Failure(ReportPublicationStatus::StageFailed, L"HTML report staging failed.", staging);
    }

    if (FailJsonStage()) {
        return Failure(ReportPublicationStatus::StageFailed, L"Injected JSON staging failure.", staging);
    }
    const auto stagedJson = SaveJsonReport(report, staging.wstring());
    if (stagedJson.empty()) {
        return Failure(ReportPublicationStatus::StageFailed, L"JSON report staging failed.", staging);
    }

    if (FailFinalPublish()) {
        return Failure(ReportPublicationStatus::PublishFailed, L"Injected final report publication failure.", staging);
    }

    if (!MoveFileExW(staging.c_str(), published.c_str(), MOVEFILE_WRITE_THROUGH)) {
        const auto error = GetLastError();
        return Failure(ReportPublicationStatus::PublishFailed,
                       L"Unable to publish complete report generation; Win32 error=" + std::to_wstring(error), staging);
    }

    const auto finalHtml = published / std::filesystem::path(stagedHtml).filename();
    const auto finalJson = published / std::filesystem::path(stagedJson).filename();
    if (!std::filesystem::is_regular_file(finalHtml, ec) || ec) {
        return Failure(ReportPublicationStatus::PublishFailed, L"Published HTML artifact is missing.", published);
    }
    ec.clear();
    if (!std::filesystem::is_regular_file(finalJson, ec) || ec) {
        return Failure(ReportPublicationStatus::PublishFailed, L"Published JSON artifact is missing.", published);
    }

    if (FailHistoryCommit() || !CommitSessionHistoryBundle(report, finalHtml.wstring(), finalJson.wstring())) {
        return Failure(ReportPublicationStatus::HistoryCommitFailed,
                       L"Report pair was generated but history commit failed; generation rolled back.", published);
    }

    ReportPublicationResult result;
    result.status = ReportPublicationStatus::Published;
    result.htmlPath = finalHtml.wstring();
    result.jsonPath = finalJson.wstring();
    result.detail = L"HTML/JSON report generation published and history committed.";
    return result;
}

} // namespace lap
