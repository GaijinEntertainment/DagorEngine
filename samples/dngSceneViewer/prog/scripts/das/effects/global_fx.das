require ecs
require DagorDataBlock
require fx
require danetlibs.renderer.includes.render_events
require math/color

[es]
def fx_target_reset(evt : SetFxQuality)
  find_query() <| $ [es] (var global_fx__target : int&)
    global_fx__target = int(evt.fxQualityConfig)
    return true

[es(on_appear)]
def fx_target_init(evt : Event;
                   var global_fx__target : int&)
  global_fx__target = int(getFxQualityMask())

def calc_fx_paint_color_from_level_ri_colors(rendinst_pool : int)
  var res = float4(1., 1., 1., 1.)

  if rendinst_pool < 0
    return res

  find_query() <| $ [es] (global_fx__rendinstColors : Point4List;
                          global_fx__riColorSaturationMult : float;
                          global_fx__riColorSaturationInc : float;
                          global_fx__riColorValueMult : float;
                          global_fx__riColorValueInc : float)
    if rendinst_pool < length(global_fx__rendinstColors)
      res = global_fx__rendinstColors[rendinst_pool]
      let hsv = rgb2hsv(res.xyz)
      res = float4(hsv2rgb(float3(hsv.x,
                                  global_fx__riColorSaturationInc + global_fx__riColorSaturationMult * hsv.y,
                                  global_fx__riColorValueInc + global_fx__riColorValueMult * hsv.z)), 1.)
    return true
  return res

[es(tag=render, on_appear, before=paint_hit_fx_es, REQUIRE=paintFromLevelRiColors)]
def color_effect_from_ri_pool_es(evt : Event; effect__riPoolUsedForColoring : int; var paint_color : float4&)
  paint_color = calc_fx_paint_color_from_level_ri_colors(effect__riPoolUsedForColoring)
