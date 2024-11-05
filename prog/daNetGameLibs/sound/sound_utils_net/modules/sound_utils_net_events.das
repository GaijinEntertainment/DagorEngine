module sound_utils_net_events shared
require ecs

[event(unicast, routing=ROUTING_SERVER_TO_CLIENT)]
struct CmdPlayNetSound
  hash : uint
  param : float
  time : float

[event(unicast, routing=ROUTING_SERVER_TO_CLIENT)]
struct CmdPlayNetSoundPos
  hash : uint
  pos : float3
  time : float

[event(unicast, routing=ROUTING_SERVER_TO_CLIENT)]
struct CmdPlayNetSoundFromAnySource
  hash : uint
  time : float


//may move this events to single module in daNetGameLibs to share it across all da-net-game projects
[cpp_event(broadcast)]
struct ParallelUpdateFrameDelayed
  curTime : float
  dt : float
