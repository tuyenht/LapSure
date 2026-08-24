#pragma once
#include <atomic>
#include <string>
#include <vector>

namespace lap {
struct ProcessResult {
    bool launched{false};
    bool timedOut{false};
    bool cancelled{false};
    unsigned long exitCode{0};
    unsigned long elapsedMs{0};
    std::wstring output;
    std::wstring error;
};

// Preferred process boundary: the executable is an explicit path and arguments
// are quoted according to Windows argv rules before CreateProcessW is called.
ProcessResult RunProcessCaptureExecutable(const std::wstring& executablePath,
                                          const std::vector<std::wstring>& arguments,
                                          unsigned timeoutMs = 15000,
                                          const std::atomic_bool* cancel = nullptr);

// Compatibility wrapper for existing callers. It parses argv, resolves argv[0]
// to a concrete executable path, then delegates to RunProcessCaptureExecutable.
ProcessResult RunProcessCapture(const std::wstring& commandLine,
                                unsigned timeoutMs = 15000,
                                const std::atomic_bool* cancel = nullptr);
}
