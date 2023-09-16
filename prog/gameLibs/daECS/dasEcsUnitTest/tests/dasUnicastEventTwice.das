require ecs
require DasEcsUnitTest
require events

[es(on_event=EventStart)]
def send_das_event_twice(evt : Event)
  set_test_value("receive_twice", 0)
  print("creating entity")
  let eid = createEntity("checkUnicastTwice")
  print("sending event")
  sendEvent(eid, [[DasUnicastEventTwice]])

[es(on_event=EventStart2)]
def send_das_event_twice2(evt : Event)
  print("creating entity2")
  let eid2 = createEntity("checkUnicastTwice")
  print("sending event2")
  sendEvent(eid2, [[DasUnicastEventTwice]])

[es(REQUIRE=checkUnicastTwice)]
def receive_das_event_twice(evt : DasUnicastEventTwice)
  print("receiving event")
  set_test_value("receive_twice", get_test_value("receive_twice") + 1)

[es(on_event=EventEnd)]
def verify_das_event_twice(evt : Event)
  verify(get_test_value("receive_twice") == 2)
