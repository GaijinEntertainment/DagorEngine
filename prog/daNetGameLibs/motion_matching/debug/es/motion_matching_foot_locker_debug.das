options no_aot
require ecs
require AnimV20
require MotionMatching
require danetlibs.renderer.includes.pufd_events


[es(tag=(dev, render), after=motion_matching_update_anim_tree_foot_locker_es, before=animchar_es)]
def motion_matching_debug_foot_locker_es(evt : ParallelUpdateFrameDelayed;
                                         var animchar : AnimcharBaseComponent;
                                         motion_matching__controller : MotionMatchingController)
  if (motion_matching__controller.dataBase == null || motion_matching__controller.dataBase.footLockerParamId < 0)
    return
  assume db = motion_matching__controller.dataBase
  anim_state_holder_get_foot_locker_legs(animchar.animState, *db) <| $(var legs : array<FootLockerIKCtrlLegData>)
    query() <| $ [es] (foot_locker_debug__targetLegNo : int;
                       foot_locker_debug__ankleTargetMove : float;
                       transform : float3x4)
      if foot_locker_debug__targetLegNo < 0 || foot_locker_debug__targetLegNo >= length(legs)
        return
      assume leg = legs[foot_locker_debug__targetLegNo]
      leg.lockedPosition = transform[3]
      leg.isLocked = true
      leg.posOffset = float3(0, 0, 0)
      leg.ankleTargetMove = foot_locker_debug__ankleTargetMove

[es(tag=(dev, render))]
def motion_matching_clear_foot_locker_es(evt : ParallelUpdateFrameDelayed;
                                         mm_params__footLockerEnabled : bool)
  if !mm_params__footLockerEnabled
    query() <| $ [es] (var animchar : AnimcharBaseComponent;
                       motion_matching__controller : MotionMatchingController)
      assume db = motion_matching__controller.dataBase
      anim_state_holder_get_foot_locker_legs(animchar.animState, *db) <| $(var legs : array<FootLockerIKCtrlLegData>)
        for leg in legs
          leg.isLocked = false
