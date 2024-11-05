options no_aot
require ecs
require math.base
require DagorMath
require DagorDebug3D


[es(no_order, tag=sound, on_event=EventSoundDrawDebug)]
def shore_sound_debug_draw(evt : Event;
                           shore_sound__offsets : Point2List;
                           transform : float3x4)
  for idx in range(length(shore_sound__offsets) - 1)
    draw_debug_line_buffered(transform * float3(shore_sound__offsets[idx].x, 0., shore_sound__offsets[idx].y),
                             transform * float3(shore_sound__offsets[idx + 1].x, 0., shore_sound__offsets[idx + 1].y),
                             E3DCOLOR(0xffffffff), 2)
  if length(shore_sound__offsets) >= 2
    draw_debug_line_buffered(transform * float3(shore_sound__offsets[0].x, 0., shore_sound__offsets[0].y),
                             transform * float3(shore_sound__offsets[length(shore_sound__offsets) - 1].x, 0., shore_sound__offsets[length(shore_sound__offsets) - 1].y),
                             E3DCOLOR(0xff8000ff), 2)
