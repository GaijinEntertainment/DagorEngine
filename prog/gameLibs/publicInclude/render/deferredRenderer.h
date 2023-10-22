//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_drvDecl.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_texMgr.h>
#include <shaders/dag_postFxRenderer.h>
#include <render/deferredRT.h>
#include <EASTL/array.h>
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

  // defines specific resolve shader permutation for tiled resolve (should be in sync with classify shader)
  static const int USE_CURRENT_SHADER_VAR_VALUE = 0xffff;
  static const int MAX_SHADER_VARS = 1;
  static const int MAX_SHADER_VAR_STATES = 2;
  static const int MAX_SHADER_PERMUTATIONS = (1 << MAX_SHADER_VARS); // (MAX_SHADER_VAR_STATES ^ MAX_SHADER_VARS)
  struct PermutationsDesc
  {
    int varIds[MAX_SHADER_VARS] = {};
    int varCount = 0;
    int varValues[MAX_SHADER_VARS][MAX_SHADER_PERMUTATIONS] = {}; // only integer variables for now
    int varValuesToSkipTiled[MAX_SHADER_VARS] = {};
  };

  ShadingResolver(const char *resolve_pshader_name, const char *resolve_cshader_name, const char *classify_cshader_name,
    const PermutationsDesc &resolve_permutations_desc);
  ShadingResolver(const char *resolve_pshader_name) :
    ShadingResolver(resolve_pshader_name, nullptr /* resolve_cshader_name */, nullptr /* classify_cshader_name */,
      {} /* resolve_permutations_desc */)
  {}
  ~ShadingResolver();

  PostFxRenderer *getResolveShading() const { return resolveShading.get(); }

  void resolve(BaseTexture *resolveTarget, const TMatrix &view_tm, const TMatrix4 &proj_tm, BaseTexture *depth_bounds_tex = nullptr,
    ShadingResolver::ClearTarget clear_target = ShadingResolver::ClearTarget::No, const TMatrix4 &gbufferTm = TMatrix4::IDENT,
    const RectInt *resolve_area = nullptr);

private:
  eastl::unique_ptr<PostFxRenderer> resolveShading;
  eastl::unique_ptr<ComputeShaderElement> resolveShadingCS, classifyTilesCS, prepareArgsCS, debugTilesCS;

  PermutationsDesc resolvePermutations;
  // TODO: try to reduce buffers count (e.g. write from both sides if 2 permutations will be finalized)
  eastl::fixed_vector<UniqueBuf, MAX_SHADER_PERMUTATIONS> tileBufs;
  int tileCoordinatesVarId = -1, tiledInvocationVarId = -1;
  UniqueBuf tileCounters, indirectArguments;
  int tiles_w = -1, tiles_h = -1;

  void recreateTileBuffersIfNeeded(int w, int h);
  void debugTiles(BaseTexture *resolveTarget);
};

class DeferredRenderTarget
{
public:
  DeferredRenderTarget(const char *resolve_pshader_name, const char *resolve_cshader_name, const char *classify_cshader_name,
    const ShadingResolver::PermutationsDesc &resolve_permutations_desc, const char *name, int w, int h,
    DeferredRT::StereoMode stereo_mode, unsigned msaaFlag, int numRt, const unsigned *texFmt, uint32_t depthFmt);
  DeferredRenderTarget(const char *resolve_pshader_name, const char *name, int w, int h, DeferredRT::StereoMode stereo_mode,
    unsigned msaaFlag, int numRt, const unsigned *texFmt, uint32_t depthFmt) :
    DeferredRenderTarget(resolve_pshader_name, nullptr /* resolve_cshader_name */, nullptr /* classify_cshader_name */,
      {} /* resolve_permutations_desc */, name, w, h, stereo_mode, msaaFlag, numRt, texFmt, depthFmt)
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
  void debugRender(int show_gbuffer = USE_DEBUG_GBUFFER_MODE);
  // returns true if 32 bit depth buffer was created
  uint32_t recreateDepth(uint32_t fmt) { return renderTargets.recreateDepth(fmt); }
  int getWidth() const { return renderTargets.getWidth(); }
  int getHeight() const { return renderTargets.getHeight(); }
  Texture *getDepth() const { return renderTargets.getDepth(); }
  TEXTUREID getDepthId() const { return renderTargets.getDepthId(); }
  const ManagedTex &getDepthAll() const { return renderTargets.getDepthAll(); }
  const ManagedTexView getDepthAllView() const { return {getDepthAll()}; }
  PostFxRenderer *getResolveShading() const { return shadingResolver.getResolveShading(); }
  Texture *getRt(uint32_t idx) const { return renderTargets.getRt(idx); }
  TEXTUREID getRtId(uint32_t idx) const { return renderTargets.getRtId(idx); }
  const ManagedTex &getRtAll(uint32_t idx) const { return renderTargets.getRtAll(idx); }
  ManagedTexView getRtAllView(uint32_t idx) const { return {getRtAll(idx)}; }
  uint32_t getRtNum() const { return renderTargets.getRtNum(); }

protected:
  DeferredRT renderTargets;
  ShadingResolver shadingResolver;
  PostFxRenderer debugRenderer;
};
