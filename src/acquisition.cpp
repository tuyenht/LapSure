#include "lap/acquisition.h"
#include <algorithm>
#include <cwctype>

namespace lap {
namespace {
FunctionalItemResult Observation(const wchar_t* id,const wchar_t* name,int answer,bool warningOnly){
    FunctionalItemResult r{};r.id=id;r.name=name;r.automated=false;
    r.evidence=L"Xác nhận trực tiếp trong LapSure; phần mềm không suy đoán quan sát vật lý.";
    if(answer==IDCANCEL){r.status=FunctionalStatus::NotTested;r.detail=L"Người kiểm tra đã bỏ qua";r.confidence=Confidence::Low;}
    else if(answer==IDYES){r.status=warningOnly?FunctionalStatus::Warning:FunctionalStatus::Fail;r.detail=L"Đã quan sát thấy dấu hiệu rủi ro";r.confidence=Confidence::Medium;}
    else{r.status=FunctionalStatus::Pass;r.detail=L"Không thấy dấu hiệu rủi ro";r.confidence=Confidence::Medium;}
    return r;
}
int Ask(HWND owner,const wchar_t* question){return MessageBoxW(owner,question,L"LapSure — Kiểm tra máy cũ",MB_YESNOCANCEL|MB_ICONQUESTION);}
struct ClaimFormState{bool done{false};bool accepted{false};HWND edits[8]{};std::wstring values[8];};
LRESULT CALLBACK ClaimWndProc(HWND h,UINT m,WPARAM w,LPARAM l){
    auto*s=reinterpret_cast<ClaimFormState*>(GetWindowLongPtrW(h,GWLP_USERDATA));
    if(m==WM_NCCREATE){SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams));return TRUE;}
    if(m==WM_COMMAND&&s&&(LOWORD(w)==IDOK||LOWORD(w)==IDCANCEL)){s->accepted=LOWORD(w)==IDOK;if(s->accepted)for(int i=0;i<8;i++){int n=GetWindowTextLengthW(s->edits[i]);s->values[i].assign(static_cast<size_t>(n),L'\0');if(n)GetWindowTextW(s->edits[i],s->values[i].data(),n+1);}s->done=true;DestroyWindow(h);return 0;}
    if(m==WM_CLOSE&&s){s->done=true;DestroyWindow(h);return 0;}return DefWindowProcW(h,m,w,l);
}
uint64_t Number(const std::wstring&s){try{return s.empty()?0ULL:std::stoull(s);}catch(...){return 0ULL;}}
std::wstring Lower(std::wstring s){std::transform(s.begin(),s.end(),s.begin(),towlower);return s;}
bool ContainsI(const std::wstring&actual,const std::wstring&expected){return !expected.empty()&&Lower(actual).find(Lower(expected))!=std::wstring::npos;}
void ClaimFinding(AuditReport&r,const wchar_t*name,const std::wstring&actual,const std::wstring&expected,bool pass){r.findings.push_back({L"Cam kết người bán",name,actual,expected,pass?State::Pass:State::Fail,Severity::Critical,L"Đối chiếu cam kết đã nhập với bằng chứng phần cứng LapSure",Dimension::Factory});}
}

std::vector<FunctionalItemResult> RunPhysicalConditionWizard(HWND owner){
    MessageBoxW(owner,L"Đặt máy ở nơi đủ sáng. Với mỗi câu hỏi:\n\nCÓ = thấy vấn đề\nKHÔNG = không thấy vấn đề\nHỦY = chưa kiểm tra\n\nKhông tháo máy và không chạm vào linh kiện khi đang cấp điện.",L"Kiểm tra ngoại hình và an toàn",MB_OK|MB_ICONINFORMATION);
    std::vector<FunctionalItemResult> out;
    out.push_back(Observation(L"physical_chassis",L"Vỏ và kết cấu",Ask(owner,L"Vỏ có nứt, móp mạnh, hở bất thường hoặc dấu va đập nghiêm trọng không?"),false));
    out.push_back(Observation(L"physical_hinge",L"Bản lề",Ask(owner,L"Gập mở màn hình 3 lần. Bản lề có quá lỏng/quá cứng, kêu bất thường hoặc làm tách vỏ không?"),false));
    out.push_back(Observation(L"physical_tamper",L"Dấu hiệu tháo sửa",Ask(owner,L"Ốc có mất/toét, mép vỏ hoặc tem có dấu tháo lắp đáng ngờ không?\n\nĐây chỉ là tín hiệu cảnh báo: nâng cấp RAM/SSD hợp lệ không đồng nghĩa máy bị lỗi."),true));
    out.push_back(Observation(L"physical_liquid",L"Dấu chất lỏng hoặc ăn mòn",Ask(owner,L"Có vết ố, rỉ xanh/trắng, mùi khét hoặc dấu chất lỏng quanh bàn phím, cổng và khe tản nhiệt không?"),false));
    out.push_back(Observation(L"physical_battery",L"Dấu hiệu pin phồng",Ask(owner,L"Touchpad, bàn phím hoặc đáy máy có bị đội/phồng, cong hoặc xuất hiện khe hở bất thường không?"),false));
    out.push_back(Observation(L"physical_charger",L"An toàn bộ sạc",Ask(owner,L"Dây/củ sạc có nứt, hở lõi, chân cong, lỏng, mùi khét hoặc nóng bất thường không?"),false));
    return out;
}

