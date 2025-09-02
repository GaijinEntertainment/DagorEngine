require ecs
require DasEcsUnitTest
require events

[event(broadcast)]
struct DasBoolEvent
  b : bool

[es(on_event=EventStart)]
def das_send_complex_cpp_event2(evt : Event)
  broadcastEvent(EventEmpty())
  broadcastEvent(DasBoolEvent())

