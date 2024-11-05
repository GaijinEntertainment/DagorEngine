//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <string.h>
#include <dag/dag_relocatable.h>
#include <generic/dag_staticTab.h>
#include <util/dag_globDef.h>

namespace danet
{
class BitStream;
}

const int BULLETS_SETS_QUANTITY = 6;

using BulletSets = StaticTab<uint8_t, BULLETS_SETS_QUANTITY>;

struct WeaponState
{
  float overheatCurrentTimer;
  float nextShotAtTime;
  float lastShotAtTime;
  float shootingCharge;

  struct BulletSet
  {
    int16_t bulletsLeft;
    uint16_t bulletsBorn;
  };
  StaticTab<BulletSet, BULLETS_SETS_QUANTITY> bulletSets;
  bool jettisoned;
  int32_t scheduledReloadTick;

  int8_t scheduledShotCount;
  bool jettisoning;
  uint8_t flags;
  uint8_t chosenBulletType;
  BulletSets scheduledBulletSets;
  uint8_t misfireCount;

  enum
  {
    WS_IS_JAMMED = 1,
    WS_IS_FRONT_GUN = (1 << 1),
    WS_IS_BASE_GUN = (1 << 2),
    WS_INVALID = (1 << 3),
    WS_IS_EXECUTED = (1 << 5),
    WS_IS_NEED_REARM = (1 << 6),
    WS_IS_REUPDATEABLE_GUN = (1 << 7)
  };

  float shotStep;

  WeaponState();

  bool operator==(const WeaponState &a) const;

  bool hasLargeDifferenceWith(const WeaponState &a) const
  {
    if ((flags & (WS_IS_FRONT_GUN | WS_IS_BASE_GUN | WS_IS_REUPDATEABLE_GUN)) !=
        (a.flags & (WS_IS_FRONT_GUN | WS_IS_BASE_GUN | WS_IS_REUPDATEABLE_GUN)))
    {
      return true;
    }

    return false;
  }

  void serialize(danet::BitStream &bs) const;
  bool deserialize(const danet::BitStream &bs);
};
DAG_DECLARE_RELOCATABLE(WeaponState);
