options stack = 102400
require app
require ecs
require remote.jsonrpc_boost


[rpc_call(name="app.get_timespeed")]
def app_get_time_speed(id : uint64)
  send_response(JV(get_timespeed()), id)


[rpc_call(name="app.set_timespeed")]
def app_set_time_speed(value : float; id : uint64)
  set_timespeed(value)
  send_response(JV(get_timespeed()), id)


[rpc_call(name="app.toggle_pause")]
def app_toggle_pause(id : uint64)
  toggle_pause()
  send_response(JV(get_timespeed()), id)
