require ecs

[es(on_event=EventStart)]
def get_or_create_singleton_es(evt : Event)
  verify(getSingletonEntity("testSingleton") == INVALID_ENTITY_ID)
  let eid = getOrCreateSingletonEntity("testSingleton")
  verify(eid != INVALID_ENTITY_ID)
  verify(getOrCreateSingletonEntity("testSingleton") == eid)
  verify(getSingletonEntity("testSingleton") == eid)
  verify((get_int(eid, "test_singleton_value") ?? 0) == 7)
