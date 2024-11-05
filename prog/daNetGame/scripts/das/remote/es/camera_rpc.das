options stack = 102400
require ecs
require DagorConsole
require remote.jsonrpc_boost


[rpc_notification(name="camera.pos")]
def rpc_camera_pos(x, y, z : float)
  console_command("camera.pos {x} {y} {z}")


[rpc_notification(name="camera.pos_eid")]
def rpc_camera_pos_eid(eid : uint; x, y, z : float)
  query(EntityId(eid)) <| $ [es] (transform : float3x4)
    let pos = transform[3] + float3(x, y, z)
    console_command("camera.pos {pos.x} {pos.y} {pos.z}")


[rpc_notification(name="camera.spectate")]
def rpc_camera_spectate(eid : int)
  console_command("camera.spectate {eid}")
