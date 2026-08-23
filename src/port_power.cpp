#include "lap/port_power.h"
#include <windows.h>
#include <setupapi.h>
#include <devpkey.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <vector>
#include <string>
#include <algorithm>
#include <set>

#pragma comment(lib,"setupapi.lib")
#pragma comment(lib,"cfgmgr32.lib")

namespace lap {
namespace {
std::wstring Lower(std::wstring s){std::transform(s.begin(),s.end(),s.begin(),towlower);return s;}
struct Dev {std::wstring id,name,location,busDesc,service;};
std::wstring Prop(HDEVINFO set,SP_DEVINFO_DATA&d,const DEVPROPKEY&key){
 DEVPROPTYPE type=0;DWORD need=0;SetupDiGetDevicePropertyW(set,&d,&key,&type,nullptr,0,&need,0);
 if(!need)return L"";std::vector<BYTE>b(need+sizeof(wchar_t));
 if(!SetupDiGetDevicePropertyW(set,&d,&key,&type,b.data(),(DWORD)b.size(),nullptr,0))return L"";
 return std::wstring((wchar_t*)b.data());
}
std::vector<Dev> Devices(){
 std::vector<Dev> out;HDEVINFO set=SetupDiGetClassDevsW(nullptr,nullptr,nullptr,DIGCF_ALLCLASSES|DIGCF_PRESENT);
 if(set==INVALID_HANDLE_VALUE)return out;
 for(DWORD i=0;;i++){SP_DEVINFO_DATA d{};d.cbSize=sizeof(d);if(!SetupDiEnumDeviceInfo(set,i,&d)){if(GetLastError()==ERROR_NO_MORE_ITEMS)break;continue;}
  wchar_t id[1024]{};SetupDiGetDeviceInstanceIdW(set,&d,id,1024,nullptr);
  Dev x{};x.id=id;x.name=Prop(set,d,DEVPKEY_Device_FriendlyName);if(x.name.empty())x.name=Prop(set,d,DEVPKEY_Device_DeviceDesc);
  x.location=Prop(set,d,DEVPKEY_Device_LocationPaths);x.busDesc=Prop(set,d,DEVPKEY_Device_BusReportedDeviceDesc);x.service=Prop(set,d,DEVPKEY_Device_Service);
  out.push_back(std::move(x));
 }SetupDiDestroyDeviceInfoList(set);return out;
}
std::set<std::wstring> Ids(const std::vector<Dev>&v){std::set<std::wstring>s;for(auto&x:v)s.insert(x.id);return s;}
unsigned CountMatch(const std::vector<Dev>&v,const std::wstring&needle){
 unsigned n=0;for(auto&x:v){auto all=Lower(x.id+L" "+x.name+L" "+x.service+L" "+x.busDesc);if(all.find(Lower(needle))!=std::wstring::npos)n++;}return n;
}
bool IsUsbLike(const Dev&x){auto a=Lower(x.id+L" "+x.name+L" "+x.busDesc);return a.find(L"usb")!=std::wstring::npos||a.find(L"thunderbolt")!=std::wstring::npos;}
std::wstring SpeedHint(const Dev&x){
 auto a=Lower(x.name+L" "+x.busDesc);
 if(a.find(L"usb4")!=std::wstring::npos)return L"USB4 class/topology detected (exact link rate not claimed)";
 if(a.find(L"superspeedplus")!=std::wstring::npos||a.find(L"super speed plus")!=std::wstring::npos)return L"SuperSpeedPlus-class hint";
 if(a.find(L"superspeed")!=std::wstring::npos||a.find(L"super speed")!=std::wstring::npos)return L"SuperSpeed-class hint";
 return L"Enumerated; exact negotiated speed UNKNOWN";
}
}
void RecalculatePortPowerSummary(PortPowerSummary&s){
 if(s.ports.empty()){s.overall=L"INCOMPLETE";return;}
 bool fail=false,incomplete=false;for(auto&p:s.ports){if(p.verdict==L"FAIL")fail=true;if(p.verdict==L"NOT TESTED"||p.verdict==L"PARTIAL")incomplete=true;}
 s.overall=fail?L"FAIL":incomplete?L"INCOMPLETE":L"PASS";
}
void CollectPortPowerBaseline(AuditReport&r){
 auto dev=Devices();auto&s=r.hardware.stress.portPower;
 s.usb4HostRouterPresent=CountMatch(dev,L"Usb4HostRouter")>0||CountMatch(dev,L"USB4 Host Router")>0;
 s.usb4DeviceRouters=CountMatch(dev,L"Usb4DeviceRouter");
 s.thunderboltDevices=CountMatch(dev,L"Thunderbolt");
 SYSTEM_POWER_STATUS ps{};if(GetSystemPowerStatus(&ps)){s.power.acConnected=ps.ACLineStatus==1;s.power.batteryPercent=ps.BatteryLifePercent==255?-1:ps.BatteryLifePercent;}
 s.power.adapterIdentity=L"UNKNOWN";s.power.adapterWatts=-1;s.power.wattageConfidence=Confidence::Low;
 s.power.verdict=s.power.acConnected?L"AC CONNECTED; WATTAGE UNKNOWN":L"AC NOT CONNECTED";
 s.power.evidence=L"Windows GetSystemPowerStatus proves AC state only. Adapter identity/wattage is not inferred without a vetted OEM/PD provider.";
 RecalculatePortPowerSummary(s);
}
PortProbeResult RunPhysicalPortProbe(HWND owner,const std::wstring&label,const std::atomic_bool*cancel){
 PortProbeResult r{};r.portLabel=label;r.verdict=L"NOT TESTED";
 MessageBoxW(owner,(L"PORT TEST: "+label+L"\n\n1. Rút thiết bị test chuẩn khỏi cổng.\n2. Bấm OK để chụp baseline.\n3. Sau đó cắm thiết bị test vào ĐÚNG cổng này.").c_str(),L"Port & Power Verification",MB_OK|MB_ICONINFORMATION);
 auto before=Devices();auto bIds=Ids(before);
 MessageBoxW(owner,L"Cắm thiết bị test chuẩn vào cổng đang kiểm tra. Chờ Windows nhận thiết bị rồi bấm OK.",L"Port & Power Verification",MB_OK|MB_ICONINFORMATION);
 for(int i=0;i<25;i++){if(cancel&&cancel->load())break;Sleep(100);}
 if(cancel&&cancel->load()){r.verdict=L"NOT TESTED";r.evidence=L"Port probe cancelled before evidence collection completed.";return r;}
 auto after=Devices();std::vector<Dev> added;for(auto&x:after)if(!bIds.count(x.id)&&IsUsbLike(x))added.push_back(x);
 r.usb4RouterSeen=CountMatch(after,L"Usb4DeviceRouter")>CountMatch(before,L"Usb4DeviceRouter")||CountMatch(after,L"USB4 Device Router")>CountMatch(before,L"USB4 Device Router");
 r.thunderboltSeen=CountMatch(after,L"Thunderbolt")>CountMatch(before,L"Thunderbolt");
 unsigned db=CountMatch(before,L"display"),da=CountMatch(after,L"display");r.displayAdapterChange=da>db;
 if(!added.empty()){
   auto&x=added.front();r.deviceEnumerated=true;r.deviceDescription=x.name;r.instanceId=x.id;r.locationPath=x.location;r.busReportedDescription=x.busDesc;
   r.negotiatedSpeed=SpeedHint(x);r.confidence=Confidence::High;r.verdict=L"PASS";
   r.evidence=L"New PnP device appeared only after stimulus. LocationPath="+x.location+L"; BusReportedDesc="+x.busDesc;
 }else{
   r.deviceEnumerated=false;r.confidence=Confidence::Medium;r.verdict=L"FAIL";
   r.evidence=L"No new USB-like PnP device enumerated after plug-in stimulus.";
 }
 if(r.usb4RouterSeen)r.evidence+=L"; USB4 device-router delta observed";
 if(r.thunderboltSeen)r.evidence+=L"; Thunderbolt device delta observed";
 if(r.displayAdapterChange)r.evidence+=L"; display-device delta observed (DP Alt Mode candidate, not conclusive)";
 return r;
}
}
