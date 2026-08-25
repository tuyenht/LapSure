from pathlib import Path
R=Path(__file__).resolve().parents[1]
f={p.name:p.read_text(encoding="utf-8") for p in [R/"src/forensics.cpp",R/"src/edid.cpp",R/"src/process.cpp",R/"CMakeLists.txt"]}
checks=[
("Native EDID provider", "SetupDiOpenDevRegKey" in f["edid.cpp"] and 'L"EDID"' in f["edid.cpp"]),
("EDID header validation", "hdr[8]" in f["edid.cpp"]),
("EDID checksum validation", "(sum&0xff)!=0" in f["edid.cpp"]),
("EDID detailed timing parse", "nativeWidth" in f["edid.cpp"] and "nativeHeight" in f["edid.cpp"]),
("EDID monitor name descriptor", "0xfc" in f["edid.cpp"]),
("Display factory compares native resolution", "d.nativeWidth==p.displayWidth" in f["forensics.cpp"]),
("Current resolution not used as native proof", "Native SetupAPI registry EDID" in f["forensics.cpp"]),
("Touch separated from EDID", "Touch digitizer" in f["forensics.cpp"]),
("BIOS producer has four parser fields", "'{0}|{1}|{2}|{3}.{4}'" in f["forensics.cpp"]),
("No PowerShell hard gate before EDID", f["forensics.cpp"].find("CollectNativeDisplays") < f["forensics.cpp"].find("if(!c.powershell)")),
("Historical events labeled evidence", "Historical evidence only" in f["forensics.cpp"]),
("Process output capture capped", "kMaxCapture" in f["process.cpp"]),
("Wait failure handled", "WAIT_FAILED" in f["process.cpp"]),
("EDID source compiled", "src/edid.cpp" in f["CMakeLists.txt"]),
]
bad=[]
for n,ok in checks:
 print(("PASS" if ok else "FAIL"),n)
 if not ok: bad.append(n)
print(f"{len(checks)-len(bad)}/{len(checks)} PASS")
raise SystemExit(bool(bad))
