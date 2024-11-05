options no_aot
require ecs
require ecs.common
require math.base
require DagorConsole
require DngCamera
require danetlibs.retinal_damage.render.retinal_damage_common

[console_cmd(name="affect.retinal_damage")]
def console_retinal_damage(screen_position_x : float = 0.; screen_position_y : float = 0.; scale : float = 1.5)
  query() <| $ [es(REQUIRE=hero)] (eid : EntityId)
    let cameraTm = get_cur_cam_entity() |> get_TMatrix("transform") ?? IDENT_TM
    let halfScreenWidth = cameraTm[0]
    let halfScreenHeight = cameraTm[1] * 9. / 16.
    let viewDir = cameraTm[2]
    let camPos = cameraTm[3]
    let wPos = camPos + viewDir + screen_position_x * halfScreenWidth - screen_position_y * halfScreenHeight
    fire_retinal_damage_effect(eid, wPos, scale)
