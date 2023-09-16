#pragma once

#include "riGen/extraRender/riExRenderRecord.h"
#include "riGen/riGenExtra.h"
#include "riGen/riGenRender.h"
#include <rendInst/renderPass.h>

#include <memory/dag_framemem.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <3d/dag_drvDecl.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <shaders/dag_shStateBlockBindless.h>
#include <render/debugMesh.h>


// Tools don't have depth prepass for trees.
#define USE_DEPTH_PREPASS_FOR_TREES !(_TARGET_PC && !_TARGET_STATIC_LIB)

template <class>
class DynVariantsCache;

namespace rendinst
{

static constexpr uint32_t RI_RES_ORDER_COUNT_SHIFT = 14, RI_RES_ORDER_COUNT_MASK = (1 << RI_RES_ORDER_COUNT_SHIFT) - 1;

template <typename Alloc, class DynamicVariantsPolicy>
class RiExtraRendererT : public DynamicVariantsPolicy //-V730
{
  // TODO: make different structs
  dag::Vector<RIExRenderRecord, Alloc, false> list;
  dag::Vector<RIExRenderRecord, Alloc, false> multidrawList;
  uint32_t startStage, endStage;
  uint32_t layer;
  uint32_t instanceCountMultiply = 1;
  bool isDepthPass;
  bool optimizeDepthPass;
  bool isVoxelizationPass;
  bool isDecalPass;

  struct PackedDrawCallParams
  {
    uint32_t drawCallsCount;
    uint32_t instancesCount;
  };
  dag::Vector<PackedDrawCallParams, Alloc, false> combinedDrawcallSizes;
  dag::Vector<DrawIndexedIndirectArgs, Alloc, false> indirectDrawcalls;
  ska::flat_hash_map<uint32_t, uint16_t, eastl::hash<uint32_t>, eastl::equal_to<uint32_t>, Alloc> bindlessStatesToUpdateTexLevels;
  dag::Vector<PerInstanceParameters, Alloc, false> drawcallIds;

public:
  RiExtraRendererT() = default;
  template <typename... Args>
  RiExtraRendererT(Args &&...args)
  {
    init(eastl::forward<Args>(args)...);
  }
  void init(const int size_to_reserve, uint32_t layer_, RenderPass render_pass,
    OptimizeDepthPrepass optimization_depth_prepass = OptimizeDepthPrepass::No,
    OptimizeDepthPass optimization_depth_pass = OptimizeDepthPass::No,
    IgnoreOptimizationLimits ignore_optimization_instances_limits = IgnoreOptimizationLimits::No, uint32_t count_multiply = 1)
  {
    layer = layer_, instanceCountMultiply = eastl::max(1u, count_multiply);
    isDepthPass = render_pass == rendinst::RenderPass::ToShadow || render_pass == rendinst::RenderPass::Depth;
    optimizeDepthPass = isDepthPass && optimization_depth_pass == rendinst::OptimizeDepthPass::Yes;
    isVoxelizationPass = (render_pass == rendinst::RenderPass::VoxelizeAlbedo || render_pass == rendinst::RenderPass::Voxelize3d);
    isDecalPass = layer == rendinst::LAYER_DECALS;

    list.clear();
    list.reserve(size_to_reserve);
    multidrawList.clear();
    combinedDrawcallSizes.clear();
    indirectDrawcalls.clear();
    bindlessStatesToUpdateTexLevels.clear();
    drawcallIds.clear();
    if (layer == rendinst::LAYER_TRANSPARENT)
      startStage = endStage = ShaderMesh::STG_trans;
    else if (layer == rendinst::LAYER_DECALS)
      startStage = endStage = ShaderMesh::STG_decal;
    else if (layer == rendinst::LAYER_DISTORTION)
      startStage = endStage = ShaderMesh::STG_distortion;
    else
    {
      startStage = ShaderMesh::STG_opaque;
      if (optimization_depth_prepass == rendinst::OptimizeDepthPrepass::Yes &&
          ignore_optimization_instances_limits == rendinst::IgnoreOptimizationLimits::No)
        endStage = ShaderMesh::STG_opaque;
      else
        endStage = (render_pass == rendinst::RenderPass::Normal) ? ShaderMesh::STG_imm_decal : ShaderMesh::STG_atest;
    }
  }

  inline void setNewStages(uint32_t new_start_stage, uint32_t new_end_stage)
  {
    startStage = new_start_stage;
    endStage = new_end_stage;
  }

