#pragma once
#include "model.h"
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace lap {

struct CloudLookupResult {
    bool success{false};
    bool fromCache{false};
    bool identityMatched{false};
    bool authenticatedProvenance{false};
    FactoryProfile profile{};
    std::wstring source;
    std::wstring error;
    std::wstring rawJson;
    std::wstring cachedAt;
};

struct PreCacheSummary {
    unsigned total{0};
    unsigned succeeded{0};
    unsigned fromCache{0};
    unsigned failed{0};
    std::vector<std::pair<std::wstring, std::wstring>> details;
};

std::string GetCurrentIsoTimestamp();
std::string SerializeFactoryProfileToJson(const FactoryProfile& profile,
                                         const std::wstring& evidenceSource = L"OEM Cloud Lookup",
                                         const std::string& timestamp = "");
FactoryProfile ParseFactoryProfileFromJson(const std::string& jsonStr, std::wstring* outCachedAt = nullptr);

// Mutable cache is advisory and is never promoted to factory truth by LoadFactoryProfile().
bool SaveFactoryProfileToLocalCache(const std::wstring& appDir,
                                    const FactoryProfile& profile,
                                    const std::wstring& vendor = L"");

// HTTPS-only, bounded OEM fetch primitive.
bool FetchHttpsJson(const std::wstring& url,
                    std::string& outBody,
                    std::wstring& outError,
                    unsigned timeoutMs = 3000);

std::wstring BuildOemLookupUrl(const std::wstring& vendor,
                               const std::wstring& model,
                               const std::wstring& serviceTag);

// Network access is disabled by default. Normal GUI and inventory-only callers
// therefore fail closed without transmitting hardware identity. Technician
// pre-cache is the explicit opt-in path and passes allowNetwork=true.
CloudLookupResult LookupFactoryProfileOnline(const std::wstring& appDir,
                                            const std::wstring& vendor,
                                            const std::wstring& model,
                                            const std::wstring& serviceTag,
                                            unsigned timeoutMs = 3000,
                                            bool allowNetwork = false);

PreCacheSummary RunBatchPreCache(const std::wstring& appDir,
                                const std::vector<std::wstring>& serviceTags,
                                const std::wstring& vendor = L"",
                                unsigned timeoutMs = 3000);

}
