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

private:
  struct PackedRenderRange
  {
    uint16_t start;
    uint16_t count;
  };

private:
  using MultiDrawRenderer = MultidrawContext<RiGenPerInstanceParameters>::MultidrawRenderExecutor;

private:
  void coalesceRecords();
  void coalescePackedRecords();
  void updatePackedData(int layer);
  void sortPackedObjects();
  MultiDrawRenderer getMultiDrawRenderer();
  void renderObjects(const RendInstGenData::RtData &rtData, const RiGenVisibility &visibility);
  void renderPackedObjects(const RendInstGenData::RtData &rtData, const RiGenVisibility &visibility);
  static dag::Vector<RiShaderConstBuffers> &getPerDrawData(int layer);
  static int riGenDataOffset;
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
  ska::flat_hash_map<uint32_t, uint16_t, eastl::hash<uint32_t>, eastl::equal_to<uint32_t>, framemem_allocator>
    bindlessStatesToUpdateTexLevels;
  dag::RelocatableFixedVector<RiGenRenderRecord, 512, true, framemem_allocator> renderRecords;
  dag::RelocatableFixedVector<RiGenRenderRecord, 512, true, framemem_allocator> packedRenderRecords;
  dag::RelocatableFixedVector<PackedRenderRange, 32, true, framemem_allocator> packedRenderRanges;
};

} // namespace rendinst::render
