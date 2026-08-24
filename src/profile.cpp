#include "lap/profile.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <iterator>
#include <cwctype>

namespace lap {
namespace {
std::string ReadUtf8(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return std::string(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>());}
std::wstring W(const std::string&s){if(s.empty())return L"";int n=MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),nullptr,0);std::wstring w(n,L'\0');if(n)MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),w.data(),n);return w;}
std::string Str(const std::string&j,const char*k){std::regex rx(std::string("\\\"")+k+"\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");std::smatch m;return std::regex_search(j,m,rx)?m[1].str():"";}
uint64_t Num(const std::string&j,const char*k,uint64_t d=0){std::regex rx(std::string("\\\"")+k+"\\\"\\s*:\\s*([0-9]+(?:\\.[0-9]+)?)");std::smatch m;if(!std::regex_search(j,m,rx))return d;return static_cast<uint64_t>(std::stod(m[1].str()));}
bool Bool(const std::string&j,const char*k,bool d=false){std::regex rx(std::string("\\\"")+k+"\\\"\\s*:\\s*(true|false)");std::smatch m;if(!std::regex_search(j,m,rx))return d;return m[1].str()=="true";}
bool EqI(std::wstring a,std::wstring b){for(auto&c:a)c=(wchar_t)towlower(c);for(auto&c:b)c=(wchar_t)towlower(c);return a==b;}
FactoryProfile Parse(const std::string& j){
 FactoryProfile p{};
 p.model=W(Str(j,"model")); p.serviceTag=W(Str(j,"serviceTag")); p.cpuContains=W(Str(j,"cpuContains"));
 p.ramBytes=Num(j,"ramBytes"); p.ramSpeed=(unsigned)Num(j,"ramSpeed"); p.gpuContains=W(Str(j,"gpuContains"));
 p.gpuVramBytes=Num(j,"gpuVramBytes"); p.diskMinBytes=Num(j,"diskMinBytes"); p.displayWidth=(unsigned)Num(j,"displayWidth");
 p.displayHeight=(unsigned)Num(j,"displayHeight"); p.touchRequired=Bool(j,"touchRequired"); p.batteryDesignWh=static_cast<double>(Num(j,"batteryDesignWh")); p.adapterW=(unsigned)Num(j,"adapterW");
 if(p.cpuContains.empty()) p.cpuContains=W(Str(j,"cpu"));
 if(p.gpuContains.empty()) p.gpuContains=W(Str(j,"gpu"));
 return p;
}
}
ProfileLoadResult LoadFactoryProfile(const std::wstring& dir,const std::wstring&,const std::wstring& tag){
 ProfileLoadResult out{}; std::filesystem::path root(dir);
 if(!std::filesystem::exists(root)){out.error=L"profiles directory not found";return out;}
 
 auto scanDir = [&](const std::filesystem::path& pth) -> bool {
  std::error_code ec;
  if (!std::filesystem::exists(pth, ec)) return false;
  for(auto& e:std::filesystem::directory_iterator(pth, ec)){
   if(ec||!e.is_regular_file()||e.path().extension()!=L".json")continue;
   auto raw=ReadUtf8(e.path()); auto p=Parse(raw); if(p.model.empty())continue;
   if(!tag.empty() && !p.serviceTag.empty() && EqI(p.serviceTag,tag)){
    out.profile=p;out.exact=true;out.loaded=true;out.source=e.path().wstring();return true;
   }
  }
  return false;
 };

 if (scanDir(root)) return out;
 if (scanDir(root / L"cache")) return out;

 out.error=L"No exact Service Tag profile; Generic Audit mode";return out;
}
}