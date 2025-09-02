require DagorDebug3DSolid
require DagorMath
require ecs
options indenting = 2

[generic]
def draw_debug_tm_buffered(tm : float3x4 | #; color : Color4; frames : int)
  let box = BBox3(float3(), 1.0)
  _builtin_draw_debug_cube_buffered(box, tm, color, frames)

[generic]
def draw_debug_tm_buffered(tm : float3x4 | #; color : E3DCOLOR; frames : int)
  draw_debug_tm_buffered(tm, Color4(color), frames)

[generic]
def draw_debug_cube_buffered(box : BBox3; tm : float3x4; color : Color4; frames : int)
  _builtin_draw_debug_cube_buffered(box, tm, color, frames)

[generic]
def draw_debug_cube_buffered(box : BBox3; tm : float3x4; color : E3DCOLOR; frames : int)
  _builtin_draw_debug_cube_buffered(box, tm, Color4(color), frames)

[generic]
def draw_debug_triangle_buffered(a, b, c : float3; color : Color4; frames : int)
  _builtin_draw_debug_triangle_buffered(a, b, c, color, frames)

[generic]
def draw_debug_triangle_buffered(a, b, c : float3; color : E3DCOLOR; frames : int)
  _builtin_draw_debug_triangle_buffered(a, b, c, Color4(color), frames)

[generic]
def draw_debug_quad_buffered(top_left, bottom_left, bottom_right, top_right : float3; color : Color4; frames : int)
  _builtin_draw_debug_quad_buffered(top_left, bottom_left, bottom_right, top_right, color, frames)

[generic]
def draw_debug_quad_buffered(top_left, bottom_left, bottom_right, top_right : float3; color : E3DCOLOR; frames : int)
  _builtin_draw_debug_quad_buffered(top_left, bottom_left, bottom_right, top_right, Color4(color), frames)

[generic]
def draw_debug_quad_buffered(half_vec_i, half_vec_j : float3; tm : float3x4; color : Color4; frames : int)
  _builtin_draw_debug_quad_buffered(half_vec_i, half_vec_j, tm, color, frames)

[generic]
def draw_debug_quad_buffered(half_vec_i, half_vec_j : float3; tm : float3x4; color : E3DCOLOR; frames : int)
  _builtin_draw_debug_quad_buffered(half_vec_i, half_vec_j, tm, Color4(color), frames)

[generic]
def draw_debug_ball_buffered(sphere_c : float3; sphere_r : float; color : Color4; frames : int)
  _builtin_draw_debug_ball_buffered(sphere_c, sphere_r, color, frames)

[generic]
def draw_debug_ball_buffered(sphere_c : float3; sphere_r : float; color : E3DCOLOR; frames : int)
  _builtin_draw_debug_ball_buffered(sphere_c, sphere_r, Color4(color), frames)

[generic]
def draw_debug_disk_buffered(pos, norm : float3; radius : float; color : E3DCOLOR; frames : int)
  _builtin_draw_debug_disk_buffered(pos, norm, radius, -1, Color4(color), frames)

[generic]
def draw_debug_disk_buffered(pos, norm : float3; radius : float; segments : int; color : E3DCOLOR; frames : int)
  _builtin_draw_debug_disk_buffered(pos, norm, radius, segments, Color4(color), frames)

[generic]
def draw_debug_text_mark_buffered(pos : float3x4; msg : string; duration : int; color : E3DCOLOR = E3DCOLOR(0xffffffff))
  draw_debug_text_mark_buffered(pos[3], msg, duration, color)

[generic]
def draw_debug_text_mark_buffered(pos : float3; msg : string; duration : int; color : E3DCOLOR = E3DCOLOR(0xffffffff))
  createEntity("buffered_text_mark") <| $(init)
    init |> set("buffered_text_mark__pos", pos)
    init |> set("buffered_text_mark__msg", msg)
    init |> set("buffered_text_mark__left", duration)
    init |> set("buffered_text_mark__color", color)