from pathlib import Path
from app_source_view import read_app_source

ROOT = Path(__file__).resolve().parents[1]
MAIN = read_app_source(ROOT)

keyboard_start = MAIN.index("case WM_KEYDOWN:")
keyboard_end = MAIN.index("case WM_MOUSEWHEEL:", keyboard_start)
keyboard = MAIN[keyboard_start:keyboard_end]

focus2_start = keyboard.index("if (actionFocus == 2)")
focus2_end = keyboard.index("if (actionFocus != 3)", focus2_start)
focus2 = keyboard[focus2_start:focus2_end]
assert "gCurrentTab == MainTab::Dashboard" in focus2
for hidden in ["MainTab::AutoAudit", "MainTab::NewSession", "MainTab::Stress"]:
    assert hidden not in focus2, f"focus-2 still activates an invisible top CTA on {hidden}"

for arrow in ["case VK_LEFT:", "case VK_RIGHT:"]:
    start = keyboard.index(arrow)
    end = keyboard.index("return 0;", start)
    block = keyboard[start:end]
    assert "gCurrentTab == MainTab::Dashboard" in block
    assert "gCurrentTab == MainTab::NewSession" in block

mouse_start = MAIN.index("case WM_LBUTTONDOWN:")
mouse_end = MAIN.index("case WM_AUDIT_STATUS:", mouse_start)
mouse = MAIN[mouse_start:mouse_end]
start_marker = mouse.index("// 2–3. S01 top mode strip")
s02_marker = mouse.index("// S02:", start_marker)
s01_block = mouse[start_marker:s02_marker]
assert "gCurrentTab == MainTab::Dashboard" in s01_block, "S01 Start/Stop/mode hitboxes are still global"
assert "StartAudit(hwnd);" in s01_block
assert "gSelectedMode" in s01_block

print("Hidden control interaction sanity: OK")
