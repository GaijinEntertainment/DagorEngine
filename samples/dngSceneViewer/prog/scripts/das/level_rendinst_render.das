require ecs
require RendInst
require DagorMath

[es(tag=render, on_event=EventLevelLoaded)]
def level_load_ri_colors_es(evt : Event; var global_fx__rendinstColors : Point4List&)
  get_ri_color_infos <| $ [unused_argument(name)] (colorFrom : E3DCOLOR; colorTo : E3DCOLOR; name : string#)
    let u4ColorFrom = uint4(colorFrom)
    let p4ColorFrom = float4(float(u4ColorFrom[0]) / 255.0, float(u4ColorFrom[1]) / 255.0, float(u4ColorFrom[2]) / 255.0, float(u4ColorFrom[3]) / 255.0)
    let u4ColorTo = uint4(colorTo)
    let p4ColorTo = float4(float(u4ColorTo[0]) / 255.0, float(u4ColorTo[1]) / 255.0, float(u4ColorTo[2]) / 255.0, float(u4ColorTo[3]) / 255.0)
    let color = 0.5 * (p4ColorFrom + p4ColorTo)
    push(global_fx__rendinstColors, color)
