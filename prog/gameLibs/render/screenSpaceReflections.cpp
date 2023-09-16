#include <render/screenSpaceReflections.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_tex3d.h>
#include <shaders/dag_shaders.h>
#include <3d/dag_drv3dCmd.h>
#include <util/dag_convar.h>
#include <render/set_reprojection.h>
#include <render/noiseTex.h>
#include <shaders/dag_computeShaders.h>
#include <EASTL/string.h>

#define GLOBAL_VARS_LIST          \
  VAR(ssr_target)                 \
  VAR(ssr_prev_target)            \
  VAR(ssr_target_size)            \
  VAR(ssr_trace_size)             \
  VAR(ssr_tiles_size)             \
  VAR(ssr_frameNo)                \
  VAR(ssr_filter0)                \
  VAR(ssr_filter1)                \
  VAR(ssr_filter2)                \
  VAR(ssr_strength)               \
  VAR(vtr_target_size)            \
  VAR(vtr_target)                 \
  VAR(ssr_tile_jitter)            \
  VAR(ssr_reflection_tex)         \
  VAR(ssr_raylen_tex)             \
  VAR(ssr_reflection_history_tex) \
  VAR(ssr_variance_tex)           \
  VAR(ssr_sample_count_tex)       \
  VAR(ssr_avg_reflection_tex)     \
  VAR(ssr_reproj_reflection_tex)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

#define SSR_MIPS 4

static float blackman_harris(const Point2 &uv) { return expf(-2.29f * lengthSq(uv)); }

int ScreenSpaceReflections::getMipCount(SSRQuality ssr_quality) { return (ssr_quality >= SSRQuality::Compute) ? 1 : SSR_MIPS; }

