#include "lap/cloud_lookup.h"
#include <windows.h>
#include <winhttp.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <iterator>
#include <algorithm>
#include <cwctype>

#pragma comment(lib, "winhttp.lib")

namespace lap {

namespace {

std::wstring ToW(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    if (n) MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

std::string ToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    if (n) WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

std::string JsonStr(const std::string& j, const char* k) {
    std::regex rx(std::string("\"") + k + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch m;
    return std::regex_search(j, m, rx) ? m[1].str() : "";
}

uint64_t JsonNum(const std::string& j, const char* k, uint64_t d = 0) {
    std::regex rx(std::string("\"") + k + "\"\\s*:\\s*([0-9]+(?:\\.[0-9]+)?)");
    std::smatch m;
    if (!std::regex_search(j, m, rx)) return d;
    return static_cast<uint64_t>(std::stod(m[1].str()));
}

bool JsonBool(const std::string& j, const char* k, bool d = false) {
    std::regex rx(std::string("\"") + k + "\"\\s*:\\s*(true|false)");
    std::smatch m;
    if (!std::regex_search(j, m, rx)) return d;
    return m[1].str() == "true";
}

std::wstring SanitizeFileName(std::wstring s) {
    for (auto& c : s) {
        if (c == L'\\' || c == L'/' || c == L':' || c == L'*' || c == L'?' || c == L'"' || c == L'<' || c == L'>' || c == L'|' || c == L' ') {
            c = L'_';
        }
    }
    return s;
}

} // anonymous namespace

std::string GetCurrentIsoTimestamp() {
    SYSTEMTIME st;
    GetSystemTime(&st);
    char buf[64];
    sprintf_s(buf, "%04d-%02d-%02dT%02d:%02d:%02dZ",
              st.wYear, st.wMonth, st.wDay,
              st.wHour, st.wMinute, st.wSecond);
    return buf;
}

FactoryProfile ParseFactoryProfileFromJson(const std::string& jsonStr, std::wstring* outCachedAt) {
    FactoryProfile p{};
    p.model = ToW(JsonStr(jsonStr, "model"));
    p.serviceTag = ToW(JsonStr(jsonStr, "serviceTag"));
    p.cpuContains = ToW(JsonStr(jsonStr, "cpuContains"));
    if (p.cpuContains.empty()) p.cpuContains = ToW(JsonStr(jsonStr, "cpu"));
    
    p.ramBytes = JsonNum(jsonStr, "ramBytes");
    p.ramSpeed = static_cast<unsigned>(JsonNum(jsonStr, "ramSpeed"));
    
    p.gpuContains = ToW(JsonStr(jsonStr, "gpuContains"));
    if (p.gpuContains.empty()) p.gpuContains = ToW(JsonStr(jsonStr, "gpu"));
    
    p.gpuVramBytes = JsonNum(jsonStr, "gpuVramBytes");
    p.diskMinBytes = JsonNum(jsonStr, "diskMinBytes");
    if (p.diskMinBytes == 0) p.diskMinBytes = JsonNum(jsonStr, "storageBytes");
    
    p.displayWidth = static_cast<unsigned>(JsonNum(jsonStr, "displayWidth"));
    p.displayHeight = static_cast<unsigned>(JsonNum(jsonStr, "displayHeight"));
    p.touchRequired = JsonBool(jsonStr, "touchRequired");
    p.batteryDesignWh = static_cast<double>(JsonNum(jsonStr, "batteryDesignWh"));
    p.adapterW = static_cast<unsigned>(JsonNum(jsonStr, "adapterW"));

    if (outCachedAt) {
        *outCachedAt = ToW(JsonStr(jsonStr, "cachedAt"));
    }
    return p;
}

std::string SerializeFactoryProfileToJson(const FactoryProfile& p, const std::wstring& evidenceSource, const std::string& timestamp) {
    std::string ts = timestamp.empty() ? GetCurrentIsoTimestamp() : timestamp;
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"schemaVersion\": 2,\n";
    ss << "  \"model\": \"" << ToUtf8(p.model) << "\",\n";
    ss << "  \"serviceTag\": \"" << ToUtf8(p.serviceTag) << "\",\n";
    ss << "  \"cpuContains\": \"" << ToUtf8(p.cpuContains) << "\",\n";
    ss << "  \"ramBytes\": " << p.ramBytes << ",\n";
    ss << "  \"ramSpeed\": " << p.ramSpeed << ",\n";
    ss << "  \"gpuContains\": \"" << ToUtf8(p.gpuContains) << "\",\n";
    ss << "  \"gpuVramBytes\": " << p.gpuVramBytes << ",\n";
    ss << "  \"diskMinBytes\": " << p.diskMinBytes << ",\n";
    ss << "  \"displayWidth\": " << p.displayWidth << ",\n";
    ss << "  \"displayHeight\": " << p.displayHeight << ",\n";
    ss << "  \"touchRequired\": " << (p.touchRequired ? "true" : "false") << ",\n";
    ss << "  \"batteryDesignWh\": " << static_cast<uint64_t>(p.batteryDesignWh) << ",\n";
    ss << "  \"adapterW\": " << p.adapterW << ",\n";
    ss << "  \"cachedAt\": \"" << ts << "\",\n";
    ss << "  \"sourceEngine\": \"LapSure OEM Engine v0.1.1\",\n";
    ss << "  \"evidence\": \"" << ToUtf8(evidenceSource) << "\"\n";
    ss << "}\n";
    return ss.str();
}

bool SaveFactoryProfileToLocalCache(const std::wstring& appDir, const FactoryProfile& profile, const std::wstring& vendor) {
    if (profile.serviceTag.empty() && profile.model.empty()) return false;
    
    std::error_code ec;
    auto cacheDir = std::filesystem::path(appDir) / L"profiles" / L"cache";
    std::filesystem::create_directories(cacheDir, ec);
    if (ec) return false;

    std::wstring fn;
    if (!vendor.empty()) {
        fn += SanitizeFileName(vendor) + L"_";
    }
    if (!profile.model.empty()) {
        fn += SanitizeFileName(profile.model) + L"_";
    }
    if (!profile.serviceTag.empty()) {
        fn += SanitizeFileName(profile.serviceTag);
    } else {
        fn += L"generic";
    }
    fn += L".json";

    auto filePath = cacheDir / fn;
    std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
    if (!out) return false;

    std::wstring evidence = L"OEM Cloud Lookup Cached (" + (vendor.empty() ? L"Universal" : vendor) + L")";
    auto json = SerializeFactoryProfileToJson(profile, evidence);
    out.write(json.data(), json.size());
    return true;
}

bool FetchHttpsJson(const std::wstring& url, std::string& outBody, std::wstring& outError, unsigned timeoutMs) {
    outBody.clear();
    outError.clear();

    URL_COMPONENTS urlComp{};
    urlComp.dwStructSize = sizeof(urlComp);
    wchar_t hostName[256] = {0};
    wchar_t urlPath[1024] = {0};
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = ARRAYSIZE(hostName);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = ARRAYSIZE(urlPath);

    if (!WinHttpCrackUrl(url.c_str(), (DWORD)url.size(), 0, &urlComp)) {
        outError = L"Invalid URL format.";
        return false;
    }

    HINTERNET hSession = WinHttpOpen(L"LapSure-EvidenceEngine/0.1.1",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS,
                                     0);
    if (!hSession) {
        outError = L"Failed to initialize WinHTTP session.";
        return false;
    }

    WinHttpSetTimeouts(hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

    HINTERNET hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        outError = L"WinHTTP connect failed.";
        return false;
    }

    DWORD reqFlags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
                                            L"GET",
                                            urlPath,
                                            nullptr,
                                            WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            reqFlags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        outError = L"WinHTTP open request failed.";
        return false;
    }

    BOOL bResults = WinHttpSendRequest(hRequest,
                                       WINHTTP_NO_ADDITIONAL_HEADERS,
                                       0,
                                       WINHTTP_NO_REQUEST_DATA,
                                       0,
                                       0,
                                       0);

    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, nullptr);
    }

