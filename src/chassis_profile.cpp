#include "lap/chassis_profile.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cwctype>
namespace lap { namespace {
std::wstring L(std::wstring s){std::transform(s.begin(),s.end(),s.begin(),towlower);return s;}
std::wstring T(std::wstring s){while(!s.empty()&&iswspace(s.front()))s.erase(s.begin());while(!s.empty()&&iswspace(s.back()))s.pop_back();return s;}
}
ChassisProfile LoadChassisProfile(const std::wstring&a,const std::wstring&m){ChassisProfile best{};std::error_code ec;auto d=std::filesystem::path(a)/L"profiles"/L"chassis";if(!std::filesystem::exists(d,ec))return best;
for(auto&e:std::filesystem::directory_iterator(d,ec)){if(ec||!e.is_regular_file())continue;std::wifstream f(e.path());ChassisProfile p{};p.source=e.path().filename().wstring();p.confidence=Confidence::Medium;std::wstring line;
while(std::getline(f,line)){line=T(line);if(line.empty()||line[0]==L'#')continue;auto q=line.find(L'=');if(q==std::wstring::npos)continue;auto k=T(line.substr(0,q)),v=T(line.substr(q+1));if(k==L"profileId")p.profileId=v;else if(k==L"modelContains")p.modelContains=v;else if(k==L"displayName")p.displayName=v;else if(k==L"validationStatus")p.validationStatus=L(v);else if(k==L"reference")p.reference=v;else if(k==L"port"){std::wstringstream ss(v);std::wstring x;std::vector<std::wstring>z;while(std::getline(ss,x,L'|'))z.push_back(T(x));if(z.size()>=6){ChassisPortDefinition c{};c.id=z[0];c.label=z[1];c.side=z[2];c.connector=z[3];c.capability=z[4];c.required=L(z[5])!=L"false";p.ports.push_back(c);}}}
if(!p.modelContains.empty()&&L(m).find(L(p.modelContains))!=std::wstring::npos&&p.modelContains.size()>best.modelContains.size())best=p;}return best;}
void ApplyPortResultToChassisProfile(ChassisProfile&p,const PortProbeResult&r){for(auto&x:p.ports)if(x.label==r.portLabel||x.id==r.portLabel){x.tested=true;x.verdict=r.verdict;return;}}
unsigned RequiredPortsRemaining(const ChassisProfile&p){unsigned n=0;for(auto&x:p.ports)if(x.required&&!x.tested)n++;return n;}
}
