from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UI = (ROOT / "src" / "ui_screens_s02_s09_v2.cpp").read_text(encoding="utf-8")
MAIN = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

FORBIDDEN = [
    "LS-20260824-001",
    "Dell Precision 5560",
    "Nguyễn Văn An",
    "SanDisk 16GB USB-C",
    "DisplayPort 1920x1080",
    "LGD06B5",
    "SK Hynix",
    "HMA82GS6DJR8N-XN",
    "4A29B01C",
    "32 GB",
    "3200 MHz",
    "0 Lỗi",
    "Tấm nền zin",
    "Ổ SSD hoạt động xuất sắc",
]
for literal in FORBIDDEN:
    assert literal not in UI, f"demo/fixed literal leaked into production S02-S09: {literal}"

# S02 must render the actual selected workflow state passed by main, with no provider probing in paint.
assert "inspectionPurpose" in UI and "selectedMode" in UI and "running" in UI
assert "DetectCapabilities" not in UI
assert "gInspectionPurpose, gSelectedMode, gRunning" in MAIN

# S03 is seller-claim data bound and keeps commercial comparison separate from health.
assert "rep.sellerClaim" in UI
assert 'finding.group == L"Cam kết người bán"' in UI
assert "cam kết không thay thế kiểm tra kỹ thuật" in UI

# S05/S06 only surface real functional/manual evidence.
for item_id in [
    "display_visual", "keyboard_function", "speaker_function", "camera_function", "mic_function",
    "wifi_function", "bluetooth_function", "physical_chassis", "physical_hinge", "physical_tamper",
    "physical_liquid", "physical_battery", "physical_charger",
]:
    assert item_id in UI, f"missing functional evidence binding: {item_id}"
assert "MapFunctionalStatus" in UI
assert "pass == total" in UI

# S07 is stimulus/evidence bound; no controller-presence shortcut and no invented negotiated speed.
assert "rep.hardware.stress.portPower" in UI
assert "FindPortResult" in UI
assert "negotiatedSpeed.empty() ? L\"Không suy đoán\"" in UI
assert "controller presence không chứng minh từng cổng hoạt động" in UI
assert "adapterWatts >= 0.0" in UI

# S08 uses actual stage telemetry/event deltas and measured elapsed values.
assert "stress.stages" in UI
assert "stage.elapsedSeconds" in UI and "stage.plannedSeconds" in UI
for event in ["stage.newWhea", "stage.newDisk", "stage.newNvme", "stage.newDisplay", "stage.newBugCheck"]:
    assert event in UI
assert "telemetrySummary.maxCpuPackageTempC" in UI
assert "telemetrySummary.maxGpuTempC" in UI
assert "stress.cpuBenchmark.score" in UI

# S09 must present battery-capacity evidence, not a generic machine health score or invented runtime/wattage.
assert "battery.designWh" in UI
assert "battery.fullChargeWh" in UI
assert "battery.healthPercent" in UI
assert "Full-charge / design; không phải health score toàn máy" in UI
assert "Không suy diễn thời lượng sử dụng từ một mẫu ngắn" in UI
assert "power.adapterWatts >= 0.0" in UI

# Build routes canonical S02/S03/S05-S09 away from the legacy demo renderers.
assert "src/ui_screens_s02_s09_v2.cpp" in CMAKE
for symbol in [
    "RenderScreenS02_NewSession", "RenderScreenS03_SellerClaim", "RenderScreenS05_Functional",
    "RenderScreenS06_PhysicalSafety", "RenderScreenS07_PortsPower", "RenderScreenS08_StressStability",
    "RenderScreenS09_BatteryPower",
]:
    assert f"{symbol}={symbol}_Legacy" in CMAKE, f"legacy route missing for {symbol}"

assert "/wd" not in CMAKE
print("S02-S09 evidence-bound renderer sanity: OK")
