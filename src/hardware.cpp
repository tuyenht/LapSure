#include "lap/hardware.h"
#include <algorithm>
#include <cwctype>
#include <sstream>

namespace lap {

std::wstring Trim(std::wstring s) {
    auto notSpace=[](wchar_t c){ return !iswspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}
std::vector<std::wstring> SplitLines(const std::wstring& text) {
    std::vector<std::wstring> out; std::wstringstream ss(text); std::wstring line;
    while(std::getline(ss,line)){ if(!line.empty() && line.back()==L'\r') line.pop_back(); line=Trim(line); if(!line.empty()) out.push_back(line); }
    return out;
}
std::vector<std::wstring> Split(const std::wstring& text, wchar_t delimiter) {
    std::vector<std::wstring> out; std::wstringstream ss(text); std::wstring part;
    while(std::getline(ss,part,delimiter)) out.push_back(Trim(part));
    return out;
}
long long ParseI64(const std::wstring&s,long long fallback){ try{ size_t n=0; auto v=std::stoll(Trim(s),&n); return n? v:fallback;}catch(...){return fallback;} }
double ParseDouble(const std::wstring&s,double fallback){ try{ size_t n=0; auto v=std::stod(Trim(s),&n); return n? v:fallback;}catch(...){return fallback;} }
uint64_t ParseU64(const std::wstring&s,uint64_t fallback){ try{ size_t n=0; auto v=std::stoull(Trim(s),&n); return n? v:fallback;}catch(...){return fallback;} }

bool ParseMemoryModuleLine(const std::wstring& line, MemoryModule& out) {
    auto p=Split(line,L'|'); if(p.size()<8) return false;
    out.capacityBytes=ParseU64(p[0]); out.configuredSpeed=(unsigned)ParseI64(p[1],0); out.ratedSpeed=(unsigned)ParseI64(p[2],0);
    out.manufacturer=p[3]; out.partNumber=p[4]; out.serialNumber=p[5]; out.deviceLocator=p[6]; out.bankLabel=p[7];
    return out.capacityBytes>0;
}
bool ParseDiskInventoryLine(const std::wstring& line, StorageDevice& out) {
    auto p=Split(line,L'|'); if(p.size()<5) return false;
    out.model=p[0]; out.capacityBytes=ParseU64(p[1]); out.serialNumber=p[2]; out.firmware=p[3]; out.interfaceType=p[4];
    return !out.model.empty();
}
bool ParseBatteryLine(const std::wstring& line, BatteryInfo& out) {
    auto p=Split(line,L'|'); if(p.size()<6) return false;
    auto design=ParseDouble(p[0],-1), full=ParseDouble(p[1],-1);
    out.present=true; out.cycleCount=ParseI64(p[2],-1); out.manufacturer=p[3]; out.serialNumber=p[4]; out.status=p[5];
    if(design>0 && full>0){
        out.capacityReadable=true; out.designWh=design/1000.0; out.fullChargeWh=full/1000.0;
        out.healthPercent=100.0*full/design; if(out.healthPercent>100.0) out.healthPercent=100.0;
        out.wearPercent=100.0-out.healthPercent;
    }
    return true;
}
bool ParseNvidiaCsvLine(const std::wstring& line, GpuInfo& out) {
    auto p=Split(line,L','); if(p.size()<13) return false;
    out.name=p[0]; out.serialNumber=p[1]; out.uuid=p[2]; out.vbios=p[3]; out.driver=p[4];
    auto mb=ParseDouble(p[5],0); out.vramBytes=(uint64_t)(mb*1024.0*1024.0);
    out.temperatureC=ParseDouble(p[6]); out.tempLimitC=ParseDouble(p[7]); out.pstate=p[8];
    out.powerW=ParseDouble(p[9]); out.powerLimitW=ParseDouble(p[10]); out.gpuUtilPercent=ParseDouble(p[11]); out.memoryUtilPercent=ParseDouble(p[12]);
    return !out.name.empty();
}

bool ParseDisplayLine(const std::wstring& line, DisplayInfo& out){auto p=Split(line,L'|');if(p.size()<11)return false;out.manufacturer=p[0];out.friendlyName=p[1];out.serialNumber=p[2];out.instanceName=p[3];out.currentWidth=(unsigned)ParseI64(p[4],0);out.currentHeight=(unsigned)ParseI64(p[5],0);out.nativeWidth=(unsigned)ParseI64(p[6],0);out.nativeHeight=(unsigned)ParseI64(p[7],0);out.refreshHz=(unsigned)ParseI64(p[8],0);out.touchDetected=ParseI64(p[9],0)!=0;out.internalPanel=ParseI64(p[10],0)!=0;return !out.instanceName.empty()||!out.friendlyName.empty();}
bool ParseMainboardLine(const std::wstring& line, MainboardInfo& out){auto p=Split(line,L'|');if(p.size()<3)return false;out.manufacturer=p[0];out.product=p[1];out.serialNumber=p[2];return true;}
bool ParseBiosLine(const std::wstring& line, BiosInfo& out){auto p=Split(line,L'|');if(p.size()<3)return false;out.vendor=p[0];out.version=p[1];out.releaseDate=p[2];return true;}
bool ParseCpuTelemetryLine(const std::wstring& line, CpuTelemetry& out){auto p=Split(line,L'|');if(p.size()<3)return false;out.loadPercent=ParseDouble(p[0],-1);out.currentClockMHz=(unsigned)ParseI64(p[1],0);out.maxClockMHz=(unsigned)ParseI64(p[2],0);return true;}

double NvmeDataUnitsToTB(long long dataUnits) {
    if(dataUnits<0) return -1;
    return (double)dataUnits * 512000.0 / 1000000000000.0;
}

} // namespace lap
