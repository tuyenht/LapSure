#pragma once
#include "model.h"
#include "environment.h"
#include <atomic>
#include <string>
namespace lap { void CollectPlatformForensics(AuditReport&,const FactoryProfile&,const Capabilities&,const std::wstring&,const std::atomic_bool*); }
