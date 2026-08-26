from pathlib import Path
from app_source_view import read_app_source

ROOT = Path(__file__).resolve().parents[1]
MAIN = read_app_source(ROOT)

assert "Screen-aware primary action dispatch" in MAIN
assert "else if (gFocusIndex == 3) PostMessageW" not in MAIN

required = {
    "seller claim": "case MainTab::SellerClaim:",
    "physical safety": "case MainTab::PhysicalSafety:",
    "ports power": "case MainTab::PortsPower:",
    "display": "case MainTab::Display:",
    "audio camera": "case MainTab::AudioCamera:",
    "network": "case MainTab::Network:",
    "memory": "case MainTab::Memory:",
    "final report": "case MainTab::Reports:",
    "export": "case MainTab::ExportShare:",
}
for name, token in required.items():
    assert token in MAIN, f"missing keyboard route for {name}"

for command in ["1209", "1208", "1201", "1212", "1213"]:
    assert f"WM_COMMAND, {command}, 0" in MAIN, f"missing focused command {command}"

assert "gCurrentTab = MainTab::ExportShare;" in MAIN
assert "OpenCurrentReport(hwnd)" in MAIN
assert "IsTrustedSessionArtifactPath(path)" in MAIN
assert "Screens with no single primary CTA" in MAIN

keyboard_start = MAIN.index("case VK_RETURN:")
keyboard_end = MAIN.index("case WM_MOUSEWHEEL:", keyboard_start)
keyboard = MAIN[keyboard_start:keyboard_end]
assert "if (gFocusIndex == 2)" not in keyboard, "global focus-2 operation dispatch remains"
assert "const int actionFocus = gFocusIndex;" in keyboard
assert "if (actionFocus != 3) return 0;" in keyboard
focus2_start = keyboard.index("if (actionFocus == 2)")
focus2_end = keyboard.index("if (actionFocus != 3)", focus2_start)
focus2 = keyboard[focus2_start:focus2_end]
assert "gCurrentTab == MainTab::Dashboard" in focus2
assert "StartAudit(hwnd);" in focus2
for hidden in ["MainTab::AutoAudit", "MainTab::NewSession", "MainTab::Stress"]:
    assert hidden not in focus2, f"focus-2 still activates an invisible top CTA on {hidden}"

assert "case MainTab::Memory:" in keyboard
assert "case MainTab::SessionHistory:" not in keyboard
assert "case MainTab::InterruptedRecovery:" not in keyboard

ports_start = keyboard.index("case MainTab::PortsPower:")
ports_end = keyboard.index("case MainTab::Display:", ports_start)
ports_route = keyboard[ports_start:ports_end]
assert "WM_COMMAND, 1300, 0" in ports_route, "Ports & Power must use guided stable-ID continuation"
assert "WM_COMMAND, 1207, 0" not in ports_route, "Ports & Power primary route must not use generic label-only probing"

print("Production hardening keyboard dispatch sanity: OK")