#include "lap/session_history.h"
#include "lap/journal.h"
#include <windows.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>

namespace lap {
namespace {
std::mutex gHistoryMutex;
std::vector<SessionHistoryEntry> gHistory;
std::wstring gHistoryDir;

std::wstring NowIso() {
    SYSTEMTIME t{};
    GetLocalTime(&t);
    wchar_t b[64]{};
    swprintf_s(b, L"%04u-%02u-%02u %02u:%02u:%02u", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    return b;
}

std::wstring SafeField(std::wstring s) {
    for (auto& c : s) if (c == L'\t' || c == L'\r' || c == L'\n') c = L' ';
    return s;
}

std::string ToUtf8(const std::wstring& s) {
    if (s.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string out(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), out.data(), n, nullptr, nullptr);
    return out;
}

std::wstring FromUtf8(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0) return {};
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}

std::filesystem::path IndexPath() {
    return std::filesystem::path(gHistoryDir) / L"session_history.tsv";
}

std::vector<std::wstring> SplitTabs(const std::wstring& line) {
    std::vector<std::wstring> parts;
    size_t start = 0;
    while (true) {
        const auto p = line.find(L'\t', start);
        parts.push_back(line.substr(start, p == std::wstring::npos ? p : p - start));
        if (p == std::wstring::npos) break;
        start = p + 1;
    }
    return parts;
}

bool SaveIndexLocked() {
    if (gHistoryDir.empty()) return false;
    std::error_code ec;
    std::filesystem::create_directories(gHistoryDir, ec);
    if (ec) return false;
    const auto target = IndexPath();
    const auto temp = target.wstring() + L".tmp";
    std::ofstream out(std::filesystem::path(temp), std::ios::binary | std::ios::trunc);
    if (!out) return false;
    for (const auto& e : gHistory) {
        std::wstring line = SafeField(e.sessionId) + L"\t" + SafeField(e.timestamp) + L"\t" + SafeField(e.model) + L"\t" +
            SafeField(e.serviceTag) + L"\t" + SafeField(e.verdict) + L"\t" + SafeField(e.status) + L"\t" +
            SafeField(e.htmlPath) + L"\t" + SafeField(e.jsonPath) + L"\t" + SafeField(e.evidencePath) + L"\t" + SafeField(e.note) + L"\n";
        const auto bytes = ToUtf8(line);
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
    out.flush();
    if (!out.good()) return false;
    out.close();
    return MoveFileExW(temp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
}

void LoadIndexLocked() {
    gHistory.clear();
    if (gHistoryDir.empty()) return;
    std::ifstream in(IndexPath(), std::ios::binary);
    if (!in) return;
    std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::wistringstream rows(FromUtf8(bytes));
    std::wstring line;
    while (std::getline(rows, line)) {
        auto p = SplitTabs(line);
        if (p.size() < 10 || p[0].empty()) continue;
        SessionHistoryEntry e;
        e.sessionId = p[0]; e.timestamp = p[1]; e.model = p[2]; e.serviceTag = p[3]; e.verdict = p[4]; e.status = p[5];
        e.htmlPath = p[6]; e.jsonPath = p[7]; e.evidencePath = p[8]; e.note = p[9];
        gHistory.push_back(std::move(e));
    }
}

SessionHistoryEntry* FindLocked(const std::wstring& id) {
    for (auto& e : gHistory) if (e.sessionId == id) return &e;
    return nullptr;
}

std::wstring LowerPath(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    return s;
}

bool IsTrustedArtifactPathLocked(const std::wstring& artifactPath) {
    if (artifactPath.empty() || gHistoryDir.empty()) return false;
    std::error_code ec;
    auto root = std::filesystem::weakly_canonical(std::filesystem::path(gHistoryDir), ec);
    if (ec || root.empty()) return false;
    ec.clear();
    auto candidate = std::filesystem::weakly_canonical(std::filesystem::path(artifactPath), ec);
    if (ec || candidate.empty() || candidate == root) return false;

    ec.clear();
    if (!std::filesystem::is_regular_file(candidate, ec) || ec) return false;

    auto ext = LowerPath(candidate.extension().wstring());
    if (ext != L".html" && ext != L".json" && ext != L".txt") return false;

    std::wstring rootText = LowerPath(root.wstring());
    std::wstring candidateText = LowerPath(candidate.wstring());
    if (!rootText.empty() && rootText.back() != L'\\' && rootText.back() != L'/') rootText.push_back(L'\\');
    return candidateText.size() > rootText.size() && candidateText.compare(0, rootText.size(), rootText) == 0;
}

std::wstring FallbackSessionId(const std::wstring& artifactPath) {
    const auto stem = std::filesystem::path(artifactPath).stem().wstring();
    if (!stem.empty()) return stem;
    SYSTEMTIME t{}; GetLocalTime(&t);
    wchar_t b[64]{};
    swprintf_s(b, L"session_%04u%02u%02u_%02u%02u%02u_%lu", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, GetCurrentProcessId());
    return b;
}
}

void InitializeSessionHistory(const std::wstring& outputDir) {
    std::lock_guard<std::mutex> lk(gHistoryMutex);
    if (outputDir == gHistoryDir && !gHistoryDir.empty()) return;
    gHistoryDir = outputDir;
    LoadIndexLocked();
}

std::vector<SessionHistoryEntry> GetSessionHistorySnapshot() {
    std::lock_guard<std::mutex> lk(gHistoryMutex);
    return gHistory;
}

bool IsTrustedSessionArtifactPath(const std::wstring& artifactPath) {
    std::lock_guard<std::mutex> lk(gHistoryMutex);
    return IsTrustedArtifactPathLocked(artifactPath);
}

void RecordSessionHistoryArtifact(const AuditReport& report, const std::wstring& artifactPath, bool isHtml) {
    if (artifactPath.empty()) return;
    const auto dir = std::filesystem::path(artifactPath).parent_path().wstring();
    InitializeSessionHistory(dir);
    std::lock_guard<std::mutex> lk(gHistoryMutex);
    if (!IsTrustedArtifactPathLocked(artifactPath)) return;
    std::wstring id = report.hardware.stress.sessionId.empty() ? FallbackSessionId(artifactPath) : report.hardware.stress.sessionId;
    auto* e = FindLocked(id);
    if (!e) {
        SessionHistoryEntry item;
        item.sessionId = id;
        item.timestamp = NowIso();
        gHistory.insert(gHistory.begin(), std::move(item));
        e = &gHistory.front();
    }
    e->model = report.model;
    e->serviceTag = report.serviceTag;
    e->verdict = report.hardware.stress.decision.overall;
    e->status = report.hardware.stress.decision.overall == L"INCOMPLETE" ? L"INCOMPLETE" : L"COMPLETE";
    if (isHtml) e->htmlPath = artifactPath; else e->jsonPath = artifactPath;
    SaveIndexLocked();
}

bool ArchiveInterruptedSession(const std::wstring& appDir, const std::wstring& outputDir) {
    const auto info = ReadInterruptedStressJournal(appDir);
    if (!info.present) return false;
    InitializeSessionHistory(outputDir);
    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);
    if (ec) return false;
    const std::wstring id = info.sessionId.empty() ? FallbackSessionId(info.journalPath) : info.sessionId;
    const auto evidencePath = (std::filesystem::path(outputDir) / (L"interrupted_" + id + L".journal.txt")).wstring();
    std::ofstream out(std::filesystem::path(evidencePath), std::ios::binary | std::ios::trunc);
    if (!out) return false;
    const auto bytes = ToUtf8(info.rawEvidence);
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    out.flush();
    if (!out.good()) return false;
    out.close();

    {
        std::lock_guard<std::mutex> lk(gHistoryMutex);
        auto* e = FindLocked(id);
        if (!e) {
            SessionHistoryEntry item;
            item.sessionId = id;
            item.timestamp = info.time.empty() ? NowIso() : info.time;
            gHistory.insert(gHistory.begin(), std::move(item));
            e = &gHistory.front();
        }
        e->verdict = L"INCOMPLETE";
        e->status = L"INTERRUPTED";
        e->evidencePath = evidencePath;
        e->note = L"Phiên bị gián đoạn; journal được lưu làm bằng chứng. Không có PASS từ phiên này.";
        if (!SaveIndexLocked()) return false;
    }
    return DiscardInterruptedStressJournal(appDir);
}

bool DeleteSessionHistoryEntry(const std::wstring& sessionId, bool deleteArtifacts) {
    std::lock_guard<std::mutex> lk(gHistoryMutex);
    auto it = std::find_if(gHistory.begin(), gHistory.end(), [&](const SessionHistoryEntry& e){ return e.sessionId == sessionId; });
    if (it == gHistory.end()) return false;
    if (deleteArtifacts) {
        const std::wstring paths[] = {it->htmlPath, it->jsonPath, it->evidencePath};
        for (const auto& path : paths) {
            if (!path.empty() && !IsTrustedArtifactPathLocked(path)) return false;
        }
        for (const auto& path : paths) {
            if (path.empty()) continue;
            std::error_code ec;
            if (!std::filesystem::remove(std::filesystem::path(path), ec) || ec) return false;
        }
    }
    gHistory.erase(it);
    return SaveIndexLocked();
}

} // namespace lap
