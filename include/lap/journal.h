#pragma once
#include <string>

namespace lap {

struct InterruptedSessionInfo {
    bool present{false};
    std::wstring sessionId;
    std::wstring stage;
    std::wstring status;
    std::wstring stageStatus;
    std::wstring time;
    std::wstring rawEvidence;
    std::wstring journalPath;
};

std::wstring StressJournalPath(const std::wstring& appDir);
bool DetectInterruptedStressJournal(const std::wstring& appDir,std::wstring& evidence);
InterruptedSessionInfo ReadInterruptedStressJournal(const std::wstring& appDir);
bool WriteStressJournal(const std::wstring& appDir,const std::wstring& sessionId,const std::wstring& stage,const std::wstring& status);
bool DiscardInterruptedStressJournal(const std::wstring& appDir);
void CompleteStressJournal(const std::wstring& appDir);

} // namespace lap
