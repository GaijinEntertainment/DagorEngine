// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <rendInst/renderPass.h>

#include "render/drawOrder.h"
#include "riGen/riGenExtra.h"

#include <EASTL/unique_ptr.h>
#include <3d/dag_ringDynBuf.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_multidrawContext.h>


namespace rendinst::render
{

extern MultidrawContext<rendinst::RiExPerInstanceParameters> riExMultidrawContext;

inline constexpr uint32_t ADDITIONAL_DATA_IDX = rendinst::RIEXTRA_VECS_COUNT - 1; // It is always last vector

struct VbExtraCtx
{
  eastl::unique_ptr<RingDynamicSB> vb;
  uint32_t gen = 0;
  int lastSwitchFrameNo = 0;
};
extern carray<rendinst::render::VbExtraCtx, RI_EXTRA_VB_CTX_CNT> vbExtraCtx;
extern UniqueBuf riExtraPerDrawData;

extern float riExtraLodDistSqMul;
extern float riExtraCullDistSqMul;
extern float additionalRiExtraLodDistMul;
extern float riExtraLodsShiftDistMul;
extern float riExtraLodsShiftDistMulForCulling;
extern float riExtraMulScale;
extern bool canIncreaseRenderBuffer;


struct RenderElem
{
  ShaderElement *shader;
  short vstride; // can be 8 bit
  uint8_t vbIdx;
  PackedDrawOrder drawOrder;
  uint8_t primitive;
  int si, sv, numv, numf, baseVertex;
};
extern dag::Vector<RenderElem> allElems;
extern dag::Vector<uint32_t> allElemsIndex;
extern int pendingRebuildCnt;
extern bool relemsForGpuObjectsHasRebuilded;
void rebuildAllElemsInternal();
inline void rebuildAllElems()
{
  if (interlocked_acquire_load(pendingRebuildCnt))
    rebuildAllElemsInternal();
}

void allocateRIGenExtra(rendinst::render::VbExtraCtx &vbctx);

extern CallbackToken meshRElemsUpdatedCbToken;

void on_ri_mesh_relems_updated(const RenderableInstanceLodsResource *r, bool, int);
void on_ri_mesh_relems_updated_pool(uint32_t poolId);
void updateShaderElems(uint32_t poolI);

void termElems();

void update_per_draw_gathered_data(uint32_t id);

void renderRIGenExtra(const RiGenVisibility &v, RenderPass render_pass, OptimizeDepthPass optimization_depth_pass,
  OptimizeDepthPrepass optimization_depth_prepass, IgnoreOptimizationLimits ignore_optimization_instances_limits, LayerFlag layer,
  uint32_t instance_count_mul, TexStreamingContext texCtx, AtestStage atest_stage = AtestStage::All,
  const RiExtraRenderer *riex_renderer = nullptr);

void renderRIGenExtraSortedTransparentInstanceElems(const RiGenVisibility &v, const TexStreamingContext &texCtx,
  bool draw_partitioned_elems = false);

void write_ri_extra_per_instance_data(vec4f *dest_buffer, const RendinstTiledScene &tiled_scene, scene::pool_index pool_id,
  scene::node_index ni, mat44f_cref m, bool is_dynamic);

uint32_t get_per_draw_offset(scene::pool_index pool_id);
} // namespace rendinst::render

template <int N>
inline int find_lod(const float *__restrict lod_dists, float dist)
{
  dist *= rendinst::render::riExtraLodsShiftDistMul;
  for (int lod = 0; lod <= N - 1; lod++)
    if (lod_dists[lod] < dist)
      return lod;
  return N;
}

template <>
inline int find_lod<4>(const float *__restrict lod_dists, float dist)
{
  dist *= rendinst::render::riExtraLodsShiftDistMul;
  if (dist < lod_dists[1])
    return (dist < lod_dists[0] ? 0 : 1);
  return (dist < lod_dists[2] ? 2 : 3);
}

template <>
inline int find_lod<8>(const float *__restrict lod_dists, float dist)
{
  dist *= rendinst::render::riExtraLodsShiftDistMul;
  if (dist < lod_dists[3])
  {
    if (dist < lod_dists[1])
      return (dist < lod_dists[0] ? 0 : 1);
    else
      return (dist < lod_dists[2] ? 2 : 3);
  }
  else
  {
    if (dist < lod_dists[5])
      return (dist < lod_dists[4] ? 4 : 5);
    else
      return (dist < lod_dists[6] ? 6 : 7);
  }
}

__forceinline vec4f make_pos_and_rad(mat44f_cref tm, vec4f center_and_rad)
{
  vec4f pos = v_mat44_mul_vec3p(tm, center_and_rad);
  vec4f maxScale = v_max(v_length3_est(tm.col0), v_max(v_length3_est(tm.col1), v_length3_est(tm.col2)));
  vec4f bb_ext = v_mul(v_splat_w(center_and_rad), maxScale);
  return v_perm_xyzd(pos, bb_ext);
}
