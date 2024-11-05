module net_draw_debug_common shared
require ecs
require net
require DagorMath
require DngNet
require DagorDebug3D
require ECSGlobalTags
require danetlibs.render_debug.main.render_debug_events

def net_draw_debug_line_buffered(p0 : float3; p1 : float3; color : E3DCOLOR; frames : int)
  if is_server()
    if ecs_has_tag("dev")
      broadcast_net_event([[CmdDebugDrawLine p0=p0, p1=p1, color=color, frames=frames]])
  else
    draw_debug_line_buffered(p0, p1, color, frames)

def net_draw_debug_sphere_buffered(p0 : float3; radius : float; color : E3DCOLOR; segments : int; frames : int)
  if is_server()
    if ecs_has_tag("dev")
      broadcast_net_event([[CmdDebugDrawSphere p0=p0, radius=radius, color=color, segments=segments, frames=frames]])
  else
    draw_debug_sphere_buffered(p0, radius, color, segments, frames)

def net_draw_debug_box_buffered(box : BBox3; color : E3DCOLOR; frames : int)
  if is_server()
    if ecs_has_tag("dev")
      broadcast_net_event([[CmdDebugDrawBox box_min=box.boxMin, box_max=box.boxMax, color=color, frames=frames]])
  else
    draw_debug_box_buffered(box, color, frames)
