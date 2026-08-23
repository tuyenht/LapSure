#pragma once
#include <windows.h>
#include <string>
#include "lap/model.h"

namespace lap {

enum class CanonicalUiState {
    Idle = 0,
    Ready,
    Locked,
    Running,
    Paused,
    Pass,
    Good,
    Warning,
    Fail,
    Changed,
    Incomplete,
    NotTested,
    Unsupported,
    ManualRequired,
    ProviderUnavailable,
    PermissionDenied,
    Cancelled,
    Interrupted,
    Empty,
    Error,
    Info
};

struct StatePresentation {
    CanonicalUiState state{CanonicalUiState::NotTested};
    const wchar_t* label{L""};
    const wchar_t* icon{L""};
    COLORREF textColor{RGB(15, 23, 42)};
    COLORREF bgColor{RGB(241, 245, 249)};
    COLORREF borderColor{RGB(203, 213, 225)};
    bool allowCleanPass{false};
};

StatePresentation GetStatePresentation(CanonicalUiState s);
CanonicalUiState MapState(State s);
CanonicalUiState MapFunctionalStatus(FunctionalStatus s);
CanonicalUiState MapTestStageState(TestStageState s);
std::wstring FormatDecisionVi(const std::wstring& overallDecision);
std::wstring FormatDecisionVi(const AuditDecision& decision);
const wchar_t* CanonicalStateName(CanonicalUiState s);

} // namespace lap
