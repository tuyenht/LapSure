#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <exception>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cwctype>
#include <cwchar>
#include <ctime>
#include <utility>

#include "lap/inventory.h"
#include "lap/environment.h"
#include "lap/engines.h"
#include "lap/forensics.h"
#include "lap/stress.h"
#include "lap/functional.h"
#include "lap/functional_io.h"
#include "lap/acquisition.h"
#include "lap/port_power.h"
#include "lap/orchestrator.h"
#include "lap/chassis_profile.h"
#include "lap/runtime_validation.h"
#include "lap/port_selector.h"
#include "lap/profile.h"
#include "lap/cloud_lookup.h"
#include "lap/report.h"
#include "lap/session_history.h"
#include "lap/journal.h"
#include "lap/scoring.h"
#include "lap/process.h"
#include "lap/hardware.h"
#include "lap/ui_theme.h"
#include "lap/ui_state.h"
#include "lap/ui_components.h"
#include "lap/ui_screens.h"
#include "resource.h"

#pragma comment(lib, "Comctl32.lib")
using namespace lap;

namespace {
#include "app_runtime_state.ipp"
#include "app_audit.ipp"
#include "app_window.ipp"
#include "app_entry.ipp"
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int) {
    return RunLapSure(instance);
}
