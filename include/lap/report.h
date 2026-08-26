#pragma once
#include "model.h"
#include <string>

namespace lap {

enum class ReportPublicationStatus {
    Published,
    InvalidInput,
    StageFailed,
    PublishFailed,
    HistoryCommitFailed
};

struct ReportPublicationResult {
    ReportPublicationStatus status{ReportPublicationStatus::InvalidInput};
    std::wstring htmlPath;
    std::wstring jsonPath;
    std::wstring detail;

    bool Published() const {
        return status == ReportPublicationStatus::Published && !htmlPath.empty() && !jsonPath.empty();
    }
};

std::wstring ResolveReportDirectory(const std::wstring& appDir,bool winPE);
std::wstring SaveHtmlReport(const AuditReport& report,const std::wstring& outputDir);
std::wstring SaveJsonReport(const AuditReport& report,const std::wstring& outputDir);
ReportPublicationResult PublishReportBundle(const AuditReport& report,const std::wstring& outputDir);

#ifdef LAPSURE_ENABLE_TEST_HOOKS
enum class ReportPublicationFault {
    None,
    FailHtmlStage,
    FailJsonStage,
    FailFinalPublish,
    FailHistoryCommit
};
void SetReportPublicationFaultForTesting(ReportPublicationFault fault);
#endif

} // namespace lap
