#include "lap/acquisition.h"

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
}
