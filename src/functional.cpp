#include "lap/functional.h"
#include <windows.h>
#include <windowsx.h>
#include <setupapi.h>
#include <devguid.h>
#include <mmsystem.h>
#include <vector>
#include <string>
#include <algorithm>

#pragma comment(lib,"setupapi.lib")
#pragma comment(lib,"winmm.lib")

namespace lap {
namespace {
std::wstring Lower(std::wstring s){std::transform(s.begin(),s.end(),s.begin(),towlower);return s;}

bool PresentDeviceNameContains(const std::vector<std::wstring>&needles){
    HDEVINFO set=SetupDiGetClassDevsW(nullptr,nullptr,nullptr,DIGCF_ALLCLASSES|DIGCF_PRESENT);
    if(set==INVALID_HANDLE_VALUE)return false;bool found=false;
    for(DWORD i=0;!found;++i){SP_DEVINFO_DATA d{};d.cbSize=sizeof(d);if(!SetupDiEnumDeviceInfo(set,i,&d)){if(GetLastError()==ERROR_NO_MORE_ITEMS)break;continue;}wchar_t buf[1024]{};DWORD type=0,need=0;std::wstring text;
        if(SetupDiGetDeviceRegistryPropertyW(set,&d,SPDRP_FRIENDLYNAME,&type,(PBYTE)buf,sizeof(buf),&need)||SetupDiGetDeviceRegistryPropertyW(set,&d,SPDRP_DEVICEDESC,&type,(PBYTE)buf,sizeof(buf),&need)) text=Lower(buf);
        for(auto&n:needles)if(text.find(Lower(n))!=std::wstring::npos){found=true;break;}}
    SetupDiDestroyDeviceInfoList(set);return found;
}
unsigned PresentUsbStorageCount(){
    HDEVINFO set=SetupDiGetClassDevsW(&GUID_DEVCLASS_DISKDRIVE,nullptr,nullptr,DIGCF_PRESENT);if(set==INVALID_HANDLE_VALUE)return 0;unsigned count=0;
    for(DWORD i=0;;++i){SP_DEVINFO_DATA d{};d.cbSize=sizeof(d);if(!SetupDiEnumDeviceInfo(set,i,&d)){if(GetLastError()==ERROR_NO_MORE_ITEMS)break;continue;}wchar_t buf[1024]{};DWORD type=0,need=0;if(SetupDiGetDeviceRegistryPropertyW(set,&d,SPDRP_HARDWAREID,&type,(PBYTE)buf,sizeof(buf),&need)){if(Lower(buf).find(L"usb")!=std::wstring::npos)count++;}}
    SetupDiDestroyDeviceInfoList(set);return count;
}
FunctionalItemResult Item(std::wstring id,std::wstring name,FunctionalStatus st,std::wstring detail,std::wstring evidence,Confidence c,bool automated){FunctionalItemResult r{};r.id=std::move(id);r.name=std::move(name);r.status=st;r.detail=std::move(detail);r.evidence=std::move(evidence);r.confidence=c;r.automated=automated;return r;}

// ==================== DISPLAY COLOR & DEFECT WIZARD ====================
LRESULT CALLBACK ColorWndProc(HWND h,UINT msg,WPARAM w,LPARAM l){
    static int idx=0;
    static const COLORREF colors[]={RGB(255,255,255),RGB(0,0,0),RGB(255,0,0),RGB(0,255,0),RGB(0,0,255),RGB(128,128,128)};
    static const wchar_t* names[]={L"MÀU TRẮNG (Kiểm tra điểm chết đen, ố vàng, đốm sáng)",L"MÀU ĐEN (Kiểm tra hở sáng viền, điểm sáng kẹt)",L"MÀU ĐỎ (Kiểm tra sub-pixel chết)",L"MÀU XANH LÁ (Kiểm tra sub-pixel chết)",L"MÀU XANH DƯƠNG (Kiểm tra sub-pixel chết)",L"MÀU XÁM 50% (Kiểm tra độ đều màu, loang phản quang)"};
    if(msg==WM_CREATE){idx=0;return 0;}
    if(msg==WM_KEYDOWN){
        if(w==VK_ESCAPE){DestroyWindow(h);return 0;}
        if(w==VK_SPACE||w==VK_RIGHT||w==VK_RETURN){
            if(++idx>=6){DestroyWindow(h);return 0;}
            InvalidateRect(h,nullptr,TRUE);return 0;
        }
        if(w==VK_LEFT&&idx>0){idx--;InvalidateRect(h,nullptr,TRUE);return 0;}
    }
    if(msg==WM_LBUTTONDOWN){
        if(++idx>=6){DestroyWindow(h);return 0;}
        InvalidateRect(h,nullptr,TRUE);return 0;
    }
    if(msg==WM_PAINT){
        PAINTSTRUCT ps{};HDC dc=BeginPaint(h,&ps);
        HBRUSH b=CreateSolidBrush(colors[idx]);FillRect(dc,&ps.rcPaint,b);DeleteObject(b);
        SetBkMode(dc,TRANSPARENT);
        COLORREF txtCol=(idx==0)?RGB(40,40,40):RGB(255,255,255);
        SetTextColor(dc,txtCol);
        HFONT f=CreateFontW(20,0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        auto oldF=SelectObject(dc,f);
        std::wstring header=std::wstring(L"KIỂM TRA MÀN HÌNH (") + std::to_wstring(idx+1) + L"/6): " + names[idx];
        TextOutW(dc,32,28,header.c_str(),static_cast<int>(header.size()));
        const wchar_t* guide=L"Bấm PHÍM CÁCH / CLICK CHUỘT để đổi màu | Phím MŨI TÊN TRÁI để lùi | ESC để kết thúc";
        TextOutW(dc,32,58,guide,static_cast<int>(wcslen(guide)));
        SelectObject(dc,oldF);DeleteObject(f);
        EndPaint(h,&ps);return 0;
    }
    return DefWindowProcW(h,msg,w,l);
}

void RunColorWindow(HWND owner){
    WNDCLASSW wc{};wc.lpfnWndProc=ColorWndProc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"LAP_COLOR_TEST";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&wc);
    HWND h=CreateWindowExW(WS_EX_TOPMOST,wc.lpszClassName,L"LapSure — Kiểm tra màn hình",WS_POPUP|WS_VISIBLE,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),owner,nullptr,wc.hInstance,nullptr);
    SetForegroundWindow(h);MSG msg{};
    while(IsWindow(h)&&GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
}

struct DisplayDefectState { int selection{0}; bool done{false}; };
LRESULT CALLBACK DisplayDefectWndProc(HWND h,UINT msg,WPARAM w,LPARAM l){
    auto*st=reinterpret_cast<DisplayDefectState*>(GetWindowLongPtrW(h,GWLP_USERDATA));
    if(msg==WM_CREATE){
        SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams));
        return 0;
    }
    if(msg==WM_COMMAND){
        int id=LOWORD(w);
        if(id==IDOK&&st){
            HWND cb=GetDlgItem(h,101);
            if(cb)st->selection=static_cast<int>(SendMessageW(cb,CB_GETCURSEL,0,0));
            st->done=true;DestroyWindow(h);return 0;
        }
        if(id==IDCANCEL&&st){st->selection=0;st->done=true;DestroyWindow(h);return 0;}
    }
    if(msg==WM_CLOSE){if(st)st->done=true;DestroyWindow(h);return 0;}
    return DefWindowProcW(h,msg,w,l);
}

