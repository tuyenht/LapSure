#include "lap/cloud_lookup.h"
#include <windows.h>
#include <winhttp.h>
#include <algorithm>
#include <climits>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")

namespace lap {
namespace {
constexpr size_t kMaxCloudResponseBytes = 1024u * 1024u;
constexpr uintmax_t kMaxCloudCacheBytes = 1024u * 1024u;
constexpr size_t kMaxCloudUrlChars = 2048u;
constexpr size_t kMaxServiceTagChars = 128u;
constexpr size_t kMaxVendorChars = 256u;
constexpr size_t kMaxModelChars = 512u;
constexpr wchar_t kOemHost[] = L"api.lapsure.io";

std::wstring ToWide(const std::string& value) {
    if (value.empty() || value.size() > static_cast<size_t>(INT_MAX)) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                          static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0) return {};
    std::wstring out(static_cast<size_t>(count), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                            static_cast<int>(value.size()), out.data(), count) != count) return {};
    return out;
}

std::string ToUtf8(const std::wstring& value) {
    if (value.empty() || value.size() > static_cast<size_t>(INT_MAX)) return {};
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                                          static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (count <= 0) return {};
    std::string out(static_cast<size_t>(count), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
                            out.data(), count, nullptr, nullptr) != count) return {};
    return out;
}

std::string JsonString(const std::string& json, const char* key) {
    const std::regex rx(std::string("\"") + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    return std::regex_search(json, match, rx) ? match[1].str() : std::string{};
}

uint64_t JsonNumber(const std::string& json, const char* key, uint64_t fallback = 0) {
    const std::regex rx(std::string("\"") + key + "\"\\s*:\\s*([0-9]+(?:\\.[0-9]+)?)");
    std::smatch match;
    if (!std::regex_search(json, match, rx)) return fallback;
    try { return static_cast<uint64_t>(std::stod(match[1].str())); }
    catch (...) { return fallback; }
}

bool JsonBool(const std::string& json, const char* key, bool fallback = false) {
    const std::regex rx(std::string("\"") + key + "\"\\s*:\\s*(true|false)");
    std::smatch match;
    if (!std::regex_search(json, match, rx)) return fallback;
    return match[1].str() == "true";
}

bool IdentityEquals(std::wstring left, std::wstring right) {
    std::transform(left.begin(), left.end(), left.begin(), [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    std::transform(right.begin(), right.end(), right.begin(), [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    return left == right;
}

bool ValidLookupInput(const std::wstring& vendor, const std::wstring& model, const std::wstring& serviceTag) {
    return !serviceTag.empty() && serviceTag.size() <= kMaxServiceTagChars &&
           vendor.size() <= kMaxVendorChars && model.size() <= kMaxModelChars;
}

std::wstring PercentEncodeQueryComponent(const std::wstring& value) {
    const auto bytes = ToUtf8(value);
    static constexpr wchar_t hex[] = L"0123456789ABCDEF";
    std::wstring out;
    out.reserve(bytes.size() * 3u);
    for (unsigned char byte : bytes) {
        const bool unreserved = (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') ||
                                (byte >= '0' && byte <= '9') || byte == '-' || byte == '.' ||
                                byte == '_' || byte == '~';
        if (unreserved) {
            out.push_back(static_cast<wchar_t>(byte));
        } else {
            out.push_back(L'%');
            out.push_back(hex[(byte >> 4u) & 0x0Fu]);
            out.push_back(hex[byte & 0x0Fu]);
        }
    }
    return out;
}

std::wstring SanitizeFileName(std::wstring value) {
    for (auto& c : value) {
        const bool safe = (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') ||
                          (c >= L'0' && c <= L'9') || c == L'-' || c == L'_' || c == L'.';
        if (!safe) c = L'_';
    }
    if (value.size() > 180u) value.resize(180u);
    return value;
}

bool ReadBoundedFile(const std::filesystem::path& path, std::string& out) {
    out.clear();
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec) || ec) return false;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size == 0 || size > kMaxCloudCacheBytes) return false;
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    out.resize(static_cast<size_t>(size));
    file.read(out.data(), static_cast<std::streamsize>(out.size()));
    return file.good() || file.eof();
}

std::string JsonEscape(const std::wstring& value) {
    const auto utf8 = ToUtf8(value);
    std::string out;
    out.reserve(utf8.size());
    for (unsigned char c : utf8) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c >= 0x20u) out.push_back(static_cast<char>(c));
            break;
        }
    }
    return out;
}
} // namespace

