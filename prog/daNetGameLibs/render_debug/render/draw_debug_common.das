module draw_debug_common shared
require ecs
require DagorDebug3D
require DagorMath

def draw_cached_gizmo(tm : float3x4; axis_size : float; sphere_size : float)
  let forward = tm[3] + tm[0] * axis_size
  let up = tm[3] + tm[1] * axis_size
  let left = tm[3] + tm[2] * axis_size
  draw_cached_debug_line(tm[3], forward, E3DCOLOR(0xFFFF0000))
  draw_cached_debug_line(tm[3], up, E3DCOLOR(0xFF00FF00))
  draw_cached_debug_line(tm[3], left, E3DCOLOR(0xFF0000FF))

  draw_cached_debug_sphere(tm[3], sphere_size, E3DCOLOR(0xFFFFFFFF), 8)

def draw_gizmo_buffered(tm : float3x4; axis_size : float; sphere_size : float; frames : int = 1)
  let forward = tm[3] + tm[0] * axis_size
  let up = tm[3] + tm[1] * axis_size
  let left = tm[3] + tm[2] * axis_size
  draw_debug_line_buffered(tm[3], forward, E3DCOLOR(0xFFFF0000), frames)
  draw_debug_line_buffered(tm[3], up, E3DCOLOR(0xFF00FF00), frames)
  draw_debug_line_buffered(tm[3], left, E3DCOLOR(0xFF0000FF), frames)

  draw_debug_sphere_buffered(tm[3], sphere_size, E3DCOLOR(0xFFFFFFFF), 8, frames)
