from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
window = (root / "src" / "app_window.ipp").read_text(encoding="utf-8")
runtime = (root / "src" / "app_runtime_state.ipp").read_text(encoding="utf-8")
orchestrator = (root / "src" / "orchestrator.cpp").read_text(encoding="utf-8")
chassis = (root / "src" / "chassis_profile.cpp").read_text(encoding="utf-8")

failures = []

def require(condition: bool, message: str) -> None:
    if not condition:
        failures.append(message)

require(
    "case MainTab::PortsPower:\n                PostMessageW(hwnd, WM_COMMAND, 1300, 0);" in window,
    "Ports & Power primary keyboard action must route through the guided stable-ID command",
)
require(
    "RequiredPortsRemaining(snapshot.hardware.stress.portAttestation)" in window,
    "guided continuation must use SessionPortAttestation remaining required ports",
)
require(
    "snapshot.hardware.stress.portPower.overall != L\"PASS\"" not in window,
    "guided continuation must not stop solely because PortPowerSummary.overall is PASS",
)
require(
    "#include \"lap/port_attestation.h\"" in orchestrator,
    "orchestrator must include session attestation authority",
)
require(
    "RequiredPortsRemaining(r.hardware.stress.portAttestation)" in orchestrator,
    "orchestrator required-port progress must come from SessionPortAttestation",
)
require(
    "RequiredPortsRemaining(r.hardware.stress.chassisProfile)" not in orchestrator,
    "orchestrator must stop consuming mutable chassis tested mirror for progress authority",
)
require(
    "r.expectedPortId" in chassis and "x.id==r.expectedPortId" in chassis.replace(" ", ""),
    "chassis compatibility mirror must prefer stable expectedPortId",
)
require(
    "result.expectedPortId.empty() &&" in runtime and
    "port.portLabel == result.portLabel" in runtime,
    "portPower compatibility upsert may use label fallback only when expectedPortId is absent",
)

if failures:
    for failure in failures:
        print(f"FAIL {failure}")
    sys.exit(1)

print("PASS Task 4.1 production-route source invariants")
