void AuditWorkerCore(HWND hwnd) {
    gCancel = false;
    gAuditCurrentStage = 1;
    gAuditCompletedItems = 0;
    gSessionLifecycleState = CanonicalUiState::Running;
    ClearPublicationState();

    auto syncToGlobal = [&](const AuditReport& current) {
        std::lock_guard<std::mutex> lock(gReportMutex);
        gReport = current;
    };
    auto checkPause = [&]() {
        while (gPaused && !gCancel) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    };

    checkPause();
    PostStatus(hwnd, L"Đang nhận diện hệ điều hành và cấu hình BIOS...");
    const auto caps = DetectCapabilities(gDir);
    gReportOutputDir = EnsureReportOutputRoot();
    const auto model = Reg(L"SystemProductName");
    const auto tag = ServiceTag(caps, &gCancel);
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

    auto report = CollectInventory(profile, caps, gDir, &gCancel);
    report.profileSource = profileLoad.source;
    report.factoryExact = profileLoad.exact;
    report.genericMode = !profileLoad.exact;
    syncToGlobal(report);
    gAuditCompletedItems = 1;
    PostStatus(hwnd, report.model.empty() ? L"Đã thu thập nhận diện; model hệ thống chưa xác định."
                                          : L"Đã nhận diện hệ thống: " + report.model);

    checkPause();
    if (!gCancel) {
        gAuditCurrentStage = 2;
        PostStatus(hwnd, L"Đang thu thập danh tính và thông tin CPU...");
        syncToGlobal(report);
        gAuditCompletedItems = 2;
        PostStatus(hwnd, report.hardware.cpuName.empty() ? L"Đã thu thập CPU; chưa xác định được tên bộ xử lý."
                                                          : L"Đã nhận diện CPU: " + report.hardware.cpuName);
    }

    checkPause();
    if (!gCancel) {
        gAuditCurrentStage = 3;
        PostStatus(hwnd, L"Đang đọc chi tiết các thanh nhớ RAM...");
        syncToGlobal(report);
        gAuditCompletedItems = 3;
        const auto ram = report.hardware.installedRamBytes > 0
            ? std::to_wstring(report.hardware.installedRamBytes / (1024ull * 1024ull * 1024ull)) + L" GB"
            : L"—";
        PostStatus(hwnd, L"Đã nhận diện RAM: " + ram);
    }

    checkPause();
    if (!gCancel) {
        gAuditCurrentStage = 4;
        PostStatus(hwnd, L"Đang đọc dữ liệu SMART & NVMe cho từng ổ đĩa...");
        CollectWindowsStorageReliability(report, caps, &gCancel);
        CollectSmartctl(report, profile, caps, gDir, &gCancel);
        CollectVolumeIntegrityAudit(report);
        syncToGlobal(report);
        gAuditCompletedItems = 4;
        PostStatus(hwnd, L"Đã quét xong ổ đĩa lưu trữ (" + std::to_wstring(report.hardware.storage.size()) + L" ổ)");
    }

    checkPause();
    if (!gCancel) {
        gAuditCurrentStage = 5;
        PostStatus(hwnd, L"Đang đọc thông tin GPU và driver đồ họa...");
        CollectNvidia(report, profile, caps, gDir, &gCancel);
        syncToGlobal(report);
        gAuditCompletedItems = 5;
        PostStatus(hwnd, L"Đã quét xong GPU (" + std::to_wstring(report.hardware.gpus.size()) + L" GPU)");
    }

    checkPause();
    if (!gCancel) {
        gAuditCurrentStage = 6;
        PostStatus(hwnd, L"Đang đo thông số pin, nguồn sạc và công suất xả...");
        CollectBatteryDischargeAudit(report, caps, &gCancel);
        CollectPortPowerBaseline(report);
        syncToGlobal(report);
        gAuditCompletedItems = 6;
        PostStatus(hwnd, L"Hoàn tất thu thập Pin & Nguồn.");
    }

    checkPause();
    if (!gCancel) {
        gAuditCurrentStage = 7;
        PostStatus(hwnd, L"Đang nhận diện Wi-Fi, Bluetooth, Ethernet và controller kết nối...");
        CollectFunctionalPresence(report, caps, &gCancel);
        report.hardware.stress.chassisProfile = LoadChassisProfile(gDir, report.model);
        report.hardware.stress.portAttestation = InitializeSessionPortAttestation(
            report.hardware.stress.sessionId,
            report.hardware.stress.chassisProfile);
        syncToGlobal(report);
        gAuditCompletedItems = 7;
        PostStatus(hwnd, L"Đã thu thập hiện diện thiết bị; chức năng thực tế vẫn cần stimulus/xác nhận riêng.");
    }

    checkPause();
    if (!gCancel) {
        gAuditCurrentStage = 8;
        PostStatus(hwnd, L"Đang quét nhật ký hệ thống và sự kiện WHEA...");
        CollectPlatformForensics(report, profile, caps, gDir, &gCancel);
        syncToGlobal(report);
        gAuditCompletedItems = 8;
        PostStatus(hwnd, L"Hoàn tất thu thập nhật ký & forensics.");
    }

    checkPause();
    if (!gCancel) {
        gAuditCurrentStage = 9;
        PostStatus(hwnd, L"Chạy kiểm tra độ ổn định Stress (" + gSelectedMode + L")...");
        RunStressSession(report, caps, gDir, MakeStressPlan(gSelectedMode), &gCancel);
        if (report.hardware.stress.portAttestation.sessionId.empty()) {
            report.hardware.stress.portAttestation.sessionId = report.hardware.stress.sessionId;
        }
        RunRuntimeValidation(report, caps, gDir);
        report.hardware.stress.decision = BuildAuditDecision(report);
        BuildOrchestrator(report, false, true);
        syncToGlobal(report);
        gAuditCompletedItems = 9;
        PostStatus(hwnd, L"Hoàn tất kiểm tra độ ổn định Stress (" + gSelectedMode + L").");
    }

    ReportPublicationResult publication;
    if (!gCancel && report.hardware.stress.completed) {
        // Stress/recovery truth is independent from report filesystem publication.
        // A completed stress session must not look interrupted after a report I/O failure.
        CompleteStressJournal(gReportOutputDir);
        publication = PublishReportSnapshot(report);
        SetPublicationState(publication);
        if (publication.Published()) {
            PostStatus(hwnd, L"Đã công bố cặp báo cáo HTML/JSON và cập nhật lịch sử phiên.");
        } else {
            PostStatus(hwnd, L"Kiểm định đã hoàn tất nhưng báo cáo chưa được công bố: " + publication.detail);
        }
    } else if (!report.hardware.stress.sessionId.empty()) {
        DiscardInterruptedStressJournal(gReportOutputDir);
        ClearPublicationState();
    }

    const auto lifecycle = gCancel ? CanonicalUiState::Cancelled : LifecycleStateFromDecision(report);
    {
        std::lock_guard<std::mutex> lock(gReportMutex);
        gReport = std::move(report);
    }
    gAuditReady = !gCancel;
    gRunning = false;
    gSessionLifecycleState = lifecycle;
    PostMessageW(hwnd, WM_AUDIT_DONE, gCancel ? 1 : 0, 0);
}

