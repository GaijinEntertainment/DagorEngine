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
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_decl.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <shaders/dag_shStateBlockBindless.h>
#include <render/debugMesh.h>
#include <render/globTMVars.h>
#include <render/pointLod/range.h>
#include <rendInst/packedMultidrawParams.hlsli>
#include <shaders/dag_shaderVarsUtils.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_bits.h>
#include <util/dag_parallelForInline.h>
#include <util/dag_convar.h>
#include <osApiWrappers/dag_miscApi.h>

extern ConVarB parallel_lod_add_enabled;
extern ConVarB parallel_lod_add_main_thread_only;
extern ConVarI parallel_lod_add_min_pool_count;
extern ConVarI parallel_lod_add_chunks_per_worker;
extern ConVarI parallel_lod_add_min_chunk_pools;

template <class>
class DynVariantsCache;

namespace rendinst::render
{

extern int ri_vertex_data_no, ri_voxel_data_offset_varid, ri_voxel_depth_projection_varid;
extern bool is_tool_shaders;

extern void update_voxel_baker_textures();

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
  bool isNormalPass;
  bool isDecalPass;
  bool isVoxelBakerPass;
  bool isTransparentPass;
  bool allowTreeDepthPrepass;
  bool useExternalPerDrawCb;

  struct PackedDrawCallsRange
  {
    uint32_t start;
    uint32_t count;
  };
  dag::Vector<PackedDrawCallsRange, Alloc, false> drawcallRanges;
  ska::flat_hash_map<shaders::ConstStateIdx, uint16_t, eastl::hash<shaders::ConstStateIdx>, eastl::equal_to<shaders::ConstStateIdx>,
    Alloc>
    bindlessStatesToUpdateTexLevels;

