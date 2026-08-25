from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT_H = (ROOT / "include/lap/report.h").read_text(encoding="utf-8")
REPORT_CPP = (ROOT / "src/report.cpp").read_text(encoding="utf-8")
MAIN = (ROOT / "src/main.cpp").read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


require("ReportPublicationStatus" in REPORT_H,
        "report API must expose publication status separately from AuditDecision")
require("ReportPublicationResult" in REPORT_H and "PublishReportBundle" in REPORT_H,
        "report API must expose one bundle publication operation")
require("PublishReportBundle" in REPORT_CPP,
        "report implementation must own bundle publication")
require(".staging-" in REPORT_CPP or "staging" in REPORT_CPP.lower(),
        "bundle publication must use a staging boundary")
require("MoveFileExW" in REPORT_CPP,
        "bundle publication must use an explicit final publication move")

require("MarkReportPersistenceIncomplete" not in MAIN,
        "file I/O failure must not rewrite hardware AuditDecision")
require("PublishReportBundle" in MAIN,
        "interactive and automatic report persistence must use bundle publication")
require('decision.overall = L"INCOMPLETE"' not in MAIN,
        "publication failure must not assign INCOMPLETE into the hardware decision")

print("Round 5 report publication/decision separation contract: PASS")
