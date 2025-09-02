// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/deferredRenderer.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_rwResource.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix4.h>
#include <math/integer/dag_IPoint2.h>
#include <util/dag_string.h>
#include <perfMon/dag_statDrv.h>
#include <image/dag_texPixel.h>

#define GLOBAL_VARS_LIST          \
  VAR(screen_pos_to_texcoord)     \
  VAR(screen_size)                \
  VAR(gbuffer_view_size)          \
  VAR(albedo_gbuf)                \
  VAR(albedo_gbuf_samplerstate)   \
  VAR(normal_gbuf)                \
  VAR(normal_gbuf_samplerstate)   \
  VAR(material_gbuf)              \
  VAR(material_gbuf_samplerstate) \
  VAR(motion_gbuf)                \
  VAR(motion_gbuf_samplerstate)   \
  VAR(bvh_gbuf)                   \
  VAR(bvh_gbuf_samplerstate)      \
  VAR(depth_gbuf)                 \
  VAR(depth_gbuf_samplerstate)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

void DeferredRT::close()
{
  for (int i = 0; i < numRt; ++i)
  {
    mrts[i] = nullptr;
    mrtPools[i] = nullptr;
  }
  depth.close();
  blackPixelTex.close();
  numRt = 0;

#if DAGOR_DBGLEVEL > 0
  dbgTex.close();
#endif
}

void DeferredRT::setRt()
{
  d3d::set_render_target();
  for (int i = 0; i < numRt; ++i)
  {
    if (mrts[i] == nullptr)
    {
      mrts[i] = mrtPools[i]->acquire();
    }
    d3d::set_render_target(i, mrts[i]->getTex2D(), 0);
  }
  if (depth.getTex2D())
    d3d::set_depth(depth.getTex2D(), DepthAccess::RW);

#if DAGOR_DBGLEVEL > 0
  if (shouldRenderDbgTex)
  {
    if (!dbgTex.getTex2D())
    {
      initDebugTex();
    }
    d3d::set_render_target(5, dbgTex.getTex2D(), 0);
    d3d::clear_rt({dbgTex.getTex2D()}, make_clear_value(1.0f, 1.0f, 1.0f, 1.0f));
  }
#endif
}

void DeferredRT::setVar()
{
  ShaderGlobal::set_texture(albedo_gbufVarId, mrts[0] ? mrts[0]->getTexId() : BAD_TEXTUREID);
  ShaderGlobal::set_sampler(albedo_gbuf_samplerstateVarId, baseSampler);
  ShaderGlobal::set_texture(normal_gbufVarId, mrts[1] ? mrts[1]->getTexId() : BAD_TEXTUREID);
  ShaderGlobal::set_sampler(normal_gbuf_samplerstateVarId, baseSampler);
  ShaderGlobal::set_texture(material_gbufVarId, mrts[2] ? mrts[2]->getTexId() : BAD_TEXTUREID);
  ShaderGlobal::set_sampler(material_gbuf_samplerstateVarId, baseSampler);

  if (numRt > 3)
  {
    ShaderGlobal::set_texture(motion_gbufVarId, mrts[3] ? mrts[3]->getTexId() : BAD_TEXTUREID);
    ShaderGlobal::set_sampler(motion_gbuf_samplerstateVarId, baseSampler);
  }

  if (numRt > 4)
  {
    ShaderGlobal::set_texture(bvh_gbufVarId, mrts[4] ? mrts[4]->getTexId() : BAD_TEXTUREID);
    ShaderGlobal::set_sampler(bvh_gbuf_samplerstateVarId, baseSampler);
  }
  else if (d3d::get_driver_code().is(d3d::vulkan || d3d::xboxOne || d3d::scarlett))
  {
    if (!blackPixelTex)
    {
      TexImage32 *texel = TexImage32::create(1, 1);
      texel->getPixels()[0].u = 0;
      String dummyName(128, "%s_bvh_dummy", name);
      blackPixelTex = dag::create_tex(texel, 1, 1, TEXFMT_R32UI, 1, dummyName);
      delete texel;
      d3d::SamplerInfo smpInfo;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      smpInfo.mip_map_mode = d3d::MipMapMode::Point;
      blackPixelSampler = d3d::request_sampler(smpInfo);
    }
    ShaderGlobal::set_texture(bvh_gbufVarId, blackPixelTex.getTexId());
    ShaderGlobal::set_sampler(bvh_gbuf_samplerstateVarId, blackPixelSampler);
  }
  else
    ShaderGlobal::set_texture(bvh_gbufVarId, BAD_TEXTUREID);

  ShaderGlobal::set_texture(depth_gbufVarId, depth);
  ShaderGlobal::set_sampler(depth_gbuf_samplerstateVarId, baseSampler);
  ShaderGlobal::set_color4(screen_pos_to_texcoordVarId, 1.f / width, 1.f / height, 0, 0);
  ShaderGlobal::set_color4(screen_sizeVarId, width, height, 1.0 / width, 1.0 / height);
  ShaderGlobal::set_int4(gbuffer_view_sizeVarId, width, height, 0, 0);
}

