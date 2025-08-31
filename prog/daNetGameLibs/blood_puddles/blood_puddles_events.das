module blood_puddles_events shared

require ecs
require RendInst

[event(unicast)]
struct CmdAddBloodPuddleAndDie
  pos : float3
  normal : float3
  puddleDist : float
  riEx : riex_handle_t

[event(broadcast)]
struct BloodPuddlesLeakDecalEvent
  pos : float3
  normal : float3
  size : float
  ri_desc_handle : riex_handle_t
  hit_pos : float3
  projective : bool
  texture_variant : int

[event(broadcast)]
struct BloodPuddlesSplashDecalEvent
  pos : float3
  normal : float3
  size : float
  ri_desc_handle : riex_handle_t
  hit_pos : float3
  projective : bool

[event(broadcast)]
struct BloodPuddlesSprayDecalEvent
  pos : float3
  normal : float3
  size : float
  ri_desc_handle : riex_handle_t
  hit_pos : float3
  projective : bool

[event(broadcast)]
struct BloodPuddlesSplashEmmiterEvent
  start_pos : float3
  target_pos : float3
  normal : float3
  dir : float3
  size : float
  ri_desc_handle : riex_handle_t
  grav : float3

[event(broadcast)]
struct BloodPuddlesExplosionEmmiterEvent
  pos : float3
  normal : float3
  size : float
  ri_desc_handle : riex_handle_t
  hit_pos : float3
  projective : bool

[cpp_event(unicast)]
struct EventBloodFootStep
  pos : float3
  dir : float3
  nodeId : int