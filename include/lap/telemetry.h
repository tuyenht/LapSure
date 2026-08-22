#pragma once
#include "model.h"
#include "environment.h"
#include <atomic>
#include <functional>
#include <string>
namespace lap {
double ReadSystemCpuUtilizationPercent();
TelemetrySample SampleTelemetry(unsigned second,const Capabilities& caps,const std::wstring& appDir,const std::atomic_bool* cancel);
}
