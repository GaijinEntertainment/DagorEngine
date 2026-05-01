options no_aot
require ecs
require PhysMat
require AnimV20
require %gameLibs/anim/animReplay/imports/anim_replay_events


[es(on_appear)]
def anim_replay_set_trace_context(evt : Event; var anim_replay__animchars : Array)
  let walkRayMatId = get_material_id("walkRay")
  for animcharChildComp in anim_replay__animchars
    assume animcharData = *getRW_ecs_object(animcharChildComp)
    let animcharEid = *get_Eid(animcharData, "eid")
    query(animcharEid) <| $ [es] (var animchar : AnimcharBaseComponent)
      animchar |> animchar_setTraceContext(int64((walkRayMatId << 1) | 1)) // Enable LegsIK and MM foot locker
