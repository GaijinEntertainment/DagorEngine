require ecs
require AnimV20
require DagorMaterials
require DagorShaders
require DagorMath
require DagorSystem
require danetlibs.renderer.includes.render_events

let paintColorVarId = get_shader_variable_id("paint_color", true)
def set_paint_color(paintColor : float4; var animchar_render : AnimcharRendComponent&)
  if paintColorVarId < 0
    error("if")
    return

  let gamma = 2.2f
  let color = Color4(float4(pow(paintColor.x, gamma), pow(paintColor.y, gamma), pow(paintColor.z, gamma), paintColor.w))

  recreate_material(animchar_render) <| $(mat)
    mat |> set_color4_param(paintColorVarId, color)

[es(on_appear, tag=render, track=paintColor)]
def set_paint_color_es(evt : Event;
                       paintColor : float4;
                       var animchar_render : AnimcharRendComponent&)
  set_paint_color(paintColor, animchar_render)

struct ShaderColorInfo
  color : Color4
  varId : int

def ShaderColorInfo(color : Color4; varId : int)
  return ShaderColorInfo(color = color, varId = varId)

[es(on_appear, tag=render, track=shader_color__colors)]
def shader_color_es(evt : Event;
                    eid : EntityId;
                    shader_color__shaderVarNames : StringList;
                    shader_color__colors : Point4List;
                    var animchar_render : AnimcharRendComponent&)
  if shader_color__shaderVarNames |> length() != shader_color__colors |> length()
    logerr("shader_color.shaderVarNames size should match shader_color.colors size in '{getEntityTemplateName(eid)}'")
    return
  var colors : array<ShaderColorInfo>
  for i in range(0, length(shader_color__shaderVarNames))
    let varId = get_shader_variable_id(string(shader_color__shaderVarNames[i]), true)
    if varId < 0
      continue
    colors |> emplace(ShaderColorInfo(Color4(shader_color__colors[i]), varId))

  if colors |> empty()
    return
  recreate_material(animchar_render) <| $(mat)
    for color in colors
      mat |> set_color4_param(color.varId, color.color)

[es(tag=render)]
def shader_color_emission_es(info : UpdateStageInfoBeforeRender;
                             shader_color__emissionStregth : float;
                             animchar__visible : bool;
                             var dynamic_material_channels_arr : Array)
  if !animchar__visible
    return

  var matParams = getRW_ecs_object(dynamic_material_channels_arr[0])
  if matParams != null
    let color = float4(1, 1, 1, shader_color__emissionStregth)
    set(*matParams, "dynmat_param__emissive_color", color)

