#include "lap/provider_output.h"
#include <algorithm>
#include <cwctype>
#include <regex>
#include <sstream>
#include <vector>

namespace lap {
namespace {
std::wstring TrimString(const std::wstring& s) {
    const auto first = s.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return L"";
    const auto last = s.find_last_not_of(L" \t\r\n");
    return s.substr(first, last - first + 1);
}

std::vector<std::wstring> SplitTextLines(const std::wstring& text) {
    std::vector<std::wstring> lines;
    std::wistringstream stream(text);
    std::wstring line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}
} // namespace

OutputValidationResult ValidateSmartctlScanOutput(const std::wstring& text) {
    OutputValidationResult result{};
    const auto trimmed = TrimString(text);
    if (trimmed.empty()) {
        result.reason = L"Empty smartctl scan output";
        return result;
    }
    const auto lines = SplitTextLines(trimmed);
    for (const auto& line : lines) {
        const auto t = TrimString(line);
        if (t.rfind(L"/dev/", 0) == 0 || t.rfind(L"/dev/sd", 0) == 0 || t.rfind(L"/dev/nvme", 0) == 0) {
            ++result.recordCount;
        }
    }
    if (result.recordCount == 0) {
        result.reason = L"No valid storage device paths found in scan output";
        return result;
    }
    result.valid = true;
    return result;
}

OutputValidationResult ValidateSmartctlJsonOutput(const std::wstring& jsonText) {
    OutputValidationResult result{};
    const auto trimmed = TrimString(jsonText);
    if (trimmed.empty() || trimmed.front() != L'{' || trimmed.back() != L'}') {
        result.reason = L"Malformed smartctl JSON object: missing curly braces";
        return result;
    }
    std::wregex passedRx(LR"(\"passed\"\s*:\s*(true|false))");
    if (!std::regex_search(trimmed, passedRx)) {
        result.reason = L"Missing explicit SMART health 'passed' boolean in JSON payload";
        return result;
    }
    result.valid = true;
    result.recordCount = 1;
    return result;
}

OutputValidationResult ValidateNvidiaSmiCsvOutput(const std::wstring& csvText) {
    OutputValidationResult result{};
    const auto trimmed = TrimString(csvText);
    if (trimmed.empty()) {
        result.reason = L"Empty nvidia-smi CSV output";
        return result;
    }
    const auto lines = SplitTextLines(trimmed);
    for (const auto& line : lines) {
        const auto t = TrimString(line);
        if (t.empty() || t[0] == L'#') continue;
        size_t commaCount = std::count(t.begin(), t.end(), L',');
        if (commaCount >= 3) {
            ++result.recordCount;
        }
    }
    if (result.recordCount == 0) {
        result.reason = L"No valid CSV rows parsed from nvidia-smi output";
        return result;
    }
    result.valid = true;
    return result;
}

OutputValidationResult ValidateSensorBridgeOutput(const std::wstring& pipeText) {
    OutputValidationResult result{};
    const auto trimmed = TrimString(pipeText);
    if (trimmed.empty()) {
        result.reason = L"Empty sensor bridge output";
        return result;
    }
    const auto lines = SplitTextLines(trimmed);
    for (const auto& line : lines) {
        const auto t = TrimString(line);
        if (t.empty()) continue;
        size_t pipeCount = std::count(t.begin(), t.end(), L'|');
        if (pipeCount >= 3) {
            ++result.recordCount;
        }
    }
    if (result.recordCount == 0) {
        result.reason = L"Missing required pipe-delimited telemetry fields";
        return result;
    }
    result.valid = true;
    return result;
}

OutputValidationResult ValidateVramStressOutput(const std::wstring& text) {
    OutputValidationResult result{};
    const auto trimmed = TrimString(text);
    if (trimmed.empty()) {
        result.reason = L"Empty VRAM stress output";
        return result;
    }
    if (trimmed.find(L"STATUS=PASS") != std::wstring::npos ||
        trimmed.find(L"STATUS=FAIL") != std::wstring::npos ||
        trimmed.find(L"ERRORS=") != std::wstring::npos) {
        result.valid = true;
        result.recordCount = 1;
        return result;
    }
    result.reason = L"VRAM stress output missing status/error markers";
    return result;
}

} // namespace lap
