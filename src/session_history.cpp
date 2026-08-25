#include "lap/session_history.h"
#include "lap/journal.h"
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <climits>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace lap {
namespace {
std::mutex gHistoryMutex;
std::vector<SessionHistoryEntry> gHistory;
std::wstring gHistoryDir;
bool gHistoryLoadValid{true};

constexpr int kHistorySchemaVersion = 1;
constexpr uintmax_t kMaxHistoryFileBytes = 4u * 1024u * 1024u;
constexpr size_t kMaxHistoryLineBytes = 64u * 1024u;
constexpr size_t kMaxHistoryFieldChars = 32u * 1024u;
constexpr size_t kMaxSessionIdChars = 256u;
constexpr size_t kMaxHistoryEntries = 4096u;

#ifdef LAPSURE_ENABLE_TEST_HOOKS
std::atomic<SessionHistoryFault> gHistoryFault{SessionHistoryFault::None};
bool FailIndexWriteInjected() { return gHistoryFault.load(std::memory_order_relaxed) == SessionHistoryFault::FailIndexWrite; }
bool FailIndexPublishInjected() { return gHistoryFault.load(std::memory_order_relaxed) == SessionHistoryFault::FailIndexPublish; }
bool FailArtifactMoveInjected() { return gHistoryFault.load(std::memory_order_relaxed) == SessionHistoryFault::FailArtifactMove; }
#else
bool FailIndexWriteInjected() { return false; }
bool FailIndexPublishInjected() { return false; }
bool FailArtifactMoveInjected() { return false; }
#endif

std::string HistoryHeader() {
    return std::string("#LapSureSessionHistory\t") + std::to_string(kHistorySchemaVersion);
}

std::wstring NowIso() {
    SYSTEMTIME time{};
    GetLocalTime(&time);
    wchar_t buffer[64]{};
    swprintf_s(buffer, L"%04u-%02u-%02u %02u:%02u:%02u",
               time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
    return buffer;
}

std::wstring SafeField(std::wstring value) {
    for (auto& c : value) if (c == L'\t' || c == L'\r' || c == L'\n') c = L' ';
    return value;
}

std::string ToUtf8(const std::wstring& value) {
    if (value.empty()) return {};
    if (value.size() > static_cast<size_t>(INT_MAX)) return {};
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (count <= 0) return {};
    std::string out(static_cast<size_t>(count), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
                            out.data(), count, nullptr, nullptr) != count) return {};
    return out;
}

std::wstring FromUtf8(const std::string& value) {
    if (value.empty()) return {};
    if (value.size() > static_cast<size_t>(INT_MAX)) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0) return {};
    std::wstring out(static_cast<size_t>(count), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
                            out.data(), count) != count) return {};
    return out;
}

std::filesystem::path IndexPath() {
    return std::filesystem::path(gHistoryDir) / L"session_history.tsv";
}

std::vector<std::wstring> SplitTabs(const std::wstring& line) {
    std::vector<std::wstring> parts;
    size_t start = 0;
    while (true) {
        const auto pos = line.find(L'\t', start);
        parts.push_back(line.substr(start, pos == std::wstring::npos ? pos : pos - start));
        if (pos == std::wstring::npos) break;
        start = pos + 1;
    }
    return parts;
}

