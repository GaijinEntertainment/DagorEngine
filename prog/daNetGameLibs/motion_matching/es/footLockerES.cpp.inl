// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <anim/dag_animPostBlendCtrl.h>
#include <daECS/core/entitySystem.h>
#include <ecs/delayedAct/actInThread.h>
#include <ecs/anim/anim.h>

#include "animation/foot_locker.h"
#include "animation/motion_matching_controller.h"
#include "animation/animation_data_base.h"


ECS_TAG(render)
ECS_BEFORE(animchar_es)
ECS_AFTER(wait_motion_matching_job_es)
static void motion_matching_update_anim_tree_foot_locker_es(const ParallelUpdateFrameDelayed &,
  AnimV20::AnimcharBaseComponent &animchar,
  const MotionMatchingController &motion_matching__controller)
{
  const MotionMatchingController &controller = motion_matching__controller;
  if (!controller.dataBase)
    return;

  const MotionMatchingController::CurrentClipInfo &curAnim = controller.currentClipInfo;
  const AnimationDataBase &dataBase = *controller.dataBase;
  if (dataBase.footLockerParamId < 0)
    return;

  int numLegs = dataBase.footLockerNodes.size();
  AnimV20::FootLockerIKCtrl::LegData *legsData =
    static_cast<AnimV20::FootLockerIKCtrl::LegData *>(animchar.getAnimState()->getInlinePtr(dataBase.footLockerParamId));
  for (int legNo = 0; legNo < numLegs; ++legNo)
  {
    AnimV20::FootLockerIKCtrl::LegData &leg = legsData[legNo];

    if (controller.hasActiveAnimation())
    {
      const AnimationClip &clip = dataBase.clips[curAnim.clip];
      bool needLock = clip.footLockerStates.needLock(legNo, curAnim.frame, numLegs);
      bool lockAllowed = true;
      // Lock is possible only when previous animation frame doesn't need a lock. It prevents cycling
      // when we need lock from animation data, but immedeately unlock it by distance.
      if (controller.lastTransitionTime > (1.f / TICKS_PER_SECOND))
      {
        int prevFrame = curAnim.frame > 0 ? curAnim.frame - 1 : clip.tickDuration - 1;
        lockAllowed = !clip.footLockerStates.needLock(legNo, prevFrame, numLegs);
      }
      // And for single frame animations too, because there is no previous frame with needLock=false
      if (dataBase.clips[curAnim.clip].tickDuration == 1 && needLock)
        lockAllowed = controller.lastTransitionTime >= controller.transitionBlendTime;

      leg.needLock = needLock && (leg.isLocked || lockAllowed);
    }
    else
      leg.needLock = false;
  }
}
