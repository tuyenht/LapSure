from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UI = (ROOT / "src" / "ui_screens_s01_s04_v2.cpp").read_text(encoding="utf-8")
MAIN = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
SHELL = (ROOT / "src" / "ui_shell_dynamic.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
SCORING = (ROOT / "src" / "scoring.cpp").read_text(encoding="utf-8")

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

# Grouped functional state must not turn partially resolved groups into PASS.
assert "passed == total" in UI
assert "passed > 0 || unsupported > 0 || unresolved > 0" in UI

# Storage domain is health + required coverage, never health evidence alone.
assert "StorageDomainState" in UI
assert 'CoverageState(coverage, L"storage")' in UI
assert "THIẾU COVERAGE BẮT BUỘC" in UI

# CPU stage truth: stage 2 is inventory only; microbenchmark belongs to stress stage 9.
assert 'case 2: return L"CPU & Nhận diện";' in UI
assert "microbenchmark chỉ chạy trong bước Stress & Ổn định" in UI
assert "microbenchmark CPU được chạy và ghi bằng chứng tại bước này" in UI
assert 'case 2: return L"CPU & Microbench";' not in UI
assert 'case 2: return L"CPU & Microbench";' not in MAIN
assert "Đang đọc thông tin CPU và vi điểm chuẩn" not in MAIN

# Presence-only network collection remains explicitly non-functional evidence.
assert "chức năng thực tế vẫn cần kiểm tra riêng" in UI
assert "chức năng thực tế vẫn cần kiểm tra riêng" in MAIN

# Generic machine-health scoring must not return to these screens.
LOWER = UI.lower()
assert "điểm sức khỏe" not in LOWER
assert "DrawCircularScoreGauge" not in UI
assert "score gauge" not in LOWER

# No synthetic audit-stage duration literals. Only measured stress elapsed is formatted.
assert "stage.elapsedSeconds" in UI
assert "FormatDuration" in UI

# App shell runtime facts must stay light and evidence-safe: no provider probing from paint,
# no heuristic engine count, and no fixed database/runtime metadata.
assert "GetReadyEngineCount" not in MAIN
assert "GetReadyEngineCount" not in SHELL
assert "DetectCapabilities" not in SHELL
assert "RuntimeMeta()" in SHELL
assert "OpenProcessToken" in SHELL
assert "GetNativeSystemInfo" in SHELL
assert "MiniNT" in SHELL
assert "Provider readiness: xem trạng thái kiểm định" in SHELL
assert "readyEngines" in SHELL and "(void)readyEngines" in SHELL

# Coverage contract: seller claim is useful comparison evidence, but it is not a required
# hardware/evidence domain in docs/COVERAGE_CONTRACT.md.
assert 'add(L"seller_claim",L"Cấu hình người bán cam kết",claimComplete' in SCORING
seller_line = next(line for line in SCORING.splitlines() if 'add(L"seller_claim"' in line)
assert seller_line.rstrip().endswith(',false);'), "seller claim must remain optional in coverage contract"

# Build must route canonical S01/S04 to the production renderer and dynamic shell wrappers.
assert "src/ui_screens_s01_s04_v2.cpp" in CMAKE
assert "src/ui_shell_dynamic.cpp" in CMAKE
assert "RenderScreenS01_Overview=RenderScreenS01_Overview_Legacy" in CMAKE
assert "RenderScreenS04_AutoAudit=RenderScreenS04_AutoAudit_Legacy" in CMAKE
assert "DrawSidebar=DrawSidebar_Legacy" in CMAKE
assert "DrawAppShellFooter=DrawAppShellFooter_Legacy" in CMAKE

# Strict builds must not hide warnings with /wd suppressions.
assert "/wd" not in CMAKE

# Inventory-only source gate relies on these explicit markers and must never include stress.
assert "// inventory_only_begin" in MAIN and "// inventory_only_end" in MAIN
inventory_region = MAIN[MAIN.index("// inventory_only_begin"):MAIN.index("// inventory_only_end")]
assert "RunStressSession" not in inventory_region

print("S01/S04 dynamic-binding and shell sanity: OK")
