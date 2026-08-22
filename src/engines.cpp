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
std::wstring Fmt(long long x,const wchar_t* unit=L""){return x<0?L"N/A":std::to_wstring(x)+(unit?unit:L"");}
std::wstring Fmt1(double x,const wchar_t* unit=L""){if(x<0)return L"N/A";wchar_t b[64];swprintf_s(b,L"%.1f%s",x,unit);return b;}
StorageDevice* MatchStorage(AuditReport&r,const std::wstring&model,const std::wstring&serial){
 for(auto&d:r.hardware.storage){
   if(!serial.empty()&&!d.serialNumber.empty()&&ContainsI(d.serialNumber,serial))return &d;
   if(!model.empty()&&!d.model.empty()&&(ContainsI(d.model,model)||ContainsI(model,d.model)))return &d;
 }
 r.hardware.storage.push_back(StorageDevice{});return &r.hardware.storage.back();
}
}

void CollectSmartctl(AuditReport&r,const FactoryProfile&,const Capabilities&c,const std::wstring&dir,const std::atomic_bool* cancel){
 if(!c.smartctl){Add(r,L"Storage",L"SMART deep",L"smartctl unavailable",L"SMART/NVMe health",State::NotTested,Severity::Critical,Dimension::Health);return;}
 std::wstring exe=Exists(dir+L"\\tools\\smartctl.exe")?L"\""+dir+L"\\tools\\smartctl.exe\"":L"smartctl.exe";
 auto scan=RunProcessCapture(exe+L" --scan-open",15000,cancel);
 if(!scan.launched||scan.timedOut||scan.output.empty()){Add(r,L"Storage",L"SMART device scan",scan.error.empty()?L"No devices returned":scan.error,L"Detected storage",State::Warning,Severity::Major,Dimension::Health);return;}
 std::vector<std::wstring> devices;for(auto&line:SplitLines(scan.output)){auto d=FirstToken(line);if(!d.empty()&&d[0]==L'/')devices.push_back(d);}
 Add(r,L"Storage",L"SMART devices",std::to_wstring(devices.size())+L" device(s)",L"Detected storage",devices.empty()?State::Warning:State::Pass,Severity::Major,Dimension::Identity,L"smartctl --scan-open");
 for(const auto&dev:devices){
   auto pr=RunProcessCapture(exe+L" -a -j \""+dev+L"\"",30000,cancel);
   if(!pr.launched||pr.timedOut||pr.output.empty()){Add(r,L"Storage",L"SMART "+dev,L"Could not read",L"Readable SMART",State::Warning,Severity::Critical,Dimension::Health);continue;}
   auto model=JString(pr.output,L"model_name"),serial=JString(pr.output,L"serial_number"),fw=JString(pr.output,L"firmware_version");
   auto crit=JInt(pr.output,L"critical_warning"),used=JInt(pr.output,L"percentage_used"),spare=JInt(pr.output,L"available_spare"),spareTh=JInt(pr.output,L"available_spare_threshold");
   auto media=JInt(pr.output,L"media_errors"),errlog=JInt(pr.output,L"num_err_log_entries"),unsafe=JInt(pr.output,L"unsafe_shutdowns"),hours=JInt(pr.output,L"power_on_hours"),cycles=JInt(pr.output,L"power_cycles");
   auto temp=JInt(pr.output,L"temperature"),readUnits=JInt(pr.output,L"data_units_read"),writeUnits=JInt(pr.output,L"data_units_written"); bool smart=JBool(pr.output,L"passed",true);

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
   GpuInfo g{};if(!ParseNvidiaCsvLine(line,g))continue;r.hardware.gpus.push_back(g);
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
