#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace lap {

struct OutputValidationResult {
    bool valid{false};
    std::wstring reason;
    size_t recordCount{0};
};

// Validates smartctl --scan-open stdout format
OutputValidationResult ValidateSmartctlScanOutput(const std::wstring& text);

// Validates smartctl -a -j JSON stdout format
OutputValidationResult ValidateSmartctlJsonOutput(const std::wstring& jsonText);

// Validates nvidia-smi CSV stdout format
OutputValidationResult ValidateNvidiaSmiCsvOutput(const std::wstring& csvText);

// Validates LibreHardwareMonitor sensor bridge pipe-delimited output
OutputValidationResult ValidateSensorBridgeOutput(const std::wstring& pipeText);

// Validates discrete GPU VRAM stress test output
OutputValidationResult ValidateVramStressOutput(const std::wstring& text);

} // namespace lap
