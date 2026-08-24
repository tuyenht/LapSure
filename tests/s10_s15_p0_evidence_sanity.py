from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UI = (ROOT / "src" / "ui_screens_s10_s15_v2.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

# No legacy/demo storage, RAM, audio/camera, or system literals may enter the production P0 renderer.
FORBIDDEN = [
    "Ổ SSD hoạt động xuất sắc",
    "SK Hynix",
    "HMA82GS6DJR8N-XN",
    "4A29B01C",
    "32 GB",
    "3200 MHz",
    "0 Lỗi",
    "Bình thường (OK)",
    "Đo biên độ âm thanh > 5%",
]
for literal in FORBIDDEN:
    assert literal not in UI, f"demo/fixed literal leaked into P0 renderer: {literal}"

# S10 — Storage provider semantics and filesystem separation.
for binding in [
    "smartReadable", "smartPassed", "reliabilityReadable", "reliabilityHealthy", "reliabilityProvider",
    "criticalWarning", "percentageUsed", "enduranceRemaining", "temperatureC", "powerOnHours",
    "unsafeShutdowns", "mediaErrors", "readErrorsUncorrected", "writeErrorsUncorrected",
]:
    assert binding in UI, f"missing storage evidence binding: {binding}"
assert "!drive.smartReadable && !drive.reliabilityReadable" in UI
assert "filesystem sạch không chứng minh SSD khỏe" in UI
assert "SMART healthy cũng không thay thế kiểm tra filesystem" in UI
assert "Unsafe shutdowns" in UI and "không chứng minh SSD hỏng" in UI

# S11 — RAM inventory vs online partial coverage.
for binding in ["memoryModules", "configuredSpeed", "ratedSpeed", "bytesAllocated", "bytesTested", "passes", "mismatches"]:
    assert binding in UI, f"missing RAM evidence binding: {binding}"
assert "online test remains partial coverage" in UI
assert "không phải chứng nhận preboot" in UI
assert "Không hiển thị 0 lỗi khi test chưa chạy" in UI

# S13 — actual stimulus/capture requirements.
for item in ["camera_function", "mic_function", "speaker_function"]:
    assert item in UI
assert "Media Foundation trả actual frame/sample" in UI
assert "waveIn thu PCM" in UI
assert "PCM L/R độc lập + người dùng xác nhận" in UI
assert "Enumeration/presence" in UI
assert "KHÔNG HỖ TRỢ" in UI

# S15 — system identity/security/PnP/runtime trust are separate evidence classes.
for binding in ["mainboard", "bios.smbiosVersion", "security.tpmPresent", "secureBootKnown", "pnpProblems", "runtimeValidation.checks"]:
    assert binding in UI
assert "Không gắn nhãn 'BIOS cũ'" in UI
assert "Security posture, không phải hardware-health score" in UI
assert "Code 43" in UI and "không được gán nguyên nhân chung" in UI

# Build routes only the P0 screens implemented in this translation unit.
assert "src/ui_screens_s10_s15_v2.cpp" in CMAKE
for symbol in ["RenderScreenS10_Storage", "RenderScreenS11_Memory", "RenderScreenS13_AudioCamera", "RenderScreenS15_SystemInfo"]:
    assert f"{symbol}={symbol}_Legacy" in CMAKE
assert "/wd" not in CMAKE

print("S10/S11/S13/S15 P0 evidence sanity: OK")
