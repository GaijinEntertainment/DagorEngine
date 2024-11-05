// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <perfMon/dag_statDrv.h>

// for ECS events
#include "render/renderEvent.h"
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>

#include <ecs/render/resPtr.h>
#include <render/rendererFeatures.h>
#include <render/resolution.h>
#include <util/dag_console.h>
#include "postfxRender.h"
#include <render/daBfg/ecs/frameGraphNode.h>
#include "render/world/frameGraphNodes/frameGraphNodes.h"
#include <drv/3d/dag_texture.h>

ECS_REGISTER_RELOCATABLE_TYPE(DepthOfFieldPS, nullptr)


template <typename Callable>
static void init_dof_ecs_query(ecs::EntityId, Callable);

template <typename Callable>
static void get_dof_ecs_query(Callable);

template <typename Callable>
static void set_far_dof_ecs_query(Callable);

template <typename Callable>
static void check_bloom_ecs_query(Callable);

template <typename Callable>
static void set_fade_mul_ecs_query(Callable);

void setFadeMul(float mul)
{
  set_fade_mul_ecs_query([mul](ecs::Object &adaptation_override_settings) {
    adaptation_override_settings.addMember("adaptation__fadeMul", clamp(mul, 0.0001f, 1.f));
  });
}

template <bool REPORT_DEBUG>
static IPoint2 get_dof_resolution(const IPoint2 &postfx_resolution, const IPoint2 &rendering_resolution)
{
  if (rendering_resolution == postfx_resolution || rendering_resolution == postfx_resolution / 2)
    return postfx_resolution;
  if (rendering_resolution.y < postfx_resolution.y / 2)
    return rendering_resolution * 2;
  if (rendering_resolution.y < postfx_resolution.y)
    return rendering_resolution;
  if (REPORT_DEBUG)
    logerr("Unexpected resolutions in get_dof_resolution. DoF only supports rendering resolutions lower than postfx "
           "resolution. postfx_resolution: %dx%d, rendering_resolution: %dx%d.",
      postfx_resolution.x, postfx_resolution.y, rendering_resolution.x, rendering_resolution.y);
  return postfx_resolution;
}

ECS_TAG(render)
static void init_dof_es(const BeforeLoadLevel &, ecs::EntityManager &manager)
{
  if (renderer_has_feature(FeatureRenderFlags::BLOOM)) // todo: fix dof render implementation dependencies on bloom
  {
    init_dof_ecs_query(manager.getOrCreateSingletonEntity(ECS_HASH("depth_of_field")),
      [](DepthOfFieldPS &dof, UniqueTex &downsampled_dof_frame_fallback) {
        dof.setLinearBlend(false);
        IPoint2 postfx_resolution = get_postfx_resolution();
        IPoint2 rendering_resolution = get_rendering_resolution();
        IPoint2 dofResolution = get_dof_resolution<true>(postfx_resolution, rendering_resolution);
        downsampled_dof_frame_fallback.close();
        if (dofResolution != postfx_resolution)
        {
          uint32_t rtFmt = TEXFMT_R11G11B10F;
          if (!(d3d::get_texformat_usage(rtFmt) & d3d::USAGE_RTARGET))
            rtFmt = TEXFMT_A16B16G16R16F;
          const char *texname = "downsampled_dof_frame_fallback";
          downsampled_dof_frame_fallback =
            dag::create_tex(nullptr, dofResolution.x / 2, dofResolution.y / 2, rtFmt | TEXCF_RTARGET, 1, texname);
        }
        // This can be somewhat misleading, DoF will init to half of this resolution internally!
        dof.init(dofResolution.x, dofResolution.y);
      });
  }
}

