require ecs

[es(REQUIRE=hasTag, REQUIRE_NOT=noTag)]
def test_ecs_get_es(info : UpdateStageInfoAct; eid : EntityId; pos : float3; vel : float3)
  verify(eid == get_Eid(eid, "eid") ?? INVALID_ENTITY_ID)
  verify(pos == get_Point3(eid, "pos") ?? float3())
  verify(vel == get_Point3(eid, "vel") ?? float3())

  let newPos1 = float3(2, 3, 4)
  set(eid, "pos", newPos1)
  verify(newPos1 == get_Point3(eid, "pos") ?? float3())

  let newPos2 = float3(2, 3, 4)
  setOptional(eid, "pos", newPos2)
  verify(newPos2 == get_Point3(eid, "pos") ?? float3())
