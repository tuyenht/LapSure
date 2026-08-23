from pathlib import Path
R=Path(__file__).resolve().parents[1]
m=(R/"include/lap/model.h").read_text(encoding="utf-8")
rv=(R/"src/runtime_validation.cpp").read_text(encoding="utf-8")
main=(R/"src/main.cpp").read_text(encoding="utf-8")
rep=(R/"src/report.cpp").read_text(encoding="utf-8")
cm=(R/"CMakeLists.txt").read_text(encoding="utf-8")
pre=(R/"CMakePresets.json").read_text(encoding="utf-8")
wf=(R/".github/workflows/windows-msvc-build.yml").read_text(encoding="utf-8")
pack=(R/"package_portable.ps1").read_text(encoding="utf-8")
verify=(R/"validation/verify_portable_package.ps1").read_text(encoding="utf-8")
checks=[
("Runtime validation model","struct RuntimeValidationSummary" in m),
("Runtime validation implementation","RunRuntimeValidation" in rv),
("MSVC identity","_MSC_VER" in rv),
("Optional provider remains warning","Optional trusted provider not configured" in rv),
("Runtime gate wired after stress","RunRuntimeValidation(report,caps,gDir)" in main),
("Runtime gate reported","Xác thực chương trình và báo cáo" in rep),
("MSVC /W4","/W4" in cm),
("MSVC permissive off","/permissive-" in cm),
("Strict /WX option","/WX" in cm),
("No global warning suppression","/wd" not in cm),
("Release preset","msvc-x64-release" in pre),
("CI preset","msvc-x64-ci" in pre),
("Windows CI","runs-on: windows-2022" in wf),
("Full regression suite in CI","run_source_tests.cmd" in wf),
("Portable archive with checksum","Compress-Archive" in pack and ".zip.sha256" in pack),
("Portable docs and validation kit",'@("docs","validation")' in pack),
("Portable integrity verified in CI","verify_portable_package.ps1" in wf),
("ZIP and EXE hashes verified","ZIP SHA-256 mismatch" in verify and "LapSure.exe SHA-256 mismatch" in verify),
("Build provenance verified","Missing or invalid commit provenance" in verify),
("PR head provenance selected","github.event.pull_request.head.sha || github.sha" in wf and "SOURCE_COMMIT" in pack),
("Expected commit enforced","Commit provenance mismatch" in verify and '-ExpectedCommit "$env:SOURCE_COMMIT"' in wf),
("Inventory-only CLI declared","--inventory-only" in main and "RunInventoryOnly" in main),
("Inventory-only excludes stress","inventory_only_begin" in main and "inventory_only_end" in main and "RunStressSession" not in main[main.index("inventory_only_begin"):main.index("inventory_only_end")]),
("Inventory-only exercised in CI","Inventory-only provider preflight" in wf and "--inventory-only" in wf),
("Validation matrix",(R/"validation/REAL_MACHINE_MATRIX.tsv").exists()),
("Validation checklist",(R/"validation/VALIDATION_CHECKLIST.md").exists()),
("Pilot runbook",(R/"validation/PILOT_RUNBOOK.md").exists()),
("Session and discrepancy templates",(R/"validation/SESSION_RECORD_TEMPLATE.md").exists() and (R/"validation/DISCREPANCY_LOG_TEMPLATE.tsv").exists()),
("Runtime source compiled","src/runtime_validation.cpp" in cm),
]
bad=[]
for n,ok in checks:
 print(("PASS" if ok else "FAIL"),n)
 if not ok: bad.append(n)
print(f"{len(checks)-len(bad)}/{len(checks)} PASS")
raise SystemExit(bool(bad))
