//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <limits.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/dag_mathUtils.h>
#include <dag/dag_relocatable.h>
#include "humanWeapState.h"
#include <gameMath/quantization.h>

namespace danet
{
class BitStream;
}
class IPhysBase;
class HumanPhys;
class GeomNodeTree;

struct HumanShootPosXZScale
{
  operator float() { return 2.0f; }
};
struct HumanShootPosYScale
{
  operator float() { return 2.8f; }
};
typedef gamemath::QuantizedPos<10, 12, 10, HumanShootPosXZScale, HumanShootPosYScale, HumanShootPosXZScale> HumanShootLocalPos;

enum HumanPhysControlType : uint8_t
{
  HCT_SHOOT,
  HCT_JUMP,
  HCT_CROUCH,
  HCT_CRAWL,
  HCT_THROW_BACK,
  HCT_SPRINT,
  HCT_AIM,
  HCT_MELEE,
  HCT_RELOAD,
  HCT_THROW,
  HCT_ZOOM_VIEW,
  HCT_HOLD_GUN_MODE,

  HCT_NUM
};

enum HumanControlThrowSlot : uint8_t
{
  HCTS_EMPTY = 0,
  HCTS_SLOT0 = 1 << 0,
  HCTS_SLOT1 = 1 << 1,
  HCTS_SLOT2 = 1 << 2,
  HCTS_SLOT3 = 1 << 3,
  HCTS_SLOT4 = 1 << 4,
  HCTS_SLOT5 = 1 << 5,

  HCTS_ALL = HCTS_SLOT0 | HCTS_SLOT1 | HCTS_SLOT2 | HCTS_SLOT3 | HCTS_SLOT4 | HCTS_SLOT5,

  HCTS_NUM = 6,
};

namespace hinternal
{
inline constexpr size_t log2ceil(size_t n, size_t i = 0) { return n ? log2ceil(n >> 1, i + 1) : i; }
} // namespace hinternal

struct HumanControlState //-V730
{
  int32_t producedAtTick = 0;
  uint16_t packedState = 0; // see enum below for packing details
  uint16_t extendedState = 0;
  enum
  {
    // packedState
    EWS_SHIFT = HCT_NUM,
    EWS_BITS = hinternal::log2ceil(EWS_NUM - 1u),
    EWS_MASK = (1 << EWS_BITS) - 1,
    PS_HAS_EXT_STATE = 1 << (sizeof(decltype(packedState)) * CHAR_BIT - 1),

