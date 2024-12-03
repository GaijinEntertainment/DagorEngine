require ecs
require fx

[es(tag=render, no_order)]
def warmup_effects_update(info : UpdateStageInfoAct; eid : EntityId; warmup_effect__update : FloatList;
                          var warmup_effect__delay : float&)
  if warmup_effect__delay > 0f
    warmup_effect__delay -= info.dt
    return
  for dt in warmup_effect__update
    effects_update(dt)
  destroyEntity(eid)
