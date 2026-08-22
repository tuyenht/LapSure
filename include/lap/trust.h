#pragma once
#include <string>
namespace lap {
struct EngineTrust {
    bool fileExists{false};
    bool manifestEntry{false};
    bool hashMatches{false};
    std::wstring sha256;
    std::wstring expectedSha256;
    std::wstring reason;
};
EngineTrust VerifyEngine(const std::wstring& appDir,const std::wstring& relativePath,const std::wstring& logicalName);
}