void ScreenSpaceReflections::getRealQualityAndFmt(uint32_t &fmt, SSRQuality &ssr_quality)
{
  if (!fmt)
    fmt = TEXCF_SRGBREAD | TEXCF_SRGBWRITE; // replace with 16bit float?

  bool noHQCompute = d3d::get_driver_code() != _MAKE4C('DX12') && ssr_quality == SSRQuality::ComputeHQ;
#if _TARGET_XBOX
  noHQCompute = true;
#endif
  if (noHQCompute)
  {
    ssr_quality = SSRQuality::Compute;
  }

  if (ssr_quality == SSRQuality::Compute)
  {
    if ((d3d::get_texformat_usage(TEXFMT_A16B16G16R16F) & d3d::USAGE_PIXREADWRITE) &&
        d3d::get_driver_desc().caps.hasConservativeRassterization) // actually we check dx 11.3 support, not the conservative
                                                                   // rasterization
      fmt = TEXFMT_A16B16G16R16F | TEXCF_UNORDERED;
    else
      ssr_quality = SSRQuality::Low;
  }
  else if (ssr_quality == SSRQuality::ComputeHQ)
  {
    fmt = TEXFMT_A16B16G16R16F | TEXCF_UNORDERED;
  }

  if (ssr_quality < SSRQuality::Compute)
    fmt |= TEXCF_RTARGET; // for mips
}
ScreenSpaceReflections::ScreenSpaceReflections(const int ssr_w, const int ssr_h, const int num_views, uint32_t fmt,
  SSRQuality ssr_quality, bool own_textures)
{
  // RESOLVE RESOLUTION
  ssrW = ssr_w;
  ssrH = ssr_h;

#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_LIST
#undef VAR

  getRealQualityAndFmt(fmt, ssr_quality);
  quality = ssr_quality;
  ownTextures = own_textures;
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
  ssrMipChain.init("ssr_mipchain", NULL, true);
  randomizeOverNFrames = 8;
  if (quality == SSRQuality::High)
  {
    ssrQuad.init("ssr_only", NULL, true);
    ssrResolve.init("ssr_resolve", NULL, true);
    ssrVTR.init("ssr_vtr", NULL, true);
    ssrComposite.init("ssr_composite", NULL, true);

    if (ownTextures)
      ssrVTRRT = RTargetPool::get(ssrW / 2, ssrH / 2, fmt, 1);

    const size_t FILTER_WEIGHT_COUNT = 9;
    Point2 filterWeights[FILTER_WEIGHT_COUNT] = {Point2{0.0f, 0.0f}, Point2{-1.0f, 0.0f}, Point2{0.0f, -1.0f}, Point2{1.0f, 0.0f},
      Point2{0.0f, 1.0f}, Point2{-1.0f, -1.0f}, Point2{1.0f, -1.0f}, Point2{-1.0f, 1.0f}, Point2{1.0f, 1.0f}};

    float filter[FILTER_WEIGHT_COUNT];
    for (size_t i = 0; i < FILTER_WEIGHT_COUNT; ++i)
      filter[i] = blackman_harris(filterWeights[i]);

    ShaderGlobal::set_color4(ssr_filter0VarId, filter[0], filter[1], filter[2], filter[3]);
    ShaderGlobal::set_color4(ssr_filter1VarId, filter[4], filter[5], filter[6], filter[7]);
    ShaderGlobal::set_real(ssr_filter2VarId, filter[8]);

    ShaderGlobal::set_color4(vtr_target_sizeVarId, ssrW >> 1, ssrH >> 1, 0, 0);
  }
  else
  {
    ssrQuad.init("ssr", NULL, true);
  }

  ssrCompute.reset(new_compute_shader("tile_ssr_compute"));

  if (d3d::get_driver_code() == _MAKE4C('DX12') && quality == SSRQuality::ComputeHQ)
  {
    ssrClassify.reset(new_compute_shader("ssr_classify"));
    ssrPrepareIndirectArgs.reset(new_compute_shader("ssr_prepare_indirect_args"));
    ssrIntersect.reset(new_compute_shader("ssr_intersect"));
    ssrReproject.reset(new_compute_shader("ssr_reproject"));
    ssrPrefilter.reset(new_compute_shader("ssr_prefilter"));
    ssrTemporal.reset(new_compute_shader("ssr_temporal"));

    if (ownTextures)
      rtTempR16F = RTargetPool::get(ssrW, ssrH, TEXFMT_R16F | TEXCF_UNORDERED, 1);

    const auto grpSizes = ssrClassify->getThreadGroupSizes();

    rayListBuf = dag::buffers::create_ua_sr_structured(sizeof(uint32_t), ssrW * ssrH, "ssr_ray_list");
    tileListBuf = dag::buffers::create_ua_sr_structured(sizeof(uint32_t),
      ((ssrW + grpSizes[0] - 1) / grpSizes[0]) * ((ssrH + grpSizes[1] - 1) / grpSizes[1]), "ssr_tile_list");
    countersBuf = dag::buffers::create_ua_sr_structured(sizeof(uint32_t), 4, "ssr_counters", dag::buffers::Init::Zero);

    indirectArgsBuf = dag::create_sbuffer(sizeof(uint32_t), 6, SBCF_UA_INDIRECT, 0, "ssr_indirect_args");

    init_and_get_blue_noise();

    rtTempLow = RTargetPool::get((ssrW + grpSizes[0] - 1) / grpSizes[0], (ssrH + grpSizes[1] - 1) / grpSizes[1], fmt, 1);

    for (int i = 0; i < 2; ++i)
    {
      char name[256];
      _snprintf(name, sizeof(name), "ssr_variance_tex_%d", i);
      varianceTex[i] = dag::create_tex(nullptr, ssrW, ssrH, TEXFMT_R8 | TEXCF_UNORDERED | TEXCF_CLEAR_ON_CREATE, 1, name);
      _snprintf(name, sizeof(name), "ssr_sample_count_tex_%d", i);
      sampleCountTex[i] = dag::create_tex(nullptr, ssrW, ssrH, TEXFMT_R8 | TEXCF_UNORDERED | TEXCF_CLEAR_ON_CREATE, 1, name);
    }
  }
}

