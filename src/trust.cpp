#include "lap/trust.h"
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
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

std::wstring HashFile(const std::wstring& path) {
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
    std::ifstream f(std::filesystem::path(path), std::ios::binary);
    if (!f) {
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(alg, 0);
        return L"";
    }
    char buf[1 << 16];
    while (f) {
        f.read(buf, sizeof(buf));
        const auto n = f.gcount();
        if (n > 0 && BCryptHashData(hash, reinterpret_cast<PUCHAR>(buf), static_cast<ULONG>(n), 0) != 0) {
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
} // namespace

EngineTrust VerifyEngine(const std::wstring& appDir,
                         const std::wstring& relativePath,
                         const std::wstring& logicalName) {
    EngineTrust trust{};
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

    // Inspect the caller-supplied absolute path itself for symlink/junction/reparse
    // components. Do not infer reparse semantics by comparing textual canonical
    // spellings: Windows can legitimately normalize the same directory to a
    // different short/long-path representation.
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

    trust.sha256 = HashFile(trust.resolvedPath);
    if (trust.sha256.empty()) {
        trust.reason = L"SHA-256 calculation failed.";
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
    // Capability checks performed earlier are advisory only. Re-verify at the
    // execution boundary so a replaced or redirected bundled engine is blocked.
    out.trust = VerifyEngine(appDir, relativePath, logicalName);
    if (!out.trust.hashMatches || out.trust.resolvedPath.empty()) {
        out.process.error = L"Trusted engine execution blocked: " + out.trust.reason;
        return out;
    }
    out.process = RunProcessCaptureExecutable(out.trust.resolvedPath, arguments, timeoutMs, cancel);
    return out;
}
} // namespace lap
