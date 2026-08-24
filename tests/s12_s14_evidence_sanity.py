from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UI = (ROOT / "src" / "ui_screens_s12_s14_v2.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

# Production S12/S14 must never reintroduce the old populated mock/demo values.
FORBIDDEN = [
    "LGD06B5",
    "Tấm nền zin",
    "Checksum OK",
    "DisplayPort 1920x1080",
    "SanDisk 16GB USB-C",
    "Ping Internet",
    "Mbps",
]
for literal in FORBIDDEN:
    assert literal not in UI, f"demo/infrastructure literal leaked into S12/S14: {literal}"

# S12 — EDID/native timing is identity evidence, visual quality remains manual evidence.
for binding in [
    "friendlyName", "manufacturer", "serialNumber", "nativeWidth", "nativeHeight",
    "currentWidth", "currentHeight", "refreshHz", "touchDetected", "edidHex",
]:
    assert binding in UI, f"missing display evidence binding: {binding}"
assert "current mode không phải native proof" in UI
assert "Không dùng vendor code để suy diễn panel 'zin'" in UI
assert "Dead/stuck pixel, bleed, ám màu" in UI
assert "brightness" not in UI.lower() or "không hiển thị" in UI.lower()
assert "display_visual" in UI

# S14 — network functionality is separate from presence and external infrastructure.
for binding in ["wifi_function", "bluetooth_function", "ethernet_presence", "portPower"]:
    assert binding in UI, f"missing network evidence binding: {binding}"
assert "throughput Internet phụ thuộc hạ tầng mạng" in UI
assert "Presence chỉ nhận diện adapter" in UI
assert "không suy ra chất lượng Internet" in UI
assert "Bluetooth" in UI and "enumeration" in UI

# Canonical runtime symbols must route away from legacy demo renderers.
assert "src/ui_screens_s12_s14_v2.cpp" in CMAKE
for symbol in ["RenderScreenS12_Display", "RenderScreenS14_Network"]:
    assert f"{symbol}={symbol}_Legacy" in CMAKE
assert "/wd" not in CMAKE

print("S12/S14 evidence-bound renderer sanity: OK")
