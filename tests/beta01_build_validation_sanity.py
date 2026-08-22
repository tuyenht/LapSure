from pathlib import Path
R=Path(__file__).resolve().parents[1]
m=(R/"include/lap/model.h").read_text(encoding="utf-8")
rv=(R/"src/runtime_validation.cpp").read_text(encoding="utf-8")
main=(R/"src/main.cpp").read_text(encoding="utf-8")
rep=(R/"src/report.cpp").read_text(encoding="utf-8")
cm=(R/"CMakeLists.txt").read_text(encoding="utf-8")
pre=(R/"CMakePresets.json").read_text(encoding="utf-8")
wf=(R/".github/workflows/windows-msvc-build.yml").read_text(encoding="utf-8")
checks=[
("Runtime validation model","struct RuntimeValidationSummary" in m),
("Runtime validation implementation","RunRuntimeValidation" in rv),
("MSVC identity","_MSC_VER" in rv),
("Optional provider remains warning","Optional trusted provider not configured" in rv),
("Runtime gate wired after stress","RunRuntimeValidation(report,caps,gDir)" in main),
("Runtime gate reported","Runtime Validation Gate" in rep),
("MSVC /W4","/W4" in cm),
("MSVC permissive off","/permissive-" in cm),
("Strict /WX option","/WX" in cm),
("Release preset","msvc-x64-release" in pre),
("CI preset","msvc-x64-ci" in pre),
("Windows CI","runs-on: windows-2022" in wf),
("Portable packager",(R/"package_portable.ps1").exists()),
("Validation matrix",(R/"validation/REAL_MACHINE_MATRIX.tsv").exists()),
("Validation checklist",(R/"validation/VALIDATION_CHECKLIST.md").exists()),
("Runtime source compiled","src/runtime_validation.cpp" in cm),
]
bad=[]
for n,ok in checks:
 print(("PASS" if ok else "FAIL"),n)
 if not ok: bad.append(n)
print(f"{len(checks)-len(bad)}/{len(checks)} PASS")
raise SystemExit(bool(bad))