void AuditWorker(HWND hwnd) {
    try {
        AuditWorkerCore(hwnd);
    } catch (...) {
        gAuditReady = false;
        gRunning = false;
        gPublicationReady = false;
        gSessionLifecycleState = CanonicalUiState::Interrupted;
        PostStatus(hwnd, L"Quá trình kiểm tra bị gián đoạn; chưa đủ dữ liệu để kết luận.");
        PostMessageW(hwnd, WM_AUDIT_DONE, 2, 0);
    }
}

void StartAudit(HWND hwnd) {
    if (gRunning) {
        gCancel = true;
        gPaused = false;
        gSessionLifecycleState = CanonicalUiState::Cancelled;
        PostStatus(hwnd, L"Đang dừng kiểm tra...");
        return;
    }
    if (gWorker.joinable()) gWorker.join();

    ClearPublicationState();
    gRunning = true;
    gCancel = false;
    gPaused = false;
    gAuditReady = false;
    gSessionLifecycleState = CanonicalUiState::Running;
    gAuditStartTime = std::chrono::steady_clock::now();
    gAuditCurrentStage = 1;
    gAuditCompletedItems = 0;
    PostStatus(hwnd, L"BẮT ĐẦU KIỂM TRA TOÀ DIỆN LAPTOP...");
    gCurrentTab = MainTab::AutoAudit;
    gWorker = std::thread(AuditWorker, hwnd);
}

void ShowAboutDialog(HWND hwnd) {
    MessageBoxW(hwnd,
        L"LapSure — kiểm định & chẩn đoán laptop dựa trên bằng chứng\n"
        L"Phiên bản: v0.1.1-beta (Native C++20 / Win32 x64)\n\n"
        L"LapSure phân biệt bằng chứng phần cứng, độ phủ kiểm tra và trạng thái xuất báo cáo. "
        L"Bằng chứng thiếu hoặc chưa xác minh không được tự động chuyển thành PASS/BUY.\n\n"
        L"Dự án: github.com/tuyenht/LapSure",
        L"Giới thiệu — LapSure v0.1.1-beta",
        MB_OK | MB_ICONINFORMATION);
}
