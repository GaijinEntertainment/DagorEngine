#pragma once

#include <rendInst/renderPass.h>

#include "riGen/riGenExtra.h"

#include <3d/dag_ringDynBuf.h>
#include <3d/dag_resPtr.h>


namespace rendinst::render
{

inline constexpr uint32_t ADDITIONAL_DATA_IDX = rendinst::RIEXTRA_VECS_COUNT - 1; // It is always last vector

struct VbExtraCtx
{
  RingDynamicSB *vb = nullptr;
  uint32_t gen = 0;
  int lastSwitchFrameNo = 0;
};
extern VbExtraCtx vbExtraCtx[rendinst::RIEX_RENDERING_CONTEXTS];
extern UniqueBufHolder perDrawData;

extern float riExtraLodDistSqMul;
extern float riExtraCullDistSqMul;
extern float additionalRiExtraLodDistMul;
extern float riExtraLodsShiftDistMul;
extern float riExtraMulScale;


struct RenderElem
{
  ShaderElement *shader;
  short vstride; // can be 8 bit
  uint8_t vbIdx, drawOrder;
  int si, numf, baseVertex;
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

void on_ri_mesh_relems_updated(const RenderableInstanceLodsResource *r);
void on_ri_mesh_relems_updated_pool(uint32_t poolId);
void updateShaderElems(uint32_t poolI);

void termElems();

void update_per_draw_gathered_data(uint32_t id);

void renderRIGenExtra(const RiGenVisibility &v, RenderPass render_pass, OptimizeDepthPass optimization_depth_pass,
  OptimizeDepthPrepass optimization_depth_prepass, IgnoreOptimizationLimits ignore_optimization_instances_limits, LayerFlag layer,
  uint32_t instance_count_mul, TexStreamingContext texCtx, AtestStage atest_stage = AtestStage::All,
  const RiExtraRenderer *riex_renderer = nullptr);

void renderRIGenExtraSortedTransparentInstanceElems(const RiGenVisibility &v, const TexStreamingContext &texCtx);

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
