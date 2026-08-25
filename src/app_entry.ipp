void InitializeFastIdentity() {
    std::lock_guard<std::mutex> lock(gReportMutex);
    if (gReport.model.empty()) {
        gReport.model = Reg(L"SystemProductName");
        if (gReport.model.empty()) gReport.model = Reg(L"BaseBoardProduct");
    }
    if (gReport.serviceTag.empty()) {
        gReport.serviceTag = Reg(L"SystemSerialNumber");
        if (gReport.serviceTag.empty()) gReport.serviceTag = Reg(L"BaseBoardSerialNumber");
    }
    if (gReport.hardware.cpuName.empty()) {
        HKEY key{};
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &key) == ERROR_SUCCESS) {
            wchar_t buffer[512]{};
            DWORD size = sizeof(buffer), type = 0;
            if (RegQueryValueExW(key, L"ProcessorNameString", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &size) == ERROR_SUCCESS) {
                gReport.hardware.cpuName = buffer;
            }
            RegCloseKey(key);
        }
    }
    if (gReport.hardware.installedRamBytes == 0) {
        MEMORYSTATUSEX memory{sizeof(memory)};
        if (GlobalMemoryStatusEx(&memory)) gReport.hardware.installedRamBytes = memory.ullTotalPhys;
    }
    if (gReport.hardware.gpus.empty()) {
        DISPLAY_DEVICEW display{sizeof(display)};
        for (DWORD index = 0; EnumDisplayDevicesW(nullptr, index, &display, 0); ++index) {
            if (!(display.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) &&
                (display.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
                GpuInfo gpu{};
                gpu.name = display.DeviceString;
                gReport.hardware.gpus.push_back(std::move(gpu));
                break;
            }
            display.cb = sizeof(display);
        }
    }
}

int RunLapSure(HINSTANCE instance) {
    gDir = AppDir();
    InitializeFastIdentity();

    int argc = 0;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool inventoryOnly = false;
    std::wstring outputDir;
    std::vector<std::wstring> preCacheTags;
    std::wstring preCacheVendor;

    for (int i = 1; i < argc; ++i) {
        const std::wstring arg = argv[i];
        if (arg == L"--inventory-only") inventoryOnly = true;
        else if (arg == L"--output" && i + 1 < argc) outputDir = argv[++i];
        else if ((arg == L"--cache-tag" || arg == L"--pre-cache") && i + 1 < argc) {
            std::wstringstream stream(argv[++i]);
            std::wstring item;
            while (std::getline(stream, item, L',')) {
                while (!item.empty() && iswspace(item.front())) item.erase(item.begin());
                while (!item.empty() && iswspace(item.back())) item.pop_back();
                if (!item.empty()) preCacheTags.push_back(item);
            }
        } else if (arg == L"--vendor" && i + 1 < argc) {
            preCacheVendor = argv[++i];
        }
    }

    if (!preCacheTags.empty()) {
        AttachConsole(ATTACH_PARENT_PROCESS);
        const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        auto print = [&](const std::wstring& message) {
            if (!out || out == INVALID_HANDLE_VALUE) return;
            DWORD written = 0;
            WriteConsoleW(out, message.c_str(), static_cast<DWORD>(message.size()), &written, nullptr);
            WriteConsoleW(out, L"\r\n", 2, &written, nullptr);
        };
        print(L"[LapSure CLI Pre-Cache] Bắt đầu chuẩn bị hồ sơ OEM cho " + std::to_wstring(preCacheTags.size()) + L" máy...");
        const auto summary = RunBatchPreCache(gDir, preCacheTags, preCacheVendor);
        for (const auto& detail : summary.details) print(L"  • Tag '" + detail.first + L"': " + detail.second);
        print(L"Kết quả Pre-Cache: Thành công " + std::to_wstring(summary.succeeded) + L"/" + std::to_wstring(summary.total) +
              L" (Cache: " + std::to_wstring(summary.fromCache) + L", Lỗi: " + std::to_wstring(summary.failed) + L")");
        if (argv) LocalFree(argv);
        return (summary.succeeded > 0 || summary.failed == 0) ? 0 : 1;
    }

    if (argv) LocalFree(argv);
    if (inventoryOnly) {
        try {
            return RunInventoryOnly(outputDir);
        } catch (...) {
            return 3;
        }
    }

    gReportOutputDir = EnsureReportOutputRoot();
    const auto interrupted = ReadInterruptedStressJournal(gReportOutputDir);
    if (interrupted.present) {
        std::lock_guard<std::mutex> lock(gReportMutex);
        gReport.hardware.stress.previousInterruptedSessionDetected = true;
        gReport.hardware.stress.journalPath = interrupted.journalPath;
        gReport.findings.push_back({L"Stability", L"Previous interrupted stress session", interrupted.rawEvidence,
            L"No abandoned RUNNING journal", State::Warning, Severity::Critical,
            L"Crash/reboot/interruption evidence; not proof of hardware failure.", Dimension::Health});
        gSessionLifecycleState = CanonicalUiState::Interrupted;
        gCurrentTab = MainTab::InterruptedRecovery;
    }

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_LISTVIEW_CLASSES};
    InitCommonControlsEx(&controls);

    // Trust stores are never modified at runtime. Release signing/trust is an installer/release responsibility.
    WNDCLASSEXW windowClass{sizeof(windowClass)};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
    windowClass.hIconSm = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = L"LapSure";
    if (!RegisterClassExW(&windowClass)) return 4;

    const HWND window = CreateWindowExW(0, windowClass.lpszClassName, L"LapSure — Kiểm tra laptop toàn diện",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 820,
        nullptr, nullptr, instance, nullptr);
    if (!window) return 5;

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
