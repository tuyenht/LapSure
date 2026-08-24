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
    ("trust exposes canonical resolved path", "resolvedPath" in TRUST_H and "resolvedPath" in TRUST_CPP),
    ("trusted execution boundary declared", "RunTrustedEngineCapture" in TRUST_H and "RunTrustedEngineCapture" in TRUST_CPP),
    ("trusted execution calls explicit process API", "RunProcessCaptureExecutable" in TRUST_CPP),
    ("smartctl uses trusted execution boundary", "RunTrustedEngineCapture" in ENGINES and "smartctl" in ENGINES),
    ("nvidia telemetry uses trusted execution boundary", "RunTrustedEngineCapture" in ENGINES and "nvidia_smi" in ENGINES),
    ("VRAM engine uses trusted execution boundary", "RunTrustedEngineCapture" in STRESS and "memtest_vulkan" in STRESS),
    ("sensor bridge uses trusted execution boundary", "RunTrustedEngineCapture" in SENSORS and "lhm_bridge" in SENSORS),
    ("telemetry nvidia uses trusted execution boundary", "RunTrustedEngineCapture" in TELEMETRY and "nvidia_smi" in TELEMETRY),
    ("process/trust behavioral test compiled", "LapSureProcessSecurityTests" in CMAKE and "tests/process_security_tests.cpp" in CMAKE),
]

bad = []
for name, ok in checks:
    print(("PASS" if ok else "FAIL"), name)
    if not ok:
        bad.append(name)
print(f"{len(checks)-len(bad)}/{len(checks)} PASS")
raise SystemExit(bool(bad))
