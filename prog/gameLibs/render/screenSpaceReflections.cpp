// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/screenSpaceReflections.h>
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
#include <EASTL/string.h>
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

int ScreenSpaceReflections::getMipCount(SSRQuality ssr_quality) { return (ssr_quality >= SSRQuality::Compute) ? 1 : SSR_MIPS; }

void ScreenSpaceReflections::getRealQualityAndFmt(uint32_t &fmt, SSRQuality &ssr_quality)
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

ScreenSpaceReflections::ScreenSpaceReflections(const int ssr_w, const int ssr_h, const int num_views, uint32_t fmt,
  SSRQuality ssr_quality, SSRFlags flags) :
  denoiser(!!(flags & SSRFlag::ExternalDenoiser))
{
  // RESOLVE RESOLUTION
  ssrW = ssr_w;
  ssrH = ssr_h;

#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR
  init_and_get_blue_noise();

  getRealQualityAndFmt(fmt, ssr_quality);
  quality = ssr_quality;
  ownTextures = !!(flags & SSRFlag::CreateTextures);
  if (ownTextures)
  {
    fmt |= TEXCF_CLEAR_ON_CREATE;
    ssrTarget.forTheFirstN(num_views, [&](auto &a, int view) {
      String name(128, "ssr_target_%d", view);
      int mipCount = getMipCount(ssr_quality);
      a = dag::create_tex(NULL, ssrW, ssrH, fmt, mipCount, name, RESTAG_SSR);
      d3d::resource_barrier({a.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    });

    rtTemp = RTargetPool::get(ssrW, ssrH, fmt, 1);
  }

  afrs.forEach([](Afr &afr) {
    memset(&afr, 0, sizeof(afr));
    afr.prevGlobTm.identity();
  });
  randomizeOverNFrames = 16;
  if (quality != SSRQuality::Compute)
  {
    ssrMipChain.init("ssr_mipchain");
    ssrQuad.init("ssr");
  }
  else
    ssrCompute.reset(new_compute_shader("tile_ssr_compute"));

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
  updateSamplers();
}

ScreenSpaceReflections::~ScreenSpaceReflections()
{

  ssrQuad.clear();
  ssrMipChain.clear();
  ssrTarget.forEach([](auto &t) { t.close(); });
  ssrPrevTarget.forEach([](auto &t) { t.close(); });
  rtTemp.reset();
  rtTempLow.reset();

  ssrCompute.reset();

  release_blue_noise();

  ShaderGlobal::reset_textures(true);
}

void ScreenSpaceReflections::updatePrevTarget()
{
  if (!hasPrevTarget || !ownTextures)
    return;

  d3d::resource_barrier({ssrTarget->getTex2D(), RB_RO_COPY_SOURCE, 0, 1});
  ssrPrevTarget->getTex2D()->updateSubRegion(ssrTarget->getTex2D(), 0, 0, 0, 0, ssrW, ssrH, 1, 0, 0, 0, 0);
}

void ScreenSpaceReflections::setHistoryValid(bool valid) { isHistoryValid = valid; }

void ScreenSpaceReflections::render(const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 &world_pos,
  SubFrameSample sub_sample, const DynRes *dynamic_resolution)
{
  G_ASSERT(ownTextures);

  if (!ssrTarget->getTex2D())
    return;
  if (hasPrevTarget && !ssrPrevTarget->getTex2D())
    return;
  ManagedTexView prevTex = hasPrevTarget ? ssrPrevTarget.current() : ssrTarget.current();
  ManagedTexView ssrTex = ssrTarget.current();
  ManagedTexView targetTex;
  RTarget::Ptr rtTemp0 = hasPrevTarget ? nullptr : rtTemp->acquire();

  if (hasPrevTarget)
  {
    targetTex = ssrTarget.current();
  }
  else
  {
    targetTex = *rtTemp0;
  }

  render(view_tm, proj_tm, world_pos, sub_sample, ssrTex.getTex2D(), prevTex.getTex2D(), targetTex.getTex2D(), afrs.current().frameNo,
    dynamic_resolution);
}

void ScreenSpaceReflections::render(const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 &world_pos,
  SubFrameSample sub_sample, BaseTexture *ssr_tex, BaseTexture *ssr_prev_tex, BaseTexture *target_tex, int callId,
  const DynRes *dynamic_resolution)
{
  G_ASSERT(ssr_tex && ssr_prev_tex && target_tex);
  TIME_D3D_PROFILE(ssr);

  SCOPE_RENDER_TARGET;

  const int prevIgnoreHistory = ShaderGlobal::get_int(force_ignore_historyVarId);
  STATE_GUARD(ShaderGlobal::set_int(force_ignore_historyVarId, VALUE), !isHistoryValid || prevIgnoreHistory, prevIgnoreHistory);
  Afr &afr = afrs.current();
  set_reprojection(view_tm, proj_tm, afr.prevProjTm, afr.prevWorldPos, afr.prevGlobTm, afr.prevViewVecLT, afr.prevViewVecRT,
    afr.prevViewVecLB, afr.prevViewVecRB, &world_pos);

  TextureInfo info;
  target_tex->getinfo(info, 0);

  int dw = ssrW;
  int dh = ssrH;
  if (dynamic_resolution)
  {
    auto dd = calc_and_set_dynamic_resolution_stcode(ssrW, ssrH, *dynamic_resolution, prevDynRes.value_or(*dynamic_resolution));
    dw = dd.x;
    dh = dd.y;
  }

  ShaderGlobal::set_float4(ssr_target_sizeVarId, dw, dh, 1.0f / dw, 1.0f / dh);
  ShaderGlobal::set_float4(ssr_frameNoVarId, callId & ((1 << 23) - 1), (callId % randomizeOverNFrames) * 1551,
    (callId % randomizeOverNFrames), info.mipLevels);

  ShaderGlobal::set_texture(ssr_prev_targetVarId, ssr_prev_tex);
  const int tileX = callId & 1, tileY = (callId / 2) & 1;
  ShaderGlobal::set_float4(ssr_tile_jitterVarId, tileX, tileY, 0, 0);

  if (quality != SSRQuality::Compute)
  {
    TIME_D3D_PROFILE(ssr_ps)
    d3d::set_render_target(target_tex, 0);
    d3d::setviewscissor(0, 0, dw, dh);
    ssrQuad.render();
  }
  else
  {
    d3d::set_rwtex(STAGE_CS, 0, target_tex, 0, 0);
    if (denoiser)
    {
      G_ASSERTF_ONCE(!dynamic_resolution, "Handle dynamic resolution for non compute, denoised ssr!");
      ssrCompute->dispatchThreads(ssrW / 2, ssrH, 1);
    }
    else
    {
      auto tsz = ssrCompute->getThreadGroupSizes();
      dw += (tileX ? tsz[0] / 2 : 0);
      dh += (tileY ? tsz[1] / 2 : 0);
      ssrCompute->dispatchThreads(dw, dh, 1);
    }
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  }

  if (!hasPrevTarget && ownTextures)
  {
    // this is the case when previous frame ssr is stored in ssr_tex, while the new one
    // is rendered in target_tex. It is happended where there is no separate previous frame texture
    d3d::resource_barrier({target_tex, RB_RO_COPY_SOURCE, 0, 1});
    ssr_tex->updateSubRegion(target_tex, 0, 0, 0, 0, ssrW, ssrH, 1, 0, 0, 0, 0);
  }

  d3d::resource_barrier({ssr_tex, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 1});
  ShaderGlobal::set_texture(ssr_targetVarId, ssr_tex);
#if SSR_MIPS > 1
  if (quality < SSRQuality::Compute)
  {
    for (int i = 1; i < SSR_MIPS; ++i)
    {
      char name[] = "SSR mip 0";
      name[8] += i;
      TIME_D3D_PROFILE_NAME(mip_levels, name);

      d3d::set_render_target(ssr_tex, i);
      int w = ssrW >> i, h = ssrH >> i;
      ShaderGlobal::set_float4(ssr_target_sizeVarId, w, h, 1.0f / w, 1.0f / h);
      ssr_tex->texmiplevel(i - 1, i - 1);
      ssrMipChain.render();
      d3d::resource_barrier({ssr_tex, RB_RO_SRV | RB_STAGE_PIXEL, unsigned(i), 1});
    }
    ssr_tex->texmiplevel(-1, -1);
  }
#endif

  ShaderGlobal::set_float4(ssr_target_sizeVarId, ssrW, ssrH, 1.0f / ssrW, 1.0f / ssrH);
  if (sub_sample == SubFrameSample::Single || sub_sample == SubFrameSample::Last)
    afr.frameNo++;

  ShaderGlobal::set_texture(ssr_prev_targetVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(ssr_targetVarId, BAD_TEXTUREID);

  if (dynamic_resolution)
    prevDynRes = *dynamic_resolution;
}

void ScreenSpaceReflections::setCurrentView(int index)
{
  afrs.setCurrentView(index);
  ssrTarget.setCurrentView(index);
  ssrPrevTarget.setCurrentView(index);
  ShaderGlobal::set_texture(ssr_targetVarId, ssrTarget.current());
}

void ScreenSpaceReflections::enablePrevTarget(int num_views)
{
  if (hasPrevTarget || !ownTextures)
    return;

  hasPrevTarget = true;
  updateSamplers();

  uint32_t flags = TEXCF_SRGBREAD | TEXCF_SRGBWRITE;
  if (ssrflags & TEXFMT_A16B16G16R16F)
    flags |= TEXFMT_A16B16G16R16F;

  ssrPrevTarget.forTheFirstN(num_views, [&](auto &a, int view) {
    eastl::string name(eastl::string::CtorSprintf(), "ssr_prev_target_%d", view);
    int mipCount = 1;
    a = dag::create_tex(NULL, ssrW, ssrH, flags, mipCount, name.c_str(), RESTAG_SSR);
  });
  updatePrevTarget();
}

void ScreenSpaceReflections::disablePrevTarget()
{
  if (!hasPrevTarget)
    return;
  hasPrevTarget = false;
  updateSamplers();

  ssrPrevTarget.forEach([](auto &t) { t.close(); });
}

void ScreenSpaceReflections::changeDynamicResolution(int new_width, int new_height)
{
  ssrW = new_width;
  ssrH = new_height;
}

void ScreenSpaceReflections::updateSamplers() const
{
  d3d::SamplerInfo smpInfo;
  if (!hasPrevTarget && ownTextures)
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
  else
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
  ShaderGlobal::set_sampler(ssr_target_samplerstateVarId, d3d::request_sampler(smpInfo));
}
