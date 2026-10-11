// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtdl/rtdl_api.h>
#include <render/rtdl/shaders/rtdl_consts.hlsli>
#include <3d/dag_resPtr.h>
#include <bvh/bvh.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_tex3d.h>
#include <math/dag_mathBase.h>
#include <math/dag_Point2.h>
#include <render/viewVecs.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaders.h>
#include <startup/dag_globalSettings.h>
#include <EASTL/optional.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/unique_ptr.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>

namespace rtdl
{
static ComputeShaderElement *sunTracePass = nullptr;
static ComputeShaderElement *sunFillPass = nullptr;
static ComputeShaderElement *sunTilesPass = nullptr;
static ComputeShaderElement *sunBlurPass = nullptr;
static ComputeShaderElement *localPass = nullptr;
static ComputeShaderElement *stabilizePass = nullptr;

static int rt_direct_lighting_resolutionVarId = -1;
static int rt_direct_lighting_resolutionIVarId = -1;
static int rt_direct_lighting_full_resolutionIVarId = -1;
static int rtsm_bindless_slotVarId = -1;
static int rtsm_is_translucentVarId = -1;
static int csm_bindless_slotVarId = -1;
static int csm_sampler_bindless_slotVarId = -1;
static int vsm_bindless_slotVarId = -1;
static int vsm_sampler_bindless_slotVarId = -1;
static int rt_shadow_resolutionVarId = -1;
static int rt_shadow_resolutionIVarId = -1;
static int rt_direct_lighting_paramsVarId = -1;
static int rt_direct_lighting_sun_paramsVarId = -1;
static int rt_direct_lighting_stabilize_paramsVarId = -1;
static int rt_direct_lighting_sun_qualityVarId = -1;
static int rt_direct_lighting_frame_paramsVarId = -1;
static int rt_direct_lighting_dynamic_lightsVarId = -1;
static int precomputed_dynamic_lightsVarId = -1;
static int render_direct_lights_use_tilingVarId = -1;
static int use_precomputed_dynamic_lightsVarId = -1;
static int rtsm_has_nukeVarId = -1;
static int world_view_posVarId = -1;
static int zn_zfarVarId = -1;

static const char *const bound_texture_vars[] = {"rt_direct_lighting_history", "rt_direct_lighting_history_prev",
  "rt_direct_lighting_radiance", "rt_direct_lighting_unshadowed", "rt_direct_lighting_shadow_samples", "rt_direct_lighting_normals",
  "rt_direct_lighting_tiles", "rt_direct_lighting_tiles_dilated", "rt_direct_lighting_tiles_dilated_prev",
  "rt_direct_lighting_change_tiles", "rt_direct_lighting_change_tiles_prev", "rt_direct_lighting_sun_spatial",
  "rt_direct_lighting_sun_hit", "rt_direct_lighting_motion_vectors", "rt_direct_lighting_sun_shadow", "precomputed_dynamic_lights",
  "rt_direct_lighting_output"};
static const char *const bound_buffer_vars[] = {
  "rt_direct_lighting_sun_samples", "rt_direct_lighting_shadows", "rt_direct_lighting_shadows_prev"};

enum
{
  sun_shadow_bindless_index = 0,
  csm_bindless_index,
  vsm_bindless_index,
  bindless_texture_count
};
static int bindless_range = -1;

struct FrameState
{
  int frame = -1;
  IPoint2 resolution = IPoint2::ZERO;
  IPoint2 prevResolution = IPoint2::ZERO;
  bool historyValid = false;
};
static FrameState frame_state;

static int shadow_rays_per_pixel = 2;
static int max_sun_history = 8;
static float sun_blur_scale = 0.5f;
static float sun_blur_min_radius = 1;
static float sun_blur_max_radius = 24;
static float sun_clamp_sigma = 3;
static int max_local_shadow_history = 16;
static float local_clamp_sigma = 3;
static float local_shadow_radius = 4;
static int shadow_retest_period = 8;
static float slot_hysteresis = 1.5f;
static bool shadow_rest_lights = true;
static bool change_tiles_retest = true;

void initialize()
{
  if (!bvh::is_available())
    return;

  rt_direct_lighting_resolutionVarId = get_shader_variable_id("rt_direct_lighting_resolution");
  rt_direct_lighting_resolutionIVarId = get_shader_variable_id("rt_direct_lighting_resolutionI");
  rt_direct_lighting_full_resolutionIVarId = get_shader_variable_id("rt_direct_lighting_full_resolutionI");
  rtsm_bindless_slotVarId = get_shader_variable_id("rtsm_bindless_slot");
  rtsm_is_translucentVarId = get_shader_variable_id("rtsm_is_translucent", true);
  csm_bindless_slotVarId = get_shader_variable_id("csm_bindless_slot", true);
  csm_sampler_bindless_slotVarId = get_shader_variable_id("csm_sampler_bindless_slot", true);
  vsm_bindless_slotVarId = get_shader_variable_id("vsm_bindless_slot", true);
  vsm_sampler_bindless_slotVarId = get_shader_variable_id("vsm_sampler_bindless_slot", true);
  rt_shadow_resolutionVarId = get_shader_variable_id("rt_shadow_resolution");
  rt_shadow_resolutionIVarId = get_shader_variable_id("rt_shadow_resolutionI");
  rt_direct_lighting_paramsVarId = get_shader_variable_id("rt_direct_lighting_params");
  rt_direct_lighting_sun_paramsVarId = get_shader_variable_id("rt_direct_lighting_sun_params");
  rt_direct_lighting_stabilize_paramsVarId = get_shader_variable_id("rt_direct_lighting_stabilize_params");
  rt_direct_lighting_sun_qualityVarId = get_shader_variable_id("rt_direct_lighting_sun_quality");
  rt_direct_lighting_frame_paramsVarId = get_shader_variable_id("rt_direct_lighting_frame_params");
  rt_direct_lighting_dynamic_lightsVarId = get_shader_variable_id("rt_direct_lighting_dynamic_lights");
  precomputed_dynamic_lightsVarId = get_shader_variable_id("precomputed_dynamic_lights");
  render_direct_lights_use_tilingVarId = get_shader_variable_id("render_direct_lights_use_tiling", true);
  use_precomputed_dynamic_lightsVarId = get_shader_variable_id("use_precomputed_dynamic_lights", true);
  rtsm_has_nukeVarId = get_shader_variable_id("rtsm_has_nuke");
  world_view_posVarId = get_shader_variable_id("world_view_pos");
  zn_zfarVarId = get_shader_variable_id("zn_zfar");

  if (!sunTracePass)
    sunTracePass = new_compute_shader("rt_direct_lighting_sun_trace");
  if (!sunFillPass)
    sunFillPass = new_compute_shader("rt_direct_lighting_sun_fill");
  if (!sunTilesPass)
    sunTilesPass = new_compute_shader("rt_direct_lighting_sun_tiles");
  if (!sunBlurPass)
    sunBlurPass = new_compute_shader("rt_direct_lighting_sun_blur");
  if (!localPass)
    localPass = new_compute_shader("rt_direct_lighting_local");
  if (!stabilizePass)
    stabilizePass = new_compute_shader("rt_direct_lighting_stabilize");

  if (bindless_range < 0)
    bindless_range = d3d::allocate_bindless_resource_range(D3DResourceType::TEX, bindless_texture_count);

  frame_state = FrameState();
}

template <typename T>
static void safe_delete(T *&ptr)
{
  if (ptr)
  {
    delete ptr;
    ptr = nullptr;
  }
}

static void clean_shvars()
{
  for (const char *name : bound_texture_vars)
    ShaderGlobal::set_texture(get_shader_variable_id(name, true), nullptr);
  for (const char *name : bound_buffer_vars)
    ShaderGlobal::set_buffer(get_shader_variable_id(name, true), BAD_D3DRESID);
}

#if DAGOR_DBGLEVEL > 0
static void close_debug_view();
#else
static void close_debug_view() {}
#endif

void teardown()
{
  close_debug_view();
  safe_delete(sunTracePass);
  safe_delete(sunFillPass);
  safe_delete(sunTilesPass);
  safe_delete(sunBlurPass);
  safe_delete(localPass);
  safe_delete(stabilizePass);
  if (bindless_range >= 0)
  {
    d3d::free_bindless_resource_range(D3DResourceType::TEX, bindless_range, bindless_texture_count);
    bindless_range = -1;
  }
  frame_state = FrameState();
  ShaderGlobal::set_int(rtsm_bindless_slotVarId, -1);
  clean_shvars();
}

bool is_initialized()
{
  return sunTracePass && sunFillPass && sunTilesPass && sunBlurPass && localPass && stabilizePass && bindless_range >= 0;
}

using TexHandle = dafg::VirtualResourceHandle<BaseTexture, true, false>;
using FrameParamsHandle = dafg::VirtualResourceHandle<const FrameParams, false, false>;
using RenderedHandle = dafg::VirtualResourceHandle<bool, false, false>;
using SharedNodeParams = eastl::shared_ptr<const NodeParams>;

struct PassContext
{
  SharedNodeParams params;
  FrameParamsHandle frameParams;
  dafg::AutoResolutionRequest<2> resolution;
};

static IPoint2 tile_count(const IPoint2 &resolution)
{
  return IPoint2((resolution.x + RTDL_TILE_W - 1) / RTDL_TILE_W, (resolution.y + RTDL_TILE_H - 1) / RTDL_TILE_H);
}

static int sun_quality(const FrameParams &params) { return clamp(params.sunQuality, 0, 2); }

static uint32_t shadow_dwords(const IPoint2 &resolution) { return resolution.x * resolution.y * RTDL_SHADOW_DWORDS; }

static uint32_t sun_sample_dwords(const IPoint2 &resolution)
{
  const IPoint2 tiles = tile_count(resolution);
  return tiles.x * tiles.y * (RTDL_TILE_W / RTDL_SUN_BLOCK) * (RTDL_TILE_H / RTDL_SUN_BLOCK);
}

static dafg::Texture2dCreateInfo screen_texture(dafg::Registry registry, const NodeParams &params, unsigned int fmt)
{
  return {TEXCF_UNORDERED | fmt, registry.getResolution<2>(params.resolutionName)};
}

static dafg::Texture2dCreateInfo screen_texture_array(dafg::Registry registry, const NodeParams &params, unsigned int fmt,
  uint32_t layers)
{
  return {TEXCF_UNORDERED | fmt, registry.getResolution<2>(params.resolutionName), 1, layers};
}

static dafg::Texture2dCreateInfo tile_texture(const NodeParams &params)
{
  return {TEXCF_UNORDERED | TEXFMT_R32UI, tile_count(params.resolution)};
}

static void read_texture(dafg::Registry registry, const char *name)
{
  registry.readTexture(name).atStage(dafg::Stage::CS).bindToShaderVar(name);
}

static void read_buffer(dafg::Registry registry, const char *name)
{
  registry.read(name).buffer().atStage(dafg::Stage::CS).bindToShaderVar(name);
}

static void read_buffer_history(dafg::Registry registry, const char *name, const char *shader_var)
{
  registry.historyFor(name).buffer().atStage(dafg::Stage::CS).bindToShaderVar(shader_var);
}

static void read_history(dafg::Registry registry, const char *name, const char *shader_var)
{
  registry.readTextureHistory(name).atStage(dafg::Stage::CS).bindToShaderVar(shader_var);
}

static TexHandle create_texture(dafg::Registry registry, const char *name, const dafg::Texture2dCreateInfo &info)
{
  return registry.create(name).texture(info).atStage(dafg::Stage::CS).bindToShaderVar(name).handle();
}

static TexHandle create_history_texture(dafg::Registry registry, const char *name, const dafg::Texture2dCreateInfo &info)
{
  return registry.create(name)
    .texture(info)
    .withHistory(dafg::History::ClearZeroOnFirstFrame)
    .atStage(dafg::Stage::CS)
    .bindToShaderVar(name)
    .handle();
}

static PassContext declare_pass(dafg::Registry registry, const SharedNodeParams &params)
{
  if (params->declareInputs)
    params->declareInputs(registry);
  return {params, registry.readBlob<FrameParams>(frame_params_blob_name).handle(), registry.getResolution<2>(params->resolutionName)};
}

static void set_frame_shader_vars(const FrameParams &params, const IPoint2 &full_resolution)
{
  const IPoint2 &resolution = frame_state.resolution;
  const IPoint2 &prevResolution = frame_state.prevResolution;
  ShaderGlobal::set_float4(rt_direct_lighting_resolutionVarId, resolution.x, resolution.y, 0, 0);
  ShaderGlobal::set_int4(rt_direct_lighting_resolutionIVarId, resolution.x, resolution.y, prevResolution.x, prevResolution.y);
  ShaderGlobal::set_int4(rt_direct_lighting_full_resolutionIVarId, full_resolution.x, full_resolution.y, 0, 0);
  ShaderGlobal::set_float4(rt_direct_lighting_paramsVarId, params.defaultLightSourceRadius, shadow_rays_per_pixel, max_sun_history,
    slot_hysteresis);
  ShaderGlobal::set_float4(rt_direct_lighting_sun_paramsVarId, sun_blur_scale, sun_blur_min_radius, sun_blur_max_radius,
    sun_clamp_sigma);
  ShaderGlobal::set_float4(rt_direct_lighting_stabilize_paramsVarId, max_local_shadow_history, local_clamp_sigma, local_shadow_radius,
    shadow_retest_period);
  ShaderGlobal::set_int4(rt_direct_lighting_frame_paramsVarId, frame_state.frame, frame_state.historyValid ? 1 : 0,
    shadow_rest_lights ? 1 : 0, change_tiles_retest ? 1 : 0);
  ShaderGlobal::set_int(rt_direct_lighting_sun_qualityVarId, sun_quality(params));
  ShaderGlobal::set_float4(world_view_posVarId, params.viewPos);
  ShaderGlobal::set_int(rtsm_has_nukeVarId, params.hasNuke ? 1 : 0);
}

static const FrameParams *begin_pass(const PassContext &ctx)
{
  const FrameParams &params = ctx.frameParams.ref();
  if (!params.render || !params.ctxId || !is_initialized())
    return nullptr;

  const int frame = int(dagor_frame_no());
  const IPoint2 fullResolution = ctx.resolution.get();
  if (frame_state.frame != frame)
  {
    const IPoint2 &dynamic = params.dynamicResolution;
    const IPoint2 resolution =
      dynamic.x > 0 && dynamic.y > 0 ? IPoint2(min(dynamic.x, fullResolution.x), min(dynamic.y, fullResolution.y)) : fullResolution;
    const bool consecutive = frame_state.frame >= 0 && frame == frame_state.frame + 1;
    frame_state.prevResolution = consecutive ? frame_state.resolution : resolution;
    frame_state.resolution = resolution;
    frame_state.frame = frame;
    frame_state.historyValid = consecutive && !params.resetHistory;
  }

  if (ctx.params->setShaderState)
    ctx.params->setShaderState();
  set_viewvecs_to_shader(params.viewTm, params.projTm);
  ShaderGlobal::set_float4(zn_zfarVarId, params.znear, params.zfar, 0, 0);
  set_frame_shader_vars(params, fullResolution);
  ShaderGlobal::set_int(rt_direct_lighting_dynamic_lightsVarId, ctx.params->dynamicLights ? 1 : 0);
  return &params;
}

static void flush_uav(BaseTexture *tex)
{
  d3d::resource_barrier(ResourceBarrierDesc(tex, RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0));
}

static void flush_uav(Sbuffer *buf)
{
  d3d::resource_barrier(ResourceBarrierDesc(buf, RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE));
}

static void publish_outputs(const FrameParams &params, BaseTexture *sun_shadow, bool dynamic_lights)
{
  d3d::update_bindless_resource(D3DResourceType::TEX, bindless_range + sun_shadow_bindless_index, sun_shadow);
  ShaderGlobal::set_int(rtsm_bindless_slotVarId, bindless_range + sun_shadow_bindless_index);
  ShaderGlobal::set_int(rtsm_is_translucentVarId, 0);

  if (params.csmTexture)
  {
    d3d::update_bindless_resource(D3DResourceType::TEX, bindless_range + csm_bindless_index, params.csmTexture);
    ShaderGlobal::set_int(csm_bindless_slotVarId, bindless_range + csm_bindless_index);
    ShaderGlobal::set_int(csm_sampler_bindless_slotVarId, d3d::register_bindless_sampler(params.csmSampler));
  }
  if (params.vsmTexture)
  {
    d3d::update_bindless_resource(D3DResourceType::TEX, bindless_range + vsm_bindless_index, params.vsmTexture);
    ShaderGlobal::set_int(vsm_bindless_slotVarId, bindless_range + vsm_bindless_index);
    ShaderGlobal::set_int(vsm_sampler_bindless_slotVarId, d3d::register_bindless_sampler(params.vsmSampler));
  }

  const IPoint2 &resolution = frame_state.resolution;
  ShaderGlobal::set_float4(rt_shadow_resolutionVarId, resolution.x, resolution.y, 0, 0);
  ShaderGlobal::set_int4(rt_shadow_resolutionIVarId, resolution.x, resolution.y, 0, 0);
  if (dynamic_lights)
  {
    ShaderGlobal::set_int(render_direct_lights_use_tilingVarId, 0);
    ShaderGlobal::set_int(use_precomputed_dynamic_lightsVarId, 1);
  }
}

static dafg::NodeHandle make_sun_trace_node(dafg::NameSpace ns, const SharedNodeParams &params)
{
  return ns.registerNode("sun_trace", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    registry.create("rt_direct_lighting_sun_samples")
      .byteAddressBuffer(sun_sample_dwords(params->resolution))
      .atStage(dafg::Stage::CS)
      .bindToShaderVar("rt_direct_lighting_sun_samples");
    create_texture(registry, "rt_direct_lighting_tiles", tile_texture(*params));
    create_texture(registry, "rt_direct_lighting_sun_hit", screen_texture(registry, *params, TEXFMT_R8UI));
    read_history(registry, "rt_direct_lighting_tiles_dilated", "rt_direct_lighting_tiles_dilated_prev");
    read_history(registry, "rt_direct_lighting_history", "rt_direct_lighting_history_prev");

    return [ctx = declare_pass(registry, params)]() {
      const FrameParams *fp = begin_pass(ctx);
      if (!fp)
        return;

      const IPoint2 tiles = tile_count(frame_state.resolution);
      bvh::bind_resources(fp->ctxId, frame_state.resolution.x);
      sunTracePass->dispatch(tiles.x, tiles.y, 1);
      bvh::unbind_resources();
    };
  });
}