std::string GetCurrentIsoTimestamp() {
    SYSTEMTIME time{};
    GetSystemTime(&time);
    char buffer[64]{};
    sprintf_s(buffer, "%04u-%02u-%02uT%02u:%02u:%02uZ",
              time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
    return buffer;
}

FactoryProfile ParseFactoryProfileFromJson(const std::string& jsonStr, std::wstring* outCachedAt) {
    FactoryProfile profile{};
    if (jsonStr.empty() || jsonStr.size() > kMaxCloudResponseBytes) return profile;
    profile.model = ToWide(JsonString(jsonStr, "model"));
    profile.serviceTag = ToWide(JsonString(jsonStr, "serviceTag"));
    profile.cpuContains = ToWide(JsonString(jsonStr, "cpuContains"));
    if (profile.cpuContains.empty()) profile.cpuContains = ToWide(JsonString(jsonStr, "cpu"));
    profile.ramBytes = JsonNumber(jsonStr, "ramBytes");
    profile.ramSpeed = static_cast<unsigned>(JsonNumber(jsonStr, "ramSpeed"));
    profile.gpuContains = ToWide(JsonString(jsonStr, "gpuContains"));
    if (profile.gpuContains.empty()) profile.gpuContains = ToWide(JsonString(jsonStr, "gpu"));
    profile.gpuVramBytes = JsonNumber(jsonStr, "gpuVramBytes");
    profile.diskMinBytes = JsonNumber(jsonStr, "diskMinBytes");
    if (profile.diskMinBytes == 0) profile.diskMinBytes = JsonNumber(jsonStr, "storageBytes");
    profile.displayWidth = static_cast<unsigned>(JsonNumber(jsonStr, "displayWidth"));
    profile.displayHeight = static_cast<unsigned>(JsonNumber(jsonStr, "displayHeight"));
    profile.touchRequired = JsonBool(jsonStr, "touchRequired");
    profile.batteryDesignWh = static_cast<double>(JsonNumber(jsonStr, "batteryDesignWh"));
    profile.adapterW = static_cast<unsigned>(JsonNumber(jsonStr, "adapterW"));
    if (outCachedAt) *outCachedAt = ToWide(JsonString(jsonStr, "cachedAt"));
    return profile;
}

std::string SerializeFactoryProfileToJson(const FactoryProfile& profile,
                                         const std::wstring& evidenceSource,
                                         const std::string& timestamp) {
    const std::string time = timestamp.empty() ? GetCurrentIsoTimestamp() : timestamp;
    std::ostringstream stream;
    stream << "{\n";
    stream << "  \"schemaVersion\": 2,\n";
    stream << "  \"model\": \"" << JsonEscape(profile.model) << "\",\n";
    stream << "  \"serviceTag\": \"" << JsonEscape(profile.serviceTag) << "\",\n";
    stream << "  \"cpuContains\": \"" << JsonEscape(profile.cpuContains) << "\",\n";
    stream << "  \"ramBytes\": " << profile.ramBytes << ",\n";
    stream << "  \"ramSpeed\": " << profile.ramSpeed << ",\n";
    stream << "  \"gpuContains\": \"" << JsonEscape(profile.gpuContains) << "\",\n";
    stream << "  \"gpuVramBytes\": " << profile.gpuVramBytes << ",\n";
    stream << "  \"diskMinBytes\": " << profile.diskMinBytes << ",\n";
    stream << "  \"displayWidth\": " << profile.displayWidth << ",\n";
    stream << "  \"displayHeight\": " << profile.displayHeight << ",\n";
    stream << "  \"touchRequired\": " << (profile.touchRequired ? "true" : "false") << ",\n";
    stream << "  \"batteryDesignWh\": " << static_cast<uint64_t>(profile.batteryDesignWh) << ",\n";
    stream << "  \"adapterW\": " << profile.adapterW << ",\n";
    stream << "  \"cachedAt\": \"" << time << "\",\n";
    stream << "  \"sourceEngine\": \"LapSure OEM Engine v0.1.1\",\n";
    stream << "  \"evidence\": \"" << JsonEscape(evidenceSource) << "\"\n";
    stream << "}\n";
    return stream.str();
}

bool SaveFactoryProfileToLocalCache(const std::wstring& appDir,
                                    const FactoryProfile& profile,
                                    const std::wstring& vendor) {
    if (profile.serviceTag.empty() || profile.serviceTag.size() > kMaxServiceTagChars || profile.model.empty()) return false;
    std::error_code ec;
    const auto cacheDir = std::filesystem::path(appDir) / L"profiles" / L"cache";
    std::filesystem::create_directories(cacheDir, ec);
    if (ec) return false;

    std::wstring filename;
    if (!vendor.empty()) filename += SanitizeFileName(vendor) + L"_";
    filename += SanitizeFileName(profile.model) + L"_" + SanitizeFileName(profile.serviceTag) + L".json";
    const auto filePath = cacheDir / filename;
    const auto tempPath = filePath.wstring() + L".tmp";
    const auto json = SerializeFactoryProfileToJson(
        profile, L"OEM Cloud Lookup Cached (advisory; unauthenticated profile provenance)");
    if (json.empty() || json.size() > kMaxCloudCacheBytes) return false;

    std::ofstream out(std::filesystem::path(tempPath), std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(json.data(), static_cast<std::streamsize>(json.size()));
    out.flush();
    if (!out.good()) { out.close(); DeleteFileW(tempPath.c_str()); return false; }
    out.close();
    if (!MoveFileExW(tempPath.c_str(), filePath.wstring().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(tempPath.c_str());
        return false;
    }
    return true;
}

bool FetchHttpsJson(const std::wstring& url,
                    std::string& outBody,
                    std::wstring& outError,
                    unsigned timeoutMs) {
    outBody.clear();
    outError.clear();
    if (url.empty() || url.size() > kMaxCloudUrlChars) {
        outError = L"OEM URL is empty or exceeds the bounded request size.";
        return false;
    }

    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(components);
    wchar_t hostName[256]{};
    wchar_t urlPath[2048]{};
    wchar_t extraInfo[2048]{};
    components.lpszHostName = hostName;
    components.dwHostNameLength = static_cast<DWORD>(std::size(hostName));
    components.lpszUrlPath = urlPath;
    components.dwUrlPathLength = static_cast<DWORD>(std::size(urlPath));
    components.lpszExtraInfo = extraInfo;
    components.dwExtraInfoLength = static_cast<DWORD>(std::size(extraInfo));
    if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &components)) {
        outError = L"Invalid OEM URL format.";
        return false;
    }
    const std::wstring host(components.lpszHostName, components.dwHostNameLength);
    if (components.nScheme != INTERNET_SCHEME_HTTPS || !IdentityEquals(host, kOemHost) ||
        components.nPort != INTERNET_DEFAULT_HTTPS_PORT) {
        outError = L"OEM lookup requires the pinned HTTPS LapSure host.";
        return false;
    }

    std::wstring requestTarget(components.lpszUrlPath, components.dwUrlPathLength);
    requestTarget.append(components.lpszExtraInfo, components.dwExtraInfoLength);
    if (requestTarget.empty() || requestTarget.size() > kMaxCloudUrlChars) {
        outError = L"OEM request target exceeds the bounded request size.";
        return false;
    }

    HINTERNET session = WinHttpOpen(L"LapSure-EvidenceEngine/0.1.1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) { outError = L"Failed to initialize WinHTTP session."; return false; }
    WinHttpSetTimeouts(session, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

    HINTERNET connection = WinHttpConnect(session, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connection) {
        WinHttpCloseHandle(session);
        outError = L"WinHTTP connect failed.";
        return false;
    }
    HINTERNET request = WinHttpOpenRequest(connection, L"GET", requestTarget.c_str(), nullptr,
                                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!request) {
        WinHttpCloseHandle(connection); WinHttpCloseHandle(session);
        outError = L"WinHTTP open request failed.";
        return false;
    }

    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if (!WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy))) {
        WinHttpCloseHandle(request); WinHttpCloseHandle(connection); WinHttpCloseHandle(session);
        outError = L"Could not disable OEM HTTP redirects.";
        return false;
    }

    BOOL ok = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                 WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (ok) ok = WinHttpReceiveResponse(request, nullptr);
    if (!ok) {
        const DWORD error = GetLastError();
        WinHttpCloseHandle(request); WinHttpCloseHandle(connection); WinHttpCloseHandle(session);
        outError = L"WinHTTP request failed or timed out (Code: " + std::to_wstring(error) + L").";
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX) ||
        statusCode != 200) {
        WinHttpCloseHandle(request); WinHttpCloseHandle(connection); WinHttpCloseHandle(session);
        outError = L"OEM endpoint returned a non-200 response.";
        return false;
    }

    std::string responseData;
    for (;;) {
        DWORD bytesAvailable = 0;
        if (!WinHttpQueryDataAvailable(request, &bytesAvailable)) {
            WinHttpCloseHandle(request); WinHttpCloseHandle(connection); WinHttpCloseHandle(session);
            outError = L"Could not determine OEM response size.";
            return false;
        }
        if (bytesAvailable == 0) break;
        if (bytesAvailable > kMaxCloudResponseBytes - responseData.size()) {
            WinHttpCloseHandle(request); WinHttpCloseHandle(connection); WinHttpCloseHandle(session);
            outError = L"OEM response exceeded the bounded response size.";
            return false;
        }
        std::vector<char> buffer(bytesAvailable);
        DWORD bytesRead = 0;
        if (!WinHttpReadData(request, buffer.data(), bytesAvailable, &bytesRead)) {
            WinHttpCloseHandle(request); WinHttpCloseHandle(connection); WinHttpCloseHandle(session);
            outError = L"Could not read OEM response.";
            return false;
        }
        responseData.append(buffer.data(), bytesRead);
    }

    WinHttpCloseHandle(request); WinHttpCloseHandle(connection); WinHttpCloseHandle(session);
    if (responseData.empty()) { outError = L"OEM response was empty."; return false; }
    outBody = std::move(responseData);
    return true;
}

