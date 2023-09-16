//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <stdint.h>
#include <string.h>
#include <dag/dag_relocatable.h>

namespace danet
{
class BitStream;
}

struct WeaponState
{
  float overheatCurrentTimer;
  float nextShotAtTime;
  float lastShotAtTime;
  float shootingCharge;

  int16_t bulletsLeft;
  bool jettisoned;
  uint16_t bulletsBorn;
  int32_t scheduledReloadTick;

  int8_t scheduledShotCount;
  bool jettisoning;
  uint8_t flags;
  uint8_t chosenBulletType;
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

  WeaponState() { memset(this, 0, sizeof(*this)); }

  bool operator==(const WeaponState &a) const
  {
    if (bulletsLeft != a.bulletsLeft || jettisoned != a.jettisoned || flags != a.flags)
      return false;
    if (flags & WS_IS_BASE_GUN)
      return true;

    if (bulletsBorn != a.bulletsBorn)
      return false;
    if (!(flags & WS_IS_REUPDATEABLE_GUN))
      return true;

    bool equals = chosenBulletType == a.chosenBulletType && overheatCurrentTimer == a.overheatCurrentTimer &&
                  nextShotAtTime == a.nextShotAtTime && lastShotAtTime == a.lastShotAtTime &&
                  scheduledShotCount == a.scheduledShotCount && jettisoning == a.jettisoning &&
                  scheduledReloadTick == a.scheduledReloadTick && shotStep == a.shotStep && misfireCount == a.misfireCount;

    return equals;
  }

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
