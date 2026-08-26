#pragma once
#include "lap/process.h"
#include <atomic>
#include <string>
#include <string_view>
#include <vector>

namespace lap {

struct ProviderIdentity {
    std::wstring logicalName;
    std::wstring expectedRelativePath;
    std::wstring expectedSha256;
    std::wstring version;
    std::wstring licenseId;
    std::vector<std::wstring> requiredDependencies;
};

struct EngineTrust {
    bool fileExists{false};
    bool manifestEntry{false};
    bool hashMatches{false};
    bool embeddedCatalogMatch{false};
    std::wstring resolvedPath;
    std::wstring sha256;
    std::wstring expectedSha256;
    std::wstring logicalName;
    std::wstring version;
    std::wstring licenseId;
    std::wstring reason;
};

struct TrustedEngineRun {
    EngineTrust trust;
    ProcessResult process;
    bool outputContractValid{false};
};

// Returns the protected catalog of authorized providers embedded in the binary.
std::vector<ProviderIdentity> GetEmbeddedProviderCatalog();

// Finds a provider entry in the protected embedded catalog by logical name.
const ProviderIdentity* FindEmbeddedProvider(std::wstring_view logicalName);

#ifdef LAPSURE_ENABLE_TEST_HOOKS
// Test hooks to register/clear protected test providers.
void RegisterTestProtectedProvider(const ProviderIdentity& identity);
void ClearTestProtectedProviders();
#endif

// Verifies a provider binary against the protected embedded catalog.
EngineTrust VerifyEngine(const std::wstring& appDir,
                         const std::wstring& relativePath,
                         const std::wstring& logicalName);

// Execution boundary for bundled engines with TOCTOU handle-lock protection.
// Opens file handle with FILE_SHARE_READ, computes SHA-256 from the locked handle,
// matches against the embedded provider catalog, and executes CreateProcessW while
// holding the handle open.
TrustedEngineRun RunTrustedEngineCapture(const std::wstring& appDir,
                                         const std::wstring& relativePath,
                                         const std::wstring& logicalName,
                                         const std::vector<std::wstring>& arguments,
                                         unsigned timeoutMs = 15000,
                                         const std::atomic_bool* cancel = nullptr);
} // namespace lap

