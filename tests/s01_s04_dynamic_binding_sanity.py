from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UI = (ROOT / "src" / "ui_screens_s01_s04_v2.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

# Values that previously existed only to make mockups look populated.
FORBIDDEN = [
    "LS-20260824-001",
    "96%",
    "12 / 12",
    "18+ Chassis",
    "00:05",
    "00:34",
    "00:42",
    "01:18",
    "01:04",
    "Intel Core i7-11800H",
    "32GB DDR4 3200MHz",
    "1TB NVMe PCIe 4.0",
    "22.500.000",
    "Health 89%",
    "Health 86%",
]
for literal in FORBIDDEN:
    assert literal not in UI, f"demo/fixed literal leaked into production S01/S04: {literal}"

# Runtime renderer must be bound to evidence/decision/session sources.
REQUIRED_BINDINGS = [
    "BuildCoverageContract(rep)",
    "rep.hardware.stress.decision",
    "rep.hardware.stress.orchestrator",
    "rep.hardware.stress.functional",
    "rep.hardware.stress.portPower",
    "rep.hardware.events.querySucceeded",
    "rep.hardware.storage",
    "rep.hardware.battery",
    "auditCompletedItems",
    "auditTotalItems",
    "auditCurrentStage",
    "auditElapsedSec",
    "liveLogs",
]
for binding in REQUIRED_BINDINGS:
    assert binding in UI, f"missing dynamic binding in production S01/S04: {binding}"

# Evidence semantics: automatic presence/enumeration must not be presented as functional PASS.
assert "enumeration không chứng minh chức năng" in UI
assert "Presence ≠ functionality" in UI
assert "ĐÃ NHẬN DIỆN" in UI
assert "ProviderUnavailable" in UI
assert "ManualRequired" in UI

# Generic machine-health scoring must not return to these screens.
LOWER = UI.lower()
assert "điểm sức khỏe" not in LOWER
assert "DrawCircularScoreGauge" not in UI
assert "score gauge" not in LOWER

# No synthetic audit-stage duration literals. Only measured stress elapsed is formatted.
assert "stage.elapsedSeconds" in UI
assert "FormatDuration" in UI

# Build must route the canonical S01/S04 symbols to the production renderer file.
assert "src/ui_screens_s01_s04_v2.cpp" in CMAKE
assert "RenderScreenS01_Overview=RenderScreenS01_Overview_Legacy" in CMAKE
assert "RenderScreenS04_AutoAudit=RenderScreenS04_AutoAudit_Legacy" in CMAKE

print("S01/S04 dynamic-binding sanity: OK")
