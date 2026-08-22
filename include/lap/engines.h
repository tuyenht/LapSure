#pragma once
#include "model.h"
#include "environment.h"
#include <atomic>
#include <string>
namespace lap {
void CollectSmartctl(AuditReport& report,const FactoryProfile& profile,const Capabilities& caps,const std::wstring& appDir,const std::atomic_bool* cancel=nullptr);
void CollectNvidia(AuditReport& report,const FactoryProfile& profile,const Capabilities& caps,const std::wstring& appDir,const std::atomic_bool* cancel=nullptr);
}
