from pathlib import Path

UI_PATH = Path("src/ui_screens_s01_s04_v2.cpp")
MAIN_PATH = Path("src/main.cpp")
CMAKE_PATH = Path("CMakeLists.txt")

text = UI_PATH.read_text(encoding="utf-8")


def replace_once(old: str, new: str, label: str) -> None:
    global text
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {count}")
    text = text.replace(old, new, 1)


replace_once(
'''CanonicalUiState MergeFunctional(std::initializer_list<CanonicalUiState> states) {
    bool hasPass = false;
    bool hasUnsupported = false;
    for (auto state : states) {
        if (state == CanonicalUiState::Fail || state == CanonicalUiState::Error) return CanonicalUiState::Fail;
        if (state == CanonicalUiState::Warning) return CanonicalUiState::Warning;
        if (state == CanonicalUiState::PermissionDenied) return CanonicalUiState::PermissionDenied;
        if (state == CanonicalUiState::ProviderUnavailable) return CanonicalUiState::ProviderUnavailable;
        if (state == CanonicalUiState::ManualRequired) return CanonicalUiState::ManualRequired;
        if (state == CanonicalUiState::Incomplete) return CanonicalUiState::Incomplete;
        if (state == CanonicalUiState::Pass || state == CanonicalUiState::Good) hasPass = true;
        if (state == CanonicalUiState::Unsupported) hasUnsupported = true;
    }
    if (hasPass) return CanonicalUiState::Pass;
    if (hasUnsupported) return CanonicalUiState::Unsupported;
    return CanonicalUiState::NotTested;
}
''',
'''CanonicalUiState MergeFunctional(std::initializer_list<CanonicalUiState> states) {
    int passed = 0;
    int unsupported = 0;
    int unresolved = 0;
    const int total = static_cast<int>(states.size());
    for (auto state : states) {
        if (state == CanonicalUiState::Fail || state == CanonicalUiState::Error) return CanonicalUiState::Fail;
        if (state == CanonicalUiState::Warning) return CanonicalUiState::Warning;
        if (state == CanonicalUiState::PermissionDenied) return CanonicalUiState::PermissionDenied;
        if (state == CanonicalUiState::ProviderUnavailable) return CanonicalUiState::ProviderUnavailable;
        if (state == CanonicalUiState::ManualRequired) return CanonicalUiState::ManualRequired;
        if (state == CanonicalUiState::Incomplete) return CanonicalUiState::Incomplete;
        if (state == CanonicalUiState::Pass || state == CanonicalUiState::Good) {
            ++passed;
            continue;
        }
        if (state == CanonicalUiState::Unsupported) {
            ++unsupported;
            continue;
        }
        ++unresolved;
    }
    if (total > 0 && passed == total) return CanonicalUiState::Pass;
    if (total > 0 && unsupported == total) return CanonicalUiState::Unsupported;
    if (passed > 0 || unsupported > 0 || unresolved > 0) return CanonicalUiState::Incomplete;
    return CanonicalUiState::NotTested;
}

struct FindingCounts {
    unsigned warnings{};
    unsigned criticalFails{};
};

FindingCounts CountLiveFindings(const AuditReport& rep) {
    FindingCounts counts;
    for (const auto& finding : rep.findings) {
        if (finding.severity == Severity::Critical && finding.state == State::Fail) ++counts.criticalFails;
        if (finding.state == State::Warning) ++counts.warnings;
    }
    return counts;
}
''',
"merge functional and live finding counts")

replace_once(
'''std::wstring StorageHealthDetail(const AuditReport& rep) {
    if (rep.hardware.storage.empty()) return L"Chưa nhận diện ổ lưu trữ";
    const auto& disk = rep.hardware.storage.front();
    if (!disk.reliabilityProvider.empty()) return disk.reliabilityProvider;
    if (disk.smartReadable) return L"SMART/NVMe health evidence";
    return L"Thiếu provider sức khỏe ổ lưu trữ";
}
''',
'''std::wstring StorageHealthDetail(const AuditReport& rep) {
    if (rep.hardware.storage.empty()) return L"Chưa nhận diện ổ lưu trữ";
    const auto& disk = rep.hardware.storage.front();
    if (!disk.reliabilityProvider.empty()) return disk.reliabilityProvider;
    if (disk.smartReadable) return L"SMART/NVMe health evidence";
    return L"Thiếu provider sức khỏe ổ lưu trữ";
}

CanonicalUiState StorageDomainState(const AuditReport& rep, const CoverageSnapshot& coverage) {
    const auto health = StorageHealthState(rep);
    if (health == CanonicalUiState::Fail) return CanonicalUiState::Fail;
    if (CoverageState(coverage, L"storage") != CanonicalUiState::Pass) {
        return health == CanonicalUiState::ProviderUnavailable
            ? CanonicalUiState::ProviderUnavailable
            : CanonicalUiState::Incomplete;
    }
    return health == CanonicalUiState::Good ? CanonicalUiState::Good : CanonicalUiState::Incomplete;
}
''',
"storage domain helper")

