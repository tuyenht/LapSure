#include "lap/journal.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <sstream>
namespace lap {
std::wstring StressJournalPath(const std::wstring&appDir){
    auto p=std::filesystem::path(appDir)/L"reports";std::error_code ec;std::filesystem::create_directories(p,ec);return (p/L"stress_session.journal").wstring();
}
bool DetectInterruptedStressJournal(const std::wstring&appDir,std::wstring&evidence){
    std::wifstream f(StressJournalPath(appDir));if(!f)return false;std::wstringstream s;s<<f.rdbuf();evidence=s.str();return evidence.find(L"status=RUNNING")!=std::wstring::npos;
}
bool WriteStressJournal(const std::wstring&appDir,const std::wstring&id,const std::wstring&stage,const std::wstring&status){
    const auto target=StressJournalPath(appDir),temp=target+L".tmp";std::wofstream f(temp,std::ios::trunc);if(!f)return false;
    SYSTEMTIME t{};GetLocalTime(&t);f<<L"session="<<id<<L"\nstage="<<stage<<L"\nstatus="<<status<<L"\ntime="<<t.wYear<<L"-"<<t.wMonth<<L"-"<<t.wDay<<L" "<<t.wHour<<L":"<<t.wMinute<<L":"<<t.wSecond<<L"\n";f.flush();if(!f.good()){f.close();DeleteFileW(temp.c_str());return false;}f.close();if(!MoveFileExW(temp.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){DeleteFileW(temp.c_str());return false;}return true;
}
void CompleteStressJournal(const std::wstring&appDir){std::error_code ec;std::filesystem::remove(StressJournalPath(appDir),ec);}
}
