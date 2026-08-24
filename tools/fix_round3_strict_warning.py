from pathlib import Path

p = Path(__file__).resolve().parents[1] / "src" / "ui_screens_s16_s21_v2.cpp"
text = p.read_text(encoding="utf-8")
old = '''static RECT ScreenS18ActionPanelRect(const RECT& r, int dpi) {\n    const RECT body = ContentBody(r, dpi);\n    const int gap = UiMetrics::Scale(10, dpi);\n    const int kpiH = UiMetrics::Scale(92, dpi);\n    const int tableTop = body.top + kpiH + UiMetrics::Scale(34, dpi);\n    const int sideW = UiMetrics::Scale(320, dpi);\n    const int actionH = UiMetrics::Scale(188, dpi);\n    return RECT{body.right - sideW, body.bottom - actionH, body.right, body.bottom};\n}\n'''
new = '''static RECT ScreenS18ActionPanelRect(const RECT& r, int dpi) {\n    const RECT body = ContentBody(r, dpi);\n    const int sideW = UiMetrics::Scale(320, dpi);\n    const int actionH = UiMetrics::Scale(188, dpi);\n    return RECT{body.right - sideW, body.bottom - actionH, body.right, body.bottom};\n}\n'''
if text.count(old) != 1:
    raise SystemExit(f"expected one exact S18 helper, found {text.count(old)}")
p.write_text(text.replace(old, new), encoding="utf-8")
print("Round 3 strict-warning fix applied")
