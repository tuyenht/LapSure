from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
S11 = (ROOT / "src" / "ui_screens_s10_s15_v2.cpp").read_text(encoding="utf-8")

assert 'L"CHẠY STRESS / RAM TEST"' in S11
assert 'L"XEM BẰNG CHỨNG RAM"' in S11
assert "S11 primary action hit-test" in MAIN, "visible S11 CTA has no matching mouse action"

keyboard_start = MAIN.index("case VK_RETURN:")
keyboard_end = MAIN.index("case WM_MOUSEWHEEL:", keyboard_start)
keyboard = MAIN[keyboard_start:keyboard_end]
assert "case MainTab::Memory:" in keyboard, "S11 primary action is not keyboard reachable"

print("S11 primary action sanity: OK")
