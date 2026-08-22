from pathlib import Path
R=Path(__file__).resolve().parents[1];m=(R/"include/lap/model.h").read_text();c=(R/"src/chassis_profile.cpp").read_text();a=(R/"src/main.cpp").read_text();o=(R/"src/orchestrator.cpp").read_text();r=(R/"src/report.cpp").read_text();cm=(R/"CMakeLists.txt").read_text();p=list((R/"profiles/chassis").glob("*.profile"))
checks=[("Typed","struct ChassisProfile" in m),("Loader","directory_iterator" in c),("Audit loads profile","LoadChassisProfile(gDir,report.model)" in a),("Apply result","ApplyPortResultToChassisProfile" in a),("Guided selector","SelectNextChassisPort" in a[a.find("id==1300"):]),("Required count","RequiredPortsRemaining" in o),("Report","Model-aware Chassis / Port Map" in r),("3 profiles",len(p)>=3),("5560",any("5560" in x.name for x in p)),("5570",any("5570" in x.name for x in p)),("7670",any("7670" in x.name for x in p)),("Compiled","src/chassis_profile.cpp" in cm and "src/port_selector.cpp" in cm)]
bad=[]
for n,x in checks: print(("PASS" if x else "FAIL"),n);bad+=[] if x else [n]
print(f"{len(checks)-len(bad)}/{len(checks)} PASS");raise SystemExit(bool(bad))
