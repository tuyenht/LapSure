from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENV = (ROOT / "src" / "environment.cpp").read_text(encoding="utf-8")
ENG = (ROOT / "src" / "engines.cpp").read_text(encoding="utf-8")
MAIN = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
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

# All gReportPath ShellExecute routes must be guarded in the surrounding route/block.
unsafe_old_routes = [
    'else if (id == 2 && !gReportPath.empty()) ShellExecuteW',
    'else if (!gReportPath.empty()) ShellExecuteW(h, L"open", gReportPath.c_str()',
]
for route in unsafe_old_routes:
    assert route not in MAIN, f"legacy unguarded report-open route remains: {route}"

needle = 'ShellExecuteW(h, L"open", gReportPath.c_str(), nullptr, nullptr, SW_SHOW);'
pos = 0
count = 0
while True:
    pos = MAIN.find(needle, pos)
    if pos < 0:
        break
    count += 1
    context = MAIN[max(0, pos - 360):pos + len(needle)]
    assert 'IsTrustedSessionArtifactPath(gReportPath)' in context, context
    pos += len(needle)
assert count >= 3, "expected keyboard, command, and S19 trusted report-open routes"

assert "PATH-discovered" in SEC
assert "does not modify Windows TrustedPublisher" in SEC
print("Production security hardening sanity: OK")
