require ecs
require DagorInput
require DngNet
require outer_space.events_outer_space
require outer_space.es.input.input_events


[es(tag=input, on_appear)]
def motorbike_get_up_input_appear_es(evt : Event; var motorbike_test__aGetUp : uint16&)
  motorbike_test__aGetUp = get_action_handle("Motorbike.GetUp", TYPEGRP_DIGITAL)


[es(tag=input)]
def motorbike_get_up_input_init_es(evt : EventDaInputInit; var motorbike_test__aGetUp : uint16&)
  motorbike_test__aGetUp = evt.init ? get_action_handle("Motorbike.GetUp", TYPEGRP_DIGITAL) : BAD_ACTION_HANDLE


// stands the fallen bike up at once, even while it still rolls
[es(tag=input)]
def motorbike_get_up_input_es(evt : EventDaInputActionTriggered; eid : EntityId; motorbike_test__aGetUp : uint16;
                              motorbike__fallen : bool)
  if evt.action == motorbike_test__aGetUp && motorbike__fallen
    send_net_event(eid, RequestMotorbikeGetUp())
