// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_resetDevice.h>
#include <3d/dag_quadIndexBuffer.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <shaders/dag_shaderVarsUtils.h>
#include <shaders/dag_shStateBlockBindless.h>
#include <generic/dag_relocatableFixedVector.h>
#include <riGen/riGenData.h>
#include <riGen/riRotationPalette.h>
#include <riGen/riGenRenderer.h>
#include <render/drawOrder.h>
#include <render/riShaderConstBuffers.h>
#include <render/genRender.h>
#include <render/debugMesh.h>
#include <rendInst/packedMultidrawParams.hlsli>

static constexpr int INVALID = -1;

namespace rendinst::render
{

dag::RelocatableFixedVector<bool, 2> RiGenRenderer::packedDataUpToDateForLayer = {};

extern int ri_vertex_data_no;

static inline void updateInstancePositions(const Tab<vec4f> &instance_data, int offset, int count)
{
  rendinst::render::RiShaderConstBuffers::setInstancePositions(reinterpret_cast<const float *>(&instance_data[offset]), count);
}

static inline int getInstanceLod(const RendInstGenData::RtData &rt_data, int ri_idx, int lod)
{
  auto lodTranslation = rendinst::MAX_LOD_COUNT - rt_data.riResLodCount(ri_idx);
  if (lodTranslation && !rt_data.rtPoolData[ri_idx]->hasImpostor())
    lodTranslation--;

  return lod + lodTranslation;
}

dag::Vector<RiShaderConstBuffers> &RiGenRenderer::getPerDrawData(int layer)
{
  static dag::RelocatableFixedVector<dag::Vector<RiShaderConstBuffers>, 2> drawData;
  drawData.resize(max(static_cast<int>(drawData.size()), layer + 1));
  return drawData[layer];
}

static const RendInstGenData::CellRtData *getCellRtData(const RiGenVisibility &visibility, dag::ConstSpan<RendInstGenData::Cell> cells,
  int lod_idx, int cell_idx, IPoint2 cell_dim)
{
  const auto x = visibility.cellsLod[lod_idx][cell_idx].x;
  const auto z = visibility.cellsLod[lod_idx][cell_idx].z;
  const auto cellId = x + z * cell_dim.x;
  if (cellId >= cells.size())
  {
    logerr("RiGenVisibility::cellsLod[%d][%d] contained an invalid cell (%d, %d) while rendering riGen!"
           " Cell dimensions were (%d, %d)",
      lod_idx, cell_idx, x, z, cell_dim.x, cell_dim.y);
    return nullptr;
  }
  const auto &cell = cells[cellId];
  const auto cellRtData = cell.isReady();
  if (!cellRtData)
    return nullptr;
  G_ASSERT(cellRtData->cellVbId);
  return cellRtData;
}

static int getMeshDebugValue(dag::Span<ShaderMesh::RElem> elems, uint32_t atest_skip_mask, uint32_t atest_mask, int default_value)
{
  int debugValue = default_value;
  G_UNUSED(elems);
  G_UNUSED(atest_skip_mask);
  G_UNUSED(atest_mask);
#if DAGOR_DBGLEVEL > 0
  if (debug_mesh::is_enabled(debug_mesh::Type::drawElements)) // -V1051
  {
    debugValue = 0;
    for (unsigned int elemNo = 0; elemNo < elems.size(); elemNo++, atest_mask >>= 1)
    {
      if (!elems[elemNo].e)
        continue;
      if ((atest_mask & 1) == atest_skip_mask)
        continue;
      debugValue++;
    }
  }
#endif
  return debugValue;
}

static bool skipObject(const RendInstGenData::RtData &rt_data, const RiGenVisibility &visibility, uint32_t ri_idx)
{
  if (!rt_data.rtPoolData[ri_idx] || rt_data.isHiddenId(ri_idx))
    return true;

  if (rendinst::isResHidden(rt_data.riResHideMask[ri_idx]))
    return true;

  if (!visibility.renderRanges[ri_idx].hasOpaque() && !visibility.renderRanges[ri_idx].hasTransparent())
    return true;

  return false;
}

RiGenRenderer::RiGenRenderer(RenderPass render_pass, LayerFlags layer_flags, bool optimization_depth_prepass, bool depth_optimized,
  bool depth_prepass_for_impostors, bool depth_prepass_for_cells, int per_inst_data_dwords, int draw_order_var_id) :
  renderPass(render_pass),
  layerFlags(layer_flags),
  optimizationDepthPrepass(optimization_depth_prepass),
  depthOptimized(depth_optimized),
  impostorDepthPrepass(depth_prepass_for_impostors),
  cellDepthPrepass(depth_prepass_for_cells),
  perInstDataDwords(per_inst_data_dwords),
  drawOrderVarId(draw_order_var_id)
{
  if (renderPass == rendinst::RenderPass::Normal)
  {
    if (layerFlags == rendinst::LayerFlag::Decals)
      startStage = endStage = ShaderMesh::STG_decal;
    else if (layerFlags == rendinst::LayerFlag::Transparent)
      startStage = endStage = ShaderMesh::STG_trans;
    else
    {
      startStage = ShaderMesh::STG_opaque;
      endStage = ShaderMesh::STG_imm_decal;
    }
  }
  else
  {
    startStage = ShaderMesh::STG_opaque;
    endStage = ShaderMesh::STG_atest;
  }
}

void RiGenRenderer::updatePerDrawData(RendInstGenData::RtData &rt_data, int per_inst_data_dwords)
{
  TIME_PROFILE(ri_gen_update_per_draw_data);

  G_UNUSED(per_inst_data_dwords);
  auto &drawData = getPerDrawData(rt_data.layerIdx);
  drawData.resize(rt_data.riRes.size());
  bool layerHasData = false;
  for (int riIdx = 0; riIdx < rt_data.riRes.size(); ++riIdx)
  {
    if (!rt_data.rtPoolData[riIdx])
      continue;

    layerHasData = true;

    if (rt_data.layerIdx > 0)
      logerr("riGen (%s) found on layer #%d. Such objects should only exist on layer #0!", rt_data.riResName[riIdx], rt_data.layerIdx);

    auto &riDrawData = drawData[riIdx];

    const uint32_t vectorsCnt =
      rt_data.riPosInst[riIdx]
        ? 1
        : 3 + RIGEN_ADD_STRIDE_PER_INST_B(rt_data.riZeroInstSeeds[riIdx], per_inst_data_dwords) / RENDER_ELEM_SIZE_PACKED;
    const uint32_t flags = (rt_data.riZeroInstSeeds[riIdx] == 0) && (per_inst_data_dwords != 0) ? 2 : 0;

    auto range = rt_data.rtPoolData[riIdx]->lodRange[rt_data.riResLodCount(riIdx) - 1];
    range = rt_data.rtPoolData[riIdx]->hasImpostor() ? rt_data.get_trees_last_range(range) : rt_data.get_last_range(range);

    const auto deltaRcp = rt_data.transparencyDeltaRcp / range;
    if (rt_data.rtPoolData[riIdx]->hasTransitionLod())
    {
      const auto impostorLodNum = rt_data.riResLodCount(riIdx) - 1;
      const auto transitionLodNum = impostorLodNum - 1;
      const auto lastMeshLodNum = transitionLodNum - 1;
      const auto defaultTransitionLodRange = rt_data.riResLodRange(riIdx, transitionLodNum, nullptr);
      const auto transitionRange = defaultTransitionLodRange - rt_data.riResLodRange(riIdx, lastMeshLodNum, nullptr);
      const auto transitionStart = rt_data.get_trees_range(rt_data.rtPoolData[riIdx]->lodRange[impostorLodNum]) - transitionRange;
      riDrawData.setCrossDissolveRange(safediv(transitionStart, rendinst::render::lodsShiftDistMul));
    }
    if (rt_data.rtPoolData[riIdx]->hasImpostor())
      rt_data.rtPoolData[riIdx]->setShadowImpostorBoundingSphere(riDrawData);
    riDrawData.setOpacity(-deltaRcp, range * deltaRcp);
    riDrawData.setRandomColors(&rt_data.riColPair[riIdx * 2]);
    riDrawData.setInstancing(0, vectorsCnt, flags,
      rendinst::gen::get_rotation_palette_manager()->getImpostorDataBufferOffset({rt_data.layerIdx, riIdx},
        rt_data.rtPoolData[riIdx]->impostorDataOffsetCache));
    riDrawData.setInteractionParams(1, rt_data.riRes[riIdx]->bbox.lim[1].y - rt_data.riRes[riIdx]->bbox.lim[0].y,
      0.5 * (rt_data.riRes[riIdx]->bbox.lim[1].x + rt_data.riRes[riIdx]->bbox.lim[0].x),
      0.5 * (rt_data.riRes[riIdx]->bbox.lim[1].z + rt_data.riRes[riIdx]->bbox.lim[0].z));
    if (rt_data.rtPoolData[riIdx]->hasPLOD())
      riDrawData.setPLODRadius(rt_data.rtPoolData[riIdx]->plodRadius);
    riDrawData.setBoundingSphere(0, 0, rt_data.rtPoolData[riIdx]->sphereRadius, rt_data.rtPoolData[riIdx]->sphereRadius,
      rt_data.rtPoolData[riIdx]->sphCenterY);
  }

  if (layerHasData)
  {
    packedDataUpToDateForLayer.resize(max(static_cast<int>(packedDataUpToDateForLayer.size()), rt_data.layerIdx + 1));
    packedDataUpToDateForLayer[rt_data.layerIdx] = false; // force per draw buffer update
  }
}

template <typename RenderRecordCB>
static void addLodInstances(RendInstGenData::RtData &rt_data, const RiGenVisibility &visibility, int ri_idx, int lod_idx,
  int start_stage, int end_stage, bool impostor_depth_prepass, bool optimization_depth_prepass, int draw_order_var_id,
  const RenderRecordCB &add_record_cb)
{
  const auto rendInstRes = rt_data.riResLodScene(ri_idx, lod_idx);
  const auto mesh = rendInstRes->getMesh()->getMesh()->getMesh();
  const bool isBakedImpostor = rt_data.riRes[ri_idx]->isBakedImpostor();
  const auto lodCnt = rt_data.riResLodCount(ri_idx);
  const auto isImpostorLod = lod_idx >= lodCnt - 1 && rt_data.riPosInst[ri_idx] ? 1 : 0;

  if (!impostor_depth_prepass && optimization_depth_prepass && isBakedImpostor && isImpostorLod)
    return;

  const auto maskOffset = lod_idx < lodCnt ? lod_idx : lodCnt - 1;
  auto atestMask = rt_data.riResElemMask[ri_idx * rendinst::MAX_LOD_COUNT + maskOffset].atest;
  auto plodMask = rt_data.riResElemMask[ri_idx * rendinst::MAX_LOD_COUNT + maskOffset].plod;

  for (auto stage = start_stage; stage <= end_stage; ++stage)
  {
    const auto elems = mesh->getElems(stage);

    const auto meshDebugValue = getMeshDebugValue(elems, visibility.atest_skip_mask, atestMask, lod_idx);

    for (uint32_t elemNo = 0; elemNo < elems.size(); elemNo++, atestMask >>= 1, plodMask >>= 1)
    {
      if (!elems[elemNo].e)
        continue;

      if ((atestMask & 1) == visibility.atest_skip_mask)
        continue;

      const auto &elem = elems[elemNo];

      uint32_t prog;
      ShaderStateBlockId state;
      shaders::ConstStateIdx cstate;
      shaders::TexStateIdx tstate;
      shaders::RenderStateId rstate;

      const auto curVar = get_dynamic_variant_states(elem.e->native(), prog, state, rstate, cstate, tstate);

      if (curVar < 0)
        continue;

      const PackedDrawOrder drawOrder{elem, (unsigned int)stage, draw_order_var_id};
      const auto baseRecord = RiGenRenderRecord(elem.e, curVar, prog, state, rstate, tstate, cstate, INVALID, drawOrder, stage,
        (uint16_t)elem.vertexData->getStride(), (uint8_t)elem.vertexData->getVbIdx(), INVALID, INVALID, ri_idx, elem.si, elem.numf,
        elem.baseVertex, elem.numv, elem.numf, elem.getPrimitive(), RiGenRenderRecord::INVALID, INVALID, meshDebugValue, plodMask & 1);

      add_record_cb(baseRecord);
    }
  }
}

void RiGenRenderer::addInstanceVisibleObjects(RendInstGenData::RtData &rt_data, const RiGenVisibility &visibility)
{
  TIME_PROFILE(ri_gen_add_per_instance_objects);

  rendinst::render::setCoordType(rendinst::render::COORD_TYPE_POS_CB);
  for (int instanceLod = 0; instanceLod < visibility.PER_INSTANCE_LODS; ++instanceLod)
  {
    if (!visibility.perInstanceVisibilityCells[instanceLod].size())
      continue;
    for (int cell = 0; cell < visibility.perInstanceVisibilityCells[instanceLod].size() - 1; ++cell)
    {

      const auto riIdx = visibility.perInstanceVisibilityCells[instanceLod][cell].x;

      if (!rt_data.rtPoolData[riIdx] || rt_data.isHiddenId(riIdx))
        continue;

      // Alpha is not real lod, so alpha and impostor are mapped to the same ri lod index
      int lodIdx = instanceLod == RiGenVisibility::PI_ALPHA_LOD ? RiGenVisibility::PI_IMPOSTOR_LOD : instanceLod;
      int lodTranslation = rendinst::MAX_LOD_COUNT - rt_data.riResLodCount(riIdx);
      if (lodTranslation > 0 && !rt_data.rtPoolData[riIdx]->hasImpostor())
        lodTranslation--;

      lodIdx -= lodTranslation;

      const auto addRecordCb = [&](const RiGenRenderRecord &base_record) {
        auto record = base_record;
        record.offset = visibility.perInstanceVisibilityCells[instanceLod][cell].y;
        record.count = visibility.perInstanceVisibilityCells[instanceLod][cell + 1].y - record.offset;
        record.visibility = record.PER_INSTANCE;
        record.instanceLod = instanceLod;
        const auto isPacked = is_packed_material(record.cstate);
        auto recordOffset = record.offset;
        auto instancesLeft = record.count;
        G_ASSERT(!isPacked || record.startIndex != RELEM_NO_INDEX_BUFFER);
        while (instancesLeft > 0)
        {
          record.offset = recordOffset;
          record.count = eastl::min(instancesLeft, static_cast<uint32_t>(rendinst::render::MAX_INSTANCES));
          recordOffset += record.count;
          instancesLeft -= record.count;
          if (isPacked)
            packedRenderRecords.push_back(record);
          else
            renderRecords.push_back(record);
        }
      };
      addLodInstances(rt_data, visibility, riIdx, lodIdx, startStage, endStage, impostorDepthPrepass, optimizationDepthPrepass,
        drawOrderVarId, addRecordCb);
    }
  }
}

void RiGenRenderer::addCellVisibleObjects(RendInstGenData::RtData &rt_data, const RiGenVisibility &visibility,
  dag::ConstSpan<RendInstGenData::Cell> cells, IPoint2 cell_dim)
{
  TIME_PROFILE(ri_gen_add_cell_objects);

  const bool isNormalDecalPass = renderPass == rendinst::RenderPass::Normal && layerFlags == rendinst::LayerFlag::Decals;
  const auto &riResOrder =
    isNormalDecalPass ? rt_data.riResIdxPerStage[get_layer_index(rendinst::LayerFlag::Decals)] : rt_data.riResOrder;

  for (size_t poolOrder = 0; poolOrder < riResOrder.size(); ++poolOrder)
  {
    const auto riIdx = riResOrder[poolOrder];

    if (skipObject(rt_data, visibility, riIdx))
      continue;

    if (rt_data.rtPoolData[riIdx]->hasImpostor() && isNormalDecalPass)
      continue;

    auto curCoordType = rendinst::render::COORD_TYPE_POS;
    rendinst::render::setCoordType(curCoordType);

    auto recordsCount = renderRecords.size();
    for (auto lodIdx = rt_data.riResFirstLod(riIdx), lodCount = rt_data.riResLodCount(riIdx); lodIdx <= lodCount; ++lodIdx)
    {
      const bool posInst = rt_data.riPosInst[riIdx] ? 1 : 0;
      const auto riCoordType = posInst ? rendinst::render::COORD_TYPE_POS : rendinst::render::COORD_TYPE_TM;
      uint32_t stride = RIGEN_STRIDE_B(posInst, rt_data.riZeroInstSeeds[riIdx], perInstDataDwords);
      const uint32_t vectorsCnt =
        posInst ? 1 : 3 + RIGEN_ADD_STRIDE_PER_INST_B(rt_data.riZeroInstSeeds[riIdx], perInstDataDwords) / RENDER_ELEM_SIZE_PACKED;

      if (eastl::exchange(curCoordType, riCoordType) != riCoordType)
        rendinst::render::setCoordType(riCoordType);

      const auto addRecordCb = [&](const RiGenRenderRecord &base_record) {
        for (auto cellIdx = visibility.renderRanges[riIdx].startCell[lodIdx],
                  cellEndIdx = visibility.renderRanges[riIdx].endCell[lodIdx];
             cellIdx < cellEndIdx; cellIdx++)
        {
          const auto cellRtData = getCellRtData(visibility, cells, lodIdx, cellIdx, cell_dim);

          if (!cellRtData)
            continue;

          const auto cellBufferRegion = rt_data.cellsVb.get(cellRtData->cellVbId);

          G_ASSERT(visibility.getRangesCount(lodIdx, cellIdx) > 0);
          for (uint32_t ri = 0, re = visibility.getRangesCount(lodIdx, cellIdx); ri < re; ++ri)
          {
            const auto ofs = visibility.getOfs(lodIdx, cellIdx, ri, stride);
            const auto count = visibility.getCount(lodIdx, cellIdx, ri);

            G_ASSERT(ofs >= cellRtData->pools[riIdx].baseOfs);
            G_ASSERT(count <= cellRtData->pools[riIdx].total);
            G_ASSERT((count * stride + cellBufferRegion.offset + ofs) <= rt_data.cellsVb.getHeap().getBuf()->ressize());
            G_ASSERT((count * stride + ofs) <= cellBufferRegion.size);
            G_ASSERT(count > 0);

            const auto cellBufferOffset = cellBufferRegion.offset / RENDER_ELEM_SIZE + ofs * vectorsCnt / stride;

            auto record = base_record;
            record.poolOrder = poolOrder;
            record.offset = cellBufferOffset;
            record.count = count;
            record.visibility = record.PER_CELL;
            record.instanceLod = getInstanceLod(rt_data, riIdx, lodIdx);

            const auto isPacked = is_packed_material(record.cstate);
            G_ASSERT(!isPacked || record.startIndex != RELEM_NO_INDEX_BUFFER);

            if (isPacked)
              packedRenderRecords.emplace_back(eastl::move(record));
            else
              renderRecords.emplace_back(eastl::move(record));
          }
        }
      };

      addLodInstances(rt_data, visibility, riIdx, lodIdx, startStage, endStage, impostorDepthPrepass, optimizationDepthPrepass,
        drawOrderVarId, addRecordCb);
      recordsCount = renderRecords.size() - recordsCount;
    }
#if _TARGET_PC_TOOLS_BUILD
    if (isNormalDecalPass)
      stlsort::sort(renderRecords.begin() + recordsCount, renderRecords.end(),
        [&](const auto &r1, const auto &r2) { return r1.drawOrder < r2.drawOrder; });
#endif
  }
}

void RiGenRenderer::sortObjects()
{
  TIME_PROFILE(ri_gen_sort_objects);

  stlsort::sort(renderRecords.begin(), renderRecords.end(), [&](const auto &r1, const auto &r2) {
    if (r1.drawOrder != r2.drawOrder)
      return r1.drawOrder < r2.drawOrder;
    if (DAGOR_UNLIKELY(r1.vstride != r2.vstride))
      return r1.vstride < r2.vstride;
    if (DAGOR_UNLIKELY(r1.vbIdx != r2.vbIdx))
      return r1.vbIdx < r2.vbIdx;
    if (renderPass != RenderPass::Depth || r1.stage != ShaderMesh::STG_opaque)
    {
      if (r1.state != r2.state)
      {
        if (r1.tstate != r2.tstate)
          return r1.tstate < r2.tstate;
        if (r1.rstate != r2.rstate)
          return r1.rstate < r2.rstate;
        return r1.state < r2.state;
      }
      if (r1.prog != r2.prog)
        return r1.prog < r2.prog;
    }
    else if (r1.rstate != r2.rstate)
      return r1.rstate < r2.rstate;
    if (r1.poolOrder != r2.poolOrder)
      return r1.poolOrder < r2.poolOrder;
    return r1.offset < r2.offset;
  });
}

void RiGenRenderer::sortPackedObjects()
{
  TIME_PROFILE(ri_gen_sort_packed_objects);

  const auto firstCellRecord = eastl::find_if(packedRenderRecords.begin(), packedRenderRecords.end(),
    [](const auto &r1) { return r1.visibility == r1.PER_CELL; });

  // We sort only per instance visibility, because per cell records are already sorted
  stlsort::sort(packedRenderRecords.begin(), firstCellRecord, [&](const auto &r1, const auto &r2) {
    if (DAGOR_UNLIKELY(r1.drawOrder != r2.drawOrder))
      return r1.drawOrder < r2.drawOrder;
    if (get_material_id(r1.cstate) != get_material_id(r2.cstate))
      return get_material_id(r1.cstate) > get_material_id(r2.cstate);
    if (DAGOR_UNLIKELY(r1.vstride != r2.vstride))
      return r1.vstride < r2.vstride;
    if (DAGOR_UNLIKELY(r1.vbIdx != r2.vbIdx))
      return r1.vbIdx < r2.vbIdx;
    if (renderPass != RenderPass::Depth || r1.stage != ShaderMesh::STG_opaque)
    {
      if (r1.state != r2.state)
      {
        if (r1.tstate != r2.tstate)
          return r1.tstate < r2.tstate;
        if (r1.rstate != r2.rstate)
          return r1.rstate < r2.rstate;
        return r1.state < r2.state;
      }
      if (r1.prog != r2.prog)
        return r1.prog < r2.prog;
    }
    else if (r1.rstate != r2.rstate)
      return r1.rstate < r2.rstate;
    if (r1.poolOrder != r2.poolOrder)
      return r1.poolOrder < r2.poolOrder;
    return r1.offset < r2.offset;
  });
}

void RiGenRenderer::updatePackedData(int layer)
{
  if (layer >= packedDataUpToDateForLayer.size())
    return;
  if (DAGOR_UNLIKELY(!eastl::exchange(packedDataUpToDateForLayer[layer], true)))
  {
    const auto &layerPerDrawData = getPerDrawData(layer);
    if (!layerPerDrawData.empty())
    {
      rendinst::render::ensurePerDrawBufferExists(layer);
      rendinst::render::riGenPerDrawDataForLayer[layer]->updateData(0,
        layerPerDrawData.size() * sizeof(rendinst::render::RiShaderConstBuffers), layerPerDrawData.data(), 0);
    }
  }
}

void RiGenRenderer::render(const RendInstGenData::RtData &rt_data, const RiGenVisibility &visibility)
{
  renderObjects(rt_data, visibility);
  renderPackedObjects(rt_data, visibility);
}

void RiGenRenderer::renderObjects(const RendInstGenData::RtData &rt_data, const RiGenVisibility &visibility)
{
  TIME_D3D_PROFILE(ri_gen_render_objects);

  if (renderRecords.empty())
    return;

#if _TARGET_PC_TOOLS_BUILD
  // This condition enables sorting only for tools (daAV, daEditor) for performance reason:
  // Sorting is required for correct order of rendinst-decals via draw_order material param,
  // currently rendinst-decals are rendered via riGen only in daEditor thus it's not required to be enabled in the game.
  sortObjects();
#endif

  uint8_t curVbIdx = INVALID;
  uint16_t curStride = INVALID;
  shaders::RenderStateId curRState = shaders::RenderStateId::Invalid;
  uint16_t curVariant = INVALID;
  uint32_t curProg = INVALID;
  ShaderStateBlockId curState = ShaderStateBlockId::Invalid;
  uint32_t curPoolIdx = INVALID;
  IPoint2 curOfsAndVertexByteStart = {INVALID, INVALID};
  uint8_t curInstanceLod = INVALID;
  bool curRenderQuad = false;
  bool currentDepthPrepass = false;
  d3d_err(d3d::setind(unitedvdata::riUnitedVdata.getIB()));
  for (const auto &record : renderRecords)
  {
    G_ASSERT(record.count != INVALID);
    G_ASSERT(record.offset != INVALID);

    auto skipApply = false;
    if (record.count == 0)
      continue;

    const IPoint2 ofsAndVertexByteStart = {static_cast<int>(record.offset), record.startVertex * record.vstride};
    if (eastl::exchange(curOfsAndVertexByteStart, ofsAndVertexByteStart) != ofsAndVertexByteStart)
      d3d::set_immediate_const(STAGE_VS, (uint32_t *)&ofsAndVertexByteStart.x, 2);

    const auto &drawData = getPerDrawData(rt_data.layerIdx);
    if (eastl::exchange(curPoolIdx, record.poolIdx) != record.poolIdx)
    {
      G_ASSERT(record.poolIdx < drawData.size());
      drawData[record.poolIdx].flushPerDraw();
    }

    // impostor shadow requires a quad rendered instead of the octahedral mesh of the lod, otherwise corners will not be rendered
    bool renderQuad = (renderPass == RenderPass::ToShadow && record.instanceLod == RiGenVisibility::PI_IMPOSTOR_LOD);

    if (renderQuad != curRenderQuad)
    {
      if (!renderQuad)
      {
        d3d_err(d3d::setind(unitedvdata::riUnitedVdata.getIB()));
        curVbIdx = INVALID;
        curStride = INVALID;
      }
      else
      {
        index_buffer::use_quads_16bit();
        d3d::setvdecl(BAD_VDECL);
        d3d::setvsrc(0, NULL, 0);
      }
      curRenderQuad = renderQuad;
    }

    if (curVbIdx != record.vbIdx || curStride != record.vstride)
    {
      Vbuffer *vb = unitedvdata::riUnitedVdata.getVB(record.vbIdx);
      if (record.isSWVertexFetch && curVbIdx != record.vbIdx)
      {
        G_ASSERT(ri_vertex_data_no != -1);
        d3d::set_buffer(STAGE_VS, ri_vertex_data_no, vb);
        d3d::set_buffer(STAGE_PS, ri_vertex_data_no, vb);
      }
      curVbIdx = record.vbIdx;
      curStride = record.vstride;
      d3d_err(d3d::setvsrc(0, unitedvdata::riUnitedVdata.getVB(curVbIdx), curStride));
    }

    if (record.stage == ShaderMesh::STG_opaque && curRState == record.rstate && record.variant == curVariant &&
        record.prog == curProg && record.state == curState)
      skipApply = true;

    const bool isBakedImpostor = rt_data.riRes[record.poolIdx]->isBakedImpostor();
    const bool isImpostor = !isBakedImpostor && (record.instanceLod == RiGenVisibility::PI_IMPOSTOR_LOD);

    if (rt_data.rtPoolData[record.poolIdx]->hasImpostor() && !isImpostor)
    {
      const auto tcConsts = rendinst::render::getCommonImmediateConstants();
      const auto cacheOffset =
        static_cast<uint32_t>(rt_data.rtPoolData[record.poolIdx]->impostorDataOffsetCache & rendinst::gen::CACHE_OFFSET_MASK);
      uint32_t immediateConsts[] = {cacheOffset, tcConsts[0], tcConsts[1]};
      d3d::set_immediate_const(STAGE_PS, immediateConsts, sizeof(immediateConsts) / sizeof(immediateConsts[0]));
    }

    const bool afterDepthPrepass =
      renderPass == rendinst::RenderPass::Normal && record.instanceLod <= RiGenVisibility::PI_LAST_MESH_LOD && depthOptimized &&
      (record.visibility == record.PER_INSTANCE || (cellDepthPrepass && record.visibility == record.PER_CELL));

    if (eastl::exchange(currentDepthPrepass, afterDepthPrepass) != afterDepthPrepass)
    {
      if (afterDepthPrepass)
        shaders::overrides::set(rendinst::render::afterDepthPrepassOverride);
      else
        shaders::overrides::reset();
    }

    if (debug_mesh::set_debug_value(record.meshDebugValue))
      skipApply = false;

    if (!skipApply)
    {
      set_states_for_variant(record.curShader->native(), record.variant, record.prog, record.state);
      if (renderPass == RenderPass::ToShadow)
        d3d::settex(dynamic_impostor_texture_const_no + DYNAMIC_IMPOSTOR_TEX_SHADOW_OFFSET,
          rt_data.rtPoolData[record.poolIdx]->rendinstGlobalShadowTex.getArrayTex());
    }

    curRState = record.rstate;
    curVariant = record.variant;
    curState = record.state;
    curProg = record.prog;

    auto instancesPerDraw = record.count;
    if (record.visibility == record.PER_INSTANCE)
    {
      G_ASSERT(instancesPerDraw <= rendinst::render::MAX_INSTANCES);
      if (record.offset + record.count > rendinst::render::MAX_INSTANCES)
      {
        curInstanceLod = INVALID; // Force next buffer update, because we will invalidate data
        curOfsAndVertexByteStart.x = 0;
        d3d::set_immediate_const(STAGE_VS, (uint32_t *)&curOfsAndVertexByteStart.x, 1);
        updateInstancePositions(visibility.instanceData[record.instanceLod], record.offset, instancesPerDraw);
      }
      else if (eastl::exchange(curInstanceLod, record.instanceLod) != record.instanceLod)
      {
        const auto updateCount =
          min(visibility.instanceData[record.instanceLod].size(), static_cast<unsigned>(rendinst::render::MAX_INSTANCES));
        updateInstancePositions(visibility.instanceData[record.instanceLod], 0, updateCount);
      }
    }

    if (record.startIndex != RELEM_NO_INDEX_BUFFER)
    {
      if (!curRenderQuad)
        d3d::drawind_instanced(PRIM_TRILIST, record.startIndex, record.numFaces, record.baseVertex, instancesPerDraw, 0);
      else
        d3d::drawind_instanced(record.primitive, 0, 2, 0, instancesPerDraw, 0);
    }
    else
    {
      d3d::draw_instanced(record.primitive, record.startVertex, record.numVertex, instancesPerDraw, 0);
    }

    if (rt_data.rtPoolData[record.poolIdx]->hasImpostor() && !isImpostor)
      d3d::set_immediate_const(STAGE_PS, nullptr, 0);
  }

  if (currentDepthPrepass)
    shaders::overrides::reset();

  debug_mesh::reset_debug_value();
}

RiGenRenderer::MultiDrawRenderer RiGenRenderer::getMultiDrawRenderer()
{
  return riGenMultidrawContext.fillBuffers(packedRenderRecords.size(),
    [this](uint32_t draw_index, uint32_t &index_count_per_instance, uint32_t &instance_count, uint32_t &start_index,
      int32_t &base_vertex, RiGenPerInstanceParameters &params) {
      const auto &drawRecord = packedRenderRecords[draw_index];
      index_count_per_instance = static_cast<uint32_t>(drawRecord.numFaces * 3),
      instance_count = static_cast<uint32_t>(drawRecord.count), start_index = static_cast<uint32_t>(drawRecord.startIndex);
      base_vertex = static_cast<int32_t>(drawRecord.baseVertex);
      if (DAGOR_UNLIKELY(drawRecord.poolIdx >= MAX_PER_DRAW_OFFSET))
      {
        logerr("Too big ri gen pool index %d", drawRecord.poolIdx);
        instance_count = 0;
      }
      const auto materialOffset = get_material_offset(drawRecord.cstate);
      if (DAGOR_UNLIKELY(materialOffset >= MAX_MATERIAL_OFFSET))
      {
        logerr("Too big material offset %d", materialOffset);
        instance_count = 0;
      }
      params.instanceOffset = drawRecord.offset;
      if (DAGOR_UNLIKELY(drawRecord.visibility == drawRecord.PER_INSTANCE &&
                         drawRecord.offset + drawRecord.count >= rendinst::render::MAX_INSTANCES))
      {
        logwarn("RiGenRenderer per instance buffer has more than %d instances!", rendinst::render::MAX_INSTANCES);
        params.instanceOffset = 0;
      }
      const auto perDrawDataOffset = drawRecord.poolIdx * (sizeof(rendinst::render::RiShaderConstBuffers) / sizeof(vec4f)) + 1;
      params.perDrawData = (perDrawDataOffset << PER_DRAW_OFFSET_SHIFT) | materialOffset;
    });
}

void RiGenRenderer::renderPackedObjects(const RendInstGenData::RtData &rt_data, const RiGenVisibility &visibility)
{
  G_UNUSED(rt_data);
  TIME_D3D_PROFILE(ri_gen_render_packed_objects);

  if (packedRenderRecords.empty())
    return;

  sortPackedObjects();
  if (debug_mesh::is_enabled())
    coalescePackedRecords<true>();
  else
    coalescePackedRecords<false>();

  for (auto stateIdTexLevel : bindlessStatesToUpdateTexLevels)
    update_bindless_state(stateIdTexLevel.first, stateIdTexLevel.second);

  const auto multiDrawRenderer = getMultiDrawRenderer();
  // Need to set shader block again because `ri_gen_multidraw_perDrawArgsBuffer` buffer can be recreated.
  ShaderGlobal::setBlock(ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE), ShaderGlobal::LAYER_SCENE);

