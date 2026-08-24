from pathlib import Path

root = Path(__file__).resolve().parents[1]
main_path = root / "src" / "main.cpp"
text = main_path.read_text(encoding="utf-8")

anchor = '''std::vector<MainTab> GetVisualTabList() {
    std::vector<MainTab> list;
    auto groups = GetDefaultSidebarGroups(gDeviceGroupExpanded);
    for (const auto& g : groups) {
        if (g.isExpanded) {
            for (const auto& it : g.items) {
                list.push_back(it.tab);
            }
        }
    }
    return list;
}
'''
insert = anchor + '''

bool HasRamStressEvidence() {
    std::lock_guard<std::mutex> lk(gReportMutex);
    for (const auto& stage : gReport.hardware.stress.stages) {
        if (stage.ram.bytesAllocated || stage.ram.bytesTested || stage.ram.mismatches || stage.ram.passes) return true;
    }
    return false;
}

void ActivateMemoryPrimaryAction(HWND h) {
    if (HasRamStressEvidence()) {
        gCurrentTab = MainTab::Stress;
        gFocusIndex = 3;
        InvalidateRect(h, nullptr, FALSE);
        return;
    }
    StartAudit(h);
}
'''
if "void ActivateMemoryPrimaryAction(HWND h)" not in text:
    if anchor not in text:
        raise SystemExit("GetVisualTabList anchor not found")
    text = text.replace(anchor, insert, 1)

old_key = '''        case VK_RETURN:
        case VK_SPACE:
            if (gFocusIndex == 2) {
                StartAudit(h);
                return 0;
            }
            if (gFocusIndex != 3) return 0;

            // Screen-aware primary action dispatch. Keyboard activation must never
            // execute a generic action that does not match the visible screen CTA.
            switch (gCurrentTab) {
'''
new_key = '''        case VK_RETURN:
        case VK_SPACE: {
            const int actionFocus = gFocusIndex;
            if (actionFocus == 2) {
                // Focus selects the visible top-level CTA; the current screen still
                // decides whether that CTA is an audit action. No global StartAudit.
                switch (gCurrentTab) {
                case MainTab::Dashboard:
                case MainTab::AutoAudit:
                case MainTab::NewSession:
                case MainTab::Stress:
                    StartAudit(h);
                    break;
                default:
                    break;
                }
                return 0;
            }
            if (actionFocus != 3) return 0;

            // Screen-aware primary action dispatch. Keyboard activation must never
            // execute a generic action that does not match the visible screen CTA.
            switch (gCurrentTab) {
'''
if old_key not in text:
    raise SystemExit("keyboard dispatch anchor not found")
text = text.replace(old_key, new_key, 1)

old_switch = '''            case MainTab::Network:
                PostMessageW(h, WM_COMMAND, 1213, 0);
                break;
            case MainTab::Reports:
'''
new_switch = '''            case MainTab::Network:
                PostMessageW(h, WM_COMMAND, 1213, 0);
                break;
            case MainTab::Memory:
                ActivateMemoryPrimaryAction(h);
                break;
            case MainTab::Reports:
'''
if old_switch not in text:
    raise SystemExit("keyboard memory insertion anchor not found")
text = text.replace(old_switch, new_switch, 1)

old_close = '''            return 0;
        }
        break;
    }
    case WM_MOUSEWHEEL: {
'''
new_close = '''            return 0;
        }
        }
        break;
    }
    case WM_MOUSEWHEEL: {
'''
if old_close not in text:
    raise SystemExit("keyboard scope close anchor not found")
text = text.replace(old_close, new_close, 1)

mouse_anchor = '''        // 14–16. S12/S13/S14 primary actions: use the same C10 geometry as rendering.
'''
mouse_insert = '''        // S11 primary action hit-test: use the exact C10 rail geometry from the renderer.
        if (gCurrentTab == MainTab::Memory) {
            const int pad = UiMetrics::Scale(24, dpi);
            const int top = layout.contentRect.top + UiMetrics::Scale(72, dpi);
            const int rightW = UiMetrics::Scale(300, dpi);
            const int leftRight = layout.contentRect.right - rightW - UiMetrics::Scale(34, dpi);
            RECT rail{leftRight + UiMetrics::Scale(10, dpi), top,
                      layout.contentRect.right - pad, layout.contentRect.bottom - UiMetrics::Scale(20, dpi)};
            const RECT actionButton = GetNextActionButtonRect(rail, dpi);
            if (x >= actionButton.left && x <= actionButton.right && y >= actionButton.top && y <= actionButton.bottom) {
                ActivateMemoryPrimaryAction(h);
                return 0;
            }
        }

''' + mouse_anchor
if "S11 primary action hit-test" not in text:
    if mouse_anchor not in text:
        raise SystemExit("S11 mouse insertion anchor not found")
    text = text.replace(mouse_anchor, mouse_insert, 1)

main_path.write_text(text, encoding="utf-8")
print("Round 4A interaction patch applied")
