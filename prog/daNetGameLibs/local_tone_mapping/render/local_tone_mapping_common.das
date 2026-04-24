options persistent_heap

require ecs
require DagorShaders
require daFrameGraph.fg_ecs
require strings

let MIPS_FOR_ONE_PASS = 4
let BASE_DIVIDER_POW = 1
let LOW_QUALITY_RESOLUTION_DIVIDER = 2

def get_resultion_divider(high_quality : bool)
  return BASE_DIVIDER_POW + (high_quality ? 0 : LOW_QUALITY_RESOLUTION_DIVIDER)

[event(unicast)]
struct RegisterLocalToneMappingNodes {}

[es(on_appear, tag=render, track=local_tone_mapping__shadows_multiplier, REQUIRE=local_tone_mapping__downsample_nodes_cnt)]
def local_tonemapping_set_shadow_mult_es(evt : Event; local_tone_mapping__shadows_multiplier : float)
  set_float(get_shader_variable_id("shadows_multiplier", false), pow(2.0, local_tone_mapping__shadows_multiplier))

[es(on_appear, tag=render, track=local_tone_mapping__highlights_multiplier, REQUIRE=local_tone_mapping__downsample_nodes_cnt)]
def local_tonemapping_set_highlight_mult_es(evt : Event; local_tone_mapping__highlights_multiplier : float)
  set_float(get_shader_variable_id("highlights_multiplier", false), pow(2.0, -local_tone_mapping__highlights_multiplier))

[es(on_appear, tag=render, track=local_tone_mapping__hq_enabled, track=local_tone_mapping__hq_sigma, track=local_tone_mapping__lq_sigma)]
def local_tonemapping_set_sigma_quality_es(evt : Event;
                                           var local_tone_mapping__sigma : float&;
                                           local_tone_mapping__hq_enabled : bool;
                                           local_tone_mapping__hq_sigma : float;
                                           local_tone_mapping__lq_sigma : float)
  local_tone_mapping__sigma = local_tone_mapping__hq_enabled ? local_tone_mapping__hq_sigma : local_tone_mapping__lq_sigma

[es(on_appear, tag=render, track=local_tone_mapping__hq_enabled, track=local_tone_mapping__hq_shadows_multiplier, track=local_tone_mapping__lq_shadows_multiplier)]
def local_tonemapping_set_shadows_multiplier_es(evt : Event;
                                                var local_tone_mapping__shadows_multiplier : float&;
                                                local_tone_mapping__hq_enabled : bool;
                                                local_tone_mapping__hq_shadows_multiplier : float;
                                                local_tone_mapping__lq_shadows_multiplier : float)
  local_tone_mapping__shadows_multiplier = local_tone_mapping__hq_enabled ? local_tone_mapping__hq_shadows_multiplier : local_tone_mapping__lq_shadows_multiplier

[es(on_appear, tag=render, track=local_tone_mapping__hq_enabled, track=local_tone_mapping__hq_highlights_multiplier, track=local_tone_mapping__lq_highlights_multiplier)]
def local_tonemapping_set_highlights_multiplier_es(evt : Event;
                                                   var local_tone_mapping__highlights_multiplier : float&;
                                                   local_tone_mapping__hq_enabled : bool;
                                                   local_tone_mapping__hq_highlights_multiplier : float;
                                                   local_tone_mapping__lq_highlights_multiplier : float)
  local_tone_mapping__highlights_multiplier = local_tone_mapping__hq_enabled ? local_tone_mapping__hq_highlights_multiplier : local_tone_mapping__lq_highlights_multiplier

[es(on_appear, tag=render, track=local_tone_mapping__sigma, REQUIRE=local_tone_mapping__downsample_nodes_cnt)]
def local_tonemapping_set_sigma_es(evt : Event; local_tone_mapping__sigma : float)
  set_float(get_shader_variable_id("sigma_sq", false), 1.0 / (local_tone_mapping__sigma * local_tone_mapping__sigma))

[es(on_appear, tag=render, track=local_tone_mapping__middle_luminance, REQUIRE=local_tone_mapping__downsample_nodes_cnt)]
def local_tonemapping_set_middle_luminance_es(evt : Event; local_tone_mapping__middle_luminance : float)
  set_float(get_shader_variable_id("middle_luminance", false), local_tone_mapping__middle_luminance)

