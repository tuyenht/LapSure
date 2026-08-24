from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "src" / "main.cpp"
RUN = ROOT / "run_source_tests.cmd"
TEST = ROOT / "tests" / "production_hardening_keyboard_dispatch_sanity.py"

text = MAIN.read_text(encoding="utf-8")
old = '''        case VK_RETURN:\n        case VK_SPACE:\n            if (gFocusIndex == 2) StartAudit(h);\n            else if (gFocusIndex == 3) PostMessageW(h, WM_COMMAND, 1300, 0);\n            return 0;\n'''
new = '''        case VK_RETURN:\n        case VK_SPACE:\n            if (gFocusIndex == 2) {\n                StartAudit(h);\n                return 0;\n            }\n            if (gFocusIndex != 3) return 0;\n\n            // Screen-aware primary action dispatch. Keyboard activation must never\n            // execute a generic action that does not match the visible screen CTA.\n            switch (gCurrentTab) {\n            case MainTab::Dashboard:\n            case MainTab::AutoAudit:\n            case MainTab::Functional:\n                PostMessageW(h, WM_COMMAND, 1300, 0);\n                break;\n            case MainTab::NewSession:\n            case MainTab::Stress:\n                StartAudit(h);\n                break;\n            case MainTab::SellerClaim:\n                PostMessageW(h, WM_COMMAND, 1209, 0);\n                break;\n            case MainTab::PhysicalSafety:\n                PostMessageW(h, WM_COMMAND, 1208, 0);\n                break;\n            case MainTab::PortsPower:\n                PostMessageW(h, WM_COMMAND, 1207, 0);\n                break;\n            case MainTab::Display:\n                PostMessageW(h, WM_COMMAND, 1201, 0);\n                break;\n            case MainTab::AudioCamera:\n                PostMessageW(h, WM_COMMAND, 1212, 0);\n                break;\n            case MainTab::Network:\n                PostMessageW(h, WM_COMMAND, 1213, 0);\n                break;\n            case MainTab::Reports:\n                gCurrentTab = MainTab::ExportShare;\n                InvalidateRect(h, nullptr, FALSE);\n                break;\n            case MainTab::ExportShare:\n                if (!gReportPath.empty() && IsTrustedSessionArtifactPath(gReportPath)) {\n                    ShellExecuteW(h, L"open", gReportPath.c_str(), nullptr, nullptr, SW_SHOW);\n                } else {\n                    MessageBoxW(h, L"Phiên hiện tại chưa có HTML report tin cậy để mở.", L"LapSure", MB_OK | MB_ICONINFORMATION);\n                }\n                break;\n            default:\n                // Screens with no single primary CTA (or multiple actions such as\n                // history/recovery) require explicit pointer/focus selection.\n                break;\n            }\n            return 0;\n'''
if text.count(old) != 1:
    raise SystemExit(f"main.cpp: expected keyboard block exactly once, found {text.count(old)}")
MAIN.write_text(text.replace(old, new), encoding="utf-8")

test = r'''from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")

assert "Screen-aware primary action dispatch" in MAIN
assert "if (gFocusIndex != 3) return 0;" in MAIN
assert "else if (gFocusIndex == 3) PostMessageW(h, WM_COMMAND, 1300, 0);" not in MAIN

required = {
    "seller claim": "case MainTab::SellerClaim:",
    "physical safety": "case MainTab::PhysicalSafety:",
    "ports power": "case MainTab::PortsPower:",
    "display": "case MainTab::Display:",
    "audio camera": "case MainTab::AudioCamera:",
    "network": "case MainTab::Network:",
    "final report": "case MainTab::Reports:",
    "export": "case MainTab::ExportShare:",
}
for name, token in required.items():
    assert token in MAIN, f"missing keyboard route for {name}"

for command in ["1209", "1208", "1207", "1201", "1212", "1213"]:
    assert f"WM_COMMAND, {command}, 0" in MAIN, f"missing focused command {command}"

assert "gCurrentTab = MainTab::ExportShare;" in MAIN
assert "IsTrustedSessionArtifactPath(gReportPath)" in MAIN
assert "Screens with no single primary CTA" in MAIN

# S22/S23 have multiple actions; keyboard focus must not silently choose one.
keyboard_start = MAIN.index("case VK_RETURN:")
keyboard_end = MAIN.index("case WM_MOUSEWHEEL:", keyboard_start)
keyboard = MAIN[keyboard_start:keyboard_end]
assert "case MainTab::SessionHistory:" not in keyboard
assert "case MainTab::InterruptedRecovery:" not in keyboard

print("Production hardening keyboard dispatch sanity: OK")
'''
TEST.write_text(test, encoding="utf-8")

run = RUN.read_text(encoding="utf-8")
line = "python tests\\production_hardening_keyboard_dispatch_sanity.py\nif errorlevel 1 exit /b 1\n"
if "production_hardening_keyboard_dispatch_sanity.py" not in run:
    if not run.endswith("\n"):
        run += "\n"
    run += line
RUN.write_text(run, encoding="utf-8")