  enum
  {
    USE_GPU_INSTANCING = true,
    NO_GPU_INSTANCING = false
  };

  void coalesceDrawcalls()
  {
    TIME_D3D_PROFILE(ri_extra_coalesce_drawcalls);

    const auto mergeComparator = [](const RIExRenderRecord &a, const RIExRenderRecord &b) -> bool {
      return a.isTree == b.isTree && a.drawOrder_stage == b.drawOrder_stage && a.vstride == b.vstride && a.vbIdx == b.vbIdx &&
             a.rstate == b.rstate && get_material_id(a.cstate) == get_material_id(b.cstate) && a.prog == b.prog;
    };

    indirectDrawcalls.push_back(DrawIndexedIndirectArgs{(uint32_t)multidrawList[0].numf * 3,
      multidrawList[0].ofsAndCnt.y * instanceCountMultiply, (uint32_t)multidrawList[0].si, multidrawList[0].bv, 0});
    drawcallIds.push_back(PerInstanceParameters{get_material_offset(multidrawList[0].cstate), (uint32_t)multidrawList[0].ofsAndCnt.x});
    combinedDrawcallSizes.push_back(PackedDrawCallParams{1, multidrawList[0].ofsAndCnt.y * instanceCountMultiply});
    bindlessStatesToUpdateTexLevels.emplace(multidrawList[0].cstate, multidrawList[0].texLevel);

    for (uint32_t i = 1, ie = multidrawList.size(); i < ie; ++i)
    {
      const auto &currentRelem = multidrawList[i];
      const bool mergeWithPrevious = mergeComparator(currentRelem, multidrawList[i - 1]);
      indirectDrawcalls.push_back(DrawIndexedIndirectArgs{(uint32_t)currentRelem.numf * 3,
        currentRelem.ofsAndCnt.y * instanceCountMultiply, (uint32_t)currentRelem.si, currentRelem.bv, 0});
      drawcallIds.push_back(PerInstanceParameters{get_material_offset(currentRelem.cstate), (uint32_t)currentRelem.ofsAndCnt.x});
      if (mergeWithPrevious)
      {
        combinedDrawcallSizes.back().drawCallsCount++;
        combinedDrawcallSizes.back().instancesCount += currentRelem.ofsAndCnt.y * instanceCountMultiply;
      }
      else
        combinedDrawcallSizes.push_back(PackedDrawCallParams{1, currentRelem.ofsAndCnt.y * instanceCountMultiply});
      auto iter = bindlessStatesToUpdateTexLevels.find(currentRelem.cstate);
      if (iter == bindlessStatesToUpdateTexLevels.end())
        bindlessStatesToUpdateTexLevels.emplace(currentRelem.cstate, currentRelem.texLevel);
      else
        iter->second = max(iter->second, currentRelem.texLevel);
    }
  }

  void updateDataForPackedRender() const
  {
    G_ASSERT(indirectDrawcalls.empty() == drawcallIds.empty());
    if (indirectDrawcalls.empty())
      return;

    TIME_D3D_PROFILE(ri_extra_update_indirect_data);
    for (auto stateIdTexLevel : bindlessStatesToUpdateTexLevels)
      update_bindless_state(stateIdTexLevel.first, stateIdTexLevel.second);

    uint32_t startDrawId = rendinstgenrender::indirectDrawCallIds.update(drawcallIds.data(), drawcallIds.size());
    rendinstgenrender::indirectDrawCalls.fillBuffer(make_span_const(indirectDrawcalls), startDrawId);
  }

