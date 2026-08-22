#pragma once
#include "model.h"
#include <string>
#include <vector>

namespace lap {

std::vector<std::wstring> SplitLines(const std::wstring& text);
std::vector<std::wstring> Split(const std::wstring& text, wchar_t delimiter);
std::wstring Trim(std::wstring s);
long long ParseI64(const std::wstring& s, long long fallback = -1);
double ParseDouble(const std::wstring& s, double fallback = -1.0);
uint64_t ParseU64(const std::wstring& s, uint64_t fallback = 0);

bool ParseMemoryModuleLine(const std::wstring& line, MemoryModule& out);
bool ParseDiskInventoryLine(const std::wstring& line, StorageDevice& out);
bool ParseBatteryLine(const std::wstring& line, BatteryInfo& out);
bool ParseNvidiaCsvLine(const std::wstring& line, GpuInfo& out);
bool ParseDisplayLine(const std::wstring& line, DisplayInfo& out);
bool ParseMainboardLine(const std::wstring& line, MainboardInfo& out);
bool ParseBiosLine(const std::wstring& line, BiosInfo& out);
bool ParseCpuTelemetryLine(const std::wstring& line, CpuTelemetry& out);

double NvmeDataUnitsToTB(long long dataUnits);

} // namespace lap
