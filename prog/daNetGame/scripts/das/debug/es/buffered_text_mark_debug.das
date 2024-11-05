options no_aot
require ecs
require app
require DagorDebug3D
require DagorMath
require ecs.ecs_template

[ecs_template]
struct buffered_text_mark
  buffered_text_mark__pos : float3
  buffered_text_mark__left : int
  buffered_text_mark__msg : string
  buffered_text_mark__color : E3DCOLOR
  noECSDebug : Tag


[es(tag=(gameClient), no_order)]
def draw_debug_textmarks(act : UpdateStageInfoRenderDebug;
                         eid : EntityId;
                         buffered_text_mark__pos : float3;
                         buffered_text_mark__color : E3DCOLOR;
                         buffered_text_mark__msg : string;
                         var buffered_text_mark__left : int&)
  if get_timespeed() > .0 || buffered_text_mark__left == 1
    buffered_text_mark__left--
  if buffered_text_mark__left <= 0
    destroyEntity(eid)
  add_debug_text_mark(buffered_text_mark__pos, buffered_text_mark__msg, -1, 0., buffered_text_mark__color)