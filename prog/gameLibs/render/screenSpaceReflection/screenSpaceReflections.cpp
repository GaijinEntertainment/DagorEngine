// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "screenSpaceReflections_api.h"
#include <perfMon/dag_statDrv.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_viewScissor.h>
#include <shaders/dag_shaders.h>
#include <util/dag_convar.h>
#include <render/set_reprojection.h>
#include <render/noiseTex.h>
#include <shaders/dag_computeShaders.h>
#include <math/integer/dag_IPoint2.h>

#define GLOBAL_VARS_LIST            \
  VAR(ssr_target)                   \
  VAR(ssr_target_samplerstate)      \
  VAR(ssr_prev_target)              \
  VAR(ssr_prev_target_samplerstate) \
  VAR(ssr_target_size)              \
  VAR(ssr_frameNo)                  \
  VAR(force_ignore_history)         \
  VAR(ssr_tile_jitter)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

#define SSR_MIPS 4

static constexpr int32_t ssr_randomize_over_n_frames = 16;

namespace ssr_pass
{

int32_t get_mip_count(SSRQuality quality) { return (quality >= SSRQuality::Compute) ? 1 : SSR_MIPS; }

void get_real_quality_and_fmt(uint32_t &fmt, SSRQuality &ssr_quality)
{
  if (!fmt)
    fmt = TEXCF_SRGBREAD | TEXCF_SRGBWRITE; // replace with 16bit float?

  if (ssr_quality == SSRQuality::Compute)
  {
    if (d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & d3d::USAGE_PIXREADWRITE)
      fmt = TEXFMT_A16B16G16R16F | TEXCF_UNORDERED;
    else
      ssr_quality = SSRQuality::Low;
  }

  if (ssr_quality < SSRQuality::Compute)
    fmt |= TEXCF_RTARGET; // for mips
}

RenderResources create_render_resources(SSRQuality quality)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR

  RenderResources res;
  res.blueNoise.acquire();

  if (quality != SSRQuality::Compute)
  {
    res.mipChain.init("ssr_mipchain");
    res.quad.init("ssr");
  }
  else
    res.compute.reset(new_compute_shader("tile_ssr_compute"));

  {
    d3d::SamplerInfo smpInfo;
    // since we don't sample outside the ssr without weightening in reprojection
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    if (quality >= SSRQuality::Compute)
      smpInfo.filter_mode = d3d::FilterMode::Point;
    ShaderGlobal::set_sampler(ssr_prev_target_samplerstateVarId, d3d::request_sampler(smpInfo));
  }
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(ssr_target_samplerstateVarId, d3d::request_sampler(smpInfo));
  }

  return res;
}

static TMatrix4 calc_glob_tm_no_ofs(const TMatrix &view_tm, const TMatrix4 &proj_tm)
{
  TMatrix4 viewRot = TMatrix4(view_tm);
  viewRot.setrow(3, 0.f, 0.f, 0.f, 1.f);
  return viewRot * proj_tm;
}

void apply_reprojection(const CameraState &cur, const CameraState &prev)
{
  set_reprojection(cur.projTm, calc_glob_tm_no_ofs(cur.viewTm, cur.projTm), prev.projTm, calc_glob_tm_no_ofs(prev.viewTm, prev.projTm),
    cur.viewVecs.viewVecLT, cur.viewVecs.viewVecRT, cur.viewVecs.viewVecLB, cur.viewVecs.viewVecRB, prev.viewVecs.viewVecLT,
    prev.viewVecs.viewVecRT, prev.viewVecs.viewVecLB, prev.viewVecs.viewVecRB, cur.worldPos, prev.worldPos);
}

void trace(const RenderResources &res, const TraceParams &params, const CameraState &cur, const CameraState &prev)
{
  TIME_D3D_PROFILE(ssr);

  const int32_t prevIgnoreHistory = ShaderGlobal::get_int(force_ignore_historyVarId);
  STATE_GUARD(ShaderGlobal::set_int(force_ignore_historyVarId, VALUE), !params.isHistoryValid || prevIgnoreHistory, prevIgnoreHistory);

  apply_reprojection(cur, prev);

  int32_t dw = params.width;
  int32_t dh = params.height;

  ShaderGlobal::set_float4(ssr_target_sizeVarId, dw, dh, 1.0f / dw, 1.0f / dh);
  ShaderGlobal::set_float4(ssr_frameNoVarId, params.callId & ((1 << 23) - 1), (params.callId % ssr_randomize_over_n_frames) * 1551,
    (params.callId % ssr_randomize_over_n_frames), params.targetMipCount);

  const int32_t tileX = params.callId & 1, tileY = (params.callId / 2) & 1;
  ShaderGlobal::set_float4(ssr_tile_jitterVarId, tileX, tileY, 0, 0);

  if (!res.compute)
  {
    TIME_D3D_PROFILE(ssr_ps)
    res.quad.render();
  }
  else
  {
    auto tsz = res.compute->getThreadGroupSizes();
    dw += (tileX ? tsz[0] / 2 : 0);
    dh += (tileY ? tsz[1] / 2 : 0);
    res.compute->dispatchThreads(dw, dh, 1);
  }
}

