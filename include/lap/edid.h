#pragma once
#include "model.h"
#include <string>
#include <vector>

namespace lap {
struct EdidIdentity {
    bool valid{false};
    std::wstring manufacturer;
    unsigned productCode{};
    unsigned serialNumeric{};
    unsigned manufactureWeek{};
    unsigned manufactureYear{};
    unsigned nativeWidth{};
    unsigned nativeHeight{};
    std::wstring monitorName;
    std::wstring serialText;
    std::wstring edidHex;
};
EdidIdentity ParseEdid(const unsigned char* data, size_t size);
std::vector<DisplayInfo> CollectNativeDisplays();
}
