from pathlib import Path

path = Path("src/main.cpp")
text = path.read_text(encoding="utf-8")
old = "RenderScreenS02_NewSession(memDC, layout.contentRect, repSnapshot, gFonts, dpi, gFocusIndex);"
new = "RenderScreenS02_NewSession(memDC, layout.contentRect, repSnapshot, gFonts, dpi, gInspectionPurpose, gSelectedMode, gRunning, gFocusIndex);"
count = text.count(old)
if count != 1:
    raise SystemExit(f"S02 runtime call: expected exactly one old binding, found {count}")
text = text.replace(old, new, 1)
path.write_text(text, encoding="utf-8")
print("Applied S02 live session-state binding")
