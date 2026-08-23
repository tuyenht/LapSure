#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <exception>
#include "lap/inventory.h"
#include "lap/environment.h"
#include "lap/engines.h"
#include "lap/forensics.h"
#include "lap/stress.h"
#include "lap/functional.h"
#include "lap/functional_io.h"
#include "lap/port_power.h"
#include "lap/orchestrator.h"
#include "lap/chassis_profile.h"
#include "lap/runtime_validation.h"
#include "lap/port_selector.h"
#include "lap/profile.h"
#include "lap/report.h"
#include "lap/scoring.h"
#include "lap/process.h"
#include "lap/hardware.h"

#pragma comment(lib,"Comctl32.lib")
using namespace lap;
namespace {
constexpr UINT WM_AUDIT_DONE=WM_APP+1;
constexpr UINT WM_AUDIT_STATUS=WM_APP+2;
AuditReport gReport; std::wstring gDir,gReportPath; HWND gList,gStatus,gBtn,gOpen,gMode,gFuncDisplay,gFuncKeyboard,gFuncTouch,gFuncSpeaker,gFuncUsb,gFuncIo,gPortTest,gNext,gProgress;
std::thread gWorker; std::atomic_bool gCancel{false},gRunning{false}; std::wstring gSelectedMode=L"Quick"; std::atomic_bool gAuditReady{false}; std::atomic_bool gCloseRequested{false}; std::mutex gReportMutex;
COLORREF bg=RGB(245,247,250),text=RGB(24,31,42),accent=RGB(44,104,255);
std::wstring AppDir(){wchar_t p[MAX_PATH]{};GetModuleFileNameW(nullptr,p,MAX_PATH);return std::filesystem::path(p).parent_path().wstring();}
std::wstring Reg(const wchar_t* name){HKEY h{};if(RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"HARDWARE\\DESCRIPTION\\System\\BIOS",0,KEY_READ,&h)!=ERROR_SUCCESS)return L"";wchar_t b[512]{};DWORD sz=sizeof(b),t=0;std::wstring s;if(RegQueryValueExW(h,name,nullptr,&t,(LPBYTE)b,&sz)==ERROR_SUCCESS)s=b;RegCloseKey(h);return s;}
std::wstring ServiceTag(const Capabilities&caps,const std::atomic_bool*cancel){auto tag=Reg(L"SystemSerialNumber");if(tag.empty()&&caps.powershell){auto p=RunProcessCapture(L"powershell.exe -NoProfile -NonInteractive -Command \"(Get-CimInstance Win32_BIOS|Select-Object -First 1).SerialNumber\"",10000,cancel);auto lines=SplitLines(p.output);if(p.launched&&!p.timedOut&&!lines.empty())tag=lines.front();}return tag;}
int RunInventoryOnly(const std::wstring&outputDir){
  // inventory_only_begin: this validation preflight must never invoke the stress pipeline.
  std::atomic_bool cancel{false};auto caps=DetectCapabilities(gDir);auto model=Reg(L"SystemProductName"),tag=ServiceTag(caps,&cancel);
  auto pl=LoadFactoryProfile(gDir+L"\\profiles",model,tag);FactoryProfile profile=pl.loaded?pl.profile:FactoryProfile{};
  auto report=CollectInventory(profile,caps,gDir,&cancel);report.profileSource=pl.source;report.factoryExact=pl.exact;report.genericMode=!pl.exact;
  CollectNvidia(report,profile,caps,gDir,&cancel);CollectWindowsStorageReliability(report,caps,&cancel);CollectSmartctl(report,profile,caps,gDir,&cancel);
  CollectPlatformForensics(report,profile,caps,gDir,&cancel);CollectFunctionalPresence(report,caps,&cancel);CollectPortPowerBaseline(report);
  report.hardware.stress.chassisProfile=LoadChassisProfile(gDir,report.model);RunRuntimeValidation(report,caps,gDir);
  report.findings.push_back({L"Validation",L"Inventory-only preflight",L"COMPLETED",L"No stress stages executed",State::Warning,Severity::Info,L"Explicit --inventory-only mode; verdict must remain incomplete.",Dimension::Health});
  report.hardware.stress.decision=BuildAuditDecision(report);BuildOrchestrator(report,false,false);
  auto out=outputDir.empty()?ResolveReportDirectory(gDir,caps.winPE):std::filesystem::absolute(outputDir).wstring();
  auto html=SaveHtmlReport(report,out),json=SaveJsonReport(report,out);
  // inventory_only_end
  return html.empty()||json.empty()?2:0;
}
void Fill(){std::lock_guard<std::mutex> lk(gReportMutex);ListView_DeleteAllItems(gList);int i=0;for(auto&x:gReport.findings){LVITEMW it{};it.mask=LVIF_TEXT;it.iItem=i;it.pszText=(LPWSTR)ToString(x.dimension);ListView_InsertItem(gList,&it);ListView_SetItemText(gList,i,1,(LPWSTR)x.group.c_str());ListView_SetItemText(gList,i,2,(LPWSTR)x.name.c_str());ListView_SetItemText(gList,i,3,(LPWSTR)x.value.c_str());ListView_SetItemText(gList,i,4,(LPWSTR)ToString(x.state));++i;}}
void PostStatus(HWND h,const std::wstring&s){auto* heap=new std::wstring(s);PostMessageW(h,WM_AUDIT_STATUS,0,(LPARAM)heap);}