replace_once(
'''CanonicalUiState PhysicalSafetyState(const FunctionalTestSummary& summary) {
    bool any = false;
    bool allResolved = true;
    for (const auto& item : summary.items) {
        if (item.id.rfind(L"physical_", 0) != 0) continue;
        any = true;
        auto state = MapFunctionalStatus(item.status);
        if (state == CanonicalUiState::Fail) return CanonicalUiState::Fail;
        if (state == CanonicalUiState::Warning) return CanonicalUiState::Warning;
        if (state == CanonicalUiState::ManualRequired || state == CanonicalUiState::NotTested)
            allResolved = false;
    }
    if (!any) return CanonicalUiState::NotTested;
    return allResolved ? CanonicalUiState::Pass : CanonicalUiState::Incomplete;
}

''',
'''',
"remove unused physical helper")

replace_once(
'''    case 4:
        out.source = StorageHealthDetail(rep);
        out.detail = L"Identity + Reliability/SMART/NVMe theo provider thực tế";
        if (completed) {
            out.state = StorageHealthState(rep);
            if (out.state == CanonicalUiState::Good) out.label = L"BẰNG CHỨNG SỨC KHỎE HỢP LỆ";
            else if (out.state == CanonicalUiState::Fail) out.label = L"PHÁT HIỆN LỖI SỨC KHỎE";
            else if (out.state == CanonicalUiState::ProviderUnavailable) out.label = L"THIẾU PROVIDER";
            else out.label = L"THIẾU DỮ LIỆU";
        }
        break;
''',
'''    case 4:
        out.source = StorageHealthDetail(rep);
        out.detail = L"Identity + Reliability/SMART/NVMe theo provider thực tế";
        if (completed) {
            const auto coverage = BuildCoverageSnapshot(rep);
            out.state = StorageDomainState(rep, coverage);
            if (out.state == CanonicalUiState::Good) out.label = L"BẰNG CHỨNG SỨC KHỎE HỢP LỆ";
            else if (out.state == CanonicalUiState::Fail) out.label = L"PHÁT HIỆN LỖI SỨC KHỎE";
            else if (out.state == CanonicalUiState::ProviderUnavailable) out.label = L"THIẾU PROVIDER";
            else out.label = L"THIẾU COVERAGE BẮT BUỘC";
        }
        break;
''',
"stage 4 coverage")

replace_once(
'''    const auto coverage = BuildCoverageSnapshot(rep);
    const auto& decision = rep.hardware.stress.decision;
''',
'''    const auto coverage = BuildCoverageSnapshot(rep);
    const auto& decision = rep.hardware.stress.decision;
    const auto liveFindingCounts = CountLiveFindings(rep);
    const bool hasCollectedFindings = auditCompletedItems > 0 || !rep.findings.empty();
''',
"overview live counts")

replace_once(
'''    warnings.label = L"CẢNH BÁO";
    warnings.value = auditReady ? std::to_wstring(decision.warnings) : L"—";
    warnings.note = L"Từ findings/decision engine";
    warnings.state = auditReady ? (decision.warnings > 0 ? CanonicalUiState::Warning : CanonicalUiState::Info)
                                : CanonicalUiState::NotTested;
''',
'''    warnings.label = L"CẢNH BÁO";
    warnings.value = hasCollectedFindings ? std::to_wstring(liveFindingCounts.warnings) : L"—";
    warnings.note = L"Findings runtime của phiên hiện tại";
    warnings.state = hasCollectedFindings
        ? (liveFindingCounts.warnings > 0 ? CanonicalUiState::Warning : CanonicalUiState::Info)
        : CanonicalUiState::NotTested;
''',
"live warning card")

replace_once(
'''    critical.label = L"LỖI NGHIÊM TRỌNG";
    critical.value = auditReady ? std::to_wstring(decision.criticalFails) : L"—";
    critical.note = L"Lỗi có thể chặn quyết định mua";
    critical.state = auditReady ? (decision.criticalFails > 0 ? CanonicalUiState::Fail : CanonicalUiState::Info)
                                : CanonicalUiState::NotTested;
''',
'''    critical.label = L"LỖI NGHIÊM TRỌNG";
    critical.value = hasCollectedFindings ? std::to_wstring(liveFindingCounts.criticalFails) : L"—";
    critical.note = L"Critical FAIL trong findings runtime";
    critical.state = hasCollectedFindings
        ? (liveFindingCounts.criticalFails > 0 ? CanonicalUiState::Fail : CanonicalUiState::Info)
        : CanonicalUiState::NotTested;
''',
"live critical card")

replace_once(
'''    CanonicalUiState profileState = rep.factoryExact ? CanonicalUiState::Pass
        : (!rep.profileSource.empty() ? CanonicalUiState::Changed : CanonicalUiState::NotTested);
''',
'''    CanonicalUiState profileState = rep.factoryExact ? CanonicalUiState::Info
        : (!rep.profileSource.empty() ? CanonicalUiState::Changed : CanonicalUiState::NotTested);
''',
"factory card semantics")

replace_once(
'''    DrawStatusBadge(dc, profileBadge, profileState, fonts,
                    rep.factoryExact ? L"HỒ SƠ CHÍNH XÁC"
                                     : (!rep.profileSource.empty() ? L"HỒ SƠ THAM CHIẾU" : L"CHƯA CÓ HỒ SƠ"));
''',
'''    DrawStatusBadge(dc, profileBadge, profileState, fonts,
                    rep.factoryExact ? L"KHỚP HỒ SƠ THAM CHIẾU"
                                     : (!rep.profileSource.empty() ? L"HỒ SƠ CÓ SAI KHÁC" : L"CHƯA CÓ HỒ SƠ"));
''',
"factory badge wording")

replace_once(
'''    CanonicalUiState profileDomainState = rep.factoryExact ? CanonicalUiState::Pass
        : (!rep.profileSource.empty() ? CanonicalUiState::Changed : CanonicalUiState::NotTested);
''',
'''    CanonicalUiState profileDomainState = rep.factoryExact ? CanonicalUiState::Info
        : (!rep.profileSource.empty() ? CanonicalUiState::Changed : CanonicalUiState::NotTested);
''',
"profile domain semantics")

replace_once(
'''        { L"SSD", L"Lưu trữ", StorageHealthDetail(rep), StorageHealthState(rep), L"" },
        { L"BAT", L"Pin & Năng lượng", BatteryEvidenceDetail(rep), BatteryEvidenceState(rep), L"" },
''',
'''        { L"SSD", L"Lưu trữ", CoverageDetail(coverage, L"storage", StorageHealthDetail(rep).c_str()), StorageDomainState(rep, coverage), L"" },
        { L"BAT", L"Pin & Năng lượng", BatteryEvidenceDetail(rep), BatteryEvidenceState(rep), rep.hardware.battery.present ? L"" : L"KHÔNG CÓ PIN" },
''',
"storage and battery domain cards")

UI_PATH.write_text(text, encoding="utf-8")

main = MAIN_PATH.read_text(encoding="utf-8")
ready_helper = '''int GetReadyEngineCount() {
    auto caps = DetectCapabilities(gDir);
    int c = 6; // Native Registry, SetupAPI, CIM, MediaFoundation, WLAN, WaveIn
    if (caps.powershell) c++;
    if (caps.wmi) c++;
    if (caps.smartctl) c++;
    if (caps.nvidiaSmi) c++;
    if (caps.battery) c++;
    if (!caps.winPE) c += 2;
    return std::clamp(c, 1, 14);
}

'''
if main.count(ready_helper) != 1:
    raise SystemExit("ready engine helper: expected one match")
main = main.replace(ready_helper, "", 1)

footer_call = '''        // C01 App Shell Footer (with dynamic engine count and DPI scaling)
        int readyEngines = GetReadyEngineCount();
        DrawAppShellFooter(memDC, layout.footerRect, gFonts, dpi, readyEngines, 14);
'''
footer_replacement = '''        // C01 App Shell Footer. Runtime shell intentionally receives no heuristic provider count;
        // provider/engine readiness must come from explicit evidence state rather than WM_PAINT probing.
        DrawAppShellFooter(memDC, layout.footerRect, gFonts, dpi, 0, 0);
'''
if main.count(footer_call) != 1:
    raise SystemExit("footer call: expected one match")
main = main.replace(footer_call, footer_replacement, 1)

inventory_signature = '''int RunInventoryOnly(const std::wstring& outputDir) {
'''
if "// inventory_only_begin" not in main:
    if main.count(inventory_signature) != 1:
        raise SystemExit("inventory marker: RunInventoryOnly signature not unique")
    main = main.replace(inventory_signature, inventory_signature + "  // inventory_only_begin\n", 1)
MAIN_PATH.write_text(main, encoding="utf-8")

cmake = CMAKE_PATH.read_text(encoding="utf-8")
suppression = '''# The v2 translation unit intentionally keeps one internal physical-safety evaluator for the next
# screen-composition pass. Suppress only MSVC C4505 in this file; all other /W4 warnings remain errors in CI.
if(MSVC)
  set_source_files_properties(src/ui_screens_s01_s04_v2.cpp PROPERTIES COMPILE_OPTIONS "/wd4505")
endif()
'''
if suppression in cmake:
    cmake = cmake.replace(suppression, "", 1)
CMAKE_PATH.write_text(cmake, encoding="utf-8")

print("Applied S01/S04 semantic, UI-thread, and inventory validation fixes")
