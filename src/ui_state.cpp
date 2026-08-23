#include "lap/ui_state.h"
#include "lap/ui_theme.h"

namespace lap {

StatePresentation GetStatePresentation(CanonicalUiState s) {
    StatePresentation p;
    p.state = s;
    switch (s) {
    case CanonicalUiState::Idle:
        p.label = L"Chưa bắt đầu";
        p.icon = L"○";
        p.textColor = UiColors::TextMuted;
        p.bgColor = UiColors::GrayPillBg;
        p.borderColor = UiColors::GrayPillBorder;
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::Ready:
        p.label = L"Sẵn sàng";
        p.icon = L"●";
        p.textColor = UiColors::PrimaryBlue;
        p.bgColor = UiColors::InfoBg;
        p.borderColor = RGB(191, 219, 254);
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::Locked:
        p.label = L"Chưa thể thực hiện";
        p.icon = L"🔒";
        p.textColor = UiColors::TextLight;
        p.bgColor = RGB(248, 250, 252);
        p.borderColor = RGB(226, 232, 240);
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::Running:
        p.label = L"Đang kiểm tra";
        p.icon = L"⏳";
        p.textColor = UiColors::PrimaryBlue;
        p.bgColor = UiColors::InfoBg;
        p.borderColor = RGB(147, 197, 253);
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::Paused:
        p.label = L"Đã tạm dừng";
        p.icon = L"⏸";
        p.textColor = UiColors::WarnAmber;
        p.bgColor = UiColors::WarnBg;
        p.borderColor = RGB(253, 230, 138);
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::Pass:
        p.label = L"Đạt";
        p.icon = L"✓";
        p.textColor = UiColors::SuccessGreen;
        p.bgColor = UiColors::SuccessBg;
        p.borderColor = RGB(187, 247, 208);
        p.allowCleanPass = true;
        break;
    case CanonicalUiState::Good:
        p.label = L"Tốt";
        p.icon = L"●";
        p.textColor = UiColors::SuccessGreen;
        p.bgColor = UiColors::SuccessBg;
        p.borderColor = RGB(187, 247, 208);
        p.allowCleanPass = true;
        break;
    case CanonicalUiState::Warning:
        p.label = L"Cần lưu ý";
        p.icon = L"▲";
        p.textColor = UiColors::WarnAmber;
        p.bgColor = UiColors::WarnBg;
        p.borderColor = RGB(253, 230, 138);
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::Fail:
        p.label = L"Không đạt";
        p.icon = L"✕";
        p.textColor = UiColors::FailRed;
        p.bgColor = UiColors::FailBg;
        p.borderColor = RGB(254, 202, 202);
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::Changed:
        p.label = L"Có thay đổi";
        p.icon = L"⇄";
        p.textColor = RGB(30, 64, 175);
        p.bgColor = RGB(238, 242, 255);
        p.borderColor = RGB(199, 210, 254);
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::Incomplete:
        p.label = L"Chưa đủ dữ liệu";
        p.icon = L"!";
        p.textColor = RGB(194, 65, 12);
        p.bgColor = RGB(255, 247, 237);
        p.borderColor = RGB(254, 215, 170);
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::NotTested:
        p.label = L"Chưa kiểm tra";
        p.icon = L"—";
        p.textColor = UiColors::TextMuted;
        p.bgColor = UiColors::GrayPillBg;
        p.borderColor = UiColors::GrayPillBorder;
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::Unsupported:
        p.label = L"Không hỗ trợ";
        p.icon = L"⊘";
        p.textColor = UiColors::TextMuted;
        p.bgColor = UiColors::GrayPillBg;
        p.borderColor = UiColors::GrayPillBorder;
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::ManualRequired:
        p.label = L"Cần xác nhận";
        p.icon = L"👤";
        p.textColor = RGB(109, 40, 217);
        p.bgColor = RGB(245, 243, 255);
        p.borderColor = RGB(221, 214, 254);
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::ProviderUnavailable:
        p.label = L"Không có nguồn dữ liệu";
        p.icon = L"⚠";
        p.textColor = UiColors::WarnAmber;
        p.bgColor = UiColors::WarnBg;
        p.borderColor = RGB(253, 230, 138);
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::PermissionDenied:
        p.label = L"Thiếu quyền truy cập";
        p.icon = L"🚫";
        p.textColor = UiColors::FailRed;
        p.bgColor = UiColors::FailBg;
        p.borderColor = RGB(254, 202, 202);
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::Cancelled:
        p.label = L"Đã hủy";
        p.icon = L"⏹";
        p.textColor = UiColors::TextMuted;
        p.bgColor = UiColors::GrayPillBg;
        p.borderColor = UiColors::GrayPillBorder;
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::Interrupted:
        p.label = L"Bị gián đoạn";
        p.icon = L"⚡";
        p.textColor = UiColors::FailRed;
        p.bgColor = UiColors::FailBg;
        p.borderColor = RGB(254, 202, 202);
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::Empty:
        p.label = L"Chưa có dữ liệu";
        p.icon = L"∅";
        p.textColor = UiColors::TextLight;
        p.bgColor = UiColors::GrayPillBg;
        p.borderColor = UiColors::GrayPillBorder;
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::Info:
        p.label = L"Thông tin";
        p.icon = L"ℹ";
        p.textColor = UiColors::InfoBlue;
        p.bgColor = UiColors::InfoBg;
        p.borderColor = RGB(191, 219, 254);
        p.allowCleanPass = false;
        break;
    case CanonicalUiState::Error:
    default:
        p.label = L"Có lỗi khi kiểm tra";
        p.icon = L"✕";
        p.textColor = UiColors::FailRed;
        p.bgColor = UiColors::FailBg;
        p.borderColor = RGB(254, 202, 202);
        p.allowCleanPass = false;
        break;
    }
    return p;
}

CanonicalUiState MapState(State s) {
    switch (s) {
    case State::Pass: return CanonicalUiState::Pass;
    case State::Good: return CanonicalUiState::Good;
    case State::Warning: return CanonicalUiState::Warning;
    case State::Fail: return CanonicalUiState::Fail;
    case State::Changed: return CanonicalUiState::Changed;
    case State::NotTested: return CanonicalUiState::NotTested;
    case State::Unsupported: return CanonicalUiState::Unsupported;
    case State::Info: return CanonicalUiState::Info;
    default: return CanonicalUiState::NotTested;
    }
}

CanonicalUiState MapFunctionalStatus(FunctionalStatus s) {
    switch (s) {
    case FunctionalStatus::Pass: return CanonicalUiState::Pass;
    case FunctionalStatus::Warning: return CanonicalUiState::Warning;
    case FunctionalStatus::Fail: return CanonicalUiState::Fail;
    case FunctionalStatus::NotTested: return CanonicalUiState::NotTested;
    case FunctionalStatus::Unsupported: return CanonicalUiState::Unsupported;
    case FunctionalStatus::ManualRequired: return CanonicalUiState::ManualRequired;
    default: return CanonicalUiState::NotTested;
    }
}

CanonicalUiState MapTestStageState(TestStageState s) {
    switch (s) {
    case TestStageState::Locked: return CanonicalUiState::Locked;
    case TestStageState::Ready: return CanonicalUiState::Ready;
    case TestStageState::Running: return CanonicalUiState::Running;
    case TestStageState::Passed: return CanonicalUiState::Pass;
    case TestStageState::Warning: return CanonicalUiState::Warning;
    case TestStageState::Failed: return CanonicalUiState::Fail;
    case TestStageState::Incomplete: return CanonicalUiState::Incomplete;
    default: return CanonicalUiState::Idle;
    }
}

std::wstring FormatDecisionVi(const std::wstring& overallDecision) {
    if (overallDecision == L"BUY") return L"CÓ THỂ MUA";
    if (overallDecision == L"BUY WITH NOTES") return L"CÓ THỂ MUA — CẦN LƯU Ý";
    if (overallDecision == L"REJECT") return L"KHÔNG NÊN MUA";
    return L"CHƯA ĐỦ DỮ LIỆU ĐỂ KẾT LUẬN";
}

std::wstring FormatDecisionVi(const AuditDecision& decision) {
    return FormatDecisionVi(decision.overall);
}

const wchar_t* CanonicalStateName(CanonicalUiState s) {
    return GetStatePresentation(s).label;
}

} // namespace lap
