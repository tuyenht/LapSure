from pathlib import Path
from app_source_view import read_app_source

R=Path(__file__).resolve().parents[1]
m=(R/"include/lap/model.h").read_text(encoding="utf-8")
main=read_app_source(R)
fn=(R/"src/functional.cpp").read_text(encoding="utf-8")
forensics=(R/"src/forensics.cpp").read_text(encoding="utf-8")
checks=[
("Functional type defined before StressSession",m.find("struct FunctionalTestSummary") < m.find("struct StressSession")),
("BIOS has smbiosVersion","smbiosVersion" in m and "x.smbiosVersion" in forensics),
("No undefined RefreshList call","RefreshList(" not in main),
("Manual updates protected by mutex","void UpsertFunctional" in main and "std::lock_guard<std::mutex>" in main[main.find("void UpsertFunctional"):main.find("bool CanRunManualTest")]),
("Manual tests blocked during audit","gRunning" in main[main.find("bool CanRunManualTest"):main.find("void CommitManualResult")]),
("Manual tests require audit snapshot","!gAuditReady" in main),
("Manual result rebuilds decision","BuildAuditDecision(gReport)" in main),
("Manual result republishes transactionally","RebuildDecisionAndReports" in main and "PublishReportSnapshot(snapshot)" in main and "PublishReportBundle" in main),
("Publication failure does not rewrite hardware decision","MarkReportPersistenceIncomplete" not in main and 'decision.overall = L"INCOMPLETE"' not in main),
("Functional buttons disabled initially","SetFunctionalButtonsEnabled(FALSE)" in main),
("Functional buttons enabled from audit readiness","const BOOL enabled = gAuditReady ? TRUE : FALSE" in main and "SetFunctionalButtonsEnabled(enabled)" in main),
("Color wnd proc forwards LPARAM","DefWindowProcW(h,msg,w,l)" in fn),
]
bad=[]
for n,ok in checks:
    print(("PASS" if ok else "FAIL"),n)
    if not ok: bad.append(n)
print(f"{len(checks)-len(bad)}/{len(checks)} PASS")
raise SystemExit(bool(bad))
