#pragma once
#include <atomic>
#include <string>

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

ProcessResult RunProcessCapture(const std::wstring& commandLine,
                                unsigned timeoutMs = 15000,
                                const std::atomic_bool* cancel = nullptr);
}
