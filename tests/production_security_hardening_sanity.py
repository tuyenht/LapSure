from pathlib import Path
from app_source_view import read_app_source

ROOT = Path(__file__).resolve().parents[1]
ENV = (ROOT / "src" / "environment.cpp").read_text(encoding="utf-8")
ENG = (ROOT / "src" / "engines.cpp").read_text(encoding="utf-8")
MAIN = read_app_source(ROOT)
MANIFEST = (ROOT / "tools" / "engine_manifest.txt").read_text(encoding="utf-8")
APP = (ROOT / "app.manifest").read_text(encoding="utf-8")
SEC = (ROOT / "SECURITY.md").read_text(encoding="utf-8")

assert 'CommandExists(L"smartctl.exe")' not in ENV
assert 'CommandExists(L"nvidia-smi.exe")' not in ENV
assert 'VerifyEngine(appDir,L"tools\\\\smartctl.exe",L"smartctl")' in ENV
assert 'VerifyEngine(appDir,L"tools\\\\nvidia-smi.exe",L"nvidia_smi")' in ENV
assert '?L"\\\""+dir+L"\\\\tools\\\\smartctl.exe\\\"":L"smartctl.exe"' not in ENG
assert '?L"\\\""+dir+L"\\\\tools\\\\nvidia-smi.exe\\\"":L"nvidia-smi.exe"' not in ENG
assert 'smartctl=' in MANIFEST and 'nvidia_smi=' in MANIFEST
assert 'assemblyIdentity version="0.1.1.0"' in APP
assert 'certutil' not in MAIN.lower()

for route in [
    'else if (id == 2 && !gReportPath.empty()) ShellExecuteW',
    'ShellExecuteW(hwnd, L"open", gReportPath.c_str()',
    'ShellExecuteW(h, L"open", gReportPath.c_str()',
]:
    assert route not in MAIN, f"legacy unguarded report-open route remains: {route}"

helper_start = MAIN.index("void OpenCurrentReport")
helper_end = MAIN.index("AuditReport ReportSnapshot", helper_start)
helper = MAIN[helper_start:helper_end]
assert "CurrentReportPath()" in helper
assert "IsTrustedSessionArtifactPath(path)" in helper
assert 'ShellExecuteW(hwnd, L"open", path.c_str()' in helper
assert helper.index("IsTrustedSessionArtifactPath(path)") < helper.index("ShellExecuteW(hwnd")

# History artifacts are also trust-gated before opening.
assert "IsTrustedSessionArtifactPath(path)" in MAIN
assert MAIN.count("ShellExecuteW(") >= 2

assert "PATH-discovered" in SEC
assert "does not modify Windows TrustedPublisher" in SEC
print("Production security hardening sanity: OK")