static dafg::NodeHandle make_sun_tiles_node(dafg::NameSpace ns, const SharedNodeParams &params)
{
  return ns.registerNode("sun_tiles", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    read_texture(registry, "rt_direct_lighting_tiles");
    create_history_texture(registry, "rt_direct_lighting_tiles_dilated", tile_texture(*params));
    if (params->dynamicLights)
    {
      create_history_texture(registry, "rt_direct_lighting_change_tiles", tile_texture(*params));
      read_history(registry, "rt_direct_lighting_change_tiles", "rt_direct_lighting_change_tiles_prev");
    }

    return [ctx = declare_pass(registry, params)]() {
      if (!begin_pass(ctx))
        return;

      const IPoint2 tiles = tile_count(frame_state.resolution);
      sunTilesPass->dispatchThreads(tiles.x, tiles.y, 1);
    };
  });
}

static dafg::NodeHandle make_sun_fill_node(dafg::NameSpace ns, const SharedNodeParams &params)
{
  return ns.registerNode("sun_fill", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    read_history(registry, "rt_direct_lighting_tiles_dilated", "rt_direct_lighting_tiles_dilated_prev");
    read_history(registry, "rt_direct_lighting_history", "rt_direct_lighting_history_prev");
    auto sunSamples = registry.modify("rt_direct_lighting_sun_samples")
                        .buffer()
                        .atStage(dafg::Stage::CS)
                        .bindToShaderVar("rt_direct_lighting_sun_samples")
                        .handle();
    auto sunHit = registry.modifyTexture("rt_direct_lighting_sun_hit")
                    .atStage(dafg::Stage::CS)
                    .bindToShaderVar("rt_direct_lighting_sun_hit")
                    .handle();

    return [ctx = declare_pass(registry, params), sunSamples, sunHit]() {
      const FrameParams *fp = begin_pass(ctx);
      if (!fp || sun_quality(*fp) == 0)
        return;

      flush_uav(sunSamples.get());
      flush_uav(sunHit.get());
      bvh::bind_resources(fp->ctxId, frame_state.resolution.x);
      sunFillPass->dispatchThreads((frame_state.resolution.x + 1) / 2, frame_state.resolution.y, 1);
      bvh::unbind_resources();
    };
  });
}

