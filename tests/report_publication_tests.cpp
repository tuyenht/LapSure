#include "lap/report.h"
#include "lap/session_history.h"
#include <filesystem>
#include <fstream>
#include <iostream>

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

lap::AuditReport Fixture() {
    lap::AuditReport report{};
    report.model = L"Publication Test Laptop";
    report.serviceTag = L"TEST-PUBLICATION";
    report.hardware.stress.sessionId = L"11111111-2222-3333-4444-555555555555";
    report.hardware.stress.decision.overall = L"BUY";
    report.hardware.stress.decision.coverage = L"COMPLETE";
    report.hardware.stress.decision.confidence = lap::Confidence::High;
    return report;
}

bool HasPublishedBundle(const std::filesystem::path& root) {
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) return false;
    for (const auto& item : std::filesystem::directory_iterator(root, ec)) {
        if (ec) return false;
        if (item.is_directory() && item.path().filename().wstring().rfind(L"bundle-", 0) == 0) return true;
    }
    return false;
}
}

int main() {
    const auto root = std::filesystem::temp_directory_path() / L"lapsure-report-publication-tests";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    lap::InitializeSessionHistory(root.wstring());

    auto report = Fixture();
    const auto originalDecision = report.hardware.stress.decision.overall;

    lap::SetReportPublicationFaultForTesting(lap::ReportPublicationFault::FailJsonStage);
    auto result = lap::PublishReportBundle(report, root.wstring());
    Expect(result.status == lap::ReportPublicationStatus::StageFailed, "JSON staging failure is explicit");
    Expect(result.htmlPath.empty() && result.jsonPath.empty(), "staging failure advertises no report pair");
    Expect(report.hardware.stress.decision.overall == originalDecision, "staging failure does not mutate hardware decision");
    Expect(!HasPublishedBundle(root), "staging failure leaves no published bundle");

    lap::SetReportPublicationFaultForTesting(lap::ReportPublicationFault::FailFinalPublish);
    result = lap::PublishReportBundle(report, root.wstring());
    Expect(result.status == lap::ReportPublicationStatus::PublishFailed, "final directory publication failure is explicit");
    Expect(result.htmlPath.empty() && result.jsonPath.empty(), "final publish failure advertises no report pair");
    Expect(report.hardware.stress.decision.overall == originalDecision, "final publish failure does not mutate hardware decision");
    Expect(!HasPublishedBundle(root), "final publish failure leaves no published bundle");

    lap::SetReportPublicationFaultForTesting(lap::ReportPublicationFault::FailHistoryCommit);
    result = lap::PublishReportBundle(report, root.wstring());
    Expect(result.status == lap::ReportPublicationStatus::HistoryCommitFailed, "history commit failure is explicit");
    Expect(result.htmlPath.empty() && result.jsonPath.empty(), "history failure advertises no report pair");
    Expect(report.hardware.stress.decision.overall == originalDecision, "history failure does not mutate hardware decision");
    Expect(!HasPublishedBundle(root), "history failure rolls back the unpublished generation");

    lap::SetReportPublicationFaultForTesting(lap::ReportPublicationFault::None);
    result = lap::PublishReportBundle(report, root.wstring());
    Expect(result.status == lap::ReportPublicationStatus::Published && result.Published(), "complete pair publishes successfully");
    Expect(!result.htmlPath.empty() && !result.jsonPath.empty(), "successful publication returns both paths");
    Expect(std::filesystem::exists(result.htmlPath) && std::filesystem::exists(result.jsonPath), "published report pair exists on disk");
    Expect(std::filesystem::path(result.htmlPath).parent_path() == std::filesystem::path(result.jsonPath).parent_path(), "HTML and JSON share one immutable generation directory");
    Expect(report.hardware.stress.decision.overall == originalDecision, "successful publication preserves hardware decision");

    const auto history = lap::GetSessionHistorySnapshot();
    bool historyComplete = false;
    for (const auto& item : history) {
        if (item.sessionId == report.hardware.stress.sessionId && item.status == L"COMPLETE" && item.htmlPath == result.htmlPath && item.jsonPath == result.jsonPath) {
            historyComplete = true;
            break;
        }
    }
    Expect(historyComplete, "history advertises only the successfully published pair");

    // Task 7A: Verify JSON persists frozen policy versions and authority metadata
    auto task7Report = Fixture();
    task7Report.hardware.stress.sessionId = L"task7-json-frozen-metadata";
    task7Report.hardware.stress.decision.decisionPolicyVersion = L"5.1.0";
    task7Report.hardware.stress.decision.coveragePolicyVersion = L"5.1.0";
    task7Report.hardware.stress.decision.authorityPolicyVersion = L"5.1.0";
    task7Report.hardware.stress.decision.chassisAuthority = L"ADVISORY";
    task7Report.hardware.stress.decision.factoryAuthority = L"UNKNOWN";
    task7Report.hardware.stress.decision.discreteGpuCapability = L"ABSENT_CONFIRMED";
    task7Report.hardware.stress.decision.coverageDomains = {
        {L"frozen_test_domain_456", L"Frozen Test Domain Name", L"COMPLETE", true, L"frozen test source", L""}
    };

    auto task7Result = lap::PublishReportBundle(task7Report, root.wstring());
    Expect(task7Result.status == lap::ReportPublicationStatus::Published, "task 7 report bundle published");
    Expect(std::filesystem::exists(task7Result.jsonPath), "task 7 json file exists");

    std::string jsonContent;
    {
        std::ifstream in(task7Result.jsonPath, std::ios::binary);
        jsonContent.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
    Expect(jsonContent.find("\"decisionPolicyVersion\":\"5.1.0\"") != std::string::npos,
           "published JSON contains frozen decisionPolicyVersion 5.1.0");
    Expect(jsonContent.find("\"coveragePolicyVersion\":\"5.1.0\"") != std::string::npos,
           "published JSON contains frozen coveragePolicyVersion 5.1.0");
    Expect(jsonContent.find("\"authorityPolicyVersion\":\"5.1.0\"") != std::string::npos,
           "published JSON contains frozen authorityPolicyVersion 5.1.0");
    Expect(jsonContent.find("\"chassisAuthority\":\"ADVISORY\"") != std::string::npos,
           "published JSON contains frozen chassisAuthority");
    Expect(jsonContent.find("\"factoryAuthority\":\"UNKNOWN\"") != std::string::npos,
           "published JSON contains frozen factoryAuthority");
    Expect(jsonContent.find("\"discreteGpuCapability\":\"ABSENT_CONFIRMED\"") != std::string::npos,
           "published JSON contains frozen discreteGpuCapability");

    // Critical anti-recomputation regression check:
    Expect(jsonContent.find("frozen_test_domain_456") != std::string::npos,
           "JSON report serializes frozen decision coverageDomains without recomputing policy");

    std::string htmlContent;
    {
        std::ifstream in(task7Result.htmlPath, std::ios::binary);
        htmlContent.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
    Expect(htmlContent.find("Frozen Test Domain Name") != std::string::npos,
           "HTML report serializes frozen decision coverageDomains without recomputing policy");

    std::filesystem::remove_all(root, ec);
    return failures == 0 ? 0 : 1;
}
