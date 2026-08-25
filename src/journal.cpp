#include "lap/journal.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace lap {
namespace {
std::filesystem::path gLastJournalPath;

std::wstring ValueOf(const std::wstring& raw, const wchar_t* key) {
    const std::wstring prefix = std::wstring(key) + L"=";
    std::wistringstream in(raw);
    std::wstring line;
    while (std::getline(in, line)) {
        if (line.rfind(prefix, 0) == 0) return line.substr(prefix.size());
    }
    return L"";
}

std::filesystem::path JournalPath(const std::wstring& stateRoot) {
    if (stateRoot.empty()) return {};
    auto root = std::filesystem::path(stateRoot);
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    if (ec) return {};
    return root / L"stress_session.journal";
}

std::filesystem::path LegacyJournalPath(const std::wstring& root) {
    if (root.empty()) return {};
    return std::filesystem::path(root) / L"reports" / L"stress_session.journal";
}

InterruptedSessionInfo ReadJournalFile(const std::filesystem::path& path) {
    InterruptedSessionInfo info{};
    if (path.empty()) return info;
    info.journalPath = path.wstring();
    std::wifstream f(path);
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

bool RemoveIfPresent(const std::filesystem::path& path) {
    if (path.empty()) return true;
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) return !ec;
    return std::filesystem::remove(path, ec) && !ec;
}
}

std::wstring StressJournalPath(const std::wstring& stateRoot) {
    const auto path = JournalPath(stateRoot);
    return path.empty() ? L"" : path.wstring();
}

InterruptedSessionInfo ReadInterruptedStressJournal(const std::wstring& stateRoot) {
    auto info = ReadJournalFile(JournalPath(stateRoot));
    if (!info.rawEvidence.empty()) return info;

    // Transitional read compatibility for journals created before Round 5 when
    // callers passed the application root and journal.cpp appended "reports".
    return ReadJournalFile(LegacyJournalPath(stateRoot));
}

bool DetectInterruptedStressJournal(const std::wstring& stateRoot, std::wstring& evidence) {
    const auto info = ReadInterruptedStressJournal(stateRoot);
    evidence = info.rawEvidence;
    return info.present;
}

bool WriteStressJournal(const std::wstring& stateRoot, const std::wstring& id, const std::wstring& stage, const std::wstring& status) {
    const auto targetPath = JournalPath(stateRoot);
    if (targetPath.empty()) return false;
    const auto target = targetPath.wstring(), temp = target + L".tmp";
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
    gLastJournalPath = targetPath;
    return true;
}

bool DiscardInterruptedStressJournal(const std::wstring& stateRoot) {
    const bool direct = RemoveIfPresent(JournalPath(stateRoot));
    const bool legacy = RemoveIfPresent(LegacyJournalPath(stateRoot));
    const bool last = RemoveIfPresent(gLastJournalPath);
    gLastJournalPath.clear();
    return direct && legacy && last;
}

void CompleteStressJournal(const std::wstring& stateRoot) {
    (void)RemoveIfPresent(JournalPath(stateRoot));
    (void)RemoveIfPresent(LegacyJournalPath(stateRoot));
    (void)RemoveIfPresent(gLastJournalPath);
    gLastJournalPath.clear();
}

} // namespace lap