static dafg::NodeHandle make_sun_blur_node(dafg::NameSpace ns, const SharedNodeParams &params)
{
  return ns.registerNode("sun_blur", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    read_buffer(registry, "rt_direct_lighting_sun_samples");
    read_texture(registry, "rt_direct_lighting_tiles_dilated");
    read_texture(registry, "rt_direct_lighting_sun_hit");
    create_texture(registry, "rt_direct_lighting_sun_spatial", screen_texture(registry, *params, TEXFMT_R8));

    return [ctx = declare_pass(registry, params)]() {
      if (!begin_pass(ctx))
        return;

      sunBlurPass->dispatchThreads(frame_state.resolution.x, frame_state.resolution.y, 1);
    };
  });
}

static dafg::NodeHandle make_local_node(dafg::NameSpace ns, const SharedNodeParams &params)
{
  return ns.registerNode("local", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    read_buffer_history(registry, "rt_direct_lighting_shadows", "rt_direct_lighting_shadows_prev");
    read_history(registry, "rt_direct_lighting_history", "rt_direct_lighting_history_prev");
    read_history(registry, "rt_direct_lighting_change_tiles", "rt_direct_lighting_change_tiles_prev");
    create_texture(registry, "rt_direct_lighting_shadow_samples", screen_texture(registry, *params, TEXFMT_R32G32UI));
    create_texture(registry, "rt_direct_lighting_normals", screen_texture(registry, *params, TEXFMT_R8UI));
    create_texture(registry, "rt_direct_lighting_radiance",
      screen_texture_array(registry, *params, TEXFMT_R11G11B10F, RTDL_RADIANCE_LAYERS));
    create_texture(registry, "rt_direct_lighting_unshadowed", screen_texture(registry, *params, TEXFMT_R11G11B10F));

    return [ctx = declare_pass(registry, params)]() {
      const FrameParams *fp = begin_pass(ctx);
      if (!fp)
        return;

      bvh::bind_resources(fp->ctxId, frame_state.resolution.x);
      localPass->dispatchThreads(frame_state.resolution.x, frame_state.resolution.y, 1);
      bvh::unbind_resources();
    };
  });
}