  inline void renderSortedMeshesPacked(dag::ConstSpan<uint16_t> riResOrder) const
  {
    G_UNUSED(riResOrder);
    if (!unitedvdata::riUnitedVdata.getResCount() || !unitedvdata::riUnitedVdata.getIB())
      return;

    if (multidrawList.empty())
      return;

    updateDataForPackedRender();

    TIME_D3D_PROFILE(ri_extra_render_sorted_meshes_indirect);

    d3d_err(d3d::setind(unitedvdata::riUnitedVdata.getIB()));

    rendinstgenrender::RiShaderConstBuffers cb;
    cb.setInstancing(0, 4, 0, true);
    cb.flushPerDraw();

#if USE_DEPTH_PREPASS_FOR_TREES
    shaders::OverrideStateId previousOverrideId = shaders::overrides::get_current();
    bool currentDepthPrepass = false;
    const bool validOverride = shaders::overrides::get(previousOverrideId).bits == shaders::OverrideState::CULL_NONE ||
                               shaders::overrides::get(previousOverrideId).bits == 0;
    G_UNUSED(validOverride);
#endif

    uint32_t currentDcOffset = 0;
    uint32_t currentInstOffset = 0;
    for (const auto dcParams : combinedDrawcallSizes)
    {
      const auto &rl = multidrawList[currentDcOffset];
      Vbuffer *vb = unitedvdata::riUnitedVdata.getVB(rl.vbIdx);
      d3d_err(d3d::setvsrc(0, vb, rl.vstride));

#if USE_DEPTH_PREPASS_FOR_TREES
      rendinst::RiExtraPool &riPool = rendinst::riExtra[riResOrder[rl.poolOrder] & RI_RES_ORDER_COUNT_MASK];
      const bool needDepthPrepass =
        (!isVoxelizationPass && !isDepthPass && !isDecalPass && riPool.isTree && rendinstgenrender::use_ri_depth_prepass);

      if (needDepthPrepass != currentDepthPrepass)
      {
        G_ASSERTF(validOverride, "Only disabled backface culling is available override here in colored pass");
        currentDepthPrepass = needDepthPrepass;
        shaders::overrides::reset();
        if (needDepthPrepass)
          shaders::overrides::set(rendinstgenrender::afterDepthPrepassOverride);
        else
          shaders::overrides::set(previousOverrideId);
      }
#endif
      rl.curShader->setReqTexLevel(rl.texLevel);
      set_states_for_variant(rl.curShader->native(), rl.cv, rl.prog, rl.state & ~rl.DISABLE_OPTIMIZATION_BIT_STATE);

      d3d::multi_draw_indexed_indirect(PRIM_TRILIST, rendinstgenrender::indirectDrawCalls.getBuffer(), dcParams.drawCallsCount,
        rendinstgenrender::indirectDrawCalls.getStride(), currentDcOffset * rendinstgenrender::indirectDrawCalls.getStride());

      currentInstOffset += dcParams.instancesCount;
      currentDcOffset += dcParams.drawCallsCount;
    }
#if USE_DEPTH_PREPASS_FOR_TREES
    if (currentDepthPrepass)
    {
      shaders::overrides::reset();
      shaders::overrides::set(previousOverrideId);
    }
#endif
  }


