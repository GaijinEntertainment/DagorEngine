module dashboard shared
require ecs
require remote.jsonrpc_boost
require daslib/json_boost


[generic]
def dashboard(name : string; value : auto(algebraic); priority : int = -1)
  send_notification("dashboard.set_value") <| JV(name, value, priority)

[generic]
def dashboard_s(name : string; value; priority : int = -1)
  send_notification("dashboard.set_string") <| JV(name, "{value}", priority)

def dashboard(name : string; value : string; priority : int = -1)
  send_notification("dashboard.set_string") <| JV(name, value, priority)

def dashboard(name : string; value : bool; priority : int = -1)
  send_notification("dashboard.set_string") <| JV(name, "{value}", priority)

def dashboard(name : string; value : float3; priority : int = -1)
  send_notification("dashboard.set_value") <| JV("{name}.x", value.x, priority)
  send_notification("dashboard.set_value") <| JV("{name}.y", value.y, priority)
  send_notification("dashboard.set_value") <| JV("{name}.z", value.z, priority)

def dashboard(name : string; value : float2; priority : int = -1)
  send_notification("dashboard.set_value") <| JV("{name}.x", value.x, priority)
  send_notification("dashboard.set_value") <| JV("{name}.y", value.y, priority)