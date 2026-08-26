#include "lap/trust.h"
#include "lap/provider_output.h"
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

bool CreateDirectorySymlink(const std::filesystem::path& link, const std::filesystem::path& target) {
    constexpr DWORD kDirectory = SYMBOLIC_LINK_FLAG_DIRECTORY;
    constexpr DWORD kAllowUnprivilegedCreate = 0x2;
    if (CreateSymbolicLinkW(link.wstring().c_str(), target.wstring().c_str(),
                            kDirectory | kAllowUnprivilegedCreate)) return true;
    if (GetLastError() == ERROR_INVALID_PARAMETER) {
        return CreateSymbolicLinkW(link.wstring().c_str(), target.wstring().c_str(), kDirectory) != FALSE;
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
    const auto redirectedRoot = base.parent_path() / L"lapsure-trust-root-link";
    fs::remove_all(base, ec); fs::remove(outside, ec); fs::remove(outsideManifest, ec); fs::remove(redirectedRoot, ec);
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

    if (CreateDirectorySymlink(redirectedRoot, base)) {
        auto redirected = lap::VerifyEngine(redirectedRoot.wstring(), L"tools\\probe.exe", L"probe");
        Expect(!redirected.hashMatches && redirected.reason.find(L"reparse") != std::wstring::npos,
               "application root reparse redirection is rejected explicitly");
        fs::remove(redirectedRoot, ec);
    } else {
        std::cout << "SKIP application-root reparse fixture unavailable on this Windows policy\n";
    }

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

    // Round 5.1B: Test Embedded Provider Catalog dominates external manifest
    lap::ClearTestProtectedProviders();
    lap::ProviderIdentity embeddedProvider{
        L"embedded_probe",
        L"tools\\probe.exe",
        unconfigured.sha256,
        L"1.0.0",
        L"MIT",
        {}
    };
    lap::RegisterTestProtectedProvider(embeddedProvider);

    // Write conflicting rogue hash into manifest
    fs::create_directories(base / L"tools", ec);
    {
        std::wofstream f(manifest, std::ios::trunc);
        f << L"embedded_probe=0000000000000000000000000000000000000000000000000000000000000000\n";
    }
    auto embeddedVerified = lap::VerifyEngine(base.wstring(), L"tools\\probe.exe", L"embedded_probe");
    Expect(embeddedVerified.hashMatches && embeddedVerified.embeddedCatalogMatch,
           "embedded catalog dominates external manifest for protected providers");
    Expect(embeddedVerified.version == L"1.0.0" && embeddedVerified.licenseId == L"MIT",
           "embedded catalog provides version and license metadata");

    // Round 5.1B: Test Handle-Locked TOCTOU Protection
    HANDLE hLock = CreateFileW(probe.wstring().c_str(),
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               nullptr,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               nullptr);
    Expect(hLock != INVALID_HANDLE_VALUE, "probe file locked with FILE_SHARE_READ");
    if (hLock != INVALID_HANDLE_VALUE) {
        // Attempting to open for write while handle is held must fail with sharing violation
        HANDLE hWriteAttempt = CreateFileW(probe.wstring().c_str(),
                                           GENERIC_WRITE,
                                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                                           nullptr,
                                           OPEN_EXISTING,
                                           FILE_ATTRIBUTE_NORMAL,
                                           nullptr);
        Expect(hWriteAttempt == INVALID_HANDLE_VALUE && GetLastError() == ERROR_SHARING_VIOLATION,
               "external write during handle lock is blocked by OS sharing violation");
        if (hWriteAttempt != INVALID_HANDLE_VALUE) CloseHandle(hWriteAttempt);
        CloseHandle(hLock);
    }

    // Round 5.1B: Test Provider Output Contract Validation
    // 1. smartctl scan contract
    auto validScan = lap::ValidateSmartctlScanOutput(L"/dev/sda -d ata # /dev/sda, ATA device\n/dev/nvme0 -d nvme # /dev/nvme0, NVMe device\n");
    Expect(validScan.valid && validScan.recordCount == 2, "valid smartctl scan output parses device records");
    auto invalidScan = lap::ValidateSmartctlScanOutput(L"smartctl 7.4 - error opening device\n");
    Expect(!invalidScan.valid && invalidScan.recordCount == 0, "smartctl scan output without devices fails closed");

    // 2. smartctl JSON contract
    auto validSmartJson = lap::ValidateSmartctlJsonOutput(LR"({"smart_status": {"passed": true}, "temperature": 35})");
    Expect(validSmartJson.valid && validSmartJson.recordCount == 1, "valid smartctl JSON object passes contract");
    auto malformedSmartJson = lap::ValidateSmartctlJsonOutput(LR"({"smart_status": {"temperature": 35})");
    Expect(!malformedSmartJson.valid, "malformed unclosed smartctl JSON fails closed");
    auto missingPassedJson = lap::ValidateSmartctlJsonOutput(LR"({"temperature": 35, "model_name": "TestSSD"})");
    Expect(!missingPassedJson.valid, "smartctl JSON missing 'passed' boolean fails closed");

    // 3. nvidia-smi CSV contract
    auto validNvidiaCsv = lap::ValidateNvidiaSmiCsvOutput(L"NVIDIA RTX A2000, 4096, 52.0, 15.0\n");
    Expect(validNvidiaCsv.valid && validNvidiaCsv.recordCount == 1, "valid nvidia-smi CSV row passes contract");
    auto emptyNvidiaCsv = lap::ValidateNvidiaSmiCsvOutput(L"# Comments only\n");
    Expect(!emptyNvidiaCsv.valid && emptyNvidiaCsv.recordCount == 0, "empty nvidia-smi CSV fails closed");

    // 4. Sensor bridge pipe contract
    auto validSensorPipe = lap::ValidateSensorBridgeOutput(L"65.5|28.0|3200|0\n");
    Expect(validSensorPipe.valid && validSensorPipe.recordCount == 1, "valid sensor bridge pipe output passes contract");
    auto invalidSensorPipe = lap::ValidateSensorBridgeOutput(L"65.5|28.0\n");
    Expect(!invalidSensorPipe.valid, "truncated sensor bridge pipe output fails closed");

    // 5. VRAM stress contract
    auto validVramPass = lap::ValidateVramStressOutput(L"[VRAM_STRESS] STATUS=PASS ERRORS=0 DURATION=30s\n");
    Expect(validVramPass.valid, "valid VRAM stress output with STATUS=PASS passes contract");
    auto invalidVram = lap::ValidateVramStressOutput(L"Crash / CUDA Error 999\n");
    Expect(!invalidVram.valid, "unstructured VRAM crash output fails closed");

    lap::ClearTestProtectedProviders();
    fs::remove_all(base, ec); fs::remove(outside, ec); fs::remove(outsideManifest, ec); fs::remove(redirectedRoot, ec);
    return failures == 0 ? 0 : 1;
}
