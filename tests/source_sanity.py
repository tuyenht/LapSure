from pathlib import Path
import json, sys
ROOT=Path(__file__).resolve().parents[1]
checks=[]
def check(name, cond):
    checks.append((name,bool(cond)))

inv=(ROOT/'src/inventory.cpp').read_text(encoding='utf-8')
eng=(ROOT/'src/engines.cpp').read_text(encoding='utf-8')
proc=(ROOT/'src/process.cpp').read_text(encoding='utf-8')
rep=(ROOT/'src/report.cpp').read_text(encoding='utf-8')
cm=(ROOT/'CMakeLists.txt').read_text(encoding='utf-8')
main=(ROOT/'src/main.cpp').read_text(encoding='utf-8')
fore=(ROOT/'src/forensics.cpp').read_text(encoding='utf-8')
profile=json.loads((ROOT/'profiles/Dell_Precision_5560_3ZJC6M3.json').read_text(encoding='utf-8'))

check('No broken PowerShell %%{ alias', '%%{' not in inv)
check('Battery CIM uses Namespace', '-Namespace root/wmi -ClassName BatteryStaticData' in inv)
check('smartctl does not hardcode /dev/nvme0', '/dev/nvme0' not in eng)
check('Process uses concurrent reader thread', 'std::thread reader' in proc)
check('Process has timeout kill', 'TerminateJobObject' in proc and 'TerminateProcess' in proc)
check('Dedicated JSON escape exists', 'std::wstring Json(' in rep)
check('Static MSVC CRT configured', 'CMAKE_MSVC_RUNTIME_LIBRARY' in cm)
check('UI audit uses worker thread', 'std::thread gWorker' in main)
check('Admin manifest included', 'app.manifest' in cm and (ROOT/'app.manifest').exists())
check('Exact profile Service Tag', profile.get('serviceTag')=='3ZJC6M3')
check('Factory profile current schema', profile.get('schemaVersion')==2 and profile.get('ramBytes')==34359738368)
check('Generic GPU inventory provider', 'Win32_VideoController' in inv)
check('Empty DIMM provider is not valid zero modules', 'Module details unavailable' in inv)
check('Service Tag BIOS fallback', 'Win32_BIOS' in inv and 'SerialNumber' in inv)
check('Kernel-Power evidence is unexpected restart only', 'Id=41' in fore)
check('Battery report XML fallback', 'powercfg.exe /batteryreport /xml' in inv and 'LapSureBattery-' in inv)
check('Battery fallback remains evidence gated', 'if(!bi.capacityReadable)' in inv and 'BatteryState(bi.healthPercent)' in inv)
check('Windows native storage reliability provider', 'Get-StorageReliabilityCounter' in eng and 'CollectWindowsStorageReliability' in eng)
check('Native storage runs before smartctl enrichment', 'CollectWindowsStorageReliability(report' in main and main.index('CollectWindowsStorageReliability(report') < main.index('CollectSmartctl(report'))
check('Native reliability reduces smartctl dependency', 'native reliability evidence remains available' in eng)

bad=[n for n,ok in checks if not ok]
for n,ok in checks: print(('PASS ' if ok else 'FAIL ')+n)
if bad:
    print(f'\n{len(bad)} sanity checks failed.', file=sys.stderr)
    raise SystemExit(1)
print(f'\nAll {len(checks)} source sanity checks passed.')