void SetFunctionalButtonsEnabled(BOOL enabled){
    if(gFuncDisplay)EnableWindow(gFuncDisplay,enabled);
    if(gFuncKeyboard)EnableWindow(gFuncKeyboard,enabled);
    if(gFuncTouch)EnableWindow(gFuncTouch,enabled);
    if(gFuncSpeaker)EnableWindow(gFuncSpeaker,enabled);
    if(gFuncUsb)EnableWindow(gFuncUsb,enabled);
    if(gFuncIo)EnableWindow(gFuncIo,enabled);
    if(gPortTest)EnableWindow(gPortTest,enabled);
}
void RebuildDecisionAndReports(){
    std::lock_guard<std::mutex> lk(gReportMutex);
    gReport.hardware.stress.decision=BuildAuditDecision(gReport);
    BuildOrchestrator(gReport,gRunning.load(),gAuditReady.load());
    auto caps=DetectCapabilities(gDir);
    auto out=ResolveReportDirectory(gDir,caps.winPE);
    gReportPath=SaveHtmlReport(gReport,out);
    SaveJsonReport(gReport,out);
}

void AuditWorkerCore(HWND h){
  gCancel=false; PostStatus(h,L"Đang nhận diện môi trường và cấu hình máy...");
  auto caps=DetectCapabilities(gDir); auto model=Reg(L"SystemProductName"),tag=ServiceTag(caps,&gCancel);
  auto pl=LoadFactoryProfile(gDir+L"\\profiles",model,tag); FactoryProfile profile=pl.loaded?pl.profile:FactoryProfile{};
  PostStatus(h,pl.exact?L"Đã tìm thấy factory profile chính xác theo Service Tag.":L"Không có profile chính xác — chạy Generic Audit, không gắn FAIL factory giả.");
  auto report=CollectInventory(profile,caps,gDir,&gCancel); report.profileSource=pl.source; report.factoryExact=pl.exact; report.genericMode=!pl.exact;
  if(!gCancel){PostStatus(h,L"Đang đọc GPU telemetry...");CollectNvidia(report,profile,caps,gDir,&gCancel);}
  if(!gCancel){PostStatus(h,L"Đang đọc sức khỏe ổ đĩa bằng Windows...");CollectWindowsStorageReliability(report,caps,&gCancel);}
  if(!gCancel){PostStatus(h,L"Đang đọc SMART/NVMe cho từng ổ...");CollectSmartctl(report,profile,caps,gDir,&gCancel);}
  if(!gCancel){PostStatus(h,L"Đang đọc màn hình, BIOS, TPM và Event Log...");CollectPlatformForensics(report,profile,caps,gDir,&gCancel);CollectFunctionalPresence(report,caps,&gCancel);CollectPortPowerBaseline(report);report.hardware.stress.chassisProfile=LoadChassisProfile(gDir,report.model);}
  if(!gCancel){PostStatus(h,L"Đang chạy "+gSelectedMode+L" Stress: CPU + RAM + GPU/VRAM stability...");RunStressSession(report,caps,gDir,MakeStressPlan(gSelectedMode),&gCancel);}
  if(!gCancel){PostStatus(h,L"Đang chạy Runtime Validation Gate...");RunRuntimeValidation(report,caps,gDir);}
  if(!gCancel){report.hardware.stress.decision=BuildAuditDecision(report);BuildOrchestrator(report,false,true);}
  std::wstring reportPath;
  if(!gCancel){auto out=ResolveReportDirectory(gDir,caps.winPE);reportPath=SaveHtmlReport(report,out);SaveJsonReport(report,out);}
  {
    std::lock_guard<std::mutex> lk(gReportMutex);gReport=std::move(report);gReportPath=std::move(reportPath);
  }
  gAuditReady=!gCancel;gRunning=false;PostMessageW(h,WM_AUDIT_DONE,gCancel?1:0,0);
}
void AuditWorker(HWND h){try{AuditWorkerCore(h);}catch(const std::exception&){gAuditReady=false;gRunning=false;PostStatus(h,L"Audit failed with a runtime exception; no acceptance verdict was issued.");PostMessageW(h,WM_AUDIT_DONE,1,0);}catch(...){gAuditReady=false;gRunning=false;PostStatus(h,L"Audit failed with an unexpected runtime error; no acceptance verdict was issued.");PostMessageW(h,WM_AUDIT_DONE,1,0);}}
void UpsertFunctional(const FunctionalItemResult&x){
    std::lock_guard<std::mutex> lk(gReportMutex);
    auto&items=gReport.hardware.stress.functional.items;
    for(auto&v:items)if(v.id==x.id){v=x;RecalculateFunctionalSummary(gReport.hardware.stress.functional);return;}
    items.push_back(x);RecalculateFunctionalSummary(gReport.hardware.stress.functional);
}
bool CanRunManualTest(HWND h){
    if(gRunning){MessageBoxW(h,L"Automated audit is still running. Wait until it finishes before interactive functional tests.",L"LapSure",MB_OK|MB_ICONINFORMATION);return false;}
    if(!gAuditReady){MessageBoxW(h,L"Run KIỂM TRA TOÀN BỘ first. Interactive results are attached to the current audit snapshot and report.",L"LapSure",MB_OK|MB_ICONINFORMATION);return false;}
    return true;
}
void CommitManualResult(const FunctionalItemResult&x){UpsertFunctional(x);RebuildDecisionAndReports();Fill();}
void CommitManualResults(const std::vector<FunctionalItemResult>&xs){for(auto&x:xs)UpsertFunctional(x);RebuildDecisionAndReports();Fill();}
void UpsertPortResult(const PortProbeResult&x){auto&ports=gReport.hardware.stress.portPower.ports;for(auto&port:ports)if(port.portLabel==x.portLabel){port=x;return;}ports.push_back(x);}
void CommitPortResultGuided(const PortProbeResult&x){
 {std::lock_guard<std::mutex> lk(gReportMutex);UpsertPortResult(x);ApplyPortResultToChassisProfile(gReport.hardware.stress.chassisProfile,x);RecalculatePortPowerSummary(gReport.hardware.stress.portPower);}RebuildDecisionAndReports();Fill();
}
void CommitPortResult(const PortProbeResult&x){
    {std::lock_guard<std::mutex> lk(gReportMutex);UpsertPortResult(x);RecalculatePortPowerSummary(gReport.hardware.stress.portPower);}RebuildDecisionAndReports();Fill();
}
void StartAudit(HWND h){if(gRunning){gCancel=true;SetWindowTextW(gStatus,L"Đang yêu cầu dừng kiểm tra...");return;}if(gWorker.joinable())gWorker.join();
wchar_t modeBuf[32]=L"Quick";int sel=(int)SendMessageW(gMode,CB_GETCURSEL,0,0);if(sel>=0)SendMessageW(gMode,CB_GETLBTEXT,sel,(LPARAM)modeBuf);gSelectedMode=modeBuf;
gRunning=true;gCancel=false;gAuditReady=false;EnableWindow(gOpen,FALSE);SetFunctionalButtonsEnabled(FALSE);SetWindowTextW(gBtn,L"DỪNG KIỂM TRA");SetWindowTextW(gStatus,L"Khởi động audit...");gWorker=std::thread(AuditWorker,h);}
}