    // extendedState
    LEAN_SHIFT = 0,
    LEAN_BITS = 2,
    LEAN_MASK = (1 << LEAN_BITS) - 1,
    TS_SHIFT = LEAN_BITS,
    TS_BITS = HCTS_NUM,
    TS_MASK = (1 << TS_BITS) - 1,
    DEVICE_SHIFT = LEAN_BITS + TS_BITS,
    DEVICE_BITS = 1,
    DEVICE_MASK = (1 << DEVICE_BITS) - 1,
    ALT_ATTACK_SHIFT = DEVICE_SHIFT + DEVICE_BITS,
    ALT_ATTACK_BITS = 1,
    ALT_ATTACK_MASK = (1 << ALT_ATTACK_BITS) - 1,
    DODGE_SHIFT = ALT_ATTACK_SHIFT + ALT_ATTACK_BITS,
    DODGE_BITS = 2, // [0, 0] -> no dodge, [0, 1] -> dodge left, [1, 0] -> dodge right, [1, 1] -> dodge back
    DODGE_MASK = (1 << DODGE_BITS) - 1,
    QUICK_RELOAD_SHIFT = DODGE_SHIFT + DODGE_BITS,
    QUICK_RELOAD_BITS = 1,
    QUICK_RELOAD_MASK = (1 << QUICK_RELOAD_BITS) - 1,
    EXT_BITS = LEAN_BITS + TS_BITS + DEVICE_BITS + ALT_ATTACK_BITS + DODGE_BITS + QUICK_RELOAD_BITS,
  };
  G_STATIC_ASSERT((HCT_NUM + EWS_BITS + /*PS_HAS_EXT_STATE*/ 1) <= sizeof(decltype(packedState)) * CHAR_BIT);
  G_STATIC_ASSERT(EXT_BITS <= sizeof(decltype(extendedState)) * CHAR_BIT);
  uint16_t walkPacked = 0;
  enum
  {
    WALK_SPEED_SHIFT = 0,
    WALK_SPEED_BITS = 5,
    WALK_SPEED_MASK = (1 << WALK_SPEED_BITS) - 1,
    WALK_DIR_SHIFT = WALK_SPEED_SHIFT + WALK_SPEED_BITS,
    WALK_DIR_BITS = 11,
    WALK_DIR_MASK = (1 << WALK_DIR_BITS) - 1
  };
  G_STATIC_ASSERT((WALK_SPEED_BITS + WALK_DIR_BITS) == sizeof(decltype(walkPacked)) * CHAR_BIT);

private:
  void setWalkDirBits(uint32_t v)
  {
    walkPacked = (walkPacked & ~(WALK_DIR_MASK << WALK_DIR_SHIFT)) | ((v & WALK_DIR_MASK) << WALK_DIR_SHIFT);
  }
  uint32_t getWalkDirBits() const { return (walkPacked >> WALK_DIR_SHIFT) & WALK_DIR_MASK; }
  void setExtendedState(decltype(extendedState) estate)
  {
    extendedState = estate;
    if (estate)
      packedState |= PS_HAS_EXT_STATE;
    else
      packedState &= ~PS_HAS_EXT_STATE;
  }

public:
  gamemath::UnitVecPacked32 wishShootDirPacked, wishLookDirPacked;
  HumanShootLocalPos shootPos;
  uint8_t unitVersion; // always present (regardless of HUMAN_USE_UNIT_VERSION) for bin compat
#if HUMAN_USE_UNIT_VERSION
  void setUnitVersion(uint8_t v) { unitVersion = v; }
  uint8_t getUnitVersion() const { return unitVersion; }
#else
  void setUnitVersion(uint8_t v)
  {
    G_ASSERTF(v == 0, "Human phys doesn't support unit versions!");
    G_UNUSED(v);
  }
  uint8_t getUnitVersion() const { return 0; }
#endif
  // logically it should be out of producedCT?
  bool lastEnqueuedHctShoot = false;
  bool haveUnenqueuedHctShoot = false;

private:
  bool isForged = false;
  Point3 wishShootDir; // normalized
  Point3 wishLookDir;  // normalized
  Point2 walkDir;      // normalized, in world coords
public:
  int getSequenceNumber() const { return 0; }
  void setSequenceNumber(int) {}
  void setControlsForged(bool val) { isForged = val; }
  bool isControlsForged() const { return isForged; }
  bool isControlBitSet(HumanPhysControlType ct) const { return (packedState & (1 << ct)) != 0; }
  DAGOR_NOINLINE void setControlBit(HumanPhysControlType ct, bool val) // workaround clang x64 bug
  {
    if (val)
      packedState |= 1 << ct;
    else
      packedState &= ~(1 << ct);
  }
  bool isThrowStateSet(HumanControlThrowSlot slot_idx = HCTS_SLOT0) const
  {
    return isControlBitSet(HCT_THROW) && (((extendedState >> TS_SHIFT) & TS_MASK) & slot_idx);
  }
  void setThrowState(bool value, HumanControlThrowSlot slot_idx = HCTS_SLOT0)
  {
    setControlBit(HCT_THROW, value);
    if (!value || slot_idx == HCTS_EMPTY)
      setExtendedState(extendedState & ~(TS_MASK << TS_SHIFT));
    else
      setExtendedState(extendedState | (slot_idx << TS_SHIFT));
  }
  void enqueueShoot()
  {
    lastEnqueuedHctShoot = isControlBitSet(HCT_SHOOT);
    haveUnenqueuedHctShoot = false;
  }
  void setLeanPosition(float lpos);
  float getLeanPosition() const; // guaranteed to be in range [-1,1]
  enum class DodgeState : uint8_t
  {
    No = 0b00000000u,
    Left = 0b00000001u,
    Right = 0b00000010u,
    Back = 0b00000011u
  };
  void setDodgeState(DodgeState);
  DodgeState getDodgeState() const;
  void setDeviceState(bool value) { setExtendedState((extendedState & ~(DEVICE_MASK << DEVICE_SHIFT)) | (value << DEVICE_SHIFT)); }
  bool isDeviceStateSet() const { return (extendedState >> DEVICE_SHIFT) & DEVICE_MASK; }
  void setAltAttackState(bool value)
  {
    setExtendedState((extendedState & ~(ALT_ATTACK_MASK << ALT_ATTACK_SHIFT)) | (value << ALT_ATTACK_SHIFT));
  }
  bool isAltAttackState() const { return (extendedState >> ALT_ATTACK_SHIFT) & ALT_ATTACK_MASK; }
  void setQuickReloadState(bool value)
  {
    setExtendedState((extendedState & ~(QUICK_RELOAD_MASK << QUICK_RELOAD_SHIFT)) | (value << QUICK_RELOAD_SHIFT));
  }
  bool isQuickReloadStateSet() const { return (extendedState >> QUICK_RELOAD_SHIFT) & QUICK_RELOAD_MASK; }
  void setChosenWeapon(HUWeaponSlots slot);
  HUWeaponSlots getChosenWeapon() const { return (HUWeaponSlots)((packedState >> EWS_SHIFT) & EWS_MASK); }

