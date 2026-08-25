#include "lap/process.h"
#include <windows.h>
#include <shellapi.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cwctype>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace lap {
namespace {
std::wstring DecodeBytes(const std::string& bytes) {
    if (bytes.empty()) return L"";
    auto decode = [&](UINT cp, DWORD flags) -> std::wstring {
        int n = MultiByteToWideChar(cp, flags, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
        if (n <= 0) return L"";
        std::wstring w(static_cast<size_t>(n), L'\0');
        if (MultiByteToWideChar(cp, flags, bytes.data(), static_cast<int>(bytes.size()), w.data(), n) <= 0) return L"";
        return w;
    };
    auto utf8 = decode(CP_UTF8, MB_ERR_INVALID_CHARS);
    return utf8.empty() ? decode(CP_ACP, 0) : utf8;
}

void DrainPipe(HANDLE pipe, std::string& dst) {
    constexpr size_t kMaxCapture = 16u * 1024u * 1024u;
    char buffer[8192];
    DWORD got = 0;
    while (ReadFile(pipe, buffer, sizeof(buffer), &got, nullptr) && got > 0) {
        if (dst.size() < kMaxCapture) {
            const size_t room = kMaxCapture - dst.size();
            dst.append(buffer, buffer + (got < room ? got : room));
        }
    }
}

bool IsRegularFile(const std::wstring& path) {
    const DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool EqualsI(std::wstring a, std::wstring b) {
    std::transform(a.begin(), a.end(), a.begin(), towlower);
    std::transform(b.begin(), b.end(), b.begin(), towlower);
    return a == b;
}

std::wstring ResolveExecutableForLegacy(const std::wstring& argv0) {
    if (argv0.empty()) return L"";
    namespace fs = std::filesystem;
    const fs::path requested(argv0);

    // Do not let a writable current directory or user PATH shadow Windows PowerShell.
    if (!requested.has_parent_path() && EqualsI(requested.filename().wstring(), L"powershell.exe")) {
        std::vector<wchar_t> systemDir(32768);
        const UINT n = GetSystemDirectoryW(systemDir.data(), static_cast<UINT>(systemDir.size()));
        if (n > 0 && n < systemDir.size()) {
            const fs::path candidate = fs::path(std::wstring(systemDir.data(), n)) /
                L"WindowsPowerShell" / L"v1.0" / L"powershell.exe";
            if (IsRegularFile(candidate.wstring())) return candidate.wstring();
        }
    }

    if (requested.has_parent_path() || requested.is_absolute()) {
        std::error_code ec;
        auto full = fs::absolute(requested, ec);
        if (!ec && IsRegularFile(full.wstring())) {
            auto canonical = fs::weakly_canonical(full, ec);
            return ec ? full.wstring() : canonical.wstring();
        }
        return L"";
    }

    std::vector<wchar_t> found(32768);
    const DWORD n = SearchPathW(nullptr, argv0.c_str(), nullptr, static_cast<DWORD>(found.size()), found.data(), nullptr);
    if (!n || n >= found.size()) return L"";
    const std::wstring resolved(found.data(), n);
    return IsRegularFile(resolved) ? resolved : L"";
}

std::wstring QuoteWindowsArgument(const std::wstring& arg) {
    if (arg.empty()) return L"\"\"";
    const bool needsQuotes = std::any_of(arg.begin(), arg.end(), [](wchar_t c) {
        return iswspace(c) != 0 || c == L'\"';
    });
    if (!needsQuotes) return arg;

    std::wstring out;
    out.push_back(L'\"');
    size_t backslashes = 0;
    for (wchar_t c : arg) {
        if (c == L'\\') {
            ++backslashes;
            continue;
        }
        if (c == L'\"') {
            out.append(backslashes * 2 + 1, L'\\');
            out.push_back(L'\"');
            backslashes = 0;
            continue;
        }
        out.append(backslashes, L'\\');
        backslashes = 0;
        out.push_back(c);
    }
    out.append(backslashes * 2, L'\\');
    out.push_back(L'\"');
    return out;
}

std::wstring BuildCommandLine(const std::wstring& executablePath, const std::vector<std::wstring>& arguments) {
    std::wstring line = QuoteWindowsArgument(executablePath);
    for (const auto& arg : arguments) {
        line.push_back(L' ');
        line += QuoteWindowsArgument(arg);
    }
    return line;
}
} // namespace

ProcessResult RunProcessCaptureExecutable(const std::wstring& executablePath,
                                          const std::vector<std::wstring>& arguments,
                                          unsigned timeoutMs,
                                          const std::atomic_bool* cancel) {
    ProcessResult result{};
    const auto started = std::chrono::steady_clock::now();
    if (executablePath.empty() || !IsRegularFile(executablePath)) {
        result.error = L"Executable path is empty, missing, or not a regular file.";
        return result;
    }

    SECURITY_ATTRIBUTES inheritable{sizeof(inheritable), nullptr, TRUE};
    HANDLE readPipe = nullptr, writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &inheritable, 0)) {
        result.error = L"CreatePipe failed";
        return result;
    }
    if (!SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(writePipe);
        CloseHandle(readPipe);
        result.error = L"SetHandleInformation failed: " + std::to_wstring(GetLastError());
        return result;
    }

    HANDLE nullInput = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   &inheritable, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (nullInput == INVALID_HANDLE_VALUE) {
        CloseHandle(writePipe);
        CloseHandle(readPipe);
        result.error = L"Could not open explicit child stdin: " + std::to_wstring(GetLastError());
        return result;
    }

    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.StartupInfo.hStdOutput = writePipe;
    startup.StartupInfo.hStdError = writePipe;
    startup.StartupInfo.hStdInput = nullInput;
    startup.StartupInfo.wShowWindow = SW_HIDE;

