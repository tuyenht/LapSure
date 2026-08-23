#pragma once
#include <string>
namespace lap {
std::wstring StressJournalPath(const std::wstring& appDir);
bool DetectInterruptedStressJournal(const std::wstring& appDir,std::wstring& evidence);
bool WriteStressJournal(const std::wstring& appDir,const std::wstring& sessionId,const std::wstring& stage,const std::wstring& status);
void CompleteStressJournal(const std::wstring& appDir);
}
