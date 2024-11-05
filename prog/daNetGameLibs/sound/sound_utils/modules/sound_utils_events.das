module sound_utils_events shared
require ecs

[cpp_event(broadcast)]
struct EventOnSoundPresetLoaded
  hash : uint
  isLoaded : bool

[event(unicast)]
struct CmdPlaySound
  hash : uint
  pos : float3
  param : float

[event(unicast)]
struct CmdPlaySoundSimple
  hash : uint

[cpp_event(unicast)]
struct EventOnCollision
  vel : float3
  pvel : float3
  pos : float3
  offender : EntityId
  cvel : float3
  dt : float
  damage : float
