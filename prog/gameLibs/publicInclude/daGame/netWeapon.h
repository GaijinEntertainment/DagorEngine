//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gamePhys/phys/weaponState.h>

#include "math/dag_Point3.h"
#include "math/dag_Point2.h"

class NetWeapon
{
public:
  struct Turret
  {
    int index;
    Point3 pos;
    Point2 visualAngles;
    Point2 angles;
  };
  virtual void saveWeaponState(WeaponState &out_weapon_state) const = 0;
  virtual void restoreWeaponState(const WeaponState &saved_state) = 0;
  virtual bool correctInternalState(const WeaponState &incoming_state, const WeaponState &matching_state) = 0;
  virtual void onGunStateChanged(const WeaponState &previous_state) = 0;
  virtual float getWeaponMass(Point3 &cm) const = 0;
  virtual float getWeaponDragX() const = 0;
  virtual void updateFm(bool is_for_real) = 0;
  virtual bool getTurret(Turret &out_turret) const = 0;
};
