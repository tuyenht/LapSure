#pragma once
#include "model.h"
namespace lap { ChassisProfile LoadChassisProfile(const std::wstring&,const std::wstring&); void ApplyPortResultToChassisProfile(ChassisProfile&,const PortProbeResult&); unsigned RequiredPortsRemaining(const ChassisProfile&); }
