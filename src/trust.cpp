#include "lap/trust.h"
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <vector>
#pragma comment(lib,"bcrypt.lib")

namespace lap {
namespace {
std::wstring Lower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), towlower);
    return s;
}

bool IsSha256Hex(const std::wstring& value) {
    if (value.size() != 64) return false;
    return std::all_of(value.begin(), value.end(), [](wchar_t c) { return iswxdigit(c) != 0; });
}

void Trim(std::wstring& value) {
    const auto first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) { value.clear(); return; }
    const auto last = value.find_last_not_of(L" \t\r\n");
    value = value.substr(first, last - first + 1);
}

bool SamePathComponent(const std::filesystem::path& a, const std::filesystem::path& b) {
    return Lower(a.native()) == Lower(b.native());
}

bool IsWithinRoot(const std::filesystem::path& root, const std::filesystem::path& candidate) {
    auto r = root.begin(), c = candidate.begin();
    for (; r != root.end(); ++r, ++c) {
        if (c == candidate.end() || !SamePathComponent(*r, *c)) return false;
    }
    return true;
}

bool IsReparsePoint(const std::filesystem::path& path) {
    const DWORD attrs = GetFileAttributesW(path.wstring().c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

bool HasReparsePathComponents(const std::filesystem::path& absolutePath,
                              std::filesystem::path& offending) {
    const auto normalized = absolutePath.lexically_normal();
    auto current = normalized.root_path();
    if (!current.empty() && IsReparsePoint(current)) {
        offending = current;
        return true;
    }
    for (const auto& part : normalized.relative_path()) {
        if (part.empty() || part == L".") continue;
        current /= part;
        if (IsReparsePoint(current)) {
            offending = current;
            return true;
        }
    }
    return false;
}

bool HasReparseUnderRoot(const std::filesystem::path& root,
                         const std::filesystem::path& relative,
                         std::filesystem::path& offending) {
    if (IsReparsePoint(root)) { offending = root; return true; }
    auto current = root;
    for (const auto& part : relative) {
        if (part.empty() || part == L".") continue;
        current /= part;
        if (IsReparsePoint(current)) { offending = current; return true; }
    }
    return false;
}

#ifdef LAPSURE_ENABLE_TEST_HOOKS
std::mutex gTestProviderMutex;
std::vector<ProviderIdentity> gTestProviders;
#endif

std::wstring HashHandle(HANDLE hFile) {
    if (hFile == INVALID_HANDLE_VALUE || hFile == nullptr) return L"";
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return L"";
    DWORD objLen = 0, cb = 0, hashLen = 0;
    if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objLen), sizeof(objLen), &cb, 0) != 0 ||
        BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashLen), sizeof(hashLen), &cb, 0) != 0) {
        BCryptCloseAlgorithmProvider(alg, 0);
        return L"";
    }
    std::vector<UCHAR> obj(objLen), digest(hashLen);
    if (BCryptCreateHash(alg, &hash, obj.data(), objLen, nullptr, 0, 0) != 0) {
        BCryptCloseAlgorithmProvider(alg, 0);
        return L"";
    }

    SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);
    char buf[1 << 16];
    DWORD bytesRead = 0;
    while (ReadFile(hFile, buf, sizeof(buf), &bytesRead, nullptr) && bytesRead > 0) {
        if (BCryptHashData(hash, reinterpret_cast<PUCHAR>(buf), bytesRead, 0) != 0) {
            BCryptDestroyHash(hash);
            BCryptCloseAlgorithmProvider(alg, 0);
            return L"";
        }
    }
    if (BCryptFinishHash(hash, digest.data(), hashLen, 0) != 0) {
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(alg, 0);
        return L"";
    }
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(alg, 0);
    std::wstringstream ss;
    ss << std::hex << std::setfill(L'0');
    for (auto b : digest) ss << std::setw(2) << static_cast<unsigned>(b);
    return Lower(ss.str());
}

