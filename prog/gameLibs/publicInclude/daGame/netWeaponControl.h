//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <gamePhys/phys/weaponControlState.h>

class NetWeaponControl
{
public:
  virtual void saveWeaponControlState(WeaponControlState &out_state) const = 0;
  virtual void restoreWeaponControlState(const WeaponControlState &saved_state) = 0;
};
