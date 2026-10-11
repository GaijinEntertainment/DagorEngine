// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gtao_public.h"
#include <render/viewVecs.h>
#include <render/set_reprojection.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_viewScissor.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaders.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint4.h>

#define GLOBAL_VARS_LIST          \
  VAR(gtao_temporal_directions)   \
  VAR(gtao_temporal_offset)       \
  VAR(gtao_half_hfov_tan)         \
  VAR(gtao_dispatch_grid_dim)     \
  VAR(raw_gtao_tex)               \
  VAR(raw_gtao_tex_samplerstate)  \
  VAR(gtao_prev_tex)              \
  VAR(gtao_prev_tex_samplerstate) \
  VAR(gtao_tex)                   \
  VAR(ssao_tex)                   \
  VAR(ssao_tex_samplerstate)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

static constexpr const char *gtao_ps_name = "gtao_main";
static constexpr const char *spatial_filter_ps_name = "gtao_spatial";
static constexpr const char *temporal_filter_ps_name = "gtao_temporal";
static constexpr const char *gtao_cs_name = "gtao_main_cs";
static constexpr const char *spatial_filter_cs_name = "gtao_spatial_cs";
static constexpr const char *temporal_filter_cs_name = "gtao_temporal_cs";

static float TEMPORAL_ROTATIONS[] = {60, 300, 180, 240, 120, 0};
static float TEMPORAL_OFFSETS[] = {0.0f, 0.5f, 0.25f, 0.75f};

