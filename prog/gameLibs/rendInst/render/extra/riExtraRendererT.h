// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <rendInst/renderPass.h>

#include "riGen/riGenExtra.h"
#include "render/extra/riExRenderRecord.h"
#include "render/genRender.h"
#include "render/extra/consoleHandler.h"
#include "visibility/extraVisibility.h"

#include <memory/dag_framemem.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_decl.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <shaders/dag_shStateBlockBindless.h>
#include <render/debugMesh.h>
#include <render/pointLod/range.h>
#include <rendInst/packedMultidrawParams.hlsli>
#include <shaders/dag_shaderVarsUtils.h>
#include <math/integer/dag_IPoint3.h>


// Tools don't have depth prepass for trees.
#define USE_DEPTH_PREPASS_FOR_TREES !(_TARGET_PC_TOOLS_BUILD)

template <class>
class DynVariantsCache;

namespace rendinst::render
{

extern int ri_vertex_data_no;

static constexpr uint32_t RI_RES_ORDER_COUNT_SHIFT = 14, RI_RES_ORDER_COUNT_MASK = (1 << RI_RES_ORDER_COUNT_SHIFT) - 1;

template <typename Alloc, class DynamicVariantsPolicy>
class RiExtraRendererT : public DynamicVariantsPolicy //-V730
{
  // TODO: make different structs
  dag::Vector<RIExRenderRecord, Alloc, false> list;
  dag::Vector<RIExRenderRecord, Alloc, false> multidrawList;
  uint32_t startStage, endStage;
  LayerFlag layer;
  uint32_t instanceCountMultiply = 1;
  bool isDepthPass;
  bool optimizeDepthPass;
  bool isVoxelizationPass;
  bool isDecalPass;
  bool isTransparentPass;

