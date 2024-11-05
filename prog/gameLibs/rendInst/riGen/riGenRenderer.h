// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ska_hash_map/flat_hash_map2.hpp>
#include <rendInst/riShaderConstBuffers.h>
#include <riGen/riGenRenderRecord.h>
#include <riGen/riGenData.h>
#include <render/genRender.h>
#include <generic/dag_relocatableFixedVector.h>
#include <generic/dag_span.h>
#include <visibility/genVisibility.h>
#include <memory/dag_framemem.h>
#include <render/riShaderConstBuffers.h>
#include <shaders/dag_shStateBlockBindless.h>

namespace rendinst::render
{

class RiGenRenderer
{
public:
  RiGenRenderer(RenderPass render_pass, LayerFlags layer_flags, bool optimization_depth_prepass, bool depth_optimized,
    bool depth_prepass_for_impostors, bool depth_prepass_for_cells, int per_inst_data_dwords, int draw_order_var_id);
  void addCellVisibleObjects(RendInstGenData::RtData &rt_data, const RiGenVisibility &visibility,
    dag::ConstSpan<RendInstGenData::Cell> cells, IPoint2 cell_dim);
  void addInstanceVisibleObjects(RendInstGenData::RtData &rt_data, const RiGenVisibility &visibility);
  void render(const RendInstGenData::RtData &rtData, const RiGenVisibility &visibility);
  void sortObjects();
  static void updatePerDrawData(RendInstGenData::RtData &rt_data, int per_inst_data_dwords);
  static void updatePackedData(int layer);

private:
  struct PackedRenderRange
  {
    uint16_t start;
    uint16_t count;
  };

private:
  using MultiDrawRenderer = MultidrawContext<RiGenPerInstanceParameters>::MultidrawRenderExecutor;

private:
  template <bool separate_mesh_debug_values>
  void coalescePackedRecords()
  {
    TIME_PROFILE(ri_gen_coalesce_packed_render_records);

    if (!packedRenderRecords.size())
      return;

    const auto isMergeable = [](const RiGenRenderRecord &r1, const RiGenRenderRecord &r2) {
      bool result = DAGOR_LIKELY(r1.vstride == r2.vstride && r1.vbIdx == r2.vbIdx) && r1.stage == r2.stage && r1.rstate == r2.rstate &&
                    r1.prog == r2.prog && get_material_id(r1.cstate) == get_material_id(r2.cstate) && r1.visibility == r2.visibility &&
                    r1.instanceLod == r2.instanceLod;

      if constexpr (separate_mesh_debug_values)
        result &= r1.meshDebugValue == r2.meshDebugValue;

      return result;
    };
    packedRenderRanges.clear();
    packedRenderRanges.emplace_back(PackedRenderRange{0, 1});
    bindlessStatesToUpdateTexLevels.emplace(packedRenderRecords.front().cstate, 15);
    const auto &firstRecord = packedRenderRecords[0];
    bool prevRecordExceededInstanceLimit =
      firstRecord.visibility == firstRecord.PER_INSTANCE && firstRecord.offset + firstRecord.count >= rendinst::render::MAX_INSTANCES;
    for (size_t i = 1, ie = packedRenderRecords.size(); i < ie; ++i)
    {
      const auto &prevRecord = packedRenderRecords[i - 1];
      const auto &curRecord = packedRenderRecords[i];
      auto &lastRange = packedRenderRanges.back();
      const bool isInstanceLimitExceeeded =
        curRecord.visibility == curRecord.PER_INSTANCE && curRecord.offset + curRecord.count >= rendinst::render::MAX_INSTANCES;
      if (isMergeable(prevRecord, curRecord) && DAGOR_LIKELY(!isInstanceLimitExceeeded && !prevRecordExceededInstanceLimit))
        lastRange.count++;
      else
        packedRenderRanges.emplace_back(PackedRenderRange{static_cast<uint16_t>(lastRange.count + lastRange.start), 1});
      const auto iter = bindlessStatesToUpdateTexLevels.find(curRecord.cstate);
      if (iter == bindlessStatesToUpdateTexLevels.end())
        bindlessStatesToUpdateTexLevels.emplace(curRecord.cstate, 15);
      prevRecordExceededInstanceLimit = isInstanceLimitExceeeded;
    }
  }
  void sortPackedObjects();
  MultiDrawRenderer getMultiDrawRenderer();
  void renderObjects(const RendInstGenData::RtData &rtData, const RiGenVisibility &visibility);
  void renderPackedObjects(const RendInstGenData::RtData &rtData, const RiGenVisibility &visibility);
  static dag::Vector<RiShaderConstBuffers> &getPerDrawData(int layer);

  static dag::RelocatableFixedVector<bool, 2> packedDataUpToDateForLayer;
  RenderPass renderPass;
  LayerFlags layerFlags;
  uint32_t startStage;
  uint32_t endStage;
  int perInstDataDwords;
  int drawOrderVarId;
  bool optimizationDepthPrepass;
  bool depthOptimized;
  bool impostorDepthPrepass;
  bool cellDepthPrepass;
  ska::flat_hash_map<shaders::ConstStateIdx, uint16_t, eastl::hash<shaders::ConstStateIdx>, eastl::equal_to<shaders::ConstStateIdx>,
    framemem_allocator>
    bindlessStatesToUpdateTexLevels;
  dag::RelocatableFixedVector<RiGenRenderRecord, 512, true, framemem_allocator> renderRecords;
  dag::RelocatableFixedVector<RiGenRenderRecord, 512, true, framemem_allocator> packedRenderRecords;
  dag::RelocatableFixedVector<PackedRenderRange, 32, true, framemem_allocator> packedRenderRanges;
};

} // namespace rendinst::render
