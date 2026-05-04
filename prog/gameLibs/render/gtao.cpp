// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/gtao.h>
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

#define GLOBAL_VARS_LIST          \
  VAR(gtao_temporal_directions)   \
  VAR(gtao_temporal_offset)       \
  VAR(gtao_half_hfov_tan)         \
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


GTAORenderer::GTAORenderer(int w, int h, int views, uint32_t flags, bool use_own_textures, bool use_own_reprojection_params) :
  even{false}, rawTargetIdx{0}, spatialTargetIdx{0}, temporalTargetIdx{0}, historyIdx{0}
{

  aoWidth = w;
  aoHeight = h;
  useOwnTextures = use_own_textures;
  useOwnReprojectionParams = use_own_reprojection_params;

  uint32_t cflags = ssao_detail::creation_flags_to_format(ssao_detail::consider_shader_assumes(flags));

  if (d3d::should_use_compute_for_image_processing({cflags}))
  {
    aoRendererCS.reset(new_compute_shader((String(gtao_sh_name) + "_cs").data(), true));
    spatialFilterRendererCS.reset(new_compute_shader((String(spatial_filter_sh_name) + "_cs").data(), true));
    temporalFilterRendererCS.reset(new_compute_shader((String(temporal_filter_sh_name) + "_cs").data(), true));
  }

  if (useOwnTextures)
  {
    cflags |= aoRendererCS ? TEXCF_UNORDERED : TEXCF_RTARGET;
    gtaoTex.forTheFirstN(views, [&](auto &a, int view, int item) {
      String name(128, "gtao_tex_%d_%d", view, item);
      a = dag::create_tex(NULL, aoWidth, aoHeight, cflags, 1, name, RESTAG_AO);
      d3d::resource_barrier({a.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    });
  }

  afrs.forEach([](Afr &afr) { memset(&afr, 0, sizeof(afr)); });
  afrs.setCurrentView(0);

  aoRenderer.reset(create_postfx_renderer(gtao_sh_name));
  spatialFilterRenderer.reset(create_postfx_renderer(spatial_filter_sh_name));
  temporalFilterRenderer.reset(create_postfx_renderer(temporal_filter_sh_name));

  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    smpInfo.border_color = d3d::BorderColor::Color::OpaqueWhite;
    d3d::SamplerHandle sampler = d3d::request_sampler(smpInfo);
    ShaderGlobal::set_sampler(raw_gtao_tex_samplerstateVarId, sampler);
    ShaderGlobal::set_sampler(gtao_prev_tex_samplerstateVarId, sampler);
    ShaderGlobal::set_sampler(ssao_tex_samplerstateVarId, sampler);
  }
}


void GTAORenderer::changeResolution(int w, int h)
{
  // copy-paste from GTAORenderer::render()
  Afr &afr = afrs.current();
  even = afr.sampleNo % 2 == 0;
  rawTargetIdx = even ? 0 : 2;
  spatialTargetIdx = 1;

  if (useOwnTextures)
  {
    gtaoTex[rawTargetIdx].resize(w, h);
    gtaoTex[spatialTargetIdx].resize(w, h);
  }
  // temporalTargetIdx == rawTargetIdx, so already resized

  aoWidth = w;
  aoHeight = h;
}

static float TEMPORAL_ROTATIONS[] = {60, 300, 180, 240, 120, 0};
static float TEMPORAL_OFFSETS[] = {0.0f, 0.5f, 0.25f, 0.75f};

void GTAORenderer::setCurrentView(int viewIndex)
{
  gtaoTex.setCurrentView(viewIndex);
  afrs.setCurrentView(viewIndex);
}

void GTAORenderer::updateCurFrameIdxs()
{
  Afr &afr = afrs.current();
  even = (afr.sampleNo & 1) == 0;
  rawTargetIdx = even ? 0 : 2;
  spatialTargetIdx = 1;
  temporalTargetIdx = rawTargetIdx;
  historyIdx = even ? 2 : 0; // last frame's temporalTargetIdx
}

void GTAORenderer::renderGTAO(BaseTexture *rawTex, const DynRes *dynamic_resolution)
{
  Afr &afr = afrs.current();

  TIME_D3D_PROFILE(GTAO_main);
  const float temporalRotation = TEMPORAL_ROTATIONS[afr.sampleNo % 6];
  const float temporalOffset = TEMPORAL_OFFSETS[(afr.sampleNo / 6) % 4];
  ShaderGlobal::set_float(gtao_temporal_directionsVarId, temporalRotation / 360);
  ShaderGlobal::set_float(gtao_temporal_offsetVarId, temporalOffset);

  IPoint2 sres = dynamic_resolution
                   ? calc_and_set_dynamic_resolution_stcode(*rawTex, *dynamic_resolution, prevDynRes.value_or(*dynamic_resolution))
                   : IPoint2(aoWidth, aoHeight);

  if (aoRendererCS)
  {
    d3d::set_rwtex(STAGE_CS, 0, rawTex, 0, 0);
    aoRendererCS->dispatchThreads(sres.x, sres.y, 1);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  }
  else
  {
    d3d::set_render_target({}, DepthAccess::RW, {{rawTex, 0, 0}});
    d3d::setviewscissor(0, 0, sres.x, sres.y);
    aoRenderer->render();
  }
}

void GTAORenderer::applySpatialFilter(BaseTexture *rawTex, BaseTexture *spatialTex, const DynRes *dynamic_resolution)
{
  ShaderGlobal::set_texture(raw_gtao_texVarId, rawTex);

  IPoint2 sres = dynamic_resolution
                   ? calc_and_set_dynamic_resolution_stcode(*spatialTex, *dynamic_resolution, prevDynRes.value_or(*dynamic_resolution))
                   : IPoint2(aoWidth, aoHeight);

  if (spatialFilterRendererCS)
  {
    d3d::set_rwtex(STAGE_CS, 0, spatialTex, 0, 0);
    spatialFilterRendererCS->dispatchThreads(sres.x, sres.y, 1);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  }
  else
  {
    d3d::set_render_target({}, DepthAccess::RW, {{spatialTex, 0, 0}});
    d3d::setviewscissor(0, 0, sres.x, sres.y);
    spatialFilterRenderer->render();
  }

  ShaderGlobal::set_texture(raw_gtao_texVarId, BAD_TEXTUREID);
}

void GTAORenderer::applyTemporalFilter(BaseTexture *rawTex, BaseTexture *prevGtaoTex, BaseTexture *spatialTex,
  const DynRes *dynamic_resolution)
{
  ShaderGlobal::set_texture(gtao_prev_texVarId, prevGtaoTex);
  ShaderGlobal::set_texture(gtao_texVarId, spatialTex);
  ShaderGlobal::set_texture(raw_gtao_texVarId, BAD_TEXTUREID);

  IPoint2 sres = dynamic_resolution
                   ? calc_and_set_dynamic_resolution_stcode(*rawTex, *dynamic_resolution, prevDynRes.value_or(*dynamic_resolution))
                   : IPoint2(aoWidth, aoHeight);

  if (temporalFilterRendererCS)
  {
    d3d::set_rwtex(STAGE_CS, 0, rawTex, 0, 0);
    temporalFilterRendererCS->dispatchThreads(sres.x, sres.y, 1);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  }
  else
  {
    d3d::set_render_target({}, DepthAccess::RW, {{rawTex, 0, 0}});
    d3d::setviewscissor(0, 0, sres.x, sres.y);
    temporalFilterRenderer->render();
  }

  ShaderGlobal::set_texture(gtao_prev_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(gtao_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(raw_gtao_texVarId, BAD_TEXTUREID);
}

void GTAORenderer::render(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *, BaseTexture *ssao_tex,
  BaseTexture *prev_ssao_tex, BaseTexture *tmp_tex, const DPoint3 *world_pos, SubFrameSample sub_sample,
  const DynRes *dynamic_resolution)
{
  renderGTAO(view_tm, proj_tm, ssao_tex, world_pos, dynamic_resolution);
  applyGTAOFilter(ssao_tex, prev_ssao_tex, tmp_tex, sub_sample, dynamic_resolution);
}

void GTAORenderer::renderGTAO(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *ssao_tex, const DPoint3 *world_pos,
  const DynRes *dynamic_resolution)
{
  // SSAO Renderer can work in two modes - using it's own textures or external ones. Mode is set in constructor.
  G_ASSERT(useOwnTextures || ssao_tex);
  TIME_D3D_PROFILE(GTAO_render);
  SCOPE_RENDER_TARGET;

  // Index setup for 3 textures, 3 pass rendering, while always keeping last result as history
  updateCurFrameIdxs();

  BaseTexture *rawTex = ssao_tex ? ssao_tex : gtaoTex[rawTargetIdx].getBaseTex();

  Afr &afr = afrs.current();
  if (useOwnReprojectionParams)
  {
    set_reprojection(view_tm, proj_tm, afr.prevProjTm, afr.prevWorldPos, afr.prevGlobTm, afr.prevViewVecLT, afr.prevViewVecRT,
      afr.prevViewVecLB, afr.prevViewVecRB, world_pos);
  }

  ShaderGlobal::set_texture(gtao_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(raw_gtao_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_float(gtao_half_hfov_tanVarId, 1.0f / max(proj_tm.m[0][0], 1e-6f));

  // GTAO uses local_view_N, which is initialized by setting view_tm
  d3d::settm(TM_VIEW, view_tm);

  renderGTAO(rawTex, dynamic_resolution);
}

void GTAORenderer::applyGTAOFilter(BaseTexture *ssao_tex, BaseTexture *prev_ssao_tex, BaseTexture *tmp_tex, SubFrameSample sub_sample,
  const DynRes *dynamic_resolution)
{
  // SSAO Renderer can work in two modes - using it's own textures or external ones. Mode is set in constructor.
  G_ASSERT(useOwnTextures || (ssao_tex && prev_ssao_tex && tmp_tex));
  SCOPE_RENDER_TARGET;

  BaseTexture *rawTex = ssao_tex ? ssao_tex : gtaoTex[rawTargetIdx].getBaseTex();
  BaseTexture *prevGtaoTex = prev_ssao_tex ? prev_ssao_tex : gtaoTex[historyIdx].getBaseTex();
  BaseTexture *spatialTex = tmp_tex ? tmp_tex : gtaoTex[spatialTargetIdx].getBaseTex();

  Afr &afr = afrs.current();
  d3d::resource_barrier({rawTex, RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});

  {
    TIME_D3D_PROFILE(GTAO_spatial);
    applySpatialFilter(rawTex, spatialTex, dynamic_resolution);
  }
  d3d::resource_barrier({spatialTex, RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});

  {
    TIME_D3D_PROFILE(GTAO_temporal);
    applyTemporalFilter(rawTex, prevGtaoTex, spatialTex, dynamic_resolution);
  }

  if (useOwnTextures)
  {
    // if rawTex is external, barrier for it will be set in framegraph automatically
    d3d::resource_barrier({rawTex, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
    ShaderGlobal::set_texture(ssao_texVarId, rawTex);
  }

  if (sub_sample == SubFrameSample::Single || sub_sample == SubFrameSample::Last)
    afr.sampleNo++;
}

GTAORenderer::~GTAORenderer()
{
  gtaoTex.forEach([&](ResizableTex &holder) { holder.close(); });
}

void GTAORenderer::reset() {}

Texture *GTAORenderer::getSSAOTex() { return gtaoTex[rawTargetIdx].getTex2D(); };

TEXTUREID GTAORenderer::getSSAOTexId() { return gtaoTex[rawTargetIdx].getTexId(); };
