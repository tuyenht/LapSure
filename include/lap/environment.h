#pragma once
#include <string>
namespace lap {
struct Capabilities {
    bool winPE{};
    bool powershell{};
    bool smartctl{};
    bool nvidiaSmi{};
    bool wmi{};
    bool battery{};
};
Capabilities DetectCapabilities(const std::wstring& appDir);
std::wstring EnvironmentName(const Capabilities& c);
}
