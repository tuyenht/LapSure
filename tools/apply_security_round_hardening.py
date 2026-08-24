from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def replace_once(rel, old, new):
    p = ROOT / rel
    text = p.read_text(encoding="utf-8-sig" if rel == "resources/resource.rc" else "utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{rel}: expected exactly one match, found {count}")
    text = text.replace(old, new)
    p.write_text(text, encoding="utf-8")

# Capability discovery must not mark PATH-resolved diagnostic engines trusted.
replace_once(
    "src/environment.cpp",
    '#include "lap/environment.h"\n#include <windows.h>\n',
    '#include "lap/environment.h"\n#include "lap/trust.h"\n#include <windows.h>\n',
)
replace_once(
    "src/environment.cpp",
    ' c.powershell=CommandExists(L"powershell.exe");\n c.nvidiaSmi=CommandExists(L"nvidia-smi.exe")||Exists(appDir+L"\\\\tools\\\\nvidia-smi.exe");\n c.smartctl=CommandExists(L"smartctl.exe")||Exists(appDir+L"\\\\tools\\\\smartctl.exe");\n',
    ' c.powershell=CommandExists(L"powershell.exe");\n const auto nvidiaTrust=VerifyEngine(appDir,L"tools\\\\nvidia-smi.exe",L"nvidia_smi");\n const auto smartctlTrust=VerifyEngine(appDir,L"tools\\\\smartctl.exe",L"smartctl");\n c.nvidiaSmi=nvidiaTrust.hashMatches;\n c.smartctl=smartctlTrust.hashMatches;\n',
)

# Execution must use the same reviewed bundled path that capability discovery verified.
replace_once(
    "src/engines.cpp",
    ' std::wstring exe=Exists(dir+L"\\\\tools\\\\smartctl.exe")?L"\\\""+dir+L"\\\\tools\\\\smartctl.exe\\\"":L"smartctl.exe";\n',
    ' std::wstring exe=L"\\\""+dir+L"\\\\tools\\\\smartctl.exe\\\"";\n',
)
replace_once(
    "src/engines.cpp",
    ' std::wstring exe=Exists(dir+L"\\\\tools\\\\nvidia-smi.exe")?L"\\\""+dir+L"\\\\tools\\\\nvidia-smi.exe\\\"":L"nvidia-smi.exe";\n',
    ' std::wstring exe=L"\\\""+dir+L"\\\\tools\\\\nvidia-smi.exe\\\"";\n',
)

# All report-opening command routes use the same trusted session artifact policy.
replace_once(
    "src/main.cpp",
    '        else if (id == 2 && !gReportPath.empty()) ShellExecuteW(h, L"open", gReportPath.c_str(), nullptr, nullptr, SW_SHOW);\n',
    '        else if (id == 2) {\n            if (!gReportPath.empty() && IsTrustedSessionArtifactPath(gReportPath)) ShellExecuteW(h, L"open", gReportPath.c_str(), nullptr, nullptr, SW_SHOW);\n            else if (!gReportPath.empty()) MessageBoxW(h, L"Đường dẫn báo cáo không vượt qua kiểm tra vùng tin cậy.", L"LapSure", MB_OK | MB_ICONERROR);\n            return 0;\n        }\n',
)
replace_once(
    "src/main.cpp",
    '            else if (!gReportPath.empty()) ShellExecuteW(h, L"open", gReportPath.c_str(), nullptr, nullptr, SW_SHOW);\n',
    '            else if (!gReportPath.empty() && IsTrustedSessionArtifactPath(gReportPath)) ShellExecuteW(h, L"open", gReportPath.c_str(), nullptr, nullptr, SW_SHOW);\n            else if (!gReportPath.empty()) MessageBoxW(h, L"Đường dẫn báo cáo không vượt qua kiểm tra vùng tin cậy.", L"LapSure", MB_OK | MB_ICONERROR);\n',
)

# Manifest identity must match the current product version used by CMake/resources.
replace_once("app.manifest", 'assemblyIdentity version="0.1.0.0"', 'assemblyIdentity version="0.1.1.0"')

manifest = ROOT / "tools" / "engine_manifest.txt"
m = manifest.read_text(encoding="utf-8")
if "smartctl=" not in m:
    m += "smartctl=\n"
if "nvidia_smi=" not in m:
    m += "nvidia_smi=\n"
manifest.write_text(m, encoding="utf-8")

security = '''# Security Policy

LapSure executes low-level diagnostic workflows and may invoke optional external diagnostic engines. Reports can contain model, serial/service-tag and hardware-identifying information and should be handled accordingly.

## External engine policy
- No silent runtime downloads.
- Diagnostic engines are executed only from reviewed bundled paths under the LapSure application directory.
- Reviewed binaries must be pinned by SHA-256 in `tools/engine_manifest.txt`.
- PATH-discovered `smartctl.exe`, `nvidia-smi.exe`, or similarly named binaries are not trusted merely because they exist.
- A missing allowlist entry, empty/unconfigured hash, missing file, or hash mismatch blocks that external engine from execution.
- Missing providers remain NOT TESTED / UNSUPPORTED / INCOMPLETE as appropriate; missing tooling never becomes hardware PASS.
- System/vendor binaries may be supported later only through an explicit trust policy (for example, validated Authenticode publisher/path rules), not implicit PATH lookup.

## Report and persistence boundary
- Report/history paths are treated as untrusted persisted input when reopened or deleted.
- Open/delete operations must canonicalize and remain inside the configured report/history root with an allowed artifact extension.
- LapSure does not modify Windows TrustedPublisher or other trust stores at runtime.

## Privilege model
The current beta executable requests administrator elevation because some diagnostic providers require privileged access. This is a known broad privilege boundary. Production hardening must continue to minimize privileged operations and should move toward a standard-user UI plus a narrowly scoped privileged helper if practical without weakening diagnostic evidence.

## Reporting a vulnerability
Avoid posting sensitive device identifiers or exploit details in a public issue. Contact the repository owner privately through an appropriate GitHub contact channel, then provide a minimal reproducible description and affected version/commit.

## Release status
The project is beta and must not yet be treated as a hardened security product or forensic-certification tool.
'''
(ROOT / "SECURITY.md").write_text(security, encoding="utf-8")

test = r'''from pathlib import Path

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
'''
(ROOT / "tests" / "production_security_hardening_sanity.py").write_text(test, encoding="utf-8")

run = (ROOT / "run_source_tests.cmd").read_text(encoding="utf-8")
if "production_security_hardening_sanity.py" not in run:
    if not run.endswith("\n"):
        run += "\n"
    run += "python tests\\production_security_hardening_sanity.py\nif errorlevel 1 exit /b 1\n"
(ROOT / "run_source_tests.cmd").write_text(run, encoding="utf-8")
