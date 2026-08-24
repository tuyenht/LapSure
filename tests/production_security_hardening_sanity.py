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

# All gReportPath ShellExecute routes must be guarded by the trusted artifact predicate.
for line in MAIN.splitlines():
    if 'ShellExecuteW' in line and 'gReportPath.c_str()' in line:
        assert 'IsTrustedSessionArtifactPath(gReportPath)' in line, line

assert "PATH-discovered" in SEC
assert "does not modify Windows TrustedPublisher" in SEC
print("Production security hardening sanity: OK")