  // These are guaranteed to be normalized
  const Point3 &getWishShootDir() const { return wishShootDir; }
  const Point3 &getWishLookDir() const { return wishLookDir; }
  const Point2 &getWalkDir() const { return walkDir; }

  float getWalkSpeed() const; // guaranteed to be in range [0..1]
  bool isMoving() const { return getWalkSpeed() > 1e-2f; }

  void setWishShootDir(const Point3 &shoot);
  void setWishShootLookDir(const Point3 &dir);
  void setWishLookDir(const Point3 &look);
  void setWalkDir(const Point2 &wd);
  void setWorldWalkDir(const Point2 &wd, const Point2 &look_dir);
  void setWalkSpeed(float len);

  void serializeDelta(danet::BitStream &bs, const HumanControlState &base) const;
  bool deserializeDelta(const danet::BitStream &bs, const HumanControlState &base, int cur_tick);

  //
  // Stubs for compilation only (shall not be called)
  //
#if HUMAN_USE_UNIT_VERSION
  void serialize(danet::BitStream &);
  bool deserialize(const danet::BitStream &, IPhysBase &, int32_t &);
  void interpolate(const HumanControlState &, const HumanControlState &, float, int);
  void serializeMinimalState(danet::BitStream &, const HumanControlState &) const;
  bool deserializeMinimalState(const danet::BitStream &, const HumanControlState &);
  void applyMinimalState(const HumanControlState &);
#else
  void serialize(danet::BitStream &) const { G_ASSERT(0); }
  bool deserialize(const danet::BitStream &, IPhysBase &, int32_t &)
  {
    G_ASSERT(0);
    return false;
  }
  void interpolate(const HumanControlState &, const HumanControlState &, float, int) { G_ASSERT(0); }
  void serializeMinimalState(danet::BitStream &, const HumanControlState &) const { G_ASSERT(0); }
  bool deserializeMinimalState(const danet::BitStream &, const HumanControlState &)
  {
    G_ASSERT(0);
    return false;
  }
  void applyMinimalState(const HumanControlState &) { G_ASSERT(0); }
#endif

  void init() { reset(); }

  void reset()
  {
    setWishShootLookDir(Point3(1, 0, 0));
    setNeutralCtrl();
    shootPos.reset();

    isForged = false;
    producedAtTick = 0;
    setUnitVersion(0);
    setChosenWeapon(EWS_PRIMARY);
  }

  void setNeutralCtrl()
  {
    setWalkDir(Point2(0, 0));
    setLeanPosition(0);
    clearLastHistory();
  }

  void stepHistory() {}
  void clearLastHistory()
  {
    for (int i = 0; i < HCT_NUM; ++i)
      setControlBit((HumanPhysControlType)i, false);
  }
};
DAG_DECLARE_RELOCATABLE(HumanControlState);

inline void HumanControlState::setChosenWeapon(HUWeaponSlots slot)
{
#ifdef _DEBUG_TAB_
  G_FAST_ASSERT((unsigned)slot < EWS_NUM);
#endif
  packedState = (packedState & ~(EWS_MASK << EWS_SHIFT)) | (((unsigned)slot < EWS_NUM ? slot : EWS_PRIMARY) << EWS_SHIFT);
}

inline float HumanControlState::getLeanPosition() const
{
  int esn = (extendedState >> LEAN_SHIFT) & LEAN_MASK;
  return (esn == 0) ? 0 : ((esn & 2) ? -1 : 1);
}
inline void HumanControlState::setLeanPosition(float lpos)
{
  int esn = (fabsf(lpos) < 1e-6f) ? 0 : ((lpos < 0) ? 3 : 1);
  setExtendedState((extendedState & ~(LEAN_MASK << LEAN_SHIFT)) | (esn << LEAN_SHIFT));
}
inline HumanControlState::DodgeState HumanControlState::getDodgeState() const
{
  return HumanControlState::DodgeState((extendedState & (DODGE_MASK << DODGE_SHIFT)) >> DODGE_SHIFT);
}
inline void HumanControlState::setDodgeState(HumanControlState::DodgeState state)
{
  setExtendedState((extendedState & ~(DODGE_MASK << DODGE_SHIFT)) | (uint8_t(state) << DODGE_SHIFT));
}
inline float HumanControlState::getWalkSpeed() const
{
  return float((walkPacked >> WALK_SPEED_SHIFT) & WALK_SPEED_MASK) / float(WALK_SPEED_MASK);
}
inline void HumanControlState::setWalkSpeed(float len)
{
  uint32_t ews = real2int(saturate(len) * float(WALK_SPEED_MASK));
  walkPacked = (walkPacked & ~(WALK_SPEED_MASK << WALK_SPEED_SHIFT)) | (ews << WALK_SPEED_SHIFT);
}