// ==================== KEYBOARD VISUAL MATRIX WIZARD ====================
struct KeyDefinition {
    int vk;
    const wchar_t* label;
    int row;
    float col;
    float width;
};

static const KeyDefinition kAnsiLayout[] = {
    // Row 0 - Function keys
    {VK_ESCAPE,L"Esc",0,0.0f,1.0f},{VK_F1,L"F1",0,1.5f,1.0f},{VK_F2,L"F2",0,2.5f,1.0f},{VK_F3,L"F3",0,3.5f,1.0f},{VK_F4,L"F4",0,4.5f,1.0f},
    {VK_F5,L"F5",0,6.0f,1.0f},{VK_F6,L"F6",0,7.0f,1.0f},{VK_F7,L"F7",0,8.0f,1.0f},{VK_F8,L"F8",0,9.0f,1.0f},
    {VK_F9,L"F9",0,10.5f,1.0f},{VK_F10,L"F10",0,11.5f,1.0f},{VK_F11,L"F11",0,12.5f,1.0f},{VK_F12,L"F12",0,13.5f,1.0f},{VK_DELETE,L"Del",0,15.0f,1.0f},
    // Row 1 - Number row
    {VK_OEM_3,L"`",1,0.0f,1.0f},{'1',L"1",1,1.0f,1.0f},{'2',L"2",1,2.0f,1.0f},{'3',L"3",1,3.0f,1.0f},{'4',L"4",1,4.0f,1.0f},
    {'5',L"5",1,5.0f,1.0f},{'6',L"6",1,6.0f,1.0f},{'7',L"7",1,7.0f,1.0f},{'8',L"8",1,8.0f,1.0f},{'9',L"9",1,9.0f,1.0f},
    {'0',L"0",1,10.0f,1.0f},{VK_OEM_MINUS,L"-",1,11.0f,1.0f},{VK_OEM_PLUS,L"=",1,12.0f,1.0f},{VK_BACK,L"Backspace",1,13.0f,2.0f},{VK_HOME,L"Home",1,15.0f,1.0f},
    // Row 2 - QWERTY
    {VK_TAB,L"Tab",2,0.0f,1.5f},{'Q',L"Q",2,1.5f,1.0f},{'W',L"W",2,2.5f,1.0f},{'E',L"E",2,3.5f,1.0f},{'R',L"R",2,4.5f,1.0f},
    {'T',L"T",2,5.5f,1.0f},{'Y',L"Y",2,6.5f,1.0f},{'U',L"U",2,7.5f,1.0f},{'I',L"I",2,8.5f,1.0f},{'O',L"O",2,9.5f,1.0f},
    {'P',L"P",2,10.5f,1.0f},{VK_OEM_4,L"[",2,11.5f,1.0f},{VK_OEM_6,L"]",2,12.5f,1.0f},{VK_OEM_5,L"\\",2,13.5f,1.5f},{VK_PRIOR,L"PgUp",2,15.0f,1.0f},
    // Row 3 - Home row
    {VK_CAPITAL,L"Caps",3,0.0f,1.75f},{'A',L"A",3,1.75f,1.0f},{'S',L"S",3,2.75f,1.0f},{'D',L"D",3,3.75f,1.0f},{'F',L"F",3,4.75f,1.0f},
    {'G',L"G",3,5.75f,1.0f},{'H',L"H",3,6.75f,1.0f},{'J',L"J",3,7.75f,1.0f},{'K',L"K",3,8.75f,1.0f},{'L',L"L",3,9.75f,1.0f},
    {VK_OEM_1,L";",3,10.75f,1.0f},{VK_OEM_7,L"'",3,11.75f,1.0f},{VK_RETURN,L"Enter",3,12.75f,2.25f},{VK_NEXT,L"PgDn",3,15.0f,1.0f},
    // Row 4 - Shift row
    {VK_SHIFT,L"Shift",4,0.0f,2.25f},{'Z',L"Z",4,2.25f,1.0f},{'X',L"X",4,3.25f,1.0f},{'C',L"C",4,4.25f,1.0f},{'V',L"V",4,5.25f,1.0f},
    {'B',L"B",4,6.25f,1.0f},{'N',L"N",4,7.25f,1.0f},{'M',L"M",4,8.25f,1.0f},{VK_OEM_COMMA,L",",4,9.25f,1.0f},{VK_OEM_PERIOD,L".",4,10.25f,1.0f},
    {VK_OEM_2,L"/",4,11.25f,1.0f},{VK_RSHIFT,L"RShift",4,12.25f,1.75f},{VK_UP,L"▲",4,14.0f,1.0f},{VK_END,L"End",4,15.0f,1.0f},
    // Row 5 - Bottom row
    {VK_CONTROL,L"Ctrl",5,0.0f,1.25f},{VK_LWIN,L"Win",5,1.25f,1.0f},{VK_MENU,L"Alt",5,2.25f,1.25f},{VK_SPACE,L"Spacebar",5,3.5f,5.5f},
    {VK_RMENU,L"RAlt",5,9.0f,1.25f},{VK_RCONTROL,L"RCtrl",5,10.25f,1.25f},{VK_LEFT,L"◄",5,13.0f,1.0f},{VK_DOWN,L"▼",5,14.0f,1.0f},{VK_RIGHT,L"►",5,15.0f,1.0f}
};
static const size_t kKeyCount = sizeof(kAnsiLayout)/sizeof(kAnsiLayout[0]);

