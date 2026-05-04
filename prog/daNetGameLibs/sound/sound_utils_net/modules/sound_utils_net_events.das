module sound_utils_net_events shared
require ecs
require gameLibs.pufd_events.pufd_events public

[event(unicast, routing=ROUTING_SERVER_TO_CLIENT)]
struct CmdPlayNetSound
  hash : uint
  param : float
  time : float

[event(unicast, routing=ROUTING_SERVER_TO_CLIENT)]
struct CmdPlayNetSoundPos
  hash : uint
  pos : float3
  param : float
  time : float

[event(unicast, routing=ROUTING_SERVER_TO_CLIENT)]
struct CmdPlaySceneNetSound
  hash : uint
  param : float
  time : float