std::wstring HashFile(const std::wstring& path) {
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return L"";
    std::wstring digest = HashHandle(hFile);
    CloseHandle(hFile);
    return digest;
}
} // namespace

std::vector<ProviderIdentity> GetEmbeddedProviderCatalog() {
    return {
        {
            L"smartctl",
            L"tools\\smartctl.exe",
            L"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            L"7.4",
            L"GPL-2.0-or-later",
            {}
        },
        {
            L"nvidia_smi",
            L"tools\\nvidia-smi.exe",
            L"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            L"535.0",
            L"NVIDIA-Redistributable",
            {}
        },
        {
            L"lhm_bridge",
            L"tools\\sensors\\lhm_bridge.exe",
            L"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            L"1.0.0",
            L"MPL-2.0",
            {}
        },
        {
            L"memtest_vulkan",
            L"tools\\gpu\\memtest_vulkan.exe",
            L"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            L"1.0.0",
            L"GPL-3.0-or-later",
            {}
        },
        {
            L"vram_engine",
            L"tools\\vram_stress.exe",
            L"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            L"1.0.0",
            L"LapSure-Internal",
            {}
        }
    };
}

const ProviderIdentity* FindEmbeddedProvider(std::wstring_view logicalName) {
#ifdef LAPSURE_ENABLE_TEST_HOOKS
    {
        std::lock_guard<std::mutex> lock(gTestProviderMutex);
        for (const auto& item : gTestProviders) {
            if (Lower(item.logicalName) == Lower(std::wstring(logicalName))) {
                return &item;
            }
        }
    }
#endif
    static const auto catalog = GetEmbeddedProviderCatalog();
    for (const auto& item : catalog) {
        if (Lower(item.logicalName) == Lower(std::wstring(logicalName))) {
            return &item;
        }
    }
    return nullptr;
}

#ifdef LAPSURE_ENABLE_TEST_HOOKS
void RegisterTestProtectedProvider(const ProviderIdentity& identity) {
    std::lock_guard<std::mutex> lock(gTestProviderMutex);
    for (auto& item : gTestProviders) {
        if (Lower(item.logicalName) == Lower(identity.logicalName)) {
            item = identity;
            return;
        }
    }
    gTestProviders.push_back(identity);
}

void ClearTestProtectedProviders() {
    std::lock_guard<std::mutex> lock(gTestProviderMutex);
    gTestProviders.clear();
}
#endif

