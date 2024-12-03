module effect_controller_common shared
require ecs

struct EffectStateParams
  time : float2
  spawnRate : float2

def get_effect_state_params(effect_controller__states : Array; effect_controller__state : int) : EffectStateParams
  let obj = get_ecs_object(effect_controller__states[effect_controller__state])
  if obj == null
    return [[EffectStateParams time=float2(0, 0), spawnRate=float2(0, 0)]]
  let time = get_Point2(obj, "time") ?? float2(0, 0)
  let spawnRate = get_Point2(obj, "spawnRate") ?? float2(0, 0)
  return [[EffectStateParams time=time, spawnRate=spawnRate]]