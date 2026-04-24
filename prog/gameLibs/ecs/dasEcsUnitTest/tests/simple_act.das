require ecs

//simple ES example
[es(REQUIRE=hasTag, REQUIRE_NOT=noTag)]
def simple_act_es(info : UpdateStageInfoAct; var pos : float3&; vel : float3)
  pos += info.dt * vel
