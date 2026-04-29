options persistent_heap

require ecs
require daFrameGraph.fg_ecs
require WorldRenderer
require gameLibs.render_events.render_events
require danetlibs.local_tone_mapping.render.local_tone_mapping_common

[fg_ecs_node(on_event=RegisterLocalToneMappingNodes)]
def register_filter_node(var local_tone_mapping__filter_node : NodeHandle&;
                         local_tone_mapping__hq_enabled : bool;
                         local_tone_mapping__hq_final_tex_scale : float;
                         local_tone_mapping__lq_final_tex_scale : float)
  register_filter_node_common(
    local_tone_mapping__filter_node,
    local_tone_mapping__hq_enabled,
    local_tone_mapping__hq_final_tex_scale,
    local_tone_mapping__lq_final_tex_scale,
    "frame_for_postfx"
  )

[fg_ecs_node(on_event=RegisterLocalToneMappingNodes)]
def register_downsample_nodes(var local_tone_mapping__downsample_nodes : NodeHandleVector&;
                              local_tone_mapping__hq_enabled : bool;
                              local_tone_mapping__downsample_nodes_cnt : int;
                              local_tone_mapping__mips_cnt : int;
                              local_tone_mapping__display_mip : int)
  register_downsample_nodes_common(
    local_tone_mapping__downsample_nodes,
    local_tone_mapping__hq_enabled,
    local_tone_mapping__downsample_nodes_cnt,
    local_tone_mapping__mips_cnt,
    local_tone_mapping__display_mip,
    "frame_for_postfx",
    "depth_for_postfx"
  )

[es(on_appear, tag=render, REQUIRE=local_tone_mapping__hq_final_tex_scale, REQUIRE=local_tone_mapping__lq_final_tex_scale,
    track=local_tone_mapping__hq_enabled, track=local_tone_mapping__hq_display_mip,
    track=local_tone_mapping__hq_mips_cnt, track=local_tone_mapping__hq_final_tex_scale,
    track=local_tone_mapping__lq_display_mip, track=local_tone_mapping__lq_mips_cnt,
    track=local_tone_mapping__lq_final_tex_scale)]
def local_tone_mapping_update_node_chains_es(evt : Event;
                                             eid : EntityId;
                                             local_tone_mapping__hq_enabled : bool;
                                             local_tone_mapping__hq_display_mip : int;
                                             local_tone_mapping__hq_mips_cnt : int;
                                             local_tone_mapping__lq_display_mip : int;
                                             local_tone_mapping__lq_mips_cnt : int;
                                             var local_tone_mapping__mips_cnt : int&;
                                             var local_tone_mapping__display_mip : int&;
                                             var local_tone_mapping__downsample_nodes_cnt : int&;
                                             var local_tone_mapping__upsample_nodes_cnt : int&)
  if !does_world_renderer_exist()
    return
  var width, height : int
  get_rendering_resolution(width, height)

  init_local_tone_mapping(eid,
    int2(width, height),
    local_tone_mapping__hq_enabled,
    local_tone_mapping__hq_display_mip,
    local_tone_mapping__hq_mips_cnt,
    local_tone_mapping__lq_display_mip,
    local_tone_mapping__lq_mips_cnt,
    local_tone_mapping__mips_cnt,
    local_tone_mapping__display_mip,
    local_tone_mapping__downsample_nodes_cnt,
    local_tone_mapping__upsample_nodes_cnt)

[es(tag=render)]
def local_tone_mapping_update_node_chains_change_res_es(evt : SetResolutionEvent;
                                                        eid : EntityId;
                                                        local_tone_mapping__hq_enabled : bool;
                                                        local_tone_mapping__hq_display_mip : int;
                                                        local_tone_mapping__hq_mips_cnt : int;
                                                        local_tone_mapping__lq_display_mip : int;
                                                        local_tone_mapping__lq_mips_cnt : int;
                                                        var local_tone_mapping__mips_cnt : int&;
                                                        var local_tone_mapping__display_mip : int&;
                                                        var local_tone_mapping__downsample_nodes_cnt : int&;
                                                        var local_tone_mapping__upsample_nodes_cnt : int&)
  if evt.type_ == 1 // SetResolutionEvent::Type::DYNAMIC_RESOLUTION from cpp
    return
  init_local_tone_mapping(eid,
    evt.renderingResolution,
    local_tone_mapping__hq_enabled,
    local_tone_mapping__hq_display_mip,
    local_tone_mapping__hq_mips_cnt,
    local_tone_mapping__lq_display_mip,
    local_tone_mapping__lq_mips_cnt,
    local_tone_mapping__mips_cnt,
    local_tone_mapping__display_mip,
    local_tone_mapping__downsample_nodes_cnt,
    local_tone_mapping__upsample_nodes_cnt)

[cpp_event(broadcast)]
struct OnRenderSettingsReady {}

[es(on_event=OnRenderSettingsReady, track=render_settings__giAlgorithm)]
def local_tonemapping_quality_es(evt : Event; render_settings__giAlgorithm : string)
  let useHQ = render_settings__giAlgorithm != "low"
  query() <| $ [es] (var local_tone_mapping__hq_enabled : bool&)
    local_tone_mapping__hq_enabled = useHQ

