//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <math/dag_Point3.h>
#include <generic/dag_carray.h>

class DataBlock;
namespace danet
{
class BitStream;
}

struct HumanPhysState;

struct HumanWeaponParams
{
  float walkMoveMagnitude;

  float holsterTime;
  float equipTime;

  float breathShakeMagnitude;
  float crouchBreathShakeMagnitude;
  float crawlBreathShakeMagnitude;

  float predictTime;
  float targetSpdVisc;
  float gunSpdVisc;
  float gunSpdDeltaMult;
  float maxGunSpd;
  float moveToSpd;
  float vertOffsetRestoreVisc;
  float vertOffsetMoveDownVisc;
  float equipSpeedMult;
  float holsterSwapSpeedMult;

  Point3 offsAimNode;
  Point3 offsCheckLeftNode;
  Point3 offsCheckRightNode;
  float gunLen;

  bool exists = false;

  void loadFromBlk(const DataBlock *blk);
  HumanWeaponParams() {} //-V730
};

enum HUWeaponSlots : uint8_t
{
  EWS_PRIMARY,
  EWS_SECONDARY,
  EWS_TERTIARY,
  EWS_MELEE,
  EWS_GRENADE,
  EWS_SPECIAL,
  EWS_UNARMED,
  EWS_QUATERNARY,

  EWS_NUM
};

typedef carray<HumanWeaponParams, EWS_NUM> HumanWeaponParamsVec;

static const char *humanWeaponSlotNames[] = {
  "primary", "secondary", "tertiary", "melee", "grenade", "special", "unarmed", "quaternary"};

enum HUWeaponEquipState : uint8_t
{
  EES_EQUIPED,
  EES_HOLSTERING,
  EES_EQUIPING,
  EES_DOWN,
  EES_ES_NUM
};

struct HumanWeaponEquipState
{
  float progress;
  HUWeaponEquipState curState;
  HUWeaponSlots curSlot;
  HUWeaponSlots nextSlot;

  HumanWeaponEquipState() { reset(); }

  HUWeaponSlots getEffectiveCurSlot() const { return curState == EES_EQUIPING ? nextSlot : curSlot; }

  void reset()
  {
    progress = 0.f;
    curState = EES_EQUIPED;
    curSlot = EWS_PRIMARY;
    nextSlot = EWS_PRIMARY;
  }
};