struct LocalOutputs
{
  TexHandle changeTiles;
  TexHandle output;
};

static dafg::NodeHandle make_stabilize_node(dafg::NameSpace ns, const SharedNodeParams &params)
{
  return ns.registerNode("stabilize", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    read_buffer(registry, "rt_direct_lighting_sun_samples");
    read_texture(registry, "rt_direct_lighting_tiles_dilated");
    read_texture(registry, "rt_direct_lighting_sun_hit");
    read_texture(registry, "rt_direct_lighting_sun_spatial");
    read_history(registry, "rt_direct_lighting_history", "rt_direct_lighting_history_prev");
    create_history_texture(registry, "rt_direct_lighting_history", screen_texture(registry, *params, TEXFMT_R32G32UI));
    auto sunShadow = create_texture(registry, sun_shadow_texture_name, screen_texture(registry, *params, TEXCF_RTARGET | TEXFMT_R8));
    auto rendered = registry.createBlob<bool>(rendered_blob_name).handle();

    eastl::unique_ptr<LocalOutputs> local;
    if (params->dynamicLights)
    {
      read_texture(registry, "rt_direct_lighting_shadow_samples");
      read_texture(registry, "rt_direct_lighting_normals");
      read_texture(registry, "rt_direct_lighting_radiance");
      read_texture(registry, "rt_direct_lighting_unshadowed");
      read_buffer_history(registry, "rt_direct_lighting_shadows", "rt_direct_lighting_shadows_prev");
      registry.create("rt_direct_lighting_shadows")
        .byteAddressBuffer(shadow_dwords(params->resolution))
        .withHistory(dafg::History::ClearZeroOnFirstFrame)
        .atStage(dafg::Stage::CS)
        .bindToShaderVar("rt_direct_lighting_shadows");
      auto output = registry.create(output_texture_name)
                      .texture(screen_texture(registry, *params, TEXFMT_R11G11B10F))
                      .atStage(dafg::Stage::CS)
                      .useAs(dafg::Usage::SHADER_RESOURCE)
                      .handle();
      auto changeTiles = registry.modifyTexture("rt_direct_lighting_change_tiles")
                           .atStage(dafg::Stage::CS)
                           .bindToShaderVar("rt_direct_lighting_change_tiles")
                           .handle();
      local = eastl::make_unique<LocalOutputs>(LocalOutputs{changeTiles, output});
    }

    return [ctx = declare_pass(registry, params), local = eastl::move(local), sunShadow, rendered]() {
      rendered.ref() = false;
      const FrameParams *fp = begin_pass(ctx);
      if (!fp)
      {
        ShaderGlobal::set_int(rtsm_bindless_slotVarId, -1);
        return;
      }

      if (local)
      {
        ShaderGlobal::set_texture(precomputed_dynamic_lightsVarId, local->output.get());
        flush_uav(local->changeTiles.get());
      }
      stabilizePass->dispatchThreads(frame_state.resolution.x, frame_state.resolution.y, 1);

      publish_outputs(*fp, sunShadow.get(), local != nullptr);
      rendered.ref() = true;
    };
  });
}

