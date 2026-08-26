constexpr UINT WM_AUDIT_DONE = WM_APP + 1;
constexpr UINT WM_AUDIT_STATUS = WM_APP + 2;

AuditReport gReport;
std::wstring gDir;
std::wstring gReportPath;
std::wstring gReportOutputDir;

HWND gMainHwnd = nullptr;
HWND gList = nullptr;
HWND gStatus = nullptr;
HWND gBtn = nullptr;
HWND gOpen = nullptr;
HWND gMode = nullptr;
HWND gFuncDisplay = nullptr;
HWND gFuncKeyboard = nullptr;
HWND gFuncTouch = nullptr;
HWND gFuncSpeaker = nullptr;
HWND gFuncUsb = nullptr;
HWND gFuncIo = nullptr;
HWND gPhysical = nullptr;
HWND gSeller = nullptr;
HWND gPortTest = nullptr;
HWND gNext = nullptr;
HWND gProgress = nullptr;

std::thread gWorker;
std::atomic_bool gCancel{false};
std::atomic_bool gRunning{false};
std::atomic_bool gPaused{false};
std::atomic_bool gAuditReady{false};
std::atomic_bool gPublicationReady{false};
std::atomic_bool gCloseRequested{false};

CanonicalUiState gSessionLifecycleState{CanonicalUiState::Idle};
std::wstring gSelectedMode = L"Standard";
std::mutex gReportMutex;
std::mutex gLogsMutex;
std::mutex gRootMutex;
std::mutex gPublicationMutex;
std::mutex gPublicationStateMutex;
std::vector<LiveLogEntry> gLiveLogs;
std::chrono::steady_clock::time_point gAuditStartTime;
int gAuditElapsedSec = 0;
constexpr int gAuditTotalItems = 9;
std::atomic<int> gAuditCompletedItems{0};
std::atomic<int> gAuditCurrentStage{0};

MainTab gCurrentTab = MainTab::Dashboard;
bool gDeviceGroupExpanded = true;
int gSidebarScrollOffset = 0;
int gTableScrollOffset = 0;
int gFocusIndex = 0; // 0: Sidebar, 1: Mode pills, 2: S01 CTA, 3: screen primary CTA.
int gHistorySelectedIndex = 0;
int gInspectionPurpose = 0;
UiFonts gFonts;

std::wstring AppDir() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return std::filesystem::path(path).parent_path().wstring();
}

CanonicalUiState LifecycleStateFromDecision(const AuditReport& report) {
    const auto& overall = report.hardware.stress.decision.overall;
    if (overall == L"BUY") return CanonicalUiState::Pass;
    if (overall == L"BUY WITH NOTES") return CanonicalUiState::Warning;
    if (overall == L"REJECT") return CanonicalUiState::Fail;
    return CanonicalUiState::Incomplete;
}

std::wstring Reg(const wchar_t* name) {
    HKEY key{};
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &key) != ERROR_SUCCESS) return {};
    wchar_t buffer[512]{};
    DWORD size = sizeof(buffer), type = 0;
    std::wstring value;
    if (RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &size) == ERROR_SUCCESS) value = buffer;
    RegCloseKey(key);
    return value;
}

std::wstring ServiceTag(const Capabilities& caps, const std::atomic_bool* cancel) {
    auto tag = Reg(L"SystemSerialNumber");
    if (tag.empty() && caps.powershell) {
        const auto process = RunProcessCapture(
            L"powershell.exe -NoProfile -NonInteractive -Command \"(Get-CimInstance Win32_BIOS|Select-Object -First 1).SerialNumber\"",
            10000, cancel);
        const auto lines = SplitLines(process.output);
        if (process.launched && !process.timedOut && !process.cancelled && !lines.empty()) tag = lines.front();
    }
    return tag;
}

void AddLiveLog(const std::wstring& message, const std::wstring& source = L"LapSure Engine", int state = 3) {
    const auto now = std::chrono::system_clock::now();
    const auto raw = std::chrono::system_clock::to_time_t(now);
    tm local{};
    localtime_s(&local, &raw);
    wchar_t time[32]{};
    swprintf_s(time, L"%02d:%02d:%02d", local.tm_hour, local.tm_min, local.tm_sec);
    {
        std::lock_guard<std::mutex> lock(gLogsMutex);
        gLiveLogs.push_back({time, message, source, state});
    }
    if (gMainHwnd) InvalidateRect(gMainHwnd, nullptr, FALSE);
}

void PostStatus(HWND hwnd, const std::wstring& message) {
    auto* heap = new std::wstring(message);
    PostMessageW(hwnd, WM_AUDIT_STATUS, 0, reinterpret_cast<LPARAM>(heap));
    AddLiveLog(message);
}

