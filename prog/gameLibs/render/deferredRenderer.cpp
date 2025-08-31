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

CONSOLE_BOOL_VAL("render", debug_tiled_resolve, false);
CONSOLE_BOOL_VAL("render", disable_resolve_classification, false);

#define DEFERRED_RENDERER_VARS_LIST VAR(gbuffertm)

#define VAR(a) static int a##VarId = -1;
DEFERRED_RENDERER_VARS_LIST
#undef VAR

ShadingResolver::ShadingResolver(const char *resolve_pshader_name, const char *resolve_cshader_name, const char *classify_cshader_name,
  const PermutationsDesc &resolve_permutations_desc)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  DEFERRED_RENDERER_VARS_LIST
#undef VAR

  if (resolve_cshader_name)
    resolveShadingCS.reset(new_compute_shader(resolve_cshader_name, true /* optional */));

  if (classify_cshader_name)
  {
    classifyTilesCS.reset(new_compute_shader(classify_cshader_name, true /* optional */));
    prepareArgsCS.reset(new_compute_shader("deferred_shadow_prepare_indirect_args", true /* optional */));
  }

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

  resolvePermutations = resolve_permutations_desc;
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

  // Save current state
  int savedState[MAX_SHADER_VARS];
  bool skipClassification = true;
  for (int j = 0; j < resolvePermutations.varCount; ++j)
  {
    savedState[j] = ShaderGlobal::get_int(resolvePermutations.varIds[j]);
    skipClassification &= (savedState[j] == resolvePermutations.varValuesToSkipTiled[j]);
  }

  skipClassification |= disable_resolve_classification.get();

  TextureInfo info;
  resolveTarget->getinfo(info);

  if (resolveShadingCS && classifyTilesCS && prepareArgsCS && !skipClassification)
  {
    recreateTileBuffersIfNeeded(info.w, info.h);

    { // fill tile buffers
      TIME_D3D_PROFILE(classify_tiles);

      d3d::resource_barrier({tileCounters.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), tileCounters.getBuf());

      for (int i = 0; i < RESOLVE_GBUFFER_MAX_PERMUTATIONS; ++i)
        d3d::set_rwbuffer(STAGE_CS, 1 + i, tileBufs[i].getBuf());

      // Group size != RESOLVE_GBUFFER_TILE_SIZE, but it still has to dispatch a group for each tile
      classifyTilesCS->dispatch((info.w + RESOLVE_GBUFFER_TILE_SIZE - 1) / RESOLVE_GBUFFER_TILE_SIZE,
        (info.h + RESOLVE_GBUFFER_TILE_SIZE - 1) / RESOLVE_GBUFFER_TILE_SIZE, 1);

      for (int i = 0; i < RESOLVE_GBUFFER_MAX_PERMUTATIONS; ++i)
        d3d::set_rwbuffer(STAGE_CS, 1 + i, nullptr);
    }

    d3d::resource_barrier({indirectArguments.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});

    { // prepare indirect arguments
      d3d::resource_barrier({tileCounters.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), tileCounters.getBuf());
      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 1, VALUE), indirectArguments.getBuf());
      prepareArgsCS->dispatch(1, 1, 1);
    }

    { // shade tiles
      TIME_D3D_PROFILE(shade_tiles);

      d3d::resource_barrier({indirectArguments.getBuf(), RB_RO_INDIRECT_BUFFER});

      if (clear_target == ClearTarget::Yes)
      {
        const float zeroes[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        d3d::clear_rwtexf(resolveTarget, zeroes, 0, 0);
        d3d::resource_barrier({resolveTarget, RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
      }

      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), resolveTarget);

      ShaderGlobal::set_int(tiledInvocationVarId, 1);

      const int permutationsCount = (1 << resolvePermutations.varCount);
      d3d::driver_command(Drv3dCommand::DELAY_SYNC);
      for (int i = 0; i < permutationsCount; ++i)
      {
        if (i > 0)
          d3d::resource_barrier({resolveTarget, RB_NONE, 0, 0});
        for (int j = 0; j < resolvePermutations.varCount; ++j)
        {
          if (resolvePermutations.varValues[j][i] != USE_CURRENT_SHADER_VAR_VALUE)
          {
            ShaderGlobal::set_int(resolvePermutations.varIds[j], resolvePermutations.varValues[j][i]);
          }
          else
          {
            ShaderGlobal::set_int(resolvePermutations.varIds[j], savedState[j]);
          }
        }

        d3d::resource_barrier({tileBufs[i].getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
        ShaderGlobal::set_buffer(tileCoordinatesVarId, tileBufs[i]);
        resolveShadingCS->dispatch_indirect(indirectArguments.getBuf(), i * 3 * sizeof(uint32_t));
      }
      d3d::driver_command(Drv3dCommand::CONTINUE_SYNC);
    }

    // Restore saved state
    for (int j = 0; j < resolvePermutations.varCount; ++j)
      ShaderGlobal::set_int(resolvePermutations.varIds[j], savedState[j]);

    if (debug_tiled_resolve)
      debugTiles(resolveTarget);

    d3d::resource_barrier({resolveTarget, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
  }
  else if (!resolveShadingCS)
  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(resolveTarget, 0);
    if (depth_bounds_tex)
      d3d::set_depth(depth_bounds_tex, DepthAccess::SampledRO);
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
    recreateTileBuffersIfNeeded(info.w, info.h);

    if (clear_target == ClearTarget::Yes)
    {
      const float zeroes[4] = {0.0f, 0.0f, 0.0f, 0.0f};
      d3d::clear_rwtexf(resolveTarget, zeroes, 0, 0);
      d3d::resource_barrier({resolveTarget, RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
    }

    STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), resolveTarget);

    ShaderGlobal::set_int(tiledInvocationVarId, 0);
    resolveShadingCS->dispatchThreads(info.w, info.h, 1);

    d3d::resource_barrier({resolveTarget, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
  }
}

void ShadingResolver::recreateTileBuffersIfNeeded(const int w, const int h)
{
  const int new_tiles_w = (w + RESOLVE_GBUFFER_TILE_SIZE - 1) / RESOLVE_GBUFFER_TILE_SIZE;
  const int new_tiles_h = (h + RESOLVE_GBUFFER_TILE_SIZE - 1) / RESOLVE_GBUFFER_TILE_SIZE;
  if (new_tiles_w == tiles_w && new_tiles_h == tiles_h)
  {
    G_ASSERT(tileBufs.size() == RESOLVE_GBUFFER_MAX_PERMUTATIONS);
    return;
  }

  tileBufs.clear();
  G_STATIC_ASSERT(decltype(tileBufs)::kMaxSize >= RESOLVE_GBUFFER_MAX_PERMUTATIONS);
  tileCoordinatesVarId = get_shader_variable_id("tile_coordinates");
  tiledInvocationVarId = get_shader_variable_id("tiled_invocation", true);
  for (int i = 0; i < RESOLVE_GBUFFER_MAX_PERMUTATIONS; ++i)
  {
    eastl::string resource_name;
    resource_name.sprintf("tiles_buffer #%i", i);
    tileBufs.emplace_back(dag::buffers::create_ua_sr_byte_address(new_tiles_w * new_tiles_h, resource_name.c_str()));
  }
  tileCounters = dag::buffers::create_ua_sr_byte_address(RESOLVE_GBUFFER_MAX_PERMUTATIONS, "tile_counters");
  indirectArguments =
    dag::buffers::create_ua_indirect(dag::buffers::Indirect::Dispatch, RESOLVE_GBUFFER_MAX_PERMUTATIONS, "tile_indirect_arguments");

  tiles_w = new_tiles_w;
  tiles_h = new_tiles_h;

  d3d::zero_rwbufi(tileCounters.getBuf());
}

void ShadingResolver::debugTiles(BaseTexture *resolveTarget)
{
  if (!debugTilesCS)
    debugTilesCS.reset(new_compute_shader("deferred_shadow_debug_tiles"));

  d3d::resource_barrier({resolveTarget, RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 1});
  STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), resolveTarget);

  static int debug_colorVarId = get_shader_variable_id("debug_color", true);

  static const Color4 debug_colors[RESOLVE_GBUFFER_MAX_PERMUTATIONS] = {{0.0, 1.0, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}};

  for (int i = 0; i < RESOLVE_GBUFFER_MAX_PERMUTATIONS; ++i)
  {
    ShaderGlobal::set_color4(debug_colorVarId, debug_colors[i]);

    ShaderGlobal::set_buffer(tileCoordinatesVarId, tileBufs[i]);
    debugTilesCS->dispatch_indirect(indirectArguments.getBuf(), i * 3 * sizeof(uint32_t));
  }
}

DeferredRenderTarget::DeferredRenderTarget(const char *resolve_pshader_name, const char *resolve_cshader_name,
  const char *classify_cshader_name, const ShadingResolver::PermutationsDesc &permutations_desc, const char *name_, const int w,
  const int h, DeferredRT::StereoMode stereo_mode, const unsigned msaaFlag, const int num_rt, const unsigned *texFmt,
  const uint32_t depth_fmt) :
  renderTargets(name_, w, h, stereo_mode, msaaFlag, num_rt, texFmt, depth_fmt),
  shadingResolver(resolve_pshader_name, resolve_cshader_name, classify_cshader_name, permutations_desc)
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

void DeferredRenderTarget::debugRender(int mode)
{
  if (debugRenderer.getMat() == NULL)
    debugRenderer = PostFxRenderer(DEBUG_RENDER_GBUFFER_SHADER_NAME);
  debug_render_gbuffer(debugRenderer, renderTargets, mode);
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