module retinal_damage_common shared
require ecs
require ecs.common

def stop_retinal_damage_effect(owner_eid : EntityId)
  find_query() <| $ [es(REQUIRE=retinalDamageScreenEffect)] (eid : EntityId; game_effect__attachedTo : EntityId)
    if game_effect__attachedTo != owner_eid
      return false
    destroyEntity(eid)
    return true

def fire_retinal_damage_effect(owner_eid : EntityId; world_position : float3; spot_scale : float)
  createEntity("retinal_damage_effect")  <| $(var init : ComponentsInitializer)
    set(init, "game_effect__attachedTo", owner_eid)
    set(init, "retinal_damage__explosion_world_position", world_position)
    set(init, "retinal_damage__scale", spot_scale)
