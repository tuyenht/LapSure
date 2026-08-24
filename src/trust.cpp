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
}

EngineTrust VerifyEngine(const std::wstring& appDir,
                         const std::wstring& relativePath,
                         const std::wstring& logicalName) {
    EngineTrust t{};
    namespace fs = std::filesystem;
    const fs::path rel(relativePath);
    if (relativePath.empty() || rel.is_absolute() || rel.has_root_name() || rel.has_root_directory()) {
        t.reason = L"Engine path must be relative to the LapSure application root.";
        return t;
    }
    for (const auto& part : rel) {
        if (part == L"..") {
            t.reason = L"Engine path traversal outside the LapSure application root is blocked.";
            return t;
        }
    }

    std::error_code ec;
    const auto root = fs::weakly_canonical(fs::absolute(fs::path(appDir), ec), ec);
    if (ec || root.empty()) {
        t.reason = L"Application root canonicalization failed.";
        return t;
    }
    const auto candidate = fs::weakly_canonical(root / rel, ec);
    if (ec || candidate.empty()) {
        t.reason = L"Engine path canonicalization failed.";
        return t;
    }
    if (!IsWithinRoot(root, candidate)) {
        t.reason = L"Engine resolved outside the LapSure application root.";
        return t;
    }
    if (!fs::is_regular_file(candidate, ec) || ec) {
        t.reason = L"Engine file not found or is not a regular file.";
        return t;
    }
    t.fileExists = true;
    t.resolvedPath = candidate.wstring();

    t.sha256 = HashFile(t.resolvedPath);
    if (t.sha256.empty()) {
        t.reason = L"SHA-256 calculation failed.";
        return t;
    }

    const auto manifest = root / L"tools" / L"engine_manifest.txt";
    std::wifstream f(manifest);
    if (!f) {
        t.reason = L"Trust manifest missing.";
        return t;
    }
    std::wstring line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == L'#') continue;
        const auto p = line.find(L'=');
        if (p == std::wstring::npos) continue;
        auto name = line.substr(0, p), expected = line.substr(p + 1);
        Trim(name);
        Trim(expected);
        if (Lower(name) == Lower(logicalName)) {
            t.manifestEntry = true;
            t.expectedSha256 = Lower(expected);
            break;
        }
    }
    if (!t.manifestEntry) {
        t.reason = L"No allowlist entry for engine.";
        return t;
    }
    if (!IsSha256Hex(t.expectedSha256)) {
        t.reason = L"Allowlist SHA-256 is not configured or invalid.";
        return t;
    }
    t.hashMatches = Lower(t.sha256) == Lower(t.expectedSha256);
    t.reason = t.hashMatches ? L"Trusted engine hash matched." : L"Engine hash mismatch.";
    return t;
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
}