EngineTrust VerifyEngine(const std::wstring& appDir,
                         const std::wstring& relativePath,
                         const std::wstring& logicalName) {
    EngineTrust trust{};
    trust.logicalName = logicalName;
    namespace fs = std::filesystem;
    const fs::path rel(relativePath);
    if (appDir.empty() || logicalName.empty()) {
        trust.reason = L"Application root and engine logical name are required.";
        return trust;
    }
    if (relativePath.empty() || rel.is_absolute() || rel.has_root_name() || rel.has_root_directory()) {
        trust.reason = L"Engine path must be relative to the LapSure application root.";
        return trust;
    }
    for (const auto& part : rel) {
        if (part == L"..") {
            trust.reason = L"Engine path traversal outside the LapSure application root is blocked.";
            return trust;
        }
    }

    std::error_code ec;
    const auto absoluteRoot = fs::absolute(fs::path(appDir), ec);
    if (ec || absoluteRoot.empty()) {
        trust.reason = L"Application root resolution failed.";
        return trust;
    }

    fs::path offending;
    const auto normalizedAbsoluteRoot = absoluteRoot.lexically_normal();
    if (HasReparsePathComponents(normalizedAbsoluteRoot, offending)) {
        trust.reason = L"Application root contains path redirection/reparse semantics.";
        return trust;
    }

    ec.clear();
    const auto root = fs::weakly_canonical(normalizedAbsoluteRoot, ec);
    if (ec || root.empty()) {
        trust.reason = L"Application root canonicalization failed.";
        return trust;
    }

    if (HasReparseUnderRoot(root, rel, offending)) {
        trust.reason = L"Engine path contains a reparse point and is not trusted.";
        return trust;
    }

    ec.clear();
    const auto candidate = fs::weakly_canonical(root / rel, ec);
    if (ec || candidate.empty()) {
        trust.reason = L"Engine path canonicalization failed.";
        return trust;
    }
    if (!IsWithinRoot(root, candidate)) {
        trust.reason = L"Engine resolved outside the LapSure application root.";
        return trust;
    }
    if (!fs::is_regular_file(candidate, ec) || ec) {
        trust.reason = L"Engine file not found or is not a regular file.";
        return trust;
    }
    trust.fileExists = true;
    trust.resolvedPath = candidate.wstring();

    HANDLE hFile = CreateFileW(trust.resolvedPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        trust.reason = L"Cannot open engine file for verification.";
        return trust;
    }
    trust.sha256 = HashHandle(hFile);
    CloseHandle(hFile);

    if (trust.sha256.empty()) {
        trust.reason = L"SHA-256 calculation failed.";
        return trust;
    }

    const auto* embedded = FindEmbeddedProvider(logicalName);
    if (embedded) {
        trust.manifestEntry = true;
        trust.embeddedCatalogMatch = true;
        trust.expectedSha256 = Lower(embedded->expectedSha256);
        trust.version = embedded->version;
        trust.licenseId = embedded->licenseId;

        if (!IsSha256Hex(trust.expectedSha256)) {
            trust.reason = L"Allowlist SHA-256 is not configured or invalid.";
            return trust;
        }
        trust.hashMatches = Lower(trust.sha256) == trust.expectedSha256;
        trust.reason = trust.hashMatches ? L"Trusted engine hash matched." : L"Engine hash mismatch.";
        return trust;
    }

    // Fallback for non-embedded engines (e.g. legacy/development manifest lookup)
    const fs::path manifestRel = fs::path(L"tools") / L"engine_manifest.txt";
    if (HasReparseUnderRoot(root, manifestRel, offending)) {
        trust.reason = L"Trust manifest path contains a reparse point and is not trusted.";
        return trust;
    }
    ec.clear();
    const auto manifest = fs::weakly_canonical(root / manifestRel, ec);
    if (ec || manifest.empty() || !IsWithinRoot(root, manifest) || !fs::is_regular_file(manifest, ec) || ec) {
        trust.reason = L"Trust manifest missing, redirected, or not a regular file.";
        return trust;
    }

    std::wifstream file(manifest);
    if (!file) {
        trust.reason = L"Trust manifest missing.";
        return trust;
    }
    size_t logicalMatches = 0;
    std::wstring matchedHash;
    std::wstring line;
    while (std::getline(file, line)) {
        Trim(line);
        if (line.empty() || line[0] == L'#') continue;
        const auto separator = line.find(L'=');
        if (separator == std::wstring::npos) continue;
        auto name = line.substr(0, separator);
        auto expected = line.substr(separator + 1);
        Trim(name);
        Trim(expected);
        if (Lower(name) != Lower(logicalName)) continue;
        ++logicalMatches;
        if (logicalMatches == 1) matchedHash = Lower(expected);
    }

    if (logicalMatches == 0) {
        trust.reason = L"No allowlist entry for engine.";
        return trust;
    }
    if (logicalMatches > 1) {
        trust.reason = L"Allowlist contains duplicate entries for the engine logical name.";
        return trust;
    }

    trust.manifestEntry = true;
    trust.expectedSha256 = std::move(matchedHash);
    if (!IsSha256Hex(trust.expectedSha256)) {
        trust.reason = L"Allowlist SHA-256 is not configured or invalid.";
        return trust;
    }
    trust.hashMatches = Lower(trust.sha256) == Lower(trust.expectedSha256);
    trust.reason = trust.hashMatches ? L"Trusted engine hash matched." : L"Engine hash mismatch.";
    return trust;
}

