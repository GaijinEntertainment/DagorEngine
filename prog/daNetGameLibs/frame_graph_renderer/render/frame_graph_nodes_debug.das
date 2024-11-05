options no_aot
options persistent_heap

require DagorShaders
require DagorMath
require WorldRenderer
require ecs
require DagorConsole
require daBfg.bfg_ecs

let debug_visualization_node_name = "debug_visualization_node"
let debug_visualization_node_entity = "debug_visualization_node"

let gbuffer_diffuse_override_node_name = "gbuffer_diffuse_override"
let gbuffer_diffuse_override_node_entity = "gbuffer_diffuse_override_debug_node"

let gbuffer_specular_override_node_name = "gbuffer_specular_override"
let gbuffer_specular_override_node_entity = "gbuffer_specular_override_debug_node"

def is_forward_rendering()
  return find_query() <| $ [es] (render_settings__forwardRendering : bool)
    return render_settings__forwardRendering

[bfg_ecs_node(on_appear)]
def register_debug_visualization_node(var debug_visualization_node : NodeHandle&)
  debug_visualization_node <- root() |> registerNode(debug_visualization_node_name) <| @(var registry : Registry)
    if is_forward_rendering()
      registry |> requestRenderPass |> color([[auto[] "target_after_under_water_fog"]]) |> depthRo("depth_for_transparent_effects")
    else
      registry |> orderMeBefore("transparent_scene_late_node")
      registry |> requestRenderPass |> color([[auto[] "target_for_transparency"]]) |> depthRw("depth_for_transparency")

    registry |> requestState() |> setFrameBlock("global_frame")
    return <- @ <|
      worldRenderer_renderDebug()


def register_gbuffer_override_node(var handle : NodeHandle& | #; name : string; entity : string; target : string; colorMask : WriteMask)
  handle <- root() |> registerNode(name) <| @(var registry : Registry)
    registry |> requestState |> enableOverride([[OverrideRenderState() colorWr = colorMask]])
    registry |> requestRenderPass |> color([[auto[] target]])
    let eid = getSingletonEntity(entity)
    return <- @ <| [[=eid]]
      query(eid) <| $ [es] (debug_fill_gbuffer : PostFxRenderer; debug_fill_color : float4)
        set_color4(get_shader_variable_id("gbuffer_fix_color", true), debug_fill_color)
        set_int(get_shader_variable_id("debug_fix_gbuffer", true), 0)
        debug_fill_gbuffer |> render


[bfg_ecs_node(on_appear)]
def register_gbuffer_diffuse_override_node(var gbuffer_diffuse_override : NodeHandle&)
  gbuffer_diffuse_override |> register_gbuffer_override_node(gbuffer_diffuse_override_node_name, gbuffer_diffuse_override_node_entity, "gbuf_0", WriteMask RGB)


[bfg_ecs_node(on_appear)]
def register_gbuffer_specular_override_node(var gbuffer_specular_override : NodeHandle&)
  gbuffer_specular_override |> register_gbuffer_override_node(gbuffer_specular_override_node_name, gbuffer_specular_override_node_entity, "gbuf_2", WriteMask GREEN)


[es(on_event=BeforeLoadLevel)]
def init_debug_nodes(evt : Event)
  createEntity(debug_visualization_node_entity)


[console_cmd(name="render.fixedDiffuse")]
def const_diffuse(enable : bool; shade_of_gray : float = 0.5f)
  var eid = getSingletonEntity(gbuffer_diffuse_override_node_entity)
  if !enable && eid != INVALID_ENTITY_ID
    destroyEntity(eid)
  if enable
    if !eid
      eid = createEntity(gbuffer_diffuse_override_node_entity) <| $(init)
        init |> set("debug_fill_color", float4(shade_of_gray))
    else
      query(eid) <| $ [es] (var debug_fill_color : float4&; var gbuffer_diffuse_override : NodeHandle&)
        debug_fill_color = float4(shade_of_gray)
        gbuffer_diffuse_override |> register_gbuffer_diffuse_override_node


[console_cmd(name="render.noSpecular")]
def const_specular(enable : bool)
  var eid = getSingletonEntity(gbuffer_specular_override_node_entity)
  if !enable && eid != INVALID_ENTITY_ID
    destroyEntity(eid)
  if enable
    if !eid
      eid = createEntity(gbuffer_specular_override_node_entity) <| $(init)
        init |> set("debug_fill_color", float4(0.0))
    else
      query(eid) <| $ [es] (var debug_fill_color : float4&; var gbuffer_specular_override : NodeHandle&)
        debug_fill_color = float4(0.0)
        gbuffer_specular_override |> register_gbuffer_specular_override_node
