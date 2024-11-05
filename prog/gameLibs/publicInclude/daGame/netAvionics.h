//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gamePhys/phys/avionicsState.h>

class NetAvionics
{
public:
  virtual void saveAvionicsState(AvionicsState &out_state) const = 0;
  virtual void restoreAvionicsState(const AvionicsState &saved_state) = 0;
};
