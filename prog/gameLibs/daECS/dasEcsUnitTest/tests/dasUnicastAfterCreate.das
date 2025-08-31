options no_global_variables = false
require ecs
require events

var
  sent, received : int

// The code sends UnicastAfterCreateTestEvent
// On receive, the event handler sends the same event again
// To control number of sends, the event is accompanied with a counter
// Each time the event is sent, the "sent" var is incremented
// Each time the event is received, the "received" var is incremented
// In ther end, we expect sent == received == 2

def sendDasEmptyUnicastEvent(counter : int)
  if counter != 0
    let eid = createEntity("unicastAfterCreateEntity")
    sendEvent(eid, UnicastAfterCreateTestEvent(counter = counter))
    debug("Sending UnicastAfterCreateTestEvent event to {int(uint(eid))}")
    sent++

[es(on_event=EventEnd)]
def verify_receive_after_create_event(evt : Event)
  debug("UnicastAfterCreateTestEvent is sent/received {sent}/{received}")
  assert(sent == 2)
  assert(received == 2)

[es(REQUIRE=unicastAfterCreateEntity)]
def receive_das_event_and_send_next(evt : UnicastAfterCreateTestEvent; eid : EntityId)
  debug("Receiving UnicastAfterCreateTestEvent event to {int(uint(eid))}")
  received++
  sendDasEmptyUnicastEvent(evt.counter - 1)

[es(on_event=EventStart)]
def send_initial_unicast_event(evt : Event)
  sendDasEmptyUnicastEvent(2)