#if DAGOR_DBGLEVEL > 0
static int debug_view = RTDL_DEBUG_OFF;
static int debug_slot = 0;
static float debug_exposure_ev = 0;
static float debug_zoom = 1;
static Point2 debug_center = Point2(0.5f, 0.5f);
static int rt_direct_lighting_debug_paramsVarId = -1;
static int rt_direct_lighting_debug_scaleVarId = -1;
static ComputeShaderElement *debugPass = nullptr;
static UniqueTexWithShaderVar debug_texture;
static dafg::NodeHandle debug_node;
static eastl::optional<dafg::NameSpace> debug_name_space;
static SharedNodeParams debug_node_params;

static const char *const debug_view_names[] = {"Off", "Sun: traced samples (before fill)", "Sun: samples after fill",
  "Sun: penumbra tiles", "Sun: cone softness", "Sun: spatial blur", "Sun: stabilized", "Sun: history length", "Lights: output",
  "Lights: shadowless lights", "Lights: rest (beyond the slots)", "Lights: change tiles", "Slot: radiance", "Slot: ray samples",
  "Slot: stabilized visibility", "Slot: light"};
static const char *const debug_view_legends[] = {"",
  "White lit, black shadowed, gray not traced this frame, red faces away from the sun, magenta no sample, blue no surface.",
  "White lit, black shadowed, red faces away from the sun, magenta no sample, blue no surface.",
  "Dilated tiles over the sun visibility: yellow penumbra (blurred), red shadowed only, green lit only.",
  "Trace cone of the pixel for the next frame, from its own hit or the farthest hit of the dilated tiles around it: black the sun "
  "center (penumbra under 2.5 px), white the full disc (5 px, or no hit), gray between.",
  "Sun visibility after the spatial blur, 0 where no sample informs.", "Sun visibility after the temporal stabilization.",
  "Sun history length divided by the max sun history.", "Final dynamic light radiance.", "Radiance of the lights without shadows.",
  "Radiance of the shadow casting lights that got no slot, before the rest visibility.",
  "Red: a settled shadow changed in the tile (fades in 8 frames), the tile and its neighbours retest all slots in the next frame.",
  "Radiance of the slot light without its shadow.",
  "Shadow ray of the slot light in this frame: white visible, black occluded, gray no ray, green no light in the slot.",
  "Stabilized visibility of the slot light, red tint where unsettled, green no light in the slot.",
  "One color for each light key, green no light in the slot."};
