#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace lap {

enum class State { Pass, Good, Warning, Fail, Changed, NotTested, Unsupported, Info };
enum class Severity { Info, Minor, Major, Critical };
enum class Dimension { Identity, Factory, Health, Usage, Performance, Stability, Functional, Evidence };

struct Finding {
    std::wstring group;
    std::wstring name;
    std::wstring value;
    std::wstring expected;
    State state{State::Info};
    Severity severity{Severity::Info};
    std::wstring evidence;
    Dimension dimension{Dimension::Evidence};
};

struct FactoryProfile {
    std::wstring model;
    std::wstring serviceTag;
    std::wstring cpuContains;
    uint64_t ramBytes{};
    unsigned ramSpeed{};
    std::wstring gpuContains;
    uint64_t gpuVramBytes{};
    uint64_t diskMinBytes{};
    unsigned displayWidth{};
    unsigned displayHeight{};
    bool touchRequired{};
    double batteryDesignWh{};
    unsigned adapterW{};
};

struct MemoryModule {
    uint64_t capacityBytes{};
    unsigned configuredSpeed{};
    unsigned ratedSpeed{};
    std::wstring manufacturer;
    std::wstring partNumber;
    std::wstring serialNumber;
    std::wstring deviceLocator;
    std::wstring bankLabel;
};

struct BatteryInfo {
    bool present{false};
    bool capacityReadable{false};
    double designWh{};
    double fullChargeWh{};
    double healthPercent{-1};
    double wearPercent{-1};
    long long cycleCount{-1};
    int currentChargePercent{-1};
    std::wstring manufacturer;
    std::wstring serialNumber;
    std::wstring status;
};

struct StorageDevice {
    std::wstring devicePath;
    std::wstring model;
    std::wstring serialNumber;
    std::wstring firmware;
    std::wstring interfaceType;
    uint64_t capacityBytes{};
    bool smartReadable{false};
    bool smartPassed{false};
    bool reliabilityReadable{false};
    bool reliabilityHealthy{false};
    std::wstring reliabilityProvider;
    std::wstring healthStatus;
    std::wstring operationalStatus;
    long long criticalWarning{-1};
    long long percentageUsed{-1};
    long long enduranceRemaining{-1};
    long long availableSpare{-1};
    long long spareThreshold{-1};
    long long mediaErrors{-1};
    long long readErrorsUncorrected{-1};
    long long writeErrorsUncorrected{-1};
    long long errorLogEntries{-1};
    long long unsafeShutdowns{-1};
    long long powerOnHours{-1};
    long long powerCycles{-1};
    long long temperatureC{-1};
    long long dataUnitsRead{-1};
    long long dataUnitsWritten{-1};
    double approxDataReadTB{-1};
    double approxDataWrittenTB{-1};
};

struct GpuInfo {
    std::wstring name;
    std::wstring serialNumber;
    std::wstring uuid;
    std::wstring vbios;
    std::wstring driver;
    uint64_t vramBytes{};
    double temperatureC{-1};
    double tempLimitC{-1};
    double powerW{-1};
    double powerLimitW{-1};
    double gpuUtilPercent{-1};
    double memoryUtilPercent{-1};
    std::wstring pstate;
};

struct DisplayInfo {
    std::wstring manufacturer; std::wstring friendlyName; std::wstring serialNumber; std::wstring instanceName;
    unsigned currentWidth{}; unsigned currentHeight{}; unsigned nativeWidth{}; unsigned nativeHeight{}; unsigned refreshHz{};
    bool touchDetected{false}; bool internalPanel{false};
    std::wstring edidHex;
};
struct MainboardInfo { std::wstring manufacturer; std::wstring product; std::wstring serialNumber; };
struct BiosInfo { std::wstring vendor; std::wstring version; std::wstring releaseDate; std::wstring smbiosVersion; };
struct SecurityInfo { bool tpmPresent{false}; bool tpmReady{false}; bool secureBootKnown{false}; bool secureBootEnabled{false}; };
struct CpuTelemetry { double loadPercent{-1}; unsigned currentClockMHz{}; unsigned maxClockMHz{}; };
struct EventForensicSummary { long long whea{}; long long disk{}; long long stornvme{}; long long ntfs{}; long long display{}; long long kernelPower{}; long long bugCheck{}; bool querySucceeded{false}; };

enum class TestVerdict { Pass, Warning, Fail, NotTested, Cancelled };
enum class Confidence { Low, Medium, High };

