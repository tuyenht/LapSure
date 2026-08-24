from pathlib import Path

R = Path(__file__).resolve().parents[1]
h = (R / "include/lap/ui_screens.h").read_text(encoding="utf-8")
ui = (R / "src/ui_screens_s16_s21_v2.cpp").read_text(encoding="utf-8")
main = (R / "src/main.cpp").read_text(encoding="utf-8")

checks = [
    ("S18 primary action geometry exported", "GetScreenS18PrimaryActionRect" in h and "GetScreenS18PrimaryActionRect" in ui),
    ("S19 primary action geometry exported", "GetScreenS19PrimaryActionRect" in h and "GetScreenS19PrimaryActionRect" in ui),
    ("S18 visibly renders export CTA", 'action.buttonText = L"XUẤT BÁO CÁO"' in ui and "DrawNextActionPanel(dc, actionPanel, action" in ui),
    ("S18 click navigates to export screen", "GetScreenS18PrimaryActionRect(layout.contentRect, dpi)" in main and "gCurrentTab = MainTab::ExportShare" in main),
    ("S19 availability comes from persisted history", "GetSessionHistorySnapshot()" in ui and "IsTrustedSessionArtifactPath(htmlPath)" in ui and "IsTrustedSessionArtifactPath(jsonPath)" in ui),
    ("S19 does not fake unavailable formats", 'L"PDF / Ký số", L"CHƯA HỖ TRỢ"' in ui and 'action.isButtonEnabled = htmlReady' in ui),
    ("S19 click uses renderer geometry", "GetScreenS19PrimaryActionRect(layout.contentRect, dpi)" in main),
    ("S19 open is trust-gated", "IsTrustedSessionArtifactPath(gReportPath)" in main),
    ("no legacy invisible S18/S19 action-card hotspots", "Reports Screen: Export Button Hit-Test" not in main and "Export & Share Screen: Open in Browser Button Hit-Test" not in main),
    ("explicit post-stress cancel discards journal", "else if (!report.hardware.stress.sessionId.empty())" in main and "DiscardInterruptedStressJournal(gDir)" in main),
]

bad = []
for name, ok in checks:
    print(("PASS" if ok else "FAIL"), name)
    if not ok:
        bad.append(name)
print(f"{len(checks)-len(bad)}/{len(checks)} PASS")
raise SystemExit(bool(bad))
