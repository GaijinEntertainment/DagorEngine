options no_aot
require ecs
require DagorMath
require DagorDebug3D
require math.color


def get_color(spline_idx : int)
  let colors = [[E3DCOLOR[6] E3DCOLOR(0xffffffff); E3DCOLOR(0xffffff40); E3DCOLOR(0xffff4040); E3DCOLOR(0xff8040ff); E3DCOLOR(0xff404040); E3DCOLOR(0xff40ff40)]]
  return colors[spline_idx % 6]


[es(no_order, tag=sound, on_event=EventSoundDrawDebug)]
def ocean_sound_debug_draw(evt : Event;
                           ocean_sound__points : Point3List;
                           ocean_sound__waterLevel : float)

  let numPoints = length(ocean_sound__points)
  if numPoints == 0
    return

  var idx = 0
  var firstIdx = 0
  var splineIdx = 0
  let visualOffset = float3(0., 1., 0.)

  while ++idx < numPoints
    let a = ocean_sound__points[idx - 1]
    let b = ocean_sound__points[idx]

    let p0 = float3(a.x, ocean_sound__waterLevel, a.y)
    let p1 = float3(b.x, ocean_sound__waterLevel, b.y)
    draw_debug_line_buffered(p0 + visualOffset, p1 + visualOffset, get_color(splineIdx), 2)
    add_debug_text_mark(p0, "{idx - 1}", -1, 0., get_color(splineIdx))

    if b.xy == ocean_sound__points[firstIdx].xy
      firstIdx = ++idx
      ++splineIdx
