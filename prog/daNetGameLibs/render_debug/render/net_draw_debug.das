options no_aot
require ecs
require DagorDebug3D
require DagorMath
require danetlibs.render_debug.main.net_draw_debug_common
require danetlibs.render_debug.main.render_debug_events

[es(tag=gameClient, no_order)]
def net_draw_debug_line_es(evt : CmdDebugDrawLine)
  draw_debug_line_buffered(evt.p0, evt.p1, evt.color, evt.frames)

[es(tag=gameClient, no_order)]
def net_draw_debug_sphere_es(evt : CmdDebugDrawSphere)
  draw_debug_sphere_buffered(evt.p0, evt.radius, evt.color, evt.segments, evt.frames)

[es(tag=gameClient, no_order)]
def net_draw_debug_box_es(evt : CmdDebugDrawBox)
  draw_debug_box_buffered(BBox3(evt.box_min, evt.box_max), evt.color, evt.frames)

[es(tag=gameClient, no_order)]
def net_draw_debug_tehedron_es(evt : CmdDebugDrawTehedron)
  draw_debug_tehedron_buffered(evt.p0, evt.radius, evt.color, evt.frames)

[es(tag=gameClient, no_order)]
def net_draw_debug_rect_es(evt : CmdDebugDrawRect)
  draw_debug_rect_buffered(evt.p0, evt.p1, evt.p2, evt.color, evt.frames)

[es(tag=gameClient, no_order)]
def net_draw_debug_circle_es(evt : CmdDebugDrawCircle)
  draw_debug_circle_buffered(evt.pos, evt.norm, evt.radius, evt.color, evt.segments, evt.frames)

[es(tag=gameClient, no_order)]
def net_draw_debug_capsule_es(evt : CmdDebugDrawCapsule)
  draw_debug_capsule_buffered(evt.p0, evt.p1, evt.radius, evt.color, evt.segments, evt.frames)

[es(tag=gameClient, no_order)]
def net_draw_debug_arrow_es(evt : CmdDebugDrawArrow)
  draw_debug_arrow_buffered(evt.p0, evt.p1, evt.color, evt.frames)

[es(tag=gameClient, no_order)]
def net_draw_debug_cone_es(evt : CmdDebugDrawCone)
  draw_debug_cone_buffered(evt.p0, evt.p1, evt.angle, evt.color, evt.segs, evt.frames)

[es(tag=gameClient, no_order)]
def net_draw_debug_tube_es(evt : CmdDebugDrawTube)
  draw_debug_tube_buffered(evt.p0, evt.p1, evt.radius, evt.color, evt.segs, evt.circle_density, evt.frames)

[es(tag=gameClient, no_order)]
def net_draw_debug_tetrapod_es(evt : CmdDebugDrawTetrapod)
  draw_debug_tetrapod_buffered(evt.p0, evt.radius, evt.color, evt.frames)