static_assert(countof(debug_view_names) == RTDL_DEBUG_VIEW_COUNT);
static_assert(countof(debug_view_legends) == RTDL_DEBUG_VIEW_COUNT);

static bool is_lights_debug_view(int view) { return view >= RTDL_DEBUG_LIGHTS_OUTPUT; }

static bool is_slot_debug_view(int view) { return view >= RTDL_DEBUG_SLOT_RADIANCE; }

static bool is_radiance_debug_view(int view)
{
  return view == RTDL_DEBUG_LIGHTS_OUTPUT || view == RTDL_DEBUG_LIGHTS_SHADOWLESS || view == RTDL_DEBUG_LIGHTS_REST ||
         view == RTDL_DEBUG_SLOT_RADIANCE;
}

static void read_optional_texture(dafg::Registry registry, const char *name)
{
  registry.readTexture(name).atStage(dafg::Stage::CS).bindToShaderVar(name).optional();
}

static void read_optional_buffer(dafg::Registry registry, const char *name)
{
  registry.read(name).buffer().atStage(dafg::Stage::CS).bindToShaderVar(name).optional();
}

static dafg::NodeHandle make_debug_node(dafg::NameSpace ns, const SharedNodeParams &params)
{
  return ns.registerNode("debug", DAFG_PP_NODE_SRC, [params](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    read_optional_buffer(registry, "rt_direct_lighting_sun_samples");
    read_optional_texture(registry, "rt_direct_lighting_tiles_dilated");
    read_optional_texture(registry, "rt_direct_lighting_sun_spatial");
    read_optional_texture(registry, "rt_direct_lighting_history");
    if (params->dynamicLights)
    {
      read_optional_texture(registry, "rt_direct_lighting_shadow_samples");
      read_optional_texture(registry, "rt_direct_lighting_radiance");
      read_optional_texture(registry, "rt_direct_lighting_unshadowed");
      read_optional_texture(registry, "rt_direct_lighting_change_tiles");
      read_optional_texture(registry, output_texture_name);
      read_optional_buffer(registry, "rt_direct_lighting_shadows");
    }
    auto frameParams = registry.readBlob<FrameParams>(frame_params_blob_name).optional().handle();

    return [frameParams, resolution = registry.getResolution<2>(params->resolutionName), dynamicLights = params->dynamicLights]() {
      const FrameParams *fp = frameParams.get();
      Texture *target = debug_texture.getTex2D();
      if (!fp || !fp->render || !debugPass || !target || frame_state.frame != int(dagor_frame_no()) ||
          (is_lights_debug_view(debug_view) && !dynamicLights))
        return;

      TextureInfo ti;
      target->getinfo(ti);
      const IPoint2 &res = frame_state.resolution;
      if (res.x > ti.w || res.y > ti.h)
        return;

      set_frame_shader_vars(*fp, resolution.get());
      set_viewvecs_to_shader(fp->viewTm, fp->projTm);
      ShaderGlobal::set_int(rt_direct_lighting_dynamic_lightsVarId, dynamicLights ? 1 : 0);
      ShaderGlobal::set_int4(rt_direct_lighting_debug_paramsVarId, debug_view, debug_slot, sun_quality(*fp), 0);
      ShaderGlobal::set_float4(rt_direct_lighting_debug_scaleVarId, powf(2.f, debug_exposure_ev), max_sun_history, 0, 0);
      d3d::resource_barrier(ResourceBarrierDesc(target, RB_RW_UAV | RB_STAGE_COMPUTE, 0, 0));
      debug_texture.setVar();
      debugPass->dispatchThreads(res.x, res.y, 1);
      d3d::resource_barrier(ResourceBarrierDesc(target, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0));
    };
  });
}

