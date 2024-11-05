options no_aot
require ecs
require DagorMath
require DagorDebug3D
require Dacoll


let DEBUG_ADD_DRAW_HEIGHT = 0.5


[es(no_order, tag=sound, on_event=EventSoundDrawDebug)]
def spline_sound_debug_draw(evt : Event;
                            spline_sound__points : Point4List;
                            spline_sound__floorOnWater : bool)
  var idx = 0
  for pt in spline_sound__points
    if pt.w > 0. // not a separator && nonzero radius
      var pos = pt.xyz
      if spline_sound__floorOnWater
        traceht_water(pos, pos.y)
      pos.y += DEBUG_ADD_DRAW_HEIGHT
      draw_debug_sphere_buffered(pos, pt.w, E3DCOLOR(0xffffffff), 24, 1)
      draw_debug_line_buffered(pos, pos + float3(20., 0., 0.), E3DCOLOR(0xffff0000), 2)
      draw_debug_line_buffered(pos, pos + float3(0., 0., 20.), E3DCOLOR(0xff0000ff), 2)
      add_debug_text_mark(pos, "{idx}", -1, 0., E3DCOLOR(0xffffffff))
    ++idx

  idx = 0
  while idx + 1 < length(spline_sound__points)
    let p0 = spline_sound__points[idx]
    let p1 = spline_sound__points[idx + 1]
    if p1.w < 0.
      idx += 2
    else
      var a = p0.xyz
      var b = p1.xyz
      if spline_sound__floorOnWater
        traceht_water(a, a.y)
        traceht_water(b, b.y)
      a.y += DEBUG_ADD_DRAW_HEIGHT
      b.y += DEBUG_ADD_DRAW_HEIGHT
      let tan = normalize(float3(b.z - a.z, 0., a.x - b.x))
      draw_debug_line_buffered(a + tan * p0.w, b + tan * p1.w, E3DCOLOR(0xffffffff), 2)
      draw_debug_line_buffered(a - tan * p0.w, b - tan * p1.w, E3DCOLOR(0xffffffff), 2)
      ++idx