struct TelemetrySummary {
    double avgCpuUtil{-1};
    double maxCpuUtil{-1};
    double avgGpuTempC{-1};
    double maxGpuTempC{-1};
    double avgGpuPowerW{-1};
    double maxGpuPowerW{-1};
    double avgGpuUtil{-1};
    double maxGpuUtil{-1};
    double avgCpuPackageTempC{-1};
    double maxCpuPackageTempC{-1};
    double avgCpuPackagePowerW{-1};
    double maxCpuPackagePowerW{-1};
    bool cpuThrottleObserved{false};
    Confidence cpuThermalConfidence{Confidence::Low};
    unsigned sampleCount{};
};

struct AuditDecision {
    std::wstring overall{L"INCOMPLETE"};
    std::wstring stability{L"NOT TESTED"};
    std::wstring thermal{L"UNKNOWN"};
    std::wstring performance{L"NOT SCORED"};
    std::wstring factory{L"UNKNOWN"};
    std::wstring coverage{L"PARTIAL"};
    unsigned criticalFails{};
    unsigned criticalNotTested{};
    unsigned warnings{};
    Confidence confidence{Confidence::Low};
    std::vector<std::wstring> reasons;
};

struct CoverageDomain {
    std::wstring id;
    std::wstring name;
    std::wstring status{L"NOT TESTED"};
    bool required{true};
    std::wstring sources;
    std::wstring missingEvidence;
};

struct SellerClaim {
    bool provided{false};
    std::wstring model;
    std::wstring cpuContains;
    uint64_t ramBytes{};
    std::wstring gpuContains;
    uint64_t storageBytes{};
    unsigned displayWidth{};
    unsigned displayHeight{};
    long long askingPriceVnd{};
    unsigned warrantyDays{};
};

struct SensorValue {
    std::wstring name;
    double value{-1};
    std::wstring unit;
    std::wstring source;
    Confidence confidence{Confidence::Low};
    bool valid{false};
};

struct CpuBenchmarkResult {
    std::wstring benchmarkName{L"BuiltIn-FP-Mix-v1"};
    double score{-1};
    double expectedLow{-1};
    double expectedHigh{-1};
    double percentOfBaseline{-1};
    std::wstring baselineSource;
    Confidence confidence{Confidence::Low};
    std::wstring verdict{L"NOT SCORED"};
};

struct TelemetrySample {
    unsigned second{};
    double cpuUtilPercent{-1};
    double gpuTempC{-1};
    double gpuPowerW{-1};
    double gpuUtilPercent{-1};
    double gpuMemoryUtilPercent{-1};
    double cpuPackageTempC{-1};
    double cpuPackagePowerW{-1};
    double cpuPackageClockMHz{-1};
    int cpuThermalThrottle{-1};
    Confidence cpuThermalConfidence{Confidence::Low};
    std::wstring cpuThermalSource;
};

struct RamOnlineMetrics { uint64_t bytesAllocated{}; uint64_t bytesTested{}; unsigned passes{}; uint64_t mismatches{}; };
struct GpuVramMetrics { uint64_t errors{}; double writtenGB{}; double checkedGB{}; double throughputGBs{}; bool standardFiveMinutePassed{false}; };

struct StressStageResult {
    std::wstring name;
    TestVerdict verdict{TestVerdict::NotTested};
    unsigned plannedSeconds{};
    unsigned elapsedSeconds{};
    long long newWhea{};
    long long newDisk{};
    long long newNvme{};
    long long newDisplay{};
    long long newBugCheck{};
    bool timedOut{false};
    bool cancelled{false};
    std::wstring evidence;
    std::vector<TelemetrySample> telemetry;
    TelemetrySummary telemetrySummary;
    RamOnlineMetrics ram;
    GpuVramMetrics gpuVram;
};

enum class FunctionalStatus { Pass, Warning, Fail, NotTested, Unsupported, ManualRequired };
struct FunctionalItemResult {
    std::wstring id;
    std::wstring name;
    FunctionalStatus status{FunctionalStatus::NotTested};
    std::wstring detail;
    std::wstring evidence;
    Confidence confidence{Confidence::Low};
    bool automated{false};
};
struct FunctionalTestSummary {
    std::vector<FunctionalItemResult> items;
    unsigned passed{};
    unsigned failed{};
    unsigned warning{};
    unsigned notTested{};
    unsigned manualRequired{};
    std::wstring overall{L"INCOMPLETE"};
};

