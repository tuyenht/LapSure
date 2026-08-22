#pragma once
#include "model.h"
#include <windows.h>
#include <vector>

namespace lap {
std::vector<FunctionalItemResult> RunFunctionalIoWizard(HWND owner);
}