ScreenSpaceReflections::~ScreenSpaceReflections()
{
  ssrResolve.clear();
  ssrVTR.clear();
  ssrComposite.clear();
  ssrVTRRT.reset();

  ssrQuad.clear();
  ssrMipChain.clear();
  ssrTarget.forEach([](auto &t) { t.close(); });
  ssrPrevTarget.forEach([](auto &t) { t.close(); });
  rtTemp.reset();
  rtTempLow.reset();

  ssrCompute.reset();

  ssrClassify.reset();
  ssrIntersect.reset();
  ssrReproject.reset();
  ssrPrefilter.reset();
  rayListBuf.close();
  tileListBuf.close();
  countersBuf.close();

  if (quality == SSRQuality::ComputeHQ)
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

void ScreenSpaceReflections::render(const DPoint3 &world_pos, SubFrameSample sub_sample)
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

  if (quality == SSRQuality::Low || quality == SSRQuality::Medium || quality == SSRQuality::Compute)
  {
    render(world_pos, sub_sample, ssrTex, prevTex, targetTex, {}, afrs.current().frameNo);
  }
  else if (quality == SSRQuality::High)
  {
    RTarget::Ptr rtVTR = ssrVTRRT->acquire();
    RTarget::Ptr rtTemp1 = rtTemp->acquire();
    eastl::array<ManagedTexView, 2> tmpTextures{*rtVTR, *rtTemp1};
    render(world_pos, sub_sample, ssrTex, prevTex, targetTex, tmpTextures, afrs.current().frameNo);
  }
  else if (quality == SSRQuality::ComputeHQ)
  {
    RTarget::Ptr rtTempReprojected = rtTemp->acquire();
    RTarget::Ptr rtTempPrefiltered = rtTemp->acquire();
    RTarget::Ptr tempRayLength = rtTempR16F->acquire();
    eastl::array<ManagedTexView, 3> tmpTextures{*rtTempReprojected, *rtTempPrefiltered, *tempRayLength};
    render(world_pos, sub_sample, ssrTex, prevTex, targetTex, tmpTextures, afrs.current().frameNo);
  }
}