struct PortProbeResult {
    std::wstring portLabel;
    std::wstring connectorHint;
    std::wstring deviceDescription;
    std::wstring instanceId;
    std::wstring locationPath;
    std::wstring busReportedDescription;
    std::wstring negotiatedSpeed;
    bool usb4RouterSeen{false};
    bool thunderboltSeen{false};
    bool displayAdapterChange{false};
    bool deviceEnumerated{false};
    Confidence confidence{Confidence::Low};
    std::wstring verdict{L"NOT TESTED"};
    std::wstring evidence;
};
struct PowerProbeResult {
    bool acConnected{false};
    int batteryPercent{-1};
    std::wstring adapterIdentity;
    double adapterWatts{-1};
    Confidence wattageConfidence{Confidence::Low};
    std::wstring verdict{L"PARTIAL"};
    std::wstring evidence;
};
struct PortPowerSummary {
    std::vector<PortProbeResult> ports;
    PowerProbeResult power;
    bool usb4HostRouterPresent{false};
    unsigned usb4DeviceRouters{};
    unsigned thunderboltDevices{};
    std::wstring overall{L"INCOMPLETE"};
};

enum class TestStageState { Locked, Ready, Running, Passed, Warning, Failed, Incomplete };
struct TestStageView {
    std::wstring id,title,subtitle;
    TestStageState state{TestStageState::Locked};
    unsigned completed{},total{};
    bool requiresOperator{false};
    std::wstring actionLabel;
};
struct OrchestratorSummary {
    std::vector<TestStageView> stages;
    unsigned completedStages{},totalStages{},percent{};
    std::wstring nextAction;
    std::wstring overall{L"NOT STARTED"};
};

struct ChassisPortDefinition {
    std::wstring id,label,side,connector,capability;
    bool required{true};
    bool tested{false};
    std::wstring verdict{L"NOT TESTED"};
};
struct ChassisProfile {
    std::wstring profileId,modelContains,displayName;
    std::wstring validationStatus{L"draft"};
    std::wstring reference;
    std::vector<ChassisPortDefinition> ports;
    Confidence confidence{Confidence::Low};
    std::wstring source;
};

enum class ValidationStatus { Pass, Warning, Fail, NotRun };
struct ValidationCheck {
    std::wstring id;
    std::wstring name;
    ValidationStatus status{ValidationStatus::NotRun};
    std::wstring detail;
    std::wstring evidence;
};
struct RuntimeValidationSummary {
    std::vector<ValidationCheck> checks;
    unsigned passed{};
    unsigned warning{};
    unsigned failed{};
    unsigned notRun{};
    std::wstring overall{L"NOT RUN"};
    std::wstring buildLabel;
    std::wstring compilerLabel;
    std::wstring architecture;
};

struct StressSession {
    std::wstring mode{L"Quick"};
    bool completed{false};
    long long wheaBefore{};
    long long diskBefore{};
    long long nvmeBefore{};
    long long displayBefore{};
    long long bugCheckBefore{};
    long long wheaAfter{};
    long long diskAfter{};
    long long nvmeAfter{};
    long long displayAfter{};
    long long bugCheckAfter{};
    std::vector<StressStageResult> stages;
    bool previousInterruptedSessionDetected{false};
    std::wstring journalPath;
    AuditDecision decision;
    CpuBenchmarkResult cpuBenchmark;
    FunctionalTestSummary functional;
    PortPowerSummary portPower;
    OrchestratorSummary orchestrator;
    ChassisProfile chassisProfile;
    RuntimeValidationSummary runtimeValidation;
};

struct PnpProblemDevice {
    std::wstring friendlyName;
    std::wstring deviceDesc;
    std::wstring instanceId;
    std::wstring classGuid;
    unsigned long problemCode{};
    std::wstring problemDescription;
};

struct HardwareSnapshot {
    std::vector<PnpProblemDevice> pnpProblems;
    std::wstring cpuName;
    unsigned cpuCores{};
    unsigned cpuThreads{};
    uint64_t installedRamBytes{};
    std::vector<MemoryModule> memoryModules;
    std::vector<StorageDevice> storage;
    std::vector<GpuInfo> gpus;
    BatteryInfo battery;
    std::vector<DisplayInfo> displays;
    MainboardInfo mainboard;
    BiosInfo bios;
    SecurityInfo security;
    CpuTelemetry cpuTelemetry;
    EventForensicSummary events;
    StressSession stress;
};

struct AuditReport {
    std::wstring model;
    std::wstring serviceTag;
    std::wstring environment;
    std::wstring profileSource;
    bool factoryExact{false};
    bool genericMode{false};
    SellerClaim sellerClaim;
    HardwareSnapshot hardware;
    std::vector<Finding> findings;
};

const wchar_t* ToString(State s);
const wchar_t* ToString(Severity s);
const wchar_t* ToString(Dimension d);

} // namespace lap
