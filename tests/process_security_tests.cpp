#include "lap/process.h"
#include "lap/trust.h"
#include <windows.h>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
int failures = 0;

void Expect(bool ok, const char* message) {
    if (ok) std::cout << "PASS " << message << '\n';
    else { std::cerr << "FAIL " << message << '\n'; ++failures; }
}

std::wstring SelfPath() {
    std::vector<wchar_t> buffer(32768);
    const DWORD n = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!n || n >= buffer.size()) return L"";
    return std::wstring(buffer.data(), n);
}

const std::vector<std::wstring>& EdgeArgs() {
    static const std::vector<std::wstring> values{
        L"", L"plain", L"with space", L"quote\"inside", L"trailing\\", L"slashes\\\\\"quote"
    };
    return values;
}

int ChildMode(int argc, wchar_t** argv) {
    const auto& expected = EdgeArgs();
    if (argc != static_cast<int>(expected.size()) + 2) return 21;
    for (size_t i = 0; i < expected.size(); ++i) {
        if (expected[i] != argv[i + 2]) return 30 + static_cast<int>(i);
    }
    return 0;
}

int HandleChildMode(int argc, wchar_t** argv) {
    if (argc != 3) return 61;
    wchar_t* end = nullptr;
    const auto raw = _wcstoui64(argv[2], &end, 10);
    if (!end || *end != L'\0') return 62;
    const HANDLE candidate = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(raw));
    DWORD available = 0;
    if (PeekNamedPipe(candidate, nullptr, 0, nullptr, &available, nullptr)) return 63;
    return GetLastError() == ERROR_INVALID_HANDLE ? 0 : 64;
}
} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc > 1 && std::wstring(argv[1]) == L"--argv-child") return ChildMode(argc, argv);
    if (argc > 1 && std::wstring(argv[1]) == L"--handle-child") return HandleChildMode(argc, argv);
    if (argc > 1 && std::wstring(argv[1]) == L"--sleep-child") { Sleep(3000); return 0; }

    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path root = fs::temp_directory_path() / L"LapSure Process Security Tests";
    const fs::path tools = root / L"tools";
    const fs::path copied = tools / L"probe engine.exe";
    fs::remove_all(root, ec);
    fs::create_directories(tools, ec);
    Expect(!ec, "temporary process-security workspace created");

    const auto self = SelfPath();
    Expect(!self.empty(), "test executable path resolved");
    fs::copy_file(self, copied, fs::copy_options::overwrite_existing, ec);
    Expect(!ec && fs::is_regular_file(copied), "test executable copied to path containing spaces");

    std::vector<std::wstring> args{L"--argv-child"};
    const auto& edge = EdgeArgs();
    args.insert(args.end(), edge.begin(), edge.end());
    auto explicitRun = lap::RunProcessCaptureExecutable(copied.wstring(), args, 10000, nullptr);
    Expect(explicitRun.launched && !explicitRun.timedOut && explicitRun.exitCode == 0,
           "explicit executable API preserves empty/space/quote/backslash argv");

    SECURITY_ATTRIBUTES inheritable{sizeof(inheritable), nullptr, TRUE};
    HANDLE sentinelRead = nullptr, sentinelWrite = nullptr;
    const bool sentinelCreated = CreatePipe(&sentinelRead, &sentinelWrite, &inheritable, 0) != FALSE;
    Expect(sentinelCreated, "inheritable sentinel pipe created in parent");
    if (sentinelCreated) {
        const auto raw = static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(sentinelRead));
        auto handleRun = lap::RunProcessCaptureExecutable(
            copied.wstring(), {L"--handle-child", std::to_wstring(raw)}, 10000, nullptr);
        Expect(handleRun.launched && !handleRun.timedOut && handleRun.exitCode == 0,
               "unlisted inheritable parent handle is not inherited by child");
        CloseHandle(sentinelWrite);
        CloseHandle(sentinelRead);
    }

    auto timeoutRun = lap::RunProcessCaptureExecutable(copied.wstring(), {L"--sleep-child"}, 200, nullptr);
    Expect(timeoutRun.launched && timeoutRun.timedOut, "explicit executable API preserves timeout termination");

    const fs::path manifest = tools / L"engine_manifest.txt";
    { std::ofstream f(manifest, std::ios::binary | std::ios::trunc); f << "probe=\n"; }
    auto measured = lap::VerifyEngine(root.wstring(), L"tools\\probe engine.exe", L"probe");
    Expect(measured.fileExists && !measured.sha256.empty() && !measured.hashMatches,
           "trusted fixture hash measured before allowlisting");
    { std::wofstream f(manifest, std::ios::trunc); f << L"probe=" << measured.sha256 << L"\n"; }

    auto trustedRun = lap::RunTrustedEngineCapture(root.wstring(), L"tools\\probe engine.exe", L"probe", args, 10000, nullptr);
    Expect(trustedRun.trust.hashMatches && trustedRun.process.launched && trustedRun.process.exitCode == 0,
           "trusted engine boundary re-verifies and launches canonical path");

    { std::ofstream f(copied, std::ios::binary | std::ios::app); f << "tamper"; }
    auto tampered = lap::RunTrustedEngineCapture(root.wstring(), L"tools\\probe engine.exe", L"probe", args, 10000, nullptr);
    Expect(!tampered.trust.hashMatches && !tampered.process.launched,
           "tampered bundled engine is blocked before execution");

    fs::remove_all(root, ec);
    return failures == 0 ? 0 : 1;
}
