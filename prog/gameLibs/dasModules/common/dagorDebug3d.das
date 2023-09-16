options indenting = 2
require DagorDebug3D
require DagorMath

[generic]
def draw_debug_line_buffered(p0, p1 : float3; color : E3DCOLOR; frames : int)
  _builtin_draw_debug_line_buffered(p0, p1, color, frames)

[generic]
def draw_debug_rect_buffered(p0, p1, p2 : float3; color : E3DCOLOR; frames : int)
  _builtin_draw_debug_rect_buffered(p0, p1, p2, color, frames)

[generic]
def draw_debug_box_buffered(box : BBox3; color : E3DCOLOR; frames : int)
  _builtin_draw_debug_box_buffered(box, color, frames)

[generic]
def draw_debug_box_buffered(p1, p2 : float3; color : E3DCOLOR; frames : int)
  let box = BBox3(p1, p2)
  _builtin_draw_debug_box_buffered(box, color, frames)

[generic]
def draw_debug_box_buffered(p0, ax, ay, az : float3; color : E3DCOLOR; frames : int)
  _builtin_draw_debug_box_buffered(p0, ax, ay, az, color, frames)

[generic]
def draw_debug_box_buffered(box : BBox3; tm : float3x4; color : E3DCOLOR; frames : int)
  _builtin_draw_debug_box_buffered(box, tm, color, frames)

[generic]
def draw_debug_elipse_buffered(pos, half_diag1, half_diag2 : float3; color : E3DCOLOR; segments : int; frames : int)
  _builtin_draw_debug_elipse_buffered(pos, half_diag1, half_diag2, color, segments, frames)

[generic]
def draw_debug_circle_buffered(pos, norm : float3; radius; color : E3DCOLOR; segments : int; frames : int)
  _builtin_draw_debug_circle_buffered(pos, norm, float(radius), color, segments, frames)

[generic]
def draw_debug_sphere_buffered(p0 : float3; radius; color : E3DCOLOR; segments : int; frames : int)
  _builtin_draw_debug_sphere_buffered(p0, float(radius), color, segments, frames)

[generic]
def draw_debug_sphere_buffered(p0 : float3; radius; color : E3DCOLOR; frames : int)
  _builtin_draw_debug_sphere_buffered(p0, float(radius), color, 12, frames)

[generic]
def draw_debug_tube_buffered(p0, p1 : float3; radius; color : E3DCOLOR; segs : int; circle_density; frames : int)
  _builtin_draw_debug_tube_buffered(p0, p1, float(radius), color, segs, float(circle_density), frames)

[generic]
def draw_debug_cone_buffered(p0, p1 : float3; radius; color : E3DCOLOR; segs : int; frames : int)
  _builtin_draw_debug_cone_buffered(p0, p1, float(radius), color, segs, frames)

[generic]
def draw_debug_capsule_buffered(p0, p1 : float3; radius; color : E3DCOLOR; segments : int; frames : int)
  _builtin_draw_debug_capsule_buffered(p0, p1, float(radius), color, segments, frames)

[generic]
def draw_debug_arrow_buffered(from, to : float3; color : E3DCOLOR; frames : int)
  _builtin_draw_debug_arrow_buffered(from, to, color, frames)

[generic]
def draw_cached_debug_quad(p0, p1, p2, p3 : float3; color : E3DCOLOR)
  _builtin_draw_cached_debug_quad(p0, p1, p2, p3, color)

[generic]
def draw_cached_debug_trilist(arr : array<float3> | float3[]; color : E3DCOLOR)
  verify(length(arr) % 3 == 0)
  _builtin_draw_cached_debug_trilist(unsafe(addr(arr[0])), length(arr) / 3, color)

//Use negative radius to turn it upside down
[generic]
def draw_debug_tetrapod_buffered(p0 : float3; radius; color : E3DCOLOR; frames : int)
  _builtin_draw_debug_tetrapod_buffered(p0, float(radius), color, frames)

[generic]
def draw_debug_tehedron_buffered(p0 : float3; radius; color : E3DCOLOR; frames : int)
  _builtin_draw_debug_tehedron_buffered(p0, float(radius), color, frames)