    if (!bResults) {
        DWORD err = GetLastError();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        outError = L"WinHTTP request failed or timed out (Code: " + std::to_wstring(err) + L").";
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &statusCode,
                        &statusCodeSize,
                        WINHTTP_NO_HEADER_INDEX);

    if (statusCode != 200) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        outError = L"HTTP status code: " + std::to_wstring(statusCode);
        return false;
    }

    DWORD bytesAvailable = 0;
    std::string responseData;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        std::vector<char> buffer(bytesAvailable);
        DWORD bytesRead = 0;
        if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead) && bytesRead > 0) {
            responseData.append(buffer.data(), bytesRead);
        } else {
            break;
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    outBody = responseData;
    return !outBody.empty();
}

std::wstring BuildOemLookupUrl(const std::wstring& vendor, const std::wstring& model, const std::wstring& serviceTag) {
    // LapSure Standardized OEM Gateway Endpoint format
    std::wstring url = L"https://api.lapsure.io/v1/factory?tag=" + serviceTag;
    if (!vendor.empty()) {
        url += L"&vendor=" + vendor;
    }
    if (!model.empty()) {
        url += L"&model=" + model;
    }
    return url;
}

CloudLookupResult LookupFactoryProfileOnline(const std::wstring& appDir,
                                            const std::wstring& vendor,
                                            const std::wstring& model,
                                            const std::wstring& serviceTag,
                                            unsigned timeoutMs) {
    CloudLookupResult res{};
    if (serviceTag.empty() && model.empty()) {
        res.error = L"Missing Service Tag / Serial number for OEM Lookup.";
        return res;
    }

    // 1. Check Local Cache First (<appDir>/profiles/cache/)
    auto cacheDir = std::filesystem::path(appDir) / L"profiles" / L"cache";
    std::error_code ec;
    if (std::filesystem::exists(cacheDir, ec)) {
        for (auto& entry : std::filesystem::directory_iterator(cacheDir, ec)) {
            if (!entry.is_regular_file() || entry.path().extension() != L".json") continue;
            std::ifstream f(entry.path(), std::ios::binary);
            std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            std::wstring cachedAt;
            auto parsed = ParseFactoryProfileFromJson(content, &cachedAt);
            if (!serviceTag.empty() && !parsed.serviceTag.empty()) {
                auto a = serviceTag, b = parsed.serviceTag;
                std::transform(a.begin(), a.end(), a.begin(), towlower);
                std::transform(b.begin(), b.end(), b.begin(), towlower);
                if (a == b) {
                    res.success = true;
                    res.fromCache = true;
                    res.profile = parsed;
                    res.source = entry.path().wstring();
                    res.rawJson = ToW(content);
                    res.cachedAt = cachedAt;
                    return res;
                }
            }
        }
    }

    // 2. Query Cloud API via WinHTTP
    std::wstring url = BuildOemLookupUrl(vendor, model, serviceTag);
    std::string body;
    std::wstring err;
    if (!FetchHttpsJson(url, body, err, timeoutMs)) {
        res.error = L"Cloud Lookup failed: " + err;
        return res;
    }

    // 3. Validate and Parse Schema
    std::wstring cachedAt;
    auto profile = ParseFactoryProfileFromJson(body, &cachedAt);
    if (profile.model.empty()) {
        res.error = L"Cloud response did not contain valid FactoryProfile schema.";
        return res;
    }

    if (profile.serviceTag.empty()) {
        profile.serviceTag = serviceTag;
    }

    // 4. Save to Local Cache for future offline / WinPE runs
    SaveFactoryProfileToLocalCache(appDir, profile, vendor);

    res.success = true;
    res.fromCache = false;
    res.profile = profile;
    res.source = L"OEM Cloud Lookup (" + url + L")";
    res.rawJson = ToW(body);
    res.cachedAt = cachedAt.empty() ? ToW(GetCurrentIsoTimestamp()) : cachedAt;
    return res;
}

PreCacheSummary RunBatchPreCache(const std::wstring& appDir,
                                const std::vector<std::wstring>& serviceTags,
                                const std::wstring& vendor,
                                unsigned timeoutMs) {
    PreCacheSummary summary{};
    summary.total = static_cast<unsigned>(serviceTags.size());

    for (const auto& tag : serviceTags) {
        if (tag.empty()) continue;
        auto result = LookupFactoryProfileOnline(appDir, vendor, L"", tag, timeoutMs);
        if (result.success) {
            summary.succeeded++;
            if (result.fromCache) {
                summary.fromCache++;
                summary.details.push_back({tag, L"Đã tồn tại trong Cache (" + result.source + L")"});
            } else {
                summary.details.push_back({tag, L"Tải thành công từ OEM Cloud và lưu vào Cache"});
            }
        } else {
            summary.failed++;
            summary.details.push_back({tag, L"Thất bại: " + result.error});
        }
    }
    return summary;
}

} // namespace lap
