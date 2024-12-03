require ecs
require math.base
require das.main_events
require math.random

[es(tag=render, REQUIRE_NOT=deadEntity, no_order)]
def periodic_random_effect_spawner(info : ParallelUpdateFrameDelayed;
                                   transform : float3x4;
                                   [[shared_comp]] periodic_random_effects__templates : StringList;
                                   periodic_random_effects__spawnOffsetsX : float2;
                                   periodic_random_effects__spawnOffsetsY : float2;
                                   periodic_random_effects__spawnOffsetsZ : float2;
                                   periodic_random_effects__spawnPeriod : float2;
                                   var periodic_random_effects__nextSpawnTime : float&)
  if info.curTime < periodic_random_effects__nextSpawnTime
    return
  periodic_random_effects__nextSpawnTime = info.curTime + rnd_float(periodic_random_effects__spawnPeriod)
  let index = rnd_int(0, length(periodic_random_effects__templates) - 1)
  let effect = string(periodic_random_effects__templates[index])
  createEntity(effect) <| $(var init)
    var tm = IDENT_TM
    let x = rnd_float(periodic_random_effects__spawnOffsetsX)
    let y = rnd_float(periodic_random_effects__spawnOffsetsY) * rnd_sign()
    let z = rnd_float(periodic_random_effects__spawnOffsetsZ) * rnd_sign()
    tm[3] = transform * float3(x, y, z)
    init |> set("transform", tm)
