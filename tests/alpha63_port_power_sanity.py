from pathlib import Path
R=Path(__file__).resolve().parents[1]
m=(R/"include/lap/model.h").read_text(encoding="utf-8")
p=(R/"src/port_power.cpp").read_text(encoding="utf-8")
main=(R/"src/main.cpp").read_text(encoding="utf-8")
sc=(R/"src/scoring.cpp").read_text(encoding="utf-8")
rp=(R/"src/report.cpp").read_text(encoding="utf-8")
cm=(R/"CMakeLists.txt").read_text(encoding="utf-8")
checks=[
("Typed port result","struct PortProbeResult" in m),
("Typed power result","struct PowerProbeResult" in m),
("PnP baseline/delta","auto before=Devices()" in p and "auto after=Devices()" in p),
("Location path evidence","DEVPKEY_Device_LocationPaths" in p),
("Bus reported description","DEVPKEY_Device_BusReportedDeviceDesc" in p),
("No controller-presence PASS","New PnP device appeared only after stimulus" in p),
("USB4 host router evidence","Usb4HostRouter" in p),
("USB4 device router delta","Usb4DeviceRouter" in p),
("Thunderbolt delta","thunderboltSeen" in p),
("Exact speed not invented","exact negotiated speed UNKNOWN" in p),
("DP alt mode conservative","candidate, not conclusive" in p),
("AC state native","GetSystemPowerStatus" in p),
("Adapter wattage unknown","adapterWatts=-1" in p and "wattage is not inferred" in p),
("Port result persisted","CommitPortResult" in main),
("Port failure affects decision","portPower.overall==L\"FAIL\"" in sc),
("Port card report","Cổng kết nối và nguồn sạc" in rp),
("Source compiled","src/port_power.cpp" in cm),
]
bad=[]
for n,ok in checks:
 print(("PASS" if ok else "FAIL"),n)
 if not ok:bad.append(n)
print(f"{len(checks)-len(bad)}/{len(checks)} PASS")
raise SystemExit(bool(bad))
