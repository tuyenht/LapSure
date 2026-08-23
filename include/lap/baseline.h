#pragma once
#include "model.h"
#include <atomic>
#include <string>
namespace lap {
CpuBenchmarkResult RunCpuMicroBenchmark(const std::wstring& cpuName,const std::wstring& appDir,const std::atomic_bool* cancel);
}
