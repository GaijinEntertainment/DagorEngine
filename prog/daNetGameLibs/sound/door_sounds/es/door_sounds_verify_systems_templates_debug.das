options no_aot
require ecs
require ecs.verify_common
require ecs.verify_macro

/* TODO: unshare(move from dng)
[req_template]
struct DoorWithSound
  door_operations__curState : bool
  door_sound__openPath : string
  door_sound__closePath : string
  transform : float3x4
*/