  rendinst::render::RiShaderConstBuffers cb;
  cb.setInstancing(0, 1, 0x5, 0);
  cb.flushPerDraw();

  bool currentDepthPrepass = false;

  uint8_t curVbIdx = INVALID;
  uint8_t curStride = INVALID;

  d3d_err(d3d::setind(unitedvdata::riUnitedVdata.getIB()));

  for (const auto &drawRange : packedRenderRanges)
  {
    const auto &record = packedRenderRecords[drawRange.start];
    if (curVbIdx != record.vbIdx || curStride != record.vstride)
    {
      d3d_err(d3d::setvsrc(0, unitedvdata::riUnitedVdata.getVB(record.vbIdx), record.vstride));
      curVbIdx = record.vbIdx;
      curStride = record.vstride;
    }

    if (record.visibility == record.PER_INSTANCE)
    {
      G_ASSERT(record.count <= rendinst::render::MAX_INSTANCES);
      if (DAGOR_LIKELY(record.offset + record.count <= rendinst::render::MAX_INSTANCES))
      {
        const auto updateCount =
          min(visibility.instanceData[record.instanceLod].size(), static_cast<unsigned>(rendinst::render::MAX_INSTANCES));
        updateInstancePositions(visibility.instanceData[record.instanceLod], 0, updateCount);
      }
      else
      {
        // For records exceeding the limit, the per_instance_data will only contain info for this one draw call
        G_ASSERT(drawRange.count == 1);
        updateInstancePositions(visibility.instanceData[record.instanceLod], record.offset, record.count);
      }
    }

    const bool afterDepthPrepass =
      renderPass == rendinst::RenderPass::Normal && record.instanceLod <= RiGenVisibility::PI_LAST_MESH_LOD && depthOptimized &&
      (record.visibility == record.PER_INSTANCE || (cellDepthPrepass && record.visibility == record.PER_CELL));

    if (eastl::exchange(currentDepthPrepass, afterDepthPrepass) != afterDepthPrepass)
    {
      if (afterDepthPrepass)
        shaders::overrides::set(rendinst::render::afterDepthPrepassOverride);
      else
        shaders::overrides::reset();
    }

    debug_mesh::set_debug_value(record.meshDebugValue);
    set_states_for_variant(record.curShader->native(), record.variant, record.prog, record.state);

    multiDrawRenderer.render(PRIM_TRILIST, drawRange.start, drawRange.count);
  }

