//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <rendInst/constants.h>
#include <rendInst/renderPass.h>
#include <rendInst/layerFlags.h>
#include <3d/dag_texStreamingContext.h>
#include <rendInst/riExtraRenderer.h>


struct RiGenVisibility;
class Occlusion;
class ShaderElement;

namespace rendinst::render
{

inline constexpr int MAX_LOD_COUNT_WITH_ALPHA = rendinst::MAX_LOD_COUNT + 1;
inline constexpr int GPU_INSTANCING_OFSBUFFER_TEXREG = 11;
inline constexpr int TREECROWN_TEXREG = 16;

extern bool avoidStaticShadowRecalc;
extern bool useConditionalRendering;
extern bool debugLods;
extern void setApexInstancing();
extern bool tmInstColored;
extern bool impostorPreshadowNeedUpdate;
extern float riExtraMinSizeForReflection;
extern float riExtraMinSizeForDraftDepth;
extern int instancingTexRegNo;

void useRiDepthPrepass(bool use);
void useRiCellsDepthPrepass(bool use);
void useImpostorDepthPrepass(bool use);

void setRIGenRenderMode(int mode);
int getRIGenRenderMode();

bool enableSecLayerRender(bool en);
bool enablePrimaryLayerRender(bool en);
bool enableRiExtraRender(bool en);
bool isSecLayerRenderEnabled();

void before_draw(RenderPass render_pass, const RiGenVisibility *visibility, const Frustum &frustum, const Occlusion *occlusion,
  const char *mission_name = "", const char *map_name = "", bool gpu_instancing = false);

// autocomputes visibility
void renderRIGen(RenderPass render_pass, mat44f_cref globtm, const Point3 &view_pos, const TMatrix &view_itm,
  LayerFlags layer_flags = LayerFlag::Opaque, bool for_vsm = false, TexStreamingContext texCtx = TexStreamingContext(0));
void renderRIGen(RenderPass render_pass, const RiGenVisibility *visibility, const TMatrix &view_itm, LayerFlags layer_flags,
  OptimizeDepthPass depth_optimized = OptimizeDepthPass::No, uint32_t instance_count_mul = 1, AtestStage atest_stage = AtestStage::All,
  const RiExtraRenderer *riex_renderer = nullptr, TexStreamingContext texCtx = TexStreamingContext(0));
void renderGpuObjectsFromVisibility(RenderPass render_pass, const RiGenVisibility *visibility, LayerFlags layer_flags);
void renderRIGenOptimizationDepth(RenderPass render_pass, const RiGenVisibility *visibility, const TMatrix &view_itm,
  IgnoreOptimizationLimits ignore_optimization_instances_limits = IgnoreOptimizationLimits::No, SkipTrees skip_trees = SkipTrees::No,
  RenderGpuObjects gpu_objects = RenderGpuObjects::No, uint32_t instance_count_mul = 1,
  TexStreamingContext texCtx = TexStreamingContext(0));
void renderRITreeDepth(const RiGenVisibility *visibility, const TMatrix &view_itm);

bool renderRIGenClipmapShadowsToTextures(const Point3 &sunDir0, bool for_sli, bool force_update = true);
bool notRenderedClipmapShadowsBBox(BBox2 &box, int cascadeNo);
bool notRenderedStaticShadowsBBox(BBox3 &box);
void setClipmapShadowsRendered(int cascadeNo);
void renderRIGenShadowsToClipmap(const BBox2 &region, int renderNewForCascadeNo); //-1 - render all, not only new
bool renderRIGenGlobalShadowsToTextures(const Point3 &sunDir0, bool force_update = true, bool use_compression = true);
// compatibility
void renderRendinstShadowsToTextures(const Point3 &sunDir0);

void renderDebug();

}; // namespace rendinst::render
