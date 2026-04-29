require ecs
require DasEcsUnitTest
require %daslibEcs/ecs_events


[es(on_event=EventCPPTestEvent)] //for this Event we only know it's name
def EventCPPTestEvent_simple_receiver(evt : Event)
  assert(getEventName(evt) == "EventCPPTestEvent")
  set_test_value("EventCPPTestEvent_simple", 1)

[es]//verify we got it
def EventCPPTestEvent_simple_verifier(info : UpdateStageInfoAct)
  verify(get_test_value("EventCPPTestEvent_simple") == 1)
