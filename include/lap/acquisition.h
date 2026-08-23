#pragma once
#include "model.h"
#include <windows.h>
#include <vector>

namespace lap {
std::vector<FunctionalItemResult> RunPhysicalConditionWizard(HWND owner);
bool RunSellerClaimWizard(HWND owner,SellerClaim& claim);
void ApplySellerClaimComparison(AuditReport& report);
}
