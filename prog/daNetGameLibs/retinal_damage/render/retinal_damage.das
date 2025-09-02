require ecs
require danetlibs.retinal_damage.render.retinal_damage_common

[event(unicast)] //to be moved to separate module
struct EventRetinalDamageEffect
  position : float3
  distance : float

// retinal damage spot size does not decrease beyond this distance
let flash_blind_spot_attenuation_distance : float const = 50.

[es(tag=render, REQUIRE_NOT=deadEntity)]
def do_retinal_damage(evt : EventRetinalDamageEffect; eid : EntityId)
  let distance_factor = min(evt.distance / flash_blind_spot_attenuation_distance, 1.) - 1.
  let spot_scale = 3. - 2. * sqrt(1. - distance_factor * distance_factor)
  fire_retinal_damage_effect(eid, evt.position, spot_scale)

[es(tag=render, track=isAlive)]
def on_death_disable_retinal_damage(evt : Event; eid : EntityId; isAlive : bool)
  if !isAlive
    stop_retinal_damage_effect(eid)
