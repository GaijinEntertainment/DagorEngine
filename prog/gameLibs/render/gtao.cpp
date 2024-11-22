// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/gtao.h>
#include <render/viewVecs.h>
#include <render/set_reprojection.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaders.h>

#define GLOBAL_VARS_LIST          \
  VAR(gtao_tex_size)              \
  VAR(gtao_temporal_directions)   \
  VAR(gtao_temporal_offset)       \
  VAR(gtao_half_hfov_tan)         \
  VAR(raw_gtao_tex)               \
  VAR(raw_gtao_tex_samplerstate)  \
  VAR(gtao_prev_tex)              \
  VAR(gtao_prev_tex_samplerstate) \
  VAR(gtao_tex)                   \
  VAR(gtao_tex_samplerstate)      \
  VAR(ssao_tex)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR


GTAORenderer::GTAORenderer(int w, int h, int views, uint32_t flags, bool use_own_textures) :
  even{false}, rawTargetIdx{0}, spatialTargetIdx{0}, temporalTargetIdx{0}, historyIdx{0}
{

  aoWidth = w;
  aoHeight = h;
  useOwnTextures = use_own_textures;

  ShaderGlobal::set_int(gtao_tex_sizeVarId, min(w, h));

  uint32_t cflags = ssao_detail::creation_flags_to_format(ssao_detail::consider_shader_assumes(flags));

  if (use_own_textures && d3d::should_use_compute_for_image_processing({cflags}))
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
      a = dag::create_tex(NULL, aoWidth, aoHeight, cflags, 1, name);
      a.getTex2D()->texbordercolor(0xFFFFFFFF);
      a.getTex2D()->texaddr(TEXADDR_CLAMP);
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
    ShaderGlobal::set_sampler(gtao_tex_samplerstateVarId, sampler);
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
    gtaoTex[rawTargetIdx].getTex2D()->texbordercolor(0xFFFFFFFF);
    gtaoTex[rawTargetIdx].getTex2D()->texaddr(TEXADDR_CLAMP);
    gtaoTex[spatialTargetIdx].resize(w, h);
    gtaoTex[spatialTargetIdx].getTex2D()->texbordercolor(0xFFFFFFFF);
    gtaoTex[spatialTargetIdx].getTex2D()->texaddr(TEXADDR_CLAMP);
  }
  // temporalTargetIdx == rawTargetIdx, so already resized

  aoWidth = w;
  aoHeight = h;
  ShaderGlobal::set_int(gtao_tex_sizeVarId, min(w, h));
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

void GTAORenderer::renderGTAO(const ManagedTex &rawTex)
{
  Afr &afr = afrs.current();

  TIME_D3D_PROFILE(GTAO_main);
  const float temporalRotation = TEMPORAL_ROTATIONS[afr.sampleNo % 6];
  const float temporalOffset = TEMPORAL_OFFSETS[(afr.sampleNo / 6) % 4];
  ShaderGlobal::set_real(gtao_temporal_directionsVarId, temporalRotation / 360);
  ShaderGlobal::set_real(gtao_temporal_offsetVarId, temporalOffset);
  if (aoRendererCS)
  {
    d3d::set_rwtex(STAGE_CS, 0, rawTex.getTex2D(), 0, 0);
    aoRendererCS->dispatchThreads(aoWidth, aoHeight, 1);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  }
  else
  {
    d3d::set_render_target(rawTex.getTex2D(), 0);
    aoRenderer->render();
  }
}

void GTAORenderer::applySpatialFilter(const ManagedTex &rawTex, const ManagedTex &spatialTex)
{
  ShaderGlobal::set_texture(raw_gtao_texVarId, rawTex);

  if (spatialFilterRendererCS)
  {
    d3d::set_rwtex(STAGE_CS, 0, spatialTex.getTex2D(), 0, 0);
    spatialFilterRendererCS->dispatchThreads(aoWidth, aoHeight, 1);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  }
  else
  {
    d3d::set_render_target(spatialTex.getTex2D(), 0);
    spatialFilterRenderer->render();
  }
}

void GTAORenderer::applyTemporalFilter(const ManagedTex &rawTex, const ManagedTex &prevGtaoTex, const ManagedTex &spatialTex)
{
  ShaderGlobal::set_texture(gtao_prev_texVarId, prevGtaoTex);
  ShaderGlobal::set_texture(gtao_texVarId, spatialTex);
  ShaderGlobal::set_texture(raw_gtao_texVarId, BAD_TEXTUREID);

  if (temporalFilterRendererCS)
  {
    d3d::set_rwtex(STAGE_CS, 0, rawTex.getTex2D(), 0, 0);
    temporalFilterRendererCS->dispatchThreads(aoWidth, aoHeight, 1);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  }
  else
  {
    d3d::set_render_target(rawTex.getTex2D(), 0);
    temporalFilterRenderer->render();
  }
}

void GTAORenderer::render(const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *, const ManagedTex *ssao_tex,
  const ManagedTex *prev_ssao_tex, const ManagedTex *tmp_tex, const DPoint3 *world_pos, SubFrameSample sub_sample)
{
  // SSAO Renderer can work in two modes - using it's own textures or external ones. Mode is set in constructor.
  G_ASSERT(useOwnTextures || (ssao_tex && prev_ssao_tex && tmp_tex));
  TIME_D3D_PROFILE(GTAO_render);
  SCOPE_RENDER_TARGET;

  // Index setup for 3 textures, 3 pass rendering, while always keeping last result as history
  updateCurFrameIdxs();

  const ManagedTex &rawTex = ssao_tex ? *ssao_tex : gtaoTex[rawTargetIdx];
  const ManagedTex &prevGtaoTex = prev_ssao_tex ? *prev_ssao_tex : gtaoTex[historyIdx];
  const ManagedTex &spatialTex = tmp_tex ? *tmp_tex : gtaoTex[spatialTargetIdx];

  Afr &afr = afrs.current();
  set_reprojection(view_tm, proj_tm, afr.prevProjTm, afr.prevWorldPos, afr.prevGlobTm, afr.prevViewVecLT, afr.prevViewVecRT,
    afr.prevViewVecLB, afr.prevViewVecRB, world_pos);

  ShaderGlobal::set_texture(gtao_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(raw_gtao_texVarId, BAD_TEXTUREID);
  ShaderGlobal::set_real(gtao_half_hfov_tanVarId, 1.0f / max(proj_tm.m[0][0], 1e-6f));

  renderGTAO(rawTex);

  d3d::resource_barrier({rawTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});

  {
    TIME_D3D_PROFILE(GTAO_spatial);
    applySpatialFilter(rawTex, spatialTex);
  }
  d3d::resource_barrier({spatialTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 1});

  {
    TIME_D3D_PROFILE(GTAO_temporal);
    applyTemporalFilter(rawTex, prevGtaoTex, spatialTex);
  }

  // if rawTex is external, barrier for it will be set in framegraph automatically
  if (useOwnTextures)
    d3d::resource_barrier({rawTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});

  ShaderGlobal::set_texture(ssao_texVarId, rawTex);
  if (sub_sample == SubFrameSample::Single || sub_sample == SubFrameSample::Last)
    afr.sampleNo++;
}

GTAORenderer::~GTAORenderer()
{
  gtaoTex.forEach([&](ResizableTex &holder) { holder.close(); });
};

void GTAORenderer::reset(){

};

Texture *GTAORenderer::getSSAOTex() { return gtaoTex[rawTargetIdx].getTex2D(); };

TEXTUREID GTAORenderer::getSSAOTexId() { return gtaoTex[rawTargetIdx].getTexId(); };
