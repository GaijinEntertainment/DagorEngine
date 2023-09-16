#include <gamePhys/phys/walker/humanAnimState.h>
#include <animChar/dag_animCharacter2.h>
#include <anim/dag_animBlendCtrl.h>

void HumanAnimState::init(const AnimV20::AnimcharBaseComponent &anim_char)
{
  const AnimV20::AnimationGraph *animGraph = anim_char.getAnimGraph();
  lowerState.stand = animGraph->getStateIdx("lower_stand");
  lowerState.standStill = animGraph->getStateIdx("lower_stand_still");
  lowerState.run = animGraph->getStateIdx("lower_run");
  lowerState.walk = animGraph->getStateIdx("lower_walk");
  lowerState.crawlMove = animGraph->getStateIdx("lower_crawlmove");
  lowerState.crawlStill = animGraph->getStateIdx("lower_crawlstill");
  lowerState.rotateLeft = animGraph->getStateIdx("lower_rotate_left");
  lowerState.rotateRight = animGraph->getStateIdx("lower_rotate_right");
  lowerState.crouchRotateLeft = animGraph->getStateIdx("lower_crouch_rotate_left");
  lowerState.crouchRotateRight = animGraph->getStateIdx("lower_crouch_rotate_right");
  lowerState.jumpStand = animGraph->getStateIdx("lower_jump_stand");
  lowerState.jumpRun = animGraph->getStateIdx("lower_jump_run");
  lowerState.ridingWalk = animGraph->getStateIdx("lower_beast_rider");
  lowerState.ridingSprint = animGraph->getStateIdx("lower_beast_rider_sprint");
  lowerState.ridingStand = animGraph->getStateIdx("lower_beast_rider_static");
  lowerState.lowerSprint = animGraph->getStateIdx("lower_sprint");

  upperState.aim = animGraph->getStateIdx("upper_aim");
  upperState.fireReady = animGraph->getStateIdx("upper_fireready");
  upperState.changeWeapon = animGraph->getStateIdx("upper_changeweapon");
  upperState.reload = animGraph->getStateIdx("weapon_reload");
  upperState.gunDown = animGraph->getStateIdx("upper_down");
  upperState.throwGrenade = animGraph->getStateIdx("point_hand_throw");
  upperState.fastThrow = animGraph->getStateIdx("upper_fast_throw");
  upperState.deflect = animGraph->getStateIdx("upper_deflect");

  upperState.aimCrouch = animGraph->getStateIdx("upper_aim_crouch");
  upperState.fireReadyCrouch = animGraph->getStateIdx("upper_fireready_crouch");
  upperState.changeWeaponCrouch = animGraph->getStateIdx("upper_changeweapon_crouch");
  upperState.reloadCrouch = animGraph->getStateIdx("weapon_reload_crouch");
  upperState.heal = animGraph->getStateIdx("upper_heal");
  upperState.healCrawl = animGraph->getStateIdx("upper_heal_crawl");
  upperState.swimEatStill = animGraph->getStateIdx("swim_eat_still");
  upperState.upperSprint = animGraph->getStateIdx("upper_sprint");

  comboState.crawlReload = animGraph->getStateIdx("crawl_reload");
  comboState.crawlMove = animGraph->getStateIdx("crawl_move");
  comboState.crawlRotateLeft = animGraph->getStateIdx("crawl_rotate_left");
  comboState.crawlRotateRight = animGraph->getStateIdx("crawl_rotate_right");
  comboState.healInjured = animGraph->getStateIdx("heal_injured");
  comboState.injuredCrawlStill = animGraph->getStateIdx("injured_crawl_still");
  comboState.injuredCrawlMove = animGraph->getStateIdx("injured_crawl_move");
  comboState.crawlAim = animGraph->getStateIdx("crawl_aim");
  comboState.crawlFireReady = animGraph->getStateIdx("crawl_fireready");
  comboState.crawlChangeWeapons = animGraph->getStateIdx("crawlChangeWeapons");
  comboState.attackedState = animGraph->getStateIdx("sm_bitten");
  comboState.swimStill = animGraph->getStateIdx("swim_still");
  comboState.swimStillUnderwater = animGraph->getStateIdx("swim_still_underwater");
  comboState.swimOnWater = animGraph->getStateIdx("swim_onwater");
  comboState.swimUnderWater = animGraph->getStateIdx("swim_underwater");
  comboState.climbHigh = animGraph->getStateIdx("climb_high");
  comboState.climbLadder = animGraph->getStateIdx("climb_ladder");
  comboState.useMortar = animGraph->getStateIdx("use_device_state");
  comboState.useDefibrillatorStart = animGraph->getStateIdx("defibrillator_crawl");
  comboState.useDefibrillatorFinish = animGraph->getStateIdx("defibrillator_jump");
  comboState.climbOverObstacle = animGraph->getStateIdx("hurdle_jump_state_01");
  comboState.fastClimb = animGraph->getStateIdx("hurdle_jump_state_02");

  dead = animGraph->getStateIdx("dead");
}

