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
