module input_events shared
require ecs
require DagorInput

[cpp_event(broadcast)]
struct EventDaInputInit
  init : bool

[cpp_event(broadcast)]
struct EventDaInputActionTriggered
  action : action_handle_t
  duration_ms : uint16

[cpp_event(broadcast)]
struct EventDaInputActionTerminated
  action : action_handle_t
  duration_ms : uint16

[cpp_event(broadcast)]
struct EventHidGlobalInputSink
  t0_msec : uint
  dev : int
  pressed : bool
  button_id : uint
