// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_convar.h>
#include <render/spheres_consts.hlsli>
#include <shaders/dag_DynamicShaderHelper.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <3d/dag_ringCPUQueryLock.h>
#include "volumetricGI.h"
#include "radianceCache.h"
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_lock.h>

#define GLOBAL_VARS_LIST                                 \
  VAR(gi_current_froxels_radiance)                       \
  VAR(gi_current_froxels_radiance_samplerstate)          \
  VAR(gi_froxels_world_view_pos)                         \
  VAR(gi_froxels_zn_zfar)                                \
  VAR(gi_froxels_prev_zn_zfar)                           \
  VAR(gi_froxels_view_vecLT)                             \
  VAR(gi_froxels_view_vecRT)                             \
  VAR(gi_froxels_view_vecLB)                             \
  VAR(gi_froxels_jittered_current_to_unjittered_history) \
  VAR(gi_prev_froxels_sph0)                              \
  VAR(gi_prev_froxels_sph1)                              \
  VAR(gi_prev_froxels_sph0_samplerstate)                 \
  VAR(gi_prev_froxels_sph1_samplerstate)                 \
  VAR(gi_froxels_sph0)                                   \
  VAR(gi_froxels_sph1)                                   \
  VAR(gi_froxels_sph1_samplerstate)                      \
  VAR(gi_froxels_temporal_frame)                         \
  VAR(gi_froxels_debug_type)                             \
  VAR(gi_froxels_sizei)                                  \
  VAR(gi_froxels_history_blur_ofs)                       \
  VAR(gi_froxels_atlas_sizei)                            \
  VAR(gi_froxels_dist)

#define VAR(a) static ShaderVariableInfo a##VarId(#a);
GLOBAL_VARS_LIST
#undef VAR

static void set_frame_info(const DaGIFrameInfo &fi)
{
  ShaderGlobal::set_color4(gi_froxels_world_view_posVarId, P3D(fi.world_view_pos), 0);
  ShaderGlobal::set_color4(gi_froxels_view_vecLTVarId, Color4(&fi.viewVecLT.x));
  ShaderGlobal::set_color4(gi_froxels_view_vecRTVarId, Color4(&fi.viewVecRT.x));
  ShaderGlobal::set_color4(gi_froxels_view_vecLBVarId, Color4(&fi.viewVecLB.x));
  ShaderGlobal::set_color4(gi_froxels_zn_zfarVarId, fi.znear, fi.zfar, 1.f / fi.zfar, (fi.zfar - fi.znear) / (fi.znear * fi.zfar));
}

void VolumetricGI::afterReset()
{
  temporalFrame = 0;
  validHistory = false;
  d3d::GpuAutoLock gpuLock;
  d3d::clear_rwtexf(gi_froxels_sph0[temporalFrame & 1].getVolTex(), ResourceClearValue{}.asFloat, 0, 0);
  d3d::clear_rwtexf(gi_froxels_sph1[temporalFrame & 1].getVolTex(), ResourceClearValue{}.asFloat, 0, 0);
}

static void set_prev_frame_info(const VolumetricGI::FrameInfo &prev, const VolumetricGI::FrameInfo &cur, float blur_texel, int d)
{
  TMatrix4 globTm = get_reprojection_campos_to_unjittered_history(cur, prev).transpose();
  ShaderGlobal::set_float4x4(gi_froxels_jittered_current_to_unjittered_historyVarId, globTm);
  ShaderGlobal::set_color4(gi_froxels_history_blur_ofsVarId, blur_texel / prev.res.w, blur_texel / prev.res.h, blur_texel / d, 0);
  ShaderGlobal::set_color4(gi_froxels_prev_zn_zfarVarId, prev.znear, prev.zfar, 1.f / prev.zfar,
    (prev.zfar - prev.znear) / (prev.znear * prev.zfar));
}

