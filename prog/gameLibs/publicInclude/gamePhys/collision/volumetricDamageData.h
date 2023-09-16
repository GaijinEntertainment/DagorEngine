//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>

namespace gamephys
{
enum DamageReason : uint8_t
{
  DMG_REASON_COLLISION,
  DMG_REASON_DROWN
};

const float timerTreshold = 0.5f;
struct VolumetricDamageData
{
  Point3 pos;
  float damage;
  float radius;
  float timer;
  int offenderId;
  DamageReason reason;

  void reset()
  {
    pos.zero();
    damage = 0.f;
    radius = 0.f;
    timer = 0.f;
    reason = DMG_REASON_COLLISION;
    offenderId = -1;
  }

  void doDamage(float dmg, float rad, DamageReason reas)
  {
    damage += dmg;
    radius = rad;
    reason = reas;
  }

  bool update(float dt)
  {
    timer += dt;
    return damage > 0.f && timer > timerTreshold;
  }
};
}; // namespace gamephys
