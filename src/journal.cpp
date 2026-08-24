#include "lap/journal.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace lap {
namespace {
std::wstring ValueOf(const std::wstring& raw, const wchar_t* key) {
    const std::wstring prefix = std::wstring(key) + L"=";
    std::wistringstream in(raw);
    std::wstring line;
    while (std::getline(in, line)) {
        if (line.rfind(prefix, 0) == 0) return line.substr(prefix.size());
    }
    return L"";
}
}

std::wstring StressJournalPath(const std::wstring& appDir) {
    auto p = std::filesystem::path(appDir) / L"reports";
    std::error_code ec;
    std::filesystem::create_directories(p, ec);
    return (p / L"stress_session.journal").wstring();
}

InterruptedSessionInfo ReadInterruptedStressJournal(const std::wstring& appDir) {
    InterruptedSessionInfo info{};
    info.journalPath = StressJournalPath(appDir);
    std::wifstream f(info.journalPath);
    if (!f) return info;
    std::wstringstream s;
    s << f.rdbuf();
    info.rawEvidence = s.str();
    info.sessionId = ValueOf(info.rawEvidence, L"session");
    info.stage = ValueOf(info.rawEvidence, L"stage");
    info.status = ValueOf(info.rawEvidence, L"status");
    info.stageStatus = ValueOf(info.rawEvidence, L"stage_status");
    info.time = ValueOf(info.rawEvidence, L"time");
    info.present = (info.status == L"RUNNING");
    return info;
}

bool DetectInterruptedStressJournal(const std::wstring& appDir, std::wstring& evidence) {
    const auto info = ReadInterruptedStressJournal(appDir);
    evidence = info.rawEvidence;
    return info.present;
}

bool WriteStressJournal(const std::wstring& appDir, const std::wstring& id, const std::wstring& stage, const std::wstring& status) {
    const auto target = StressJournalPath(appDir), temp = target + L".tmp";
    std::wofstream f(temp, std::ios::trunc);
    if (!f) return false;
    SYSTEMTIME t{};
    GetLocalTime(&t);
    f << L"session=" << id << L"\nstage=" << stage << L"\nstatus=RUNNING"
      << L"\nstage_status=" << status
      << L"\ntime=" << t.wYear << L"-" << t.wMonth << L"-" << t.wDay << L" "
      << t.wHour << L":" << t.wMinute << L":" << t.wSecond << L"\n";
    f.flush();
    if (!f.good()) {
        f.close();
        DeleteFileW(temp.c_str());
        return false;
    }
    f.close();
    if (!MoveFileExW(temp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temp.c_str());
        return false;
    }
    return true;
}

bool DiscardInterruptedStressJournal(const std::wstring& appDir) {
    std::error_code ec;
    const auto p = std::filesystem::path(StressJournalPath(appDir));
    if (!std::filesystem::exists(p, ec)) return true;
    return std::filesystem::remove(p, ec) && !ec;
}

void CompleteStressJournal(const std::wstring& appDir) {
    std::error_code ec;
    std::filesystem::remove(StressJournalPath(appDir), ec);
}

} // namespace lap
