require ecs

[event(broadcast)]
struct DasEventA
  recursionCount : int

[event(broadcast)]
struct DasEventB
  recursionCount : int

[es]
def receive_das_eventA(evt : DasEventA)
  //print("A: rec={evt.recursionCount}")
  if evt.recursionCount > 0
    broadcastEvent([[DasEventB recursionCount = evt.recursionCount - 1]])

[es]
def receive_das_eventB(evt : DasEventB)
  //print("B: rec={evt.recursionCount}")
  if evt.recursionCount > 0
    broadcastEvent([[DasEventA recursionCount = evt.recursionCount - 1]])

[es(on_event=EventEntityDestroyed, REQUIRE=(pos, vel))]
def start_destroy_recursion(evt : Event)
  broadcastEvent([[DasEventA recursionCount = 1024 * 2 + 1]])