  template <bool gpu_instancing>
  void renderSortedMeshes(dag::ConstSpan<uint16_t> riResOrder, Sbuffer *indirect_buffer = nullptr) const
  {
    G_UNUSED(riResOrder);
    if (!unitedvdata::riUnitedVdata.getResCount() || !unitedvdata::riUnitedVdata.getIB())
      return;

    TIME_D3D_PROFILE(ri_extra_render_sorted_meshes);

#if _TARGET_C1 | _TARGET_C2 // workaround: flush caches for indirect args

#endif

    d3d_err(d3d::setind(unitedvdata::riUnitedVdata.getIB()));

    rendinstgenrender::RiShaderConstBuffers cb;
    cb.setInstancing(0, 4, 0, true);
    cb.flushPerDraw();

    int cVbIdx = -1, cStride = 0;
    IPoint2 curOfsAndCnt(-1, -1);
    shaders::RenderStateId curRstate;
#if USE_DEPTH_PREPASS_FOR_TREES
    shaders::OverrideStateId previousOverrideId = shaders::overrides::get_current();
    bool currentDepthPrepass = false;
    const bool validOverride = shaders::overrides::get(previousOverrideId).bits == shaders::OverrideState::CULL_NONE ||
                               shaders::overrides::get(previousOverrideId).bits == 0;
    G_UNUSED(validOverride);
#endif
    bool currentTessellationState = false;

    for (auto &rl : list)
    {
      bool skipApply = false;
      if (cVbIdx != rl.vbIdx || cStride != rl.vstride)
      {
        cVbIdx = rl.vbIdx;
        cStride = rl.vstride;
        Vbuffer *vb = unitedvdata::riUnitedVdata.getVB(cVbIdx);
        d3d_err(d3d::setvsrc(0, vb, cStride));
      }
      if (optimizeDepthPass && (rl.drawOrder_stage & rl.STAGE_MASK) == ShaderMesh::STG_opaque && curRstate == rl.rstate &&
          !(rl.state & RIExRenderRecord::DISABLE_OPTIMIZATION_BIT_STATE))
        skipApply = true; // we only switch renderstate - zbias,culling, etc

      const bool isTessellated = rl.isTessellated;
      if (isTessellated != currentTessellationState)
      {
        currentTessellationState = isTessellated;
        skipApply = false;
      }

      rendinst::RiExtraPool &riPool = rendinst::riExtra[riResOrder[rl.poolOrder] & RI_RES_ORDER_COUNT_MASK];
#if USE_DEPTH_PREPASS_FOR_TREES

      const bool needDepthPrepass =
        (!isVoxelizationPass && !isDepthPass && !isDecalPass && riPool.isTree && rendinstgenrender::use_ri_depth_prepass);
      if (needDepthPrepass != currentDepthPrepass)
      {
        G_ASSERTF(validOverride, "Only disabled backface culling is available override here in colored pass");
        currentDepthPrepass = needDepthPrepass;
        shaders::overrides::reset();
        if (needDepthPrepass)
          shaders::overrides::set(rendinstgenrender::afterDepthPrepassOverride);
        else
          shaders::overrides::set(previousOverrideId);
      }
#endif

      if (debug_mesh::set_debug_value(rl.lod))
        skipApply = false; // stencil must be applied for lod coloring, no matter what

      rl.curShader->setReqTexLevel(rl.texLevel);

      if (!skipApply)
        set_states_for_variant(rl.curShader->native(), rl.cv, rl.prog, rl.state & ~rl.DISABLE_OPTIMIZATION_BIT_STATE);

      IPoint2 ofsAndCnt = rl.ofsAndCnt;
      const bool setInstancing = curOfsAndCnt.x != ofsAndCnt.x;
      if ((setInstancing || gpu_instancing) && (globalRendinstRenderTypeVarId < 0))
      {
        d3d::set_immediate_const(STAGE_VS, (uint32_t *)&ofsAndCnt.x, 1);
        curOfsAndCnt = ofsAndCnt;
      }
      curRstate = rl.rstate;
      if (gpu_instancing)
        d3d::draw_indexed_indirect(PRIM_TRILIST, indirect_buffer, sizeof(uint32_t) * DRAW_INDEXED_INDIRECT_NUM_ARGS * ofsAndCnt.x);
      else
      {
        bool isImpostor = rl.lod == RiExtraPool::MAX_LODS - 1;
        if (riPool.isTree && !isImpostor)
        {
          const auto tcConsts = rendinstgenrender::getCommonImmediateConstants();
          uint32_t immediateConsts[] = {0u, tcConsts[0], tcConsts[1]};
          d3d::set_immediate_const(STAGE_PS, immediateConsts, sizeof(immediateConsts) / sizeof(immediateConsts[0]));
        }
        d3d::drawind_instanced(PRIM_TRILIST, rl.si, rl.numf, rl.bv, ofsAndCnt.y * instanceCountMultiply,
          // use base instance value if shaders have interval for that
          globalRendinstRenderTypeVarId >= 0 ? ofsAndCnt.x / RIEXTRA_VECS_COUNT : 0);
        if (riPool.isTree && !isImpostor)
        {
          d3d::set_immediate_const(STAGE_PS, nullptr, 0);
        }
      }

      if (rl.state & RIExRenderRecord::DISABLE_OPTIMIZATION_BIT_STATE)
        curRstate = shaders::RenderStateId();
    }
#if USE_DEPTH_PREPASS_FOR_TREES
    if (currentDepthPrepass)
    {
      shaders::overrides::reset();
      shaders::overrides::set(previousOverrideId);
    }
#endif
    debug_mesh::reset_debug_value();
  }

