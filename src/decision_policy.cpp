#include "lap/decision_policy.h"
#include <algorithm>
#include <cwctype>
#include <initializer_list>

namespace lap {
namespace {

std::wstring Lower(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    return value;
}

bool ContainsAny(const std::wstring& value, std::initializer_list<const wchar_t*> tokens) {
    for (const auto* token : tokens) {
        if (value.find(token) != std::wstring::npos) return true;
    }
    return false;
}

bool IsKnownDiscrete(const std::wstring& lowerName) {
    return ContainsAny(lowerName,
        {L"nvidia", L"rtx", L"quadro", L"radeon pro", L"radeon rx", L"firepro"});
}

bool IsKnownIntegrated(const std::wstring& lowerName) {
    return ContainsAny(lowerName,
        {L"intel uhd", L"intel iris", L"amd radeon(tm) graphics", L"amd radeon graphics"});
}

} // namespace

ObservedCapabilities NormalizeObservedCapabilities(const AuditReport& report) {
    ObservedCapabilities out{};
    auto& discreteGpu = out.discreteGpu;

    if (report.hardware.gpuInventoryStatus != ProviderCollectionStatus::Complete) {
        discreteGpu.state = CapabilityTruth::Unknown;
        discreteGpu.evidence = L"GPU inventory provider did not complete successfully.";
        return out;
    }

    if (report.hardware.gpus.empty()) {
        discreteGpu.state = CapabilityTruth::Unknown;
        discreteGpu.evidence = L"GPU inventory completed but returned no usable adapters.";
        return out;
    }

    bool allIntegrated = true;
    for (const auto& gpu : report.hardware.gpus) {
        const auto name = Lower(gpu.name);
        if (name.empty()) {
            allIntegrated = false;
            continue;
        }
        if (IsKnownDiscrete(name)) {
            discreteGpu.state = CapabilityTruth::Present;
            discreteGpu.evidence = L"Completed GPU inventory identified a recognized discrete adapter.";
            return out;
        }
        if (!IsKnownIntegrated(name)) allIntegrated = false;
    }

    if (allIntegrated) {
        discreteGpu.state = CapabilityTruth::AbsentConfirmed;
        discreteGpu.evidence = L"Completed GPU inventory returned only recognized integrated adapters.";
    } else {
        discreteGpu.state = CapabilityTruth::Unknown;
        discreteGpu.evidence = L"Completed GPU inventory contained an unclassified adapter.";
    }
    return out;
}

} // namespace lap
