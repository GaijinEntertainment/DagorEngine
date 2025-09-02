options no_aot
require ecs
require ecs.ecs_template
require RendInst
require DagorDebug3D
require DagorMath
require DagorConsole
require soundSystem

[ecs_template]
struct snd_debug_rotating_doors
  sndDebugRotatingDoors : bool

[console_cmd(name="snd.debug_rotating_doors")]
def snd_debug_rotating_doors__console_cmd()
  let found = find_query() <| $ [es(REQUIRE=sndDebugRotatingDoors)] (eid : EntityId)
    destroyEntity(eid)
    sound_debug("snd_debug_rotating_doors=false")
    return true
  if !found
    createEntity("snd_debug_rotating_doors")
    sound_debug("snd_debug_rotating_doors=true")

[es(tag=sound, no_order, REQUIRE=sndDebugRotatingDoors)]
def snd_debug_rotating_doors(evt : UpdateStageInfoRenderDebug)
  query() <| $ [es(REQUIRE=rotating_door_simple_phys__closeOnClosedAngleReached)] (eid : EntityId;
                                                                                   ri_extra__handle : riex_handle_t;
                                                                                   transform : float3x4;
                                                                                   rotating_door_sound__openedPath = "")
    let riType = handle_to_ri_type(ri_extra__handle)
    let riName = riex_getRIGenExtraName(uint(riType))
    let pos = transform[3]
    if length_sq(pos - get_listener_pos()) < 100. * 100.
      add_debug_text_mark(pos, "rotating_door riName='{riName}'; open_sound='{rotating_door_sound__openedPath}'; templ='{getEntityTemplateName(eid)}'", -1, 0., E3DCOLOR(0xff000000))

[ecs_template]
struct snd_debug_doors
  sndDebugDoors : bool

[console_cmd(name="snd.debug_doors")]
def snd_debug_doors__console_cmd()
  let found = find_query() <| $ [es(REQUIRE=sndDebugDoors)] (eid : EntityId)
    destroyEntity(eid)
    sound_debug("snd_debug_doors=false")
    return true
  if !found
    createEntity("snd_debug_doors")
    sound_debug("snd_debug_doors=true")

[es(tag=sound, no_order, REQUIRE=sndDebugDoors)]
def snd_debug_doors(evt : UpdateStageInfoRenderDebug)
  query() <| $ [es(REQUIRE=door_operations__curState, REQUIRE_NOT=rotating_door_simple_phys__closeOnClosedAngleReached)] (eid : EntityId;
                                                                                                                          ri_extra__handle : riex_handle_t;
                                                                                                                          transform : float3x4;
                                                                                                                          door_sound__openPath = "")
    let riType = handle_to_ri_type(ri_extra__handle)
    let riName = riex_getRIGenExtraName(uint(riType))
    let pos = transform[3]
    if length_sq(pos - get_listener_pos()) < 100. * 100.
      add_debug_text_mark(pos, "door riName='{riName}'; open_sound='{door_sound__openPath}'; templ='{getEntityTemplateName(eid)}'", -1, 0., E3DCOLOR(0xff000000))
