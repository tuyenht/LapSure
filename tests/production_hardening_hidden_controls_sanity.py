from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")

# Keyboard focus may only activate controls actually rendered on the current screen.
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

# The top mode strip and Start/Stop button are drawn by S01 only. Their mouse
# hit regions must not exist globally on unrelated screens.
mouse_start = MAIN.index("case WM_LBUTTONDOWN:")
mouse_end = MAIN.index("case WM_SIZE:", mouse_start) if "case WM_SIZE:" in MAIN[mouse_start:] else len(MAIN)
mouse = MAIN[mouse_start:mouse_end]

start_hit = mouse.index("// 2. Start / Stop Button Click") if "// 2. Start / Stop Button Click" in mouse else mouse.index("// 2–3. S01 top mode strip")
mode_hit = mouse.index("// 3. Mode Pills Click", start_hit) if "// 3. Mode Pills Click" in mouse[start_hit:] else mouse.index("// 3.1 S01 Dashboard Specific Click Hit-Tests", start_hit)
start_block = mouse[start_hit:mode_hit]
assert "gCurrentTab == MainTab::Dashboard" in start_block, "S01 Start/Stop hitbox is still global"

new_session_marker = mouse.find("New Session", mode_hit)
assert new_session_marker != -1, "could not locate S02-specific mouse handling"
mode_block = mouse[start_hit:new_session_marker]
assert "gCurrentTab == MainTab::Dashboard" in mode_block, "S01 mode-pill hitbox is still global"

print("Hidden control interaction sanity: OK")