std::wstring EnsureReportOutputRoot() {
    std::lock_guard<std::mutex> rootLock(gRootMutex);
    if (!gReportOutputDir.empty()) return gReportOutputDir;
    const auto caps = DetectCapabilities(gDir);
    gReportOutputDir = ResolveReportDirectory(gDir, caps.winPE);
    InitializeSessionHistory(gReportOutputDir);
    return gReportOutputDir;
}

void SetPublicationState(const ReportPublicationResult& publication) {
    std::lock_guard<std::mutex> lock(gPublicationStateMutex);
    gPublicationReady = publication.Published();
    gReportPath = publication.Published() ? publication.htmlPath : L"";
}

void ClearPublicationState() {
    std::lock_guard<std::mutex> lock(gPublicationStateMutex);
    gPublicationReady = false;
    gReportPath.clear();
}

std::wstring CurrentReportPath() {
    std::lock_guard<std::mutex> lock(gPublicationStateMutex);
    return gReportPath;
}

ReportPublicationResult PublishReportSnapshot(const AuditReport& report) {
    const auto outputRoot = EnsureReportOutputRoot();
    std::lock_guard<std::mutex> publicationLock(gPublicationMutex);
    return PublishReportBundle(report, outputRoot);
}

int RunInventoryOnly(const std::wstring& outputDir) {
    std::atomic_bool cancel{false};
    const auto caps = DetectCapabilities(gDir);
    const auto model = Reg(L"SystemProductName");
    const auto tag = ServiceTag(caps, &cancel);
    auto profileLoad = LoadFactoryProfile(gDir + L"\\profiles", model, tag);
    if (!profileLoad.loaded && !tag.empty()) {
        const auto vendor = Reg(L"SystemManufacturer");
        const auto cloud = LookupFactoryProfileOnline(gDir, vendor, model, tag, 1500);
        if (cloud.success) {
            profileLoad.loaded = true;
            profileLoad.exact = true;
            profileLoad.profile = cloud.profile;
            profileLoad.source = cloud.source;
        }
    }

    const FactoryProfile profile = profileLoad.loaded ? profileLoad.profile : FactoryProfile{};
    auto report = CollectInventory(profile, caps, gDir, &cancel);
    report.profileSource = profileLoad.source;
    report.factoryExact = profileLoad.exact;
    report.genericMode = !profileLoad.exact;
    CollectNvidia(report, profile, caps, gDir, &cancel);
    CollectWindowsStorageReliability(report, caps, &cancel);
    CollectSmartctl(report, profile, caps, gDir, &cancel);
    CollectPlatformForensics(report, profile, caps, gDir, &cancel);
    CollectFunctionalPresence(report, caps, &cancel);
    CollectPortPowerBaseline(report);
    CollectVolumeIntegrityAudit(report);
    CollectBatteryDischargeAudit(report, caps, &cancel);
    report.hardware.stress.chassisProfile = LoadChassisProfile(gDir, report.model);
    report.hardware.stress.portAttestation = InitializeSessionPortAttestation(
        report.hardware.stress.sessionId,
        report.hardware.stress.chassisProfile);
    RunRuntimeValidation(report, caps, gDir);
    report.findings.push_back({L"Validation", L"Inventory-only preflight", L"COMPLETED", L"No stress stages executed",
        State::Warning, Severity::Info, L"Explicit --inventory-only mode; verdict must remain incomplete.", Dimension::Health});
    report.hardware.stress.decision = BuildAuditDecision(report);
    BuildOrchestrator(report, false, false);

    const auto outputRoot = outputDir.empty() ? ResolveReportDirectory(gDir, caps.winPE)
                                               : std::filesystem::absolute(outputDir).wstring();
    InitializeSessionHistory(outputRoot);
    const auto publication = PublishReportBundle(report, outputRoot);
    return publication.Published() ? 0 : 2;
}

void Fill() {
    std::lock_guard<std::mutex> lock(gReportMutex);
    if (!gList) return;
    ListView_DeleteAllItems(gList);
    int row = 0;
    for (auto& finding : gReport.findings) {
        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = row;
        item.pszText = const_cast<wchar_t*>(finding.group.c_str());
        ListView_InsertItem(gList, &item);
        ListView_SetItemText(gList, row, 1, const_cast<wchar_t*>(finding.name.c_str()));
        ListView_SetItemText(gList, row, 2, const_cast<wchar_t*>(finding.value.c_str()));
        ++row;
    }
}

