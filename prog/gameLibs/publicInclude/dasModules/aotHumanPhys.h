//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <gamePhys/phys/walker/humanPhys.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotGamePhys.h>
#include <dasModules/aotDacoll.h>
#include <gamePhys/phys/walker/humanWeapState.h>
#include <gamePhys/phys/walker/humanControlState.h>
#include <gamePhys/phys/walker/humanAnimState.h>
#include <util/dag_lookup.h>

DAS_BIND_ENUM_CAST_98(HUMoveState);
DAS_BIND_ENUM_CAST_98(HUStandState);
DAS_BIND_ENUM_CAST_98(HUWeaponSlots)
DAS_BASE_BIND_ENUM_98(HUWeaponSlots, HUWeaponSlots, EWS_PRIMARY, EWS_SECONDARY, EWS_TERTIARY, EWS_MELEE, EWS_GRENADE, EWS_SPECIAL,
  EWS_UNARMED, EWS_QUATERNARY, EWS_NUM)
DAS_BIND_ENUM_CAST_98(HUWeaponEquipState)
DAS_BASE_BIND_ENUM_98(HUWeaponEquipState, HUWeaponEquipState, EES_EQUIPED, EES_HOLSTERING, EES_EQUIPING, EES_DOWN)
DAS_BIND_ENUM_CAST_98(HumanPhysControlType)
DAS_BASE_BIND_ENUM_98(HumanPhysControlType, HumanPhysControlType, HCT_SHOOT, HCT_JUMP, HCT_CROUCH, HCT_CRAWL, HCT_THROW_BACK,
  HCT_SPRINT, HCT_AIM, HCT_MELEE, HCT_RELOAD, HCT_THROW, HCT_ZOOM_VIEW, HCT_HOLD_GUN_MODE)
DAS_BIND_ENUM_CAST_98(HumanControlThrowSlot)
DAS_BASE_BIND_ENUM_98(HumanControlThrowSlot, HumanControlThrowSlot, HCTS_EMPTY, HCTS_SLOT0, HCTS_SLOT1, HCTS_SLOT2, HCTS_SLOT3,
  HCTS_SLOT4, HCTS_SLOT5, HCTS_ALL, HCTS_NUM)
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(HumanPhysState::StateFlag, StateFlag);
DAS_BIND_ENUM_CAST_98_IN_NAMESPACE(HumanControlState::DodgeState, DodgeState);
DAS_BIND_ENUM_CAST_98(PrecomputedPresetMode)
DAS_BIND_ENUM_CAST_98(HumanStatePos);
DAS_BASE_BIND_ENUM_98(HumanStatePos, HumanStatePos, E_STAND, E_CROUCH, E_CRAWL, E_SWIM, E_SWIM_UNDERWATER, E_DOWNED, E_BIPOD);
DAS_BIND_ENUM_CAST_98(HumanStateMove);
DAS_BASE_BIND_ENUM_98(HumanStateMove, HumanStateMove, E_STILL, E_MOVE, E_RUN, E_SPRINT, E_ROTATE_LEFT, E_ROTATE_RIGHT);
DAS_BIND_ENUM_CAST_98(HumanStateUpperBody);
DAS_BASE_BIND_ENUM_98(HumanStateUpperBody, HumanStateUpperBody, E_READY, E_AIM, E_RELOAD, E_CHANGE, E_DOWN, E_DEFLECT, E_FAST_THROW,
  E_THROW, E_HEAL, E_PUT_OUT_FIRE, E_USE_RADIO, E_USE_MORTAR, E_USE_DEFIBRILLATOR_START, E_USE_DEFIBRILLATOR_FINISH);
DAS_BIND_ENUM_CAST_98(StateJump);
DAS_BASE_BIND_ENUM_98(StateJump, StateJump, E_NOT_JUMP, E_FROM_STAND, E_FROM_RUN);
DAS_BIND_ENUM_CAST_98(RidingState)
DAS_BIND_ENUM_CAST(HumanAnimStateFlags)
DAS_BASE_BIND_ENUM(HumanAnimStateFlags, HumanAnimStateFlags, None, Dead, Attacked, Climbing, Ladder, Gunner, RidingStand, RidingWalk,
  RidingSprint, ClimbingOverObstacle, FastClimbing);
DAS_BASE_BIND_ENUM_98(PrecomputedPresetMode, PrecomputedPresetMode, FPV, TPV)