struct VisualKeyState {
    bool pressed[kKeyCount]{};
    bool active[kKeyCount]{};
    unsigned totalEvents{};
    unsigned verifiedCount{};
    bool reportedBroken{false};
    bool accepted{false};
    bool done{false};
};

LRESULT CALLBACK KeyVisualWndProc(HWND h,UINT msg,WPARAM w,LPARAM l){
    auto*ks=reinterpret_cast<VisualKeyState*>(GetWindowLongPtrW(h,GWLP_USERDATA));
    if(msg==WM_CREATE){
        SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams));
        return 0;
    }
    if((msg==WM_KEYDOWN||msg==WM_SYSKEYDOWN)&&ks){
        ks->totalEvents++;
        int vk=static_cast<int>(w);
        for(size_t i=0;i<kKeyCount;i++){
            if(kAnsiLayout[i].vk==vk||(vk==VK_SHIFT&&(kAnsiLayout[i].vk==VK_SHIFT||kAnsiLayout[i].vk==VK_RSHIFT))
               ||(vk==VK_CONTROL&&(kAnsiLayout[i].vk==VK_CONTROL||kAnsiLayout[i].vk==VK_RCONTROL))
               ||(vk==VK_MENU&&(kAnsiLayout[i].vk==VK_MENU||kAnsiLayout[i].vk==VK_RMENU))){
                if(!ks->pressed[i]){ks->pressed[i]=true;ks->verifiedCount++;}
                ks->active[i]=true;
            }
        }
        InvalidateRect(h,nullptr,FALSE);
        if(msg==WM_SYSKEYDOWN)return 0;
    }
    if((msg==WM_KEYUP||msg==WM_SYSKEYUP)&&ks){
        int vk=static_cast<int>(w);
        for(size_t i=0;i<kKeyCount;i++){
            if(kAnsiLayout[i].vk==vk||(vk==VK_SHIFT&&(kAnsiLayout[i].vk==VK_SHIFT||kAnsiLayout[i].vk==VK_RSHIFT))
               ||(vk==VK_CONTROL&&(kAnsiLayout[i].vk==VK_CONTROL||kAnsiLayout[i].vk==VK_RCONTROL))
               ||(vk==VK_MENU&&(kAnsiLayout[i].vk==VK_MENU||kAnsiLayout[i].vk==VK_RMENU))){
                ks->active[i]=false;
            }
        }
        InvalidateRect(h,nullptr,FALSE);
        if(msg==WM_SYSKEYUP)return 0;
    }
    if(msg==WM_COMMAND&&ks){
        int id=LOWORD(w);
        if(id==IDOK){ks->accepted=true;ks->done=true;DestroyWindow(h);return 0;}
        if(id==1001){ks->reportedBroken=true;ks->done=true;DestroyWindow(h);return 0;}
        if(id==IDCANCEL){ks->done=true;DestroyWindow(h);return 0;}
    }
    if(msg==WM_PAINT&&ks){
        PAINTSTRUCT ps{};HDC hdc=BeginPaint(h,&ps);
        RECT client{};GetClientRect(h,&client);
        HDC memDC=CreateCompatibleDC(hdc);
        HBITMAP memBM=CreateCompatibleBitmap(hdc,client.right,client.bottom);
        auto oldBM=SelectObject(memDC,memBM);

        HBRUSH bgBrush=CreateSolidBrush(RGB(245,247,250));
        FillRect(memDC,&client,bgBrush);DeleteObject(bgBrush);

        SetBkMode(memDC,TRANSPARENT);
        HFONT titleFont=CreateFontW(22,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        HFONT subFont=CreateFontW(14,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        HFONT keyFont=CreateFontW(13,0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");

        auto oldFont=SelectObject(memDC,titleFont);
        SetTextColor(memDC,RGB(20,30,45));
        TextOutW(memDC,24,18,L"LapSure — Kiểm tra ma trận bàn phím (Keyboard Visual Matrix)",58);

        SelectObject(memDC,subFont);
        SetTextColor(memDC,RGB(80,95,115));
        std::wstring stat=L"Đã kiểm tra hoạt động: " + std::to_wstring(ks->verifiedCount) + L" / " + std::to_wstring(kKeyCount) + L" phím ("
            + std::to_wstring(kKeyCount?ks->verifiedCount*100/kKeyCount:0) + L"%) | Tổng số lần bấm: " + std::to_wstring(ks->totalEvents);
        TextOutW(memDC,24,46,stat.c_str(),static_cast<int>(stat.size()));
        const wchar_t* note=L"Hãy gõ lần lượt các phím trên bàn phím. Phím hoạt động tốt sẽ chuyển sang MÀU XANH LÁ.";
        TextOutW(memDC,24,68,note,static_cast<int>(wcslen(note)));

        // Draw Keyboard Grid
        SelectObject(memDC,keyFont);
        int startX=24,startY=100;
        int unitW=56,unitH=50,gap=5;

        for(size_t i=0;i<kKeyCount;i++){
            const auto&k=kAnsiLayout[i];
            int x=startX + static_cast<int>(k.col * (unitW + gap));
            int y=startY + k.row * (unitH + gap);
            int w=static_cast<int>(k.width * unitW + (k.width - 1.0f) * gap);
            int h=unitH;

            RECT keyRect{x,y,x+w,y+h};
            COLORREF fillCol=RGB(255,255,255);
            COLORREF borderCol=RGB(210,215,225);
            COLORREF textCol=RGB(40,50,65);

            if(ks->active[i]){
                fillCol=RGB(37,99,235);borderCol=RGB(29,78,216);textCol=RGB(255,255,255);
            }else if(ks->pressed[i]){
                fillCol=RGB(22,163,74);borderCol=RGB(21,128,61);textCol=RGB(255,255,255);
            }

            HBRUSH kb=CreateSolidBrush(fillCol);
            HPEN kp=CreatePen(PS_SOLID,1,borderCol);
            auto oldBrush=SelectObject(memDC,kb);
            auto oldPen=SelectObject(memDC,kp);
            RoundRect(memDC,keyRect.left,keyRect.top,keyRect.right,keyRect.bottom,8,8);
            SelectObject(memDC,oldBrush);SelectObject(memDC,oldPen);
            DeleteObject(kb);DeleteObject(kp);

            SetTextColor(memDC,textCol);
            DrawTextW(memDC,k.label,-1,&keyRect,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        }

        BitBlt(hdc,0,0,client.right,client.bottom,memDC,0,0,SRCCOPY);
        SelectObject(memDC,oldFont);
        DeleteObject(titleFont);DeleteObject(subFont);DeleteObject(keyFont);
        SelectObject(memDC,oldBM);DeleteObject(memBM);DeleteDC(memDC);
        EndPaint(h,&ps);return 0;
    }
    if(msg==WM_CLOSE){if(ks)ks->done=true;DestroyWindow(h);return 0;}
    return DefWindowProcW(h,msg,w,l);
}

// ==================== TOUCH & TOUCHPAD WIZARD ====================
struct TouchState{
    unsigned hits{};
    bool cells[80]{};
    bool leftClicked{false};
    bool rightClicked{false};
    int scrollEvents{};
};

LRESULT CALLBACK TouchWndProc(HWND h,UINT msg,WPARAM w,LPARAM l){
    auto*ts=reinterpret_cast<TouchState*>(GetWindowLongPtrW(h,GWLP_USERDATA));
    if(msg==WM_CREATE){
        SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams));
        return 0;
    }
    if(msg==WM_KEYDOWN&&w==VK_ESCAPE){DestroyWindow(h);return 0;}
    if(msg==WM_MOUSEWHEEL&&ts){ts->scrollEvents++;InvalidateRect(h,nullptr,TRUE);return 0;}
    if(msg==WM_RBUTTONDOWN&&ts){ts->rightClicked=true;InvalidateRect(h,nullptr,TRUE);return 0;}
    if((msg==WM_POINTERDOWN||msg==WM_LBUTTONDOWN||msg==WM_MOUSEMOVE)&&ts){
        if(msg==WM_LBUTTONDOWN)ts->leftClicked=true;
        POINT p{};
        if(msg==WM_POINTERDOWN){
            POINTER_INFO pi{};
            if(GetPointerInfo(GET_POINTERID_WPARAM(w),&pi)){p=pi.ptPixelLocation;ScreenToClient(h,&p);}
        }else{
            p.x=GET_X_LPARAM(l);p.y=GET_Y_LPARAM(l);
        }
        RECT r{};GetClientRect(h,&r);
        int cols=10,rows=8;
        int col=p.x*cols/(r.right?r.right:1),row=p.y*rows/(r.bottom?r.bottom:1);
        if(col>=0&&col<cols&&row>=0&&row<rows){
            int ix=row*cols+col;
            if(!ts->cells[ix]){
                ts->cells[ix]=true;ts->hits++;
                InvalidateRect(h,nullptr,TRUE);
                if(ts->hits>=static_cast<unsigned>(cols*rows)&&ts->leftClicked&&ts->rightClicked){
                    Sleep(200);DestroyWindow(h);
                }
            }
        }
        return 0;
    }
    if(msg==WM_PAINT&&ts){
        PAINTSTRUCT ps{};HDC dc=BeginPaint(h,&ps);
        RECT r{};GetClientRect(h,&r);
        int cols=10,rows=8;
        for(int row=0;row<rows;row++){
            for(int col=0;col<cols;col++){
                int ix=row*cols+col;
                RECT c{col*r.right/cols,row*r.bottom/rows,(col+1)*r.right/cols,(row+1)*r.bottom/rows};
                HBRUSH b=CreateSolidBrush(ts->cells[ix]?RGB(180,245,190):RGB(240,242,245));
                FillRect(dc,&c,b);DeleteObject(b);
                FrameRect(dc,&c,reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            }
        }
        SetBkMode(dc,TRANSPARENT);
        SetTextColor(dc,RGB(15,23,42));
        HFONT hf=CreateFontW(18,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        auto oldF=SelectObject(dc,hf);
        std::wstring t=L"KIỂM TRA TOUCHPAD / MÀN HÌNH CẢM ỨNG: Rê qua toàn bộ các ô (" + std::to_wstring(ts->hits) + L"/80 ô đã nhận)";
        TextOutW(dc,24,20,t.c_str(),static_cast<int>(t.size()));
        std::wstring stat=std::wstring(L"Chuột Trái: ") + (ts->leftClicked?L"[ĐÃ NHẬN ✓]":L"[Chưa bấm]")
            + L" | Chuột Phải: " + (ts->rightClicked?L"[ĐÃ NHẬN ✓]":L"[Chưa bấm]")
            + L" | Cuộn 2 ngón/bánh xe: " + (ts->scrollEvents>0?L"[ĐÃ NHẬN ✓]":L"[Chưa cuộn]")
            + L" | Phím ESC = Hoàn tất";
        TextOutW(dc,24,46,stat.c_str(),static_cast<int>(stat.size()));
        SelectObject(dc,oldF);DeleteObject(hf);
        EndPaint(h,&ps);return 0;
    }
    return DefWindowProcW(h,msg,w,l);
}

unsigned RunTouchWindow(HWND owner){
    TouchState ts{};WNDCLASSW wc{};wc.lpfnWndProc=TouchWndProc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"LAP_TOUCH_TEST";wc.hCursor=LoadCursor(nullptr,IDC_CROSS);RegisterClassW(&wc);
    HWND h=CreateWindowExW(WS_EX_TOPMOST,wc.lpszClassName,L"LapSure — Kiểm tra cảm ứng & Touchpad",WS_POPUP|WS_VISIBLE,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),owner,nullptr,wc.hInstance,&ts);
    SetForegroundWindow(h);MSG msg{};
    while(IsWindow(h)&&GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
    return ts.hits;
}
} // namespace

const wchar_t* FunctionalStatusText(FunctionalStatus s){
    switch(s){
        case FunctionalStatus::Pass:return L"PASS";
        case FunctionalStatus::Warning:return L"WARNING";
        case FunctionalStatus::Fail:return L"FAIL";
        case FunctionalStatus::Unsupported:return L"UNSUPPORTED";
        case FunctionalStatus::ManualRequired:return L"MANUAL";
        default:return L"NOT TESTED";
    }
}

FunctionalCapabilities DetectFunctionalCapabilities(const Capabilities&,const std::atomic_bool*){
    FunctionalCapabilities f{};
    f.keyboardPresent=PresentDeviceNameContains({L"keyboard"});
    f.pointingDevicePresent=PresentDeviceNameContains({L"touchpad",L"pointing",L"mouse"});
    f.touchPresent=PresentDeviceNameContains({L"touch screen",L"touchscreen",L"digitizer"});
    f.cameraPresent=PresentDeviceNameContains({L"camera",L"webcam"});
    f.biometricPresent=PresentDeviceNameContains({L"fingerprint",L"biometric"});
    f.audioPresent=PresentDeviceNameContains({L"audio",L"speaker",L"realtek"});
    f.wifiPresent=PresentDeviceNameContains({L"wireless",L"wi-fi",L"wifi"});
    f.bluetoothPresent=PresentDeviceNameContains({L"bluetooth"});
    f.ethernetPresent=PresentDeviceNameContains({L"ethernet",L"gigabit"});
    f.thunderboltOrUsb4Present=PresentDeviceNameContains({L"thunderbolt",L"usb4"});
    f.usbControllerPresent=PresentDeviceNameContains({L"usb xhci",L"usb host controller",L"usb root hub"});
    SYSTEM_POWER_STATUS ps{};if(GetSystemPowerStatus(&ps))f.acPowerConnected=ps.ACLineStatus==1;
    return f;
}

void RecalculateFunctionalSummary(FunctionalTestSummary&s){
    s.passed=s.failed=s.warning=s.notTested=s.manualRequired=0;
    for(auto&i:s.items){
        switch(i.status){
            case FunctionalStatus::Pass:s.passed++;break;
            case FunctionalStatus::Fail:s.failed++;break;
            case FunctionalStatus::Warning:s.warning++;break;
            case FunctionalStatus::ManualRequired:s.manualRequired++;break;
            default:s.notTested++;break;
        }
    }
    if(s.failed)s.overall=L"FAIL";
    else if(s.manualRequired||s.notTested)s.overall=L"INCOMPLETE";
    else if(s.warning)s.overall=L"PASS WITH NOTES";
    else s.overall=L"PASS";
}

void CollectFunctionalPresence(AuditReport&r,const Capabilities&caps,const std::atomic_bool*cancel){
    auto c=DetectFunctionalCapabilities(caps,cancel);
    auto&f=r.hardware.stress.functional;
    f.items.clear();
    auto add=[&](const wchar_t*id,const wchar_t*name,bool present,const wchar_t*detail){
        f.items.push_back(Item(id,name,present?FunctionalStatus::Pass:FunctionalStatus::NotTested,present?detail:L"Chưa phát hiện",L"Native SetupAPI presence detection; presence is not functional proof",Confidence::Medium,true));
    };
    add(L"keyboard_presence",L"Bàn phím vật lý",c.keyboardPresent,L"Có thiết bị");
    add(L"touchpad_presence",L"Touchpad / Chuột cảm ứng",c.pointingDevicePresent,L"Có thiết bị");
    add(L"touch_presence",L"Màn hình cảm ứng (Touch)",c.touchPresent,L"Có thiết bị");
    add(L"camera_presence",L"Camera / Webcam",c.cameraPresent,L"Có thiết bị");
    add(L"biometric_presence",L"Cảm biến vân tay / Sinh trắc",c.biometricPresent,L"Có thiết bị");
    add(L"audio_presence",L"Thiết bị âm thanh (Audio)",c.audioPresent,L"Có thiết bị");
    add(L"wifi_presence",L"Card mạng Wi-Fi",c.wifiPresent,L"Có thiết bị");
    add(L"bluetooth_presence",L"Bộ thu phát Bluetooth",c.bluetoothPresent,L"Có thiết bị");
    add(L"ethernet_presence",L"Cổng mạng LAN Ethernet",c.ethernetPresent,L"Có thiết bị");
    add(L"thunderbolt_presence",L"Bộ điều khiển Thunderbolt / USB4",c.thunderboltOrUsb4Present,L"Có controller");
    add(L"usb_presence",L"Bộ điều khiển USB Host",c.usbControllerPresent,L"Có controller");
    f.items.push_back(Item(L"ac_power",L"Bộ sạc nguồn AC",c.acPowerConnected?FunctionalStatus::Pass:FunctionalStatus::Warning,c.acPowerConnected?L"Đang cắm sạc AC":L"Đang chạy pin (chưa cắm sạc)",L"GetSystemPowerStatus; công suất sạc sẽ đối chiếu thêm",Confidence::Medium,true));
    f.items.push_back(Item(L"display_visual",L"Chất lượng hiển thị màn hình",FunctionalStatus::ManualRequired,L"Cần chạy kiểm tra màu toàn màn hình",L"Phát hiện điểm chết/hở sáng/mura cần quan sát trực quan",Confidence::Low,false));
    f.items.push_back(Item(L"keyboard_function",L"Kiểm tra chức năng bàn phím",FunctionalStatus::ManualRequired,L"Cần chạy ma trận phím trực quan",L"Có thiết bị không đảm bảo không bị liệt phím",Confidence::Low,false));
    if(c.touchPresent)f.items.push_back(Item(L"touch_grid",L"Lưới cảm ứng 60 vùng",FunctionalStatus::ManualRequired,L"Cần kiểm tra phản hồi cảm ứng",L"Yêu cầu mọi vùng đều nhận phản hồi",Confidence::Low,false));
    f.items.push_back(Item(L"speaker_function",L"Loa ngoài Stereo",FunctionalStatus::ManualRequired,L"Cần nghe thử loa trái/phải",L"Cần người dùng xác nhận âm thanh",Confidence::Low,false));
    f.items.push_back(Item(L"usb_ports",L"Các cổng cắm USB vật lý",FunctionalStatus::ManualRequired,L"Cần cắm/rút USB từng cổng",L"Bộ điều khiển có sẵn không thay thế việc kiểm tra từng cổng vật lý",Confidence::Low,false));
    f.items.push_back(Item(L"camera_function",L"Thu hình Camera thực tế",FunctionalStatus::ManualRequired,L"Cần thu mẫu khung hình Media Foundation",L"Có driver không chứng minh camera không bị mờ/hỏng mắt đọc",Confidence::Low,false));
    f.items.push_back(Item(L"mic_function",L"Thu âm Microphone",FunctionalStatus::ManualRequired,L"Cần ghi âm và đo mức tín hiệu thực tế",L"Có driver không chứng minh mic không bị nghẹt/hỏng màng thu",Confidence::Low,false));
    if(c.wifiPresent)f.items.push_back(Item(L"wifi_function",L"Trạng thái kết nối Wi-Fi",FunctionalStatus::ManualRequired,L"Cần chạy kiểm tra thiết bị tự động",L"Cần xác thực chất lượng sóng và trạng thái kết nối",Confidence::Low,false));
    if(c.bluetoothPresent)f.items.push_back(Item(L"bluetooth_function",L"Trạng thái sóng Bluetooth",FunctionalStatus::ManualRequired,L"Cần chạy kiểm tra thiết bị tự động",L"Cần xác thực ngăn xếp radio Bluetooth",Confidence::Low,false));
    RecalculateFunctionalSummary(f);
}

FunctionalItemResult RunDisplayColorWizard(HWND owner){
    RunColorWindow(owner);

    WNDCLASSW wc{};wc.lpfnWndProc=DisplayDefectWndProc;wc.hInstance=GetModuleHandleW(nullptr);
    wc.lpszClassName=L"LAP_DISPLAY_DEFECT_FORM";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&wc);

    DisplayDefectState state{};
    HWND h=CreateWindowExW(WS_EX_DLGMODALFRAME,wc.lpszClassName,L"LapSure — Đánh giá tình trạng màn hình",
                           WS_CAPTION|WS_SYSMENU,240,160,580,290,owner,nullptr,wc.hInstance,&state);
    if(!h)return Item(L"display_visual",L"Chất lượng hiển thị màn hình",FunctionalStatus::Pass,L"Đã kiểm tra 6 màu toàn màn hình",L"Full-screen color inspection",Confidence::Medium,false);

    HFONT font=reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HWND label=CreateWindowW(L"STATIC",L"Sau khi quan sát kỹ 6 màu màn hình, hãy chọn tình trạng thực tế:",
                             WS_CHILD|WS_VISIBLE,24,20,520,24,h,nullptr,nullptr,nullptr);
    HWND combo=CreateWindowW(L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_TABSTOP,
                             24,54,516,200,h,reinterpret_cast<HMENU>(101),nullptr,nullptr);
    SendMessageW(label,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);
    SendMessageW(combo,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);

    SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"1. Màn hình hoàn hảo (Không điểm chết, không sọc, sáng đều, màu đẹp)"));
    SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"2. Có 1-2 điểm chết/sáng nhỏ ở rìa (Chấp nhận được nếu giá rẻ)"));
    SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"3. Hở sáng viền/góc (Backlight bleed) trên nền đen"));
    SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"4. Nhiều điểm chết (>3 điểm) / Loang đốm phản quang, đốm trắng"));
    SendMessageW(combo,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"5. Sọc màn hình / Chớp giật / Ám màu nặng (Hư hỏng nghiêm trọng)"));
    SendMessageW(combo,CB_SETCURSEL,0,0);

    HWND ok=CreateWindowW(L"BUTTON",L"LƯU KẾT QUẢ",WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON|WS_TABSTOP,
                          260,195,140,34,h,reinterpret_cast<HMENU>(IDOK),nullptr,nullptr);
    HWND cancel=CreateWindowW(L"BUTTON",L"HỦY",WS_CHILD|WS_VISIBLE|WS_TABSTOP,
                              410,195,130,34,h,reinterpret_cast<HMENU>(IDCANCEL),nullptr,nullptr);
    SendMessageW(ok,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);
    SendMessageW(cancel,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);

    EnableWindow(owner,FALSE);ShowWindow(h,SW_SHOW);SetFocus(combo);
    MSG msg{};
    while(!state.done&&GetMessageW(&msg,nullptr,0,0)>0){
        if(!IsDialogMessageW(h,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    }
    EnableWindow(owner,TRUE);SetForegroundWindow(owner);

    switch(state.selection){
        case 0:
            return Item(L"display_visual",L"Chất lượng hiển thị màn hình",FunctionalStatus::Pass,
                        L"Màn hình hiển thị hoàn hảo; không phát hiện điểm chết, sọc hay loang màu",
                        L"Full-screen white/black/red/green/blue/gray + xác nhận trực quan hoàn hảo",Confidence::High,false);
        case 1:
            return Item(L"display_visual",L"Chất lượng hiển thị màn hình",FunctionalStatus::Warning,
                        L"Có 1-2 điểm chết/sáng nhỏ ở rìa (chấp nhận được nếu giá bán phù hợp)",
                        L"Full-screen inspection: 1-2 minor dead/stuck pixels reported",Confidence::High,false);
        case 2:
            return Item(L"display_visual",L"Chất lượng hiển thị màn hình",FunctionalStatus::Warning,
                        L"Hở sáng viền/góc (Backlight bleed) quan sát được trên nền tối",
                        L"Full-screen inspection: backlight bleed observed on dark screen",Confidence::High,false);
        case 3:
            return Item(L"display_visual",L"Chất lượng hiển thị màn hình",FunctionalStatus::Fail,
                        L"Màn hình có nhiều điểm chết (>3 điểm) hoặc loang đốm phản quang nặng",
                        L"Full-screen inspection: multiple dead pixels or severe mura defect",Confidence::High,false);
        case 4:
        default:
            return Item(L"display_visual",L"Chất lượng hiển thị màn hình",FunctionalStatus::Fail,
                        L"Màn hình bị sọc, chớp giật hoặc ám màu nghiêm trọng (nguy cơ lỗi cáp/tấm nền)",
                        L"Full-screen inspection: severe lines, flickering, or heavy color cast",Confidence::High,false);
    }
}

FunctionalItemResult RunKeyboardWizard(HWND owner){
    WNDCLASSW wc{};wc.lpfnWndProc=KeyVisualWndProc;wc.hInstance=GetModuleHandleW(nullptr);
    wc.lpszClassName=L"LAP_KEY_VISUAL_TEST";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&wc);

    VisualKeyState ks{};
    HWND h=CreateWindowExW(WS_EX_DLGMODALFRAME,wc.lpszClassName,
                           L"LapSure — Kiểm tra ma trận bàn phím trực quan (Keyboard Matrix)",
                           WS_CAPTION|WS_SYSMENU,120,60,940,540,owner,nullptr,wc.hInstance,&ks);
    if(!h)return Item(L"keyboard_function",L"Kiểm tra chức năng bàn phím",FunctionalStatus::NotTested,L"Không mở được cửa sổ bàn phím",L"",Confidence::Low,false);

    HFONT font=reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HWND btnOk=CreateWindowW(L"BUTTON",L"LƯU & XÁC NHẬN PHÍM TỐT",WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON|WS_TABSTOP,
                             510,446,230,38,h,reinterpret_cast<HMENU>(IDOK),nullptr,nullptr);
    HWND btnFail=CreateWindowW(L"BUTTON",L"BÁO CÓ PHÍM LIỆT / KẸT",WS_CHILD|WS_VISIBLE|WS_TABSTOP,
                               750,446,160,38,h,reinterpret_cast<HMENU>(1001),nullptr,nullptr);
    SendMessageW(btnOk,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);
    SendMessageW(btnFail,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);

    EnableWindow(owner,FALSE);ShowWindow(h,SW_SHOW);SetFocus(h);
    MSG msg{};
    while(!ks.done&&GetMessageW(&msg,nullptr,0,0)>0){
        if(!IsDialogMessageW(h,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    }
    EnableWindow(owner,TRUE);SetForegroundWindow(owner);

    if(ks.reportedBroken){
        return Item(L"keyboard_function",L"Kiểm tra chức năng bàn phím",FunctionalStatus::Fail,
                    L"Người dùng báo phát hiện phím bị liệt, chập chờn hoặc kẹt phím",
                    L"Visual Key Matrix: operator confirmed malfunctioning key(s)",Confidence::High,false);
    }
    if(!ks.totalEvents){
        return Item(L"keyboard_function",L"Kiểm tra chức năng bàn phím",FunctionalStatus::NotTested,
                    L"Chưa ghi nhận sự kiện bấm phím nào",
                    L"Interactive visual keyboard test: 0 events recorded",Confidence::Low,false);
    }

    unsigned pct=kKeyCount?(ks.verifiedCount*100/static_cast<unsigned>(kKeyCount)):0;
    std::wstring detail=L"Đã kiểm tra nhận diện tốt: " + std::to_wstring(ks.verifiedCount) + L"/" + std::to_wstring(kKeyCount) + L" phím (" + std::to_wstring(pct) + L"%)";
    std::wstring ev=L"Keyboard Visual Matrix: " + std::to_wstring(ks.verifiedCount) + L" unique keys verified, " + std::to_wstring(ks.totalEvents) + L" total key events";

    if(pct>=60||ks.accepted){
        return Item(L"keyboard_function",L"Kiểm tra chức năng bàn phím",FunctionalStatus::Pass,detail,ev,Confidence::High,false);
    }
    return Item(L"keyboard_function",L"Kiểm tra chức năng bàn phím",FunctionalStatus::Warning,
                detail + L" (chưa bấm đủ các phím chính)",ev,Confidence::Medium,false);
}

FunctionalItemResult RunTouchGridWizard(HWND owner,bool present){
    if(!present)return Item(L"touch_grid",L"Lưới cảm ứng & Touchpad",FunctionalStatus::Unsupported,L"Không phát hiện màn hình cảm ứng",L"",Confidence::Medium,false);
    unsigned n=RunTouchWindow(owner);
    bool ok=n>=60;
    return Item(L"touch_grid",L"Lưới cảm ứng & Touchpad",ok?FunctionalStatus::Pass:FunctionalStatus::Fail,
                std::to_wstring(n)+L"/80 ô cảm ứng nhận diện tốt",
                L"WM_POINTERDOWN + Touchpad pointer movement coverage: " + std::to_wstring(n) + L"/80 zones hit",
                ok?Confidence::High:Confidence::Medium,false);
}

FunctionalItemResult RunSpeakerWizard(HWND owner,bool present){
    if(!present)return Item(L"speaker_function",L"Loa ngoài Stereo",FunctionalStatus::Unsupported,L"Không phát hiện thiết bị âm thanh",L"",Confidence::Medium,false);
    MessageBeep(MB_ICONASTERISK);Sleep(400);MessageBeep(MB_ICONEXCLAMATION);
    int ans=MessageBoxW(owner,L"Bạn có vừa nghe thấy âm thanh thử nghiệm phát ra rõ ràng, không bị rè hay méo tiếng không?\n\nYES = Âm thanh rõ, tốt\nNO = Loa bị rè, nghẹt hoặc không nghe thấy",
                        L"LapSure — Kiểm tra loa ngoài",MB_YESNO|MB_ICONQUESTION);
    return Item(L"speaker_function",L"Loa ngoài Stereo",ans==IDYES?FunctionalStatus::Pass:FunctionalStatus::Fail,
                ans==IDYES?L"Âm thanh phát ra rõ ràng, mượt mà":L"Người dùng báo loa có vấn đề rè hoặc mất tiếng",
                L"Audio stimulus + human sound verification",Confidence::Medium,false);
}

FunctionalItemResult RunUsbPortWizard(HWND owner,const Capabilities&,const std::atomic_bool*){
    unsigned before=PresentUsbStorageCount();
    MessageBoxW(owner,L"Bước 1: Hãy RÚT thiết bị USB (USB Flash/chuột) đang cắm ở cổng cần kiểm tra, sau đó bấm OK.",
                L"LapSure — Kiểm tra cổng USB",MB_OK|MB_ICONINFORMATION);
    before=PresentUsbStorageCount();
    MessageBoxW(owner,L"Bước 2: Hãy CẮM thiết bị USB vào cổng USB đang kiểm tra. Đợi đèn USB sáng hoặc Windows nhận diện, sau đó bấm OK.",
                L"LapSure — Kiểm tra cổng USB",MB_OK|MB_ICONINFORMATION);
    Sleep(2500);
    unsigned after=PresentUsbStorageCount();
    bool ok=after>before;
    return Item(L"usb_ports",L"Các cổng cắm USB vật lý",ok?FunctionalStatus::Pass:FunctionalStatus::Fail,
                ok?L"Đã nhận diện thiết bị USB mới cắm vào cổng":L"Không nhận diện được thiết bị USB cắm vào cổng",
                L"SetupAPI DiskDrive count: before="+std::to_wstring(before)+L", after="+std::to_wstring(after),Confidence::High,false);
}
} // namespace lap
