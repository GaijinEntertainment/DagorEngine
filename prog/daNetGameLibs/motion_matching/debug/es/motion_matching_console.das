options no_aot
require ecs
require DagorConsole
require MotionMatching
require danetlibs/motion_matching/es/mm_events


[console_cmd(name="motion_matching.invalidate_anim_data_base")]
def invalidate_anim_data_base()
  // one event for all databases, but maybe will be reworked into one event per each database in future
  broadcastEvent(InvalidateAnimationDataBase())

[console_cmd(name="motion_matching.toggle_optional_subtemplates")]
def mm_toggle_optional_subtemplates()
  query <| $ [es] (var motion_matching__optionalSubtemplatesEnabled : bool&)
    motion_matching__optionalSubtemplatesEnabled ^^= true

[console_cmd(name="motion_matching.override_pbc_weight")]
def mm_override_pbc_weight(pbc_name : string; weight : float)
  query <| $ [es] (var dataBase : AnimationDataBase)
    let footLockerPBCIdx = get_post_blend_controller_idx(dataBase, pbc_name)
    if footLockerPBCIdx < 0
      console_print("Unknown controller '{pbc_name}'")
    else
      dataBase |> changePBCWeightOverride(footLockerPBCIdx, weight)
