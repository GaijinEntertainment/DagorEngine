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
#include <shaders/dag_shaders.h>
#include <util/dag_convar.h>
#include <render/set_reprojection.h>
#include <render/noiseTex.h>
#include <shaders/dag_computeShaders.h>
#include <EASTL/string.h>

#define GLOBAL_VARS_LIST            \
  VAR(ssr_target)                   \
  VAR(ssr_target_samplerstate)      \
  VAR(ssr_prev_target)              \
  VAR(ssr_prev_target_samplerstate) \
  VAR(ssr_target_size)              \
  VAR(ssr_frameNo)                  \
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
      a = dag::create_tex(NULL, ssrW, ssrH, fmt, mipCount, name);
      a.getTex2D()->texaddr(TEXADDR_CLAMP); // since we don't sample outside the ssr without weightening, there is no reason to have
                                            // border
      // moreover clamping helps in reprojection when weight is not zero
      a.getTex2D()->texbordercolor(0);
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
  ssrPrevTarget->getTex2D()->texaddr(TEXADDR_BORDER);
}

void ScreenSpaceReflections::render(const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 &world_pos,
  SubFrameSample sub_sample)
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
    rtTemp0->getTex2D()->texaddr(TEXADDR_BORDER);
    rtTemp0->getTex2D()->texbordercolor(0);

    targetTex = *rtTemp0;
  }

  render(view_tm, proj_tm, world_pos, sub_sample, ssrTex, prevTex, targetTex, afrs.current().frameNo);
}

void ScreenSpaceReflections::render(const TMatrix &view_tm, const TMatrix4 &proj_tm, const DPoint3 &world_pos,
  SubFrameSample sub_sample, ManagedTexView ssr_tex, ManagedTexView ssr_prev_tex, ManagedTexView target_tex, int callId)
{
  G_ASSERT(ssr_tex && ssr_prev_tex && target_tex);
  TIME_D3D_PROFILE(ssr);

  SCOPE_RENDER_TARGET;

  Afr &afr = afrs.current();
  set_reprojection(view_tm, proj_tm, afr.prevProjTm, afr.prevWorldPos, afr.prevGlobTm, afr.prevViewVecLT, afr.prevViewVecRT,
    afr.prevViewVecLB, afr.prevViewVecRB, &world_pos);

  ShaderGlobal::set_color4(ssr_target_sizeVarId, ssrW, ssrH, 1.0f / ssrW, 1.0f / ssrH);
  ShaderGlobal::set_color4(ssr_frameNoVarId, callId & ((1 << 23) - 1), (callId % randomizeOverNFrames) * 1551,
    (callId % randomizeOverNFrames), 0);

  ShaderGlobal::set_texture(ssr_prev_targetVarId, ssr_prev_tex);
  const int tileX = callId & 1, tileY = (callId / 2) & 1;
  ShaderGlobal::set_color4(ssr_tile_jitterVarId, tileX, tileY, 0, 0);

  if (quality != SSRQuality::Compute)
  {
    TIME_D3D_PROFILE(ssr_ps)
    d3d::set_render_target(target_tex.getTex2D(), 0);
    ssrQuad.render();
  }
  else
  {
    d3d::set_rwtex(STAGE_CS, 0, target_tex.getTex2D(), 0, 0);
    if (denoiser)
      ssrCompute->dispatchThreads(ssrW / 2, ssrH, 1);
    else
    {
      auto tsz = ssrCompute->getThreadGroupSizes();
      ssrCompute->dispatchThreads(ssrW + (tileX ? tsz[0] / 2 : 0), ssrH + (tileY ? tsz[1] / 2 : 0), 1);
    }
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  }

  if (!hasPrevTarget && ownTextures)
  {
    // this is the case when previous frame ssr is stored in ssr_tex, while the new one
    // is rendered in target_tex. It is happended where there is no separate previous frame texture
    d3d::resource_barrier({target_tex.getTex2D(), RB_RO_COPY_SOURCE, 0, 1});
    ssr_tex.getTex2D()->updateSubRegion(target_tex.getTex2D(), 0, 0, 0, 0, ssrW, ssrH, 1, 0, 0, 0, 0);
  }

  d3d::resource_barrier({ssr_tex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 1});
  ShaderGlobal::set_texture(ssr_targetVarId, ssr_tex);
#if SSR_MIPS > 1
  if (quality < SSRQuality::Compute)
  {
    for (int i = 1; i < SSR_MIPS; ++i)
    {
      char name[] = "SSR mip 0";
      name[8] += i;
      TIME_D3D_PROFILE_NAME(mip_levels, name);

      d3d::set_render_target(ssr_tex.getTex2D(), i);
      int w = ssrW >> i, h = ssrH >> i;
      ShaderGlobal::set_color4(ssr_target_sizeVarId, w, h, 1.0f / w, 1.0f / h);
      ssr_tex.getTex2D()->texmiplevel(i - 1, i - 1);
      ssrMipChain.render();
      d3d::resource_barrier({ssr_tex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, unsigned(i), 1});
    }
    ssr_tex.getTex2D()->texmiplevel(-1, -1);
  }
#endif

  ShaderGlobal::set_color4(ssr_target_sizeVarId, ssrW, ssrH, 1.0f / ssrW, 1.0f / ssrH);
  if (sub_sample == SubFrameSample::Single || sub_sample == SubFrameSample::Last)
    afr.frameNo++;
}

void ScreenSpaceReflections::setCurrentView(int index)
{
  afrs.setCurrentView(index);
  ssrTarget.setCurrentView(index);
  ssrPrevTarget.setCurrentView(index);
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
    a = dag::create_tex(NULL, ssrW, ssrH, flags, mipCount, name.c_str());
    a.getTex2D()->texaddr(TEXADDR_CLAMP); // since we don't sample outside the ssr without weightening, there is no reason to have
                                          // border
    // moreover clamping helps in reprojection when weight is not zero
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