MAKE_TYPE_FACTORY(HumanWeaponParams, HumanWeaponParams);
MAKE_TYPE_FACTORY(HumanWeaponEquipState, HumanWeaponEquipState);
MAKE_TYPE_FACTORY(HumanPhysState, HumanPhysState);
MAKE_TYPE_FACTORY(HumanControlState, HumanControlState);
MAKE_TYPE_FACTORY(PrecomputedWeaponPositions, PrecomputedWeaponPositions);
MAKE_TYPE_FACTORY(HumanPhys, HumanPhys);

typedef Tab<HumanControlState> HumanControlStateTab;

DAS_BIND_VECTOR(HumanControlStateTab, HumanControlStateTab, HumanControlState, "HumanControlStateTab");

using HumanWeaponParamsArray = carray<HumanWeaponParams, EWS_NUM>;
DAS_BIND_ARRAY(HumanWeaponParamsArray, HumanWeaponParamsArray, HumanWeaponParams);

namespace bind_dascript
{
inline HUWeaponSlots cast_int_to_weapon_slots(int i, das::Context *context, das::LineInfoArg *line_info)
{
  if (i < 0 || i >= EWS_NUM)
    context->throw_error_at(line_info, "HUWeaponSlots(%d) index out of range 0..%u", i, EWS_NUM);
  return (HUWeaponSlots)i;
}

inline const char *get_human_weapon_slot_name(int i)
{
  if (uint32_t(i) < eastl::size(humanWeaponSlotNames))
    return humanWeaponSlotNames[i];
  return nullptr;
}

inline HUWeaponSlots cast_str_to_weapon_slots(const char *i)
{
  return (HUWeaponSlots)lup(i, humanWeaponSlotNames, countof(humanWeaponSlotNames));
}

inline bool human_control_state_is_control_bit_set(const HumanControlState &ct, HumanPhysControlType type)
{
  return ct.isControlBitSet(type);
}

inline bool human_control_state_is_throw_state_set(const HumanControlState &ct, HumanControlThrowSlot throw_slot)
{
  return ct.isThrowStateSet(throw_slot);
}

inline bool human_phys_state_is_hold_breath(const HumanPhysState &st) { return st.isHoldBreath; }

inline void human_phys_state_set_is_hold_breath(HumanPhysState &st, bool value) { st.isHoldBreath = value; }

inline void human_control_state_set_chosen_weapon(HumanControlState &ct, HUWeaponSlots slot) { ct.setChosenWeapon(slot); }

inline void human_control_state_set_device_state(HumanControlState &ct, bool value) { ct.setDeviceState(value); }

inline void human_phys_state_set_block_sprint(HumanPhysState &phys, bool value) { phys.blockSprint = value; }

inline void human_phys_wakeUp(HumanPhys &phys) { phys.wakeUp(); }

inline void human_phys_rescheduleAuthorityApprovedSend(HumanPhys &phys) { phys.rescheduleAuthorityApprovedSend(); }

inline void human_phys_state_reset_stamina(HumanPhysState &phys, float max_stamina) { phys.resetStamina(max_stamina); }

inline void human_phys_restore_stamina(HumanPhys &phys, float dt, float mult) { phys.restoreStamina(dt, mult); }

inline Point3 human_phys_calcGunPos(const HumanPhys &phys, PrecomputedPresetMode mode, const das::float3x4 &tm, float gun_angles,
  float lean_pos, float height)
{
  return phys.calcGunPos(reinterpret_cast<const TMatrix &>(tm), gun_angles, lean_pos, height, mode);
}

inline void human_phys_calcGunTm(const HumanPhys &phys, PrecomputedPresetMode mode, const TMatrix &tm, float gun_angles,
  float lean_pos, float height, das::float3x4 &out_gun_tm)
{
  reinterpret_cast<TMatrix &>(out_gun_tm) = phys.calcGunTm(reinterpret_cast<const TMatrix &>(tm), gun_angles, lean_pos, height, mode);
}

inline void human_control_state_set_walk_speed(HumanControlState &ct, float speed) { ct.setWalkSpeed(speed); }

inline void human_control_state_set_walk_dir(HumanControlState &ct, Point2 walk_dir) { ct.setWalkDir(walk_dir); }

inline void human_control_state_set_world_walk_dir(HumanControlState &ct, Point2 walk_dir, Point2 look_dir)
{
  ct.setWorldWalkDir(walk_dir, look_dir);
}

inline void human_control_state_set_wish_look_dir(HumanControlState &ct, Point3 look_dir) { ct.setWishLookDir(look_dir); }

inline void human_control_state_set_wish_shoot_dir(HumanControlState &ct, Point3 look_dir) { ct.setWishShootDir(look_dir); }

inline void human_control_state_set_neutral_ctrl(HumanControlState &ct) { ct.setNeutralCtrl(); }

inline void human_control_state_clear_unaproved_ctrl(HumanPhys &phys) { clear_and_shrink(phys.unapprovedCT); }

inline void human_control_state_reset(HumanControlState &ct) { ct.reset(); }

inline void human_phys_state_set_can_aim(HumanPhysState &phys, bool value) { phys.canAim = value; }

inline bool human_phys_state_get_overrideWallClimb(const HumanPhysState &phys) { return phys.overrideWallClimb; }

inline void human_phys_state_set_overrideWallClimb(HumanPhysState &phys, bool value) { phys.overrideWallClimb = value; }

inline bool human_phys_state_get_is_downed(const HumanPhysState &phys) { return phys.isDowned; }

inline void human_phys_state_set_is_downed(HumanPhysState &phys, bool value) { phys.isDowned = value; }

inline bool human_phys_state_get_canShoot(const HumanPhysState &phys) { return phys.canShoot; }

inline void human_phys_state_set_canShoot(HumanPhysState &phys, bool value) { phys.canShoot = value; }

inline bool human_phys_state_get_reduceToWalk(const HumanPhysState &phys) { return phys.reduceToWalk; }

inline void human_phys_state_set_reduceToWalk(HumanPhysState &phys, bool value) { phys.reduceToWalk = value; }

inline bool human_phys_state_get_isLadderQuickMovingUp(const HumanPhysState &phys) { return phys.isLadderQuickMovingUp; }

inline void human_phys_state_set_isLadderQuickMovingUp(HumanPhysState &phys, bool value) { phys.isLadderQuickMovingUp = value; }

inline bool human_phys_state_get_isLadderQuickMovingDown(const HumanPhysState &phys) { return phys.isLadderQuickMovingDown; }

inline void human_phys_state_set_isLadderQuickMovingDown(HumanPhysState &phys, bool value) { phys.isLadderQuickMovingDown = value; }

inline bool human_phys_state_get_forceWeaponDown(const HumanPhysState &phys) { return phys.forceWeaponDown; }

inline void human_phys_state_set_forceWeaponDown(HumanPhysState &phys, bool value) { phys.forceWeaponDown = value; }

inline bool human_phys_state_get_isClimbing(const HumanPhysState &phys) { return phys.isClimbing; }

inline bool human_phys_state_get_isClimbingOverObstacle(const HumanPhysState &phys) { return phys.isClimbingOverObstacle; }

inline bool human_phys_state_get_isFastClimbing(const HumanPhysState &phys) { return phys.isFastClimbing; }

inline bool human_phys_state_get_is_swimming(const HumanPhysState &phys) { return phys.isSwimming; }

inline bool human_phys_state_get_is_underwater(const HumanPhysState &phys) { return phys.isUnderwater; }

inline void human_phys_state_set_can_zoom(HumanPhysState &phys, bool value) { phys.canZoom = value; }

inline bool human_phys_state_can_aim(const HumanPhysState &phys) { return phys.canAim; }

inline bool human_phys_state_can_zoom(const HumanPhysState &phys) { return phys.canZoom; }

inline void human_phys_state_set_stoppedSprint(HumanPhysState &phys, bool value) { phys.stoppedSprint = value; }

inline bool human_phys_state_stoppedSprint(const HumanPhysState &phys) { return phys.stoppedSprint; }

inline void human_phys_state_set_attachedToExternalGun(HumanPhysState &phys, bool value) { phys.attachedToExternalGun = value; }

inline bool human_phys_state_attachedToExternalGun(const HumanPhysState &phys) { return phys.attachedToExternalGun; }

inline void human_phys_state_set_attachedToLadder(HumanPhysState &phys, bool value) { phys.attachedToLadder = value; }

inline bool human_phys_state_attachedToLadder(const HumanPhysState &phys) { return phys.attachedToLadder; }

inline void human_phys_state_set_detachedFromLadder(HumanPhysState &phys, bool value) { phys.detachedFromLadder = value; }

inline void human_phys_state_set_forceWeaponUp(HumanPhysState &phys, bool value) { phys.forceWeaponUp = value; }

inline bool human_phys_state_forceWeaponUp(const HumanPhysState &phys) { return phys.forceWeaponUp; }

inline void human_phys_state_set_useSecondaryCcd(HumanPhysState &phys, bool value) { phys.useSecondaryCcd = value; }

inline bool human_phys_state_useSecondaryCcd(const HumanPhysState &phys) { return phys.useSecondaryCcd; }

inline void human_control_state_set_control_bit(HumanControlState &ct, HumanPhysControlType type, bool value)
{
  ct.setControlBit(type, value);
}

inline void human_control_state_set_throw_state(HumanControlState &ct, bool value, HumanControlThrowSlot slot_idx = HCTS_SLOT0)
{
  ct.setThrowState(value, slot_idx);
}

inline void human_control_state_set_lean_position(HumanControlState &ct, float value) { ct.setLeanPosition(value); }

inline void human_control_state_set_dodge_state(HumanControlState &ct, HumanControlState::DodgeState state)
{
  ct.setDodgeState(state);
}

inline uint32_t human_control_state_get_shootPos_packed(const HumanControlState &ct) { return ct.shootPos.qpos; }

inline Point3 human_control_state_unpack_shootPos(const HumanControlState &ct) { return ct.shootPos.unpackPos(); }

inline void human_phys_state_set_isAttached(HumanPhysState &phys, bool value) { phys.isAttached = value; }

inline bool human_phys_state_get_isAttached(const HumanPhysState &phys) { return phys.isAttached; }

inline void human_phys_state_set_disableCollision(HumanPhysState &phys, bool value) { phys.disableCollision = value; }

inline bool human_phys_state_get_disableCollision(const HumanPhysState &phys) { return phys.disableCollision; }

inline bool human_phys_processCcdOffset(HumanPhys &phys, das::float3x4 &tm, const Point3 &to_pos, const Point3 &offset,
  float collision_margin, float speed_hardness, bool secondary_ccd, const Point3 &ccd_pos)
{
  return phys.processCcdOffset(reinterpret_cast<TMatrix &>(tm), to_pos, offset, collision_margin, speed_hardness, secondary_ccd,
    ccd_pos);
}

inline bool human_phys_getCollisionLinkData(HumanPhys &phys, HUStandState state, const char *nodeSlot,
  const das::TBlock<bool, dacoll::CollisionLinkData &> &block, das::Context *context, das::LineInfoArg *at)
{
  dacoll::CollisionLinks &links = phys.getCollisionLinks(state);
  int nodeSlotId = dacoll::get_link_name_id(nodeSlot ? nodeSlot : "");
  for (int i = 0; i < links.size(); ++i)
  {
    dacoll::CollisionLinkData &link = links[i];
    if (link.nameId == nodeSlotId)
    {
      vec4f arg;
      context->invokeEx(
        block, &arg, nullptr,
        [&](das::SimNode *code) {
          arg = das::cast<dacoll::CollisionLinkData &>::from(const_cast<dacoll::CollisionLinkData &>(link));
          code->eval(*context);
        },
        at);
      return true;
    }
  }
  return false;
}

inline void human_phys_setCollisionMatId(HumanPhys &phys, int mat_id) { phys.setCollisionMatId(mat_id); }

inline bool human_phys_state_get_isMount(const HumanPhysState &phys) { return phys.isMount; }

inline void human_phys_state_set_isMount(HumanPhysState &phys, bool value) { phys.isMount = value; }


inline bool human_phys_state_get_isVisible(const HumanPhysState &phys) { return phys.isVisible; }

inline void human_phys_state_set_isVisible(HumanPhysState &phys, bool value) { phys.isVisible = value; }

inline void human_phys_state_set_isExternalControlled(HumanPhysState &phys, bool value) { phys.externalControlled = value; }

inline TraceMeshFaces *human_phys_getTraceHandle(HumanPhys &phys) { return phys.getTraceHandle(); }

inline void human_phys_state_get_torso_contacts(const HumanPhysState &state,
  const das::TBlock<void, const das::TTemporary<const das::TArray<gamephys::CollisionContactDataMin>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  das::Array arr;
  arr.data = (char *)state.torsoContacts.data();
  arr.size = state.numTorsoContacts;
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}
} // namespace bind_dascript