void VolumetricGI::allocate(uint32_t tile_sz, uint32_t max_sw_, uint32_t max_sh_, uint32_t res_, uint32_t spatial_passes_,
  float irradiance_probe_size, uint32_t irradiance_clip_w, bool reproject_history, const ZDistributionParams &z_distr)
{
  const uint32_t w_ = (max_sw_ + tile_sz - 1) / tile_sz, h_ = (max_sh_ + tile_sz - 1) / tile_sz;
  ZParams zparams_;
  const int d_ = max<int>(2, z_distr.slices);
  zparams_.zExpMul = 1. / z_distr.zLogMul;
  zparams_.zMul = (z_distr.zDist - z_distr.zStart) / (exp2f((d_ - 0.5) * zparams_.zExpMul) - exp2f(0.5 * zparams_.zExpMul));
  zparams_.zAdd = z_distr.zStart - zparams_.zMul * exp2f(0.5 * zparams_.zExpMul);
  zparams_.zDist = z_distr.zDist;
  uint16_t traced_slices = 0;
  const float sliceExpMul = exp2f(zparams_.zExpMul);
  float irradianceClipDist = (sqrt(2.f) * 0.5f) * irradiance_clip_w * irradiance_probe_size; // half diagonal dist
  for (; traced_slices < d_; ++traced_slices)
  {
    const float curExpZ = exp2f((traced_slices + 0.5f) * zparams_.zExpMul);
    const float curZNoAdd = curExpZ * zparams_.zMul;
    // zSizeSize exp2f((cSlice + 1) * zExpMul)*zMul - exp2(cSlice*zExpMul)*zMul = curZ*(exp2(zExpMul) - 1)
    const float zSliceSize = curZNoAdd * (sliceExpMul - 1.f);
    const float zSliceDist = max(curZNoAdd + zparams_.zAdd, 1e-5f);
    const uint32_t fixedH = (1080 + 63) / 64;            // 1080p with 64 tile
    const float hSliceSize = zSliceDist * 1.3f / fixedH; // assuming fov = 100deg, and using fixed resolution
    const int irradianceClip = max(0.f, floorf(1.f + log2f(zSliceDist / irradianceClipDist)));

    const float irradianceProbeSizeAtSlice = irradiance_probe_size * (1 << irradianceClip);
    if (zSliceSize > irradianceProbeSizeAtSlice && hSliceSize > irradianceProbeSizeAtSlice)
      break;
  }
  if (traced_slices < 3)
    traced_slices = 0;
  reproject_history |= traced_slices > 0;

  if (zParams != zparams_)
  {
    validHistory = false;
    temporalFrame = 0;
  }
  zParams = zparams_;
  if (maxW == w_ && maxH == h_ && d == d_ && radianceRes == res_ &&
      (spatial_passes_ != 0) == bool(gi_froxels_radiance[1].getVolTex()) &&
      reproject_history == bool(gi_froxels_sph0[1].getVolTex()) && tracedSlices == traced_slices)
    return;
  debug("irradiance_probe_size = %f, clipW = %d, z_distr.zDist = %f", irradiance_probe_size, irradiance_clip_w, z_distr.zDist);
  spatialPasses = spatial_passes_;
  temporalFrame = 0;
  maxW = w_;
  maxH = h_;
  const int w = maxW, h = maxH;
  d = d_;
  radianceRes = res_;
  tracedSlices = traced_slices;
  validHistory = false;

  gi_froxels_radiance[0].close();
  gi_froxels_radiance[1].close();
  gi_froxels_sph0[0].close();
  gi_froxels_sph0[1].close();
  gi_froxels_sph1[0].close();
  gi_froxels_sph1[1].close();

  if (tracedSlices)
  {
    debug("create volumetric GI radiance resolution %dx%dx%d : %d^2 spatial=%d", w, h, tracedSlices, radianceRes, spatialPasses);
    // this is actually not better than a buffer, since we don't sample it
    gi_froxels_radiance[0] = dag::create_voltex(w * radianceRes, h * radianceRes, tracedSlices, TEXCF_UNORDERED | TEXFMT_R11G11B10F, 1,
      "gi_froxels_radiance0");
    gi_froxels_radiance[0]->disableSampler();
    if (spatialPasses)
    {
      gi_froxels_radiance[1] = dag::create_voltex(w * radianceRes, h * radianceRes, tracedSlices, TEXCF_UNORDERED | TEXFMT_R11G11B10F,
        1, "gi_froxels_radiance1");
      gi_froxels_radiance[1]->disableSampler();
    }
  }
  else
    ShaderGlobal::set_texture(gi_current_froxels_radianceVarId, BAD_TEXTUREID);
  debug("create volumetric GI irradiance resolution %dx%dx%d, history %d", w, h, d, reproject_history);
  gi_froxels_sph0[0] = dag::create_voltex(w, h, d, TEXCF_UNORDERED | TEXFMT_R11G11B10F, 1, "gi_froxels_sph0_0");
  gi_froxels_sph1[0] = dag::create_voltex(w, h, d, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F, 1, "gi_froxels_sph1_0");
  gi_froxels_sph0[0]->disableSampler();
  gi_froxels_sph1[0]->disableSampler();
  if (reproject_history)
  {
    gi_froxels_sph0[1] = dag::create_voltex(w, h, d, TEXCF_UNORDERED | TEXFMT_R11G11B10F, 1, "gi_froxels_sph0_1");
    gi_froxels_sph1[1] = dag::create_voltex(w, h, d, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F, 1, "gi_froxels_sph1_1");
    gi_froxels_sph0[1]->disableSampler();
    gi_froxels_sph1[1]->disableSampler();
  }
#define CS(a) a.reset(new_compute_shader(#a))
  CS(calc_current_gi_froxels_cs);
  CS(spatial_filter_gi_froxels_cs);
  CS(calc_gi_froxels_irradiance_cs);
  CS(calc_gi_froxels_irradiance_simple_cs);
#undef CS
  ShaderGlobal::set_int4(gi_froxels_sizeiVarId, w, h, d, radianceRes);
  ShaderGlobal::set_int4(gi_froxels_atlas_sizeiVarId, w * radianceRes, h * radianceRes, tracedSlices,
    w * radianceRes * h * radianceRes);
  prevFrameInfo.res = nextRes = curRes = FroxelsRes{uint32_t(w), uint32_t(h)};
}

