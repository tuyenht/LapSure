#include "lap/telemetry.h"
#include "lap/hardware.h"
#include "lap/sensors.h"
#include "lap/trust.h"
#include <windows.h>
#include <mutex>

namespace lap {
namespace {
unsigned long long FT(const FILETIME& f) {
    ULARGE_INTEGER u{};
    u.LowPart = f.dwLowDateTime;
    u.HighPart = f.dwHighDateTime;
    return u.QuadPart;
}
}

double ReadSystemCpuUtilizationPercent() {
    static std::mutex mx;
    static bool init = false;
    static unsigned long long pi = 0, pk = 0, pu = 0;
    std::lock_guard<std::mutex> g(mx);
    FILETIME i{}, k{}, u{};
    if (!GetSystemTimes(&i, &k, &u)) return -1;
    auto ni = FT(i), nk = FT(k), nu = FT(u);
    if (!init) {
        pi = ni; pk = nk; pu = nu; init = true;
        return -1;
    }
    auto di = ni - pi, dk = nk - pk, du = nu - pu;
    pi = ni; pk = nk; pu = nu;
    auto total = dk + du;
    if (!total) return -1;
    double busy = 100.0 * static_cast<double>(total - di) / static_cast<double>(total);
    if (busy < 0) busy = 0;
    if (busy > 100) busy = 100;
    return busy;
}

TelemetrySample SampleTelemetry(unsigned second, const Capabilities& caps,
                                const std::wstring& appDir, const std::atomic_bool* cancel) {
    TelemetrySample s{};
    s.second = second;
    s.cpuUtilPercent = ReadSystemCpuUtilizationPercent();
    if (caps.nvidiaSmi) {
        auto run = RunTrustedEngineCapture(
            appDir, L"tools\\nvidia-smi.exe", L"nvidia_smi",
            {L"--query-gpu=temperature.gpu,power.draw,utilization.gpu,utilization.memory",
             L"--format=csv,noheader,nounits"},
            5000, cancel);
        const auto& p = run.process;
        if (run.trust.hashMatches && p.launched && !p.timedOut && !p.output.empty()) {
            auto lines = SplitLines(p.output);
            if (!lines.empty()) {
                auto q = Split(lines.front(), L',');
                if (q.size() >= 4) {
                    s.gpuTempC = ParseDouble(q[0]);
                    s.gpuPowerW = ParseDouble(q[1]);
                    s.gpuUtilPercent = ParseDouble(q[2]);
                    s.gpuMemoryUtilPercent = ParseDouble(q[3]);
                }
            }
        }
    }
    auto cpu = ReadCpuSensors(caps, appDir, cancel);
    if (cpu.available) {
        if (cpu.cpuPackageTemp.valid) s.cpuPackageTempC = cpu.cpuPackageTemp.value;
        if (cpu.cpuPackagePower.valid) s.cpuPackagePowerW = cpu.cpuPackagePower.value;
        if (cpu.cpuPackageClock.valid) s.cpuPackageClockMHz = cpu.cpuPackageClock.value;
        if (cpu.cpuThermalThrottle.valid) s.cpuThermalThrottle = static_cast<int>(cpu.cpuThermalThrottle.value);
        s.cpuThermalConfidence = cpu.confidence;
        s.cpuThermalSource = cpu.providerName;
    }
    return s;
}
}
