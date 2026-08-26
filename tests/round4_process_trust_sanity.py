from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROCESS_H = (ROOT / "include/lap/process.h").read_text(encoding="utf-8")
PROCESS_CPP = (ROOT / "src/process.cpp").read_text(encoding="utf-8")
TRUST_H = (ROOT / "include/lap/trust.h").read_text(encoding="utf-8")
TRUST_CPP = (ROOT / "src/trust.cpp").read_text(encoding="utf-8")
ENGINES = (ROOT / "src/engines.cpp").read_text(encoding="utf-8")
STRESS = (ROOT / "src/stress.cpp").read_text(encoding="utf-8")
SENSORS = (ROOT / "src/sensors.cpp").read_text(encoding="utf-8")
TELEMETRY = (ROOT / "src/telemetry.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

checks = [
    ("explicit executable API declared", "RunProcessCaptureExecutable" in PROCESS_H and "std::vector<std::wstring>" in PROCESS_H),
    ("explicit lpApplicationName used", "CreateProcessW(executablePath.c_str()" in PROCESS_CPP),
    ("null lpApplicationName removed", "CreateProcessW(nullptr" not in PROCESS_CPP),
    ("legacy wrapper parses argv", "CommandLineToArgvW" in PROCESS_CPP and "RunProcessCaptureExecutable" in PROCESS_CPP),
    ("Windows argv quoting helper present", "QuoteWindowsArgument" in PROCESS_CPP),
    ("bare PowerShell prefers System32 executable", "GetSystemDirectoryW" in PROCESS_CPP and "WindowsPowerShell" in PROCESS_CPP),
    ("trust exposes canonical resolved path", "resolvedPath" in TRUST_H and "resolvedPath" in TRUST_CPP),
    ("trusted execution boundary declared", "RunTrustedEngineCapture" in TRUST_H and "RunTrustedEngineCapture" in TRUST_CPP),
    ("trusted execution calls explicit process API", "RunProcessCaptureExecutable" in TRUST_CPP),
    ("smartctl uses trusted execution boundary", "RunTrustedEngineCapture" in ENGINES and "L\"smartctl\"" in ENGINES),
    ("smartctl no longer constructs quoted executable command", 'std::wstring exe=L"\\\""+dir+L"\\\\tools\\\\smartctl.exe\\\""' not in ENGINES),
    ("nvidia telemetry uses trusted execution boundary", "RunTrustedEngineCapture" in ENGINES and "L\"nvidia_smi\"" in ENGINES),
    ("nvidia no longer constructs quoted executable command", 'std::wstring exe=L"\\\""+dir+L"\\\\tools\\\\nvidia-smi.exe\\\""' not in ENGINES),
    ("VRAM engine uses trusted execution boundary", "RunTrustedEngineCapture" in STRESS and "memtest_vulkan" in STRESS and "RunProcessCapture(Q(exe)" not in STRESS),
    ("sensor bridge uses trusted execution boundary", "RunTrustedEngineCapture" in SENSORS and "lhm_bridge" in SENSORS and "RunProcessCapture(L\"\\\\\"\"+exe" not in SENSORS),
    ("telemetry nvidia uses trusted execution boundary", "RunTrustedEngineCapture" in TELEMETRY and "nvidia_smi" in TELEMETRY and 'exe=L"nvidia-smi.exe"' not in TELEMETRY),
    ("process/trust behavioral test compiled", "LapSureProcessSecurityTests" in CMAKE and "tests/process_security_tests.cpp" in CMAKE),
    ("embedded provider catalog declared", "GetEmbeddedProviderCatalog" in TRUST_H and "GetEmbeddedProviderCatalog" in TRUST_CPP),
    ("TOCTOU handle locking present", "FILE_SHARE_READ" in TRUST_CPP and "HashHandle" in TRUST_CPP),
    ("provider output contract validated", "ValidateSmartctlScanOutput" in ENGINES and "ValidateSensorBridgeOutput" in SENSORS),
]

bad = []
for name, ok in checks:
    print(("PASS" if ok else "FAIL"), name)
    if not ok:
        bad.append(name)
print(f"{len(checks)-len(bad)}/{len(checks)} PASS")
raise SystemExit(bool(bad))