void VolumetricGI::init(uint32_t tile_sz, uint32_t sw, uint32_t sh, uint32_t max_sw, uint32_t max_sh, uint32_t res,
  uint32_t spatial_passes, float irradiance_probe_size, uint32_t irradiance_clip_w, bool reproject_history,
  const ZDistributionParams &z_distr)
{
  allocate(tile_sz, max_sw, max_sh, res, spatial_passes, irradiance_probe_size, irradiance_clip_w, reproject_history, z_distr);
  nextRes.w = (sw + tile_sz - 1) / tile_sz;
  nextRes.h = (sh + tile_sz - 1) / tile_sz;
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    d3d::SamplerHandle sampler = d3d::request_sampler(smpInfo);
    gi_froxels_sph1_samplerstateVarId.set_sampler(sampler);
    gi_prev_froxels_sph0_samplerstateVarId.set_sampler(sampler);
    gi_prev_froxels_sph1_samplerstateVarId.set_sampler(sampler);
    gi_current_froxels_radiance_samplerstateVarId.set_sampler(sampler);
  }
}

CONSOLE_BOOL_VAL("render", gi_volumetrics_calc, true);

static void resize_clamp(ResizableTex &t, int w, int h, int d)
{
  if (!t)
    return;
  t.resize(w, h, d);
}

