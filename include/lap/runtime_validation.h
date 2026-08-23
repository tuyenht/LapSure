#pragma once
#include "model.h"
#include "environment.h"
namespace lap {
const wchar_t* ValidationStatusText(ValidationStatus s);
void RunRuntimeValidation(AuditReport& report,const Capabilities& caps,const std::wstring& appDir);
}
