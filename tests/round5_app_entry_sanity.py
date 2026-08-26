from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
ENTRY = ROOT / "src/main_round5.cpp"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


# Round 5 must be the production entrypoint before publication closure can be green.
require(ENTRY.exists(), "Round 5 production entrypoint is missing")
require("src/main_round5.cpp" in CMAKE, "LapSure target must compile the Round 5 production entrypoint")

parts = [ENTRY]
for name in ["app_runtime_state.ipp", "app_audit.ipp", "app_window.ipp", "app_entry.ipp"]:
    path = ROOT / "src" / name
    require(path.exists(), f"production app fragment missing: {name}")
    parts.append(path)
APP = "\n".join(path.read_text(encoding="utf-8") for path in parts)

require("PublishReportBundle" in APP, "runtime must publish reports through the transactional bundle API")
require("MarkReportPersistenceIncomplete" not in APP,
        "publication failure must not rewrite hardware decision")
require("PersistReportBundle" not in APP,
        "legacy sequential report persistence must not remain in production runtime")
require('decision.overall = L"INCOMPLETE"' not in APP,
        "publication failure must not assign INCOMPLETE to the hardware decision")
require("gPublicationReady" in APP,
        "runtime must track report publication readiness separately from audit readiness")
require("gAuditReady = !gCancel" in APP,
        "hardware/evidence readiness must not depend on report filesystem publication")

# Journal/recovery truth is rooted in the explicit persistent state root.
for token in [
    "CompleteStressJournal(gReportOutputDir)",
    "DiscardInterruptedStressJournal(gReportOutputDir)",
    "ReadInterruptedStressJournal(gReportOutputDir)",
]:
    require(token in APP, f"runtime must use persistent report/state root: {token}")

print("Round 5 production entry/publication integration contract: PASS")
