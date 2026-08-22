#pragma once
#include "model.h"
#include "environment.h"
#include "hardware.h"
#include <atomic>
#include <string>

namespace lap {
struct StressPlan {
    std::wstring mode{L"Quick"};
    unsigned cpuSeconds{30};
    unsigned ramSeconds{30};
    unsigned gpuSeconds{30};
};
StressPlan MakeStressPlan(const std::wstring& mode);
void RunStressSession(AuditReport& report, const Capabilities& caps, const std::wstring& appDir,
                      const StressPlan& plan, const std::atomic_bool* cancel);
}
