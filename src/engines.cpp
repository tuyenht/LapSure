#include "lap/engines.h"
#include "lap/process.h"
#include "lap/hardware.h"
#include <windows.h>
#include <regex>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cwctype>
#include <cmath>

namespace lap {
namespace {
bool Exists(const std::wstring&p){return GetFileAttributesW(p.c_str())!=INVALID_FILE_ATTRIBUTES;}
void Add(AuditReport&r,std::wstring g,std::wstring n,std::wstring v,std::wstring e,State st,Severity sv,Dimension d,std::wstring ev=L""){r.findings.push_back({std::move(g),std::move(n),std::move(v),std::move(e),st,sv,std::move(ev),d});}
bool ContainsI(std::wstring a,std::wstring b){std::transform(a.begin(),a.end(),a.begin(),towlower);std::transform(b.begin(),b.end(),b.begin(),towlower);return a.find(b)!=std::wstring::npos;}
std::wstring FirstToken(const std::wstring&line){auto p=line.find_first_of(L" \t");return p==std::wstring::npos?line:line.substr(0,p);}
std::wstring JString(const std::wstring&j,const wchar_t* key){std::wregex rx(std::wstring(L"\\\"")+key+L"\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");std::wsmatch m;return std::regex_search(j,m,rx)?m[1].str():L"";}
long long JInt(const std::wstring&j,const wchar_t* key,long long def=-1){std::wregex rx(std::wstring(L"\\\"")+key+L"\\\"\\s*:\\s*(-?[0-9]+)");std::wsmatch m;return std::regex_search(j,m,rx)?std::stoll(m[1].str()):def;}
bool JBool(const std::wstring&j,const wchar_t* key,bool def=false){std::wregex rx(std::wstring(L"\\\"")+key+L"\\\"\\s*:\\s*(true|false)");std::wsmatch m;return std::regex_search(j,m,rx)?m[1].str()==L"true":def;}
bool HasBool(const std::wstring&j,const wchar_t*key){std::wregex rx(std::wstring(L"\\\"")+key+L"\\\"\\s*:\\s*(true|false)");return std::regex_search(j,rx);}
std::wstring Fmt(long long x,const wchar_t* unit=L""){return x<0?L"N/A":std::to_wstring(x)+(unit?unit:L"");}
std::wstring Fmt1(double x,const wchar_t* unit=L""){if(x<0)return L"N/A";wchar_t b[64];swprintf_s(b,L"%.1f%s",x,unit);return b;}
std::vector<std::wstring> SplitPipeLine(const std::wstring&line){std::vector<std::wstring> fields;size_t start=0;for(;;){auto end=line.find(L'|',start);fields.push_back(line.substr(start,end==std::wstring::npos?end:end-start));if(end==std::wstring::npos)break;start=end+1;}return fields;}
StorageDevice* MatchStorage(AuditReport&r,const std::wstring&model,const std::wstring&serial){
 for(auto&d:r.hardware.storage){
   if(!serial.empty()&&!d.serialNumber.empty()&&ContainsI(d.serialNumber,serial))return &d;
   if(!model.empty()&&!d.model.empty()&&(ContainsI(d.model,model)||ContainsI(model,d.model)))return &d;
 }
 r.hardware.storage.push_back(StorageDevice{});return &r.hardware.storage.back();
}
}
GpuInfo* MatchGpu(AuditReport&r,const std::wstring&name){for(auto&g:r.hardware.gpus)if(!name.empty()&&!g.name.empty()&&(ContainsI(g.name,name)||ContainsI(name,g.name)))return &g;r.hardware.gpus.push_back(GpuInfo{});return &r.hardware.gpus.back();}

bool ParseSmartctlHealthJson(const std::wstring&j,StorageDevice&sd,std::wstring&error){
 error.clear();if(j.empty()||j.front()!=L'{'||j.back()!=L'}'){error=L"Malformed smartctl JSON object";return false;}
 if(!HasBool(j,L"passed")){error=L"Missing explicit SMART health verdict";return false;}
 sd.model=JString(j,L"model_name");sd.serialNumber=JString(j,L"serial_number");sd.firmware=JString(j,L"firmware_version");
 sd.smartReadable=true;sd.smartPassed=JBool(j,L"passed",false);sd.criticalWarning=JInt(j,L"critical_warning");sd.percentageUsed=JInt(j,L"percentage_used");
 sd.enduranceRemaining=sd.percentageUsed>=0?std::max<long long>(0,100-sd.percentageUsed):-1;sd.availableSpare=JInt(j,L"available_spare");sd.spareThreshold=JInt(j,L"available_spare_threshold");
 sd.mediaErrors=JInt(j,L"media_errors");sd.errorLogEntries=JInt(j,L"num_err_log_entries");sd.unsafeShutdowns=JInt(j,L"unsafe_shutdowns");sd.powerOnHours=JInt(j,L"power_on_hours");sd.powerCycles=JInt(j,L"power_cycles");sd.temperatureC=JInt(j,L"temperature");sd.dataUnitsRead=JInt(j,L"data_units_read");sd.dataUnitsWritten=JInt(j,L"data_units_written");sd.approxDataReadTB=NvmeDataUnitsToTB(sd.dataUnitsRead);sd.approxDataWrittenTB=NvmeDataUnitsToTB(sd.dataUnitsWritten);return true;
}

bool ParseWindowsStorageReliabilityLine(const std::wstring&line,StorageDevice&sd){
 auto fields=SplitPipeLine(line);if(fields.size()<10||fields[0].empty())return false;
 auto number=[](const std::wstring&s){try{return s.empty()?-1LL:std::stoll(s);}catch(...){return -1LL;}};
 sd.model=fields[0];sd.serialNumber=fields[1];sd.healthStatus=fields[2];sd.operationalStatus=fields[3];
 sd.temperatureC=number(fields[4]);auto maxTemperature=number(fields[5]);sd.percentageUsed=number(fields[6]);
 sd.enduranceRemaining=sd.percentageUsed>=0?std::max<long long>(0,100-sd.percentageUsed):-1;
 sd.powerOnHours=number(fields[7]);sd.readErrorsUncorrected=number(fields[8]);sd.writeErrorsUncorrected=number(fields[9]);
 sd.reliabilityReadable=!sd.healthStatus.empty()||sd.temperatureC>=0||sd.percentageUsed>=0||sd.powerOnHours>=0||sd.readErrorsUncorrected>=0||sd.writeErrorsUncorrected>=0;
 const bool healthOk=ContainsI(sd.healthStatus,L"Healthy")&&!ContainsI(sd.healthStatus,L"Unhealthy");
 const bool operationOk=sd.operationalStatus.empty()||ContainsI(sd.operationalStatus,L"OK");
 const bool errorsOk=sd.readErrorsUncorrected<=0&&sd.writeErrorsUncorrected<=0;
 sd.reliabilityHealthy=sd.reliabilityReadable&&healthOk&&operationOk&&errorsOk;sd.reliabilityProvider=L"Windows Storage Management";
 (void)maxTemperature;return sd.reliabilityReadable;
}

void CollectWindowsStorageReliability(AuditReport&r,const Capabilities&c,const std::atomic_bool* cancel){
 if(!c.powershell){Add(r,L"Storage",L"Windows native reliability",L"PowerShell unavailable",L"Native storage reliability evidence",State::NotTested,Severity::Critical,Dimension::Health);return;}
 const std::wstring command=L"powershell.exe -NoProfile -NonInteractive -Command \"Get-PhysicalDisk | ForEach-Object { $d=$_; $x=$d | Get-StorageReliabilityCounter -ErrorAction SilentlyContinue; if($x){ '{0}|{1}|{2}|{3}|{4}|{5}|{6}|{7}|{8}|{9}' -f $d.FriendlyName,$d.SerialNumber,$d.HealthStatus,($d.OperationalStatus -join ','),$x.Temperature,$x.TemperatureMax,$x.Wear,$x.PowerOnHours,$x.ReadErrorsUncorrected,$x.WriteErrorsUncorrected } }\"";
 auto pr=RunProcessCapture(command,30000,cancel);
 if(!pr.launched||pr.timedOut||pr.output.empty()){Add(r,L"Storage",L"Windows native reliability",pr.error.empty()?L"No readable reliability counters (administrator access may be required)":pr.error,L"Native health, temperature and wear evidence",State::NotTested,Severity::Critical,Dimension::Health,L"Get-StorageReliabilityCounter");return;}
 size_t parsedCount=0;
 for(const auto&line:SplitLines(pr.output)){
   StorageDevice native{};if(!ParseWindowsStorageReliabilityLine(line,native))continue;++parsedCount;
   auto*sd=MatchStorage(r,native.model,native.serialNumber);auto capacity=sd->capacityBytes;auto firmware=sd->firmware;auto interfaceType=sd->interfaceType;auto devicePath=sd->devicePath;
   *sd=native;sd->capacityBytes=capacity;sd->firmware=firmware;sd->interfaceType=interfaceType;sd->devicePath=devicePath;
   State state=sd->reliabilityHealthy?State::Pass:State::Fail;
   std::wstringstream value;value<<sd->healthStatus<<L" | Operational="<<sd->operationalStatus<<L" | Temp="<<Fmt(sd->temperatureC,L" C")<<L" | Used="<<Fmt(sd->percentageUsed,L"%")<<L" | Power-on="<<Fmt(sd->powerOnHours,L" h")<<L" | Read/Write uncorrected="<<Fmt(sd->readErrorsUncorrected)<<L"/"<<Fmt(sd->writeErrorsUncorrected);
   Add(r,L"Storage",L"Windows reliability "+sd->model,value.str(),L"Healthy; Operational OK; uncorrected errors=0",state,Severity::Critical,Dimension::Health,L"Get-PhysicalDisk + Get-StorageReliabilityCounter");
 }
 if(!parsedCount)Add(r,L"Storage",L"Windows native reliability",L"Provider returned no parseable devices",L"One result per physical disk",State::NotTested,Severity::Critical,Dimension::Evidence,pr.output);
}

void CollectSmartctl(AuditReport&r,const FactoryProfile&,const Capabilities&c,const std::wstring&dir,const std::atomic_bool* cancel){
 if(!c.smartctl){const bool native=!r.hardware.storage.empty()&&std::all_of(r.hardware.storage.begin(),r.hardware.storage.end(),[](const auto&d){return d.reliabilityReadable;});Add(r,L"Storage",L"Advanced SMART/NVMe log",L"smartctl unavailable",L"Advanced controller log",State::NotTested,native?Severity::Minor:Severity::Critical,Dimension::Health,native?L"Advanced enrichment unavailable; native reliability evidence remains available":L"No storage-health provider available");return;}
 std::wstring exe=Exists(dir+L"\\tools\\smartctl.exe")?L"\""+dir+L"\\tools\\smartctl.exe\"":L"smartctl.exe";
 auto scan=RunProcessCapture(exe+L" --scan-open",15000,cancel);
 if(!scan.launched||scan.timedOut||scan.output.empty()){Add(r,L"Storage",L"SMART device scan",scan.error.empty()?L"No devices returned":scan.error,L"Detected storage",State::Warning,Severity::Major,Dimension::Health);return;}
 std::vector<std::wstring> devices;for(auto&line:SplitLines(scan.output)){auto d=FirstToken(line);if(!d.empty()&&d[0]==L'/')devices.push_back(d);}
 Add(r,L"Storage",L"SMART devices",std::to_wstring(devices.size())+L" device(s)",L"Detected storage",devices.empty()?State::Warning:State::Pass,Severity::Major,Dimension::Identity,L"smartctl --scan-open");
 for(const auto&dev:devices){
   auto pr=RunProcessCapture(exe+L" -a -j \""+dev+L"\"",30000,cancel);
   if(!pr.launched||pr.timedOut||pr.output.empty()){Add(r,L"Storage",L"SMART "+dev,L"Could not read",L"Readable SMART",State::Warning,Severity::Critical,Dimension::Health);continue;}
   StorageDevice parsed{};std::wstring parseError;if(!ParseSmartctlHealthJson(pr.output,parsed,parseError)){Add(r,L"Storage",L"SMART "+dev,parseError,L"Explicit SMART health schema",State::NotTested,Severity::Critical,Dimension::Health,L"smartctl JSON rejected");continue;}
   auto model=parsed.model,serial=parsed.serialNumber,fw=parsed.firmware;auto crit=parsed.criticalWarning,used=parsed.percentageUsed,spare=parsed.availableSpare,spareTh=parsed.spareThreshold;
   auto media=parsed.mediaErrors,errlog=parsed.errorLogEntries,unsafe=parsed.unsafeShutdowns,hours=parsed.powerOnHours,cycles=parsed.powerCycles;
   auto temp=parsed.temperatureC,readUnits=parsed.dataUnitsRead,writeUnits=parsed.dataUnitsWritten;bool smart=parsed.smartPassed;

   StorageDevice* sd=MatchStorage(r,model,serial);sd->devicePath=dev;if(!model.empty())sd->model=model;if(!serial.empty())sd->serialNumber=serial;if(!fw.empty())sd->firmware=fw;
   sd->smartReadable=true;sd->smartPassed=smart;sd->criticalWarning=crit;sd->percentageUsed=used;sd->enduranceRemaining=(used>=0?std::max<long long>(0,100-used):-1);sd->availableSpare=spare;sd->spareThreshold=spareTh;
   sd->mediaErrors=media;sd->errorLogEntries=errlog;sd->unsafeShutdowns=unsafe;sd->powerOnHours=hours;sd->powerCycles=cycles;sd->temperatureC=temp;sd->dataUnitsRead=readUnits;sd->dataUnitsWritten=writeUnits;
   sd->approxDataReadTB=NvmeDataUnitsToTB(readUnits);sd->approxDataWrittenTB=NvmeDataUnitsToTB(writeUnits);

   State health=(!smart||crit>0||media>0)?State::Fail:(used>=20?State::Warning:(used>=10?State::Good:State::Pass));
   Add(r,L"Storage",L"Identity "+dev,sd->model+L" | SN "+sd->serialNumber+L" | FW "+sd->firmware,L"",State::Info,Severity::Info,Dimension::Identity);
   std::wstringstream h;h<<L"SMART="<<(smart?L"PASS":L"FAIL")<<L" | Endurance remaining="<<Fmt(sd->enduranceRemaining,L"%")<<L" | Used="<<Fmt(used,L"%")<<L" | Critical="<<Fmt(crit)<<L" | MediaErrors="<<Fmt(media)<<L" | Spare="<<Fmt(spare,L"%")<<L" | Temp="<<Fmt(temp,L" C");
   Add(r,L"Storage",L"Health "+dev,h.str(),L"Critical=0; MediaErrors=0",health,Severity::Critical,Dimension::Health,L"smartctl JSON");
   std::wstringstream u;u<<L"Power-on "<<Fmt(hours,L" h")<<L" | Cycles "<<Fmt(cycles)<<L" | Unsafe shutdowns "<<Fmt(unsafe)<<L" | Read "<<Fmt1(sd->approxDataReadTB,L" TB")<<L" | Written "<<Fmt1(sd->approxDataWrittenTB,L" TB")<<L" | Error log "<<Fmt(errlog);
   Add(r,L"Storage",L"Usage "+dev,u.str(),L"",State::Info,Severity::Info,Dimension::Usage);
 }
}

void CollectNvidia(AuditReport&r,const FactoryProfile&p,const Capabilities&c,const std::wstring&dir,const std::atomic_bool* cancel){
 if(!c.nvidiaSmi){Add(r,L"GPU",L"NVIDIA telemetry",L"nvidia-smi unavailable",L"GPU/VRAM telemetry",State::NotTested,Severity::Major,Dimension::Health);return;}
 std::wstring exe=Exists(dir+L"\\tools\\nvidia-smi.exe")?L"\""+dir+L"\\tools\\nvidia-smi.exe\"":L"nvidia-smi.exe";
 auto pr=RunProcessCapture(exe+L" --query-gpu=name,serial,uuid,vbios_version,driver_version,memory.total,temperature.gpu,temperature.gpu.tlimit,pstate,power.draw,power.limit,utilization.gpu,utilization.memory --format=csv,noheader,nounits",15000,cancel);
 if(!pr.launched||pr.timedOut||pr.output.empty()){Add(r,L"GPU",L"NVIDIA telemetry",pr.error.empty()?L"No output":pr.error,L"",State::Warning,Severity::Major,Dimension::Health);return;}
 for(const auto&line:SplitLines(pr.output)){
   GpuInfo g{};if(!ParseNvidiaCsvLine(line,g))continue;auto*existing=MatchGpu(r,g.name);*existing=g;
   State factory=p.gpuContains.empty()?State::Info:(ContainsI(g.name,p.gpuContains)?State::Pass:State::Fail);
   Add(r,L"GPU",L"Adapter",g.name,p.gpuContains,factory,p.gpuContains.empty()?Severity::Info:Severity::Critical,p.gpuContains.empty()?Dimension::Identity:Dimension::Factory,L"nvidia-smi");
   std::wstringstream d;d<<Fmt1((double)g.vramBytes/(1024.0*1024.0*1024.0),L" GiB")<<L" VRAM | Driver "<<g.driver<<L" | VBIOS "<<g.vbios;
   Add(r,L"GPU",L"Identity / VRAM",d.str(),p.gpuVramBytes?Fmt1((double)p.gpuVramBytes/(1024.0*1024.0*1024.0),L" GiB"):L"",State::Info,Severity::Major,Dimension::Identity);
   std::wstringstream t;t<<L"Temp "<<Fmt1(g.temperatureC,L" C")<<L" / limit "<<Fmt1(g.tempLimitC,L" C")<<L" | "<<g.pstate<<L" | Power "<<Fmt1(g.powerW,L" W")<<L" / "<<Fmt1(g.powerLimitW,L" W")<<L" | GPU "<<Fmt1(g.gpuUtilPercent,L"%")<<L" | VRAM "<<Fmt1(g.memoryUtilPercent,L"%");
   Add(r,L"GPU",L"Live telemetry",t.str(),L"",State::Info,Severity::Info,Dimension::Usage,L"nvidia-smi");
 }
 if(r.hardware.gpus.empty())Add(r,L"GPU",L"NVIDIA parse",L"Output present but no valid CSV row parsed",L"",State::Warning,Severity::Major,Dimension::Evidence,pr.output);
}

} // namespace lap
