#include "lap/environment.h"
#include "lap/trust.h"
#include <windows.h>
#include <wbemidl.h>
#include <comdef.h>
#pragma comment(lib,"wbemuuid.lib")

namespace lap {
static bool Exists(const std::wstring& p){return GetFileAttributesW(p.c_str())!=INVALID_FILE_ATTRIBUTES;}
static bool CommandExists(const wchar_t* exe){wchar_t buf[MAX_PATH]{};return SearchPathW(nullptr,exe,nullptr,MAX_PATH,buf,nullptr)>0;}
static bool WmiAvailable(){
 HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED); bool uninit=SUCCEEDED(hr);
 IWbemLocator* loc=nullptr; bool ok=false;
 if(SUCCEEDED(CoCreateInstance(CLSID_WbemLocator,nullptr,CLSCTX_INPROC_SERVER,IID_IWbemLocator,(void**)&loc))&&loc){
  IWbemServices* svc=nullptr; BSTR ns=SysAllocString(L"ROOT\\CIMV2");
  ok=SUCCEEDED(loc->ConnectServer(ns,nullptr,nullptr,nullptr,0,nullptr,nullptr,&svc));
  SysFreeString(ns); if(svc)svc->Release(); loc->Release();
 }
 if(uninit)CoUninitialize(); return ok;
}
Capabilities DetectCapabilities(const std::wstring& appDir){
 Capabilities c{};
 HKEY h{};c.winPE=RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"SYSTEM\\CurrentControlSet\\Control\\MiniNT",0,KEY_READ,&h)==ERROR_SUCCESS;if(h)RegCloseKey(h);
 c.powershell=CommandExists(L"powershell.exe");
 const auto nvidiaTrust=VerifyEngine(appDir,L"tools\\nvidia-smi.exe",L"nvidia_smi");
 const auto smartctlTrust=VerifyEngine(appDir,L"tools\\smartctl.exe",L"smartctl");
 c.nvidiaSmi=nvidiaTrust.hashMatches;
 c.smartctl=smartctlTrust.hashMatches;
 c.wmi=WmiAvailable();
 SYSTEM_POWER_STATUS ps{};c.battery=GetSystemPowerStatus(&ps)&&ps.BatteryFlag!=128;
 return c;
}
std::wstring EnvironmentName(const Capabilities& c){return c.winPE?L"Windows PE":L"Windows";}
}
