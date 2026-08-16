//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_renderTarget.h>
#include <3d/dag_texMgr.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint4.h>
#include <resourcePool/resourcePool.h>

class BaseTexture;
typedef BaseTexture Texture;
class DeferredRT
{
public:
  enum class StereoMode
  {
    MonoOrMultipass,
    SideBySideHorizontal,
    SideBySideVertical,
  };

  static const int MAX_NUM_MRT = 5;

  void close();
  // Binds the gbuffer color slots and the deferred depth in the given access mode.
  // Pass a non-null depth_override to attach a different depth texture instead.
  void setRt(DepthAccess depth_access = DepthAccess::RW, BaseTexture *depth_override = nullptr);
  void resolve();
  void setVar();
  void resetVar();
  // How shaders address this gbuffer, published by setVar() along with the textures it describes. Defaults to
  // 1-to-1; only a gbuffer shared by several views needs to set it, one sub-rect per eye under VR multiview.
  void setUvTransform(const Point4 &uv_transform, const IPoint4 &uv_transform_i)
  {
    uvTransform = uv_transform;
    uvTransformI = uv_transform_i;
  }
  void debugRender(int show_gbuffer);
  void initDebugTex();
  const ManagedTex &getDbgTex();
  void setShouldRenderDbgTex(bool value) { shouldRenderDbgTex = value; }
  uint32_t recreateDepth(uint32_t fmt); // returns true if 32 bit depth buffer was created
  DeferredRT(const char *name, int w, int h, StereoMode stereo_mode, unsigned msaaFlag, int numRt, const unsigned *texFmt,
    uint32_t depth_fmt);
  ~DeferredRT() { close(); }
  void changeResolution(int w, int h);
  int getWidth() const { return width; }
  int getHeight() const { return height; }
  Texture *getDepth() const { return depth.getTex2D(); }
  TEXTUREID getDepthId() const { return depth.getTexId(); }
  const ManagedTex &getDepthAll() const { return depth; }
  Texture *getRt(uint32_t idx, bool optional = false) const
  {
    G_ASSERT(optional || mrts[idx]);
    return mrts[idx] ? mrts[idx]->getTex2D() : nullptr;
    G_UNUSED(optional);
  }
  TEXTUREID getRtId(uint32_t idx, bool optional = false) const
  {
    G_ASSERT(optional || mrts[idx]);
    return mrts[idx] ? mrts[idx]->getTexId() : BAD_TEXTUREID;
    G_UNUSED(optional);
  }
  const ManagedTex &getRtAll(uint32_t idx) const
  {
    G_ASSERT(mrts[idx]);
    return *mrts[idx];
  }
  uint32_t getRtNum() const { return numRt; }
  void swapDepth(ResizableTex &ndepth) { eastl::swap(depth, ndepth); }
  d3d::SamplerHandle getSampler() const { return baseSampler; }
  bool useRt(uint32_t idx) const
  {
    G_ASSERT(idx < MAX_NUM_MRT);
    return mrtPools[idx] != nullptr;
  }
  void acquirePooledRTs();
  void releasePooledRT(uint32_t idx);

protected:
  uint32_t recreateDepthInternal(uint32_t fmt);

  IPoint2 calcCreationSize() const;

  StereoMode stereoMode = StereoMode::MonoOrMultipass;
  int numRt = 0, width = 0, height = 0;
  char name[64] = {};

  Point4 uvTransform = Point4(1, 1, 0, 0);
  IPoint4 uvTransformI = IPoint4::ZERO;

  ResizableRTargetPool::Ptr mrtPools[MAX_NUM_MRT];
  ResizableRTarget::Ptr mrts[MAX_NUM_MRT];
  d3d::SamplerHandle baseSampler = d3d::INVALID_SAMPLER_HANDLE;
  d3d::SamplerHandle blackPixelSampler = d3d::INVALID_SAMPLER_HANDLE;
  ResizableTex depth;
  ResizableTex dbgTex;
  UniqueTex blackPixelTex;

  bool useResolvedDepth = false;
  bool shouldRenderDbgTex = false;
};
