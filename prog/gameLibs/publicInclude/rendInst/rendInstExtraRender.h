//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <rendInst/renderPass.h>
#include <rendInst/layerFlags.h>
#include <rendInst/constants.h>
#include <rendInst/riExtraRenderer.h>

#include <3d/dag_drv3d_buffers.h>
#include <3d/dag_texStreamingContext.h>
#include <shaders/dag_shaders.h>
#include <generic/dag_span.h>

#include <EASTL/vector.h>
#include <EASTL/vector_map.h>
#include <EASTL/string_view.h>


struct RiGenVisibility;

namespace rendinst::render
{

using ri_extra_render_marker_cb = void (*)(bool begin);
void registerRIGenExtraRenderMarkerCb(ri_extra_render_marker_cb cb);
bool unregisterRIGenExtraRenderMarkerCb(ri_extra_render_marker_cb cb);

void setRIGenExtraDiffuseTexture(uint16_t ri_idx, int tex_var_id);

void ensureElemsRebuiltRIGenExtra(bool gpu_instancing);
void renderRIGenExtraFromBuffer(Sbuffer *buffer, dag::ConstSpan<IPoint2> offsets_and_count, dag::ConstSpan<uint16_t> ri_indices,
  dag::ConstSpan<uint32_t> lod_offsets, RenderPass render_pass, OptimizeDepthPass optimization_depth_pass,
  OptimizeDepthPrepass optimization_depth_prepass, IgnoreOptimizationLimits ignore_optimization_instances_limits, LayerFlag layer,
  ShaderElement *shader_override = nullptr, uint32_t instance_multiply = 1, bool gpu_instancing = false,
  Sbuffer *indirect_buffer = nullptr, Sbuffer *ofs_buffer = nullptr);

void renderSortedTransparentRiExtraInstances(const RiGenVisibility &v, const TexStreamingContext &tex_ctx);

void collectPixelsHistogramRIGenExtra(const RiGenVisibility *main_visibility, vec4f camera_fov, float histogram_scale,
  eastl::vector<eastl::vector_map<eastl::string_view, int>> &histogram);

void validateFarLodsWithHeavyShaders(const RiGenVisibility *main_visibility, vec4f camera_fov, float histogram_scale);

// NOTE: this actually only affects RI extra
void reinitOnShadersReload();

void invalidateRIGenExtraShadowsVisibility();
void invalidateRIGenExtraShadowsVisibilityBox(bbox3f_cref box);

void prepareToStartAsyncRIGenExtraOpaqueRender(RiGenVisibility &vis, int frame_stblk, TexStreamingContext texCtx, bool enable = true);
void startPreparedAsyncRIGenExtraOpaqueRender(RiGenVisibility &vis, bool wake = true);
void waitAsyncRIGenExtraOpaqueRenderVbFill(const RiGenVisibility *vis);
RiExtraRenderer *waitAsyncRIGenExtraOpaqueRender(const RiGenVisibility *vis = nullptr);

} // namespace rendinst::render