bool ValidSessionId(const std::wstring& id) {
    if (id.empty() || id.size() > kMaxSessionIdChars) return false;
    for (wchar_t c : id) {
        const bool asciiAlphaNum = (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9');
        if (!asciiAlphaNum && c != L'-' && c != L'_' && c != L'.') return false;
    }
    return true;
}

bool ValidVerdict(const std::wstring& verdict) {
    return verdict == L"BUY" || verdict == L"BUY WITH NOTES" || verdict == L"INCOMPLETE" || verdict == L"REJECT";
}

bool ValidStatus(const std::wstring& status) {
    return status == L"COMPLETE" || status == L"ARTIFACT_PARTIAL" || status == L"INTERRUPTED" || status == L"INCOMPLETE";
}

bool ValidateEntry(const SessionHistoryEntry& entry) {
    if (!ValidSessionId(entry.sessionId)) return false;
    const std::wstring* fields[] = {
        &entry.timestamp, &entry.model, &entry.serviceTag, &entry.verdict, &entry.status,
        &entry.htmlPath, &entry.jsonPath, &entry.evidencePath, &entry.note,
    };
    for (const auto* field : fields) if (field->size() > kMaxHistoryFieldChars) return false;
    if (!ValidVerdict(entry.verdict) || !ValidStatus(entry.status)) return false;
    if (entry.status == L"COMPLETE" && (entry.htmlPath.empty() || entry.jsonPath.empty())) return false;
    return true;
}

bool ValidateCandidate(const std::vector<SessionHistoryEntry>& candidate) {
    if (candidate.size() > kMaxHistoryEntries) return false;
    std::unordered_set<std::wstring> ids;
    ids.reserve(candidate.size());
    for (const auto& entry : candidate) {
        if (!ValidateEntry(entry) || !ids.insert(entry.sessionId).second) return false;
    }
    return true;
}

bool ParseEntry(const std::wstring& line, SessionHistoryEntry& entry) {
    auto parts = SplitTabs(line);
    if (parts.size() != 10 || parts[0].empty()) return false;
    entry.sessionId = std::move(parts[0]);
    entry.timestamp = std::move(parts[1]);
    entry.model = std::move(parts[2]);
    entry.serviceTag = std::move(parts[3]);
    entry.verdict = std::move(parts[4]);
    entry.status = std::move(parts[5]);
    entry.htmlPath = std::move(parts[6]);
    entry.jsonPath = std::move(parts[7]);
    entry.evidencePath = std::move(parts[8]);
    entry.note = std::move(parts[9]);
    return ValidateEntry(entry);
}

std::vector<SessionHistoryEntry> LoadIndexLocked(bool& valid) {
    valid = true;
    std::vector<SessionHistoryEntry> loaded;
    if (gHistoryDir.empty()) return loaded;

    const auto path = IndexPath();
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) return loaded;
    if (ec) { valid = false; return {}; }
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size > kMaxHistoryFileBytes) { valid = false; return {}; }

    std::ifstream in(path, std::ios::binary);
    if (!in) { valid = false; return {}; }
    std::string bytesLine;
    bool firstContentLine = true;
    std::unordered_set<std::wstring> ids;
    while (std::getline(in, bytesLine)) {
        if (bytesLine.size() > kMaxHistoryLineBytes) { valid = false; return {}; }
        if (!bytesLine.empty() && bytesLine.back() == '\r') bytesLine.pop_back();
        if (bytesLine.empty()) continue;

        if (firstContentLine) {
            firstContentLine = false;
            constexpr const char* prefix = "#LapSureSessionHistory\t";
            if (bytesLine.rfind(prefix, 0) == 0) {
                if (bytesLine != HistoryHeader()) { valid = false; return {}; }
                continue;
            }
        }

        const auto wide = FromUtf8(bytesLine);
        if (wide.empty() && !bytesLine.empty()) { valid = false; return {}; }
        SessionHistoryEntry entry;
        if (!ParseEntry(wide, entry) || !ids.insert(entry.sessionId).second) { valid = false; return {}; }
        loaded.push_back(std::move(entry));
        if (loaded.size() > kMaxHistoryEntries) { valid = false; return {}; }
    }
    if (!in.eof() && in.fail()) { valid = false; return {}; }
    return loaded;
}

