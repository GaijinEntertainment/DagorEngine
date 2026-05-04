module render_debug_events shared
require ecs
require DagorMath

[event(broadcast, routing=ROUTING_SERVER_TO_CLIENT, net_liable=ignore)]
struct CmdDebugDrawLine
  p0 : float3
  p1 : float3
  color : E3DCOLOR
  frames : int

[event(broadcast, routing=ROUTING_SERVER_TO_CLIENT, net_liable=ignore)]
struct CmdDebugDrawSphere
  p0 : float3
  radius : float
  color : E3DCOLOR
  segments : int
  frames : int

[event(broadcast, routing=ROUTING_SERVER_TO_CLIENT, net_liable=ignore)]
struct CmdDebugDrawBox
  box_min : float3
  box_max : float3
  color : E3DCOLOR
  frames : int

[event(broadcast, routing=ROUTING_SERVER_TO_CLIENT, net_liable=ignore)]
struct CmdDebugDrawTehedron
  p0 : float3
  radius : float
  color : E3DCOLOR
  frames : int

[event(broadcast, routing=ROUTING_SERVER_TO_CLIENT, net_liable=ignore)]
struct CmdDebugDrawRect
  p0 : float3
  p1 : float3
  p2 : float3
  color : E3DCOLOR
  frames : int

[event(broadcast, routing=ROUTING_SERVER_TO_CLIENT, net_liable=ignore)]
struct CmdDebugDrawCircle
  pos : float3
  norm : float3
  radius : float
  color : E3DCOLOR
  segments : int
  frames : int

[event(broadcast, routing=ROUTING_SERVER_TO_CLIENT, net_liable=ignore)]
struct CmdDebugDrawCapsule
  p0 : float3
  p1 : float3
  radius : float
  color : E3DCOLOR
  segments : int
  frames : int

[event(broadcast, routing=ROUTING_SERVER_TO_CLIENT, net_liable=ignore)]
struct CmdDebugDrawArrow
  p0 : float3
  p1 : float3
  color : E3DCOLOR
  frames : int

[event(broadcast, routing=ROUTING_SERVER_TO_CLIENT, net_liable=ignore)]
struct CmdDebugDrawCone
  p0 : float3
  p1 : float3
  angle : float
  color : E3DCOLOR
  segs : int
  frames : int

[event(broadcast, routing=ROUTING_SERVER_TO_CLIENT, net_liable=ignore)]
struct CmdDebugDrawTube
  p0 : float3
  p1 : float3
  radius : float
  color : E3DCOLOR
  segs : int
  circle_density : float
  frames : int

[event(broadcast, routing=ROUTING_SERVER_TO_CLIENT, net_liable=ignore)]
struct CmdDebugDrawTetrapod
  p0 : float3
  radius : float
  color : E3DCOLOR
  frames : int
