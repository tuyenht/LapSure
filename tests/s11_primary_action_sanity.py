from pathlib import Path
from app_source_view import read_app_source

ROOT = Path(__file__).resolve().parents[1]
MAIN = read_app_source(ROOT)
S11 = (ROOT / "src" / "ui_screens_s10_s15_v2.cpp").read_text(encoding="utf-8")

assert 'L"CHẠY STRESS / RAM TEST"' in S11
assert 'L"XEM BẰNG CHỨNG RAM"' in S11
assert "S11-S14 primary actions share C10 rail geometry" in MAIN
assert "ActivateMemoryPrimaryAction(hwnd)" in MAIN

keyboard_start = MAIN.index("case VK_RETURN:")
keyboard_end = MAIN.index("case WM_MOUSEWHEEL:", keyboard_start)
keyboard = MAIN[keyboard_start:keyboard_end]
assert "case MainTab::Memory:" in keyboard

print("S11 primary action sanity: OK")
