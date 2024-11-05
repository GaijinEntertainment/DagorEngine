options stack = 102400
require ecs
require DagorConsole
require DagorSystem
require remote.jsonrpc_boost


[rpc_notification_raw(name="visual_log")]
def rpc_visual_log(var params : JsonValue?)
  visual_log(write_json(params))

[rpc_notification_raw(name="print")]
def rpc_print(var params : JsonValue?)
  print(write_json(params))

[rpc_notification_raw(name="logerr")]
def rpc_logerr(var params : JsonValue?)
  logerr(write_json(params))

[rpc_notification_raw(name="console_command")]
def rpc_console_cmd(var params : JsonValue?)
  console_command(write_json(params))
