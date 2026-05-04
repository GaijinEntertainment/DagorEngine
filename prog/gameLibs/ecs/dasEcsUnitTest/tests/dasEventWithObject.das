require ecs
require DasEcsUnitTest
require events

[es(on_event=EventStart)]
def send_das_event_with_object(evt : Event)
  set_test_value("object_received", 0)
  set_test_value("null_object_received", 0)
  print("broadcast event with object")
  using() <| $(var obj : Object)
    obj |> set("i", 1)
    obj |> set("f", 2.3)
    obj |> set("s", "str")
    broadcastEvent(EventWithObject(name = "test event", damage = 42, data = ecs_addr(obj)))

  broadcastEvent(EventWithNullObject(data = null))

[es]
def receive_das_event_with_null_object(evt : EventWithNullObject)
  debug(evt, "received event with null object")
  verify(evt.data != null) // object is never null here!
  verify(length(*evt.data) == 0) // object is empty as expected
  set_test_value("null_object_received", 1)

[es]
def receive_das_event_with_object(evt : EventWithObject)
  debug(evt, "received event with object")
  verify(evt.name == "test event")
  verify(evt.damage == 42)
  verify(evt.data != null)
  verify(evt.data |> get_int("i") ?? -1 == 1)
  verify(evt.data |> get_float("f") ?? -1. == 2.3)
  verify(evt.data |> get_ecs_string("s") != null)
  verify("{evt.data |> get_ecs_string("s")}" == "str")
  set_test_value("object_received", 1)

[es(on_event=EventEnd)]
def verify_das_event_with_object(evt : Event)
  print("verify event with object")
  verify(get_test_value("object_received") == 1)
  print("verify event with null object")
  verify(get_test_value("null_object_received") == 1)