void HumanAnimState::updateState(AnimV20::AnimcharBaseComponent &anim_char, HumanStatePos state_pos, HumanStateMove state_move,
  StateJump state_jump, HumanStateUpperBody state_upper_body, float spd, HumanAnimStateFlags state_flags)
{
  StateResult res = updateState(state_pos, state_move, state_jump, state_upper_body, state_flags);

  AnimV20::AnimationGraph *animGraph = anim_char.getAnimGraph();
  animGraph->enqueueState(*anim_char.getAnimState(), animGraph->getState(res.lower), -1, spd);
  animGraph->enqueueState(*anim_char.getAnimState(), animGraph->getState(res.upper), -1, spd);
}


#define HST_FLAG(x, f) (((x)&HumanAnimStateFlags::f) != HumanAnimStateFlags::None)
HumanAnimState::StateResult HumanAnimState::updateState(HumanStatePos state_pos, HumanStateMove state_move, StateJump state_jump,
  HumanStateUpperBody state_upper_body, HumanAnimStateFlags state_flags)
{
  if (HST_FLAG(state_flags, Dead))
    return StateResult(dead, dead);
  if (HST_FLAG(state_flags, Attacked))
    return StateResult(comboState.attackedState, comboState.attackedState);
  if (HST_FLAG(state_flags, ClimbingOverObstacle))
    return StateResult(comboState.climbOverObstacle, comboState.climbOverObstacle);
  if (HST_FLAG(state_flags, FastClimbing))
    return StateResult(comboState.fastClimb, comboState.fastClimb);
  if (HST_FLAG(state_flags, Climbing))
    return StateResult(comboState.climbHigh, comboState.climbHigh);
  if (HST_FLAG(state_flags, Ladder))
    return StateResult(comboState.climbLadder, comboState.climbLadder);
  int upperStateIdx = -1;
  int lowerStateIdx = -1;
  int comboStateIdx = -1;
  switch (state_pos)
  {
    case E_STAND:
      switch (state_move)
      {
        case E_STILL: lowerStateIdx = lowerState.stand; break;
        case E_MOVE: lowerStateIdx = lowerState.walk; break;
        case E_RUN: lowerStateIdx = lowerState.run; break;
        case E_ROTATE_LEFT: lowerStateIdx = lowerState.rotateLeft; break;
        case E_ROTATE_RIGHT: lowerStateIdx = lowerState.rotateRight; break;
        default: break;
      };
      break;
    case E_CROUCH:
      switch (state_move)
      {
        case E_STILL: lowerStateIdx = lowerState.stand; break;
        case E_MOVE:
        case E_RUN: lowerStateIdx = lowerState.walk; break;
        case E_ROTATE_LEFT: lowerStateIdx = lowerState.crouchRotateLeft; break;
        case E_ROTATE_RIGHT: lowerStateIdx = lowerState.crouchRotateRight; break;
        default: break;
      };
      break;
    case E_CRAWL:
      switch (state_move)
      {
        case E_STILL: lowerStateIdx = lowerState.crawlStill; break;
        case E_MOVE:
        case E_RUN: lowerStateIdx = lowerState.crawlMove; break;
        default: break;
      };
      break;
    case E_DOWNED:
      switch (state_move)
      {
        case E_MOVE:
        case E_RUN:
        case E_ROTATE_LEFT:
        case E_ROTATE_RIGHT:
        case E_SPRINT: comboStateIdx = comboState.injuredCrawlMove; break;
        default: comboStateIdx = comboState.injuredCrawlStill; break;
      };
      break;
    default: break;
  };
  switch (state_upper_body)
  {
    case E_READY:
      upperStateIdx = state_pos == E_CROUCH && upperState.fireReadyCrouch >= 0 ? upperState.fireReadyCrouch : upperState.fireReady;
      break;
    case E_AIM: upperStateIdx = state_pos == E_CROUCH && upperState.aimCrouch >= 0 ? upperState.aimCrouch : upperState.aim; break;
    case E_CHANGE:
      upperStateIdx =
        state_pos == E_CROUCH && upperState.changeWeaponCrouch >= 0 ? upperState.changeWeaponCrouch : upperState.changeWeapon;
      break;
    case E_RELOAD:
      upperStateIdx = state_pos == E_CROUCH && upperState.reloadCrouch >= 0 ? upperState.reloadCrouch : upperState.reload;
      break;
    case E_DOWN: upperStateIdx = state_pos != E_CRAWL ? upperState.gunDown : upperState.fireReady; break;
    case E_FAST_THROW: upperStateIdx = upperState.fastThrow; break;
    case E_THROW: upperStateIdx = upperState.throwGrenade; break;
    case E_DEFLECT: upperStateIdx = upperState.deflect; break;
    case E_HEAL:
      switch (state_pos)
      {
        case E_CRAWL: upperStateIdx = upperState.healCrawl; break;
        case E_DOWNED: comboStateIdx = comboState.healInjured; break;
        default: upperStateIdx = upperState.heal; break;
      };
      break;
    case E_PUT_OUT_FIRE:
      switch (state_pos)
      {
        case E_CRAWL: upperStateIdx = upperState.healCrawl; break;
        default: upperStateIdx = upperState.heal; break;
      };
      break;
    case E_USE_RADIO:
      upperStateIdx = upperState.fireReady;
      lowerStateIdx = lowerState.stand;
      break;
    case E_USE_MORTAR: comboStateIdx = comboState.useMortar; break;
    case E_USE_DEFIBRILLATOR_START: comboStateIdx = comboState.useDefibrillatorStart; break;
    case E_USE_DEFIBRILLATOR_FINISH: comboStateIdx = comboState.useDefibrillatorFinish; break;
  };
  switch (state_pos)
  {
    case E_CRAWL:
      switch (state_upper_body)
      {
        case E_RELOAD: comboStateIdx = comboState.crawlReload; break;
        case E_AIM: comboStateIdx = comboState.crawlAim; break;
        case E_READY: comboStateIdx = comboState.crawlFireReady; break;
        case E_CHANGE: comboStateIdx = comboState.crawlChangeWeapons; break;
        default: break;
      };
      switch (state_move)
      {
        case E_ROTATE_LEFT: comboStateIdx = comboState.crawlRotateLeft; break;
        case E_ROTATE_RIGHT: comboStateIdx = comboState.crawlRotateRight; break;
        case E_MOVE:
        case E_RUN: comboStateIdx = comboState.crawlMove; break;
        default: break;
      };
      break;
    default: break;
  };
  switch (state_move)
  {
    case E_SPRINT:
      lowerStateIdx = lowerState.lowerSprint;
      switch (state_upper_body)
      {
        case E_CHANGE: upperStateIdx = upperState.changeWeapon; break;
        case E_RELOAD: upperStateIdx = upperState.reload; break;
        default: upperStateIdx = upperState.upperSprint; break;
      };
      break;
    default: break;
  };
  switch (state_pos)
  {
    case E_SWIM:
      switch (state_move)
      {
        case E_STILL:
          if (state_upper_body == E_HEAL)
          {
            upperStateIdx = upperState.swimEatStill;
            lowerStateIdx = comboState.swimStill;
          }
          else
            comboStateIdx = comboState.swimStill;
          break;
        default: comboStateIdx = comboState.swimOnWater; break;
      };
      break;
    case E_SWIM_UNDERWATER:
      switch (state_move)
      {
        case E_STILL: comboStateIdx = comboState.swimStillUnderwater; break;
        default: comboStateIdx = comboState.swimUnderWater; break;
      };
      break;
    default: break;
  };
  switch (state_jump)
  {
    case E_FROM_STAND: lowerStateIdx = lowerState.jumpStand; break;
    case E_FROM_RUN: lowerStateIdx = lowerState.jumpRun; break;
    default: break;
  };

  if (HST_FLAG(state_flags, RidingSprint))
    return StateResult(upperStateIdx, lowerState.ridingSprint);
  if (HST_FLAG(state_flags, RidingWalk))
    return StateResult(upperStateIdx, lowerState.ridingWalk);
  if (HST_FLAG(state_flags, RidingStand))
    return StateResult(upperStateIdx, lowerState.ridingStand);

  const bool isCrawlState = lowerStateIdx >= 0 && (lowerStateIdx == lowerState.crawlMove || lowerStateIdx == lowerState.crawlStill);
  if (state_pos == E_BIPOD && !isCrawlState)
    lowerStateIdx = lowerState.standStill;

  if (comboStateIdx >= 0)
    return StateResult(comboStateIdx, comboStateIdx);
  return StateResult(upperStateIdx, lowerStateIdx);
}
