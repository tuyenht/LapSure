#include "lap/port_selector.h"

namespace lap {

bool SelectNextChassisPort(
    HWND hwnd,
    const ChassisProfile& profile,
    std::wstring& portId,
    std::wstring& label,
    std::wstring& capability) {
    for (const auto& port : profile.ports) {
        const bool completed = port.tested &&
            (port.verdict == L"PASS" || port.verdict == L"FAIL");
        if (!port.required || completed) continue;

        portId = port.id;
        label = port.label;
        capability = port.capability;
        const auto message = L"Kiểm cổng:\n" + port.label +
            L"\nVị trí: " + port.side +
            L"\nChuẩn: " + port.connector +
            L"\nKhả năng: " + port.capability;
        return MessageBoxW(
                   hwnd,
                   message.c_str(),
                   L"Model-aware Port Test",
                   MB_OKCANCEL | MB_ICONINFORMATION) == IDOK;
    }
    return false;
}

} // namespace lap
