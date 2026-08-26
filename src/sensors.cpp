#include "lap/sensors.h"
#include "lap/trust.h"
#include "lap/provider_output.h"
#include "lap/hardware.h"

namespace lap {
namespace {
SensorValue V(const std::wstring& name, double value, const std::wstring& unit,
              const std::wstring& source, Confidence c) {
    SensorValue s{};
    s.name = name;
    s.value = value;
    s.unit = unit;
    s.source = source;
    s.confidence = c;
    s.valid = value >= 0;
    return s;
}
}

SensorProviderResult ReadCpuSensors(const Capabilities&, const std::wstring& appDir,
                                    const std::atomic_bool* cancel) {
    SensorProviderResult r{};
    const std::wstring rel = L"tools\\sensors\\lhm_bridge.exe";
    auto run = RunTrustedEngineCapture(appDir, rel, L"lhm_bridge",
                                       {L"--once", L"--format", L"pipe"}, 5000, cancel);
    const auto& trust = run.trust;
    const auto& p = run.process;
    if (!trust.hashMatches) {
        r.evidence = L"No trusted CPU package sensor provider. " + trust.reason;
        return r;
    }
    if (!p.launched || p.timedOut || p.output.empty()) {
        r.evidence = L"Trusted sensor bridge failed to return data. " + p.error;
        return r;
    }
    auto bridgeVal = ValidateSensorBridgeOutput(p.output);
    if (!bridgeVal.valid) {
        r.evidence = L"Sensor bridge output contract mismatch: " + bridgeVal.reason;
        return r;
    }
    auto lines = SplitLines(p.output);
    if (lines.empty()) return r;
    auto q = Split(lines.front(), L'|');
    if (q.size() < 4) {
        r.evidence = L"Sensor bridge output contract mismatch.";
        return r;
    }
    auto temp = ParseDouble(q[0], -1), power = ParseDouble(q[1], -1),
         clock = ParseDouble(q[2], -1), throttle = ParseDouble(q[3], -1);
    r.available = temp >= 0 || power >= 0 || clock >= 0 || throttle >= 0;
    r.providerName = L"LibreHardwareMonitor bridge";
    r.confidence = Confidence::High;
    r.cpuPackageTemp = V(L"CPU Package Temperature", temp, L"C", r.providerName, Confidence::High);
    r.cpuPackagePower = V(L"CPU Package Power", power, L"W", r.providerName, Confidence::High);
    r.cpuPackageClock = V(L"CPU Package Clock", clock, L"MHz", r.providerName, Confidence::Medium);
    r.cpuThermalThrottle = V(L"CPU Thermal Throttle", throttle, L"bool", r.providerName, Confidence::Medium);
    r.evidence = L"Trusted provider SHA256=" + trust.sha256;
    return r;
}
}
