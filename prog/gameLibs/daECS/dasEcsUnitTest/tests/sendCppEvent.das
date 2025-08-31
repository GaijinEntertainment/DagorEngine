options no_global_variables = false
require ecs
require DasEcsUnitTest
require events

[cpp_event(broadcast)]
struct EventCPPSimpleTestEvent
  f : float
  i : int

var
  received : int

[es]
def on_das_cpp_event_handler_es(evt : EventCPPSimpleTestEvent)
  assert(evt.i == received)
  assert(evt.f == float(received))
  ++received
  set_test_value("EventCPPSimpleTestEvent", received)

[es(on_event=EventStart)]
def on_das_cpp_event_start_handler_es(evt : Event)
  for i in range(0, 20)
    broadcastEvent(EventCPPSimpleTestEvent(f = float(i), i = i))

[es(on_event=EventEnd)]
def on_das_cpp_event_ent_handler_es(evt : Event)
  verify(get_test_value("EventCPPSimpleTestEvent") == 20)


[cpp_event(broadcast)]
struct EventCPPTestStringEvent
  s : string

[es]
def on_string_cpp_event_handler(evt : EventCPPTestStringEvent)
  assert(evt.s == "test_copied_string")
  set_test_value("EventCPPTestStringEvent", 1)

[es]
def on_empty_event_handler(evt : EventEmpty)
  set_test_value("EventEmpty", 1)

[es(on_event=EventStart)]
def das_send_complex_cpp_event(evt : Event)
  broadcastEvent(EventCPPTestStringEvent(s = "test_copied_string"))
  broadcastEvent(EventEmpty())

[es(on_event=EventEnd)]
def on_string_cpp_event_handler_verifier(evt : Event)
  verify(get_test_value("EventCPPTestStringEvent") == 1)
  verify(get_test_value("EventEmpty") == 1)