  inline void sortMeshesByMaterial()
  {
    TIME_D3D_PROFILE(ri_extra_sort_meshes);
    stlsort::sort(list.begin(), list.end(), [&](const auto &a, const auto &b) {
      // const RIExRenderRecord & a = list[ai], &b = list[bi];
      if (a.drawOrder_stage != b.drawOrder_stage)
        return a.drawOrder_stage < b.drawOrder_stage;
      if (a.vstride != b.vstride)
        return a.vstride < b.vstride;
      if (a.vbIdx != b.vbIdx)
        return a.vbIdx < b.vbIdx;
      if (a.isTessellated != b.isTessellated)
        return a.isTessellated < b.isTessellated;

      if (!isDepthPass || (a.drawOrder_stage & a.STAGE_MASK) != ShaderMesh::STG_opaque) // opaque stage is all of one shader anyway
      {
        if (a.state != b.state) // maybe split state into sampler state (heavy) and const buffer (cheap)?
        {
          if (a.tstate != b.tstate) // maybe split state into sampler state (heavy) and const buffer (cheap)?
            return a.tstate < b.tstate;
          if (a.rstate != b.rstate)
            return a.rstate < b.rstate;
          return a.state < b.state;
        }
        if (a.prog != b.prog)
          return a.prog < b.prog;
      }
      else
      {
        if (a.rstate != b.rstate)
          return a.rstate < b.rstate;
      }

      if (a.poolOrder != b.poolOrder)
        return a.poolOrder < b.poolOrder;
      return a.ofsAndCnt.x < b.ofsAndCnt.x;
    });

    if (multidrawList.empty())
      return;

    stlsort::sort(multidrawList.begin(), multidrawList.end(), [&](const auto &a, const auto &b) {
      // When we will be GPU bound we can move this check after pools check.
      // It will increase amount of drawcalls (maybe 2 times which is still not a lot), but GPU time could be significantly improved
      // (depends on exact scene).
      if (get_material_id(a.cstate) != get_material_id(b.cstate))
        return get_material_id(a.cstate) > get_material_id(b.cstate);
      // const RIExRenderRecord & a = list[ai], &b = list[bi];
      if (a.drawOrder_stage != b.drawOrder_stage)
        return a.drawOrder_stage < b.drawOrder_stage;
      if (a.vstride != b.vstride)
        return a.vstride < b.vstride;
      if (a.vbIdx != b.vbIdx)
        return a.vbIdx < b.vbIdx;

      if (!isDepthPass || (a.drawOrder_stage & a.STAGE_MASK) != ShaderMesh::STG_opaque) // opaque stage is all of one shader anyway
      {
        if (a.rstate != b.rstate)
          return a.rstate < b.rstate;
        if (a.prog != b.prog)
          return a.prog < b.prog;
        if (a.isTree != b.isTree)
          return a.isTree < b.isTree;
      }
      else
      {
        if (a.rstate != b.rstate)
          return a.rstate < b.rstate;
      }

      if (a.poolOrder != b.poolOrder)
        return a.poolOrder < b.poolOrder;
      return a.ofsAndCnt.x < b.ofsAndCnt.x;
    });

    coalesceDrawcalls();
  }