ECS_TAG(render)
static void set_dof_resolution_es(const SetResolutionEvent &event, DepthOfFieldPS &dof)
{
  IPoint2 dofResolution = get_dof_resolution<false>(event.postFxResolution, event.renderingResolution);
  if (event.type == SetResolutionEvent::Type::SETTINGS_CHANGED)
  {
    dof.init(dofResolution.x, dofResolution.y);
    dof.changeResolution(dofResolution.x, dofResolution.y);
  }
  else if (event.type == SetResolutionEvent::Type::DYNAMIC_RESOLUTION)
    dof.changeResolution(dofResolution.x, dofResolution.y);
}


ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(*)
static void init_dof_params_es(const ecs::Event &,
  DepthOfFieldPS &dof,
  bool dof__on,
  bool dof__is_filmic,
  float dof__focusDistance,
  float dof__fStop,
  float dof__sensorHeight_mm,
  float dof__focalLength_mm,
  float dof__nearDofStart,
  float dof__nearDofEnd,
  float dof__nearDofAmountPercent,
  float dof__farDofStart,
  float dof__farDofEnd,
  float dof__farDofAmountPercent,
  float dof__bokehShape_kernelSize,
  float dof__bokehShape_bladesCount,
  float dof__minCheckDistance,
  dabfg::NodeHandle *dof__depth_for_transparency_node_handle = nullptr,
  dabfg::NodeHandle *dof__downsample_depth_for_transparency_node_handle = nullptr)
{
  if (dof__on)
  {
    dof.setOn(true);
    DOFProperties focus = dof.getDoFParams();
    if (dof__is_filmic)
    {
      focus.setFilmicDoF(dof__focusDistance, dof__fStop, dof__sensorHeight_mm * 0.001f, dof__focalLength_mm * 0.001f);
      dof.setMinCheckDistance(0.08f);
      dof.setCocAccumulation(true);
    }
    else
    {
      focus.setNearDof(dof__nearDofStart, dof__nearDofEnd, dof__nearDofAmountPercent);
      if (focus.setFarDof(dof__farDofStart, dof__farDofEnd, dof__farDofAmountPercent))
      {
        if (dof__depth_for_transparency_node_handle && dof__downsample_depth_for_transparency_node_handle)
        {
          *dof__depth_for_transparency_node_handle = makeDepthWithTransparencyNode();
          *dof__downsample_depth_for_transparency_node_handle = makeDownsampleDepthWithTransparencyNode();
        }
      }
      else
      {
        if (dof__depth_for_transparency_node_handle && dof__downsample_depth_for_transparency_node_handle)
        {
          *dof__depth_for_transparency_node_handle = {};
          *dof__downsample_depth_for_transparency_node_handle = {};
        }
      }
      dof.setMinCheckDistance(dof__minCheckDistance);
      dof.setCocAccumulation(true);
    }
    dof.setDoFParams(focus);
    dof.setBokehShape(dof__bokehShape_kernelSize, dof__bokehShape_bladesCount);
  }
  else
    dof.setOn(false);
}

ECS_TAG(render)
static void render_dof_es(const RenderPostFx &event, DepthOfFieldPS &dof, const UniqueTex &downsampled_dof_frame_fallback)
{
  IPoint2 postFxResolution = get_postfx_resolution();
  IPoint2 renderingResolution = get_rendering_resolution();
  IPoint2 dofResolution = get_dof_resolution<false>(postFxResolution, renderingResolution);

  TextureIDPair frameForDof, closeDepthForDof;
  if (renderingResolution == dofResolution)
  {
    frameForDof = event.downsampledColor;
    closeDepthForDof = event.closedDepth;
  }
  else if (renderingResolution == dofResolution / 2)
  {
    frameForDof = event.prevRTColor;
    closeDepthForDof = event.targetDepth;
  }
  else if (renderingResolution.y < dofResolution.y / 2)
  {
    TIME_D3D_PROFILE(dof_fallback_downsample);
    d3d::stretch_rect(event.prevRTColor.getTex2D(), downsampled_dof_frame_fallback.getTex2D());
    frameForDof = {downsampled_dof_frame_fallback.getTex2D(), downsampled_dof_frame_fallback.getTexId()};
    closeDepthForDof = event.targetDepth;
  }
  else
  {
    TIME_D3D_PROFILE(dof_fallback_downsample);
    d3d::stretch_rect(event.downsampledColor.getTex2D(), downsampled_dof_frame_fallback.getTex2D());
    frameForDof = {downsampled_dof_frame_fallback.getTex2D(), downsampled_dof_frame_fallback.getTexId()};
    closeDepthForDof = event.closedDepth;
  }

  dof.perform(frameForDof, closeDepthForDof, event.zNear, event.zFar, event.fovScale);
}


void set_dof_blend_depth_tex(TEXTUREID tex)
{
  get_dof_ecs_query([&](DepthOfFieldPS &dof) { dof.setBlendDepthTex(tex); });
}


