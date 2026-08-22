#pragma once
#include "model.h"
#include "environment.h"
#include <atomic>
#include <string>
namespace lap {
struct SensorProviderResult {
    bool available{false};
    std::wstring providerName;
    Confidence confidence{Confidence::Low};
    SensorValue cpuPackageTemp;
    SensorValue cpuPackagePower;
    SensorValue cpuPackageClock;
    SensorValue cpuThermalThrottle;
    std::wstring evidence;
};
SensorProviderResult ReadCpuSensors(const Capabilities& caps,const std::wstring& appDir,const std::atomic_bool* cancel);
}
