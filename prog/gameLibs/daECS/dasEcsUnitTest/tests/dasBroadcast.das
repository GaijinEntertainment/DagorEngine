options no_global_variables = false
require ecs

[event(broadcast)]
struct DasBroadcastEvent
  val : int

var
  sent, received : int

[es]
def send_das_broadcast_event(info : UpdateStageInfoAct)
  broadcastEvent(DasBroadcastEvent(val = sent))
  sent++

[es]
def receive_das_broadcast_event(evt : DasBroadcastEvent)
  assert(received <= sent)//because of delayed events, can be many more in queue
  received++

[es(on_event=EventEnd)]
def verify_receive_das_broadcast_event(evt : Event)
  assert(sent == 100)
  assert(sent == received)

