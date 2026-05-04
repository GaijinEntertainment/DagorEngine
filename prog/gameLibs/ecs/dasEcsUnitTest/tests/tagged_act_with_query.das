require ecs

// ES with query example
[es(tag = server)]
def act_with_query(info : UpdateStageInfoAct)
  let dt = info.dt
  ecs::query() <| $ [es] (var pos : float3&; vel : float3)
    pos += dt * vel

