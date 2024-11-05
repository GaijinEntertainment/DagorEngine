module puddles_manager_events shared
require ecs

[cpp_event(broadcast)]
struct RemovePuddlesInRadius
  position : float3
  radius : float