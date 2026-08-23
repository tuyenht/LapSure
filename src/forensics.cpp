#include "lap/forensics.h"
#include "lap/process.h"
#include "lap/hardware.h"
#include "lap/edid.h"
#include <sstream>
#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>

#pragma comment(lib,"setupapi.lib")
#pragma comment(lib,"cfgmgr32.lib")

namespace lap { namespace {
void Add(AuditReport&r,std::wstring g,std::wstring n,std::wstring v,std::wstring e,State st,Severity sv,Dimension d,std::wstring ev=L""){r.findings.push_back({std::move(g),std::move(n),std::move(v),std::move(e),st,sv,std::move(ev),d});}
std::wstring B(bool v){return v?L"Yes":L"No";}

void CollectPnpProblemAudit(AuditReport& r, const std::atomic_bool* cancel){
    HDEVINFO set = SetupDiGetClassDevsW(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES);
    if(set == INVALID_HANDLE_VALUE) return;
    
    unsigned problemCount = 0;
    for(DWORD i = 0;; ++i){
        if(cancel && cancel->load()) break;
        SP_DEVINFO_DATA d{}; d.cbSize = sizeof(d);
        if(!SetupDiEnumDeviceInfo(set, i, &d)){
            if(GetLastError() == ERROR_NO_MORE_ITEMS) break;
            continue;
        }
        ULONG status = 0, problem = 0;
        CONFIGRET cr = CM_Get_DevNode_Status(&status, &problem, d.DevInst, 0);
        if(cr == CR_SUCCESS && ((status & DN_HAS_PROBLEM) || problem != 0)){
            wchar_t nameBuf[1024]{}; DWORD type = 0, need = 0;
            std::wstring name;
            if(SetupDiGetDeviceRegistryPropertyW(set, &d, SPDRP_FRIENDLYNAME, &type, (PBYTE)nameBuf, sizeof(nameBuf), &need) ||
               SetupDiGetDeviceRegistryPropertyW(set, &d, SPDRP_DEVICEDESC, &type, (PBYTE)nameBuf, sizeof(nameBuf), &need)){
                name = nameBuf;
            }
            if(name.empty()) name = L"Thiết bị PnP không tên";

            wchar_t instBuf[1024]{};
            if(CM_Get_Device_IDW(d.DevInst, instBuf, 1024, 0) != CR_SUCCESS){
                instBuf[0] = 0;
            }

            std::wstring probDesc;
            Severity sev = Severity::Major;
            State st = State::Warning;
            switch(problem){
                case 10: // CM_PROB_FAILED_START
                    probDesc = L"Mã lỗi 10 (CM_PROB_FAILED_START): Thiết bị không thể khởi động";
                    sev = Severity::Critical; st = State::Fail; break;
                case 14: // CM_PROB_NEED_RESTART
                    probDesc = L"Mã lỗi 14 (CM_PROB_NEED_RESTART): Cần khởi động lại máy";
                    sev = Severity::Minor; st = State::Warning; break;
                case 22: // CM_PROB_DISABLED
                    probDesc = L"Mã lỗi 22 (CM_PROB_DISABLED): Thiết bị đang bị vô hiệu hóa";
                    sev = Severity::Minor; st = State::Warning; break;
                case 28: // CM_PROB_FAILED_INSTALL
                    probDesc = L"Mã lỗi 28 (CM_PROB_FAILED_INSTALL): Thiếu driver điều khiển";
                    sev = Severity::Major; st = State::Warning; break;
                case 29: // CM_PROB_HARDWARE_DISABLED
                    probDesc = L"Mã lỗi 29 (CM_PROB_HARDWARE_DISABLED): Phần cứng bị tắt trong BIOS";
                    sev = Severity::Major; st = State::Warning; break;
                case 43: // CM_PROB_FAILED_POST_START
                    probDesc = L"Mã lỗi 43 (CM_PROB_FAILED_POST_START): Windows đã dừng thiết bị do sự cố phần cứng/chip (Rất nguy hiểm)";
                    sev = Severity::Critical; st = State::Fail; break;
                default:
                    probDesc = L"Mã lỗi PnP: Code " + std::to_wstring(problem);
                    sev = Severity::Major; st = State::Warning; break;
            }

            r.hardware.pnpProblems.push_back({name, name, instBuf, L"", problem, probDesc});
            Add(r, L"Driver / PnP", name + L" (Mã lỗi " + std::to_wstring(problem) + L")",
                probDesc, L"Hoạt động bình thường (Code 0)", st, sev, Dimension::Functional,
                std::wstring(L"SetupAPI CM_Get_DevNode_Status: ") + instBuf);
            problemCount++;
        }
    }
    SetupDiDestroyDeviceInfoList(set);

    if(problemCount == 0){
        Add(r, L"Driver / PnP", L"Trạng thái phần cứng PnP",
            L"Tất cả thiết bị phần cứng hoạt động tốt (0 mã lỗi PnP / Yellow Bang)",
            L"0 mã lỗi", State::Pass, Severity::Info, Dimension::Functional,
            L"Native SetupAPI + CM_Get_DevNode_Status full device tree audit");
    }
}
}

void CollectPlatformForensics(AuditReport&r,const FactoryProfile&p,const Capabilities&c,const std::wstring&,const std::atomic_bool* cancel){
 r.hardware.displays=CollectNativeDisplays();
 if(!r.hardware.displays.empty()){
   for(size_t i=0;i<r.hardware.displays.size();++i){auto&d=r.hardware.displays[i];
     std::wstringstream s;s<<d.manufacturer<<L" "<<d.friendlyName<<L" | native "<<d.nativeWidth<<L"x"<<d.nativeHeight<<L" | current "<<d.currentWidth<<L"x"<<d.currentHeight<<L"@"<<d.refreshHz<<L"Hz | SN "<<d.serialNumber;
     State st=State::Info;Dimension dim=Dimension::Identity;std::wstring expected;
     if(p.displayWidth&&p.displayHeight){expected=std::to_wstring(p.displayWidth)+L"x"+std::to_wstring(p.displayHeight);dim=Dimension::Factory;st=(d.nativeWidth==p.displayWidth&&d.nativeHeight==p.displayHeight)?State::Pass:State::Changed;}
     Add(r,L"Display",L"EDID panel "+std::to_wstring(i+1),s.str(),expected,st,Severity::Major,dim,L"Native SetupAPI registry EDID; checksum validated");
   }
 } else Add(r,L"Display",L"EDID panel identity",L"No valid EDID exposed",L"",State::NotTested,Severity::Major,Dimension::Identity,L"Native SetupAPI");

 CollectPnpProblemAudit(r, cancel);

 if(!c.powershell){Add(r,L"Platform",L"Windows extended providers",L"PowerShell unavailable; native EDID and PnP audit still completed",L"",State::Unsupported,Severity::Minor,Dimension::Evidence);return;}

 auto mb=RunProcessCapture(L"powershell.exe -NoProfile -NonInteractive -Command \"$b=Get-CimInstance Win32_BaseBoard|Select-Object -First 1;if($b){'{0}|{1}|{2}' -f $b.Manufacturer,$b.Product,$b.SerialNumber}\"",15000,cancel);
 if(mb.launched&&!mb.timedOut&&!mb.output.empty()){MainboardInfo x{};if(ParseMainboardLine(SplitLines(mb.output).front(),x)){r.hardware.mainboard=x;Add(r,L"Mainboard",L"Identity",x.manufacturer+L" | "+x.product+L" | SN "+x.serialNumber,L"",State::Info,Severity::Info,Dimension::Identity,L"Win32_BaseBoard");}}
 else Add(r,L"Mainboard",L"Identity",L"Not available",L"",State::NotTested,Severity::Minor,Dimension::Identity);

 auto bi=RunProcessCapture(L"powershell.exe -NoProfile -NonInteractive -Command \"$b=Get-CimInstance Win32_BIOS|Select-Object -First 1;if($b){'{0}|{1}|{2}|{3}.{4}' -f $b.Manufacturer,$b.SMBIOSBIOSVersion,$b.ReleaseDate,$b.SMBIOSMajorVersion,$b.SMBIOSMinorVersion}\"",15000,cancel);
 if(bi.launched&&!bi.timedOut&&!bi.output.empty()){BiosInfo x{};if(ParseBiosLine(SplitLines(bi.output).front(),x)){r.hardware.bios=x;Add(r,L"BIOS",L"Identity",x.vendor+L" | "+x.version+L" | "+x.releaseDate+L" | SMBIOS "+x.smbiosVersion,L"",State::Info,Severity::Info,Dimension::Identity,L"Win32_BIOS");}}
 else Add(r,L"BIOS",L"Identity",L"Not available",L"",State::NotTested,Severity::Minor,Dimension::Identity);

 auto tp=RunProcessCapture(L"powershell.exe -NoProfile -NonInteractive -Command \"try{$t=Get-Tpm;'{0}|{1}' -f [int]$t.TpmPresent,[int]$t.TpmReady}catch{'-1|-1'}\"",15000,cancel);
 if(tp.launched&&!tp.output.empty()){auto q=Split(SplitLines(tp.output).front(),L'|');if(q.size()>=2&&ParseI64(q[0],-1)>=0){r.hardware.security.tpmPresent=ParseI64(q[0],0)!=0;r.hardware.security.tpmReady=ParseI64(q[1],0)!=0;Add(r,L"Security",L"TPM",L"Present="+B(r.hardware.security.tpmPresent)+L" | Ready="+B(r.hardware.security.tpmReady),L"Present / Ready",r.hardware.security.tpmReady?State::Pass:State::Warning,Severity::Minor,Dimension::Functional,L"Get-Tpm");}else Add(r,L"Security",L"TPM",L"Provider unavailable",L"",State::NotTested,Severity::Minor,Dimension::Functional);}

 auto sb=RunProcessCapture(L"powershell.exe -NoProfile -NonInteractive -Command \"try{$s=Confirm-SecureBootUEFI;'1|'+[int]$s}catch{'0|0'}\"",15000,cancel);
 if(sb.launched&&!sb.output.empty()){auto q=Split(SplitLines(sb.output).front(),L'|');if(q.size()>=2){r.hardware.security.secureBootKnown=ParseI64(q[0],0)!=0;r.hardware.security.secureBootEnabled=ParseI64(q[1],0)!=0;Add(r,L"Security",L"Secure Boot",r.hardware.security.secureBootKnown?B(r.hardware.security.secureBootEnabled):L"Unknown",L"Enabled preferred",r.hardware.security.secureBootKnown?(r.hardware.security.secureBootEnabled?State::Pass:State::Warning):State::NotTested,Severity::Minor,Dimension::Functional,L"Confirm-SecureBootUEFI");}}

 auto ct=RunProcessCapture(L"powershell.exe -NoProfile -NonInteractive -Command \"$c=Get-CimInstance Win32_Processor|Select-Object -First 1;if($c){'{0}|{1}|{2}' -f $c.LoadPercentage,$c.CurrentClockSpeed,$c.MaxClockSpeed}\"",15000,cancel);
 if(ct.launched&&!ct.output.empty()){CpuTelemetry x{};if(ParseCpuTelemetryLine(SplitLines(ct.output).front(),x)){r.hardware.cpuTelemetry=x;std::wstringstream s;s<<L"Load "<<x.loadPercent<<L"% | "<<x.currentClockMHz<<L" / "<<x.maxClockMHz<<L" MHz";Add(r,L"CPU",L"Live telemetry",s.str(),L"",State::Info,Severity::Info,Dimension::Usage,L"Win32_Processor");}}

 auto touch=RunProcessCapture(L"powershell.exe -NoProfile -NonInteractive -Command \"$t=[bool](Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue|Where-Object{$_.FriendlyName -match 'touch screen|digitizer'}|Select-Object -First 1);[int]$t\"",15000,cancel);
 if(touch.launched&&!touch.output.empty()){
   bool detected=ParseI64(SplitLines(touch.output).front(),0)!=0;for(auto&d:r.hardware.displays)d.touchDetected=detected;
   State st=p.touchRequired?(detected?State::Pass:State::Changed):State::Info;
   Add(r,L"Display",L"Touch digitizer",detected?L"Detected":L"Not detected",p.touchRequired?L"Required by exact factory profile":L"",st,Severity::Major,p.touchRequired?Dimension::Factory:Dimension::Functional,L"Present PnP touch/digitizer device");
 }

 auto ev=RunProcessCapture(L"powershell.exe -NoProfile -NonInteractive -Command \"$s=(Get-Date).AddDays(-30);$ps=@('WHEA-Logger','Disk','stornvme','Ntfs','Display','BugCheck');foreach($p in $ps){try{$c=@(Get-WinEvent -FilterHashtable @{LogName='System';ProviderName=$p;StartTime=$s} -ErrorAction Stop).Count}catch{$c=0};'{0}|{1}' -f $p,$c};try{$k=@(Get-WinEvent -FilterHashtable @{LogName='System';ProviderName='Microsoft-Windows-Kernel-Power';Id=41;StartTime=$s} -ErrorAction Stop).Count}catch{$k=0};'Microsoft-Windows-Kernel-Power|{0}' -f $k\"",25000,cancel);
 if(ev.launched&&!ev.timedOut&&!ev.output.empty()){for(auto&line:SplitLines(ev.output)){auto q=Split(line,L'|');if(q.size()<2)continue;auto n=ParseI64(q[1],0);if(q[0]==L"WHEA-Logger")r.hardware.events.whea=n;else if(q[0]==L"Disk")r.hardware.events.disk=n;else if(q[0]==L"stornvme")r.hardware.events.stornvme=n;else if(q[0]==L"Ntfs")r.hardware.events.ntfs=n;else if(q[0]==L"Display")r.hardware.events.display=n;else if(q[0]==L"Microsoft-Windows-Kernel-Power")r.hardware.events.kernelPower=n;else if(q[0]==L"BugCheck")r.hardware.events.bugCheck=n;}
   std::wstringstream x;x<<L"WHEA "<<r.hardware.events.whea<<L" | Disk "<<r.hardware.events.disk<<L" | NVMe "<<r.hardware.events.stornvme<<L" | NTFS "<<r.hardware.events.ntfs<<L" | Display "<<r.hardware.events.display<<L" | KernelPower41 "<<r.hardware.events.kernelPower<<L" | BugCheck "<<r.hardware.events.bugCheck;
   State st=(r.hardware.events.whea||r.hardware.events.disk||r.hardware.events.stornvme||r.hardware.events.ntfs||r.hardware.events.display||r.hardware.events.bugCheck)?State::Warning:State::Pass;
   Add(r,L"Forensics",L"Historical hardware events / 30d",x.str(),L"0 preferred",st,Severity::Major,Dimension::Evidence,L"Historical evidence only; KernelPower counts unexpected-restart Event ID 41, not all provider events; not a current-failure verdict");
 }
}
}
