#pragma once
#include "model.h"
#include "environment.h"
#include <atomic>
#include <string>
namespace lap {
bool ParseSmartctlHealthJson(const std::wstring& json,StorageDevice& storage,std::wstring& error);
bool ParseWindowsStorageReliabilityLine(const std::wstring& line,StorageDevice& storage);
void CollectWindowsStorageReliability(AuditReport& report,const Capabilities& caps,const std::atomic_bool* cancel=nullptr);
void CollectSmartctl(AuditReport& report,const FactoryProfile& profile,const Capabilities& caps,const std::wstring& appDir,const std::atomic_bool* cancel=nullptr);
void CollectNvidia(AuditReport& report,const FactoryProfile& profile,const Capabilities& caps,const std::wstring& appDir,const std::atomic_bool* cancel=nullptr);
}
