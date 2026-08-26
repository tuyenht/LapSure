#pragma once
#include "model.h"
namespace lap {
ChassisProfile LoadChassisProfile(const std::wstring&,const std::wstring&);

// Production decision boundary. For protected pilot models the expected required-port
// denominator is release-defined in compiled code; mutable portable profiles are only
// advisory overlays and cannot delete/demote those protected required IDs.
ChassisProfile LoadDecisionChassisProfile(const std::wstring& appDir,
                                          const std::wstring& model);

void ApplyPortResultToChassisProfile(ChassisProfile&,const PortProbeResult&);
unsigned RequiredPortsRemaining(const ChassisProfile&);
}
