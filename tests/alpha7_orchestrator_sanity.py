from pathlib import Path
from app_source_view import read_app_source

R=Path(__file__).resolve().parents[1]
m=(R/"include/lap/model.h").read_text(encoding="utf-8")
o=(R/"src/orchestrator.cpp").read_text(encoding="utf-8")
a=read_app_source(R)
r=(R/"src/report.cpp").read_text(encoding="utf-8")
c=(R/"CMakeLists.txt").read_text(encoding="utf-8")
checks=[
("Typed states","enum class TestStageState" in m),
("Summary","struct OrchestratorSummary" in m),
("4 stages",all(x in o for x in ['L"automatic"','L"functional"','L"ports"','L"decision"'])),
("Locks", "!ready?TestStageState::Locked" in o),
("Functional fail","f.failed?TestStageState::Failed" in o),
("Port fail",'p.overall==L"FAIL"' in o),
("Progress","100ull*u/t" in o),
("Next action","nextAction" in o),
("Next button","TIẾP TỤC BƯỚC KẾ" in a),
("Next enabled only from audit readiness","const BOOL enabled = gAuditReady ? TRUE : FALSE" in a and "EnableWindow(gNext, enabled)" in a),
("Functional route","RunFunctionalIoWizard" in a[a.find("if (id == 1300)"):]),
("Port route","RunPhysicalPortProbe" in a[a.find("if (id == 1300)"):]),
("Final report route","OpenCurrentReport" in a[a.find("if (id == 1300)"):]),
("Report progress","Quy trình kiểm tra có hướng dẫn" in r),
("Compiled","src/orchestrator.cpp" in c),
]
bad=[]
for n,x in checks:
 print(("PASS" if x else "FAIL"),n)
 if not x: bad.append(n)
print(f"{len(checks)-len(bad)}/{len(checks)} PASS")
raise SystemExit(bool(bad))
