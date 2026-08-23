#include "lap/report.h"
#include "lap/scoring.h"
#include "lap/functional.h"
#include "lap/port_power.h"
#include "lap/orchestrator.h"
#include "lap/runtime_validation.h"
#include "lap/chassis_profile.h"
#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace lap {
namespace {
std::wstring Html(std::wstring s){for(size_t p=0;(p=s.find(L"&",p))!=std::wstring::npos;p+=5)s.replace(p,1,L"&amp;");for(size_t p=0;(p=s.find(L"<",p))!=std::wstring::npos;p+=4)s.replace(p,1,L"&lt;");for(size_t p=0;(p=s.find(L">",p))!=std::wstring::npos;p+=4)s.replace(p,1,L"&gt;");for(size_t p=0;(p=s.find(L"\"",p))!=std::wstring::npos;p+=6)s.replace(p,1,L"&quot;");return s;}
std::wstring Json(const std::wstring&s){std::wstring o;for(wchar_t c:s){switch(c){case L'\"':o+=L"\\\"";break;case L'\\':o+=L"\\\\";break;case L'\b':o+=L"\\b";break;case L'\f':o+=L"\\f";break;case L'\n':o+=L"\\n";break;case L'\r':o+=L"\\r";break;case L'\t':o+=L"\\t";break;default:if(c<0x20){wchar_t b[7];swprintf_s(b,L"\\u%04x",(unsigned)c);o+=b;}else o+=c;}}return o;}
std::wstring Ts(){SYSTEMTIME t;GetLocalTime(&t);wchar_t b[64];swprintf_s(b,L"%04d%02d%02d_%02d%02d%02d",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond);return b;}
bool Writable(const std::filesystem::path&p){std::error_code ec;std::filesystem::create_directories(p,ec);if(ec)return false;auto test=p/L".write_test.tmp";std::ofstream f(test,std::ios::binary);if(!f)return false;f<<"ok";f.close();std::filesystem::remove(test,ec);return true;}
std::wstring F(double x,int n=1){if(x<0)return L"N/A";wchar_t b[64];swprintf_s(b,n==1?L"%.1f":L"%.2f",x);return b;}
std::wstring GiB(uint64_t b){return F((double)b/(1024.0*1024.0*1024.0),1);}
bool WriteUtf8File(const std::filesystem::path&p,const std::wstring&text){if(text.size()>static_cast<size_t>(INT_MAX))return false;int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);if(n<=0)return false;std::string bytes(static_cast<size_t>(n),'\0');if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),bytes.data(),n,nullptr,nullptr)!=n)return false;std::ofstream out(p,std::ios::binary|std::ios::trunc);if(!out)return false;out.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));out.flush();return out.good();}
}
std::wstring ResolveReportDirectory(const std::wstring& appDir,bool winPE){
 std::filesystem::path app(appDir);auto local=app/L"reports";
 if(Writable(local)&&(!winPE||towupper(app.root_name().wstring().empty()?L'?':app.root_name().wstring()[0])!=L'X'))return local.wstring();
 if(winPE){DWORD mask=GetLogicalDrives();for(wchar_t d=L'D';d<=L'Z';++d){if(!(mask&(1u<<(d-L'A')))||d==L'X')continue;std::wstring root;root+=d;root+=L":\\";UINT type=GetDriveTypeW(root.c_str());if(type==DRIVE_REMOVABLE||type==DRIVE_FIXED){std::filesystem::path p=root;p/=L"LapSureReports";if(Writable(p))return p.wstring();}}}
 wchar_t docs[MAX_PATH]{};if(SUCCEEDED(SHGetFolderPathW(nullptr,CSIDL_MYDOCUMENTS,nullptr,SHGFP_TYPE_CURRENT,docs))){std::filesystem::path p(docs);p/=L"LapSureReports";if(Writable(p))return p.wstring();}
 wchar_t temp[MAX_PATH]{};GetTempPathW(MAX_PATH,temp);std::filesystem::path p(temp);p/=L"LapSureReports";Writable(p);return p.wstring();
}
std::wstring SaveHtmlReport(const AuditReport&r,const std::wstring&dir){
 std::filesystem::create_directories(dir);auto p=std::filesystem::path(dir)/(L"audit_"+Ts()+L".html");std::wostringstream f;
 f<<L"<meta charset='utf-8'><style>body{font-family:Segoe UI;margin:28px;color:#20242a;background:#f6f8fb}.hero,.card{background:white;border:1px solid #e0e4ea;border-radius:12px;padding:16px;margin:12px 0}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(260px,1fr));gap:12px}.metric{font-size:24px;font-weight:700}table{border-collapse:collapse;width:100%;background:white}td,th{border:1px solid #ddd;padding:8px;vertical-align:top}th{background:#eee}pre{white-space:pre-wrap;margin:0}.PASS{background:#dff3e4}.GOOD{background:#eaf4e8}.FAIL{background:#ffd9d9}.WARNING{background:#fff4ce}.CHANGED{background:#e8e4ff}</style>";
 f<<L"<div class='hero'><h1>LapSure v0.1-beta — Laptop Verification & Diagnostics</h1><b>"<<Html(r.model)<<L"</b> | Service Tag "<<Html(r.serviceTag)<<L" | "<<Html(r.environment)<<L"<br>Factory exact: "<<(r.factoryExact?L"Yes":L"No")<<L" | Generic mode: "<<(r.genericMode?L"Yes":L"No")<<L"</div>";
 f<<L"<div class='grid'>";
 f<<L"<div class='card'><b>CPU</b><div class='metric'>"<<Html(r.hardware.cpuName)<<L"</div><div>Threads: "<<r.hardware.cpuThreads<<L"</div></div>";
 f<<L"<div class='card'><b>RAM</b><div class='metric'>"<<GiB(r.hardware.installedRamBytes)<<L" GiB</div><div>Modules: "<<r.hardware.memoryModules.size()<<L"</div></div>";
 f<<L"<div class='card'><b>Storage</b><div class='metric'>"<<r.hardware.storage.size()<<L" device(s)</div>";
 for(auto&d:r.hardware.storage)f<<L"<div>"<<Html(d.model)<<L" | "<<GiB(d.capacityBytes)<<L" GiB | Endurance "<<(d.enduranceRemaining>=0?std::to_wstring(d.enduranceRemaining)+L"%":L"N/A")<<L" | "<<(d.powerOnHours>=0?std::to_wstring(d.powerOnHours)+L"h":L"N/A")<<L"</div>";
 f<<L"</div>";
 f<<L"<div class='card'><b>Battery</b><div class='metric'>"<<F(r.hardware.battery.healthPercent)<<L"% health</div><div>Design "<<F(r.hardware.battery.designWh)<<L" Wh | Full "<<F(r.hardware.battery.fullChargeWh)<<L" Wh | Wear "<<F(r.hardware.battery.wearPercent)<<L"%</div></div>";
 f<<L"<div class='card'><b>GPU</b><div class='metric'>"<<r.hardware.gpus.size()<<L" GPU(s)</div>";for(auto&g:r.hardware.gpus)f<<L"<div>"<<Html(g.name)<<L" | "<<GiB(g.vramBytes)<<L" GiB | "<<F(g.temperatureC)<<L" C</div>";f<<L"</div>";
 f<<L"<div class='card'><b>Audit Decision</b><div class='metric'>"<<Html(r.hardware.stress.decision.overall)<<L"</div><div>Stability: "<<Html(r.hardware.stress.decision.stability)<<L"</div><div>Thermal: "<<Html(r.hardware.stress.decision.thermal)<<L"</div><div>Performance: "<<Html(r.hardware.stress.decision.performance)<<L"</div><div>CPU microbench: "<<F(r.hardware.stress.cpuBenchmark.score)<<L" Mops/s | "<<Html(r.hardware.stress.cpuBenchmark.verdict)<<L"</div><div>Coverage: "<<Html(r.hardware.stress.decision.coverage)<<L" | Confidence: "<<ConfidenceText(r.hardware.stress.decision.confidence)<<L"</div>";
 for(auto&reason:r.hardware.stress.decision.reasons)f<<L"<div>• "<<Html(reason)<<L"</div>";
 f<<L"</div>";
 f<<L"<div class='card'><b>Stress Session</b><div class='metric'>"<<Html(r.hardware.stress.mode)<<L"</div>";
 for(auto&s:r.hardware.stress.stages){f<<L"<div><b>"<<Html(s.name)<<L"</b> | "<<VerdictText(s.verdict)<<L" | "<<s.elapsedSeconds<<L"s | WHEA+"<<s.newWhea;
   if(s.ram.bytesTested)f<<L" | RAM "<<F((double)s.ram.bytesTested/1073741824.0)<<L" GiB tested | mismatch "<<s.ram.mismatches;
   if(s.gpuVram.checkedGB>0||s.gpuVram.errors)f<<L" | VRAM "<<F(s.gpuVram.checkedGB)<<L" GB checked | errors "<<s.gpuVram.errors;
   if(s.telemetrySummary.sampleCount){f<<L" | CPU avg "<<F(s.telemetrySummary.avgCpuUtil)<<L"% | GPU max "<<F(s.telemetrySummary.maxGpuTempC)<<L"C";if(s.telemetrySummary.maxCpuPackageTempC>=0)f<<L" | CPU pkg max "<<F(s.telemetrySummary.maxCpuPackageTempC)<<L"C | CPU pkg max power "<<F(s.telemetrySummary.maxCpuPackagePowerW)<<L"W";}
   f<<L"</div>";
 }
 f<<L"</div>";
 f<<L"</div>";
 f<<L"<div class='card'><b>Functional Test Center</b><div class='metric'>"<<Html(r.hardware.stress.functional.overall)<<L"</div><div>PASS "<<r.hardware.stress.functional.passed<<L" | FAIL "<<r.hardware.stress.functional.failed<<L" | WARNING "<<r.hardware.stress.functional.warning<<L" | MANUAL "<<r.hardware.stress.functional.manualRequired<<L"</div>";
 for(auto&i:r.hardware.stress.functional.items)f<<L"<div>"<<Html(i.name)<<L" — "<<FunctionalStatusText(i.status)<<L" — "<<Html(i.detail)<<L"</div>";
 f<<L"</div>";
 f<<L"<div class='card'><b>Port & Power Verification</b><div class='metric'>"<<Html(r.hardware.stress.portPower.overall)<<L"</div>";
 f<<L"<div>USB4 host router: "<<(r.hardware.stress.portPower.usb4HostRouterPresent?L"YES":L"NO/UNKNOWN")<<L" | USB4 device routers: "<<r.hardware.stress.portPower.usb4DeviceRouters<<L" | Thunderbolt matches: "<<r.hardware.stress.portPower.thunderboltDevices<<L"</div>";
 f<<L"<div>Power: "<<Html(r.hardware.stress.portPower.power.verdict)<<L" | Adapter watts: "<<(r.hardware.stress.portPower.power.adapterWatts<0?L"UNKNOWN":F(r.hardware.stress.portPower.power.adapterWatts))<<L"</div>";
 for(auto&port:r.hardware.stress.portPower.ports)f<<L"<div>"<<Html(port.portLabel)<<L" — "<<Html(port.verdict)<<L" — "<<Html(port.deviceDescription)<<L" — "<<Html(port.negotiatedSpeed)<<L"</div>";
 f<<L"</div>";
 f<<L"<div class='card'><b>Guided Test Progress</b><div class='metric'>"<<r.hardware.stress.orchestrator.percent<<L"%</div><div>Next: "<<Html(r.hardware.stress.orchestrator.nextAction)<<L"</div>";
 for(auto&s:r.hardware.stress.orchestrator.stages)f<<L"<div>"<<Html(s.title)<<L" — "<<StageStateText(s.state)<<L" — "<<s.completed<<L"/"<<s.total<<L" — "<<Html(s.subtitle)<<L"</div>";
 f<<L"</div>";
 f<<L"<div class='card'><b>Model-aware Chassis / Port Map</b><div class='metric'>"<<Html(r.hardware.stress.chassisProfile.displayName.empty()?L"Generic":r.hardware.stress.chassisProfile.displayName)<<L"</div>";
 for(auto&port:r.hardware.stress.chassisProfile.ports)f<<L"<div>"<<Html(port.label)<<L" | "<<Html(port.side)<<L" | "<<Html(port.connector)<<L" | "<<Html(port.capability)<<L" | "<<(port.tested?Html(port.verdict):L"NOT TESTED")<<L"</div>";
 f<<L"</div>";
 f<<L"<div class='card'><b>Runtime Validation Gate</b><div class='metric'>"<<Html(r.hardware.stress.runtimeValidation.overall)<<L"</div><div>Build: "<<Html(r.hardware.stress.runtimeValidation.buildLabel)<<L" | Compiler: "<<Html(r.hardware.stress.runtimeValidation.compilerLabel)<<L" | Arch: "<<Html(r.hardware.stress.runtimeValidation.architecture)<<L"</div>";
 for(auto&x:r.hardware.stress.runtimeValidation.checks)f<<L"<div>"<<Html(x.name)<<L" — "<<ValidationStatusText(x.status)<<L" — "<<Html(x.detail)<<L"</div>";
 f<<L"</div>";
 f<<L"<h2>Detailed findings</h2><table><tr><th>Dimension</th><th>Group</th><th>Item</th><th>Value</th><th>Expected</th><th>Status</th><th>Severity</th><th>Evidence</th></tr>";
 for(auto&x:r.findings)f<<L"<tr><td>"<<ToString(x.dimension)<<L"</td><td>"<<Html(x.group)<<L"</td><td>"<<Html(x.name)<<L"</td><td><pre>"<<Html(x.value)<<L"</pre></td><td>"<<Html(x.expected)<<L"</td><td class='"<<ToString(x.state)<<L"'>"<<ToString(x.state)<<L"</td><td>"<<ToString(x.severity)<<L"</td><td>"<<Html(x.evidence)<<L"</td></tr>";
 f<<L"</table>";return WriteUtf8File(p,f.str())?p.wstring():L"";
}
std::wstring SaveJsonReport(const AuditReport&r,const std::wstring&dir){
 std::filesystem::create_directories(dir);auto p=std::filesystem::path(dir)/(L"audit_"+Ts()+L".json");std::wostringstream f;
 f<<L"{\n\"model\":\""<<Json(r.model)<<L"\",\"serviceTag\":\""<<Json(r.serviceTag)<<L"\",\"environment\":\""<<Json(r.environment)<<L"\",\"factoryExact\":"<<(r.factoryExact?L"true":L"false")<<L",\"genericMode\":"<<(r.genericMode?L"true":L"false")<<L",\n";
 f<<L"\"hardware\":{\"cpu\":{\"name\":\""<<Json(r.hardware.cpuName)<<L"\",\"threads\":"<<r.hardware.cpuThreads<<L"},\"ram\":{\"totalBytes\":"<<r.hardware.installedRamBytes<<L",\"modules\":[";
 for(size_t i=0;i<r.hardware.memoryModules.size();++i){auto&m=r.hardware.memoryModules[i];f<<L"{\"capacityBytes\":"<<m.capacityBytes<<L",\"configuredSpeed\":"<<m.configuredSpeed<<L",\"ratedSpeed\":"<<m.ratedSpeed<<L",\"manufacturer\":\""<<Json(m.manufacturer)<<L"\",\"partNumber\":\""<<Json(m.partNumber)<<L"\",\"serial\":\""<<Json(m.serialNumber)<<L"\"}"<<(i+1<r.hardware.memoryModules.size()?L",":L"");}
 f<<L"]},\"battery\":{\"present\":"<<(r.hardware.battery.present?L"true":L"false")<<L",\"designWh\":"<<r.hardware.battery.designWh<<L",\"fullWh\":"<<r.hardware.battery.fullChargeWh<<L",\"healthPercent\":"<<r.hardware.battery.healthPercent<<L",\"wearPercent\":"<<r.hardware.battery.wearPercent<<L",\"cycleCount\":"<<r.hardware.battery.cycleCount<<L"},\"storage\":[";
 for(size_t i=0;i<r.hardware.storage.size();++i){auto&d=r.hardware.storage[i];f<<L"{\"device\":\""<<Json(d.devicePath)<<L"\",\"model\":\""<<Json(d.model)<<L"\",\"serial\":\""<<Json(d.serialNumber)<<L"\",\"firmware\":\""<<Json(d.firmware)<<L"\",\"capacityBytes\":"<<d.capacityBytes<<L",\"reliabilityReadable\":"<<(d.reliabilityReadable?L"true":L"false")<<L",\"reliabilityHealthy\":"<<(d.reliabilityHealthy?L"true":L"false")<<L",\"reliabilityProvider\":\""<<Json(d.reliabilityProvider)<<L"\",\"healthStatus\":\""<<Json(d.healthStatus)<<L"\",\"operationalStatus\":\""<<Json(d.operationalStatus)<<L"\",\"smartReadable\":"<<(d.smartReadable?L"true":L"false")<<L",\"smartPassed\":"<<(d.smartPassed?L"true":L"false")<<L",\"percentageUsed\":"<<d.percentageUsed<<L",\"enduranceRemaining\":"<<d.enduranceRemaining<<L",\"mediaErrors\":"<<d.mediaErrors<<L",\"readErrorsUncorrected\":"<<d.readErrorsUncorrected<<L",\"writeErrorsUncorrected\":"<<d.writeErrorsUncorrected<<L",\"criticalWarning\":"<<d.criticalWarning<<L",\"powerOnHours\":"<<d.powerOnHours<<L",\"powerCycles\":"<<d.powerCycles<<L",\"unsafeShutdowns\":"<<d.unsafeShutdowns<<L",\"dataReadTB\":"<<d.approxDataReadTB<<L",\"dataWrittenTB\":"<<d.approxDataWrittenTB<<L",\"temperatureC\":"<<d.temperatureC<<L"}"<<(i+1<r.hardware.storage.size()?L",":L"");}
 f<<L"],\"gpus\":[";
 for(size_t i=0;i<r.hardware.gpus.size();++i){auto&g=r.hardware.gpus[i];f<<L"{\"name\":\""<<Json(g.name)<<L"\",\"vramBytes\":"<<g.vramBytes<<L",\"driver\":\""<<Json(g.driver)<<L"\",\"vbios\":\""<<Json(g.vbios)<<L"\",\"temperatureC\":"<<g.temperatureC<<L",\"powerW\":"<<g.powerW<<L"}"<<(i+1<r.hardware.gpus.size()?L",":L"");}
 f<<L"],\"stress\":{\"mode\":\""<<Json(r.hardware.stress.mode)<<L"\",\"completed\":"<<(r.hardware.stress.completed?L"true":L"false")<<L",\"decision\":{\"overall\":\""<<Json(r.hardware.stress.decision.overall)<<L"\",\"stability\":\""<<Json(r.hardware.stress.decision.stability)<<L"\",\"thermal\":\""<<Json(r.hardware.stress.decision.thermal)<<L"\",\"performance\":\""<<Json(r.hardware.stress.decision.performance)<<L"\",\"coverage\":\""<<Json(r.hardware.stress.decision.coverage)<<L"\",\"confidence\":\""<<ConfidenceText(r.hardware.stress.decision.confidence)<<L"\",\"reasons\":[";for(size_t i=0;i<r.hardware.stress.decision.reasons.size();++i)f<<L"\""<<Json(r.hardware.stress.decision.reasons[i])<<L"\""<<(i+1<r.hardware.stress.decision.reasons.size()?L",":L"");f<<L"]},\"stages\":[";
 for(size_t i=0;i<r.hardware.stress.stages.size();++i){auto&s=r.hardware.stress.stages[i];f<<L"{\"name\":\""<<Json(s.name)<<L"\",\"verdict\":\""<<VerdictText(s.verdict)<<L"\",\"elapsedSeconds\":"<<s.elapsedSeconds<<L",\"newWhea\":"<<s.newWhea<<L",\"newDisk\":"<<s.newDisk<<L",\"newNvme\":"<<s.newNvme<<L",\"newDisplay\":"<<s.newDisplay<<L",\"newBugCheck\":"<<s.newBugCheck<<L",\"telemetrySamples\":"<<s.telemetrySummary.sampleCount<<L",\"avgCpuUtil\":"<<s.telemetrySummary.avgCpuUtil<<L",\"maxGpuTempC\":"<<s.telemetrySummary.maxGpuTempC<<L",\"ramMismatches\":"<<s.ram.mismatches<<L",\"vramErrors\":"<<s.gpuVram.errors<<L"}"<<(i+1<r.hardware.stress.stages.size()?L",":L"");}
 f<<L"]},\"functional\":{\"overall\":\""<<Json(r.hardware.stress.functional.overall)<<L"\",\"passed\":"<<r.hardware.stress.functional.passed<<L",\"failed\":"<<r.hardware.stress.functional.failed<<L",\"warning\":"<<r.hardware.stress.functional.warning<<L",\"notTested\":"<<r.hardware.stress.functional.notTested<<L",\"manualRequired\":"<<r.hardware.stress.functional.manualRequired<<L",\"items\":[";
 for(size_t i=0;i<r.hardware.stress.functional.items.size();++i){auto&x=r.hardware.stress.functional.items[i];f<<L"{\"id\":\""<<Json(x.id)<<L"\",\"name\":\""<<Json(x.name)<<L"\",\"status\":\""<<FunctionalStatusText(x.status)<<L"\",\"detail\":\""<<Json(x.detail)<<L"\",\"evidence\":\""<<Json(x.evidence)<<L"\"}"<<(i+1<r.hardware.stress.functional.items.size()?L",":L"");}
 f<<L"]},\"portPower\":{\"overall\":\""<<Json(r.hardware.stress.portPower.overall)<<L"\",\"ports\":[";for(size_t i=0;i<r.hardware.stress.portPower.ports.size();++i){auto&x=r.hardware.stress.portPower.ports[i];f<<L"{\"label\":\""<<Json(x.portLabel)<<L"\",\"verdict\":\""<<Json(x.verdict)<<L"\",\"instanceId\":\""<<Json(x.instanceId)<<L"\",\"locationPath\":\""<<Json(x.locationPath)<<L"\"}"<<(i+1<r.hardware.stress.portPower.ports.size()?L",":L"");}f<<L"]},\"runtimeValidation\":{\"overall\":\""<<Json(r.hardware.stress.runtimeValidation.overall)<<L"\",\"failed\":"<<r.hardware.stress.runtimeValidation.failed<<L",\"warning\":"<<r.hardware.stress.runtimeValidation.warning<<L"},\"chassisProfile\":{\"id\":\""<<Json(r.hardware.stress.chassisProfile.profileId)<<L"\",\"requiredRemaining\":"<<RequiredPortsRemaining(r.hardware.stress.chassisProfile)<<L"},\"orchestrator\":{\"overall\":\""<<Json(r.hardware.stress.orchestrator.overall)<<L"\",\"percent\":"<<r.hardware.stress.orchestrator.percent<<L"}},\n\"findings\":[\n";
 for(size_t i=0;i<r.findings.size();++i){auto&x=r.findings[i];f<<L"{\"dimension\":\""<<ToString(x.dimension)<<L"\",\"group\":\""<<Json(x.group)<<L"\",\"name\":\""<<Json(x.name)<<L"\",\"value\":\""<<Json(x.value)<<L"\",\"expected\":\""<<Json(x.expected)<<L"\",\"state\":\""<<ToString(x.state)<<L"\",\"severity\":\""<<ToString(x.severity)<<L"\",\"evidence\":\""<<Json(x.evidence)<<L"\"}"<<(i+1<r.findings.size()?L",":L"")<<L"\n";}
 f<<L"]}\n";return WriteUtf8File(p,f.str())?p.wstring():L"";
}
} // namespace lap
