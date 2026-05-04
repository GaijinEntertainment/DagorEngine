module events

require ecs


[event(unicast)]
struct DasUnicastEvent
  pos, vel : float3
  val, at : int

[cpp_event(broadcast)]
struct EventEmpty {}

[event(unicast)]
struct DasUnicastEventTwice {}

[event(unicast)]
struct UnicastAfterCreateTestEvent
  counter : int

[event(broadcast)]
struct EventWithObject
  name : string
  damage : int
  data : Object const?

[event(broadcast)]
struct EventWithNullObject
  data : Object const?
