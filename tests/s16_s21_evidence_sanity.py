from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UI = (ROOT / "src" / "ui_screens_s16_s21_v2.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

# Legacy mock/demo values must never enter the production S16-S21 renderer.
FORBIDDEN = [
    "Dell Precision 5560",
    "8TM8D33",
    'L"95%"',
    'L"96%"',
    'L"12 / 12"',
    'L"Health 89%"',
    'L"Pin còn 89% dung lượng"',
    "VNPT_Office_5G",
    "8.8.8.8",
    "18 ms",
    "24/08/2026 09:42:31",
    "612 KB (JPG)",
    "Ảnh chụp (11)",
    "Tất cả (28)",
]
for literal in FORBIDDEN:
    assert literal not in UI, f"demo/fixed literal leaked into S16-S21 production renderer: {literal}"

# S16 uses actual findings and keeps seller/factory semantics separate from health.
assert "rep.findings" in UI
assert "Dimension::Factory" in UI
assert 'finding.group == L"Cam kết người bán"' in UI
assert "rep.factoryExact" in UI and "rep.genericMode" in UI

# S17 is an evidence index over actual report evidence, not a gallery of invented screenshots.
for source in [
    "rep.findings",
    "rep.hardware.stress.functional.items",
    "rep.hardware.stress.stages",
    "rep.hardware.stress.portPower.ports",
    "rep.hardware.stress.runtimeValidation.checks",
]:
    assert source in UI, f"missing evidence source: {source}"
assert "sự tồn tại của bằng chứng không tự tạo trạng thái ĐẠT" in UI

# S18 verdict and required coverage are computed from product data.
assert "BuildCoverageContract(rep)" in UI
assert "FormatDecisionVi(decision)" in UI
assert "decision.criticalFails" in UI
assert "decision.warnings" in UI
assert "decision.confidence" in UI
assert "decision.reasons" in UI
assert "requiredComplete" in UI and "requiredTotal" in UI
assert "Không phải điểm sức khỏe" in UI

# S19 exposes only actually implemented report backends.
assert 'L"HTML", L"HỖ TRỢ"' in UI
assert 'L"JSON", L"HỖ TRỢ"' in UI
assert 'L"PDF / Ký số", L"CHƯA HỖ TRỢ"' in UI
assert "không tuyên bố có cloud sharing hoặc PDF" in UI

# S20 uses live logs and EventLog counts without claiming causality.
assert "liveLogs" in UI
assert "rep.hardware.events" in UI
assert "Không suy diễn ổ hỏng chỉ từ count" in UI
assert "không tự gán GPU hỏng" in UI

# S21 is read-only/policy-backed until settings persistence exists.
assert "không hiển thị toggle nếu chưa có persistence thực" in UI
assert "rep.hardware.stress.runtimeValidation.checks" in UI
assert "SHA-256 allowlist" in UI

# Canonical runtime symbols must route away from the legacy demo implementations.
assert "src/ui_screens_s16_s21_v2.cpp" in CMAKE
for symbol in [
    "RenderScreenS16_FactoryCompare",
    "RenderScreenS17_EvidenceLibrary",
    "RenderScreenS18_FinalReport",
    "RenderScreenS19_ExportShare",
    "RenderScreenS20_LogsEvents",
    "RenderScreenS21_Settings",
]:
    assert f"{symbol}={symbol}_Legacy" in CMAKE, f"legacy route missing for {symbol}"

assert "/wd" not in CMAKE
print("S16-S21 evidence-bound renderer sanity: OK")