static void close_debug_view()
{
  debug_node = {};
  debug_texture.close();
  safe_delete(debugPass);
}

static void update_debug_view()
{
  if (debug_view == RTDL_DEBUG_OFF || !debug_name_space || !debug_node_params || !is_initialized())
  {
    if (debug_node || debugPass)
      close_debug_view();
    return;
  }

  if (!debugPass)
  {
    debugPass = new_compute_shader("rt_direct_lighting_debug", true);
    rt_direct_lighting_debug_paramsVarId = get_shader_variable_id("rt_direct_lighting_debug_params", true);
    rt_direct_lighting_debug_scaleVarId = get_shader_variable_id("rt_direct_lighting_debug_scale", true);
  }

  const IPoint2 &size = debug_node_params->resolution;
  Texture *tex = debug_texture.getTex2D();
  TextureInfo ti;
  if (tex)
    tex->getinfo(ti);
  if (!tex || ti.w != size.x || ti.h != size.y)
    debug_texture = dag::create_tex(nullptr, size.x, size.y, TEXCF_UNORDERED | TEXCF_CLEAR_ON_CREATE | TEXFMT_R8G8B8A8, 1,
      "rt_direct_lighting_debug");

  if (!debug_node)
    debug_node = make_debug_node(*debug_name_space, debug_node_params);
}

static void track_debug_node_params(dafg::NameSpace ns, const SharedNodeParams &params)
{
  debug_name_space = ns;
  debug_node_params = params;
  debug_node = {};
  update_debug_view();
}

