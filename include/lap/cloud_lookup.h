#pragma once
#include "model.h"
#include <string>
#include <vector>
#include <utility>

namespace lap {

struct CloudLookupResult {
    bool success{false};
    bool fromCache{false};
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
    std::vector<std::pair<std::wstring, std::wstring>> details; // <tag, status_or_error_msg>
};

// Generates ISO-8601 UTC timestamp string (e.g. 2026-08-24T10:28:30Z)
std::string GetCurrentIsoTimestamp();

// Pure JSON serializer/deserializer for FactoryProfile with timestamp & provenance metadata
std::string SerializeFactoryProfileToJson(const FactoryProfile& p,
                                         const std::wstring& evidenceSource = L"OEM Cloud Lookup",
                                         const std::string& timestamp = "");
FactoryProfile ParseFactoryProfileFromJson(const std::string& jsonStr, std::wstring* outCachedAt = nullptr);

// Saves a factory profile to <appDir>/profiles/cache/<tag>.json with cachedAt metadata
bool SaveFactoryProfileToLocalCache(const std::wstring& appDir, const FactoryProfile& profile, const std::wstring& vendor = L"");

// Native WinHTTP HTTPS fetcher (TLS 1.2/1.3, strict timeout)
bool FetchHttpsJson(const std::wstring& url, std::string& outBody, std::wstring& outError, unsigned timeoutMs = 3000);

// Constructs OEM Cloud Lookup URL based on vendor, model, and service tag
std::wstring BuildOemLookupUrl(const std::wstring& vendor, const std::wstring& model, const std::wstring& serviceTag);

// Executes On-Demand OEM Cloud Lookup with automatic local disk caching
CloudLookupResult LookupFactoryProfileOnline(const std::wstring& appDir,
                                            const std::wstring& vendor,
                                            const std::wstring& model,
                                            const std::wstring& serviceTag,
                                            unsigned timeoutMs = 3000);

// Batch Pre-caching tool for technician USB setup
PreCacheSummary RunBatchPreCache(const std::wstring& appDir,
                                const std::vector<std::wstring>& serviceTags,
                                const std::wstring& vendor = L"",
                                unsigned timeoutMs = 3000);

}