[es(on_appear, tag=render, track=local_tone_mapping__boost_local_contrast, REQUIRE=local_tone_mapping__downsample_nodes_cnt)]
def local_tonemapping_set_boost_local_contrast_es(evt : Event; local_tone_mapping__boost_local_contrast : bool)
  set_int(get_shader_variable_id("boostLocalContrast", false), local_tone_mapping__boost_local_contrast ? 1 : 0)

def register_downsample_nodes_common(var local_tone_mapping__downsample_nodes : NodeHandleVector&;
                                     local_tone_mapping__hq_enabled : bool;
                                     local_tone_mapping__downsample_nodes_cnt : int;
                                     local_tone_mapping__mips_cnt : int;
                                     local_tone_mapping__display_mip : int;
                                     local_tone_mapping_input_color : string;
                                     local_tone_mapping_input_depth : string)
  let source_mipVarId = get_shader_variable_id("source_mip", false)
  let mips_cntVarId = get_shader_variable_id("mips_cnt", false)
  let display_mipVarId = get_shader_variable_id("display_mip", false)
  let apply_per_tile_blurVarId = get_shader_variable_id("apply_per_tile_blur", false)
  let skip_each_odd_texelVarId = get_shader_variable_id("skip_each_odd_texel", false)
  let local_tone_mapping_downsample_output_uav_noVarId = get_shader_variable_id("local_tone_mapping_downsample_output_uav_no", false)
  let local_tone_mapping_downsample_weights_tex_uav_noVarId = get_shader_variable_id("local_tone_mapping_downsample_weights_tex_uav_no", false)

  local_tone_mapping__downsample_nodes |> resize(local_tone_mapping__downsample_nodes_cnt)
  for index, handle in count(), local_tone_mapping__downsample_nodes
    handle |> resetNode()

    let nodeName = "downsample_{index:d}"
    handle <- (root() / "local_tone_mapping") |> registerNode(nodeName) <| @(var registry : Registry)
      let lastNode = index + 1 == local_tone_mapping__downsample_nodes_cnt
      let mipsCnt = uint(lastNode ? local_tone_mapping__mips_cnt - MIPS_FOR_ONE_PASS * index : MIPS_FOR_ONE_PASS)

      let source_tex_name = index == 0 ? (local_tone_mapping__hq_enabled ? local_tone_mapping_input_color : "downsampled_color") : "exposures_{index - 1:d}"
      registry |> read(source_tex_name) |> texture |> atStage(Stage.COMPUTE) |> bindToShaderVar("src_tex")
      let defaultSampler = d3d_request_sampler() <| $(var smpInfo : SamplerInfo#)
        smpInfo.address_mode_u = AddressMode.Clamp
        smpInfo.address_mode_v = AddressMode.Clamp
        smpInfo.address_mode_w = AddressMode.Clamp
      if index == 0
        registry |> create("local_tone_mapping_src_sampler") |> blob(defaultSampler) |> bindToShaderVar("src_tex_samplerstate")
      else
        registry |> read("local_tone_mapping_src_sampler") |> blob(type<SamplerHandle>) |> bindToShaderVar("src_tex_samplerstate")

      // one of the below is being bound to "dof_blend_depth_tex" but exactly which is decided dynamically
      registry |> read("depth_with_transparency") |> texture |> atStage(Stage.COMPUTE) |> useAs(Usage.SHADER_RESOURCE) |> optional()
      registry |> read(local_tone_mapping_input_depth) |> texture |> atStage(Stage.COMPUTE) |> useAs(Usage.SHADER_RESOURCE) |> optional()
      registry |> read("lens_dof_blend_depth_tex") |> texture |> atStage(Stage.COMPUTE) |> useAs(Usage.SHADER_RESOURCE) |> optional()

      if index != 0
        registry |> read("weights_{index - 1:d}") |> texture |> atStage(Stage.COMPUTE) |> bindToShaderVar("src_weight_tex")
      else
        (registry |> root() / "bloom") |> read("upsampled_bloom_mip_0") |> texture |> atStage(Stage.COMPUTE) |> bindToShaderVar("bloom_tex") |> optional()

      var displayMip = 0
      if local_tone_mapping__display_mip >= (index + 1) * MIPS_FOR_ONE_PASS
        displayMip = MIPS_FOR_ONE_PASS - 1
      elif local_tone_mapping__display_mip < (index + 1) * MIPS_FOR_ONE_PASS && local_tone_mapping__display_mip >= index * MIPS_FOR_ONE_PASS
        displayMip = local_tone_mapping__display_mip - index * MIPS_FOR_ONE_PASS

      var highQuality = local_tone_mapping__hq_enabled
      var resolutionDivider = get_resultion_divider(local_tone_mapping__hq_enabled)
      let scale = 1.0 / float(1 << (index * MIPS_FOR_ONE_PASS + resolutionDivider))
      let nextLayerInfo = Texture2dCreateInfo(
        creationFlags = TEXCF_UNORDERED | TEXFMT_A16B16G16R16F,
        resolution = TextureResolution2(autoRes = registry |> getResolution`2("post_fx", scale)),
        mipLevels = mipsCnt
      )
      var exposureHndl = registry |> create("exposures_{index:d}") |> texture(nextLayerInfo) |> atStage(Stage.COMPUTE) |> useAs(Usage.SHADER_RESOURCE) |> handle()
      var weightHndl = registry |> create("weights_{index:d}") |> texture(nextLayerInfo) |> atStage(Stage.COMPUTE) |> useAs(Usage.SHADER_RESOURCE) |> handle()
      var postfxResolution = registry |> getResolution`2("post_fx")
      let local_tone_mapping_downsample_output_uav_no = uint(get_int(local_tone_mapping_downsample_output_uav_noVarId))
      let local_tone_mapping_downsample_weights_tex_uav_no = uint(get_int(local_tone_mapping_downsample_weights_tex_uav_noVarId))
      return <- @ <|  capture(<- postfxResolution, <- exposureHndl, <- weightHndl, <- displayMip, <- resolutionDivider, <- highQuality)
        set_int(source_mipVarId, index == 0 ? 0 : MIPS_FOR_ONE_PASS - 1)
        set_int(mips_cntVarId, int(mipsCnt))
        set_int(display_mipVarId, int(displayMip))
        set_int(apply_per_tile_blurVarId, lastNode ? 1 : 0)
        set_int(skip_each_odd_texelVarId, highQuality ? 0 : 1)
        for i in 0u..mipsCnt
          d3d_set_rwtex(STAGE_CS, local_tone_mapping_downsample_output_uav_no + i, exposureHndl |> view |> get_tex2d(), 0u, i, false)
          d3d_set_rwtex(STAGE_CS, local_tone_mapping_downsample_weights_tex_uav_no + i, weightHndl |> view |> get_tex2d(), 0u, i, false)
        ecs::query() <| $ [es] (local_tone_mapping_downsample : ComputeShader)
          local_tone_mapping_downsample |> dispatchThreads(
            max((postfxResolution |> get()).x >> (resolutionDivider + index * MIPS_FOR_ONE_PASS), 1),
            max((postfxResolution |> get()).y >> (resolutionDivider + index * MIPS_FOR_ONE_PASS), 1),
            1)
        for i in 0u..mipsCnt
          d3d_set_rwtex(STAGE_CS, local_tone_mapping_downsample_output_uav_no + i, null, 0u, 0u, false)
          d3d_set_rwtex(STAGE_CS, local_tone_mapping_downsample_weights_tex_uav_no + i, null, 0u, 0u, false)

[fg_ecs_node(on_event=RegisterLocalToneMappingNodes)]
def register_upsample_nodes(var local_tone_mapping__upsample_nodes : NodeHandleVector&;
                            local_tone_mapping__hq_enabled : bool;
                            local_tone_mapping__upsample_nodes_cnt : int;
                            local_tone_mapping__mips_cnt : int;
                            local_tone_mapping__display_mip : int;
                            local_tone_mapping__downsample_nodes_cnt : int)
  let upsample_modeVarId = get_shader_variable_id("upsample_mode", false)
  let mips_cntVarId = get_shader_variable_id("mips_cnt", false)
  let base_mipVarId = get_shader_variable_id("base_mip", false)
  let local_tone_mapping_upsample_output_uav_noVarId = get_shader_variable_id("local_tone_mapping_upsample_output_uav_no", false)

  local_tone_mapping__upsample_nodes |> resize(local_tone_mapping__upsample_nodes_cnt)
  for index, handle in count(), local_tone_mapping__upsample_nodes
    handle |> resetNode()

    let nodeName = "upsample_{index:d}"
    handle <- (root() / "local_tone_mapping") |> registerNode(nodeName) <| @(var registry : Registry)
      let lastNode = index == 0
      let firstNode = index + 1 == local_tone_mapping__upsample_nodes_cnt
      let mipsCnt = lastNode ? local_tone_mapping__mips_cnt - (local_tone_mapping__upsample_nodes_cnt - index) * MIPS_FOR_ONE_PASS : (
        firstNode ? MIPS_FOR_ONE_PASS - local_tone_mapping__display_mip : MIPS_FOR_ONE_PASS)

      var upsampleMode = 0
      if firstNode
        if local_tone_mapping__upsample_nodes_cnt == 1
          upsampleMode = 3
        else
          upsampleMode = 1
      else
        if lastNode
          upsampleMode = 0
        else
          upsampleMode = 2
      let inversedIndex = local_tone_mapping__upsample_nodes_cnt - index - 1
      let srcIndex = local_tone_mapping__downsample_nodes_cnt - index - 1

      registry |> read("exposures_{srcIndex:d}") |> texture |> atStage(Stage.COMPUTE) |> bindToShaderVar("exposures_tex")
      registry |> read("weights_{srcIndex:d}") |> texture |> atStage(Stage.COMPUTE) |> bindToShaderVar("weights_tex")
      if !lastNode
        registry |> read("exposure_multiplier_{inversedIndex + 1:d}") |> texture |> atStage(Stage.COMPUTE) |> bindToShaderVar("accumulated_tex")
        registry |> read("exposures_{srcIndex + 1:d}") |> texture |> atStage(Stage.COMPUTE) |> bindToShaderVar("coarse_exposures_tex")

      var displayMip = firstNode ? local_tone_mapping__display_mip % MIPS_FOR_ONE_PASS : 0

      var resolutionDivider = get_resultion_divider(local_tone_mapping__hq_enabled)
      let scale = 1.0 / float(1 << (srcIndex * MIPS_FOR_ONE_PASS + displayMip + resolutionDivider))
      let texFormat = TEXFMT_R16F
      let nextLayerInfo = Texture2dCreateInfo(
        creationFlags = TEXCF_UNORDERED | texFormat,
        resolution = TextureResolution2(autoRes = registry |> getResolution`2("post_fx", scale)),
        mipLevels = 1u
      )

      var exposureHndl = registry |> create(firstNode ? "exposure_multiplier_tex" : "exposure_multiplier_{inversedIndex:d}") |> texture(nextLayerInfo) |> atStage(Stage.COMPUTE) |> useAs(Usage.SHADER_RESOURCE) |> handle()
      var postfxResolution = registry |> getResolution`2("post_fx")
      let local_tone_mapping_upsample_output_uav_no = uint(get_int(local_tone_mapping_upsample_output_uav_noVarId))

      return <- @ <|  capture(<- postfxResolution, <- exposureHndl, <- weightHndls, <- displayMip, <- upsampleMode, <- resolutionDivider)
        set_int(upsample_modeVarId, upsampleMode)
        set_int(mips_cntVarId, mipsCnt)
        set_int(base_mipVarId, displayMip)
        d3d_set_rwtex(STAGE_CS, local_tone_mapping_upsample_output_uav_no, exposureHndl |> view |> get_tex2d(), 0u, 0u, false)
        let divider = 1 << (srcIndex * MIPS_FOR_ONE_PASS + displayMip + resolutionDivider)
        ecs::query() <| $ [es] (local_tone_mapping_upsample : ComputeShader)
          local_tone_mapping_upsample |> dispatchThreads(
            ((postfxResolution |> get()).x + divider - 1) / divider,
            ((postfxResolution |> get()).y + divider - 1) / divider,
            1)
        d3d_set_rwtex(STAGE_CS, local_tone_mapping_upsample_output_uav_no, null, 0u, 0u, false)

def register_filter_node_common(var local_tone_mapping__filter_node : NodeHandle&;
                                local_tone_mapping__hq_enabled : bool;
                                local_tone_mapping__hq_final_tex_scale : float;
                                local_tone_mapping__lq_final_tex_scale : float;
                                local_tone_mapping_input : string)
  local_tone_mapping__filter_node <- (root() / "local_tone_mapping") |> registerNode("filter") <| @(var registry : Registry)
    registry |> read("exposure_multiplier_tex") |> texture |> atStage(Stage.POST_RASTER) |> bindToShaderVar("exposure_tex")
    registry |> read((local_tone_mapping__hq_enabled ? local_tone_mapping_input : "downsampled_color")) |> texture |> atStage(Stage.POST_RASTER) |> bindToShaderVar("frame_tex")
    let defaultSampler = d3d_request_sampler() <| $(var smpInfo : SamplerInfo#)
      smpInfo.address_mode_u = AddressMode.Clamp
      smpInfo.address_mode_v = AddressMode.Clamp
      smpInfo.address_mode_w = AddressMode.Clamp
    registry |> create("clamp_sampler") |> blob(defaultSampler) |> bindToShaderVar("frame_tex_samplerstate")

    let scale = local_tone_mapping__hq_enabled ? local_tone_mapping__hq_final_tex_scale : local_tone_mapping__lq_final_tex_scale
    let texFormat = TEXFMT_R16F | TEXCF_ESRAM_ONLY
    let nextLayerInfo = Texture2dCreateInfo(
      creationFlags = TEXCF_RTARGET | texFormat,
      resolution = TextureResolution2(autoRes = registry |> getResolution`2("post_fx", scale)),
      mipLevels = 1u
    )
    var exposureTexReq = registry |> create("exposure_multiplier_filtered_tex") |> texture(nextLayerInfo)
    registry |> requestRenderPass |> color(fixed_array(exposureTexReq))
    return <- @ <|
      ecs::query() <| $ [es] (local_tone_mapping_filter : PostFxRenderer)
        local_tone_mapping_filter |> render()

def init_local_tone_mapping(eid : EntityId;
                            mainResolution : int2;
                            local_tone_mapping__hq_enabled : bool;
                            local_tone_mapping__hq_display_mip : int;
                            local_tone_mapping__hq_mips_cnt : int;
                            local_tone_mapping__lq_display_mip : int;
                            local_tone_mapping__lq_mips_cnt : int;
                            var local_tone_mapping__mips_cnt : int&;
                            var local_tone_mapping__display_mip : int&;
                            var local_tone_mapping__downsample_nodes_cnt : int&;
                            var local_tone_mapping__upsample_nodes_cnt : int&)
  let resolutionDivider = get_resultion_divider(local_tone_mapping__hq_enabled)
  let maxMipLevel = int(min(log2(float(mainResolution.x)), log2(float(mainResolution.y)))) - resolutionDivider

  local_tone_mapping__mips_cnt = local_tone_mapping__hq_enabled ? local_tone_mapping__hq_mips_cnt : local_tone_mapping__lq_mips_cnt
  local_tone_mapping__display_mip = local_tone_mapping__hq_enabled ? local_tone_mapping__hq_display_mip : local_tone_mapping__lq_display_mip

  local_tone_mapping__mips_cnt = clamp(local_tone_mapping__mips_cnt, resolutionDivider - 1, maxMipLevel)
  local_tone_mapping__display_mip = clamp(local_tone_mapping__display_mip, resolutionDivider - 1, local_tone_mapping__mips_cnt)

  local_tone_mapping__downsample_nodes_cnt = (local_tone_mapping__mips_cnt + MIPS_FOR_ONE_PASS - 1) / MIPS_FOR_ONE_PASS
  let upsample_mips_cnt = local_tone_mapping__downsample_nodes_cnt * MIPS_FOR_ONE_PASS - local_tone_mapping__display_mip
  local_tone_mapping__upsample_nodes_cnt = (upsample_mips_cnt + MIPS_FOR_ONE_PASS - 1) / MIPS_FOR_ONE_PASS

  sendEventImmediate(eid, RegisterLocalToneMappingNodes())

[es(on_appear, tag=render)]
def local_tone_mapping_settings_es(evt : Event; local_tone_mapping_setting__hq_shadows_multiplier : float; local_tone_mapping_setting__hq_highlights_multiplier : float;
                                   local_tone_mapping_setting__lq_shadows_multiplier : float; local_tone_mapping_setting__lq_highlights_multiplier : float)
  var count = 0
  query() <| $ [es] (var local_tone_mapping__hq_shadows_multiplier : float&;
                     var local_tone_mapping__hq_highlights_multiplier : float&;
                     var local_tone_mapping__lq_shadows_multiplier : float&;
                     var local_tone_mapping__lq_highlights_multiplier : float&)
    local_tone_mapping__hq_shadows_multiplier = local_tone_mapping_setting__hq_shadows_multiplier
    local_tone_mapping__hq_highlights_multiplier = local_tone_mapping_setting__hq_highlights_multiplier
    local_tone_mapping__lq_shadows_multiplier = local_tone_mapping_setting__lq_shadows_multiplier
    local_tone_mapping__lq_highlights_multiplier = local_tone_mapping_setting__lq_highlights_multiplier
    count++
  if (count != 1)
    logerr("local_tone_mapping_settings expected 1 entity to change settings for but {count} were found!")
