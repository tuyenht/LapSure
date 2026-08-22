#include "lap/edid.h"
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#pragma comment(lib,"setupapi.lib")

namespace lap {
namespace {
std::wstring TrimDescriptor(const unsigned char* p){
    std::string s; for(int i=0;i<13;i++){char c=(char)p[i]; if(c=='\n'||c=='\r'||c==0)break; s+=c;}
    while(!s.empty()&&(s.back()==' '||s.back()=='\t'))s.pop_back();
    if(s.empty())return L"";
    int n=MultiByteToWideChar(CP_ACP,0,s.data(),(int)s.size(),nullptr,0);std::wstring w(n,L'\0');
    if(n)MultiByteToWideChar(CP_ACP,0,s.data(),(int)s.size(),w.data(),n);return w;
}
std::wstring Mfg(unsigned code){
    wchar_t s[4]{};
    s[0]=(wchar_t)(((code>>10)&31)+L'A'-1);s[1]=(wchar_t)(((code>>5)&31)+L'A'-1);s[2]=(wchar_t)((code&31)+L'A'-1);
    for(int i=0;i<3;i++)if(s[i]<L'A'||s[i]>L'Z')return L"";
    return s;
}
std::wstring Hex(const unsigned char*d,size_t n){
    std::wstringstream s;s<<std::hex<<std::setfill(L'0');
    for(size_t i=0;i<n;i++)s<<std::setw(2)<<(unsigned)d[i];
    return s.str();
}
}
EdidIdentity ParseEdid(const unsigned char*d,size_t n){
    EdidIdentity e{}; if(!d||n<128)return e;
    static const unsigned char hdr[8]={0,0xff,0xff,0xff,0xff,0xff,0xff,0};
    if(!std::equal(hdr,hdr+8,d))return e;
    unsigned sum=0;for(size_t i=0;i<128;i++)sum+=d[i]; if((sum&0xff)!=0)return e;
    unsigned m=(d[8]<<8)|d[9];e.manufacturer=Mfg(m);e.productCode=d[10]|(d[11]<<8);
    e.serialNumeric=(unsigned)d[12]|((unsigned)d[13]<<8)|((unsigned)d[14]<<16)|((unsigned)d[15]<<24);
    e.manufactureWeek=d[16];e.manufactureYear=1990u+d[17];e.edidHex=Hex(d,n);
    for(int off=54;off<=108;off+=18){
        if(d[off]==0&&d[off+1]==0){
            if(d[off+3]==0xfc)e.monitorName=TrimDescriptor(d+off+5);
            else if(d[off+3]==0xff)e.serialText=TrimDescriptor(d+off+5);
            continue;
        }
        unsigned h=d[off+2]|((d[off+4]&0xF0)<<4);
        unsigned v=d[off+5]|((d[off+7]&0xF0)<<4);
        if(h&&v&&!e.nativeWidth){e.nativeWidth=h;e.nativeHeight=v;}
    }
    e.valid=!e.manufacturer.empty();return e;
}
std::vector<DisplayInfo> CollectNativeDisplays(){
    std::vector<DisplayInfo> out;
    HDEVINFO set=SetupDiGetClassDevsW(&GUID_DEVCLASS_MONITOR,nullptr,nullptr,DIGCF_PRESENT);
    if(set==INVALID_HANDLE_VALUE)return out;
    for(DWORD i=0;;++i){
        SP_DEVINFO_DATA dev{};dev.cbSize=sizeof(dev);if(!SetupDiEnumDeviceInfo(set,i,&dev)){if(GetLastError()==ERROR_NO_MORE_ITEMS)break;continue;}
        wchar_t instance[512]{};SetupDiGetDeviceInstanceIdW(set,&dev,instance,512,nullptr);
        HKEY key=SetupDiOpenDevRegKey(set,&dev,DICS_FLAG_GLOBAL,0,DIREG_DEV,KEY_READ);
        if(key==INVALID_HANDLE_VALUE)continue;
        DWORD type=0,size=0;if(RegQueryValueExW(key,L"EDID",nullptr,&type,nullptr,&size)!=ERROR_SUCCESS||type!=REG_BINARY||size<128){RegCloseKey(key);continue;}
        std::vector<unsigned char> bytes(size);if(RegQueryValueExW(key,L"EDID",nullptr,&type,bytes.data(),&size)!=ERROR_SUCCESS){RegCloseKey(key);continue;}RegCloseKey(key);
        auto e=ParseEdid(bytes.data(),bytes.size());if(!e.valid)continue;
        DisplayInfo d{};d.manufacturer=e.manufacturer;d.friendlyName=e.monitorName;d.serialNumber=!e.serialText.empty()?e.serialText:std::to_wstring(e.serialNumeric);
        d.instanceName=instance;d.nativeWidth=e.nativeWidth;d.nativeHeight=e.nativeHeight;d.edidHex=e.edidHex;
        d.internalPanel=(d.nativeWidth>0&&d.nativeHeight>0);out.push_back(std::move(d));
    }
    SetupDiDestroyDeviceInfoList(set);
    DEVMODEW dm{};dm.dmSize=sizeof(dm);if(EnumDisplaySettingsW(nullptr,ENUM_CURRENT_SETTINGS,&dm)){
        for(auto&d:out){d.currentWidth=dm.dmPelsWidth;d.currentHeight=dm.dmPelsHeight;d.refreshHz=dm.dmDisplayFrequency;}
    }
    return out;
}
}