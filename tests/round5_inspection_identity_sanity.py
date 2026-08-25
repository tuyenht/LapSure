from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INVENTORY = (ROOT / "src/inventory.cpp").read_text(encoding="utf-8")
STRESS = (ROOT / "src/stress.cpp").read_text(encoding="utf-8")
REPORT = (ROOT / "src/report.cpp").read_text(encoding="utf-8")
HISTORY = (ROOT / "src/session_history.cpp").read_text(encoding="utf-8")
JOURNAL_H = (ROOT / "include/lap/journal.h").read_text(encoding="utf-8")
JOURNAL_CPP = (ROOT / "src/journal.cpp").read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


# One inspection identity must exist before the inventory provider starts emitting evidence.
require("CreateInspectionId" in INVENTORY, "CollectInventory must create/retain an inspection identity")
collect_pos = INVENTORY.find("AuditReport CollectInventory")
id_pos = INVENTORY.find("CreateInspectionId", collect_pos)
provider_pos = INVENTORY.find("Add(", collect_pos)
require(collect_pos >= 0 and id_pos > collect_pos and provider_pos > id_pos,
        "inspection identity must be established before inventory evidence is emitted")

# Stress may create a compatibility fallback only when earlier inventory identity is absent.
compact_stress = STRESS.replace(" ", "").replace("\n", "")
require("if(ss.sessionId.empty())ss.sessionId=SessionId();" in compact_stress,
        "RunStressSession must preserve an existing inspection identity")

# Reports and history use the same identity, including inventory-only workflows.
require("ReportStem" in REPORT and "stress.sessionId" in REPORT,
        "report filenames must be derived from the persisted inspection identity")
require("\\\"sessionId\\\"" in REPORT,
        "JSON report must publish the same inspection/session identity")
require("report.hardware.stress.sessionId" in HISTORY,
        "history must key entries by the report inspection identity")

# Journal root terminology is explicit: it is a persistent state root, not implicitly an app install root.
require("stateRoot" in JOURNAL_H and "stateRoot" in JOURNAL_CPP,
        "journal APIs must use an explicit persistent state root contract")
require("std::filesystem::path(stateRoot)" in JOURNAL_CPP,
        "journal path must be rooted directly in the supplied persistent state root")

print("Round 5 inspection identity/state-root contract: PASS")
