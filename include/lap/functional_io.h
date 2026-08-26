#pragma once
#include "model.h"
#include <windows.h>
#include <vector>

namespace lap {
std::vector<FunctionalItemResult> RunAudioCameraWizard(HWND owner);
std::vector<FunctionalItemResult> RunNetworkConnectivityWizard(HWND owner);
std::vector<FunctionalItemResult> RunFunctionalIoWizard(HWND owner);
}
