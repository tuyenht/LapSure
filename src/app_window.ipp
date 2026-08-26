std::vector<MainTab> GetVisualTabList() {
    std::vector<MainTab> tabs;
    for (const auto& group : GetDefaultSidebarGroups(gDeviceGroupExpanded)) {
        if (!group.isExpanded) continue;
        for (const auto& item : group.items) tabs.push_back(item.tab);
    }
    return tabs;
}

bool HasRamStressEvidence() {
    std::lock_guard<std::mutex> lock(gReportMutex);
    for (const auto& stage : gReport.hardware.stress.stages) {
        if (stage.ram.bytesAllocated || stage.ram.bytesTested || stage.ram.mismatches || stage.ram.passes) return true;
    }
    return false;
}

void ActivateMemoryPrimaryAction(HWND hwnd) {
    if (HasRamStressEvidence()) {
        gCurrentTab = MainTab::Stress;
        gFocusIndex = 3;
        InvalidateRect(hwnd, nullptr, FALSE);
        return;
    }
    StartAudit(hwnd);
}

void OpenCurrentReport(HWND hwnd) {
    const auto path = CurrentReportPath();
    if (path.empty()) {
        MessageBoxW(hwnd, L"Phiên hiện tại chưa có HTML report đã công bố.", L"LapSure", MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (!IsTrustedSessionArtifactPath(path)) {
        MessageBoxW(hwnd, L"Đường dẫn báo cáo không vượt qua kiểm tra vùng tin cậy.", L"LapSure", MB_OK | MB_ICONERROR);
        return;
    }
    ShellExecuteW(hwnd, L"open", path.c_str(), nullptr, nullptr, SW_SHOW);
}

AuditReport ReportSnapshot() {
    std::lock_guard<std::mutex> lock(gReportMutex);
    return gReport;
}

std::vector<LiveLogEntry> LogSnapshot() {
    std::lock_guard<std::mutex> lock(gLogsMutex);
    return gLiveLogs;
}

void RenderFallbackScreen(HDC dc, const RECT& rect, const UiFonts& fonts, int dpi) {
    EmptyStateConfig empty;
    empty.state = CanonicalUiState::NotTested;
    empty.title = L"Chưa có màn hình khả dụng";
    empty.description = L"Phân hệ này chưa có renderer runtime tương ứng.";
    empty.recoveryHint = L"Quay lại Tổng quan để tiếp tục quy trình kiểm định.";
    DrawEmptyState(dc, rect, empty, fonts, dpi);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        gMainHwnd = hwnd;
        gFonts.Init(GetDpiForHwnd(hwnd));
        SetTimer(hwnd, 1, 1000, nullptr);

        // Hidden controls are command-routing/accessibility hooks only; they are never visible operation targets.
        gMode = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | CBS_DROPDOWNLIST, -100, -100, 10, 10, hwnd, reinterpret_cast<HMENU>(3), nullptr, nullptr);
        gBtn = CreateWindowW(L"BUTTON", L"BẮT ĐẦU KIỂM TRA", WS_CHILD, -100, -100, 10, 10, hwnd, reinterpret_cast<HMENU>(1), nullptr, nullptr);
        gOpen = CreateWindowW(L"BUTTON", L"XEM KẾT QUẢ", WS_CHILD, -100, -100, 10, 10, hwnd, reinterpret_cast<HMENU>(2), nullptr, nullptr);
        gFuncDisplay = CreateWindowW(L"BUTTON", L"Màn hình", WS_CHILD, -100, -100, 10, 10, hwnd, reinterpret_cast<HMENU>(1201), nullptr, nullptr);
        gFuncKeyboard = CreateWindowW(L"BUTTON", L"Bàn phím", WS_CHILD, -100, -100, 10, 10, hwnd, reinterpret_cast<HMENU>(1202), nullptr, nullptr);
        gFuncTouch = CreateWindowW(L"BUTTON", L"Cảm ứng", WS_CHILD, -100, -100, 10, 10, hwnd, reinterpret_cast<HMENU>(1203), nullptr, nullptr);
        gFuncSpeaker = CreateWindowW(L"BUTTON", L"Loa", WS_CHILD, -100, -100, 10, 10, hwnd, reinterpret_cast<HMENU>(1204), nullptr, nullptr);
        gFuncUsb = CreateWindowW(L"BUTTON", L"Cổng USB", WS_CHILD, -100, -100, 10, 10, hwnd, reinterpret_cast<HMENU>(1205), nullptr, nullptr);
        gFuncIo = CreateWindowW(L"BUTTON", L"Thiết bị tự động", WS_CHILD, -100, -100, 10, 10, hwnd, reinterpret_cast<HMENU>(1206), nullptr, nullptr);
        gPortTest = CreateWindowW(L"BUTTON", L"Kiểm tra cổng", WS_CHILD, -100, -100, 10, 10, hwnd, reinterpret_cast<HMENU>(1207), nullptr, nullptr);
        gPhysical = CreateWindowW(L"BUTTON", L"Ngoại hình", WS_CHILD, -100, -100, 10, 10, hwnd, reinterpret_cast<HMENU>(1208), nullptr, nullptr);
        gSeller = CreateWindowW(L"BUTTON", L"Cấu hình bán", WS_CHILD, -100, -100, 10, 10, hwnd, reinterpret_cast<HMENU>(1209), nullptr, nullptr);
        gNext = CreateWindowW(L"BUTTON", L"TIẾP TỤC BƯỚC KẾ", WS_CHILD, -100, -100, 10, 10, hwnd, reinterpret_cast<HMENU>(1300), nullptr, nullptr);
        gProgress = CreateWindowW(L"STATIC", L"Quy trình: chưa bắt đầu", WS_CHILD, -100, -100, 10, 10, hwnd, reinterpret_cast<HMENU>(1301), nullptr, nullptr);
        gStatus = CreateWindowW(L"STATIC", L"Sẵn sàng", WS_CHILD, -100, -100, 10, 10, hwnd, nullptr, nullptr, nullptr);
        gList = CreateWindowW(WC_LISTVIEWW, L"", WS_CHILD | LVS_REPORT | LVS_SINGLESEL, -100, -100, 10, 10, hwnd, nullptr, nullptr, nullptr);
        if (gOpen) EnableWindow(gOpen, FALSE);
        SetFunctionalButtonsEnabled(FALSE);
        if (gNext) EnableWindow(gNext, FALSE);
        return 0;
    }

    case WM_DPICHANGED: {
        const int newDpi = HIWORD(wParam);
        gFonts.Init(newDpi);
        const auto* rect = reinterpret_cast<RECT*>(lParam);
        SetWindowPos(hwnd, nullptr, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_TIMER:
        if (gRunning && !gPaused) {
            gAuditElapsedSec = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - gAuditStartTime).count());
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_KEYDOWN: {
        switch (wParam) {
        case VK_TAB:
            gFocusIndex = (GetKeyState(VK_SHIFT) & 0x8000) ? (gFocusIndex + 3) % 4 : (gFocusIndex + 1) % 4;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case VK_UP:
        case VK_DOWN:
            if (gFocusIndex == 0) {
                const auto tabs = GetVisualTabList();
                const auto it = std::find(tabs.begin(), tabs.end(), gCurrentTab);
                if (it != tabs.end()) {
                    const auto index = static_cast<size_t>(std::distance(tabs.begin(), it));
                    if (wParam == VK_UP && index > 0) gCurrentTab = tabs[index - 1];
                    if (wParam == VK_DOWN && index + 1 < tabs.size()) gCurrentTab = tabs[index + 1];
                }
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case VK_LEFT:
            if (gFocusIndex == 1 && (gCurrentTab == MainTab::Dashboard || gCurrentTab == MainTab::NewSession)) {
                if (gSelectedMode == L"Deep") gSelectedMode = L"Standard";
                else if (gSelectedMode == L"Standard") gSelectedMode = L"Quick";
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case VK_RIGHT:
            if (gFocusIndex == 1 && (gCurrentTab == MainTab::Dashboard || gCurrentTab == MainTab::NewSession)) {
                if (gSelectedMode == L"Quick") gSelectedMode = L"Standard";
                else if (gSelectedMode == L"Standard") gSelectedMode = L"Deep";
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case VK_RETURN:
        case VK_SPACE: {
            const int actionFocus = gFocusIndex;
            if (actionFocus == 2) {
                // Focus slot 2 belongs to the S01 top-level Start/Stop button only.
                if (gCurrentTab == MainTab::Dashboard) StartAudit(hwnd);
                return 0;
            }
            if (actionFocus != 3) return 0;

            // Screen-aware primary action dispatch. Keyboard activation must never
            // execute a generic action that does not match the visible screen CTA.
            switch (gCurrentTab) {
            case MainTab::Dashboard:
            case MainTab::AutoAudit:
            case MainTab::Functional:
                PostMessageW(hwnd, WM_COMMAND, 1300, 0);
                break;
            case MainTab::NewSession:
            case MainTab::Stress:
                StartAudit(hwnd);
                break;
            case MainTab::SellerClaim:
                PostMessageW(hwnd, WM_COMMAND, 1209, 0);
                break;
            case MainTab::PhysicalSafety:
                PostMessageW(hwnd, WM_COMMAND, 1208, 0);
                break;
            case MainTab::PortsPower:
                PostMessageW(hwnd, WM_COMMAND, 1300, 0);
                break;
            case MainTab::Display:
                PostMessageW(hwnd, WM_COMMAND, 1201, 0);
                break;
            case MainTab::AudioCamera:
                PostMessageW(hwnd, WM_COMMAND, 1212, 0);
                break;
            case MainTab::Network:
                PostMessageW(hwnd, WM_COMMAND, 1213, 0);
                break;
            case MainTab::Memory:
                ActivateMemoryPrimaryAction(hwnd);
                break;
            case MainTab::Reports:
                gCurrentTab = MainTab::ExportShare;
                InvalidateRect(hwnd, nullptr, FALSE);
                break;
            case MainTab::ExportShare:
                OpenCurrentReport(hwnd);
                break;
            default:
                // Screens with no single primary CTA (or multiple actions such as
                // history/recovery) require explicit pointer/focus selection.
                break;
            }
            return 0;
        }
        default:
            break;
        }
        break;
    }

    case WM_MOUSEWHEEL: {
        const short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta > 0) {
            gTableScrollOffset = std::max(0, gTableScrollOffset - 1);
            gSidebarScrollOffset = std::max(0, gSidebarScrollOffset - 30);
        } else {
            ++gTableScrollOffset;
            gSidebarScrollOffset += 30;
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        if (id == 1) {
            StartAudit(hwnd);
            return 0;
        }
        if (id == 2) {
            OpenCurrentReport(hwnd);
            return 0;
        }
        if (id == 1201) { if (CanRunManualTest(hwnd)) CommitManualResult(RunDisplayColorWizard(hwnd)); return 0; }
        if (id == 1202) { if (CanRunManualTest(hwnd)) CommitManualResult(RunKeyboardWizard(hwnd)); return 0; }
        if (id == 1203) {
            if (CanRunManualTest(hwnd)) {
                const auto caps = DetectCapabilities(gDir);
                const auto functionalCaps = DetectFunctionalCapabilities(caps, &gCancel);
                CommitManualResult(RunTouchGridWizard(hwnd, functionalCaps.touchPresent));
            }
            return 0;
        }
        if (id == 1204) {
            if (CanRunManualTest(hwnd)) {
                const auto caps = DetectCapabilities(gDir);
                const auto functionalCaps = DetectFunctionalCapabilities(caps, &gCancel);
                CommitManualResult(RunSpeakerWizard(hwnd, functionalCaps.audioPresent));
            }
            return 0;
        }
        if (id == 1205) {
            if (CanRunManualTest(hwnd)) {
                const auto caps = DetectCapabilities(gDir);
                CommitManualResult(RunUsbPortWizard(hwnd, caps, &gCancel));
            }
            return 0;
        }
        if (id == 1206) { if (CanRunManualTest(hwnd)) CommitManualResults(RunFunctionalIoWizard(hwnd)); return 0; }
        if (id == 1212) { if (CanRunManualTest(hwnd)) CommitManualResults(RunAudioCameraWizard(hwnd)); return 0; }
        if (id == 1213) { if (CanRunManualTest(hwnd)) CommitManualResults(RunNetworkConnectivityWizard(hwnd)); return 0; }
        if (id == 1207) {
            if (CanRunManualTest(hwnd)) {
                wchar_t label[64] = L"USB-C / USB-A port";
                CommitPortResult(RunPhysicalPortProbe(hwnd, label, &gCancel));
            }
            return 0;
        }
        if (id == 1208) { if (CanRunManualTest(hwnd)) CommitManualResults(RunPhysicalConditionWizard(hwnd)); return 0; }
        if (id == 1209) {
            if (CanRunManualTest(hwnd)) {
                SellerClaim claim;
                if (RunSellerClaimWizard(hwnd, claim)) CommitSellerClaim(claim);
            }
            return 0;
        }
        if (id == 1210) { ShowAboutDialog(hwnd); return 0; }
        if (id == 1300) {
            if (!CanRunManualTest(hwnd)) return 0;
            AuditReport snapshot;
            {
                std::lock_guard<std::mutex> lock(gReportMutex);
                BuildOrchestrator(gReport, false, true);
                snapshot = gReport;
            }
            const auto& functional = snapshot.hardware.stress.functional;
            const auto requiredPortsRemaining =
                RequiredPortsRemaining(snapshot.hardware.stress.portAttestation);
            if (functional.manualRequired || functional.notTested) {
                CommitManualResults(RunFunctionalIoWizard(hwnd));
            } else if (requiredPortsRemaining > 0) {
                std::wstring portId;
                std::wstring label;
                std::wstring capability;
                if (!SelectNextChassisPort(
                        hwnd,
                        snapshot.hardware.stress.chassisProfile,
                        portId,
                        label,
                        capability)) {
                    return 0;
                }
                auto result = RunPhysicalPortProbe(hwnd, label, &gCancel);
                result.expectedPortId = portId;
                CommitPortResultGuided(result);
            } else {
                OpenCurrentReport(hwnd);
            }
            return 0;
        }
        return 0;
    }

    case WM_SIZE:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONDOWN: {
        const int x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
        const int y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
        RECT client{};
        GetClientRect(hwnd, &client);
        const int dpi = GetDpiForHwnd(hwnd);
        const auto layout = ComputeAppShellLayout(client, dpi);

        MainTab clickedTab{};
        bool toggleDevice = false;
        const int hit = HitTestSidebar(x, y, layout.sidebarRect, dpi, gDeviceGroupExpanded, gSidebarScrollOffset, clickedTab, toggleDevice);
        if (hit == 2 && toggleDevice) {
            gDeviceGroupExpanded = !gDeviceGroupExpanded;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (hit == 1) {
            gCurrentTab = clickedTab;
            gTableScrollOffset = 0;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        // 2–3. S01 top mode strip and Start/Stop button. These hit regions exist only on Dashboard.
        const int modeY = layout.contentRect.top + UiMetrics::Scale(70, dpi);
        if (gCurrentTab == MainTab::Dashboard) {
            const int buttonW = UiMetrics::Scale(230, dpi);
            const int buttonH = UiMetrics::Scale(40, dpi);
            const RECT startButton{client.right - buttonW - UiMetrics::Scale(24, dpi), modeY - UiMetrics::Scale(2, dpi),
                                   client.right - UiMetrics::Scale(24, dpi), modeY - UiMetrics::Scale(2, dpi) + buttonH};
            if (PtInRect(&startButton, POINT{x, y})) {
                StartAudit(hwnd);
                return 0;
            }
            const int firstX = layout.contentRect.left + UiMetrics::Scale(134, dpi);
            const int pillW = UiMetrics::Scale(80, dpi);
            const int pillH = UiMetrics::Scale(28, dpi);
            const int gap = UiMetrics::Scale(6, dpi);
            if (y >= modeY && y <= modeY + pillH) {
                if (x >= firstX && x <= firstX + pillW) gSelectedMode = L"Quick";
                else if (x >= firstX + pillW + gap && x <= firstX + 2 * pillW + gap) gSelectedMode = L"Standard";
                else if (x >= firstX + 2 * (pillW + gap) && x <= firstX + 3 * pillW + 2 * gap) gSelectedMode = L"Deep";
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }

        // S02: purpose/mode selection and visible start action.
        if (gCurrentTab == MainTab::NewSession) {
            const int rightPanelW = UiMetrics::Scale(300, dpi);
            const int leftW = client.right - client.left - UiMetrics::Scale(48, dpi) - rightPanelW;
            int currentY = layout.contentRect.top + UiMetrics::Scale(94, dpi);
            const int startX = layout.contentRect.left + UiMetrics::Scale(24, dpi);
            for (int purpose = 0; purpose < 3; ++purpose) {
                const RECT card{startX, currentY, startX + leftW, currentY + UiMetrics::Scale(64, dpi)};
                if (PtInRect(&card, POINT{x, y})) {
                    gInspectionPurpose = purpose;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                currentY += UiMetrics::Scale(72, dpi);
            }
            currentY += UiMetrics::Scale(32, dpi);
            const int cardW = (leftW - UiMetrics::Scale(16, dpi)) / 3;
            const wchar_t* modes[] = {L"Quick", L"Standard", L"Deep"};
            for (int i = 0; i < 3; ++i) {
                const int cardX = startX + i * (cardW + UiMetrics::Scale(8, dpi));
                const RECT card{cardX, currentY, cardX + cardW, currentY + UiMetrics::Scale(110, dpi)};
                if (PtInRect(&card, POINT{x, y})) {
                    gSelectedMode = modes[i];
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
            }
            const int rightX = client.right - rightPanelW - UiMetrics::Scale(24, dpi);
            const int rightY = layout.contentRect.top + UiMetrics::Scale(70, dpi);
            const RECT preflight{rightX, rightY, client.right - UiMetrics::Scale(24, dpi), rightY + UiMetrics::Scale(220, dpi)};
            const RECT panel{rightX, preflight.bottom + UiMetrics::Scale(12, dpi), client.right - UiMetrics::Scale(24, dpi), preflight.bottom + UiMetrics::Scale(180, dpi)};
            const RECT action = GetNextActionButtonRect(panel, dpi);
            if (PtInRect(&action, POINT{x, y})) {
                StartAudit(hwnd);
                return 0;
            }
        }

        // S04: pause/resume and cancel/restart controls use the same rail geometry as the current renderer family.
        if (gCurrentTab == MainTab::AutoAudit) {
            const int clientWidth = static_cast<int>(client.right - client.left);
            const int rightPanelW = std::clamp(clientWidth * 26 / 100,
                                               UiMetrics::Scale(260, dpi), UiMetrics::Scale(340, dpi));
            const int rightX = client.right - rightPanelW - UiMetrics::Scale(20, dpi);
            const int mainW = rightX - layout.contentRect.left - UiMetrics::Scale(32, dpi);
            const int top = layout.contentRect.top + UiMetrics::Scale(70, dpi);
            const int progressH = UiMetrics::Scale(54, dpi);
            const RECT progress{layout.contentRect.left + UiMetrics::Scale(24, dpi), top,
                                layout.contentRect.left + UiMetrics::Scale(24, dpi) + mainW, top + progressH};
            const int pauseW = UiMetrics::Scale(85, dpi), pauseH = UiMetrics::Scale(28, dpi);
            const RECT pause{progress.right - pauseW - UiMetrics::Scale(10, dpi), progress.top + (progressH - pauseH) / 2,
                             progress.right - UiMetrics::Scale(10, dpi), progress.top + (progressH - pauseH) / 2 + pauseH};
            if (PtInRect(&pause, POINT{x, y})) {
                if (gRunning) {
                    gPaused = !gPaused;
                    gSessionLifecycleState = gPaused ? CanonicalUiState::Paused : CanonicalUiState::Running;
                    PostStatus(hwnd, gPaused ? L"Đã tạm dừng quy trình kiểm tra tự động." : L"Tiếp tục quy trình kiểm tra tự động.");
                } else if (gAuditCompletedItems == 0) {
                    StartAudit(hwnd);
                }
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            const RECT timeCard{rightX, top, client.right - UiMetrics::Scale(20, dpi), top + UiMetrics::Scale(90, dpi)};
            const RECT guide{rightX, timeCard.bottom + UiMetrics::Scale(10, dpi), client.right - UiMetrics::Scale(20, dpi), client.bottom - UiMetrics::Scale(16, dpi)};
            const int cancelH = UiMetrics::Scale(36, dpi);
            const RECT cancel{guide.left + UiMetrics::Scale(12, dpi), guide.bottom - cancelH - UiMetrics::Scale(12, dpi),
                              guide.right - UiMetrics::Scale(12, dpi), guide.bottom - UiMetrics::Scale(12, dpi)};
            if (PtInRect(&cancel, POINT{x, y})) {
                if (gRunning) {
                    gCancel = true;
                    gPaused = false;
                    gSessionLifecycleState = CanonicalUiState::Cancelled;
                    PostStatus(hwnd, L"Đã yêu cầu hủy kiểm tra...");
                } else {
                    StartAudit(hwnd);
                }
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
        }

        // S05 functional module cards.
        if (gCurrentTab == MainTab::Functional) {
            const int rightPanelW = UiMetrics::Scale(250, dpi);
            const int leftW = client.right - client.left - UiMetrics::Scale(48, dpi) - rightPanelW;
            const int top = layout.contentRect.top + UiMetrics::Scale(135, dpi);
            const int startX = layout.contentRect.left + UiMetrics::Scale(24, dpi);
            const int cardW = (leftW - UiMetrics::Scale(12, dpi)) / 2;
            const int cardH = UiMetrics::Scale(95, dpi);
            const int commands[] = {1201, 1202, 1204, 1206, 1207, 1208};
            for (int i = 0; i < 6; ++i) {
                const int col = i % 2, row = i / 2;
                const int cardX = startX + col * (cardW + UiMetrics::Scale(12, dpi));
                const int cardY = top + row * (cardH + UiMetrics::Scale(10, dpi));
                const RECT card{cardX, cardY, cardX + cardW, cardY + cardH};
                if (PtInRect(&card, POINT{x, y})) {
                    PostMessageW(hwnd, WM_COMMAND, commands[i], 0);
                    return 0;
                }
            }
            const int rightX = client.right - rightPanelW - UiMetrics::Scale(24, dpi);
            const RECT panel{rightX, top, client.right - UiMetrics::Scale(24, dpi), top + UiMetrics::Scale(305, dpi)};
            const RECT action = GetNextActionButtonRect(panel, dpi);
            if (PtInRect(&action, POINT{x, y})) {
                PostMessageW(hwnd, WM_COMMAND, 1300, 0);
                return 0;
            }
        }

        // S03/S06/S07/S08 visible right-rail actions.
        struct RailAction { MainTab tab; int height; int command; bool startAudit; };
        const RailAction railActions[] = {
            {MainTab::SellerClaim, 240, 1209, false},
            {MainTab::PhysicalSafety, 310, 1208, false},
            {MainTab::PortsPower, 280, 1300, false},
            {MainTab::Stress, 280, 0, true},
        };
        for (const auto& railAction : railActions) {
            if (gCurrentTab != railAction.tab) continue;
            const int rightW = UiMetrics::Scale(300, dpi);
            const int rightX = client.right - rightW - UiMetrics::Scale(24, dpi);
            const int top = layout.contentRect.top + UiMetrics::Scale(70, dpi);
            const RECT panel{rightX, top, client.right - UiMetrics::Scale(24, dpi), top + UiMetrics::Scale(railAction.height, dpi)};
            const RECT action = GetNextActionButtonRect(panel, dpi);
            if (PtInRect(&action, POINT{x, y})) {
                if (railAction.startAudit) StartAudit(hwnd);
                else PostMessageW(hwnd, WM_COMMAND, railAction.command, 0);
                return 0;
            }
        }

        // S18 Final Report and S19 Export use renderer-owned action geometry.
        if (gCurrentTab == MainTab::Reports) {
            const RECT action = GetScreenS18PrimaryActionRect(layout.contentRect, dpi);
            if (PtInRect(&action, POINT{x, y})) {
                gCurrentTab = MainTab::ExportShare;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
        }
        if (gCurrentTab == MainTab::ExportShare) {
            const RECT action = GetScreenS19PrimaryActionRect(layout.contentRect, dpi);
            if (PtInRect(&action, POINT{x, y})) {
                OpenCurrentReport(hwnd);
                return 0;
            }
        }

        // S11-S14 primary actions share C10 rail geometry.
        if (gCurrentTab == MainTab::Memory || gCurrentTab == MainTab::Display ||
            gCurrentTab == MainTab::AudioCamera || gCurrentTab == MainTab::Network) {
            const int pad = UiMetrics::Scale(24, dpi);
            const int top = layout.contentRect.top + UiMetrics::Scale(72, dpi);
            const int rightW = UiMetrics::Scale(300, dpi);
            const int leftRight = layout.contentRect.right - rightW - UiMetrics::Scale(34, dpi);
            const RECT rail{leftRight + UiMetrics::Scale(10, dpi), top,
                            layout.contentRect.right - pad, layout.contentRect.bottom - UiMetrics::Scale(20, dpi)};
            const RECT action = GetNextActionButtonRect(rail, dpi);
            if (PtInRect(&action, POINT{x, y})) {
                if (gCurrentTab == MainTab::Memory) ActivateMemoryPrimaryAction(hwnd);
                else if (gCurrentTab == MainTab::Display) PostMessageW(hwnd, WM_COMMAND, 1201, 0);
                else if (gCurrentTab == MainTab::AudioCamera) PostMessageW(hwnd, WM_COMMAND, 1212, 0);
                else PostMessageW(hwnd, WM_COMMAND, 1213, 0);
                return 0;
            }
        }

        // S22: persisted history open/delete actions.
        if (gCurrentTab == MainTab::SessionHistory) {
            const auto history = GetSessionHistorySnapshot();
            const int rightW = UiMetrics::Scale(300, dpi), gap = UiMetrics::Scale(12, dpi);
            const RECT body{layout.contentRect.left + UiMetrics::Scale(24, dpi), layout.contentRect.top + UiMetrics::Scale(70, dpi),
                            layout.contentRect.right - UiMetrics::Scale(24, dpi), layout.contentRect.bottom - UiMetrics::Scale(20, dpi)};
            const RECT table{body.left, body.top, body.right - rightW - gap, body.bottom};
            const int rowH = UiMetrics::Scale(UiMetrics::TableRowHeight, dpi);
            if (!history.empty() && x >= table.left && x <= table.right && y >= table.top + rowH && y < table.bottom) {
                const int visibleRow = (y - (table.top + rowH)) / rowH;
                const int index = gTableScrollOffset + visibleRow;
                if (index >= 0 && index < static_cast<int>(history.size())) {
                    gHistorySelectedIndex = index;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
            }
            if (!history.empty()) {
                const int index = std::clamp(gHistorySelectedIndex, 0, static_cast<int>(history.size()) - 1);
                const auto& selected = history[static_cast<size_t>(index)];
                const RECT detail{table.right + gap, body.top, body.right, body.bottom};
                const int buttonH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
                const RECT open{detail.left + 14, detail.bottom - buttonH * 2 - UiMetrics::Scale(24, dpi),
                                detail.right - 14, detail.bottom - buttonH - UiMetrics::Scale(18, dpi)};
                const RECT remove{detail.left + 14, detail.bottom - buttonH - UiMetrics::Scale(10, dpi),
                                  detail.right - 14, detail.bottom - UiMetrics::Scale(10, dpi)};
                if (PtInRect(&open, POINT{x, y})) {
                    const auto path = !selected.htmlPath.empty() ? selected.htmlPath :
                                      (!selected.jsonPath.empty() ? selected.jsonPath : selected.evidencePath);
                    if (!path.empty() && IsTrustedSessionArtifactPath(path)) ShellExecuteW(hwnd, L"open", path.c_str(), nullptr, nullptr, SW_SHOW);
                    else MessageBoxW(hwnd, L"Phiên này chưa có artifact tin cậy để mở.", L"LapSure", MB_OK | MB_ICONINFORMATION);
                    return 0;
                }
                if (PtInRect(&remove, POINT{x, y})) {
                    const int answer = MessageBoxW(hwnd,
                        L"YES: xóa mục lịch sử và artifact.\nNO: chỉ xóa mục index.\nCANCEL: giữ nguyên.",
                        L"Xóa phiên kiểm định", MB_YESNOCANCEL | MB_ICONWARNING);
                    if (answer == IDYES || answer == IDNO) {
                        if (!DeleteSessionHistoryEntry(selected.sessionId, answer == IDYES)) {
                            MessageBoxW(hwnd, L"Không thể xóa phiên hoặc artifact không vượt qua kiểm tra an toàn.", L"LapSure", MB_OK | MB_ICONERROR);
                            return 0;
                        }
                        const auto after = GetSessionHistorySnapshot();
                        gHistorySelectedIndex = after.empty() ? 0 : std::min(gHistorySelectedIndex, static_cast<int>(after.size()) - 1);
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    return 0;
                }
            }
        }

        // S23: interrupted stress recovery always uses the explicit persistent state root.
        if (gCurrentTab == MainTab::InterruptedRecovery) {
            const auto stateRoot = EnsureReportOutputRoot();
            const RECT body{layout.contentRect.left + UiMetrics::Scale(24, dpi), layout.contentRect.top + UiMetrics::Scale(70, dpi),
                            layout.contentRect.right - UiMetrics::Scale(24, dpi), layout.contentRect.bottom - UiMetrics::Scale(20, dpi)};
            const int rightW = UiMetrics::Scale(330, dpi), gap = UiMetrics::Scale(10, dpi);
            const RECT actions{body.right - rightW, body.top, body.right, body.bottom};
            const int buttonH = UiMetrics::Scale(UiMetrics::ButtonHeight, dpi);
            const int startY = actions.bottom - (buttonH * 3 + gap * 2 + UiMetrics::Scale(14, dpi));
            const RECT recover{actions.left + 14, startY, actions.right - 14, startY + buttonH};
            const RECT archive{actions.left + 14, recover.bottom + gap, actions.right - 14, recover.bottom + gap + buttonH};
            const RECT discard{actions.left + 14, archive.bottom + gap, actions.right - 14, archive.bottom + gap + buttonH};
            auto clearInterrupted = [&]() {
                std::lock_guard<std::mutex> lock(gReportMutex);
                gReport.hardware.stress.previousInterruptedSessionDetected = false;
                gReport.hardware.stress.journalPath.clear();
                gReport.findings.erase(std::remove_if(gReport.findings.begin(), gReport.findings.end(), [](const Finding& finding) {
                    return finding.name == L"Previous interrupted stress session";
                }), gReport.findings.end());
            };
            if (PtInRect(&recover, POINT{x, y}) || PtInRect(&archive, POINT{x, y})) {
                if (!ArchiveInterruptedSession(stateRoot, stateRoot)) {
                    MessageBoxW(hwnd, L"Không thể lưu journal gián đoạn; bằng chứng gốc được giữ nguyên.", L"LapSure", MB_OK | MB_ICONERROR);
                    return 0;
                }
                clearInterrupted();
                if (PtInRect(&recover, POINT{x, y})) StartAudit(hwnd);
                else InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            if (PtInRect(&discard, POINT{x, y})) {
                const int answer = MessageBoxW(hwnd,
                    L"Bỏ journal sẽ xóa bằng chứng gián đoạn hiện tại. Thao tác này không tạo PASS. Tiếp tục?",
                    L"Bỏ journal gián đoạn", MB_YESNO | MB_ICONWARNING);
                if (answer == IDYES) {
                    if (DiscardInterruptedStressJournal(gReportOutputDir)) {
                        clearInterrupted();
                        InvalidateRect(hwnd, nullptr, FALSE);
                    } else {
                        MessageBoxW(hwnd, L"Không thể xóa journal.", L"LapSure", MB_OK | MB_ICONERROR);
                    }
                }
                return 0;
            }
        }
        return 0;
    }

    case WM_AUDIT_STATUS: {
        auto* text = reinterpret_cast<std::wstring*>(lParam);
        delete text;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_AUDIT_DONE: {
        Fill();
        const BOOL enabled = gAuditReady ? TRUE : FALSE;
        SetFunctionalButtonsEnabled(enabled);
        if (gNext) EnableWindow(gNext, enabled);
        if (gOpen) EnableWindow(gOpen, gPublicationReady ? TRUE : FALSE);
        gRunning = false;
        if (wParam == 0) {
            const auto snapshot = ReportSnapshot();
            gSessionLifecycleState = LifecycleStateFromDecision(snapshot);
        } else if (wParam == 2) {
            gSessionLifecycleState = CanonicalUiState::Interrupted;
        } else if (gSessionLifecycleState != CanonicalUiState::Interrupted) {
            gSessionLifecycleState = CanonicalUiState::Cancelled;
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        if (gCloseRequested) DestroyWindow(hwnd);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(hwnd, &paint);
        RECT client{};
        GetClientRect(hwnd, &client);
        const int dpi = GetDpiForHwnd(hwnd);
        const auto report = ReportSnapshot();
        const auto logs = LogSnapshot();

        HDC bufferDc = CreateCompatibleDC(dc);
        HBITMAP bitmap = CreateCompatibleBitmap(dc, client.right - client.left, client.bottom - client.top);
        HGDIOBJ oldBitmap = SelectObject(bufferDc, bitmap);
        DrawAppShellBackground(bufferDc, client);
        const auto layout = ComputeAppShellLayout(client, dpi);

        std::wstring sessionStatus;
        if (gRunning) sessionStatus = L"Đang kiểm tra...";
        else if (gAuditReady && gPublicationReady) sessionStatus = L"Kiểm định xong • report đã lưu";
        else if (gAuditReady) sessionStatus = L"Kiểm định xong • report chưa lưu";
        else if (gSessionLifecycleState == CanonicalUiState::Cancelled) sessionStatus = L"Đã hủy";
        else if (gSessionLifecycleState == CanonicalUiState::Interrupted) sessionStatus = L"Bị gián đoạn";
        else sessionStatus = L"Chưa bắt đầu";
        DrawSidebar(bufferDc, layout.sidebarRect, gCurrentTab, gFonts, dpi, gDeviceGroupExpanded, gSidebarScrollOffset,
                    sessionStatus, L"Windows x64 Native");

        switch (gCurrentTab) {
        case MainTab::Dashboard:
            RenderScreenS01_Overview(bufferDc, layout.contentRect, report, gFonts, dpi, gSelectedMode, gRunning, gAuditReady,
                                     gSessionLifecycleState, gAuditCompletedItems, gAuditTotalItems, gFocusIndex); break;
        case MainTab::NewSession:
            RenderScreenS02_NewSession(bufferDc, layout.contentRect, report, gFonts, dpi, gInspectionPurpose, gSelectedMode, gRunning, gFocusIndex); break;
        case MainTab::SellerClaim:
            RenderScreenS03_SellerClaim(bufferDc, layout.contentRect, report, gFonts, dpi, gFocusIndex); break;
        case MainTab::AutoAudit:
            RenderScreenS04_AutoAudit(bufferDc, layout.contentRect, report, gFonts, dpi, gSelectedMode, gRunning, gPaused,
                                      gAuditCompletedItems, gAuditTotalItems, gAuditCurrentStage, gAuditElapsedSec, logs, gFocusIndex); break;
        case MainTab::Functional:
            RenderScreenS05_Functional(bufferDc, layout.contentRect, report, gFonts, dpi, 1, {}, {}, false, gFocusIndex); break;
        case MainTab::PhysicalSafety:
            RenderScreenS06_PhysicalSafety(bufferDc, layout.contentRect, report, gFonts, dpi, 0, 1, {}, gFocusIndex); break;
        case MainTab::PortsPower:
            RenderScreenS07_PortsPower(bufferDc, layout.contentRect, report, gFonts, dpi, 0, gFocusIndex); break;
        case MainTab::Stress:
            RenderScreenS08_StressStability(bufferDc, layout.contentRect, report, gFonts, dpi, gRunning, gAuditElapsedSec, {}, {}, {}, {}, logs, gFocusIndex); break;
        case MainTab::Battery:
            RenderScreenS09_BatteryPower(bufferDc, layout.contentRect, report, gFonts, dpi, {}, {}, gFocusIndex); break;
        case MainTab::Storage:
            RenderScreenS10_Storage(bufferDc, layout.contentRect, report, gFonts, dpi, 0, gTableScrollOffset, gFocusIndex); break;
        case MainTab::Memory:
            RenderScreenS11_Memory(bufferDc, layout.contentRect, report, gFonts, dpi, gTableScrollOffset, gFocusIndex); break;
        case MainTab::Display:
            RenderScreenS12_Display(bufferDc, layout.contentRect, report, gFonts, dpi, 0, {}, gFocusIndex); break;
        case MainTab::AudioCamera:
            RenderScreenS13_AudioCamera(bufferDc, layout.contentRect, report, gFonts, dpi, 0, gFocusIndex); break;
        case MainTab::Network:
            RenderScreenS14_Network(bufferDc, layout.contentRect, report, gFonts, dpi, {}, logs, gFocusIndex); break;
        case MainTab::SystemInfo:
            RenderScreenS15_SystemInfo(bufferDc, layout.contentRect, report, gFonts, dpi, gTableScrollOffset, gFocusIndex); break;
        case MainTab::FactoryProfileMatch:
            RenderScreenS16_FactoryCompare(bufferDc, layout.contentRect, report, gFonts, dpi, gTableScrollOffset, gFocusIndex); break;
        case MainTab::EvidenceLibrary:
            RenderScreenS17_EvidenceLibrary(bufferDc, layout.contentRect, report, gFonts, dpi, 0, 0, 0, gFocusIndex); break;
        case MainTab::Reports:
            RenderScreenS18_FinalReport(bufferDc, layout.contentRect, report, gFonts, dpi, gFocusIndex); break;
        case MainTab::ExportShare:
            RenderScreenS19_ExportShare(bufferDc, layout.contentRect, report, gFonts, dpi, 0, 0, gFocusIndex); break;
        case MainTab::LogsEvents:
            RenderScreenS20_LogsEvents(bufferDc, layout.contentRect, report, gFonts, dpi, 0, 0, logs, gTableScrollOffset, gFocusIndex); break;
        case MainTab::Settings:
            RenderScreenS21_Settings(bufferDc, layout.contentRect, report, gFonts, dpi, 0, gFocusIndex); break;
        case MainTab::SessionHistory:
            RenderScreenS22_SessionHistory(bufferDc, layout.contentRect, report, gFonts, dpi, gTableScrollOffset, gHistorySelectedIndex, gFocusIndex); break;
        case MainTab::InterruptedRecovery:
            RenderScreenS23_InterruptedRecovery(bufferDc, layout.contentRect, report, gFonts, dpi, gFocusIndex); break;
        default:
            RenderFallbackScreen(bufferDc, layout.contentRect, gFonts, dpi); break;
        }

        DrawAppShellFooter(bufferDc, layout.footerRect, gFonts, dpi, 0, 0);
        BitBlt(dc, 0, 0, client.right - client.left, client.bottom - client.top, bufferDc, 0, 0, SRCCOPY);
        SelectObject(bufferDc, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(bufferDc);
        EndPaint(hwnd, &paint);
        return 0;
    }

    case WM_CLOSE:
        if (gRunning) {
            gCloseRequested = true;
            gCancel = true;
            return 0;
        }
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        gCancel = true;
        if (gWorker.joinable()) gWorker.join();
        gFonts.Cleanup();
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}