  inline void addObjectToRender(uint16_t ri_idx, int optimizationInstances, bool optimization_depth_prepass,
    bool ignore_optimization_instances_limits, IPoint2 ofsAndCnt, int lod, uint16_t pool_order, const TexStreamingContext &texCtx,
    float dist2, const ShaderElement *shader_override = nullptr, bool gpu_instancing = false)
  {
    if (should_hide_ri_extra_object_with_id(ri_idx))
      return;

    rendinst::RiExtraPool &riPool = rendinst::riExtra[ri_idx];
    if (layer == rendinst::LAYER_RENDINST_CLIPMAP_BLEND && !riPool.usingClipmap) // to be removed from here!
      return;
    // rendinsts patching heightmap are rendered only with rendinst::LAYER_RENDINST_HEIGHTMAP_PATCH layer
    if (riPool.patchesHeightmap ^ (layer == rendinst::LAYER_RENDINST_HEIGHTMAP_PATCH))
      return;
    const bool renderBrokenTreesToDepth = isDepthPass && riPool.isTree && rendinstgenrender::use_ri_depth_prepass;
    if (optimization_depth_prepass && !ignore_optimization_instances_limits && !optimizationInstances && !renderBrokenTreesToDepth)
      return;
    int count = ofsAndCnt.y;
    if (optimization_depth_prepass && !ignore_optimization_instances_limits && !renderBrokenTreesToDepth)
      count = min(count, optimizationInstances);
    if (riPool.res != nullptr)
    {
      if (count > 0)
        riPool.res->updateReqLod(min<int>(lod, riPool.res->lods.size() - 1));
      if (lod < riPool.res->getQlBestLod())
        lod = riPool.res->getQlBestLod();
    }
    else
      logerr("Empty resource for pool %d. %d pools in total", ri_idx, rendinst::riExtra.size());
    uint32_t startEIOfs = (lod * rendinst::riExtra.size() + ri_idx) * ShaderMesh::STG_COUNT + startStage;

    // Flag shaders and
    // Tree shaders are generally incompatible by VS with other RI shaders and with each other.
    const bool disableOptimization = riPool.isTree || riPool.hasDynamicDisplacement;
    const uint32_t disableOptimizationBit = disableOptimization ? RIExRenderRecord::DISABLE_OPTIMIZATION_BIT_STATE : 0;

    int texLevel = texCtx.getTexLevel(rendinst::riExtra[ri_idx].res->getTexScale(lod), dist2);

#if USE_DEPTH_PREPASS_FOR_TREES
    uint32_t correctedEndStage = renderBrokenTreesToDepth ? ShaderMesh::STG_atest : endStage;
#else
    uint32_t correctedEndStage = endStage;
#endif
#if DAGOR_DBGLEVEL > 0
    int counter = lod;
    if (debug_mesh::is_enabled(debug_mesh::Type::drawElements))
    {
      counter = 0;
      uint32_t startEIOfsTmp = startEIOfs;
      for (unsigned int stage = startStage; stage <= correctedEndStage; stage++, startEIOfsTmp++)
        for (uint32_t EI = rendinst::allElemsIndex[startEIOfsTmp], endEI = rendinst::allElemsIndex[startEIOfsTmp + 1]; EI < endEI;
             ++EI)
        {
          auto &elem = rendinst::allElems[EI];
          if (!elem.shader)
            continue;
          counter++;
        }
    }
#endif
    for (unsigned int stage = startStage; stage <= correctedEndStage; stage++, startEIOfs++)
      for (uint32_t EI = rendinst::allElemsIndex[startEIOfs], endEI = rendinst::allElemsIndex[startEIOfs + 1]; EI < endEI; ++EI)
      {
        const auto &elem = rendinst::allElems[EI];
        if (!elem.shader)
          continue;
        uint32_t prog, state, cstate, tstate;
        shaders::RenderStateId rstate;
        const ShaderElement *currentShader = (shader_override) ? shader_override : elem.shader;
        int curVar = DynamicVariantsPolicy::getStates(currentShader->native(), prog, state, rstate, cstate, tstate);

        if (curVar < 0)
          continue;

        RIExRenderRecord record = RIExRenderRecord(currentShader, curVar, prog, state | disableOptimizationBit, rstate, tstate, cstate,
          pool_order, (uint16_t)elem.vstride, (uint8_t)elem.vbIdx, (uint8_t)(stage | (elem.drawOrder << RIExRenderRecord::STAGE_BITS)),
          IPoint2(ofsAndCnt.x, count), elem.si, elem.numf, elem.baseVertex, texLevel, riPool.isTree, riPool.isTessellated
#if DAGOR_DBGLEVEL > 0
          ,
          (uint8_t)max(counter, 0)
#endif
        );

        if (!is_packed_material(cstate))
          list.emplace_back(eastl::move(record));
        else
          multidrawList.emplace_back(eastl::move(record));
        if (gpu_instancing) // gpu instancing only supports one mesh per one object
          return;           // TO BE REMOVED AFTER RESOLVING ALL MESHES
      }
  }

  void addObjectsToRender(const RiGenExtraVisibility &v, dag::ConstSpan<uint16_t> riResOrder, TexStreamingContext texCtx,
    OptimizeDepthPrepass optimization_depth_prepass = OptimizeDepthPrepass::No,
    IgnoreOptimizationLimits ignore_optimization_instances_limits = IgnoreOptimizationLimits::No)

  {
    TIME_D3D_PROFILE(ri_extra_add_objects_to_render);
    for (int l = 0; l < RiExtraPool::MAX_LODS; l++)
    {
      if (!(v.riExLodNotEmpty & (1 << l)))
        continue;

      for (int k = 0, ke = riResOrder.size(); k < ke; k++)
      {
        int i = riResOrder[k] & RI_RES_ORDER_COUNT_MASK;
        IPoint2 ofsAndCnt = v.vbOffsets[l][i];
        if (ofsAndCnt.y == 0)
          continue;
        int optimizationInstances = (riResOrder[k] >> RI_RES_ORDER_COUNT_SHIFT);
        float distSq = v.minSqDistances[l][i];
        addObjectToRender(i, optimizationInstances, optimization_depth_prepass == OptimizeDepthPrepass::Yes,
          ignore_optimization_instances_limits == IgnoreOptimizationLimits::Yes, ofsAndCnt, l, k, texCtx, distSq);
      }
    }
  }
};

} // namespace rendinst

#undef USE_DEPTH_PREPASS_FOR_TREES
