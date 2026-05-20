//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_decl.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_texMgr.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <render/deferredRT.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/unique_ptr.h>

class DebugGbufferRenderer;
class PostFxRenderer;
class ComputeShaderElement;

extern const int USE_DEBUG_GBUFFER_MODE;

class ShadingResolver
{
public:
  enum class ClearTarget
  {
    No,
    Yes
  };

  ShadingResolver(const char *resolve_pshader_name, const char *resolve_cshader_name);
  ShadingResolver(const char *resolve_pshader_name) : ShadingResolver(resolve_pshader_name, nullptr /* resolve_cshader_name */) {}
  ~ShadingResolver();

  PostFxRenderer *getResolveShading() const { return resolveShading.get(); }

  void resolve(BaseTexture *resolveTarget, const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *depth_bounds_tex = nullptr,
    ShadingResolver::ClearTarget clear_target = ShadingResolver::ClearTarget::No, const TMatrix4 &gbufferTm = TMatrix4::IDENT,
    const RectInt *resolve_area = nullptr);

private:
  eastl::unique_ptr<PostFxRenderer> resolveShading;
  eastl::unique_ptr<ComputeShaderElement> resolveShadingCS;
};

class DeferredRenderTarget
{
public:
  DeferredRenderTarget(const char *resolve_pshader_name, const char *resolve_cshader_name, const char *name, int w, int h,
    DeferredRT::StereoMode stereo_mode, unsigned msaaFlag, int numRt, const unsigned *texFmt, uint32_t depthFmt);
  DeferredRenderTarget(const char *resolve_pshader_name, const char *name, int w, int h, DeferredRT::StereoMode stereo_mode,
    unsigned msaaFlag, int numRt, const unsigned *texFmt, uint32_t depthFmt) :
    DeferredRenderTarget(resolve_pshader_name, nullptr /* resolve_cshader_name */, name, w, h, stereo_mode, msaaFlag, numRt, texFmt,
      depthFmt)
  {}
  ~DeferredRenderTarget();
  DeferredRenderTarget(const DeferredRenderTarget &) = delete;
  DeferredRenderTarget &operator=(const DeferredRenderTarget &) = delete;

  void resourceBarrier(ResourceBarrier barrier);
  void setRt() { renderTargets.setRt(); }
  void resolve(BaseTexture *resolveTarget, const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *depth_bounds_tex = nullptr,
    ShadingResolver::ClearTarget clear_target = ShadingResolver::ClearTarget::No, const TMatrix4 &gbufferTm = TMatrix4::IDENT,
    const RectInt *resolve_area = nullptr);
  void setVar() { renderTargets.setVar(); }
  void changeResolution(const int w, const int h) { renderTargets.changeResolution(w, h); }
  void debugRender(BaseTexture *depth = nullptr, int show_gbuffer = USE_DEBUG_GBUFFER_MODE);
  void debugRenderVectors(int show_gbuffer = USE_DEBUG_GBUFFER_MODE, int vec_count = -1, int vec_scale = 0.f);
  // returns true if 32 bit depth buffer was created
  uint32_t recreateDepth(uint32_t fmt) { return renderTargets.recreateDepth(fmt); }
  int getWidth() const { return renderTargets.getWidth(); }
  int getHeight() const { return renderTargets.getHeight(); }
  Texture *getDepth() const { return renderTargets.getDepth(); }
  TEXTUREID getDepthId() const { return renderTargets.getDepthId(); }
  const ManagedTex &getDepthAll() const { return renderTargets.getDepthAll(); }
  const ManagedTexView getDepthAllView() const { return {getDepthAll()}; }
  PostFxRenderer *getResolveShading() const { return shadingResolver.getResolveShading(); }
  Texture *getRt(uint32_t idx, bool optional = false) const { return renderTargets.getRt(idx, optional); }
  TEXTUREID getRtId(uint32_t idx, bool optional = false) const { return renderTargets.getRtId(idx, optional); }
  const ManagedTex &getRtAll(uint32_t idx) const { return renderTargets.getRtAll(idx); }
  ManagedTexView getRtAllView(uint32_t idx) const { return {getRtAll(idx)}; }
  uint32_t getRtNum() const { return renderTargets.getRtNum(); }
  void swapDepth(ResizableTex &ndepth) { renderTargets.swapDepth(ndepth); }
  d3d::SamplerHandle getSampler() const { return renderTargets.getSampler(); }
  void acquirePooledRTs() { renderTargets.acquirePooledRTs(); }
  void releasePooledRT(uint32_t idx) { renderTargets.releasePooledRT(idx); }
  bool useRt(uint32_t idx) const { return renderTargets.useRt(idx); }

protected:
  DeferredRT renderTargets;
  ShadingResolver shadingResolver;
  PostFxRenderer debugRenderer;
  DynamicShaderHelper debugVecRenderer;
};
