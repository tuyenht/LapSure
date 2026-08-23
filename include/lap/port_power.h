#pragma once
#include "model.h"
#include <windows.h>
#include <atomic>
#include <string>
namespace lap {
void CollectPortPowerBaseline(AuditReport& report);
PortProbeResult RunPhysicalPortProbe(HWND owner,const std::wstring& portLabel,const std::atomic_bool* cancel);
void RecalculatePortPowerSummary(PortPowerSummary& s);
}
