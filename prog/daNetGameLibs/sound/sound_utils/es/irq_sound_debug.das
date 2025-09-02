options no_aot
require ecs
require ecs.common
require ecs.ecs_template
require DagorSystem
require DagorDebug3D
require DagorConsole
require math.color
// require DngCamera

[cpp_event(unicast)]
struct CmdSoundDebugAnimParam
  param : float
  nodeName : string

[ecs_template]
struct sound_anim_param_debug
  sound_anim_param_debug__offsetBlue : float
  sound_anim_param_debug__offsetYellow : float


[es(tag=sound, on_event=ParallelUpdateFrameDelayed)]
def sound_anim_param_debug_update(evt : Event;
                                  var sound_anim_param_debug__offsetBlue : float&;
                                  var sound_anim_param_debug__offsetYellow : float&)
  sound_anim_param_debug__offsetBlue = -25.
  sound_anim_param_debug__offsetYellow = -2.


def get_cur_cam_entity()
  var camEid = INVALID_ENTITY_ID
  find_query() <| $ [es] (eid : EntityId; camera__active : bool)
    camEid = eid
    return camera__active
  return camEid

[es(tag=sound, REQUIRE=watchedByPlr)]
def sound_anim_param_debug(evt : CmdSoundDebugAnimParam;
                           eid : EntityId)
  let found = find_query() <| $ [es] (var sound_anim_param_debug__offsetBlue : float&)
    let camTm = get_cur_cam_entity() |> get_TMatrix("transform")
    if camTm != null
      let pos = (*camTm)[3] + (*camTm)[2]
      add_debug_text_mark(pos, "param=[{evt.param}] node={evt.nodeName} tmpl={getEntityTemplateName(eid)}", -1, sound_anim_param_debug__offsetBlue, E3DCOLOR(0x804040FF))
      sound_anim_param_debug__offsetBlue -= 1.
    return true

  if !found
    createEntity("sound_anim_param_debug")


[es(tag=sound)]
def gun_sound_anim_param_debug(evt : CmdSoundDebugAnimParam;
                               eid : EntityId;
                               gun__owner : EntityId)

  let found = find_query() <| $ [es] (var sound_anim_param_debug__offsetYellow : float&)
    query(gun__owner) <| $ [es(REQUIRE=watchedByPlr)] ()
      let camTm = get_cur_cam_entity() |> get_TMatrix("transform")
      if camTm != null
        let pos = (*camTm)[3] + (*camTm)[2]
        add_debug_text_mark(pos, "param=[{evt.param}] node={evt.nodeName} tmpl={getEntityTemplateName(eid)}", -1, sound_anim_param_debug__offsetYellow, E3DCOLOR(0x80FFFF00))
        sound_anim_param_debug__offsetYellow += 2.
    return true

  if !found
    createEntity("sound_anim_param_debug")