void ScreenSpaceReflections::render(const DPoint3 &world_pos, SubFrameSample sub_sample, ManagedTexView ssr_tex,
  ManagedTexView ssr_prev_tex, ManagedTexView target_tex, const dag::ConstSpan<ManagedTexView> &tmp_textures, int callId)
{
  G_ASSERT(ssr_tex && ssr_prev_tex && target_tex);
  TIME_D3D_PROFILE(ssr);

  SCOPE_RENDER_TARGET;

  Afr &afr = afrs.current();
  set_reprojection(afr.prevWorldPos, afr.prevGlobTm, afr.prevViewVecLT, afr.prevViewVecRT, afr.prevViewVecLB, afr.prevViewVecRB,
    &world_pos);

  ShaderGlobal::set_color4(ssr_target_sizeVarId, ssrW, ssrH, 1.0f / ssrW, 1.0f / ssrH);
  ShaderGlobal::set_color4(ssr_frameNoVarId, callId & ((1 << 23) - 1), (callId % randomizeOverNFrames) * 1551,
    (callId % randomizeOverNFrames), 0);

  ssr_prev_tex.getTex2D()->texaddr(TEXADDR_CLAMP); // since we don't sample outside the ssr without weightening in reprojection
  ShaderGlobal::set_texture(ssr_prev_targetVarId, ssr_prev_tex);
  ShaderGlobal::set_color4(ssr_tile_jitterVarId, callId % 2, (callId / 2) % 2, 0, 0);

  if (quality == SSRQuality::High)
  {
    G_ASSERT(tmp_textures.size() == 2);
    const auto &vtr_target = tmp_textures[0];
    const auto &composed_ssr_target = tmp_textures[1];
    {
      TIME_D3D_PROFILE(ssr_only_ps)
      d3d::set_render_target(target_tex.getTex2D(), 0);
      ssrQuad.render();
    }

    {
      TIME_D3D_PROFILE(ssr_vtr)
      d3d::set_render_target(vtr_target.getTex2D(), 0);
      d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      d3d::resource_barrier({vtr_target.getTex2D(), RB_RW_RENDER_TARGET | RB_STAGE_PIXEL, 0, 0});
      d3d::resource_barrier({target_tex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      ShaderGlobal::set_texture(ssr_strengthVarId, target_tex);
      ssrVTR.render();
    }

    vtr_target.getTex2D()->texaddr(TEXADDR_CLAMP);
    {
      TIME_D3D_PROFILE(ssr_composite)
      d3d::set_render_target(composed_ssr_target.getTex2D(), 0);
      d3d::resource_barrier({vtr_target.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      ShaderGlobal::set_texture(ssr_targetVarId, target_tex);
      ShaderGlobal::set_texture(vtr_targetVarId, vtr_target);
      ssrComposite.render();
    }

    composed_ssr_target.getTex2D()->texaddr(TEXADDR_CLAMP);
    {
      TIME_D3D_PROFILE(ssr_resolve)
      d3d::set_render_target(target_tex.getTex2D(), 0);
      d3d::resource_barrier({composed_ssr_target.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      ShaderGlobal::set_texture(ssr_targetVarId, composed_ssr_target);
      ssrResolve.render();
    }
  }
  else if ((quality == SSRQuality::Low) || (quality == SSRQuality::Medium))
  {
    TIME_D3D_PROFILE(ssr_ps)
    d3d::set_render_target(target_tex.getTex2D(), 0);
    ssrQuad.render();
  }
  else if (quality == SSRQuality::Compute)
  {
    const float zeroes[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    d3d::clear_rwtexf(target_tex.getTex2D(), zeroes, 0, 0);
    d3d::set_rwtex(STAGE_CS, 0, target_tex.getTex2D(), 0, 0);
    ssrCompute->dispatchThreads(ssrW, ssrH, 1);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  }
  else if (quality == SSRQuality::ComputeHQ)
  {
    G_ASSERT(tmp_textures.size() == 3);

    // There is an unusual dependency between viewport size and rtTempLow size
    // Currently framegraph can't handle it correctly, so we have to use own texture
    RTarget::Ptr rtTempAvg = rtTempLow->acquire();

    d3d::resource_barrier({countersBuf.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});

    const float zeroes[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    d3d::clear_rwtexf(target_tex.getTex2D(), zeroes, 0, 0);

    const int currIndex = (afr.frameNo % 2);
    const int prevIndex = (currIndex + 1) % 2;

    { // classify pixel quads, output rays, determine 8x8 tiles in which to run denoiser
      TIME_D3D_PROFILE(ssr_classify)

      ShaderGlobal::set_texture(ssr_variance_texVarId, varianceTex[prevIndex]);

      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), rayListBuf.getBuf());
      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 1, VALUE), tileListBuf.getBuf());
      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 2, VALUE), countersBuf.getBuf());

      ssrClassify->dispatchThreads(ssrW, ssrH, 1);
    }

    { // prepare arguments for indirect dispatch
      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), countersBuf.getBuf());
      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 1, VALUE), indirectArgsBuf.getBuf());
      ssrPrepareIndirectArgs->dispatchThreads(1, 1, 1);
    }

    { // intersection
      TIME_D3D_PROFILE(ssr_intersect)

      d3d::resource_barrier({indirectArgsBuf.getBuf(), RB_RO_INDIRECT_BUFFER});
      d3d::resource_barrier({countersBuf.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
      d3d::resource_barrier({rayListBuf.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});

      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), target_tex.getTex2D());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 1, VALUE, 0, 0), tmp_textures[2].getTex2D());
      ssrIntersect->dispatch_indirect(indirectArgsBuf.getBuf(), 0);
    }

    tmp_textures[0].getTex2D()->texaddr(TEXADDR_BORDER);
    tmp_textures[0].getTex2D()->texbordercolor(0);

    // Temporary texture holding averate color in 8x8 tiles. Used as a fallback on disoclusion.
    rtTempAvg->getTex2D()->texaddr(TEXADDR_BORDER);
    rtTempAvg->getTex2D()->texbordercolor(0);

    { // reprojection
      TIME_D3D_PROFILE(ssr_reproject)

      ShaderGlobal::set_texture(ssr_reflection_texVarId, target_tex);
      ShaderGlobal::set_texture(ssr_raylen_texVarId, tmp_textures[2]);
      ShaderGlobal::set_texture(ssr_reflection_history_texVarId, target_tex);
      ShaderGlobal::set_texture(ssr_variance_texVarId, varianceTex[prevIndex]);
      ShaderGlobal::set_texture(ssr_sample_count_texVarId, sampleCountTex[prevIndex]);

      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), tmp_textures[0].getTex2D());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 1, VALUE, 0, 0), rtTempAvg->getTex2D());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 2, VALUE, 0, 0), varianceTex[currIndex].getTex2D());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 3, VALUE, 0, 0), sampleCountTex[currIndex].getTex2D());
      ssrReproject->dispatch_indirect(indirectArgsBuf.getBuf(), DISPATCH_INDIRECT_BUFFER_SIZE);
    }

    tmp_textures[1].getTex2D()->texaddr(TEXADDR_BORDER);
    tmp_textures[1].getTex2D()->texbordercolor(0);

    { // prefilter
      TIME_D3D_PROFILE(ssr_prefilter)
      ShaderGlobal::set_texture(ssr_reflection_texVarId, target_tex);
      ShaderGlobal::set_texture(ssr_avg_reflection_texVarId, rtTempAvg->getTexId());
      ShaderGlobal::set_texture(ssr_variance_texVarId, varianceTex[currIndex]);

      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), tmp_textures[1].getTex2D());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 1, VALUE, 0, 0), varianceTex[prevIndex].getTex2D());
      ssrPrefilter->dispatch_indirect(indirectArgsBuf.getBuf(), DISPATCH_INDIRECT_BUFFER_SIZE);
    }

    { // temporal
      TIME_D3D_PROFILE(ssr_temporal)
      ShaderGlobal::set_texture(ssr_reflection_texVarId, tmp_textures[1]);
      ShaderGlobal::set_texture(ssr_reproj_reflection_texVarId, tmp_textures[0]);
      ShaderGlobal::set_texture(ssr_sample_count_texVarId, sampleCountTex[currIndex]);
      ShaderGlobal::set_texture(ssr_avg_reflection_texVarId, rtTempAvg->getTexId());
      ShaderGlobal::set_texture(ssr_variance_texVarId, varianceTex[prevIndex]); // prefiltered current variance

      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), ssr_tex.getTex2D());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 1, VALUE, 0, 0), varianceTex[currIndex].getTex2D());
      ssrTemporal->dispatch_indirect(indirectArgsBuf.getBuf(), DISPATCH_INDIRECT_BUFFER_SIZE);
    }
  }

  if (quality != SSRQuality::ComputeHQ && !hasPrevTarget && ownTextures)
  {
    // this is the case when previous frame ssr is stored in ssr_tex, while the new one
    // is rendered in target_tex. It is happended where there is no separate previous frame texture
    d3d::resource_barrier({target_tex.getTex2D(), RB_RO_COPY_SOURCE, 0, 1});
    ssr_tex.getTex2D()->updateSubRegion(target_tex.getTex2D(), 0, 0, 0, 0, ssrW, ssrH, 1, 0, 0, 0, 0);
    ssr_tex.getTex2D()->texaddr(TEXADDR_BORDER);
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

  ssrPrevTarget.forEach([](auto &t) { t.close(); });
}