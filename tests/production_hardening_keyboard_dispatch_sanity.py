from pathlib import Path

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
    "memory": "case MainTab::Memory:",
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

keyboard_start = MAIN.index("case VK_RETURN:")
keyboard_end = MAIN.index("case WM_MOUSEWHEEL:", keyboard_start)
keyboard = MAIN[keyboard_start:keyboard_end]

# A focus index is presentation state, never an operation selector. The old
# global focus-2 route could start a full audit from an unrelated screen.
assert "if (gFocusIndex == 2)" not in keyboard, "global focus-2 operation dispatch remains"
assert "case MainTab::Memory:" in keyboard, "S11 primary action is not keyboard reachable"

# S22/S23 have multiple actions; keyboard focus must not silently choose one.
assert "case MainTab::SessionHistory:" not in keyboard
assert "case MainTab::InterruptedRecovery:" not in keyboard

print("Production hardening keyboard dispatch sanity: OK")
