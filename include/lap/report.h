#pragma once
#include "model.h"
#include <string>
namespace lap {
std::wstring ResolveReportDirectory(const std::wstring& appDir,bool winPE);
std::wstring SaveHtmlReport(const AuditReport& report,const std::wstring& outputDir);
std::wstring SaveJsonReport(const AuditReport& report,const std::wstring& outputDir);
}