  struct ChunkBucket
  {
    // NOT framemem, as it is processed in worker threads, then retrieved in the main thread
    dag::Vector<RIExRenderRecord> list;
    dag::Vector<RIExRenderRecord> multidraw;
    int acceptedCount = 0;
    void reset()
    {
      list.clear();
      multidraw.clear();
      acceptedCount = 0;
    }
  };
  // NOT framemem, as the buckets own heap vectors
  dag::Vector<ChunkBucket> buckets;

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
    IgnoreOptimizationLimits ignore_optimization_instances_limits = IgnoreOptimizationLimits::No, uint32_t count_multiply = 1,
    bool use_external_per_draw_cb = false)
  {
    layer = layer_, instanceCountMultiply = eastl::max(1u, count_multiply);
    isDepthPass = render_pass == RenderPass::ToShadow || render_pass == RenderPass::Depth;
    optimizeDepthPass = isDepthPass && optimization_depth_pass == OptimizeDepthPass::Yes;
    isVoxelizationPass = render_pass == RenderPass::VoxelizeAlbedo;
    isNormalPass = render_pass == RenderPass::Normal;
    isDecalPass = layer == LayerFlag::Decals;
    isTransparentPass = layer == LayerFlag::Transparent;
    isVoxelBakerPass = render_pass == RenderPass::ImpostorVoxel;

    // Tool shaders don't have depth prepass for trees.
    allowTreeDepthPrepass = !rendinst::render::is_tool_shaders;
    useExternalPerDrawCb = use_external_per_draw_cb;
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
      if (render_pass == RenderPass::ImpostorVoxel)
        endStage = ShaderMesh::STG_decal;
      else if (optimization_depth_prepass == OptimizeDepthPrepass::Yes &&
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
                    a.dvState.render_state == b.dvState.render_state &&
                    get_material_id(a.dvState.const_state) == get_material_id(b.dvState.const_state) &&
                    a.dvState.program == b.dvState.program && a.voxelDataOffset == b.voxelDataOffset &&
                    a.voxelSurfaceId == b.voxelSurfaceId;

      if constexpr (separate_lods)
        result &= a.lod == b.lod;

      return result;
    };

    drawcallRanges.push_back(PackedDrawCallsRange{0, 1});
    bindlessStatesToUpdateTexLevels.emplace(multidrawList[0].dvState.const_state, multidrawList[0].texLevel);

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
      auto iter = bindlessStatesToUpdateTexLevels.find(currentRelem.dvState.const_state);
      if (iter == bindlessStatesToUpdateTexLevels.end())
        bindlessStatesToUpdateTexLevels.emplace(currentRelem.dvState.const_state, currentRelem.texLevel);
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

  inline void updateVoxelBakerTextures(const RIExRenderRecord &rl) const
  {
    if (!isVoxelBakerPass)
      return;
    if (rl.drawOrder_stage->stage != ShaderMesh::STG_imm_decal and rl.drawOrder_stage->stage != ShaderMesh::STG_decal)
      return;
    update_voxel_baker_textures();
  }

  inline static void setVoxelDepthProj()
  {
    if (ri_voxel_depth_projection_varid == -1)
      return;
    TMatrix4 gtm;
    d3d::getglobtm(gtm);
    ::set_globtm_to_shader(gtm);

    const Point3 a0(gtm.m[0][0], gtm.m[1][0], gtm.m[2][0]);
    const Point3 a1(gtm.m[0][1], gtm.m[1][1], gtm.m[2][1]);
    const Point3 a3(gtm.m[0][3], gtm.m[1][3], gtm.m[2][3]);
    const Point3 u = a1 % a3, v = a3 % a0, w = a0 % a1;

    float k = a0 * u;
    Point3 c = gtm.m[3][0] * u + gtm.m[3][1] * v + gtm.m[3][3] * w;

    // scale is free: renormalize so fp32 stays in range, and orient away from the camera
    float s = 1.0f / max(fabsf(k), max(fabsf(c.x), max(fabsf(c.y), fabsf(c.z))));
    if (k < 0.0f || (k < 1e-8f && a3 * c < 0.0f))
      s = -s;
    k *= s;
    c *= s;
    ShaderGlobal::set_float4(ri_voxel_depth_projection_varid, c, k);
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
          LOGERR_ONCE("Assumption about alignment of offset in RI matrices buffer is incorrect.");
          instance_count = 0;
        }
        const uint32_t instanceOffset = (uint32_t)multidrawList[draw_index].ofsAndCnt.x >> 2;
        if (instanceOffset >= MAX_MATRIX_OFFSET)
        {
          LOGERR_ONCE("Too big offset in instance matrix buffer %d.", instanceOffset);
          instance_count = 0;
        }
        const uint32_t materialOffset = get_material_offset(multidrawList[draw_index].dvState.const_state);
        if (materialOffset >= MAX_MATERIAL_OFFSET)
        {
          LOGERR_ONCE("Too big material offset %d.", materialOffset);
          instance_count = 0;
        }
        per_draw_data = (instanceOffset << MATRICES_OFFSET_SHIFT) | materialOffset;
      });

    TIME_D3D_PROFILE(ri_extra_render_sorted_meshes_indirect);

    G_FAST_ASSERT(unitedvdata::riUnitedVdata.getIB()); // Can't be 0 if list isnt empty
    d3d_err(d3d::setind(unitedvdata::riUnitedVdata.getIB()));

    if (!useExternalPerDrawCb)
    {
      RiShaderConstBuffers cb;
      cb.setInstancing(0, 4, RI_CBUFFER_FLAGS__PER_DRAW_DATA_FROM_GLOBAL_BUFFER, 0);
      cb.flushPerDraw();
    }

    shaders::OverrideStateId previousOverrideId = shaders::overrides::get_current();
    bool currentDepthPrepass = false;
    const bool hasStencilTestStateOverride = shaders::overrides::get(previousOverrideId).bits == shaders::OverrideState::STENCIL;
    const bool validOverride = shaders::overrides::get(previousOverrideId).bits == shaders::OverrideState::CULL_NONE ||
                               hasStencilTestStateOverride || shaders::overrides::get(previousOverrideId).bits == 0;
    G_UNUSED(validOverride);

    // set lazily at the first voxel-LOD draw: the vars only feed the voxel
    // raymarch shader and cost a globtm fetch + inverse when no voxel mesh
    // is in the batch
    bool voxelDepthProjectionSet = false;

    int cVoxOfs = 0;
    uint32_t cVoxSurf = VoxelSurfaceData::INVALID_ID;

    for (const auto dcParams : drawcallRanges)
    {
      const auto &rl = multidrawList[dcParams.start];
      Vbuffer *vb = unitedvdata::riUnitedVdata.getVB(rl.vbIdx);
      d3d_err(d3d::setvsrc(0, vb, rl.vstride));

      if (rl.isSWVertexFetch)
      {
        G_ASSERT(ri_vertex_data_no != -1);
        d3d::set_buffer(STAGE_VS, ri_vertex_data_no, vb);
        d3d::set_buffer(STAGE_PS, ri_vertex_data_no, vb);
      }

      RiExtraPool &riPool = riExtra[riResOrder[rl.poolOrder] & RI_RES_ORDER_COUNT_MASK];
      const bool needDepthPrepass =
        (allowTreeDepthPrepass && !isVoxelizationPass && !isDepthPass && !isDecalPass && riPool.isTree && use_ri_depth_prepass);

      if (eastl::exchange(currentDepthPrepass, needDepthPrepass) != currentDepthPrepass)
      {
        G_ASSERTF(validOverride, "Only [disabled backface culling or stencil test] are available overrides here in colored pass");
        shaders::overrides::reset();
        if (needDepthPrepass)
          shaders::overrides::set(hasStencilTestStateOverride ? afterDepthPrepassWithStencilTestOverride : afterDepthPrepassOverride);
        else
          shaders::overrides::set(previousOverrideId);
      }

      debug_mesh::set_debug_value(rl.lod);

      if (cVoxOfs != rl.voxelDataOffset)
      {
        if (rl.voxelDataOffset != 0)
        {
          if (!voxelDepthProjectionSet)
          {
            voxelDepthProjectionSet = true;
            setVoxelDepthProj();
          }
          G_ASSERT(ri_voxel_data_offset_varid != -1);
          ShaderGlobal::set_int(ri_voxel_data_offset_varid, rl.voxelDataOffset);
        }
        cVoxOfs = rl.voxelDataOffset;
      }

      if (cVoxSurf != rl.voxelSurfaceId)
      {
        cVoxSurf = rl.voxelSurfaceId;
        VoxelSurfaceData::setToShader(cVoxSurf);
      }

      updateVoxelBakerTextures(rl);

      rl.curShader->setReqTexLevel(rl.texLevel);
      set_states_for_variant(rl.curShader->native(), rl.dvState);

      multiDrawRenderer.render(PRIM_TRILIST, dcParams.start, dcParams.count);
    }
    if (currentDepthPrepass)
    {
      shaders::overrides::reset();
      shaders::overrides::set(previousOverrideId);
    }

    debug_mesh::reset_debug_value();
  }


  template <bool gpu_instancing>
  void renderSortedMeshes(dag::ConstSpan<uint16_t> riResOrder, Sbuffer *indirect_buffer = nullptr) const
  {
    G_UNUSED(riResOrder);
    if (list.empty())
      return;

    TIME_D3D_PROFILE(ri_extra_render_sorted_meshes);
    G_FAST_ASSERT(unitedvdata::riUnitedVdata.getIB()); // Can't be 0 if list isnt empty
    d3d_err(d3d::setind(unitedvdata::riUnitedVdata.getIB()));

    if (!useExternalPerDrawCb)
    {
      RiShaderConstBuffers cb;
      cb.setInstancing(0, 4, RI_CBUFFER_FLAGS__PER_DRAW_DATA_FROM_GLOBAL_BUFFER, 0);
      cb.flushPerDraw();
    }

    // set lazily at the first voxel-LOD draw: the vars only feed the voxel
    // raymarch shader and cost a globtm fetch + inverse when no voxel mesh
    // is in the batch
    bool voxelDepthProjectionSet = false;

    int cVbIdx = -1, cStride = 0, cVoxOfs = 0;
    uint32_t cVoxSurf = VoxelSurfaceData::INVALID_ID;
    IPoint3 curOfsAndVertexByteStartPerDrawOffset(-1, -1, -1);

    shaders::OverrideStateId previousOverrideId = shaders::overrides::get_current();
    bool currentDepthPrepass = false;
    const bool hasStencilTestStateOverride = shaders::overrides::get(previousOverrideId).bits == shaders::OverrideState::STENCIL;

    const bool validOverride = shaders::overrides::get(previousOverrideId).bits == shaders::OverrideState::CULL_NONE ||
                               hasStencilTestStateOverride || shaders::overrides::get(previousOverrideId).bits == 0;
    G_UNUSED(validOverride);

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
        if (rl.isSWVertexFetch)
        {
          G_ASSERT(ri_vertex_data_no != -1);
          d3d::set_buffer(STAGE_VS, ri_vertex_data_no, vb);
          d3d::set_buffer(STAGE_PS, ri_vertex_data_no, vb);
        }
        cVbIdx = rl.vbIdx;
        cStride = rl.vstride;
        d3d_err(d3d::setvsrc(0, vb, cStride));
      }

      if (cVoxOfs != rl.voxelDataOffset)
      {
        if (rl.voxelDataOffset != 0)
        {
          if (!voxelDepthProjectionSet)
          {
            voxelDepthProjectionSet = true;
            setVoxelDepthProj();
          }
          G_ASSERT(ri_voxel_data_offset_varid != -1);
          ShaderGlobal::set_int(ri_voxel_data_offset_varid, rl.voxelDataOffset);
        }
        cVoxOfs = rl.voxelDataOffset;
      }

      if (cVoxSurf != rl.voxelSurfaceId)
      {
        cVoxSurf = rl.voxelSurfaceId;
        VoxelSurfaceData::setToShader(cVoxSurf);
      }

      bool skipApply = false;
      if (optimizeDepthPass && rl.drawOrder_stage->stage == ShaderMesh::STG_opaque && curRstate == rl.dvState.render_state &&
          !rl.disableOptimization && rl.isTessellated == currentTessellationState &&
          (!rl.isTessellated || (curState == rl.dvState.state_index && curDisableOptimization == rl.disableOptimization)))
      {
        skipApply = true; // we only switch renderstate - zbias,culling, etc
      }
      curRstate = rl.dvState.render_state;
      curState = rl.dvState.state_index;
      curDisableOptimization = rl.disableOptimization;
      currentTessellationState = rl.isTessellated;

      const uint32_t poolId = riResOrder[rl.poolOrder] & RI_RES_ORDER_COUNT_MASK;
      const RiExtraPool &riPool = riExtra[poolId];
      const bool needDepthPrepass = (allowTreeDepthPrepass && !isVoxelizationPass && !isDepthPass && !isDecalPass && riPool.isTree &&
                                     use_ri_depth_prepass && !isTransparentPass);
      if (eastl::exchange(currentDepthPrepass, needDepthPrepass) != currentDepthPrepass)
      {
        G_ASSERTF(validOverride, "Only [disabled backface culling or stencil test] are available overrides here in colored pass");
        shaders::overrides::reset();
        if (needDepthPrepass)
          shaders::overrides::set(hasStencilTestStateOverride ? afterDepthPrepassWithStencilTestOverride : afterDepthPrepassOverride);
        else
          shaders::overrides::set(previousOverrideId);
      }

      if (debug_mesh::set_debug_value(rl.lod))
        skipApply = false; // stencil must be applied for lod coloring, no matter what

      updateVoxelBakerTextures(rl);

      rl.curShader->setReqTexLevel(rl.texLevel);

      if (!skipApply)
        set_states_for_variant(rl.curShader->native(), rl.dvState);

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
    if (currentDepthPrepass)
    {
      shaders::overrides::reset();
      shaders::overrides::set(previousOverrideId);
    }
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
      if (a.voxelSurfaceId != b.voxelSurfaceId)
        return a.voxelSurfaceId < b.voxelSurfaceId;
      if (a.voxelDataOffset != b.voxelDataOffset)
        return a.voxelDataOffset < b.voxelDataOffset;

      if (!isDepthPass || a.isTessellated || a.drawOrder_stage->stage != ShaderMesh::STG_opaque) // opaque stage
                                                                                                 // is all of one
                                                                                                 // shader anyway
      {
        // maybe split state into sampler state (heavy) and const buffer (cheap)?
        if (a.dvState.state_index != b.dvState.state_index || a.disableOptimization != b.disableOptimization)
        {
          if (a.dvState.tex_state != b.dvState.tex_state) // maybe split state into sampler state (heavy) and const buffer (cheap)?
            return a.dvState.tex_state < b.dvState.tex_state;
          if (a.dvState.render_state != b.dvState.render_state)
            return a.dvState.render_state < b.dvState.render_state;
          if (a.disableOptimization != b.disableOptimization)
            return a.disableOptimization < b.disableOptimization;
          return a.dvState.state_index < b.dvState.state_index;
        }
        if (a.dvState.program != b.dvState.program)
          return a.dvState.program < b.dvState.program;
      }
      else
      {
        if (a.dvState.render_state != b.dvState.render_state)
          return a.dvState.render_state < b.dvState.render_state;
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
      if (get_material_id(a.dvState.const_state) != get_material_id(b.dvState.const_state))
        return get_material_id(a.dvState.const_state) > get_material_id(b.dvState.const_state);
      // const RIExRenderRecord & a = list[ai], &b = list[bi];
      if (a.drawOrder_stage != b.drawOrder_stage)
        return a.drawOrder_stage < b.drawOrder_stage;
      if (a.vstride != b.vstride)
        return a.vstride < b.vstride;
      if (a.vbIdx != b.vbIdx)
        return a.vbIdx < b.vbIdx;
      if (a.isTessellated != b.isTessellated)
        return a.isTessellated < b.isTessellated;
      if (a.voxelSurfaceId != b.voxelSurfaceId)
        return a.voxelSurfaceId < b.voxelSurfaceId;
      if (a.voxelDataOffset != b.voxelDataOffset)
        return a.voxelDataOffset < b.voxelDataOffset;

      if (!isDepthPass || a.isTessellated || a.drawOrder_stage->stage != ShaderMesh::STG_opaque) // opaque stage
                                                                                                 // is all of one
                                                                                                 // shader anyway
      {
        if (a.dvState.render_state != b.dvState.render_state)
          return a.dvState.render_state < b.dvState.render_state;
        if (a.dvState.program != b.dvState.program)
          return a.dvState.program < b.dvState.program;
        if (a.isTree != b.isTree)
          return a.isTree < b.isTree;
      }
      else
      {
        if (a.dvState.render_state != b.dvState.render_state)
          return a.dvState.render_state < b.dvState.render_state;
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
    RiExtraElementsToHide(uint16_t poolId, const SmallTab<RiGenExtraVisibility::HideMarkedMaterialForInstance> &elementsToHide)
    {
      shaderName = nullptr;
      if (!riExtra.isValid(poolId))
        return;

      const SimpleString &materialName = riExtra[poolId].materialMarkedForHiding;
      if (materialName.empty() || elementsToHide.size() == 0)
        return;

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
    const DynamicVariantsPolicy &sharedPolicy = static_cast<const DynamicVariantsPolicy &>(*this);
    addObjectToRenderImpl(list, multidrawList, sharedPolicy, ri_idx, optimizationInstances, optimization_depth_prepass,
      ignore_optimization_instances_limits, ofsAndCnt, lod, pool_order, texCtx, dist2, minDist2, elementsToHide, shader_override,
      gpu_instancing);
  }

  void initSortedTransparent(int pools_count)
  {
    init(pools_count, LayerFlag::Transparent, RenderPass::Normal, OptimizeDepthPrepass::No, OptimizeDepthPass::No,
      IgnoreOptimizationLimits::Yes, 1);
  }

  // Keeps the list order: it is the blend order, so no sort by material like the opaque path.
  // Returns the number of runs (adjacent elems with equal pool and lod).
  int addSortedTransparentElems(const RiGenExtraVisibility &v, int begin, int end, const TexStreamingContext &texCtx)
  {
    const auto &elems = v.sortedTransparentElems;
    int runs = 0;
    for (int i = begin; i < end; i++)
    {
      const RiGenExtraVisibility::PerInstanceElem &elem = elems[i];
      int count = 1;
      while (i + 1 < end && elems[i + 1].poolId == elem.poolId && elems[i + 1].lod == elem.lod)
      {
        i++;
        count++;
      }
      const float dist2 = bitwise_cast<float>(elem.partitionFlagDist2 & (uint32_t(-1) >> 1));
      addObjectToRender(elem.poolId, 0, false, true, IPoint2(elem.vbOffset, count), elem.lod, elem.poolOrder, texCtx, dist2, 0.0f, {});
      runs++;
    }
    if (debug_mesh::is_enabled())
      coalesceDrawcalls<true>(false);
    else
      coalesceDrawcalls<false>(false);
    return runs;
  }

#if DAGOR_DBGLEVEL > 0
  // A mismatch means an interval the riex shaders select on differs between the snapshot node and
  // the submit: a node in between left it changed, or the submitting node binds it itself.
  void checkVariantsAgainstCurrentGlobals(const char *where) const
  {
    dag::Vector<const RIExRenderRecord *, framemem_allocator> records;
    records.reserve(list.size() + multidrawList.size());
    for (const RIExRenderRecord &r : list)
      records.push_back(&r);
    for (const RIExRenderRecord &r : multidrawList)
      records.push_back(&r);
    stlsort::sort(records.begin(), records.end(), [](const auto *a, const auto *b) { return a->curShader < b->curShader; });
    const ShaderElement *checked = nullptr;
    for (const RIExRenderRecord *r : records)
    {
      if (r->curShader == checked)
        continue;
      checked = r->curShader;
      const shaders::CombinedDynVariantState now = get_dynamic_variant_states(r->curShader->native());
      if (now.program != r->dvState.program || now.state_index != r->dvState.state_index)
        LOGERR_ONCE("%s: prebuilt variant of '%s' differs from the one selected at submit", where, r->curShader->getShaderClassName());
    }
  }
#endif

private:
  template <class ListT, class MultiListT>
  void addObjectToRenderImpl(ListT &dst_list, MultiListT &dst_multidraw, const DynamicVariantsPolicy &policy, uint16_t ri_idx,
    int optimizationInstances, bool optimization_depth_prepass, bool ignore_optimization_instances_limits, IPoint2 ofsAndCnt, int lod,
    uint16_t pool_order, const TexStreamingContext &texCtx, float dist2, float minDist2, const RiExtraElementsToHide &elementsToHide,
    const ShaderElement *shader_override = nullptr, bool gpu_instancing = false, bool request_destr_lods = false)
  {
    if (!riExtra.isValid(ri_idx))
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
    const bool renderBrokenTreesToDepth = allowTreeDepthPrepass && isDepthPass && riPool.isTree && use_ri_depth_prepass;
    if (optimization_depth_prepass && !ignore_optimization_instances_limits && !optimizationInstances && !renderBrokenTreesToDepth)
      return;
    int count = ofsAndCnt.y;
    if (optimization_depth_prepass && !ignore_optimization_instances_limits && !renderBrokenTreesToDepth)
      count = min(count, optimizationInstances);
    if (DAGOR_LIKELY(riPool.res))
    {
      if (count > 0)
      {
        unsigned ri_lod = min<int>(lod, riPool.res->lods.size() - 1);
        riPool.res->updateReqLod(ri_lod);
        if (ri_lod < 2 && request_destr_lods)
          riPool.updateDestrModelReqLod(ri_lod);
      }

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

    VoxelSurfaceData *voxelSurface = riPool.res->lods[lod].scene->getVoxelSurface();
    uint32_t voxelSurfaceId = voxelSurface ? voxelSurface->getId() : VoxelSurfaceData::INVALID_ID;
    int voxelDataOffset = riPool.res->lods[lod].scene->getVoxelDataOffset();

    // Flag shaders and
    // Tree shaders are generally incompatible by VS with other RI shaders and with each other. // TODO: still true?
    const bool disableOptimization = riPool.isTree or voxelDataOffset != 0;

    int texLevel = texCtx.getTexLevel(riPool.res->getTexScale(lod), dist2);

    uint32_t correctedEndStage = renderBrokenTreesToDepth ? ShaderMesh::STG_atest : endStage;

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
    {
      for (uint32_t EI = allElemsIndex[startEIOfs], endEI = allElemsIndex[startEIOfs + 1], startEI = EI; EI < endEI; ++EI)
      {
        const auto &elem = allElems[EI];
        if (!elem.shader)
          continue;
        const ShaderElement *currentShader = (shader_override) ? shader_override : elem.shader;
        shaders::CombinedDynVariantState dynVarState = policy.getStates(currentShader->native());
        if (dynVarState.variant < 0)
          continue;

        if (elem.vbIdx == unitedvdata::BufPool::IDX_IB)
        {
          logwarn("Invalid united VB index in ri='%s', shader='%s', numf=%d", riExtraMap.getName(ri_idx),
            elem.shader->getShaderClassName(), elem.numf);
          continue;
        }

        const bool isTessellated = riPool.elemMask[lod].tessellation & (1 << (EI - startEI));
        RIExRenderRecord record = RIExRenderRecord(currentShader, dynVarState, pool_order, (uint16_t)elem.vstride, (uint8_t)elem.vbIdx,
          elem.drawOrder, EI - startEI, elem.primitive, IPoint2(ofsAndCnt.x, count), elem.si, elem.sv, elem.numv, elem.numf,
          elem.baseVertex, texLevel, riPool.isTree, isTessellated, disableOptimization, (uint8_t)max(counter, 0));

        if (voxelDataOffset != 0)
        {
          record.isSWVertexFetch = true;
          record.voxelSurfaceId = voxelSurfaceId;
          record.voxelDataOffset = elem.baseVertex * elem.vstride + voxelDataOffset;
        }

        const auto isPacked = is_packed_material(dynVarState.const_state);
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
                dst_list.emplace_back(eastl::move(record2));
              else
                dst_multidraw.emplace_back(eastl::move(record2));
            }
          }
          if (record.ofsAndCnt.y <= 0)
            continue;
        }

        if (!isPacked)
          dst_list.emplace_back(eastl::move(record));
        else
          dst_multidraw.emplace_back(eastl::move(record));
        if (gpu_instancing) // gpu instancing only supports one mesh per one object
          return;           // TO BE REMOVED AFTER RESOLVING ALL MESHES
      }
    }
  }

  // with use_positions, only the riResOrder indices listed in positions are visited; the skipped ones
  // have no instances at this lod, so they carry no dynamic instance entry either
  template <bool use_positions, class ListT, class MultiListT>
  void processPoolsForLodAdd(int l, int &next_dynamic_idx, dag::ConstSpan<uint16_t> positions, ListT &dst_list,
    MultiListT &dst_multidraw, const DynamicVariantsPolicy &policy, const RiGenExtraVisibility &v, dag::ConstSpan<uint16_t> riResOrder,
    const TexStreamingContext &texCtx, OptimizeDepthPrepass optimization_depth_prepass,
    IgnoreOptimizationLimits ignore_optimization_instances_limits, RiExtraRenderingSubset ri_extra_subset, int &accepted_count)
  {
    // a forced lod is not a distance, so it tells nothing about which _destr lod will be needed
    const bool requestDestrLods = isNormalPass && v.requestDestrLods && v.forcedExtraLod < 0;
    for (int j = 0, je = use_positions ? (int)positions.size() : (int)riResOrder.size(); j < je; ++j)
    {
      const int k = use_positions ? (int)positions[j] : j;
      int i = riResOrder[k] & RI_RES_ORDER_COUNT_MASK;
      IPoint2 ofsAndCnt = IPoint2(v.vbOffsets[l][i], v.vbCounts[l][i]);
      if (ri_extra_subset == RiExtraRenderingSubset::OnlyStatic && next_dynamic_idx < (int)v.dynamicRiExtraInstances.size() &&
          v.dynamicRiExtraInstances[next_dynamic_idx].lod == l && v.dynamicRiExtraInstances[next_dynamic_idx].poolId == i)
      {
        ofsAndCnt.y = v.dynamicRiExtraInstances[next_dynamic_idx].instancesOffset;
        next_dynamic_idx++;
      }
      else if (ri_extra_subset == RiExtraRenderingSubset::OnlyDynamic)
      {
        if (next_dynamic_idx < (int)v.dynamicRiExtraInstances.size() && v.dynamicRiExtraInstances[next_dynamic_idx].lod == l &&
            v.dynamicRiExtraInstances[next_dynamic_idx].poolId == i)
        {
          ofsAndCnt.x += v.dynamicRiExtraInstances[next_dynamic_idx].instancesOffset * rendinst::RIEXTRA_VECS_COUNT;
          ofsAndCnt.y -= v.dynamicRiExtraInstances[next_dynamic_idx].instancesOffset;
          next_dynamic_idx++;
        }
        else
          ofsAndCnt.y = 0;
      }
      if (ofsAndCnt.y == 0)
        continue;

      int optimizationInstances = (riResOrder[k] >> RI_RES_ORDER_COUNT_SHIFT);
      float distSq = v.minSqDistances[l][i];
      float minDistSq = v.approxInvDensities[l][i] * v.invDensityToMinSqAllowedDistance;
      RiExtraElementsToHide elementsToHide(i, v.hideMarkedMaterialsForInstances);

      addObjectToRenderImpl(dst_list, dst_multidraw, policy, i, optimizationInstances,
        optimization_depth_prepass == OptimizeDepthPrepass::Yes, ignore_optimization_instances_limits == IgnoreOptimizationLimits::Yes,
        ofsAndCnt, l, k, texCtx, distSq, minDistSq, elementsToHide, nullptr, false, requestDestrLods);
      ++accepted_count;
    }
  }

  void addObjectsToRenderMultiThreaded(const RiGenExtraVisibility &v, dag::ConstSpan<uint16_t> riResOrder, TexStreamingContext texCtx,
    OptimizeDepthPrepass optimization_depth_prepass, IgnoreOptimizationLimits ignore_optimization_instances_limits,
    RiExtraRenderingSubset ri_extra_subset, int &accepted_count)
  {
    G_ASSERT(ri_extra_subset == RiExtraRenderingSubset::All || (v.rendering & VisibilityRenderingFlag::AllowSeparateRendering));

    int visibleLods[RiExtraPool::MAX_LODS];
    int visibleLodCount = 0;
    for (int l = 0; l < RiExtraPool::MAX_LODS; ++l)
      if (v.riExLodNotEmpty & (1u << l))
        visibleLods[visibleLodCount++] = l;

    if (visibleLodCount == 0)
      return;

    // cache dynamic instance offsets. Splitting a lod needs the cursor into dynamicRiExtraInstances at an
    // arbitrary pool, which only works because visibility appends those entries in riexPoolOrder order
    uint32_t lodDynStart[RiExtraPool::MAX_LODS + 1] = {};
    if (ri_extra_subset != RiExtraRenderingSubset::All)
    {
      G_ASSERT(riResOrder.data() == v.riexPoolOrder.data());
      int idx = 0;
      for (int l = 0; l < RiExtraPool::MAX_LODS; ++l)
      {
        lodDynStart[l] = (uint32_t)idx;
        while (idx < (int)v.dynamicRiExtraInstances.size() && v.dynamicRiExtraInstances[idx].lod == l)
          ++idx;
      }
      lodDynStart[RiExtraPool::MAX_LODS] = (uint32_t)idx; // final one is the end interval for the last lod
      G_ASSERT(idx == (int)v.dynamicRiExtraInstances.size());
    }

    int totalPoolCount = 0;
    {
      TIME_PROFILE(count_pools);
      const uint16_t *__restrict riResOrderPtr = riResOrder.data();
      const int ke = (int)riResOrder.size();
      for (int vi = 0; vi < visibleLodCount; ++vi)
      {
        const unsigned short *__restrict cntPtr = v.vbCounts[visibleLods[vi]].data();
        for (int k = 0; k < ke; ++k)
          totalPoolCount += (cntPtr[riResOrderPtr[k] & (uint16_t)RI_RES_ORDER_COUNT_MASK] != 0);
      }
    }
    if (totalPoolCount == 0)
      return;

    // pool count is the work unit: a pool costs its element count, not its instance count, and heavy pools
    // clustering in creation order (shadows, per stage orders) is evened out by pulling a few chunks per worker
    const int chunksPerWorker = parallel_lod_add_chunks_per_worker;
    const bool splitLods = chunksPerWorker > 0;
    const int wantedChunks = eastl::max(1, (int)threadpool::get_num_workers()) * chunksPerWorker;
    const int targetChunkPools =
      splitLods ? eastl::max((int)parallel_lod_add_min_chunk_pools, (totalPoolCount + wantedChunks - 1) / wantedChunks)
                : totalPoolCount + 1;
    const int maxChunks = (splitLods ? wantedChunks : visibleLodCount) + 1;

    int chunkCount = 0;
    { // framemem locals, released before the merge so list grows in place there
      dag::Vector<uint16_t, framemem_allocator> positions;
      positions.resize_noinit(totalPoolCount);

      struct LodSegment
      {
        int lod;
        int posBegin, posEnd;
        int dynBegin, dynEnd;
      };
      dag::Vector<LodSegment, framemem_allocator> segments;
      dag::Vector<uint32_t, framemem_allocator> chunkSegBegin;
      segments.reserve(maxChunks + visibleLodCount);
      chunkSegBegin.reserve(maxChunks + 1);
      chunkSegBegin.push_back(0);

      {
        TIME_PROFILE(build_chunks);
        int writePos = 0;
        int chunkPools = 0;
        for (int vi = 0; vi < visibleLodCount; ++vi)
        {
          const int lod = visibleLods[vi];
          const unsigned short *__restrict cntPtr = v.vbCounts[lod].data();
          const int dynEnd = (int)lodDynStart[lod + 1];
          int dynCursor = (int)lodDynStart[lod];
          int segBeginPos = writePos, segBeginDyn = dynCursor;
          for (int k = 0, ke = riResOrder.size(); k < ke; ++k)
          {
            const uint16_t i = riResOrder[k] & (uint16_t)RI_RES_ORDER_COUNT_MASK;
            if (!cntPtr[i])
              continue;
            positions[writePos++] = (uint16_t)k;
            if (dynCursor < dynEnd && v.dynamicRiExtraInstances[dynCursor].poolId == i)
              ++dynCursor;
            if (++chunkPools >= targetChunkPools)
            {
              segments.push_back({lod, segBeginPos, writePos, segBeginDyn, dynCursor});
              chunkSegBegin.push_back(segments.size());
              segBeginPos = writePos;
              segBeginDyn = dynCursor;
              chunkPools = 0;
            }
          }
          if (writePos > segBeginPos)
            segments.push_back({lod, segBeginPos, writePos, segBeginDyn, dynCursor});
          if (!splitLods && segments.size() > chunkSegBegin.back())
            chunkSegBegin.push_back(segments.size());
        }
        if (segments.size() > chunkSegBegin.back())
          chunkSegBegin.push_back(segments.size());
        G_ASSERT(writePos == totalPoolCount);
      }

      chunkCount = (int)chunkSegBegin.size() - 1;
      if ((int)buckets.size() < chunkCount)
        buckets.resize(chunkCount);
      for (int ci = 0; ci < chunkCount; ++ci)
        buckets[ci].reset();

      const DynamicVariantsPolicy &sharedPolicy = static_cast<DynamicVariantsPolicy &>(*this);
      const uint16_t *positionsPtr = positions.data();
      const LodSegment *segmentsPtr = segments.data();
      const uint32_t *chunkSegBeginPtr = chunkSegBegin.data();

      threadpool::parallel_for_inline_caller_first(0u, (uint32_t)chunkCount, 1u, [&](uint32_t tbegin, uint32_t tend, uint32_t) {
        for (uint32_t ci = tbegin; ci < tend; ++ci)
        {
          ChunkBucket &b = buckets[ci];
          TIME_D3D_PROFILE(process_lod_add);
          for (uint32_t si = chunkSegBeginPtr[ci], sie = chunkSegBeginPtr[ci + 1]; si < sie; ++si)
          {
            const LodSegment &seg = segmentsPtr[si];
            int nextDynamicIdx = seg.dynBegin;
            processPoolsForLodAdd<true>(seg.lod, nextDynamicIdx,
              make_span_const(positionsPtr + seg.posBegin, seg.posEnd - seg.posBegin), b.list, b.multidraw, sharedPolicy, v,
              riResOrder, texCtx, optimization_depth_prepass, ignore_optimization_instances_limits, ri_extra_subset, b.acceptedCount);
            G_ASSERT(ri_extra_subset == RiExtraRenderingSubset::All || nextDynamicIdx == seg.dynEnd);
          }
          DA_PROFILE_TAG(process_lod_add, "chunk %d/%d, accepted = %d", (int)ci, chunkCount, b.acceptedCount);
        }
      });
    }

    // merge buckets into main list
    size_t extraList = 0, extraMulti = 0;
    for (int ci = 0; ci < chunkCount; ++ci)
    {
      extraList += buckets[ci].list.size();
      extraMulti += buckets[ci].multidraw.size();
      accepted_count += buckets[ci].acceptedCount;
    }
    list.reserve(list.size() + extraList);
    multidrawList.reserve(multidrawList.size() + extraMulti);
    for (int ci = 0; ci < chunkCount; ++ci)
    {
      auto &bl = buckets[ci].list;
      auto &bm = buckets[ci].multidraw;
      list.insert(list.end(), eastl::make_move_iterator(bl.begin()), eastl::make_move_iterator(bl.end()));
      multidrawList.insert(multidrawList.end(), eastl::make_move_iterator(bm.begin()), eastl::make_move_iterator(bm.end()));
    }
  }

  void addObjectsToRenderSingleThreaded(const RiGenExtraVisibility &v, dag::ConstSpan<uint16_t> riResOrder, TexStreamingContext texCtx,
    OptimizeDepthPrepass optimization_depth_prepass, IgnoreOptimizationLimits ignore_optimization_instances_limits,
    RiExtraRenderingSubset ri_extra_subset, int &accepted_count)
  {
    G_ASSERT(ri_extra_subset == RiExtraRenderingSubset::All || (v.rendering & VisibilityRenderingFlag::AllowSeparateRendering));
    int nextDynamicIdx = 0;
    const DynamicVariantsPolicy &sharedPolicy = static_cast<const DynamicVariantsPolicy &>(*this);
    for (int l = 0; l < RiExtraPool::MAX_LODS; l++)
    {
      if (!(v.riExLodNotEmpty & (1 << l)))
        continue;
      processPoolsForLodAdd<false>(l, nextDynamicIdx, {}, list, multidrawList, sharedPolicy, v, riResOrder, texCtx,
        optimization_depth_prepass, ignore_optimization_instances_limits, ri_extra_subset, accepted_count);
    }
    G_ASSERT(ri_extra_subset == RiExtraRenderingSubset::All || nextDynamicIdx == v.dynamicRiExtraInstances.size());
  }