  if (currentDepthPrepass)
    shaders::overrides::reset();

  debug_mesh::reset_debug_value();
}

static void RiGenRenderer_afterDeviceReset(bool /*full_reset*/)
{
  if (!RendInstGenData::renderResRequired)
    return;

  FOR_EACH_RG_LAYER_DO (rgl)
    rendinst::render::RiGenRenderer::updatePerDrawData(*rgl->rtData, rgl->perInstDataDwords);
}

REGISTER_D3D_AFTER_RESET_FUNC(RiGenRenderer_afterDeviceReset);

bool update_rigen_color(const char *name, E3DCOLOR from, E3DCOLOR to)
{
  bool ret = false;
  FOR_EACH_PRIMARY_RG_LAYER_DO (rgl)
  {
    if (rgl->updateLClassColors(name, from, to))
    {
      ret = true;
      continue;
    }

    for (int i = 0, ie = rgl->rtData->riRes.size(); i < ie; i++)
      if (rgl->rtData->riResName[i] && stricmp(rgl->rtData->riResName[i], name) == 0)
      {
        if (!rgl->rtData->riPosInst[i] && !rendinst::render::tmInstColored)
        {
          ret = true;
          break;
        }
#if RI_VERBOSE_OUTPUT
        debug("ri[%d] <%s> set from : %08X-%08X to:", i, rgl->rtData->riResName[i], rgl->rtData->riColPair[i * 2 + 0].u,
          rgl->rtData->riColPair[i * 2 + 1].u, from.u, to.u);
#endif
        rgl->rtData->riColPair[i * 2 + 0].u = from.u;
        rgl->rtData->riColPair[i * 2 + 1].u = to.u;
        rendinst::render::RiGenRenderer::updatePerDrawData(*rgl->rtData, rgl->perInstDataDwords);
        ret = true;
        break;
      }
  }
  return ret;
}

} // namespace rendinst::render