static void debug_view_image()
{
  Texture *tex = debug_texture.getTex2D();
  const IPoint2 &res = frame_state.resolution;
  if (!tex || res.x <= 0 || res.y <= 0)
    return;

  TextureInfo ti;
  tex->getinfo(ti);
  const float halfSize = 0.5f / debug_zoom;
  debug_center.x = clamp(debug_center.x, halfSize, 1 - halfSize);
  debug_center.y = clamp(debug_center.y, halfSize, 1 - halfSize);
  const Point2 viewMin = debug_center - Point2(halfSize, halfSize);
  const Point2 contentUV(float(res.x) / ti.w, float(res.y) / ti.h);
  const ImVec2 uv0(viewMin.x * contentUV.x, viewMin.y * contentUV.y);
  const ImVec2 uv1((viewMin.x + 2 * halfSize) * contentUV.x, (viewMin.y + 2 * halfSize) * contentUV.y);

  static const d3d::SamplerHandle pointSampler = [] {
    d3d::SamplerInfo info;
    info.filter_mode = d3d::FilterMode::Point;
    return d3d::request_sampler(info);
  }();
  const ImVec2 imageMin = ImGui::GetCursorScreenPos();
  const int width = int(ImGui::GetContentRegionAvail().x);
  const int height = int(width * float(res.y) / res.x);
  if (width <= 0 || height <= 0)
    return;

  ImGuiDagor::Sampler(pointSampler);
  ImGuiDagor::Image(tex, width, height, uv0, uv1);
  ImGuiDagor::Sampler(d3d::request_sampler({}));

  const ImVec2 imageMax(imageMin.x + width, imageMin.y + height);
  if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(imageMin, imageMax) && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
  {
    const ImVec2 mouse = ImGui::GetMousePos();
    debug_center = viewMin + Point2((mouse.x - imageMin.x) / width, (mouse.y - imageMin.y) / height) * (2 * halfSize);
  }
}

static void debug_view_imgui()
{
  ImGui::SeparatorText("Debug view");
  ImGui::Combo("View", &debug_view, debug_view_names, RTDL_DEBUG_VIEW_COUNT);
  update_debug_view();
  if (debug_view == RTDL_DEBUG_OFF || !debug_node_params)
    return;
  if (is_lights_debug_view(debug_view) && !debug_node_params->dynamicLights)
  {
    ImGui::TextUnformatted("Dynamic lights are off in this mode.");
    return;
  }

  if (is_slot_debug_view(debug_view))
    ImGui::SliderInt("Slot", &debug_slot, 0, RTDL_SHADOW_SLOTS - 1);
  if (is_radiance_debug_view(debug_view))
    ImGui::SliderFloat("Exposure (EV)", &debug_exposure_ev, -16, 16);
  ImGui::SliderFloat("Zoom", &debug_zoom, 1, 16, "%.1f", ImGuiSliderFlags_Logarithmic);
  ImGui::TextWrapped("%s Click the image to center the zoom.", debug_view_legends[debug_view]);
  debug_view_image();
}
#else
static void track_debug_node_params(dafg::NameSpace, const SharedNodeParams &) {}
#endif

void register_nodes(dafg::NameSpace ns, const NodeParams &params, dag::Vector<dafg::NodeHandle> &nodes)
{
  G_ASSERT_RETURN(params.resolution.x > 0 && params.resolution.y > 0, );

  SharedNodeParams shared = eastl::make_shared<NodeParams>(params);
  nodes.emplace_back(make_sun_trace_node(ns, shared));
  nodes.emplace_back(make_sun_tiles_node(ns, shared));
  nodes.emplace_back(make_sun_fill_node(ns, shared));
  nodes.emplace_back(make_sun_blur_node(ns, shared));
  if (params.dynamicLights)
    nodes.emplace_back(make_local_node(ns, shared));
  nodes.emplace_back(make_stabilize_node(ns, shared));
  track_debug_node_params(ns, shared);
}

#if DAGOR_DBGLEVEL > 0
static void imguiWindow()
{
  ImGui::SliderInt("Shadow rays per pixel", &shadow_rays_per_pixel, 1, 3);
  ImGui::SliderInt("Max sun history", &max_sun_history, 1, 63);
  ImGui::SliderFloat("Sun blur scale", &sun_blur_scale, 0, 2);
  ImGui::SliderFloat("Sun blur min radius", &sun_blur_min_radius, 0, 4);
  ImGui::SliderFloat("Sun blur max radius", &sun_blur_max_radius, 1, 64);
  ImGui::SliderFloat("Sun history clamp sigma", &sun_clamp_sigma, 0.5f, 6);
  ImGui::SliderInt("Max local shadow history", &max_local_shadow_history, 1, 63);
  ImGui::SliderFloat("Local shadow clamp sigma", &local_clamp_sigma, 0.5f, 6);
  ImGui::SliderFloat("Local shadow blur radius", &local_shadow_radius, 0, 8);
  ImGui::SliderInt("Settled shadow re-test period", &shadow_retest_period, 1, 32);
  ImGui::SliderFloat("Slot hysteresis", &slot_hysteresis, 1, 4);
  ImGui::Checkbox("Rest lights take the slot visibility", &shadow_rest_lights);
  ImGui::Checkbox("Retest the tiles where a shadow changed", &change_tiles_retest);
  debug_view_imgui();
}

REGISTER_IMGUI_WINDOW("Render", "RT direct lighting", imguiWindow);
#endif
} // namespace rtdl
