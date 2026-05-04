require ecs
require DasEcsUnitTest
require events
require %daslibEcs/ecs_events


[cpp_event(broadcast)]
struct EventCPPTestEvent
  f : float
  i : int
  s : string

[es]
def EventCPPTestEvent_receiver(evt : EventCPPTestEvent)
  verify(getEventName(evt) == "EventCPPTestEvent")
  assert(evt.f == 2.)
  assert(evt.i == 10)
  assert(evt.s == "test_string")
  set_test_value("EventCPPTestEvent", 1)

[es]
def EventCPPTestEvent_verifier(info : UpdateStageInfoAct)
  verify(get_test_value("EventCPPTestEvent") == 1)

[es(on_event=EventStart)]
def on_cpp_event_start_handler_es(evt : Event)
  verify(getEventName(evt) == "EventStart")
  let counter = get_test_value("EventStartTriggered")
  set_test_value("EventStartTriggered", counter < 0 ? 1 : counter + 1)

[es(on_event=EventEnd)]
def on_cpp_event_ent_handler_es(evt : Event)
  let counter = get_test_value("EventEndTriggered")
  set_test_value("EventEndTriggered", counter < 0 ? 1 : counter + 1)
