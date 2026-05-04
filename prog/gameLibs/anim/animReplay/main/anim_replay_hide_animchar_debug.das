options no_aot
require ecs
require math.base
require AnimV20
require %gameLibs/anim/animReplay/imports/anim_replay_events
require %gameLibs/anim/animReplay/imports/anim_replay_common

def iterate_animchars_recursively(parentEid : EntityId; hide : bool)
  query(parentEid) <| $ [es] (var animchar_render__enabled : bool&; attaches_list : EidList const?)
    animchar_render__enabled = !hide
    if attaches_list != null
      for childEid in *attaches_list
        iterate_animchars_recursively(childEid, hide);
  if (hide)
    // Some of the player's items, like a weapon, can be rendered unconditionally, so move them far away
    query(parentEid) <| $ [es(REQUIRE=watchedPlayerItem)] (var animchar_render : AnimcharRendComponent)
      assume sceneInstance = animchar_render.sceneInstance
      var tm = IDENT_TM;
      tm[3] = float3(1000, 1000, 1000)
      for nodeId in range(sceneInstance.nodeCount)
        *sceneInstance |> scene_instance_setNodeWtm(uint(nodeId), tm)

[es(on_appear)]
def anim_replay_hide_other_animchars(evt : Event; var anim_replay__hiddenAnimchars : EidList)
  query() <| $ [es(REQUIRE_NOT=anim_replay_character, REQUIRE_NOT=animchar_attach__attachedTo, REQUIRE=animchar_render)] (eid : EntityId)
    iterate_animchars_recursively(eid, true)
    push(anim_replay__hiddenAnimchars, eid)

[es(on_disappear)]
def anim_replay_restore_animchars(evt : Event; anim_replay__hiddenAnimchars : EidList)
  for hiddenAnimcharEid in anim_replay__hiddenAnimchars
    iterate_animchars_recursively(hiddenAnimcharEid, false)

[es(no_order)]
def anim_replay_hide_dead_animchars(evt : UpdateStageInfoAct;
                                    anim_replay__animchars : Array;
                                    anim_replay__currentTime : float)
  for animcharChildComp in anim_replay__animchars
    unsafe
      let animcharData & = *get_ecs_object(animcharChildComp)
      let animcharEid = *get_Eid(animcharData, "eid")
      iterate_animchars_recursively(animcharEid, is_animchar_dead(animcharData, anim_replay__currentTime))
