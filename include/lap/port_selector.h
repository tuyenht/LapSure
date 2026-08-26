#pragma once
#include "model.h"
#include <windows.h>

namespace lap {
bool SelectNextChassisPort(
    HWND hwnd,
    const ChassisProfile& profile,
    std::wstring& portId,
    std::wstring& label,
    std::wstring& capability);
}