TrustedEngineRun RunTrustedEngineCapture(const std::wstring& appDir,
                                         const std::wstring& relativePath,
                                         const std::wstring& logicalName,
                                         const std::vector<std::wstring>& arguments,
                                         unsigned timeoutMs,
                                         const std::atomic_bool* cancel) {
    TrustedEngineRun out{};
    namespace fs = std::filesystem;
    const fs::path rel(relativePath);
    if (appDir.empty() || logicalName.empty() || relativePath.empty() ||
        rel.is_absolute() || rel.has_root_name() || rel.has_root_directory()) {
        out.process.error = L"Trusted engine execution blocked: invalid arguments.";
        return out;
    }
    for (const auto& part : rel) {
        if (part == L"..") {
            out.process.error = L"Trusted engine execution blocked: traversal detected.";
            return out;
        }
    }

    std::error_code ec;
    const auto absoluteRoot = fs::absolute(fs::path(appDir), ec);
    if (ec || absoluteRoot.empty()) {
        out.process.error = L"Trusted engine execution blocked: root resolution failed.";
        return out;
    }

    fs::path offending;
    const auto normalizedAbsoluteRoot = absoluteRoot.lexically_normal();
    if (HasReparsePathComponents(normalizedAbsoluteRoot, offending)) {
        out.process.error = L"Trusted engine execution blocked: application root contains reparse point.";
        return out;
    }

    ec.clear();
    const auto root = fs::weakly_canonical(normalizedAbsoluteRoot, ec);
    if (ec || root.empty()) {
        out.process.error = L"Trusted engine execution blocked: canonicalization failed.";
        return out;
    }

    if (HasReparseUnderRoot(root, rel, offending)) {
        out.process.error = L"Trusted engine execution blocked: path contains reparse point.";
        return out;
    }

    ec.clear();
    const auto candidate = fs::weakly_canonical(root / rel, ec);
    if (ec || candidate.empty() || !IsWithinRoot(root, candidate) || !fs::is_regular_file(candidate, ec) || ec) {
        out.process.error = L"Trusted engine execution blocked: file missing or not regular.";
        return out;
    }

    // TOCTOU Prevention: Open file handle with FILE_SHARE_READ (exclusive against external writes).
    // Hold this handle across the entire verification and CreateProcessW launch sequence.
    HANDLE hFile = CreateFileW(candidate.wstring().c_str(),
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               nullptr,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        out.process.error = L"Trusted engine execution blocked: cannot lock file handle.";
        return out;
    }

    out.trust.fileExists = true;
    out.trust.resolvedPath = candidate.wstring();
    out.trust.logicalName = logicalName;
    out.trust.sha256 = HashHandle(hFile);

    if (out.trust.sha256.empty()) {
        CloseHandle(hFile);
        out.process.error = L"Trusted engine execution blocked: hashing locked handle failed.";
        return out;
    }

    const auto* embedded = FindEmbeddedProvider(logicalName);
    if (embedded) {
        out.trust.manifestEntry = true;
        out.trust.embeddedCatalogMatch = true;
        out.trust.expectedSha256 = Lower(embedded->expectedSha256);
        out.trust.version = embedded->version;
        out.trust.licenseId = embedded->licenseId;
        out.trust.hashMatches = IsSha256Hex(out.trust.expectedSha256) &&
                                Lower(out.trust.sha256) == out.trust.expectedSha256;
    } else {
        // Fallback to VerifyEngine for manifest lookup
        out.trust = VerifyEngine(appDir, relativePath, logicalName);
    }

    if (!out.trust.hashMatches || out.trust.resolvedPath.empty()) {
        CloseHandle(hFile);
        out.process.error = L"Trusted engine execution blocked: " + out.trust.reason;
        return out;
    }

    // Launch process while hFile lock is held open
    out.process = RunProcessCaptureExecutable(out.trust.resolvedPath, arguments, timeoutMs, cancel);
    CloseHandle(hFile);
    return out;
}
} // namespace lap