void VolumetricGI::calc(const TMatrix &viewItm, const TMatrix4 &projTm, float zn, float zf, float quality)
{
  if (!gi_volumetrics_calc)
    return;
  DA_PROFILE_GPU;
  const bool useReprojection = gi_froxels_sph0[1].getVolTex() && quality > 0;
  if (!useReprojection)
    temporalFrame = 0;

  const bool changed = nextRes != prevFrameInfo.res;
  FrameInfo cFrameInfo;
  cFrameInfo.res = curRes = nextRes;
  (DaGIFrameInfo &)cFrameInfo = get_frame_info(viewItm, projTm, zn, zf);
  const int cFrame = temporalFrame & 1, pFrame = 1 - cFrame;
  const int w = cFrameInfo.res.w, h = cFrameInfo.res.h;
  if (changed)
  {
    resize_clamp(gi_froxels_radiance[0], w * radianceRes, h * radianceRes, tracedSlices);
    resize_clamp(gi_froxels_radiance[1], w * radianceRes, h * radianceRes, tracedSlices);
    resize_clamp(gi_froxels_sph0[cFrame], w, h, d);
    resize_clamp(gi_froxels_sph1[cFrame], w, h, d);
    ShaderGlobal::set_int4(gi_froxels_sizeiVarId, w, h, d, radianceRes);
    ShaderGlobal::set_int4(gi_froxels_atlas_sizeiVarId, w * radianceRes, h * radianceRes, tracedSlices,
      w * radianceRes * h * radianceRes);
  }
  set_frame_info(cFrameInfo);
  set_prev_frame_info(validHistory ? prevFrameInfo : cFrameInfo, cFrameInfo, historyBlurTexelOfs, d);

  prevFrameInfo = cFrameInfo;
  ShaderGlobal::set_color4(gi_froxels_distVarId, 1.f / zParams.zMul, -zParams.zAdd / zParams.zMul, 1.f / zParams.zExpMul,
    1.f / (zParams.zExpMul * d));
  const uint32_t frames = 8;
  ShaderGlobal::set_int4(gi_froxels_temporal_frameVarId, temporalFrame % frames, validHistory, frames, temporalFrame);
  if (useReprojection)
  {
    if (gi_froxels_radiance[0].getVolTex())
    {
      {
        TIME_D3D_PROFILE(volumetric_gi_radiance);
        d3d::set_rwtex(STAGE_CS, 0, gi_froxels_radiance[0].getVolTex(), 0, 0);
        calc_current_gi_froxels_cs->dispatchThreads(w * radianceRes * h * radianceRes * d, 1, 1);
        d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
      }
      for (int i = 0; i < spatialPasses; ++i)
      {
        TIME_D3D_PROFILE(volumetric_gi_radiance_spatial);
        d3d::resource_barrier({gi_froxels_radiance[i & 1].getVolTex(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
        ShaderGlobal::set_texture(gi_current_froxels_radianceVarId, gi_froxels_radiance[i & 1].getTexId());
        d3d::set_rwtex(STAGE_CS, 0, gi_froxels_radiance[(i + 1) & 1].getVolTex(), 0, 0);
        spatial_filter_gi_froxels_cs->dispatchThreads(w * radianceRes * h * radianceRes * d, 1, 1);
        d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
      }
      d3d::resource_barrier(
        {gi_froxels_radiance[spatialPasses & 1].getVolTex(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
      ShaderGlobal::set_texture(gi_current_froxels_radianceVarId, gi_froxels_radiance[spatialPasses & 1].getTexId());
    }
    ShaderGlobal::set_texture(gi_prev_froxels_sph0VarId, gi_froxels_sph0[pFrame].getTexId());
    ShaderGlobal::set_texture(gi_prev_froxels_sph1VarId, gi_froxels_sph1[pFrame].getTexId());
  }

  {
    TIME_D3D_PROFILE(volumetric_gi_irradiance);
    d3d::set_rwtex(STAGE_CS, 0, gi_froxels_sph0[cFrame].getVolTex(), 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, gi_froxels_sph1[cFrame].getVolTex(), 0, 0);
    (useReprojection ? calc_gi_froxels_irradiance_cs : calc_gi_froxels_irradiance_simple_cs)->dispatchThreads(w * h * d, 1, 1);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, nullptr, 0, 0);
  }
  d3d::resource_barrier({gi_froxels_sph0[cFrame].getVolTex(),
    RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
  d3d::resource_barrier({gi_froxels_sph1[cFrame].getVolTex(),
    RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});

  ShaderGlobal::set_texture(gi_froxels_sph0VarId, gi_froxels_sph0[cFrame].getTexId());
  ShaderGlobal::set_texture(gi_froxels_sph1VarId, gi_froxels_sph1[cFrame].getTexId());

  if (changed)
  {
    resize_clamp(gi_froxels_sph0[pFrame], w, h, d);
    resize_clamp(gi_froxels_sph1[pFrame], w, h, d);
  }
  if (useReprojection)
    ++temporalFrame;
  validHistory = true;
}


void VolumetricGI::initDebug() { drawDebugAllProbes.init("froxels_gi_draw_debug", NULL, 0, "froxels_gi_draw_debug"); }

void VolumetricGI::drawDebug(int debug_type)
{
  if (!drawDebugAllProbes.shader)
    initDebug();
  if (!drawDebugAllProbes.shader)
    return;
  DA_PROFILE_GPU;
  ShaderGlobal::set_int(gi_froxels_debug_typeVarId, debug_type);
  drawDebugAllProbes.shader->setStates(0, true);
  d3d::setvsrc(0, 0, 0);
  d3d::draw_instanced(PRIM_TRILIST, 0, LOW_SPHERES_INDICES_TO_DRAW, curRes.w * curRes.h * d);
}