bool RunSellerClaimWizard(HWND owner,SellerClaim&claim){
    static const wchar_t*labels[]={L"Model người bán ghi:",L"CPU (từ khóa, ví dụ i7-11850H):",L"RAM (GB):",L"GPU (từ khóa, có thể bỏ trống):",L"Ổ lưu trữ (GB):",L"Màn hình (ví dụ 1920x1200):",L"Giá bán (VNĐ, có thể bỏ trống):",L"Bảo hành (ngày):"};
    WNDCLASSW wc{};wc.lpfnWndProc=ClaimWndProc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"LAPSURE_SELLER_CLAIM";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&wc);
    ClaimFormState state{};HWND h=CreateWindowExW(WS_EX_DLGMODALFRAME,wc.lpszClassName,L"LapSure — Cấu hình người bán cam kết",WS_CAPTION|WS_SYSMENU,220,80,620,520,owner,nullptr,wc.hInstance,&state);if(!h)return false;
    HFONT font=reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));for(int i=0;i<8;i++){HWND label=CreateWindowW(L"STATIC",labels[i],WS_CHILD|WS_VISIBLE,24,24+i*50,260,22,h,nullptr,nullptr,nullptr);state.edits[i]=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,292,20+i*50,292,28,h,reinterpret_cast<HMENU>(static_cast<INT_PTR>(100+i)),nullptr,nullptr);SendMessageW(label,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);SendMessageW(state.edits[i],WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);}
    HWND ok=CreateWindowW(L"BUTTON",L"LƯU VÀ ĐỐI CHIẾU",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,292,426,180,34,h,reinterpret_cast<HMENU>(IDOK),nullptr,nullptr);HWND cancel=CreateWindowW(L"BUTTON",L"HỦY",WS_CHILD|WS_VISIBLE|WS_TABSTOP,482,426,102,34,h,reinterpret_cast<HMENU>(IDCANCEL),nullptr,nullptr);SendMessageW(ok,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);SendMessageW(cancel,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);
    EnableWindow(owner,FALSE);ShowWindow(h,SW_SHOW);SetFocus(state.edits[0]);MSG msg{};while(!state.done&&GetMessageW(&msg,nullptr,0,0)>0){if(!IsDialogMessageW(h,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}EnableWindow(owner,TRUE);SetForegroundWindow(owner);if(!state.accepted)return false;
    claim={};claim.model=state.values[0];claim.cpuContains=state.values[1];claim.ramBytes=Number(state.values[2])*1024ULL*1024ULL*1024ULL;claim.gpuContains=state.values[3];claim.storageBytes=Number(state.values[4])*1000ULL*1000ULL*1000ULL;auto resolution=state.values[5];auto x=resolution.find_first_of(L"xX×");if(x!=std::wstring::npos){claim.displayWidth=static_cast<unsigned>(Number(resolution.substr(0,x)));claim.displayHeight=static_cast<unsigned>(Number(resolution.substr(x+1)));}claim.askingPriceVnd=static_cast<long long>(Number(state.values[6]));claim.warrantyDays=static_cast<unsigned>(Number(state.values[7]));claim.provided=true;return true;
}

void ApplySellerClaimComparison(AuditReport&r){
    r.findings.erase(std::remove_if(r.findings.begin(),r.findings.end(),[](const auto&f){return f.group==L"Cam kết người bán";}),r.findings.end());const auto&c=r.sellerClaim;if(!c.provided)return;
    if(!c.model.empty())ClaimFinding(r,L"Model",r.model,c.model,ContainsI(r.model,c.model)||ContainsI(c.model,r.model));
    if(!c.cpuContains.empty())ClaimFinding(r,L"CPU",r.hardware.cpuName,c.cpuContains,ContainsI(r.hardware.cpuName,c.cpuContains));
    if(c.ramBytes)ClaimFinding(r,L"RAM",std::to_wstring(r.hardware.installedRamBytes/1073741824ULL)+L" GB",std::to_wstring(c.ramBytes/1073741824ULL)+L" GB",r.hardware.installedRamBytes>=c.ramBytes);
    if(!c.gpuContains.empty()){bool found=false;std::wstring actual;for(const auto&g:r.hardware.gpus){if(!actual.empty())actual+=L"; ";actual+=g.name;found=found||ContainsI(g.name,c.gpuContains);}ClaimFinding(r,L"GPU",actual,c.gpuContains,found);}
    if(c.storageBytes){uint64_t total=0;for(const auto&d:r.hardware.storage)total+=d.capacityBytes;ClaimFinding(r,L"Ổ lưu trữ",std::to_wstring(total/1000000000ULL)+L" GB",std::to_wstring(c.storageBytes/1000000000ULL)+L" GB",total*10>=c.storageBytes*9);}
    if(c.displayWidth&&c.displayHeight){bool found=false;std::wstring actual;for(const auto&d:r.hardware.displays){if(!actual.empty())actual+=L"; ";actual+=std::to_wstring(d.nativeWidth)+L"x"+std::to_wstring(d.nativeHeight);found=found||(d.nativeWidth==c.displayWidth&&d.nativeHeight==c.displayHeight);}ClaimFinding(r,L"Màn hình",actual,std::to_wstring(c.displayWidth)+L"x"+std::to_wstring(c.displayHeight),found);}
}
}
