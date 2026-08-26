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

// stateRoot is an explicit writable/persistent state directory. Callers decide
// where it lives; journal code never appends an implicit application subfolder.
std::wstring StressJournalPath(const std::wstring& stateRoot);
bool DetectInterruptedStressJournal(const std::wstring& stateRoot,std::wstring& evidence);
InterruptedSessionInfo ReadInterruptedStressJournal(const std::wstring& stateRoot);
bool WriteStressJournal(const std::wstring& stateRoot,const std::wstring& sessionId,const std::wstring& stage,const std::wstring& status);
bool DiscardInterruptedStressJournal(const std::wstring& stateRoot);
void CompleteStressJournal(const std::wstring& stateRoot);

} // namespace lap