  struct PackedDrawCallsRange
  {
    uint32_t start;
    uint32_t count;
  };
  dag::Vector<PackedDrawCallsRange, Alloc, false> drawcallRanges;
  ska::flat_hash_map<shaders::ConstStateIdx, uint16_t, eastl::hash<shaders::ConstStateIdx>, eastl::equal_to<shaders::ConstStateIdx>,
    Alloc>
    bindlessStatesToUpdateTexLevels;

public:
  RiExtraRendererT() = default;
  template <typename... Args>
  RiExtraRendererT(Args &&...args)
  {
    init(eastl::forward<Args>(args)...);
  }
  void init(const int size_to_reserve, LayerFlag layer_, RenderPass render_pass,
    OptimizeDepthPrepass optimization_depth_prepass = OptimizeDepthPrepass::No,
    OptimizeDepthPass optimization_depth_pass = OptimizeDepthPass::No,
    IgnoreOptimizationLimits ignore_optimization_instances_limits = IgnoreOptimizationLimits::No, uint32_t count_multiply = 1)
  {
    layer = layer_, instanceCountMultiply = eastl::max(1u, count_multiply);
    isDepthPass = render_pass == RenderPass::ToShadow || render_pass == RenderPass::Depth;
    optimizeDepthPass = isDepthPass && optimization_depth_pass == OptimizeDepthPass::Yes;
    isVoxelizationPass = (render_pass == RenderPass::VoxelizeAlbedo || render_pass == RenderPass::Voxelize3d);
    isDecalPass = layer == LayerFlag::Decals;
    isTransparentPass = layer == LayerFlag::Transparent;

    list.clear();
    list.reserve(size_to_reserve);
    multidrawList.clear();
    drawcallRanges.clear();
    bindlessStatesToUpdateTexLevels.clear();
    if (layer == LayerFlag::Transparent)
      startStage = endStage = ShaderMesh::STG_trans;
    else if (layer == LayerFlag::Decals)
      startStage = endStage = ShaderMesh::STG_decal;
    else if (layer == LayerFlag::Distortion)
      startStage = endStage = ShaderMesh::STG_distortion;
    else
    {
      startStage = ShaderMesh::STG_opaque;
      if (optimization_depth_prepass == OptimizeDepthPrepass::Yes &&
          ignore_optimization_instances_limits == IgnoreOptimizationLimits::No)
        endStage = ShaderMesh::STG_opaque;
      else
        endStage = (render_pass == RenderPass::Normal) ? ShaderMesh::STG_imm_decal : ShaderMesh::STG_atest;
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

  template <bool separate_lods>
  void coalesceDrawcalls(bool allow_reordering)
  {
    TIME_D3D_PROFILE(ri_extra_coalesce_drawcalls);
    if (!multidrawList.size())
      return;

    const auto mergeComparator = [](const RIExRenderRecord &a, const RIExRenderRecord &b) -> bool {
      bool result = a.isTree == b.isTree && a.drawOrder_stage == b.drawOrder_stage && a.vstride == b.vstride && a.vbIdx == b.vbIdx &&
                    a.rstate == b.rstate && get_material_id(a.cstate) == get_material_id(b.cstate) && a.prog == b.prog;

      if constexpr (separate_lods)
        result &= a.lod == b.lod;

      return result;
    };

    drawcallRanges.push_back(PackedDrawCallsRange{0, 1});
    bindlessStatesToUpdateTexLevels.emplace(multidrawList[0].cstate, multidrawList[0].texLevel);

    for (uint32_t i = 1, ie = multidrawList.size(); i < ie; ++i)
    {
      const auto &currentRelem = multidrawList[i];
      const bool mergeWithPrevious = mergeComparator(currentRelem, multidrawList[i - 1]);
      if (mergeWithPrevious)
      {
        drawcallRanges.back().count++;
      }
      else
        drawcallRanges.push_back(PackedDrawCallsRange{drawcallRanges.back().count + drawcallRanges.back().start, 1});
      auto iter = bindlessStatesToUpdateTexLevels.find(currentRelem.cstate);
      if (iter == bindlessStatesToUpdateTexLevels.end())
        bindlessStatesToUpdateTexLevels.emplace(currentRelem.cstate, currentRelem.texLevel);
      else
        iter->second = max(iter->second, currentRelem.texLevel);
    }

    if (allow_reordering)
      stlsort::sort(drawcallRanges.begin(), drawcallRanges.end(), [&](const auto &a, const auto &b) {
        if (multidrawList[a.start].drawOrder_stage != multidrawList[b.start].drawOrder_stage)
          return multidrawList[a.start].drawOrder_stage < multidrawList[b.start].drawOrder_stage;
        else if (multidrawList[a.start].poolOrder != multidrawList[b.start].poolOrder)
          return multidrawList[a.start].poolOrder < multidrawList[b.start].poolOrder;
        return multidrawList[a.start].elemOrder < multidrawList[b.start].elemOrder;
      });
  }

  void updateDataForPackedRender() const
  {
    if (bindlessStatesToUpdateTexLevels.empty())
      return;

    TIME_D3D_PROFILE(ri_extra_update_indirect_data);
    for (auto stateIdTexLevel : bindlessStatesToUpdateTexLevels)
      update_bindless_state(stateIdTexLevel.first, stateIdTexLevel.second);
  }

  inline void renderSortedMeshesPacked(dag::ConstSpan<uint16_t> riResOrder) const
  {
    G_UNUSED(riResOrder);
    if (multidrawList.empty())
      return;

    updateDataForPackedRender();

    const auto multiDrawRenderer = riExMultidrawContext.fillBuffers(multidrawList.size(),
      [this](uint32_t draw_index, uint32_t &index_count_per_instance, uint32_t &instance_count, uint32_t &start_index_location,
        int32_t &base_vertex_location, RiExPerInstanceParameters &per_draw_data) {
        index_count_per_instance = (uint32_t)multidrawList[draw_index].numf * 3;
        instance_count = (uint32_t)multidrawList[draw_index].ofsAndCnt.y * instanceCountMultiply;
        start_index_location = (uint32_t)multidrawList[draw_index].si;
        base_vertex_location = (int32_t)multidrawList[draw_index].bv;
        if (multidrawList[draw_index].ofsAndCnt.x % 4 != 0)
        {
          logerr("Assumption about alignment of offset in RI matrices buffer is incorrect.");
          instance_count = 0;
        }
        const uint32_t instanceOffset = (uint32_t)multidrawList[draw_index].ofsAndCnt.x >> 2;
        if (instanceOffset >= MAX_MATRIX_OFFSET)
        {
          logerr("Too big offset in instance matrix buffer %d.", instanceOffset);
          instance_count = 0;
        }
        const uint32_t materialOffset = get_material_offset(multidrawList[draw_index].cstate);
        if (materialOffset >= MAX_MATERIAL_OFFSET)
        {
          logerr("Too big material offset %d.", materialOffset);
          instance_count = 0;
        }
        per_draw_data = (instanceOffset << MATRICES_OFFSET_SHIFT) | materialOffset;
      });

    TIME_D3D_PROFILE(ri_extra_render_sorted_meshes_indirect);

    G_FAST_ASSERT(unitedvdata::riUnitedVdata.getIB()); // Can't be 0 if list isnt empty
    d3d_err(d3d::setind(unitedvdata::riUnitedVdata.getIB()));

    RiShaderConstBuffers cb;
    cb.setInstancing(0, 4, 0x5, 0);
    cb.flushPerDraw();

#if USE_DEPTH_PREPASS_FOR_TREES
    shaders::OverrideStateId previousOverrideId = shaders::overrides::get_current();
    bool currentDepthPrepass = false;
    const bool hasStencilTestStateOverride = shaders::overrides::get(previousOverrideId).bits == shaders::OverrideState::STENCIL;
    const bool validOverride = shaders::overrides::get(previousOverrideId).bits == shaders::OverrideState::CULL_NONE ||
                               hasStencilTestStateOverride || shaders::overrides::get(previousOverrideId).bits == 0;
    G_UNUSED(validOverride);
#endif

    for (const auto dcParams : drawcallRanges)
    {
      const auto &rl = multidrawList[dcParams.start];
      Vbuffer *vb = unitedvdata::riUnitedVdata.getVB(rl.vbIdx);
      d3d_err(d3d::setvsrc(0, vb, rl.vstride));

#if USE_DEPTH_PREPASS_FOR_TREES
      RiExtraPool &riPool = riExtra[riResOrder[rl.poolOrder] & RI_RES_ORDER_COUNT_MASK];
      const bool needDepthPrepass = (!isVoxelizationPass && !isDepthPass && !isDecalPass && riPool.isTree && use_ri_depth_prepass);

      if (needDepthPrepass != currentDepthPrepass)
      {
        G_ASSERTF(validOverride, "Only [disabled backface culling or stencil test] are available overrides here in colored pass");
        currentDepthPrepass = needDepthPrepass;
        shaders::overrides::reset();
        if (needDepthPrepass)
          shaders::overrides::set(hasStencilTestStateOverride ? afterDepthPrepassWithStencilTestOverride : afterDepthPrepassOverride);
        else
          shaders::overrides::set(previousOverrideId);
      }
#endif
      debug_mesh::set_debug_value(rl.lod);

      rl.curShader->setReqTexLevel(rl.texLevel);
      set_states_for_variant(rl.curShader->native(), rl.cv, rl.prog, rl.state);

      multiDrawRenderer.render(PRIM_TRILIST, dcParams.start, dcParams.count);
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


  template <bool gpu_instancing>
  void renderSortedMeshes(dag::ConstSpan<uint16_t> riResOrder, Sbuffer *indirect_buffer = nullptr) const
  {
    G_UNUSED(riResOrder);
    if (list.empty())
      return;

    TIME_D3D_PROFILE(ri_extra_render_sorted_meshes);

#if _TARGET_C1 | _TARGET_C2 // workaround: flush caches for indirect args

#endif

    G_FAST_ASSERT(unitedvdata::riUnitedVdata.getIB()); // Can't be 0 if list isnt empty
    d3d_err(d3d::setind(unitedvdata::riUnitedVdata.getIB()));

    RiShaderConstBuffers cb;
    cb.setInstancing(0, 4, 0x5, 0);
    cb.flushPerDraw();

    int cVbIdx = -1, cStride = 0;
    IPoint3 curOfsAndVertexByteStartPerDrawOffset(-1, -1, -1);
#if USE_DEPTH_PREPASS_FOR_TREES
    shaders::OverrideStateId previousOverrideId = shaders::overrides::get_current();
    bool currentDepthPrepass = false;
    const bool hasStencilTestStateOverride = shaders::overrides::get(previousOverrideId).bits == shaders::OverrideState::STENCIL;

    const bool validOverride = shaders::overrides::get(previousOverrideId).bits == shaders::OverrideState::CULL_NONE ||
                               hasStencilTestStateOverride || shaders::overrides::get(previousOverrideId).bits == 0;
    G_UNUSED(validOverride);
#endif

    shaders::RenderStateId curRstate = shaders::RenderStateId();
    ShaderStateBlockId curState = ShaderStateBlockId::Invalid;
    bool curDisableOptimization = true;
    bool currentTessellationState = !list[0].isTessellated;

    for (auto &rl : list)
    {
      if (cVbIdx != rl.vbIdx || cStride != rl.vstride)
      {
        if (rl.vbIdx == unitedvdata::BufPool::IDX_IB)
        {
          logerr("Invalid united VB index in ri='%s', shader='%s', numf=%d",
            riExtraMap.getName(riResOrder[rl.poolOrder] & RI_RES_ORDER_COUNT_MASK), rl.curShader->getShaderClassName(), rl.numf);
          continue;
        }
        Vbuffer *vb = unitedvdata::riUnitedVdata.getVB(rl.vbIdx);
        if (rl.isSWVertexFetch && cVbIdx != rl.vbIdx)
        {
          G_ASSERT(ri_vertex_data_no != -1);
          d3d::set_buffer(STAGE_VS, ri_vertex_data_no, vb);
          d3d::set_buffer(STAGE_PS, ri_vertex_data_no, vb);
        }
        cVbIdx = rl.vbIdx;
        cStride = rl.vstride;
        d3d_err(d3d::setvsrc(0, vb, cStride));
      }

      bool skipApply = false;
      if (optimizeDepthPass && rl.drawOrder_stage->stage == ShaderMesh::STG_opaque && curRstate == rl.rstate &&
          !rl.disableOptimization && rl.isTessellated == currentTessellationState &&
          (!rl.isTessellated || (curState == rl.state && curDisableOptimization == rl.disableOptimization)))
      {
        skipApply = true; // we only switch renderstate - zbias,culling, etc
      }
      curRstate = rl.rstate;
      curState = rl.state;
      curDisableOptimization = rl.disableOptimization;
      currentTessellationState = rl.isTessellated;

      const uint32_t poolId = riResOrder[rl.poolOrder] & RI_RES_ORDER_COUNT_MASK;
      const RiExtraPool &riPool = riExtra[poolId];
#if USE_DEPTH_PREPASS_FOR_TREES

      const bool needDepthPrepass =
        (!isVoxelizationPass && !isDepthPass && !isDecalPass && riPool.isTree && use_ri_depth_prepass && !isTransparentPass);
      if (needDepthPrepass != currentDepthPrepass)
      {
        G_ASSERTF(validOverride, "Only [disabled backface culling or stencil test] are available overrides here in colored pass");
        currentDepthPrepass = needDepthPrepass;
        shaders::overrides::reset();
        if (needDepthPrepass)
          shaders::overrides::set(hasStencilTestStateOverride ? afterDepthPrepassWithStencilTestOverride : afterDepthPrepassOverride);
        else
          shaders::overrides::set(previousOverrideId);
      }
#endif

      if (debug_mesh::set_debug_value(rl.lod))
        skipApply = false; // stencil must be applied for lod coloring, no matter what

      rl.curShader->setReqTexLevel(rl.texLevel);

      if (!skipApply)
        set_states_for_variant(rl.curShader->native(), rl.cv, rl.prog, rl.state);

      const int perDataBufferOffset = get_per_draw_offset(poolId);
      const IPoint3 ofsAndVertexByteStartPerDrawOffset = {rl.ofsAndCnt.x, rl.bv * rl.vstride, perDataBufferOffset};
      const bool setInstancing = curOfsAndVertexByteStartPerDrawOffset != ofsAndVertexByteStartPerDrawOffset;
      if (setInstancing || gpu_instancing)
      {
        d3d::set_immediate_const(STAGE_VS, (uint32_t *)&ofsAndVertexByteStartPerDrawOffset.x, 3);
        curOfsAndVertexByteStartPerDrawOffset = ofsAndVertexByteStartPerDrawOffset;
      }
      if (gpu_instancing)
        d3d::draw_indexed_indirect(PRIM_TRILIST, indirect_buffer, sizeof(uint32_t) * DRAW_INDEXED_INDIRECT_NUM_ARGS * rl.ofsAndCnt.x);
      else
      {
        bool isImpostor = rl.lod == RiExtraPool::MAX_LODS - 1;
        if (riPool.isTree && !isImpostor)
        {
          const auto tcConsts = getCommonImmediateConstants();
          uint32_t immediateConsts[] = {0u, tcConsts[0], tcConsts[1]};
          d3d::set_immediate_const(STAGE_PS, immediateConsts, sizeof(immediateConsts) / sizeof(immediateConsts[0]));
        }
        if (rl.si != RELEM_NO_INDEX_BUFFER)
          d3d::drawind_instanced(rl.primitive, rl.si, rl.numf, rl.bv, rl.ofsAndCnt.y * instanceCountMultiply,
            // use base instance value if shaders have interval for that
            globalRendinstRenderTypeVarId >= 0 ? rl.ofsAndCnt.x / RIEXTRA_VECS_COUNT : 0);
        else
          d3d::draw_instanced(rl.primitive, rl.sv, rl.numv, rl.ofsAndCnt.y * instanceCountMultiply,
            globalRendinstRenderTypeVarId >= 0 ? rl.ofsAndCnt.x / RIEXTRA_VECS_COUNT : 0);
        if (riPool.isTree && !isImpostor)
        {
          d3d::set_immediate_const(STAGE_PS, nullptr, 0);
        }
      }

      if (rl.disableOptimization)
      {
        curRstate = shaders::RenderStateId();
        curState = ShaderStateBlockId::Invalid;
      }
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

      if (!isDepthPass || a.isTessellated || a.drawOrder_stage->stage != ShaderMesh::STG_opaque) // opaque stage
                                                                                                 // is all of one
                                                                                                 // shader anyway
      {
        // maybe split state into sampler state (heavy) and const buffer (cheap)?
        if (a.state != b.state || a.disableOptimization != b.disableOptimization)
        {
          if (a.tstate != b.tstate) // maybe split state into sampler state (heavy) and const buffer (cheap)?
            return a.tstate < b.tstate;
          if (a.rstate != b.rstate)
            return a.rstate < b.rstate;
          if (a.disableOptimization != b.disableOptimization)
            return a.disableOptimization < b.disableOptimization;
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
      if (a.ofsAndCnt.x != b.ofsAndCnt.x)
        return a.ofsAndCnt.x < b.ofsAndCnt.x;
      return a.elemOrder < b.elemOrder;
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
      if (a.isTessellated != b.isTessellated)
        return a.isTessellated < b.isTessellated;

      if (!isDepthPass || a.isTessellated || a.drawOrder_stage->stage != ShaderMesh::STG_opaque) // opaque stage
                                                                                                 // is all of one
                                                                                                 // shader anyway
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
      if (a.ofsAndCnt.x != b.ofsAndCnt.x)
        return a.ofsAndCnt.x < b.ofsAndCnt.x;
      return a.elemOrder < b.elemOrder;
    });

    const bool allowReordering = !isDecalPass && !isTransparentPass;
    if (debug_mesh::is_enabled())
      coalesceDrawcalls<true>(allowReordering);
    else
      coalesceDrawcalls<false>(allowReordering);
  }

  struct RiExtraElementsToHide
  {
    RiExtraElementsToHide() = default;
    RiExtraElementsToHide(const SimpleString &materialName, uint16_t poolId,
      const SmallTab<RiGenExtraVisibility::HideMarkedMaterialForInstance> &elementsToHide)
    {
      if (materialName.empty() || elementsToHide.size() == 0)
      {
        shaderName = nullptr;
        return;
      }

      // This returns the pointer stored in the shaderClass allowing pointer equality checks in the hot path
      shaderName = get_shader_class_name_by_material_name(materialName.c_str());
      if (!shaderName)
        return;

      // elementsToHide is already sorted by poolId, store the range of hidden roots for this pool
      int beginIdx = -1;
      for (int i = 0; i < elementsToHide.size(); ++i)
      {
        if (beginIdx < 0 && elementsToHide[i].poolId == poolId)
          beginIdx = i;
        else if (beginIdx >= 0 && elementsToHide[i].poolId != poolId)
        {
          instances =
            dag::ConstSpan<RiGenExtraVisibility::HideMarkedMaterialForInstance>(elementsToHide.data() + beginIdx, i - beginIdx);
          return;
        }
      }
      if (beginIdx >= 0)
        instances = dag::ConstSpan<RiGenExtraVisibility::HideMarkedMaterialForInstance>(elementsToHide.data() + beginIdx,
          elementsToHide.size() - beginIdx);
    }

    const char *shaderName = nullptr;
    dag::ConstSpan<RiGenExtraVisibility::HideMarkedMaterialForInstance> instances;
  };

  inline void addObjectToRender(uint16_t ri_idx, int optimizationInstances, bool optimization_depth_prepass,
    bool ignore_optimization_instances_limits, IPoint2 ofsAndCnt, int lod, uint16_t pool_order, const TexStreamingContext &texCtx,
    float dist2, float minDist2, const RiExtraElementsToHide &elementsToHide, const ShaderElement *shader_override = nullptr,
    bool gpu_instancing = false)
  {
    if (ri_idx >= riExtra.size())
    {
      logerr("Attempted to add riex with pool index '%d' to RiExtraRendererT, total pool amount was '%d'.", ri_idx, riExtra.size());
      return;
    }

    if (should_hide_ri_extra_object_with_id(ri_idx))
      return;

    const RiExtraPool &riPool = riExtra[ri_idx];
    if (layer == LayerFlag::RendinstClipmapBlend && !riPool.usingClipmap) // to be removed from here!
      return;
    // rendinsts patching heightmap are rendered only with LAYER_RENDINST_HEIGHTMAP_PATCH layer
    if (riPool.patchesHeightmap ^ (layer == LayerFlag::RendinstHeightmapPatch))
      return;
    const bool renderBrokenTreesToDepth = isDepthPass && riPool.isTree && use_ri_depth_prepass;
    if (optimization_depth_prepass && !ignore_optimization_instances_limits && !optimizationInstances && !renderBrokenTreesToDepth)
      return;
    int count = ofsAndCnt.y;
    if (optimization_depth_prepass && !ignore_optimization_instances_limits && !renderBrokenTreesToDepth)
      count = min(count, optimizationInstances);
    if (DAGOR_LIKELY(riPool.res))
    {
      if (count > 0)
        riPool.res->updateReqLod(min<int>(lod, riPool.res->lods.size() - 1));

      if (lod < riPool.res->getQlBestLod())
      {
        lod = riPool.res->getQlBestLod();
        if (riPool.isPosInst() && (lod == riPool.res->lods.size() - 1)) // No imposters allowed here
          return;
        if (lod >= RiExtraPool::MAX_LODS) // -V::547 with 8 this is always false because lod is stored in 3 bits in the resource
          return;
      }
    }
    else
    {
      logerr("Empty resource for pool %d. %d pools in total", ri_idx, riExtra.size());
      return;
    }
    uint32_t startEIOfs = (lod * riExtra.size() + ri_idx) * ShaderMesh::STG_COUNT + startStage;

    // Flag shaders and
    // Tree shaders are generally incompatible by VS with other RI shaders and with each other. // TODO: still true?
    const bool disableOptimization = riPool.isTree;

    int texLevel = texCtx.getTexLevel(riPool.res->getTexScale(lod), dist2);

#if USE_DEPTH_PREPASS_FOR_TREES
    uint32_t correctedEndStage = renderBrokenTreesToDepth ? ShaderMesh::STG_atest : endStage;
#else
    uint32_t correctedEndStage = endStage;
#endif
    int counter = lod;
#if DAGOR_DBGLEVEL > 0
    if (debug_mesh::is_enabled(debug_mesh::Type::drawElements))
    {
      counter = 0;
      uint32_t startEIOfsTmp = startEIOfs;
      for (unsigned int stage = startStage; stage <= correctedEndStage; stage++, startEIOfsTmp++)
        for (uint32_t EI = allElemsIndex[startEIOfsTmp], endEI = allElemsIndex[startEIOfsTmp + 1]; EI < endEI; ++EI)
        {
          auto &elem = allElems[EI];
          if (!elem.shader)
            continue;
          counter++;
        }
    }
#endif

    for (unsigned int stage = startStage; stage <= correctedEndStage; stage++, startEIOfs++)
      for (uint32_t EI = allElemsIndex[startEIOfs], endEI = allElemsIndex[startEIOfs + 1], startEI = EI; EI < endEI; ++EI)
      {
        const auto &elem = allElems[EI];
        if (!elem.shader)
          continue;
        uint32_t prog;
        ShaderStateBlockId state;
        shaders::ConstStateIdx cstate;
        shaders::TexStateIdx tstate;
        shaders::RenderStateId rstate;
        const ShaderElement *currentShader = (shader_override) ? shader_override : elem.shader;
        int curVar = DynamicVariantsPolicy::getStates(currentShader->native(), prog, state, rstate, cstate, tstate);

        if (curVar < 0)
          continue;

        if (elem.vbIdx == unitedvdata::BufPool::IDX_IB)
        {
          logwarn("Invalid united VB index in ri='%s', shader='%s', numf=%d", riExtraMap.getName(ri_idx),
            elem.shader->getShaderClassName(), elem.numf);
          continue;
        }

        const bool isTessellated = riPool.elemMask[lod].tessellation & (1 << (EI - startEI));
        RIExRenderRecord record = RIExRenderRecord(currentShader, curVar, prog, state, rstate, tstate, cstate, pool_order,
          (uint16_t)elem.vstride, (uint8_t)elem.vbIdx, elem.drawOrder, EI - startEI, elem.primitive, IPoint2(ofsAndCnt.x, count),
          elem.si, elem.sv, elem.numv, elem.numf, elem.baseVertex, texLevel, riPool.isTree, isTessellated, disableOptimization,
          (uint8_t)max(counter, 0));

        const auto isPacked = is_packed_material(cstate);
        G_ASSERT(!isPacked || elem.si != RELEM_NO_INDEX_BUFFER);

        const bool isPlod = riPool.elemMask[lod].plod & (1 << (EI - startEI));
        if (isPlod)
        {
          record.isSWVertexFetch = true;
          if (dist2 > minDist2)
            record.numv >>= plod::get_density_power2_scale(dist2, minDist2);
        }

        // Checking pointer equality, this is valid because shaderName is pointing to the same memory as shaderClassName
        if (elem.shader->getShaderClassName() == elementsToHide.shaderName)
        {
          // Split the current record in two for each hidden element
          uint32_t processedCounter = 0;
          for (uint32_t i = 0; i < elementsToHide.instances.size(); i++)
          {
            RIExRenderRecord record2 = record;
            uint32_t instance = elementsToHide.instances[i].instanceIdx;
            record2.ofsAndCnt.y = instance - processedCounter;
            record.ofsAndCnt.x += (record2.ofsAndCnt.y + 1) * rendinst::RIEXTRA_VECS_COUNT;
            record.ofsAndCnt.y -= record2.ofsAndCnt.y + 1;
            processedCounter = instance + 1;
            if (record2.ofsAndCnt.y > 0)
            {
              if (!isPacked)
                list.emplace_back(eastl::move(record2));
              else
                multidrawList.emplace_back(eastl::move(record2));
            }
          }
          if (record.ofsAndCnt.y <= 0)
            continue;
        }

        if (!isPacked)
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
        float minDistSq = v.minAllowedSqDistances[l][i];
        RiExtraElementsToHide elementsToHide(riExtra[i].materialMarkedForHiding, i, v.hideMarkedMaterialsForInstances);
        addObjectToRender(i, optimizationInstances, optimization_depth_prepass == OptimizeDepthPrepass::Yes,
          ignore_optimization_instances_limits == IgnoreOptimizationLimits::Yes, ofsAndCnt, l, k, texCtx, distSq, minDistSq,
          elementsToHide);
      }
    }
  }
};

} // namespace rendinst::render

#undef USE_DEPTH_PREPASS_FOR_TREES
