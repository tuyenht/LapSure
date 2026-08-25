#include "lap/trust.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {
int failures = 0;

void Expect(bool ok, const char* message) {
    if (ok) std::cout << "PASS " << message << '\n';
    else { std::cerr << "FAIL " << message << '\n'; ++failures; }
}

bool CreateFileSymlink(const std::filesystem::path& link, const std::filesystem::path& target) {
    constexpr DWORD kAllowUnprivilegedCreate = 0x2;
    if (CreateSymbolicLinkW(link.wstring().c_str(), target.wstring().c_str(), kAllowUnprivilegedCreate)) return true;
    if (GetLastError() == ERROR_INVALID_PARAMETER) {
        return CreateSymbolicLinkW(link.wstring().c_str(), target.wstring().c_str(), 0) != FALSE;
    }
    return false;
}
} // namespace

int main() {
    namespace fs = std::filesystem;
    std::error_code ec;
    const auto base = fs::temp_directory_path() / L"lapsure-trust-security-tests";
    const auto outside = base.parent_path() / L"lapsure-trust-outside.exe";
    const auto outsideManifest = base.parent_path() / L"lapsure-trust-outside-manifest.txt";
    fs::remove_all(base, ec); fs::remove(outside, ec); fs::remove(outsideManifest, ec);
    fs::create_directories(base / L"tools", ec);
    if (ec) { std::cerr << "FAIL could not create trust test directory\n"; return 1; }

    const auto probe = base / L"tools" / L"probe.exe";
    { std::ofstream f(probe, std::ios::binary | std::ios::trunc); f << "LapSure trust fixture"; }
    { std::ofstream f(outside, std::ios::binary | std::ios::trunc); f << "outside fixture"; }
    const auto manifest = base / L"tools" / L"engine_manifest.txt";
    { std::ofstream f(manifest, std::ios::binary | std::ios::trunc); f << "probe=\n"; }

    auto unconfigured = lap::VerifyEngine(base.wstring(), L"tools\\probe.exe", L"probe");
    Expect(unconfigured.fileExists, "regular bundled engine is found");
    Expect(unconfigured.manifestEntry, "single allowlist entry is found");
    Expect(!unconfigured.hashMatches && !unconfigured.sha256.empty(),
           "empty allowlist hash remains untrusted while actual SHA is measured");
    Expect(unconfigured.reason.find(L"not configured") != std::wstring::npos ||
           unconfigured.reason.find(L"invalid") != std::wstring::npos,
           "unconfigured hash has explicit reason");

    { std::wofstream f(manifest, std::ios::trunc); f << L"probe=" << unconfigured.sha256 << L"\n"; }
    auto trusted = lap::VerifyEngine(base.wstring(), L"tools\\probe.exe", L"probe");
    Expect(trusted.hashMatches, "matching SHA-256 trusts reviewed bundled engine");

    { std::wofstream f(manifest, std::ios::trunc);
      f << L"probe=" << unconfigured.sha256 << L"\n";
      f << L"probe=" << unconfigured.sha256 << L"\n"; }
    auto duplicate = lap::VerifyEngine(base.wstring(), L"tools\\probe.exe", L"probe");
    Expect(!duplicate.hashMatches && duplicate.reason.find(L"duplicate") != std::wstring::npos,
           "duplicate logical allowlist entries fail closed explicitly");

    { std::wofstream f(manifest, std::ios::trunc); f << L"probe=not-a-sha256\n"; }
    auto malformed = lap::VerifyEngine(base.wstring(), L"tools\\probe.exe", L"probe");
    Expect(malformed.manifestEntry && !malformed.hashMatches && malformed.reason.find(L"invalid") != std::wstring::npos,
           "malformed single allowlist hash fails closed explicitly");

    { std::wofstream f(manifest, std::ios::trunc); f << L"other=" << unconfigured.sha256 << L"\n"; }
    auto missing = lap::VerifyEngine(base.wstring(), L"tools\\probe.exe", L"probe");
    Expect(!missing.manifestEntry && !missing.hashMatches,
           "missing logical allowlist entry fails closed");

    { std::wofstream f(manifest, std::ios::trunc); f << L"probe=" << unconfigured.sha256 << L"\n"; }
    auto traversal = lap::VerifyEngine(base.wstring(), L"..\\lapsure-trust-outside.exe", L"probe");
    Expect(!traversal.fileExists && !traversal.hashMatches, "parent traversal is rejected before hashing");
    Expect(traversal.reason.find(L"traversal") != std::wstring::npos || traversal.reason.find(L"outside") != std::wstring::npos,
           "parent traversal rejection is explicit");

    auto absolute = lap::VerifyEngine(base.wstring(), outside.wstring(), L"probe");
    Expect(!absolute.fileExists && !absolute.hashMatches, "absolute engine path is rejected");

    auto directory = lap::VerifyEngine(base.wstring(), L"tools", L"probe");
    Expect(!directory.fileExists && !directory.hashMatches, "directory cannot masquerade as an engine file");

    const auto inRootLink = base / L"tools" / L"probe-link.exe";
    if (CreateFileSymlink(inRootLink, probe)) {
        auto reparseEngine = lap::VerifyEngine(base.wstring(), L"tools\\probe-link.exe", L"probe");
        Expect(!reparseEngine.hashMatches && reparseEngine.reason.find(L"reparse") != std::wstring::npos,
               "in-root engine reparse path is rejected explicitly");
        fs::remove(inRootLink, ec);
    } else {
        std::cout << "SKIP engine reparse behavioral fixture unavailable on this Windows policy\n";
    }

    { std::wofstream f(outsideManifest, std::ios::trunc); f << L"probe=" << unconfigured.sha256 << L"\n"; }
    fs::remove(manifest, ec);
    if (CreateFileSymlink(manifest, outsideManifest)) {
        auto reparseManifest = lap::VerifyEngine(base.wstring(), L"tools\\probe.exe", L"probe");
        Expect(!reparseManifest.hashMatches && reparseManifest.reason.find(L"reparse") != std::wstring::npos,
               "manifest reparse redirection is rejected explicitly");
    } else {
        std::cout << "SKIP manifest reparse behavioral fixture unavailable on this Windows policy\n";
    }

    fs::remove_all(base, ec); fs::remove(outside, ec); fs::remove(outsideManifest, ec);
    return failures == 0 ? 0 : 1;
}