static bool dof_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("postfx", "dof_infinite", 1, 3)
  {
    float fNumber = argc > 1 ? atof(argv[1]) : 1;
    float sensorHeight_mm = argc > 2 ? atof(argv[2]) : 24.0f;
    get_dof_ecs_query([&](DepthOfFieldPS &dof) {
      dof.setOn(true);
      DOFProperties focus = dof.getDoFParams();
      focus.setFilmicDoF(-1.f, fNumber, sensorHeight_mm * 0.001f, 0.5f * sensorHeight_mm * 0.001f);
      dof.setDoFParams(focus);
    });
    console::print("usage: postfx.dof_focus [fNumber=%f] [sensorHeight_mm=%f] Focuses at infinity", fNumber, sensorHeight_mm);
  }
  CONSOLE_CHECK_NAME("postfx", "dof_near", 1, 4)
  {
    float amount = argc > 1 ? atof(argv[1]) : 0;
    float start = argc > 2 ? atof(argv[2]) : 0.08;
    float end = argc > 3 ? atof(argv[3]) : start + 10;
    get_dof_ecs_query([&](DepthOfFieldPS &dof) {
      dof.setOn(true);
      DOFProperties focus = dof.getDoFParams();
      focus.setNearDof(start, end, amount);
      dof.setDoFParams(focus);
    });
    console::print("usage: postfx.dof_near [amount(percent of screen)=%f] [start=%f] [end=%f]", amount, start, end);
  }
  CONSOLE_CHECK_NAME("postfx", "dof_far", 1, 4)
  {
    float amount = argc > 1 ? atof(argv[1]) : 0;
    float start = argc > 2 ? atof(argv[2]) : 0.001;
    float end = argc > 3 ? atof(argv[3]) : start + 10;
    set_far_dof_ecs_query([&](bool &dof__on, float &dof__farDofStart, float &dof__farDofEnd, float &dof__farDofAmountPercent) {
      dof__on = true;
      dof__farDofStart = start;
      dof__farDofEnd = end;
      dof__farDofAmountPercent = amount;
    });
    console::print("usage: postfx.dof_far [amount(percent of screen)=%f] [start=%f] [end=%f]", amount, start, end);
  }
  CONSOLE_CHECK_NAME("postfx", "dof_bokeh", 1, 3)
  {
    float blades = argc > 1 ? atof(argv[1]) : 6;
    float fStops = argc > 2 ? atof(argv[2]) : 1;
    if (argc > 1)
    {
      get_dof_ecs_query([&](DepthOfFieldPS &dof) {
        dof.setOn(true);
        DOFProperties focus = dof.getDoFParams();
        dof.setBokehShape(fStops, blades);
        dof.setDoFParams(focus);
      });
    }
    console::print("usage: postfx.dof_bokeh [blades=%f] [fStops/fNumber=%f]", blades, fStops);
  }
  CONSOLE_CHECK_NAME("postfx", "dof_minCheckDistance", 1, 2)
  {
    float dist = argc > 1 ? atof(argv[1]) : -1;
    get_dof_ecs_query([&](DepthOfFieldPS &dof) {
      dof.setOn(true);
      DOFProperties focus = dof.getDoFParams();
      dof.setMinCheckDistance(dist);
      dof.setDoFParams(focus);
    });
    console::print("usage: postfx.dof_minCheckDistance [min distance to check if near dof is on=%f]", dist);
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(dof_console_handler);

template <typename Callable>
static void postfx_bind_additional_textures_from_registry_ecs_query(Callable c);
void postfx_bind_additional_textures_from_registry(dabfg::Registry &registry)
{
  postfx_bind_additional_textures_from_registry_ecs_query([&registry](const ecs::StringList &postfx__additional_bind_textures) {
    for (const ecs::string texName : postfx__additional_bind_textures)
      registry.readTexture(texName.c_str()).atStage(dabfg::Stage::PS).bindToShaderVar(texName.c_str()).optional();
  });
}

template <typename Callable>
static void postfx_bind_additional_textures_from_namespace_ecs_query(Callable c);
void postfx_bind_additional_textures_from_namespace(dabfg::NameSpaceRequest &ns)
{
  postfx_bind_additional_textures_from_namespace_ecs_query([&ns](const ecs::StringList &postfx__additional_bind_textures) {
    for (const ecs::string texName : postfx__additional_bind_textures)
      ns.readTexture(texName.c_str()).atStage(dabfg::Stage::POST_RASTER).bindToShaderVar(texName.c_str()).optional();
  });
}

template <typename Callable>
static void postfx_read_additional_textures_from_registry_ecs_query(Callable c);
void postfx_read_additional_textures_from_registry(dabfg::Registry &registry)
{
  postfx_read_additional_textures_from_registry_ecs_query([&registry](const ecs::StringList &postfx__additional_read_textures) {
    for (const ecs::string texName : postfx__additional_read_textures)
      registry.readTexture(texName.c_str()).atStage(dabfg::Stage::POST_RASTER).useAs(dabfg::Usage::SHADER_RESOURCE).optional();
  });
}