static LRESULT CALLBACK WndProc(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){
case WM_CREATE:{
 gMode=CreateWindowW(L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST,28,82,150,160,h,(HMENU)3,nullptr,nullptr);
 SendMessageW(gMode,CB_ADDSTRING,0,(LPARAM)L"Quick");SendMessageW(gMode,CB_ADDSTRING,0,(LPARAM)L"Standard");SendMessageW(gMode,CB_ADDSTRING,0,(LPARAM)L"Deep");SendMessageW(gMode,CB_SETCURSEL,0,0);
 gBtn=CreateWindowW(L"BUTTON",L"KIỂM TRA TOÀN BỘ",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,200,82,230,48,h,(HMENU)1,nullptr,nullptr);
 gOpen=CreateWindowW(L"BUTTON",L"Mở báo cáo",WS_CHILD|WS_VISIBLE,442,82,120,48,h,(HMENU)2,nullptr,nullptr);EnableWindow(gOpen,FALSE);
 gFuncDisplay=CreateWindowW(L"BUTTON",L"Test màn",WS_CHILD|WS_VISIBLE,580,82,84,30,h,(HMENU)1201,nullptr,nullptr);EnableWindow(gFuncDisplay,FALSE);
 gFuncKeyboard=CreateWindowW(L"BUTTON",L"Keyboard",WS_CHILD|WS_VISIBLE,670,82,84,30,h,(HMENU)1202,nullptr,nullptr);EnableWindow(gFuncKeyboard,FALSE);
 gFuncTouch=CreateWindowW(L"BUTTON",L"Touch",WS_CHILD|WS_VISIBLE,760,82,70,30,h,(HMENU)1203,nullptr,nullptr);EnableWindow(gFuncTouch,FALSE);
 gFuncSpeaker=CreateWindowW(L"BUTTON",L"Speaker",WS_CHILD|WS_VISIBLE,836,82,76,30,h,(HMENU)1204,nullptr,nullptr);EnableWindow(gFuncSpeaker,FALSE);
 gFuncUsb=CreateWindowW(L"BUTTON",L"USB Port",WS_CHILD|WS_VISIBLE,918,82,84,30,h,(HMENU)1205,nullptr,nullptr);EnableWindow(gFuncUsb,FALSE);
 gFuncIo=CreateWindowW(L"BUTTON",L"I/O AUTO",WS_CHILD|WS_VISIBLE,580,116,120,28,h,(HMENU)1206,nullptr,nullptr);EnableWindow(gFuncIo,FALSE);
 gPortTest=CreateWindowW(L"BUTTON",L"TEST PORT",WS_CHILD|WS_VISIBLE,706,116,120,28,h,(HMENU)1207,nullptr,nullptr);EnableWindow(gPortTest,FALSE);
 gNext=CreateWindowW(L"BUTTON",L"TIẾP TỤC BƯỚC KẾ",WS_CHILD|WS_VISIBLE,836,116,166,28,h,(HMENU)1300,nullptr,nullptr);EnableWindow(gNext,FALSE);
 gProgress=CreateWindowW(L"STATIC",L"Quy trình: chưa bắt đầu",WS_CHILD|WS_VISIBLE,580,148,422,42,h,(HMENU)1301,nullptr,nullptr);
 gStatus=CreateWindowW(L"STATIC",L"Sẵn sàng",WS_CHILD|WS_VISIBLE,28,145,900,24,h,nullptr,nullptr,nullptr);
 gList=CreateWindowW(WC_LISTVIEWW,L"",WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SINGLESEL,28,185,1100,490,h,nullptr,nullptr,nullptr);
 ListView_SetExtendedListViewStyle(gList,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_GRIDLINES);
 for(auto [t,cx]:std::vector<std::pair<const wchar_t*,int>>{{L"Lớp",100},{L"Nhóm",100},{L"Hạng mục",180},{L"Giá trị thực tế",560},{L"Trạng thái",120}}){LVCOLUMNW c{};c.mask=LVCF_TEXT|LVCF_WIDTH;c.pszText=(LPWSTR)t;c.cx=cx;ListView_InsertColumn(gList,Header_GetItemCount(ListView_GetHeader(gList)),&c);}return 0;}
case WM_COMMAND:{
 int id=LOWORD(w);
 if(id==1)StartAudit(h);
 else if(id==2&&!gReportPath.empty())ShellExecuteW(h,L"open",gReportPath.c_str(),nullptr,nullptr,SW_SHOW);
 else if(id==1201){if(CanRunManualTest(h))CommitManualResult(RunDisplayColorWizard(h));return 0;}
 else if(id==1202){if(CanRunManualTest(h))CommitManualResult(RunKeyboardWizard(h));return 0;}
 else if(id==1203){if(CanRunManualTest(h)){auto caps=DetectCapabilities(gDir);auto fc=DetectFunctionalCapabilities(caps,&gCancel);CommitManualResult(RunTouchGridWizard(h,fc.touchPresent));}return 0;}
 else if(id==1204){if(CanRunManualTest(h)){auto caps=DetectCapabilities(gDir);auto fc=DetectFunctionalCapabilities(caps,&gCancel);CommitManualResult(RunSpeakerWizard(h,fc.audioPresent));}return 0;}
 else if(id==1205){if(CanRunManualTest(h)){auto caps=DetectCapabilities(gDir);CommitManualResult(RunUsbPortWizard(h,caps,&gCancel));}return 0;}
 else if(id==1206){if(CanRunManualTest(h))CommitManualResults(RunFunctionalIoWizard(h));return 0;}
 else if(id==1207){if(CanRunManualTest(h)){wchar_t label[64]=L"USB-C / USB-A port";CommitPortResult(RunPhysicalPortProbe(h,label,&gCancel));}return 0;}
 else if(id==1300){if(!CanRunManualTest(h))return 0;BuildOrchestrator(gReport,false,true);auto&f=gReport.hardware.stress.functional;
  if(f.manualRequired||f.notTested)CommitManualResults(RunFunctionalIoWizard(h));
  else if(gReport.hardware.stress.portPower.overall!=L"PASS"){std::wstring label=L"USB-C / USB-A port",cap;auto prof=gReport.hardware.stress.chassisProfile;if(!prof.ports.empty()&&!SelectNextChassisPort(h,prof,label,cap))return 0;CommitPortResultGuided(RunPhysicalPortProbe(h,label,&gCancel));}
  else if(!gReportPath.empty())ShellExecuteW(h,L"open",gReportPath.c_str(),nullptr,nullptr,SW_SHOW);return 0;}
 return 0;}
case WM_AUDIT_STATUS:{auto*s=(std::wstring*)l;if(s){SetWindowTextW(gStatus,s->c_str());delete s;}return 0;}
case WM_AUDIT_DONE:{Fill();SetWindowTextW(gBtn,L"KIỂM TRA TOÀN BỘ");EnableWindow(gOpen,!gReportPath.empty());SetFunctionalButtonsEnabled(w?FALSE:TRUE);if(gNext)EnableWindow(gNext,w?FALSE:TRUE);if(w)SetWindowTextW(gStatus,L"Đã dừng. Kết quả chưa hoàn chỉnh, không được chứng nhận.");else SetWindowTextW(gStatus,(L"Hoàn tất tự động — "+std::to_wstring(gReport.findings.size())+L" mục. Có thể chạy các test chức năng; report sẽ tự cập nhật.").c_str());InvalidateRect(h,nullptr,TRUE);if(gCloseRequested){DestroyWindow(h);}return 0;}
case WM_DRAWITEM:{auto*d=(DRAWITEMSTRUCT*)l;if(d->CtlID==1){HBRUSH b=CreateSolidBrush(gRunning?RGB(190,60,60):accent);FillRect(d->hDC,&d->rcItem,b);DeleteObject(b);SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,RGB(255,255,255));DrawTextW(d->hDC,gRunning?L"DỪNG KIỂM TRA":L"KIỂM TRA TOÀN BỘ",-1,&d->rcItem,DT_CENTER|DT_VCENTER|DT_SINGLELINE);return TRUE;}break;}
case WM_ERASEBKGND:{RECT r;GetClientRect(h,&r);HBRUSH b=CreateSolidBrush(bg);FillRect((HDC)w,&r,b);DeleteObject(b);return 1;}
case WM_PAINT:{PAINTSTRUCT p;HDC dc=BeginPaint(h,&p);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,text);HFONT h1=CreateFontW(32,0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");auto old=SelectObject(dc,h1);TextOutW(dc,28,24,L"LapSure",15);SelectObject(dc,old);DeleteObject(h1);SetTextColor(dc,RGB(90,100,115));TextOutW(dc,30,65,L"beta0.1 Build Validation — MSVC/portable/real-machine acceptance gate",54);EndPaint(h,&p);return 0;}
case WM_CLOSE:if(gRunning){gCloseRequested=true;gCancel=true;SetWindowTextW(gStatus,L"Đang dừng tác vụ trước khi thoát...");return 0;}DestroyWindow(h);return 0;
case WM_DESTROY:gCancel=true;if(gWorker.joinable())gWorker.join();PostQuitMessage(0);return 0;
}return DefWindowProcW(h,m,w,l);}

int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,LPWSTR,int){gDir=AppDir();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);bool inventoryOnly=false;std::wstring outputDir;for(int i=1;i<argc;i++){if(std::wstring(argv[i])==L"--inventory-only")inventoryOnly=true;else if(std::wstring(argv[i])==L"--output"&&i+1<argc)outputDir=argv[++i];}if(argv)LocalFree(argv);if(inventoryOnly){try{return RunInventoryOnly(outputDir);}catch(...){return 3;}}SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);INITCOMMONCONTROLSEX ic{sizeof(ic),ICC_LISTVIEW_CLASSES};InitCommonControlsEx(&ic);WNDCLASSEXW wc{sizeof(wc)};wc.lpfnWndProc=WndProc;wc.hInstance=hi;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);wc.lpszClassName=L"LapSure";RegisterClassExW(&wc);CreateWindowExW(0,wc.lpszClassName,L"LapSure v0.1-beta Build Validation",WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,1180,760,nullptr,nullptr,hi,nullptr);MSG msg;while(GetMessageW(&msg,nullptr,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);}return 0;}