std::wstring BuildOemLookupUrl(const std::wstring& vendor,
                               const std::wstring& model,
                               const std::wstring& serviceTag) {
    if (!ValidLookupInput(vendor, model, serviceTag)) return {};
    std::wstring url = L"https://api.lapsure.io/v1/factory?tag=" + PercentEncodeQueryComponent(serviceTag);
    if (!vendor.empty()) url += L"&vendor=" + PercentEncodeQueryComponent(vendor);
    if (!model.empty()) url += L"&model=" + PercentEncodeQueryComponent(model);
    return url.size() <= kMaxCloudUrlChars ? url : std::wstring{};
}

CloudLookupResult LookupFactoryProfileOnline(const std::wstring& appDir,
                                            const std::wstring& vendor,
                                            const std::wstring& model,
                                            const std::wstring& serviceTag,
                                            unsigned timeoutMs,
                                            bool allowNetwork) {
    CloudLookupResult res{};
    res.authenticatedProvenance = false;
    if (!allowNetwork) {
        res.error = L"Cloud lookup is disabled by default; use explicit technician pre-cache to opt in.";
        return res;
    }
    if (!ValidLookupInput(vendor, model, serviceTag)) {
        res.error = L"Invalid or missing bounded Service Tag / model input for OEM lookup.";
        return res;
    }

    const auto cacheDir = std::filesystem::path(appDir) / L"profiles" / L"cache";
    std::error_code ec;
    if (std::filesystem::exists(cacheDir, ec) && !ec) {
        for (std::filesystem::directory_iterator it(cacheDir, ec), end; !ec && it != end; it.increment(ec)) {
            const auto& entry = *it;
            if (!entry.is_regular_file(ec) || ec || entry.path().extension() != L".json") {
                ec.clear();
                continue;
            }
            std::string content;
            if (!ReadBoundedFile(entry.path(), content)) continue;
            std::wstring cachedAt;
            const auto parsed = ParseFactoryProfileFromJson(content, &cachedAt);
            if (!parsed.serviceTag.empty() && IdentityEquals(parsed.serviceTag, serviceTag)) {
                res.success = true;
                res.fromCache = true;
                res.identityMatched = true;
                res.authenticatedProvenance = false;
                res.profile = parsed;
                res.source = L"OEM profile cache (advisory; unauthenticated profile provenance)";
                res.rawJson = ToWide(content);
                res.cachedAt = cachedAt;
                return res;
            }
        }
    }

    const auto url = BuildOemLookupUrl(vendor, model, serviceTag);
    if (url.empty()) {
        res.error = L"OEM lookup URL could not be constructed within privacy bounds.";
        return res;
    }
    std::string body;
    std::wstring error;
    if (!FetchHttpsJson(url, body, error, timeoutMs)) {
        res.error = L"Cloud Lookup failed: " + error;
        return res;
    }
    if (JsonNumber(body, "schemaVersion") != 2) {
        res.error = L"Cloud response schema version is missing or unsupported.";
        return res;
    }

    std::wstring cachedAt;
    const auto profile = ParseFactoryProfileFromJson(body, &cachedAt);
    if (profile.model.empty() || profile.serviceTag.empty() || !IdentityEquals(profile.serviceTag, serviceTag)) {
        res.error = L"Cloud response identity did not exactly match the requested Service Tag.";
        return res;
    }

    res.identityMatched = true;
    res.authenticatedProvenance = false;
    (void)SaveFactoryProfileToLocalCache(appDir, profile, vendor);
    res.success = true;
    res.fromCache = false;
    res.profile = profile;
    res.source = L"OEM Cloud Lookup (advisory; unauthenticated profile provenance)";
    res.rawJson = ToWide(body);
    res.cachedAt = cachedAt.empty() ? ToWide(GetCurrentIsoTimestamp()) : cachedAt;
    return res;
}

PreCacheSummary RunBatchPreCache(const std::wstring& appDir,
                                const std::vector<std::wstring>& serviceTags,
                                const std::wstring& vendor,
                                unsigned timeoutMs) {
    PreCacheSummary summary{};
    summary.total = static_cast<unsigned>(serviceTags.size());
    for (const auto& tag : serviceTags) {
        if (tag.empty()) { ++summary.failed; summary.details.push_back({tag, L"Thất bại: Service Tag trống"}); continue; }
        const auto result = LookupFactoryProfileOnline(appDir, vendor, L"", tag, timeoutMs, true);
        if (result.success) {
            ++summary.succeeded;
            if (result.fromCache) ++summary.fromCache;
            summary.details.push_back({tag, result.fromCache
                ? L"Đã có trong cache tư vấn; chưa phải factory truth đã xác thực"
                : L"Đã tải và cache tư vấn; chưa phải factory truth đã xác thực"});
        } else {
            ++summary.failed;
            summary.details.push_back({tag, L"Thất bại: " + result.error});
        }
    }
    return summary;
}

} // namespace lap
