options no_global_variables = false
require ecs
require events

var
  sent, received : int

[es]
def send_das_event(info : UpdateStageInfoAct)
  var wasSent = false
  ecs::query() <| $ [es] (eid : EntityId; var ev_pos : float3&; ev_vel : float3; var ev_val : int&)
    ev_pos += info.dt * ev_vel
    ev_val++
    sendEvent(eid, DasUnicastEvent(pos = ev_pos, vel = ev_vel, val = ev_val, at = sent))
    wasSent = true
  assert(wasSent)
  sent++

[es]
def receive_das_event(evt : DasUnicastEvent; ev_val : int; ev_vel : float3)
  assert(evt.val <= ev_val)//since events can be delayed, only ordering is guaranteed
  assert(evt.vel == ev_vel)
  assert(evt.at == received)
  received++

[es(on_event=EventEnd)]
def verify_receive_das_event(evt : Event)
  assert(sent == 100)
  assert(sent == received)