void SetFunctionalButtonsEnabled(BOOL enabled) {
    if (gFuncDisplay) EnableWindow(gFuncDisplay, enabled);
    if (gFuncKeyboard) EnableWindow(gFuncKeyboard, enabled);
    if (gFuncTouch) EnableWindow(gFuncTouch, enabled);
    if (gFuncSpeaker) EnableWindow(gFuncSpeaker, enabled);
    if (gFuncUsb) EnableWindow(gFuncUsb, enabled);
    if (gFuncIo) EnableWindow(gFuncIo, enabled);
    if (gPhysical) EnableWindow(gPhysical, enabled);
    if (gSeller) EnableWindow(gSeller, enabled);
    if (gPortTest) EnableWindow(gPortTest, enabled);
}

void RebuildDecisionAndReports() {
    AuditReport snapshot;
    {
        std::lock_guard<std::mutex> lock(gReportMutex);
        gReport.hardware.stress.decision = BuildAuditDecision(gReport);
        BuildOrchestrator(gReport, gRunning.load(), gAuditReady.load());
        snapshot = gReport;
    }

    const auto publication = PublishReportSnapshot(snapshot);
    SetPublicationState(publication);
    gSessionLifecycleState = LifecycleStateFromDecision(snapshot);
    if (!publication.Published()) {
        AddLiveLog(L"Kết luận phần cứng được giữ nguyên; report HTML/JSON chưa được công bố: " + publication.detail,
                   L"Report publication", 1);
    }
}

void UpsertFunctional(const FunctionalItemResult& result) {
    std::lock_guard<std::mutex> lock(gReportMutex);
    auto& items = gReport.hardware.stress.functional.items;
    for (auto& item : items) {
        if (item.id == result.id) {
            item = result;
            RecalculateFunctionalSummary(gReport.hardware.stress.functional);
            return;
        }
    }
    items.push_back(result);
    RecalculateFunctionalSummary(gReport.hardware.stress.functional);
}

bool CanRunManualTest(HWND hwnd) {
    if (gRunning) {
        MessageBoxW(hwnd, L"LapSure đang kiểm tra tự động. Vui lòng chờ hoàn tất trước khi kiểm tra tương tác.",
                    L"LapSure", MB_OK | MB_ICONINFORMATION);
        return false;
    }
    if (!gAuditReady) {
        MessageBoxW(hwnd, L"Hãy bấm “BẮT ĐẦU KIỂM TRA” trước. Kết quả tương tác sẽ được lưu chung vào phiên hiện tại.",
                    L"LapSure", MB_OK | MB_ICONINFORMATION);
        return false;
    }
    return true;
}

void CommitManualResult(const FunctionalItemResult& result) {
    UpsertFunctional(result);
    RebuildDecisionAndReports();
    Fill();
    if (gMainHwnd) InvalidateRect(gMainHwnd, nullptr, FALSE);
}

void CommitManualResults(const std::vector<FunctionalItemResult>& results) {
    for (const auto& result : results) UpsertFunctional(result);
    RebuildDecisionAndReports();
    Fill();
    if (gMainHwnd) InvalidateRect(gMainHwnd, nullptr, FALSE);
}

void CommitSellerClaim(const SellerClaim& claim) {
    {
        std::lock_guard<std::mutex> lock(gReportMutex);
        gReport.sellerClaim = claim;
        ApplySellerClaimComparison(gReport);
    }
    RebuildDecisionAndReports();
    Fill();
    if (gMainHwnd) InvalidateRect(gMainHwnd, nullptr, FALSE);
}

void UpsertPortResultUnlocked(const PortProbeResult& result) {
    auto& ports = gReport.hardware.stress.portPower.ports;
    for (auto& port : ports) {
        const bool sameExpectedId = !result.expectedPortId.empty() &&
            port.expectedPortId == result.expectedPortId;
        const bool labelFallback = result.expectedPortId.empty() &&
            port.portLabel == result.portLabel;
        if (sameExpectedId || labelFallback) {
            port = result;
            return;
        }
    }
    ports.push_back(result);
}

void CommitPortResultGuided(const PortProbeResult& result) {
    {
        std::lock_guard<std::mutex> lock(gReportMutex);
        UpsertPortResultUnlocked(result);
        ApplyPortResultToAttestation(gReport.hardware.stress.portAttestation, result);
        ApplyPortResultToChassisProfile(gReport.hardware.stress.chassisProfile, result);
        RecalculatePortPowerSummary(gReport.hardware.stress.portPower);
    }
    RebuildDecisionAndReports();
    Fill();
    if (gMainHwnd) InvalidateRect(gMainHwnd, nullptr, FALSE);
}

void CommitPortResult(const PortProbeResult& result) {
    {
        std::lock_guard<std::mutex> lock(gReportMutex);
        UpsertPortResultUnlocked(result);
        RecalculatePortPowerSummary(gReport.hardware.stress.portPower);
    }
    RebuildDecisionAndReports();
    Fill();
    if (gMainHwnd) InvalidateRect(gMainHwnd, nullptr, FALSE);
}
