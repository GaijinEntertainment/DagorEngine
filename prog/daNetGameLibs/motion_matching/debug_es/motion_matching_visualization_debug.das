options no_aot
require ecs
require AnimV20
require DagorMath
require DagorDebug3D
require MotionMatching


[es(tag=(dev, render), no_order)]
def draw_foot_locker_debug(evt : UpdateStageInfoRenderDebug;
                           mm_params__footLockerEnabled : bool;
                           mm_visualization_show_foot_locker_positions : bool)
  if !mm_params__footLockerEnabled
    return
  begin_draw_cached_debug_lines(false, false, false)
  query() <| $ [es] (motion_matching__controller : MotionMatchingController;
                     animchar : AnimcharBaseComponent)

    if motion_matching__controller.dataBase == null
      return

    assume db = motion_matching__controller.dataBase
    anim_state_holder_iterate_foot_locker_legs_const(animchar.animState, *db) <| $(leg : FootLockerIKCtrlLegData)
      if mm_visualization_show_foot_locker_positions && leg.isLocked
        draw_cached_debug_sphere(leg.lockedPosition, 0.03, E3DCOLOR(0xFFFF0000), 4)
  end_draw_cached_debug_lines()