bool PersistIndexLocked(const std::vector<SessionHistoryEntry>& candidate) {
    if (gHistoryDir.empty() || !ValidateCandidate(candidate)) return false;

    std::error_code ec;
    std::filesystem::create_directories(gHistoryDir, ec);
    if (ec) return false;

    const auto target = IndexPath();
    const auto temp = target.wstring() + L".tmp";
    if (FailIndexWriteInjected()) return false;

    std::ofstream out(std::filesystem::path(temp), std::ios::binary | std::ios::trunc);
    if (!out) return false;
    uintmax_t written = 0;
    auto writeBytes = [&](const std::string& bytes) -> bool {
        written += bytes.size();
        if (written > kMaxHistoryFileBytes) return false;
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        return out.good();
    };

    if (!writeBytes(HistoryHeader() + "\n")) {
        out.close(); DeleteFileW(temp.c_str()); return false;
    }
    for (const auto& entry : candidate) {
        const std::wstring line = SafeField(entry.sessionId) + L"\t" + SafeField(entry.timestamp) + L"\t" +
            SafeField(entry.model) + L"\t" + SafeField(entry.serviceTag) + L"\t" + SafeField(entry.verdict) + L"\t" +
            SafeField(entry.status) + L"\t" + SafeField(entry.htmlPath) + L"\t" + SafeField(entry.jsonPath) + L"\t" +
            SafeField(entry.evidencePath) + L"\t" + SafeField(entry.note) + L"\n";
        const auto bytes = ToUtf8(line);
        if ((bytes.empty() && !line.empty()) || bytes.size() > kMaxHistoryLineBytes) {
            out.close(); DeleteFileW(temp.c_str()); return false;
        }
        if (!writeBytes(bytes)) { out.close(); DeleteFileW(temp.c_str()); return false; }
    }
    out.flush();
    if (!out.good()) { out.close(); DeleteFileW(temp.c_str()); return false; }
    out.close();

    if (FailIndexPublishInjected()) { DeleteFileW(temp.c_str()); return false; }
    if (!MoveFileExW(temp.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temp.c_str());
        return false;
    }
    return true;
}

SessionHistoryEntry* FindIn(std::vector<SessionHistoryEntry>& items, const std::wstring& id) {
    for (auto& entry : items) if (entry.sessionId == id) return &entry;
    return nullptr;
}

std::wstring LowerPath(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    return value;
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

    const auto ext = LowerPath(candidate.extension().wstring());
    if (ext != L".html" && ext != L".json" && ext != L".txt") return false;
    std::wstring rootText = LowerPath(root.wstring());
    const std::wstring candidateText = LowerPath(candidate.wstring());
    if (!rootText.empty() && rootText.back() != L'\\' && rootText.back() != L'/') rootText.push_back(L'\\');
    return candidateText.size() > rootText.size() && candidateText.compare(0, rootText.size(), rootText) == 0;
}

std::wstring FallbackSessionId(const std::wstring& artifactPath) {
    const auto stem = std::filesystem::path(artifactPath).stem().wstring();
    if (ValidSessionId(stem)) return stem;
    SYSTEMTIME time{};
    GetLocalTime(&time);
    wchar_t buffer[96]{};
    swprintf_s(buffer, L"session_%04u%02u%02u_%02u%02u%02u_%lu",
               time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, GetCurrentProcessId());
    return buffer;
}

std::wstring SafeEvidenceName(const std::wstring& sessionId) {
    if (ValidSessionId(sessionId)) return sessionId;
    return FallbackSessionId(L"");
}