void build_mip_chain(const RenderResources &res, BaseTexture *ssr_tex, int32_t width, int32_t height)
{
  G_ASSERT(ssr_tex);

  d3d::resource_barrier({ssr_tex, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 1});

  if (res.compute)
    return;

  SCOPE_RENDER_TARGET;
  ShaderGlobal::set_texture(ssr_targetVarId, ssr_tex);

  char name[] = "SSR mip 0";
  for (int32_t i = 1; i < SSR_MIPS; ++i)
  {
    ++name[8];
    TIME_D3D_PROFILE_NAME(mip_levels, name);

    d3d::set_render_target({}, DepthAccess::RW, {{ssr_tex, static_cast<uint32_t>(i), 0}});
    const int32_t w = width >> i, h = height >> i;
    ShaderGlobal::set_float4(ssr_target_sizeVarId, w, h, 1.0f / w, 1.0f / h);
    ssr_tex->texmiplevel(i - 1, i - 1);
    res.mipChain.render();
    d3d::resource_barrier({ssr_tex, RB_RO_SRV | RB_STAGE_PIXEL, unsigned(i), 1});
  }
  ssr_tex->texmiplevel(-1, -1);

  ShaderGlobal::set_float4(ssr_target_sizeVarId, width, height, 1.0f / width, 1.0f / height);
  ShaderGlobal::set_texture(ssr_targetVarId, BAD_TEXTUREID);
}

} // namespace ssr_pass

ScreenSpaceReflections::ScreenSpaceReflections(const int ssr_w, const int ssr_h, const int num_views, uint32_t fmt,
  SSRQuality ssr_quality, bool own_textures, const char *tag)
{
  ssrW = ssr_w;
  ssrH = ssr_h;

  ssr_pass::get_real_quality_and_fmt(fmt, ssr_quality);
  ownTextures = own_textures;
  const char *tagSuffix = (tag && *tag) ? tag : "";
  if (ownTextures)
  {
    fmt |= TEXCF_CLEAR_ON_CREATE;
    ssrTarget.forTheFirstN(num_views, [&](auto &a, int view) {
      String name(128, "ssr_target_%d%s", view, tagSuffix);
      int mipCount = ssr_pass::get_mip_count(ssr_quality);
      a = dag::create_tex(NULL, ssrW, ssrH, fmt, mipCount, name, RESTAG_SSR);
      d3d::resource_barrier({a.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    });

    rtTemp = RTargetPool::get(ssrW, ssrH, fmt, 1);
  }

  resources = ssr_pass::create_render_resources(ssr_quality);
  updateSamplers();
}

ScreenSpaceReflections::~ScreenSpaceReflections()
{
  resources = {};

  ssrTarget.forEach([](auto &t) { t.close(); });
  rtTemp.reset();

  ShaderGlobal::reset_textures(true);
}

void ScreenSpaceReflections::render(const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 &world_pos,
  SubFrameSample sub_sample)
{
  G_ASSERT(ownTextures);

  if (!ssrTarget->getTex2D())
    return;

  ManagedTexView ssrTex = ssrTarget.current();
  RTarget::Ptr rtTemp0 = rtTemp->acquire();
  ManagedTexView targetTex = *rtTemp0;

  render(view_tm, proj_tm, world_pos, sub_sample, ssrTex.getTex2D(), ssrTex.getTex2D(), targetTex.getTex2D(),
    viewSpecific.current().frameNo);
}

void ScreenSpaceReflections::render(const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 &world_pos,
  SubFrameSample sub_sample, BaseTexture *ssr_tex, BaseTexture *ssr_prev_tex, BaseTexture *target_tex, int callId)
{
  G_ASSERT(ssr_tex && ssr_prev_tex && target_tex);
  SCOPE_RENDER_TARGET;

  ViewSpecific &view = viewSpecific.current();
  TextureInfo info;
  target_tex->getinfo(info, 0);

  const ssr_pass::TraceParams params = {.width = ssrW, .height = ssrH, .targetMipCount = info.mipLevels, .callId = callId};
  const ssr_pass::CameraState camera = {view_tm, proj_tm, world_pos, calc_view_vecs(view_tm, proj_tm)};

  if (resources.compute)
    d3d::set_rwtex(STAGE_CS, 0, target_tex, 0, 0);
  else
  {
    d3d::set_render_target({}, DepthAccess::RW, {{target_tex, 0, 0}});
    d3d::setviewscissor(0, 0, ssrW, ssrH);
  }

  ShaderGlobal::set_texture(ssr_prev_targetVarId, ssr_prev_tex);
  ssr_pass::trace(resources, params, camera, view.prevCamera);
  ShaderGlobal::set_texture(ssr_prev_targetVarId, BAD_TEXTUREID);
  view.prevCamera = camera;

  if (resources.compute)
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);

  if (ownTextures)
  {
    // this is the case when previous frame ssr is stored in ssr_tex, while the new one
    // is rendered in target_tex. It is happended where there is no separate previous frame texture
    d3d::resource_barrier({target_tex, RB_RO_COPY_SOURCE, 0, 1});
    d3d::update_sub_region(target_tex, 0, 0, 0, 0, ssrW, ssrH, 1, ssr_tex, 0, 0, 0, 0);
  }

  ssr_pass::build_mip_chain(resources, ssr_tex, ssrW, ssrH);

  if (sub_sample == SubFrameSample::Single || sub_sample == SubFrameSample::Last)
    view.frameNo++;
}

void ScreenSpaceReflections::updateSamplers() const
{
  d3d::SamplerInfo smpInfo;
  if (ownTextures)
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
  else
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
  ShaderGlobal::set_sampler(ssr_target_samplerstateVarId, d3d::request_sampler(smpInfo));
}
