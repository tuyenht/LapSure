from pathlib import Path
R=Path(__file__).resolve().parents[1]
cm=(R/'CMakeLists.txt').read_text(encoding='utf-8')
manifest=(R/'app.manifest').read_text(encoding='utf-8')
pack=(R/'package_portable.ps1').read_text(encoding='utf-8')
checks=[
('LapSure CMake project','project(LapSure' in cm),
('Beta 0.1.1 CMake version','project(LapSure VERSION 0.1.1' in cm),
('LapSure executable target','add_executable(LapSure' in cm),
('LapSure install target','install(TARGETS LapSure' in cm),
('LapSure manifest identity','name="LapSure"' in manifest),
('LapSure portable exe','LapSure.exe' in pack),
('Full source tree',len(list((R/'src').glob('*.cpp')))>=20 and len(list((R/'include/lap').glob('*.h')))>=20),
('No legacy product target','LaptopAuditPro' not in cm and 'LaptopAuditPro.exe' not in pack),
('Chassis profiles present',len(list((R/'profiles/chassis').glob('*.profile')))>=3),
('Validation kit present',(R/'validation/REAL_MACHINE_MATRIX.tsv').exists()),
]
bad=[]
for n,ok in checks:
    print(('PASS' if ok else 'FAIL'),n)
    if not ok: bad.append(n)
print(f'{len(checks)-len(bad)}/{len(checks)} PASS')
raise SystemExit(bool(bad))
