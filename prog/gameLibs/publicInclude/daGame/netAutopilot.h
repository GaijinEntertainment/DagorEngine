//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gamePhys/phys/autopilotState.h>

class NetAutopilot
{
public:
  virtual void saveAutopilotState(AutopilotState &out_state) const = 0;
  virtual void restoreAutopilotState(const AutopilotState &saved_state) = 0;
};