bool WriteEvidenceFile(const std::filesystem::path& target, const std::wstring& evidence) {
    const auto bytes = ToUtf8(evidence);
    if (bytes.empty() && !evidence.empty()) return false;
    const auto temp = target.wstring() + L".tmp";
    std::ofstream out(std::filesystem::path(temp), std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    out.flush();
    if (!out.good()) { out.close(); DeleteFileW(temp.c_str()); return false; }
    out.close();
    if (!MoveFileExW(temp.c_str(), target.wstring().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(temp.c_str());
        return false;
    }
    return true;
}

struct MovedArtifact {
    std::filesystem::path original;
    std::filesystem::path quarantine;
};

bool RollbackMovedArtifacts(const std::vector<MovedArtifact>& moved) {
    bool allRestored = true;
    for (auto it = moved.rbegin(); it != moved.rend(); ++it) {
        if (!MoveFileExW(it->quarantine.wstring().c_str(), it->original.wstring().c_str(), MOVEFILE_WRITE_THROUGH)) {
            allRestored = false;
        }
    }
    return allRestored;
}
} // namespace

void InitializeSessionHistory(const std::wstring& outputDir) {
    std::lock_guard<std::mutex> lock(gHistoryMutex);
    gHistoryDir = outputDir;
    bool valid = true;
    auto loaded = LoadIndexLocked(valid);
    gHistoryLoadValid = valid;
    gHistory = valid ? std::move(loaded) : std::vector<SessionHistoryEntry>{};
}

std::vector<SessionHistoryEntry> GetSessionHistorySnapshot() {
    std::lock_guard<std::mutex> lock(gHistoryMutex);
    return gHistory;
}

bool IsTrustedSessionArtifactPath(const std::wstring& artifactPath) {
    std::lock_guard<std::mutex> lock(gHistoryMutex);
    return IsTrustedArtifactPathLocked(artifactPath);
}

bool CommitSessionHistoryBundle(const AuditReport& report, const std::wstring& htmlPath, const std::wstring& jsonPath) {
    if (htmlPath.empty() && jsonPath.empty()) return false;
    const std::wstring seed = !htmlPath.empty() ? htmlPath : jsonPath;
    const auto dir = std::filesystem::path(seed).parent_path().wstring();

    std::lock_guard<std::mutex> lock(gHistoryMutex);
    if (gHistoryDir.empty()) {
        gHistoryDir = dir;
        bool valid = true;
        gHistory = LoadIndexLocked(valid);
        gHistoryLoadValid = valid;
    }
    if (!gHistoryLoadValid) return false;
    if ((!htmlPath.empty() && !IsTrustedArtifactPathLocked(htmlPath)) ||
        (!jsonPath.empty() && !IsTrustedArtifactPathLocked(jsonPath))) return false;

    auto candidate = gHistory;
    const std::wstring id = report.hardware.stress.sessionId.empty() ? FallbackSessionId(seed) : report.hardware.stress.sessionId;
    auto* entry = FindIn(candidate, id);
    if (!entry) {
        SessionHistoryEntry item;
        item.sessionId = id;
        item.timestamp = NowIso();
        candidate.insert(candidate.begin(), std::move(item));
        entry = &candidate.front();
    }
    entry->model = report.model;
    entry->serviceTag = report.serviceTag;
    entry->verdict = report.hardware.stress.decision.overall;
    if (!htmlPath.empty()) entry->htmlPath = htmlPath;
    if (!jsonPath.empty()) entry->jsonPath = jsonPath;
    const bool pairComplete = !entry->htmlPath.empty() && !entry->jsonPath.empty();
    entry->status = pairComplete ? L"COMPLETE" : L"ARTIFACT_PARTIAL";
    entry->note = pairComplete ? L"" : L"Report bundle chưa đầy đủ; không được coi là phiên đã persist hoàn chỉnh.";

    if (!PersistIndexLocked(candidate)) return false;
    gHistory.swap(candidate);
    gHistoryLoadValid = true;
    return true;
}

void RecordSessionHistoryArtifact(const AuditReport& report, const std::wstring& artifactPath, bool isHtml) {
    if (artifactPath.empty()) return;
    if (isHtml) (void)CommitSessionHistoryBundle(report, artifactPath, L"");
    else (void)CommitSessionHistoryBundle(report, L"", artifactPath);
}

bool ArchiveInterruptedSession(const std::wstring& stateRoot, const std::wstring& outputDir) {
    const auto info = ReadInterruptedStressJournal(stateRoot);
    if (!info.present) return false;
    InitializeSessionHistory(outputDir);

    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);
    if (ec) return false;
    const std::wstring id = ValidSessionId(info.sessionId) ? info.sessionId : FallbackSessionId(info.journalPath);
    const auto evidencePath = std::filesystem::path(outputDir) / (L"interrupted_" + SafeEvidenceName(id) + L".journal.txt");
    if (!WriteEvidenceFile(evidencePath, info.rawEvidence)) return false;

    {
        std::lock_guard<std::mutex> lock(gHistoryMutex);
        if (!gHistoryLoadValid) { std::filesystem::remove(evidencePath, ec); return false; }
        auto candidate = gHistory;
        auto* entry = FindIn(candidate, id);
        if (!entry) {
            SessionHistoryEntry item;
            item.sessionId = id;
            item.timestamp = info.time.empty() ? NowIso() : info.time;
            candidate.insert(candidate.begin(), std::move(item));
            entry = &candidate.front();
        }
        entry->verdict = L"INCOMPLETE";
        entry->status = L"INTERRUPTED";
        entry->evidencePath = evidencePath.wstring();
        entry->note = L"Phiên bị gián đoạn; journal được lưu làm bằng chứng. Không có PASS từ phiên này.";
        if (!PersistIndexLocked(candidate)) {
            std::filesystem::remove(evidencePath, ec);
            return false;
        }
        gHistory.swap(candidate);
        gHistoryLoadValid = true;
    }
    return DiscardInterruptedStressJournal(stateRoot);
}