namespace gtao_pass
{

static d3d::SamplerHandle request_clamp_sampler()
{
  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
  smpInfo.border_color = d3d::BorderColor::Color::OpaqueWhite;
  return d3d::request_sampler(smpInfo);
}

FilterResources create_filter_resources(bool use_compute)
{
  FilterResources res;

  if (use_compute)
  {
    res.spatialFilterRendererCS.reset(new_compute_shader(spatial_filter_cs_name, true));
    res.temporalFilterRendererCS.reset(new_compute_shader(temporal_filter_cs_name, true));
  }
  else
  {
    res.spatialFilterRenderer.reset(create_postfx_renderer(spatial_filter_ps_name));
    res.temporalFilterRenderer.reset(create_postfx_renderer(temporal_filter_ps_name));
  }

  d3d::SamplerHandle sampler = request_clamp_sampler();
  ShaderGlobal::set_sampler(raw_gtao_tex_samplerstateVarId, sampler);
  ShaderGlobal::set_sampler(gtao_prev_tex_samplerstateVarId, sampler);

  return res;
}

RenderResources create_render_resources(uint32_t flags)
{
  uint32_t cflags = ao_common::creation_flags_to_format(ao_common::consider_shader_assumes(flags));
  const bool useCompute = d3d::should_use_compute_for_image_processing({cflags});

  RenderResources res;
  res.filter = create_filter_resources(useCompute);
  if (useCompute)
    res.aoRendererCS.reset(new_compute_shader(gtao_cs_name, true));
  else
    res.aoRenderer.reset(create_postfx_renderer(gtao_ps_name));

  ShaderGlobal::set_sampler(ssao_tex_samplerstateVarId, request_clamp_sampler());

  return res;
}

void resolve(const RenderResources &res, ao_common::FrameState &frame_state, ao_common::ReprojectionState *reprojection,
  const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 *world_pos, SubFrameSample sub_sample, bool is_main_view, int width,
  int height)
{
  TIME_D3D_PROFILE(GTAO_render);

  if (reprojection)
  {
    set_reprojection(view_tm, proj_tm, reprojection->prevProjTm, reprojection->prevWorldPos, reprojection->prevGlobTm,
      reprojection->prevViewVecLT, reprojection->prevViewVecRT, reprojection->prevViewVecLB, reprojection->prevViewVecRB, world_pos);
  }

  ShaderGlobal::set_float(gtao_half_hfov_tanVarId, 1.0f / max(proj_tm.m[0][0], 1e-6f));

  // GTAO uses local_view_N, which is initialized by setting view_tm
  d3d::settm(TM_VIEW, view_tm);

  const float temporalRotation = TEMPORAL_ROTATIONS[frame_state.frameNo % 6];
  const float temporalOffset = TEMPORAL_OFFSETS[(frame_state.frameNo / 6) % 4];
  ShaderGlobal::set_float(gtao_temporal_directionsVarId, temporalRotation / 360);
  ShaderGlobal::set_float(gtao_temporal_offsetVarId, temporalOffset);

  if (is_main_view && (sub_sample == SubFrameSample::Single || sub_sample == SubFrameSample::Last))
    frame_state.frameNo++;

  IPoint2 sres = IPoint2(width, height);

  if (res.aoRendererCS)
  {
    // Must match the [numthreads(8, 8, 1)] in gtao_resolve_cs.
    ShaderGlobal::set_int4(gtao_dispatch_grid_dimVarId, IPoint4((sres.x + 7) / 8, (sres.y + 7) / 8, 0, 0));
    res.aoRendererCS->dispatchThreads(sres.x, sres.y, 1);
  }
  else
  {
    d3d::setviewscissor(0, 0, sres.x, sres.y);
    res.aoRenderer->render();
  }
}

static void spatial_filter(const FilterResources &res, int width, int height)
{
  TIME_D3D_PROFILE(GTAO_spatial);

  IPoint2 sres = IPoint2(width, height);

  if (res.spatialFilterRendererCS)
  {
    res.spatialFilterRendererCS->dispatchThreads(sres.x, sres.y, 1);
  }
  else
  {
    d3d::setviewscissor(0, 0, sres.x, sres.y);
    res.spatialFilterRenderer->render();
  }
}

static void temporal_filter(const FilterResources &res, int width, int height)
{
  TIME_D3D_PROFILE(GTAO_temporal);

  IPoint2 sres = IPoint2(width, height);

  if (res.temporalFilterRendererCS)
  {
    res.temporalFilterRendererCS->dispatchThreads(sres.x, sres.y, 1);
  }
  else
  {
    d3d::setviewscissor(0, 0, sres.x, sres.y);
    res.temporalFilterRenderer->render();
  }
}

void spatiotemporal_filter(const FilterResources &res, BaseTexture *raw_tex, BaseTexture *prev_tex, BaseTexture *spatial_tex,
  int width, int height)
{
  SCOPE_RENDER_TARGET;

  if (res.spatialFilterRendererCS)
    d3d::set_rwtex(STAGE_CS, 0, spatial_tex, 0, 0);
  else
    d3d::set_render_target({}, DepthAccess::RW, {{spatial_tex, 0, 0}});

  d3d::resource_barrier({raw_tex, RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});
  ShaderGlobal::set_texture(raw_gtao_texVarId, raw_tex);

  spatial_filter(res, width, height);

  if (res.temporalFilterRendererCS)
    d3d::set_rwtex(STAGE_CS, 0, raw_tex, 0, 0);
  else
    d3d::set_render_target({}, DepthAccess::RW, {{raw_tex, 0, 0}});

  d3d::resource_barrier({spatial_tex, RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});
  ShaderGlobal::set_texture(gtao_prev_texVarId, prev_tex);
  ShaderGlobal::set_texture(gtao_texVarId, spatial_tex);
  ShaderGlobal::set_texture(raw_gtao_texVarId, BAD_TEXTUREID);

  temporal_filter(res, width, height);

  ShaderGlobal::set_texture(gtao_prev_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(gtao_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(raw_gtao_texVarId, BAD_TEXTUREID);

  if (res.temporalFilterRendererCS)
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
}

} // namespace gtao_pass

GTAORenderer::GTAORenderer(int w, int h, int views, uint32_t flags, bool use_own_textures) :
  rawTargetIdx{0}, spatialTargetIdx{0}, historyIdx{0}
{
  aoWidth = w;
  aoHeight = h;
  useOwnTextures = use_own_textures;

  if (useOwnTextures)
  {
    uint32_t cflags = ao_common::creation_flags_to_format(ao_common::consider_shader_assumes(flags));
    cflags |= d3d::should_use_compute_for_image_processing({cflags}) ? TEXCF_UNORDERED : TEXCF_RTARGET;
    gtaoTex.forTheFirstN(views, [&](auto &a, int view, int item) {
      String name(128, "gtao_tex_%d_%d", view, item);
      a = dag::create_tex(NULL, aoWidth, aoHeight, cflags, 1, name, RESTAG_AO);
      d3d::resource_barrier({a.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    });
  }

  resources = gtao_pass::create_render_resources(flags);
}

void GTAORenderer::setCurrentView(int viewIndex)
{
  gtaoTex.setCurrentView(viewIndex);
  frameState.setCurrentView(viewIndex);
  reprojection.setCurrentView(viewIndex);
}

void GTAORenderer::updateCurFrameIdxs()
{
  const uint32_t sampleNo = frameState.current().frameNo;
  const bool even = (sampleNo & 1) == 0;
  rawTargetIdx = even ? 0 : 2;
  spatialTargetIdx = 1;
  historyIdx = even ? 2 : 0; // last frame's rawTargetIdx
}

void GTAORenderer::render(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *, BaseTexture *ssao_tex,
  BaseTexture *prev_ssao_tex, BaseTexture *tmp_tex, const DPoint3 *world_pos, SubFrameSample sub_sample)
{
  renderGTAO(view_tm, proj_tm, ssao_tex, world_pos, sub_sample);
  applyGTAOFilter(ssao_tex, prev_ssao_tex, tmp_tex);
}

void GTAORenderer::renderGTAO(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *ssao_tex, const DPoint3 *world_pos,
  SubFrameSample sub_sample)
{
  // GTAO Renderer can work in two modes - using it's own textures or external ones. Mode is set in constructor.
  G_ASSERT(useOwnTextures || ssao_tex);

  // Index setup for 3 textures, 3 pass rendering, while always keeping last result as history
  if (useOwnTextures)
    updateCurFrameIdxs();

  BaseTexture *rawTex = ssao_tex ? ssao_tex : gtaoTex[rawTargetIdx].getBaseTex();

  SCOPE_RENDER_TARGET;
  if (resources.aoRendererCS)
    d3d::set_rwtex(STAGE_CS, 0, rawTex, 0, 0);
  else
    d3d::set_render_target({}, DepthAccess::RW, {{rawTex, 0, 0}});

  ShaderGlobal::set_texture(gtao_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(raw_gtao_texVarId, BAD_TEXTUREID);

  gtao_pass::resolve(resources, frameState.current(), &reprojection.current(), view_tm, proj_tm, world_pos, sub_sample,
    /*is_main_view*/ true, aoWidth, aoHeight);

  if (resources.aoRendererCS)
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
}

void GTAORenderer::applyGTAOFilter(BaseTexture *ssao_tex, BaseTexture *prev_ssao_tex, BaseTexture *tmp_tex)
{
  // GTAO Renderer can work in two modes - using it's own textures or external ones. Mode is set in constructor.
  G_ASSERT(useOwnTextures || (ssao_tex && prev_ssao_tex && tmp_tex));

  BaseTexture *rawTex = ssao_tex ? ssao_tex : gtaoTex[rawTargetIdx].getBaseTex();
  BaseTexture *prevGtaoTex = prev_ssao_tex ? prev_ssao_tex : gtaoTex[historyIdx].getBaseTex();
  BaseTexture *spatialTex = tmp_tex ? tmp_tex : gtaoTex[spatialTargetIdx].getBaseTex();

  gtao_pass::spatiotemporal_filter(resources.filter, rawTex, prevGtaoTex, spatialTex, aoWidth, aoHeight);

  if (useOwnTextures)
  {
    // if rawTex is external, barrier for it will be set in framegraph automatically
    d3d::resource_barrier({rawTex, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
    ShaderGlobal::set_texture(ssao_texVarId, rawTex);
  }
}

GTAORenderer::~GTAORenderer()
{
  gtaoTex.forEach([&](ResizableTex &holder) { holder.close(); });
}

void GTAORenderer::reset() {}

Texture *GTAORenderer::getSSAOTex() { return gtaoTex[rawTargetIdx].getTex2D(); };

TEXTUREID GTAORenderer::getSSAOTexId() { return gtaoTex[rawTargetIdx].getTexId(); };
