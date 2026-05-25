// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix4.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaders.h>
#include <util/dag_convar.h>
#include <util/dag_string.h>
#include <render/debugGbuffer.h>
#include <render/deferredRenderer.h>
#include <render/resolve_gbuffer_compute_inc.hlsli>
#include <render/viewVecs.h>

#define DEFERRED_RENDERER_VARS_LIST VAR(gbuffertm)

#define VAR(a) static int a##VarId = -1;
DEFERRED_RENDERER_VARS_LIST
#undef VAR

ShadingResolver::ShadingResolver(const char *resolve_pshader_name, const char *resolve_cshader_name)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  DEFERRED_RENDERER_VARS_LIST
#undef VAR

  if (resolve_cshader_name)
    resolveShadingCS.reset(new_compute_shader(resolve_cshader_name, true /* optional */));

  if (resolve_pshader_name && !resolveShadingCS)
  {
    resolveShading = eastl::make_unique<PostFxRenderer>();
    resolveShading->init(resolve_pshader_name);
    if (!resolveShading->getElem())
    {
      resolveShading.reset();
      logerr("no shading %s", resolve_pshader_name);
    }
  }
}

ShadingResolver::~ShadingResolver() = default;

void ShadingResolver::resolve(BaseTexture *resolveTarget, const TMatrix &view_tm, const TMatrix4 &proj_tm,
  BaseTexture *depth_bounds_tex, ClearTarget clear_target, const TMatrix4 &gbufferTm, const RectInt *resolve_area)
{
  if (!resolveShading && !resolveShadingCS)
    return;

  set_inv_globtm_to_shader(view_tm, proj_tm, true);
  set_viewvecs_to_shader(view_tm, proj_tm);
  ShaderGlobal::set_float4x4(gbuffertmVarId, gbufferTm);

  TextureInfo info;
  resolveTarget->getinfo(info);

  if (!resolveShadingCS)
  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target({depth_bounds_tex, 0, 0}, depth_bounds_tex ? DepthAccess::SampledRO : DepthAccess::RW,
      {{resolveTarget, 0, 0}});
    if (resolve_area)
    {
      d3d::setview(resolve_area->left, resolve_area->top, resolve_area->right - resolve_area->left,
        resolve_area->bottom - resolve_area->top, 0, 1);
      d3d::setscissor(resolve_area->left, resolve_area->top, resolve_area->right - resolve_area->left,
        resolve_area->bottom - resolve_area->top);
    }

    if (clear_target == ClearTarget::Yes)
      d3d::clearview(CLEAR_TARGET, 0, 0, 0);

    resolveShading->render();
  }
  else
  {
    if (clear_target == ClearTarget::Yes)
    {
      const float zeroes[4] = {0.0f, 0.0f, 0.0f, 0.0f};
      d3d::clear_rwtexf(resolveTarget, zeroes, 0, 0);
      d3d::resource_barrier({resolveTarget, RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
    }

    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), resolveTarget);

    resolveShadingCS->dispatchThreads(info.w, info.h, 1);

    d3d::resource_barrier({resolveTarget, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
  }
}

DeferredRenderTarget::DeferredRenderTarget(const char *resolve_pshader_name, const char *resolve_cshader_name, const char *name_,
  const int w, const int h, DeferredRT::StereoMode stereo_mode, const unsigned msaaFlag, const int num_rt, const unsigned *texFmt,
  const uint32_t depth_fmt) :
  renderTargets(name_, w, h, stereo_mode, msaaFlag, num_rt, texFmt, depth_fmt),
  shadingResolver(resolve_pshader_name, resolve_cshader_name)
{}

DeferredRenderTarget::~DeferredRenderTarget() = default;

void DeferredRenderTarget::resourceBarrier(ResourceBarrier barrier)
{
  for (int i = 0; i < renderTargets.getRtNum(); ++i)
    if (Texture *rt = renderTargets.getRt(i, true))
      d3d::resource_barrier({rt, barrier, 0, 0});
  if (Texture *ds = renderTargets.getDepth())
    d3d::resource_barrier({ds, barrier, 0, 0});
}

void DeferredRenderTarget::resolve(BaseTexture *resolveTarget, const TMatrix &view_tm, const TMatrix4 &proj_tm,
  BaseTexture *depth_bounds_tex, ShadingResolver::ClearTarget clear_target, const TMatrix4 &gbufferTm, const RectInt *resolve_area)
{
  renderTargets.setVar();
  shadingResolver.resolve(resolveTarget, view_tm, proj_tm, depth_bounds_tex, clear_target, gbufferTm, resolve_area);
}

void DeferredRenderTarget::debugRender(BaseTexture *depth, int mode)
{
  if (debugRenderer.getMat() == NULL)
    debugRenderer = PostFxRenderer(DEBUG_RENDER_GBUFFER_SHADER_NAME);
  debug_render_gbuffer(debugRenderer, renderTargets, depth, mode);
}

void DeferredRenderTarget::debugRenderVectors(int mode, int vec_count, int vec_scale)
{
  if (debugVecRenderer.material == NULL)
  {
    debugVecRenderer = DynamicShaderHelper();
    debugVecRenderer.init(DEBUG_RENDER_GBUFFER_WITH_VECTORS_SHADER_NAME, nullptr, 0, DEBUG_RENDER_GBUFFER_WITH_VECTORS_SHADER_NAME,
      true);
  }
  debug_render_gbuffer_with_vectors(debugVecRenderer, renderTargets, mode, vec_count, vec_scale);
}