bool DeleteSessionHistoryEntry(const std::wstring& sessionId, bool deleteArtifacts) {
    std::lock_guard<std::mutex> lock(gHistoryMutex);
    if (!gHistoryLoadValid || !ValidSessionId(sessionId)) return false;
    const auto current = std::find_if(gHistory.begin(), gHistory.end(), [&](const SessionHistoryEntry& entry) {
        return entry.sessionId == sessionId;
    });
    if (current == gHistory.end()) return false;

    std::vector<std::filesystem::path> artifactPaths;
    if (deleteArtifacts) {
        const std::wstring paths[] = {current->htmlPath, current->jsonPath, current->evidencePath};
        for (const auto& path : paths) {
            if (path.empty()) continue;
            if (!IsTrustedArtifactPathLocked(path)) return false;
            artifactPaths.emplace_back(path);
        }
    }

    auto candidate = gHistory;
    candidate.erase(candidate.begin() + std::distance(gHistory.begin(), current));

    if (!deleteArtifacts) {
        if (!PersistIndexLocked(candidate)) return false;
        gHistory.swap(candidate);
        gHistoryLoadValid = true;
        return true;
    }

    std::error_code ec;
    const auto quarantine = std::filesystem::path(gHistoryDir) /
        (L".delete-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
    std::filesystem::create_directories(quarantine, ec);
    if (ec) return false;

    std::vector<MovedArtifact> moved;
    moved.reserve(artifactPaths.size());
    for (size_t i = 0; i < artifactPaths.size(); ++i) {
        if (FailArtifactMoveInjected()) {
            (void)RollbackMovedArtifacts(moved);
            std::filesystem::remove_all(quarantine, ec);
            return false;
        }
        const auto staged = quarantine / (std::to_wstring(i) + L"_" + artifactPaths[i].filename().wstring());
        if (!MoveFileExW(artifactPaths[i].wstring().c_str(), staged.wstring().c_str(), MOVEFILE_WRITE_THROUGH)) {
            (void)RollbackMovedArtifacts(moved);
            std::filesystem::remove_all(quarantine, ec);
            return false;
        }
        moved.push_back({artifactPaths[i], staged});
    }

    if (!PersistIndexLocked(candidate)) {
        const bool restored = RollbackMovedArtifacts(moved);
        if (restored) std::filesystem::remove_all(quarantine, ec);
        return false;
    }

    gHistory.swap(candidate);
    gHistoryLoadValid = true;
    std::filesystem::remove_all(quarantine, ec); // best-effort cleanup after durable logical deletion
    return true;
}

#ifdef LAPSURE_ENABLE_TEST_HOOKS
void SetSessionHistoryFaultForTesting(SessionHistoryFault fault) {
    gHistoryFault.store(fault, std::memory_order_relaxed);
}
#endif

} // namespace lap
