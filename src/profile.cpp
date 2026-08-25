#include "lap/profile.h"
#include <windows.h>
#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>

namespace lap {
namespace {
constexpr uintmax_t kMaxStaticProfileBytes = 1024u * 1024u;

bool ReadUtf8Bounded(const std::filesystem::path& path, std::string& out) {
    out.clear();
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec) || ec) return false;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size == 0 || size > kMaxStaticProfileBytes) return false;
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    out.resize(static_cast<size_t>(size));
    file.read(out.data(), static_cast<std::streamsize>(out.size()));
    return file.good() || file.eof();
}

std::wstring ToWide(const std::string& value) {
    if (value.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                          static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0) return {};
    std::wstring out(static_cast<size_t>(count), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                            static_cast<int>(value.size()), out.data(), count) != count) return {};
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

bool EqualsIdentity(std::wstring left, std::wstring right) {
    std::transform(left.begin(), left.end(), left.begin(), [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    std::transform(right.begin(), right.end(), right.begin(), [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    return left == right;
}

FactoryProfile ParseProfile(const std::string& json) {
    FactoryProfile profile{};
    profile.model = ToWide(JsonString(json, "model"));
    profile.serviceTag = ToWide(JsonString(json, "serviceTag"));
    profile.cpuContains = ToWide(JsonString(json, "cpuContains"));
    profile.ramBytes = JsonNumber(json, "ramBytes");
    profile.ramSpeed = static_cast<unsigned>(JsonNumber(json, "ramSpeed"));
    profile.gpuContains = ToWide(JsonString(json, "gpuContains"));
    profile.gpuVramBytes = JsonNumber(json, "gpuVramBytes");
    profile.diskMinBytes = JsonNumber(json, "diskMinBytes");
    profile.displayWidth = static_cast<unsigned>(JsonNumber(json, "displayWidth"));
    profile.displayHeight = static_cast<unsigned>(JsonNumber(json, "displayHeight"));
    profile.touchRequired = JsonBool(json, "touchRequired");
    profile.batteryDesignWh = static_cast<double>(JsonNumber(json, "batteryDesignWh"));
    profile.adapterW = static_cast<unsigned>(JsonNumber(json, "adapterW"));
    if (profile.cpuContains.empty()) profile.cpuContains = ToWide(JsonString(json, "cpu"));
    if (profile.gpuContains.empty()) profile.gpuContains = ToWide(JsonString(json, "gpu"));
    return profile;
}
} // namespace

ProfileLoadResult LoadFactoryProfile(const std::wstring& dir,
                                     const std::wstring&,
                                     const std::wstring& tag) {
    ProfileLoadResult out{};
    const std::filesystem::path root(dir);
    std::error_code ec;
    if (!std::filesystem::exists(root, ec) || ec) {
        out.error = L"profiles directory not found";
        return out;
    }
    if (tag.empty()) {
        out.error = L"No Service Tag identity; Generic Audit mode";
        return out;
    }

    unsigned exactMatches = 0;
    FactoryProfile matched{};
    std::wstring matchedSource;

    auto scanDir = [&](const std::filesystem::path& scanRoot) {
        std::error_code scanError;
        for (std::filesystem::directory_iterator it(scanRoot, scanError), end; !scanError && it != end; it.increment(scanError)) {
            const auto& entry = *it;
            if (!entry.is_regular_file(scanError) || scanError || entry.path().extension() != L".json") {
                scanError.clear();
                continue;
            }
            std::string raw;
            if (!ReadUtf8Bounded(entry.path(), raw)) continue;
            const auto profile = ParseProfile(raw);
            if (profile.model.empty() || profile.serviceTag.empty()) continue;
            if (!EqualsIdentity(profile.serviceTag, tag)) continue;
            ++exactMatches;
            if (exactMatches == 1) {
                matched = profile;
                matchedSource = entry.path().wstring();
            }
        }
    };

    // Only reviewed static files directly under profiles/ participate in factory truth.
    // Mutable cache content is intentionally excluded from this loader.
    scanDir(root);

    if (exactMatches == 1) {
        out.profile = std::move(matched);
        out.exact = true;
        out.trustedProvenance = true;
        out.loaded = true;
        out.source = std::move(matchedSource);
        return out;
    }
    if (exactMatches > 1) {
        out.error = L"Ambiguous exact Service Tag profile; multiple reviewed profiles match.";
        return out;
    }

    out.error = L"No reviewed exact Service Tag profile; Generic Audit mode";
    return out;
}
} // namespace lap