    SIZE_T attributeBytes = 0;
    (void)InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeBytes);
    if (attributeBytes == 0) {
        CloseHandle(nullInput);
        CloseHandle(writePipe);
        CloseHandle(readPipe);
        result.error = L"Could not size process attribute list: " + std::to_wstring(GetLastError());
        return result;
    }
    startup.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
        HeapAlloc(GetProcessHeap(), 0, attributeBytes));
    if (!startup.lpAttributeList) {
        CloseHandle(nullInput);
        CloseHandle(writePipe);
        CloseHandle(readPipe);
        result.error = L"Could not allocate process attribute list.";
        return result;
    }
    if (!InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &attributeBytes)) {
        HeapFree(GetProcessHeap(), 0, startup.lpAttributeList);
        startup.lpAttributeList = nullptr;
        CloseHandle(nullInput);
        CloseHandle(writePipe);
        CloseHandle(readPipe);
        result.error = L"Could not initialize process attribute list: " + std::to_wstring(GetLastError());
        return result;
    }

    HANDLE inheritedHandles[] = {writePipe, nullInput};
    if (!UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                   inheritedHandles, sizeof(inheritedHandles), nullptr, nullptr)) {
        DeleteProcThreadAttributeList(startup.lpAttributeList);
        HeapFree(GetProcessHeap(), 0, startup.lpAttributeList);
        startup.lpAttributeList = nullptr;
        CloseHandle(nullInput);
        CloseHandle(writePipe);
        CloseHandle(readPipe);
        result.error = L"Could not restrict child handle inheritance: " + std::to_wstring(GetLastError());
        return result;
    }

    PROCESS_INFORMATION process{};
    auto commandLine = BuildCommandLine(executablePath, arguments);
    std::vector<wchar_t> commandBuffer(commandLine.begin(), commandLine.end());
    commandBuffer.push_back(L'\0');

    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (job) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
            CloseHandle(job);
            job = nullptr;
        }
    }

    const DWORD creationFlags = CREATE_NO_WINDOW | CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT;
    const BOOL created = CreateProcessW(executablePath.c_str(), commandBuffer.data(), nullptr, nullptr, TRUE,
                                        creationFlags, nullptr, nullptr, &startup.StartupInfo, &process);

    DeleteProcThreadAttributeList(startup.lpAttributeList);
    HeapFree(GetProcessHeap(), 0, startup.lpAttributeList);
    startup.lpAttributeList = nullptr;
    CloseHandle(nullInput);
    CloseHandle(writePipe);
    writePipe = nullptr;

    if (!created) {
        if (job) CloseHandle(job);
        CloseHandle(readPipe);
        result.error = L"CreateProcess failed: " + std::to_wstring(GetLastError());
        return result;
    }

    result.launched = true;
    bool assignedJob = false;
    if (job) assignedJob = AssignProcessToJobObject(job, process.hProcess) != FALSE;
    if (ResumeThread(process.hThread) == static_cast<DWORD>(-1)) {
        result.error = L"ResumeThread failed: " + std::to_wstring(GetLastError());
        TerminateProcess(process.hProcess, 0xDEAD);
    }

    std::string bytes;
    std::thread reader([&] { DrainPipe(readPipe, bytes); });

    DWORD wait = WAIT_TIMEOUT;
    for (;;) {
        wait = WaitForSingleObject(process.hProcess, 100);
        if (wait == WAIT_OBJECT_0) break;
        if (wait == WAIT_FAILED) {
            result.error = L"WaitForSingleObject failed: " + std::to_wstring(GetLastError());
            result.timedOut = true;
            break;
        }
        if (cancel && cancel->load()) {
            result.cancelled = true;
            break;
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count();
        if (timeoutMs && elapsed >= timeoutMs) {
            result.timedOut = true;
            break;
        }
    }

    if (result.cancelled || result.timedOut) {
        if (job && assignedJob) TerminateJobObject(job, 0xDEAD);
        else TerminateProcess(process.hProcess, 0xDEAD);
        WaitForSingleObject(process.hProcess, 3000);
    }

    DWORD code = 0;
    if (GetExitCodeProcess(process.hProcess, &code)) result.exitCode = code;
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    if (job) CloseHandle(job);

    if (reader.joinable()) {
        if (WaitForSingleObject(static_cast<HANDLE>(reader.native_handle()), 1000) == WAIT_TIMEOUT)
            CancelSynchronousIo(static_cast<HANDLE>(reader.native_handle()));
        reader.join();
    }
    CloseHandle(readPipe);

    result.output = DecodeBytes(bytes);
    result.elapsedMs = static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started).count());
    return result;
}

ProcessResult RunProcessCapture(const std::wstring& commandLine,
                                unsigned timeoutMs,
                                const std::atomic_bool* cancel) {
    ProcessResult result{};
    if (commandLine.empty()) {
        result.error = L"Command line is empty.";
        return result;
    }

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(commandLine.c_str(), &argc);
    if (!argv || argc <= 0) {
        if (argv) LocalFree(argv);
        result.error = L"CommandLineToArgvW failed: " + std::to_wstring(GetLastError());
        return result;
    }

    const std::wstring executablePath = ResolveExecutableForLegacy(argv[0]);
    std::vector<std::wstring> arguments;
    arguments.reserve(argc > 1 ? static_cast<size_t>(argc - 1) : 0);
    for (int i = 1; i < argc; ++i) arguments.emplace_back(argv[i]);
    LocalFree(argv);

    if (executablePath.empty()) {
        result.error = L"Executable could not be resolved to a concrete file path.";
        return result;
    }
    return RunProcessCaptureExecutable(executablePath, arguments, timeoutMs, cancel);
}
} // namespace lap
