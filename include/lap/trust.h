#pragma once
#include "lap/process.h"
#include <atomic>
#include <string>
#include <vector>

namespace lap {
struct EngineTrust {
    bool fileExists{false};
    bool manifestEntry{false};
    bool hashMatches{false};
    std::wstring resolvedPath;
    std::wstring sha256;
    std::wstring expectedSha256;
    std::wstring reason;
};

struct TrustedEngineRun {
    EngineTrust trust;
    ProcessResult process;
};

EngineTrust VerifyEngine(const std::wstring& appDir,
                         const std::wstring& relativePath,
                         const std::wstring& logicalName);

// Execution boundary for bundled engines. The file is canonicalized and SHA-256
// allowlisted immediately before the explicit executable process API is called.
TrustedEngineRun RunTrustedEngineCapture(const std::wstring& appDir,
                                         const std::wstring& relativePath,
                                         const std::wstring& logicalName,
                                         const std::vector<std::wstring>& arguments,
                                         unsigned timeoutMs = 15000,
                                         const std::atomic_bool* cancel = nullptr);
}
