//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <stdint.h>
#include <generic/dag_enumBitMask.h>

namespace AnimV20
{
class AnimcharBaseComponent;
}

enum HumanStatePos : uint8_t
{
  E_STAND = 0,
  E_CROUCH,
  E_CRAWL,
  E_SWIM,
  E_SWIM_UNDERWATER,
  E_DOWNED,
  E_BIPOD,
};

enum HumanStateMove : uint8_t
{
  E_STILL = 0,
  E_MOVE,
  E_RUN,
  E_SPRINT,
  E_ROTATE_LEFT,
  E_ROTATE_RIGHT
};

enum HumanStateUpperBody : uint8_t
{
  E_READY = 0,
  E_AIM,
  E_RELOAD,
  E_CHANGE,
  E_DOWN,
  E_DEFLECT,
  E_FAST_THROW,
  E_THROW,
  E_HEAL,
  E_PUT_OUT_FIRE,
  E_USE_RADIO,
  E_USE_MORTAR,
  E_USE_DEFIBRILLATOR_START,
  E_USE_DEFIBRILLATOR_FINISH
};

enum StateJump : uint8_t
{
  E_NOT_JUMP = 0,
  E_FROM_STAND,
  E_FROM_RUN
};

enum RidingState : uint32_t
{
  E_NOT_RIDING = 0,
  E_RIDING_STAND,
  E_RIDING_WALK,
  E_RIDING_SPRINT
};

enum class HumanAnimStateFlags : uint32_t
{
  None = 0,
  Dead = (1 << 0),
  Attacked = (1 << 1),
  Climbing = (1 << 4),
  Ladder = (1 << 5),
  Gunner = (1 << 6),
  RidingStand = (1 << 8),
  RidingWalk = (1 << 9),
  RidingSprint = (1 << 10),
  ClimbingOverObstacle = (1 << 11),
  FastClimbing = (1 << 12),
};
DAGOR_ENABLE_ENUM_BITMASK(HumanAnimStateFlags);

struct HumanAnimState
{
  struct LowerState
  {
    int stand;
    int standStill;
    int run;
    int walk;
    int crawlMove;
    int crawlStill;
    int rotateLeft;
    int rotateRight;
    int crouchRotateLeft;
    int crouchRotateRight;
    int jumpStand;
    int jumpRun;
    int ridingWalk;
    int ridingSprint;
    int ridingStand;

    int lowerSprint;
  } lowerState;

  struct UpperState
  {
    int aim;
    int fireReady;
    int changeWeapon;
    int reload;
    int gunDown;
    int fastThrow;
    int throwGrenade;
    int deflect;

    int aimCrouch;
    int fireReadyCrouch;
    int changeWeaponCrouch;
    int reloadCrouch;
    int heal;
    int healCrawl;
    int swimEatStill;
    int upperSprint;
  } upperState;

  struct ComboState
  {
    int crawlReload;
    int crawlMove;
    int crawlRotateLeft;
    int crawlRotateRight;
    int crawlAim;
    int crawlFireReady;
    int crawlChangeWeapons;
    int healInjured;
    int injuredCrawlStill;
    int injuredCrawlMove;
    int attackedState;
    int swimStill;
    int swimStillUnderwater;
    int swimOnWater;
    int swimUnderWater;
    int climbHigh;
    int climbLadder;
    int climbOverObstacle;
    int fastClimb;
    int useMortar;
    int useDefibrillatorStart;
    int useDefibrillatorFinish;
  } comboState;

  int dead;

  struct StateResult
  {
    int upper;
    int lower;

    StateResult() = default;
    StateResult(int up, int lo) : upper(up), lower(lo) {}
  };

  void init(const AnimV20::AnimcharBaseComponent &anim_char);
  StateResult updateState(HumanStatePos state_pos, HumanStateMove state_move, StateJump state_jump,
    HumanStateUpperBody state_upper_body, HumanAnimStateFlags state_flags = HumanAnimStateFlags::None);
  void updateState(AnimV20::AnimcharBaseComponent &anim_char, HumanStatePos state_pos, HumanStateMove state_move, StateJump state_jump,
    HumanStateUpperBody state_upper_body, float spd, HumanAnimStateFlags state_flags = HumanAnimStateFlags::None);
};
