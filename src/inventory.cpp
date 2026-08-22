#include "lap/inventory.h"
#include "lap/process.h"
#include "lap/hardware.h"
#include <windows.h>
#include <intrin.h>
#include <sstream>
#include <algorithm>
#include <cwctype>
#include <cmath>

namespace lap {
const wchar_t* ToString(State s){switch(s){case State::Pass:return L"PASS";case State::Good:return L"GOOD";case State::Warning:return L"WARNING";case State::Fail:return L"FAIL";case State::Changed:return L"CHANGED";case State::NotTested:return L"NOT TESTED";case State::Unsupported:return L"UNSUPPORTED";default:return L"INFO";}}
const wchar_t* ToString(Severity s){switch(s){case Severity::Critical:return L"CRITICAL";case Severity::Major:return L"MAJOR";case Severity::Minor:return L"MINOR";default:return L"INFO";}}
const wchar_t* ToString(Dimension d){switch(d){case Dimension::Identity:return L"IDENTITY";case Dimension::Factory:return L"FACTORY";case Dimension::Health:return L"HEALTH";case Dimension::Usage:return L"USAGE";case Dimension::Performance:return L"PERFORMANCE";case Dimension::Stability:return L"STABILITY";case Dimension::Functional:return L"FUNCTIONAL";default:return L"EVIDENCE";}}
static void Add(AuditReport&r,std::wstring g,std::wstring n,std::wstring v,std::wstring e,State st,Severity sv,Dimension d,std::wstring ev=L""){r.findings.push_back({std::move(g),std::move(n),std::move(v),std::move(e),st,sv,std::move(ev),d});}
static std::wstring CpuBrand(){int regs[4]{};char brand[49]{};for(int i=0;i<3;i++){__cpuid(regs,0x80000002+i);memcpy(brand+i*16,regs,16);}int n=MultiByteToWideChar(CP_ACP,0,brand,-1,nullptr,0);if(n<=1)return L"";std::wstring w((size_t)n,L'\0');MultiByteToWideChar(CP_ACP,0,brand,-1,w.data(),n);if(!w.empty()&&w.back()==L'\0')w.pop_back();return Trim(w);}
static std::wstring GetRegString(HKEY root,const wchar_t* sub,const wchar_t* name){HKEY h{};if(RegOpenKeyExW(root,sub,0,KEY_READ,&h)!=ERROR_SUCCESS)return L"";wchar_t b[512]{};DWORD sz=sizeof(b),t=0;std::wstring s;if(RegQueryValueExW(h,name,nullptr,&t,(LPBYTE)b,&sz)==ERROR_SUCCESS&&(t==REG_SZ||t==REG_EXPAND_SZ))s.assign(b);RegCloseKey(h);return Trim(s);}
static bool ContainsI(std::wstring a,std::wstring b){std::transform(a.begin(),a.end(),a.begin(),towlower);std::transform(b.begin(),b.end(),b.begin(),towlower);return a.find(b)!=std::wstring::npos;}
static double Gib(uint64_t b){return (double)b/(1024.0*1024.0*1024.0);}
static std::wstring F1(double x){wchar_t b[64];swprintf_s(b,L"%.1f",x);return b;}
static State BatteryState(double health){if(health<0)return State::NotTested;if(health>=85)return State::Pass;if(health>=80)return State::Good;if(health>=70)return State::Warning;return State::Fail;}

AuditReport CollectInventory(const FactoryProfile& p,const Capabilities& caps,const std::wstring&,const std::atomic_bool* cancel){
 AuditReport r{};r.environment=EnvironmentName(caps);
 r.model=GetRegString(HKEY_LOCAL_MACHINE,L"HARDWARE\\DESCRIPTION\\System\\BIOS",L"SystemProductName");
 r.serviceTag=GetRegString(HKEY_LOCAL_MACHINE,L"HARDWARE\\DESCRIPTION\\System\\BIOS",L"SystemSerialNumber");

 if(!p.model.empty())Add(r,L"Machine",L"Model",r.model,p.model,ContainsI(r.model,p.model)?State::Pass:State::Fail,Severity::Critical,Dimension::Factory);
 else Add(r,L"Machine",L"Model",r.model,L"Generic audit",State::Info,Severity::Info,Dimension::Identity);
 if(!p.serviceTag.empty())Add(r,L"Machine",L"Service Tag",r.serviceTag,p.serviceTag,r.serviceTag==p.serviceTag?State::Pass:State::Fail,Severity::Critical,Dimension::Factory);
 else Add(r,L"Machine",L"Service Tag",r.serviceTag,L"No exact factory profile",State::Info,Severity::Info,Dimension::Identity);

 r.hardware.cpuName=CpuBrand();
 SYSTEM_INFO si{};GetNativeSystemInfo(&si);r.hardware.cpuThreads=si.dwNumberOfProcessors;
 if(!p.cpuContains.empty())Add(r,L"CPU",L"Processor",r.hardware.cpuName,p.cpuContains,ContainsI(r.hardware.cpuName,p.cpuContains)?State::Pass:State::Fail,Severity::Critical,Dimension::Factory);
 else Add(r,L"CPU",L"Processor",r.hardware.cpuName,L"",State::Info,Severity::Info,Dimension::Identity);

 MEMORYSTATUSEX ms{sizeof(ms)};GlobalMemoryStatusEx(&ms);r.hardware.installedRamBytes=ms.ullTotalPhys;
 if(p.ramBytes){double expected=Gib(p.ramBytes),actual=Gib(ms.ullTotalPhys);auto delta=std::abs(actual-expected);State st=delta<1.5?State::Pass:State::Changed;Add(r,L"RAM",L"Installed total",F1(actual)+L" GiB",F1(expected)+L" GiB",st,Severity::Major,Dimension::Factory);}
 else Add(r,L"RAM",L"Installed total",F1(Gib(ms.ullTotalPhys))+L" GiB",L"",State::Info,Severity::Info,Dimension::Identity);

 SYSTEM_POWER_STATUS ps{};
 if(GetSystemPowerStatus(&ps)&&ps.BatteryFlag!=128){r.hardware.battery.present=true;r.hardware.battery.currentChargePercent=(int)ps.BatteryLifePercent;r.hardware.battery.status=(ps.ACLineStatus==1?L"AC connected":L"On battery");Add(r,L"Battery",L"Charge now",std::to_wstring((int)ps.BatteryLifePercent)+L"%",L"",State::Info,Severity::Info,Dimension::Usage);}
 else Add(r,L"Battery",L"Presence",L"Not detected/query failed",L"Battery expected",caps.winPE?State::Unsupported:State::NotTested,Severity::Major,Dimension::Functional);

 if(caps.powershell){
   auto ram=RunProcessCapture(L"powershell.exe -NoProfile -NonInteractive -Command \"Get-CimInstance Win32_PhysicalMemory | ForEach-Object { '{0}|{1}|{2}|{3}|{4}|{5}|{6}|{7}' -f $_.Capacity,$_.ConfiguredClockSpeed,$_.Speed,$_.Manufacturer,$_.PartNumber,$_.SerialNumber,$_.DeviceLocator,$_.BankLabel }\"",20000,cancel);
   if(ram.launched&&!ram.timedOut&&!ram.output.empty()){
      for(const auto& line:SplitLines(ram.output)){MemoryModule m{};if(ParseMemoryModuleLine(line,m))r.hardware.memoryModules.push_back(std::move(m));}
      std::wstringstream summary;summary<<r.hardware.memoryModules.size()<<L" module(s)";
      for(size_t i=0;i<r.hardware.memoryModules.size();++i){auto&m=r.hardware.memoryModules[i];summary<<L"\n#"<<i+1<<L": "<<F1(Gib(m.capacityBytes))<<L" GiB @ "<<m.configuredSpeed<<L" MT/s | "<<m.manufacturer<<L" | "<<m.partNumber<<L" | SN "<<m.serialNumber;}
      State st=State::Info;
      if(!r.hardware.memoryModules.empty()&&p.ramSpeed){bool ok=true;for(auto&m:r.hardware.memoryModules)if(m.configuredSpeed+100<p.ramSpeed)ok=false;st=ok?State::Pass:State::Warning;}
      Add(r,L"RAM",L"DIMM modules",summary.str(),p.ramSpeed?L"Factory "+std::to_wstring(p.ramSpeed)+L" MT/s":L"",st,Severity::Major,p.ramSpeed?Dimension::Factory:Dimension::Identity,L"CIM typed capture");
   } else Add(r,L"RAM",L"DIMM modules",ram.error.empty()?L"No data":ram.error,L"",ram.timedOut?State::Warning:State::NotTested,Severity::Minor,Dimension::Evidence);

   auto disk=RunProcessCapture(L"powershell.exe -NoProfile -NonInteractive -Command \"Get-CimInstance Win32_DiskDrive | ForEach-Object { '{0}|{1}|{2}|{3}|{4}' -f $_.Model,$_.Size,$_.SerialNumber,$_.FirmwareRevision,$_.InterfaceType }\"",20000,cancel);
   if(disk.launched&&!disk.timedOut&&!disk.output.empty()){
      for(const auto& line:SplitLines(disk.output)){StorageDevice d{};if(ParseDiskInventoryLine(line,d))r.hardware.storage.push_back(std::move(d));}
      for(size_t i=0;i<r.hardware.storage.size();++i){auto&d=r.hardware.storage[i];Add(r,L"Storage",L"Disk #"+std::to_wstring(i+1),d.model+L" | "+F1(Gib(d.capacityBytes))+L" GiB | SN "+d.serialNumber+L" | FW "+d.firmware+L" | "+d.interfaceType,L"",State::Info,Severity::Info,Dimension::Identity,L"CIM typed capture");}
   } else Add(r,L"Storage",L"Disk inventory",disk.error.empty()?L"No data":disk.error,L"",State::NotTested,Severity::Major,Dimension::Identity);

   auto batt=RunProcessCapture(L"powershell.exe -NoProfile -NonInteractive -Command \"$s=Get-CimInstance -Namespace root/wmi -ClassName BatteryStaticData -ErrorAction SilentlyContinue|Select-Object -First 1;$f=Get-CimInstance -Namespace root/wmi -ClassName BatteryFullChargedCapacity -ErrorAction SilentlyContinue|Select-Object -First 1;$c=Get-CimInstance -Namespace root/wmi -ClassName BatteryCycleCount -ErrorAction SilentlyContinue|Select-Object -First 1;$b=Get-CimInstance Win32_Battery -ErrorAction SilentlyContinue|Select-Object -First 1;if($s -or $f -or $b){'{0}|{1}|{2}|{3}|{4}|{5}' -f $s.DesignedCapacity,$f.FullChargedCapacity,$c.CycleCount,$s.ManufactureName,$s.SerialNumber,$b.Status}\"",20000,cancel);
   if(batt.launched&&!batt.timedOut&&!batt.output.empty()){
      BatteryInfo bi=r.hardware.battery; if(ParseBatteryLine(SplitLines(batt.output).front(),bi)){bi.currentChargePercent=r.hardware.battery.currentChargePercent;r.hardware.battery=bi;
        std::wstringstream x;if(bi.capacityReadable)x<<L"Design "<<F1(bi.designWh)<<L" Wh | Full "<<F1(bi.fullChargeWh)<<L" Wh | Health "<<F1(bi.healthPercent)<<L"% | Wear "<<F1(bi.wearPercent)<<L"%";else x<<L"Capacity unavailable";x<<L" | Cycles "<<(bi.cycleCount<0?L"N/A":std::to_wstring(bi.cycleCount));
        Add(r,L"Battery",L"Capacity / wear",x.str(),p.batteryDesignWh?L"Factory design "+F1(p.batteryDesignWh)+L" Wh":L"",BatteryState(bi.healthPercent),Severity::Major,Dimension::Health,L"Battery WMI typed capture");
      }
   } else Add(r,L"Battery",L"Capacity / wear",L"Not available",p.batteryDesignWh?L"Factory design "+F1(p.batteryDesignWh)+L" Wh":L"",State::NotTested,Severity::Major,Dimension::Health);
 }
 return r;
}
} // namespace lap