public:
  void addObjectsToRender(const RiGenExtraVisibility &v, dag::ConstSpan<uint16_t> riResOrder, TexStreamingContext texCtx,
    OptimizeDepthPrepass optimization_depth_prepass = OptimizeDepthPrepass::No,
    IgnoreOptimizationLimits ignore_optimization_instances_limits = IgnoreOptimizationLimits::No,
    RiExtraRenderingSubset ri_extra_subset = RiExtraRenderingSubset::All)
  {
    TIME_D3D_PROFILE(ri_extra_add_objects_to_render);
    int acceptedCount = 0;
    if (ri_extra_subset != RiExtraRenderingSubset::OnlyDynamic && parallel_lod_add_enabled &&
        riResOrder.size() > parallel_lod_add_min_pool_count && (!parallel_lod_add_main_thread_only || is_main_thread()))
      addObjectsToRenderMultiThreaded(v, riResOrder, texCtx, optimization_depth_prepass, ignore_optimization_instances_limits,
        ri_extra_subset, acceptedCount);
    else
      addObjectsToRenderSingleThreaded(v, riResOrder, texCtx, optimization_depth_prepass, ignore_optimization_instances_limits,
        ri_extra_subset, acceptedCount);
    DA_PROFILE_TAG(ri_extra_add_objects_to_render, "resCnt = %d, instances = %d/%d", riResOrder.size(), acceptedCount,
      dag::popcount(v.riExLodNotEmpty) * (int)riResOrder.size());
  }
};

} // namespace rendinst::render
