#include "lap/chassis_profile.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cwctype>
namespace lap { namespace {
std::wstring L(std::wstring s){std::transform(s.begin(),s.end(),s.begin(),towlower);return s;}
std::wstring T(std::wstring s){while(!s.empty()&&iswspace(s.front()))s.erase(s.begin());while(!s.empty()&&iswspace(s.back()))s.pop_back();return s;}

ChassisProfile SynthesizeDellChassisProfile(const std::wstring& model) {
    ChassisProfile p{};
    std::wstring lower = L(model);
    p.confidence = Confidence::Medium;
    p.validationStatus = L"heuristic_dell";
    p.source = L"Dell Universal Architecture Engine";
    p.displayName = model.empty() ? L"Dell Laptop" : model;
    
    if (lower.find(L"xps 13") != std::wstring::npos || lower.find(L"93") != std::wstring::npos) {
        p.profileId = L"dell_xps_13_universal";
        p.modelContains = L"XPS 13";
        p.ports.push_back({L"left_tb", L"Left Thunderbolt / USB-C", L"Left", L"USB-C", L"Thunderbolt / USB-C with Power Delivery", true, false, L""});
        p.ports.push_back({L"right_tb", L"Right Thunderbolt / USB-C", L"Right", L"USB-C", L"Thunderbolt / USB-C with Power Delivery", true, false, L""});
        p.ports.push_back({L"microsd", L"MicroSD Card Slot", L"Left", L"MicroSD", L"MicroSD card reader", false, false, L""});
    } else if (lower.find(L"xps 15") != std::wstring::npos || lower.find(L"precision 55") != std::wstring::npos || lower.find(L"precision 56") != std::wstring::npos) {
        p.profileId = L"dell_xps_15_precision_universal";
        p.modelContains = L"XPS 15 / Precision 5500";
        p.ports.push_back({L"left_tb1", L"Left Thunderbolt #1", L"Left", L"USB-C", L"Thunderbolt / USB4 / Power Delivery", true, false, L""});
        p.ports.push_back({L"left_tb2", L"Left Thunderbolt #2", L"Left", L"USB-C", L"Thunderbolt / USB4 / Power Delivery", true, false, L""});
        p.ports.push_back({L"right_usbc", L"Right USB-C (DP / Power Delivery)", L"Right", L"USB-C", L"USB-C with DisplayPort and Power Delivery", true, false, L""});
        p.ports.push_back({L"right_sd", L"Right SD Card Reader", L"Right", L"SD", L"Full-size SD card slot", true, false, L""});
    } else if (lower.find(L"precision 7") != std::wstring::npos || lower.find(L"75") != std::wstring::npos || lower.find(L"77") != std::wstring::npos) {
        p.profileId = L"dell_precision_7000_universal";
        p.modelContains = L"Precision 7000";
        p.ports.push_back({L"left_tb1", L"Left Thunderbolt #1", L"Left", L"USB-C", L"Thunderbolt / DP / Power Delivery", true, false, L""});
        p.ports.push_back({L"left_tb2", L"Left Thunderbolt #2", L"Left", L"USB-C", L"Thunderbolt / DP / Power Delivery", true, false, L""});
        p.ports.push_back({L"usb_a1", L"USB 3.2 Gen 1 (PowerShare)", L"Right", L"USB-A", L"USB 3.2 with PowerShare", true, false, L""});
        p.ports.push_back({L"usb_a2", L"USB 3.2 Gen 1 #2", L"Right", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"hdmi", L"HDMI Video Output", L"Back", L"HDMI", L"HDMI 2.0 / 2.1", true, false, L""});
        p.ports.push_back({L"rj45", L"RJ-45 Ethernet LAN", L"Back", L"RJ45", L"Gigabit Ethernet LAN", true, false, L""});
        p.ports.push_back({L"sd", L"SD Card Reader", L"Right", L"SD", L"Full-size SD Card Slot", true, false, L""});
    } else if (lower.find(L"g15") != std::wstring::npos || lower.find(L"alienware") != std::wstring::npos || lower.find(L"g3") != std::wstring::npos || lower.find(L"g5") != std::wstring::npos || lower.find(L"g7") != std::wstring::npos) {
        p.profileId = L"dell_gaming_universal";
        p.modelContains = L"Dell Gaming / Alienware";
        p.ports.push_back({L"usbc", L"Thunderbolt / USB-C with DisplayPort", L"Back", L"USB-C", L"Thunderbolt / DisplayPort alt mode", true, false, L""});
        p.ports.push_back({L"hdmi", L"HDMI Video Output", L"Back", L"HDMI", L"HDMI 2.1 Video Output", true, false, L""});
        p.ports.push_back({L"rj45", L"RJ-45 Ethernet LAN", L"Left", L"RJ45", L"High-speed Ethernet LAN", true, false, L""});
        p.ports.push_back({L"usb_a1", L"USB 3.2 Gen 1 #1", L"Right", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"usb_a2", L"USB 3.2 Gen 1 #2", L"Right", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
    } else {
        // Universal Dell Latitude / Inspiron / Vostro standard layout
        p.profileId = L"dell_business_universal";
        p.modelContains = L"Dell Business/Consumer";
        p.ports.push_back({L"usbc", L"USB-C / Thunderbolt with Power Delivery", L"Left", L"USB-C", L"USB-C / Thunderbolt / DisplayPort / Power Delivery", true, false, L""});
        p.ports.push_back({L"hdmi", L"HDMI Video Output", L"Left", L"HDMI", L"HDMI Video Output", true, false, L""});
        p.ports.push_back({L"usb_a1", L"USB 3.2 Gen 1 (PowerShare)", L"Right", L"USB-A", L"USB 3.2 Gen 1 with PowerShare", true, false, L""});
        p.ports.push_back({L"usb_a2", L"USB 3.2 Gen 1 #2", L"Right", L"USB-A", L"USB 3.2 Gen 1", true, false, L""});
        p.ports.push_back({L"rj45", L"RJ-45 Ethernet LAN", L"Right", L"RJ45", L"Gigabit Ethernet LAN", false, false, L""});
        p.ports.push_back({L"sd", L"SD / MicroSD Card Slot", L"Right", L"MicroSD", L"SD or MicroSD card reader", false, false, L""});
    }
    return p;
}
}
ChassisProfile LoadChassisProfile(const std::wstring&a,const std::wstring&m){
 ChassisProfile best{};std::error_code ec;auto d=std::filesystem::path(a)/L"profiles"/L"chassis";
 if(!std::filesystem::exists(d,ec)){
  auto cur=std::filesystem::path(a);
  for(int depth=0;depth<5&&cur.has_parent_path();++depth){
   cur=cur.parent_path();
   auto alt=cur/L"profiles"/L"chassis";
   if(std::filesystem::exists(alt,ec)){d=alt;break;}
  }
 }
 if(std::filesystem::exists(d,ec)){
  for(auto&e:std::filesystem::directory_iterator(d,ec)){
   if(ec||!e.is_regular_file())continue;
   std::wifstream f(e.path());ChassisProfile p{};p.source=e.path().filename().wstring();p.confidence=Confidence::Medium;std::wstring line;
   while(std::getline(f,line)){
    line=T(line);if(line.empty()||line[0]==L'#')continue;auto q=line.find(L'=');if(q==std::wstring::npos)continue;auto k=T(line.substr(0,q)),v=T(line.substr(q+1));
    if(k==L"profileId")p.profileId=v;else if(k==L"modelContains")p.modelContains=v;else if(k==L"displayName")p.displayName=v;else if(k==L"validationStatus")p.validationStatus=L(v);else if(k==L"reference")p.reference=v;
    else if(k==L"port"){std::wstringstream ss(v);std::wstring x;std::vector<std::wstring>z;while(std::getline(ss,x,L'|'))z.push_back(T(x));if(z.size()>=6){ChassisPortDefinition c{};c.id=z[0];c.label=z[1];c.side=z[2];c.connector=z[3];c.capability=z[4];c.required=L(z[5])!=L"false";p.ports.push_back(c);}}
   }
   if(!p.modelContains.empty()&&L(m).find(L(p.modelContains))!=std::wstring::npos&&p.modelContains.size()>best.modelContains.size())best=p;
  }
 }
 if(best.profileId.empty() && !m.empty()){
  std::wstring lm = L(m);
  if(lm.find(L"dell")!=std::wstring::npos||lm.find(L"latitude")!=std::wstring::npos||lm.find(L"precision")!=std::wstring::npos||lm.find(L"xps")!=std::wstring::npos||lm.find(L"inspiron")!=std::wstring::npos||lm.find(L"vostro")!=std::wstring::npos||lm.find(L"alienware")!=std::wstring::npos||lm.find(L"g15")!=std::wstring::npos||lm.find(L"g16")!=std::wstring::npos||lm.find(L"g3")!=std::wstring::npos||lm.find(L"g5")!=std::wstring::npos){
   best = SynthesizeDellChassisProfile(m);
  }
 }
 return best;
}
void ApplyPortResultToChassisProfile(ChassisProfile&p,const PortProbeResult&r){for(auto&x:p.ports)if(x.label==r.portLabel||x.id==r.portLabel){x.tested=true;x.verdict=r.verdict;return;}}
unsigned RequiredPortsRemaining(const ChassisProfile&p){unsigned n=0;for(auto&x:p.ports)if(x.required&&!x.tested)n++;return n;}
}
