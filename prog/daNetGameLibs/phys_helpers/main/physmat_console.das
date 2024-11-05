options no_aot

require DagorConsole
require app
require ecs

[console_cmd(name="physmat.reload")]
def physmat_reload()
  physmat_init()


[console_cmd(name="physmat.roughness_vis")]
def physmat_roughness_vis()
  let found = find_query() <| $ [es(REQUIRE=terrain_roughness_vis__baseStep)] (eid : EntityId)
    destroyEntity(eid)
    return true
  if !found
    createEntity("terrain_roughness_vis")
