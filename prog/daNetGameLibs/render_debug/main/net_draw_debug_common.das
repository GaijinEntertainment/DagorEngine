module net_draw_debug_common shared
require ecs
require net
require DagorMath
require DagorDebug3D
require ECSGlobalTags
require danetlibs.render_debug.main.render_debug_events

def net_draw_debug_line_buffered(p0 : float3; p1 : float3; color : E3DCOLOR; frames : int)
  if is_server()
    if ecs_has_tag("dev")
      broadcast_net_event(CmdDebugDrawLine(p0 = p0, p1 = p1, color = color, frames = frames))
  else
    draw_debug_line_buffered(p0, p1, color, frames)

def net_draw_debug_sphere_buffered(p0 : float3; radius : float; color : E3DCOLOR; segments : int; frames : int)
  if is_server()
    if ecs_has_tag("dev")
      broadcast_net_event(CmdDebugDrawSphere(p0 = p0, radius = radius, color = color, segments = segments, frames = frames))
  else
    draw_debug_sphere_buffered(p0, radius, color, segments, frames)

def net_draw_debug_box_buffered(box : BBox3; color : E3DCOLOR; frames : int)
  if is_server()
    if ecs_has_tag("dev")
      broadcast_net_event(CmdDebugDrawBox(box_min = box.boxMin, box_max = box.boxMax, color = color, frames = frames))
  else
    draw_debug_box_buffered(box, color, frames)

def net_draw_debug_tehedron_buffered(p0 : float3; radius : float; color : E3DCOLOR; frames : int)
  if is_server()
    if ecs_has_tag("dev")
      broadcast_net_event(CmdDebugDrawTehedron(p0 = p0, radius = radius, color = color, frames = frames))
  else
    draw_debug_tehedron_buffered(p0, radius, color, frames)

def net_draw_debug_rect_buffered(p0 : float3; p1 : float3; p2 : float3; color : E3DCOLOR; frames : int)
  if is_server()
    if ecs_has_tag("dev")
      broadcast_net_event(CmdDebugDrawRect(p0 = p0, p1 = p1, p2 = p2, color = color, frames = frames))
  else
    draw_debug_rect_buffered(p0, p1, p2, color, frames)

def net_draw_debug_circle_buffered(pos : float3; norm : float3; radius : float; color : E3DCOLOR; segments : int; frames : int)
  if is_server()
    if ecs_has_tag("dev")
      broadcast_net_event(CmdDebugDrawCircle(pos = pos, norm = norm, radius = radius, color = color, segments = segments, frames = frames))
  else
    draw_debug_circle_buffered(pos, norm, radius, color, segments, frames)

def net_draw_debug_capsule_buffered(p0 : float3; p1 : float3; radius : float; color : E3DCOLOR; segments : int; frames : int)
  if is_server()
    if ecs_has_tag("dev")
      broadcast_net_event(CmdDebugDrawCapsule(p0 = p0, p1 = p1, radius = radius, color = color, segments = segments, frames = frames))
  else
    draw_debug_capsule_buffered(p0, p1, radius, color, segments, frames)

def net_draw_debug_arrow_buffered(p0 : float3; p1 : float3; color : E3DCOLOR; frames : int)
  if is_server()
    if ecs_has_tag("dev")
      broadcast_net_event(CmdDebugDrawArrow(p0 = p0, p1 = p1, color = color, frames = frames))
  else
    draw_debug_arrow_buffered(p0, p1, color, frames)

def net_draw_debug_cone_buffered(p0 : float3; p1 : float3; angle : float; color : E3DCOLOR; segs : int; frames : int)
  if is_server()
    if ecs_has_tag("dev")
      broadcast_net_event(CmdDebugDrawCone(p0 = p0, p1 = p1, angle = angle, color = color, segs = segs, frames = frames))
  else
    draw_debug_cone_buffered(p0, p1, angle, color, segs, frames)

def net_draw_debug_tube_buffered(p0 : float3; p1 : float3; radius : float; color : E3DCOLOR; segs : int; circle_density : float; frames : int)
  if is_server()
    if ecs_has_tag("dev")
      broadcast_net_event(CmdDebugDrawTube(p0 = p0, p1 = p1, radius = radius, color = color, segs = segs, circle_density = circle_density, frames = frames))
  else
    draw_debug_tube_buffered(p0, p1, radius, color, segs, circle_density, frames)

def net_draw_debug_tetrapod_buffered(p0 : float3; radius : float; color : E3DCOLOR; frames : int)
  if is_server()
    if ecs_has_tag("dev")
      broadcast_net_event(CmdDebugDrawTetrapod(p0 = p0, radius = radius, color = color, frames = frames))
  else
    draw_debug_tetrapod_buffered(p0, radius, color, frames)
