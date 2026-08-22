#pragma once
#include "model.h"
#include "environment.h"
#include <windows.h>
#include <atomic>
#include <string>

namespace lap {
const wchar_t* FunctionalStatusText(FunctionalStatus s);

struct FunctionalCapabilities {
    bool keyboardPresent{false};
    bool pointingDevicePresent{false};
    bool touchPresent{false};
    bool cameraPresent{false};
    bool biometricPresent{false};
    bool audioPresent{false};
    bool wifiPresent{false};
    bool bluetoothPresent{false};
    bool ethernetPresent{false};
    bool thunderboltOrUsb4Present{false};
    bool usbControllerPresent{false};
    bool acPowerConnected{false};
};

FunctionalCapabilities DetectFunctionalCapabilities(const Capabilities& caps,const std::atomic_bool* cancel);
void CollectFunctionalPresence(AuditReport& report,const Capabilities& caps,const std::atomic_bool* cancel);
void RecalculateFunctionalSummary(FunctionalTestSummary& summary);

FunctionalItemResult RunDisplayColorWizard(HWND owner);
FunctionalItemResult RunKeyboardWizard(HWND owner);
FunctionalItemResult RunTouchGridWizard(HWND owner,bool touchDevicePresent);
FunctionalItemResult RunSpeakerWizard(HWND owner,bool audioDevicePresent);
FunctionalItemResult RunUsbPortWizard(HWND owner,const Capabilities& caps,const std::atomic_bool* cancel);
}