uint32_t DeferredRT::recreateDepthInternal(uint32_t targetFmt)
{
  if (!(d3d::get_texformat_usage(targetFmt) & d3d::USAGE_DEPTH))
  {
    debug("not supported depth format 0x%08x, fallback to TEXFMT_DEPTH24", targetFmt);
    targetFmt = (targetFmt & (~TEXFMT_MASK)) | TEXFMT_DEPTH24;
  }
  uint32_t currentFmt = 0;
  if (depth.getTex2D())
  {
    TextureInfo tinfo;
    depth.getTex2D()->getinfo(tinfo, 0);
    currentFmt = tinfo.cflg & (TEXFMT_MASK | TEXCF_SAMPLECOUNT_MASK | TEXCF_TC_COMPATIBLE);
    targetFmt |= currentFmt & (~TEXFMT_MASK);
  }
  if (currentFmt == targetFmt)
    return currentFmt;

  depth.close();

  auto cs = calcCreationSize();

  const uint32_t flags = TEXCF_RTARGET;
  String depthName(128, "%s_intzDepthTex", name);
  TexPtr depthTex = dag::create_tex(NULL, cs.x, cs.y, targetFmt | flags, 1, depthName);

  if (!depthTex && (targetFmt & TEXFMT_MASK) != TEXFMT_DEPTH24)
  {
    debug("can't create depth format 0x%08x, fallback to TEXFMT_DEPTH24", targetFmt);
    targetFmt = (targetFmt & (~TEXFMT_MASK)) | TEXFMT_DEPTH24;
    depthTex = dag::create_tex(NULL, cs.x, cs.y, targetFmt | flags, 1, depthName);
  }

  if (!depthTex)
  {
    DAG_FATAL("can't create intzDepthTex (INTZ, DF24, RAWZ) due to err '%s'", d3d::get_last_error());
  }

  depth = eastl::move(depthTex);

  return targetFmt;
}

IPoint2 DeferredRT::calcCreationSize() const
{
  switch (stereoMode)
  {
    case StereoMode::MonoOrMultipass: return IPoint2(width, height);
    case StereoMode::SideBySideHorizontal: return IPoint2(width * 2, height);
    case StereoMode::SideBySideVertical: return IPoint2(width, height * 2);
  }

  // Thanks Linux!
  return IPoint2(1, 1);
}

uint32_t DeferredRT::recreateDepth(uint32_t targetFmt) { return recreateDepthInternal(targetFmt); }

DeferredRT::DeferredRT(const char *name_, int w, int h, StereoMode stereo_mode, unsigned msaaFlag, int num_rt, const unsigned *texFmt,
  uint32_t depth_fmt) :
  stereoMode(stereo_mode)
{
  if (screen_sizeVarId < 0)
  {
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
    GLOBAL_VARS_LIST
#undef VAR
  }

  strncpy(name, name_, sizeof(name));
  uint32_t currentFmt = depth_fmt;
  close();
  width = w;
  height = h;

  if (depth_fmt)
    recreateDepthInternal(currentFmt | (~TEXCF_RT_COMPRESSED & msaaFlag));

  auto cs = calcCreationSize();

  numRt = num_rt;
  for (int i = numRt - 1; i >= 0; --i)
  {
    if (texFmt && texFmt[i] == 0xFFFFFFFFU)
      continue;

    String mrtName(128, "%s_mrt_%d", name, i);
    unsigned mrtFmt = texFmt ? texFmt[i] : TEXFMT_A8R8G8B8;
    mrtPools[i] = ResizableRTargetPool::get(cs.x, cs.y, mrtFmt | TEXCF_RTARGET | TEXCF_CLEAR_ON_CREATE | msaaFlag, 1);
    mrts[i] = mrtPools[i]->acquire();
  }
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    smpInfo.mip_map_mode = d3d::MipMapMode::Point;
    baseSampler = d3d::request_sampler(smpInfo);
  }
}

void DeferredRT::changeResolution(const int w, const int h)
{
  width = w;
  height = h;

  auto cs = calcCreationSize();

  for (int i = 0; i < numRt; ++i)
  {
    mrts[i] = nullptr;
    G_ASSERT(mrtPools[i] != nullptr);
    int createFlags = get_format(*mrtPools[i]);
    int levels = get_levels(*mrtPools[i]);
    mrtPools[i] = ResizableRTargetPool::get(cs.x, cs.y, createFlags, levels);
  }
  depth.resize(cs.x, cs.y);

#if DAGOR_DBGLEVEL > 0
  if (dbgTex.getTex2D())
  {
    dbgTex.resize(cs.x, cs.y);
  }
#endif
}

void DeferredRT::initDebugTex()
{
#if DAGOR_DBGLEVEL > 0
  if (!dbgTex.getTex2D())
  {
    IPoint2 cs = calcCreationSize();
    auto tempTex = dag::create_tex(NULL, cs.x, cs.y, TEXFMT_R8 | TEXCF_RTARGET | TEXCF_CLEAR_ON_CREATE, 1, "dbg_gbuff_tex");

    dbgTex = ResizableTex(eastl::move(tempTex), "dbg_gbuff_tex");
  }
#endif
}

const ManagedTex &DeferredRT::getDbgTex()
{
#if DAGOR_DBGLEVEL > 0
  initDebugTex();
#endif
  return dbgTex;
}

void DeferredRT::acquirePooledRTs()
{
  for (int i = 0; i < numRt; ++i)
  {
    if (mrtPools[i] != nullptr)
    {
      if (mrts[i] == nullptr)
        mrts[i] = mrtPools[i]->acquire();
    }
  }
}

void DeferredRT::releasePooledRT(uint32_t idx)
{
  if (idx >= MAX_NUM_MRT)
    return;
  mrts[idx] = nullptr;
}
