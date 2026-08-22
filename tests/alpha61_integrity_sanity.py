from pathlib import Path
R=Path(__file__).resolve().parents[1]
m=(R/"include/lap/model.h").read_text(encoding="utf-8")
main=(R/"src/main.cpp").read_text(encoding="utf-8")
fn=(R/"src/functional.cpp").read_text(encoding="utf-8")
forensics=(R/"src/forensics.cpp").read_text(encoding="utf-8")
checks=[
("Functional type defined before StressSession",m.find("struct FunctionalTestSummary") < m.find("struct StressSession")),
("BIOS has smbiosVersion","smbiosVersion" in m and "x.smbiosVersion" in forensics),
("No undefined RefreshList call","RefreshList(" not in main),
("Manual updates protected by mutex","void UpsertFunctional" in main and "lock_guard<std::mutex>" in main[main.find("void UpsertFunctional"):main.find("bool CanRunManualTest")]),
("Manual tests blocked during audit","if(gRunning)" in main[main.find("bool CanRunManualTest"):main.find("void CommitManualResult")]),
("Manual tests require audit snapshot","!gAuditReady" in main),
("Manual result rebuilds decision","BuildAuditDecision(gReport)" in main),
("Manual result rewrites reports","SaveHtmlReport(gReport" in main and "SaveJsonReport(gReport" in main),
("Functional buttons disabled initially","EnableWindow(gFuncDisplay,FALSE)" in main),
("Functional buttons enabled after successful audit","SetFunctionalButtonsEnabled(w?FALSE:TRUE)" in main),
("Color wnd proc forwards LPARAM","DefWindowProcW(h,msg,w,l)" in fn),
]
bad=[]
for n,ok in checks:
    print(("PASS" if ok else "FAIL"),n)
    if not ok: bad.append(n)
print(f"{len(checks)-len(bad)}/{len(checks)} PASS")
raise SystemExit(bool(bad))
