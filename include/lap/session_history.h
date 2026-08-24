#pragma once
#include <string>
#include <vector>
#include "lap/model.h"

namespace lap {

struct SessionHistoryEntry {
    std::wstring sessionId;
    std::wstring timestamp;
    std::wstring model;
    std::wstring serviceTag;
    std::wstring verdict{L"INCOMPLETE"};
    std::wstring status{L"INCOMPLETE"};
    std::wstring htmlPath;
    std::wstring jsonPath;
    std::wstring evidencePath;
    std::wstring note;
};

void InitializeSessionHistory(const std::wstring& outputDir);
std::vector<SessionHistoryEntry> GetSessionHistorySnapshot();
void RecordSessionHistoryArtifact(const AuditReport& report, const std::wstring& artifactPath, bool isHtml);
bool ArchiveInterruptedSession(const std::wstring& appDir, const std::wstring& outputDir);
bool DeleteSessionHistoryEntry(const std::wstring& sessionId, bool deleteArtifacts);

} // namespace lap
