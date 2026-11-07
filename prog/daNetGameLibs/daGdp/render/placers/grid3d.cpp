// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/fixed_string.h>
#include <EASTL/shared_ptr.h>
#include <generic/dag_enumerate.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_lockSbuffer.h>
#include <drv/3d/dag_rwResource.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_refinedBlock.h>
#include <render/daFrameGraph/daFG.h>
#include <math/dag_hlsl_floatx.h>
#include <render/world/frameGraphHelpers.h>
#include "../../shaders/dagdp_common.hlsli"
#include "../../shaders/dagdp_common_placer.hlsli"
#include "../block.h"
#include "../globalManager.h"
#include "../placer.h"
#include "grid3d.h"

namespace var
{
static ShaderVariableInfo draw_ranges("dagdp_grid3d__draw_ranges");
static ShaderVariableInfo placeables("dagdp_grid3d__placeables");
static ShaderVariableInfo placeable_weights("dagdp_grid3d__placeable_weights");
static ShaderVariableInfo renderable_indices("dagdp_grid3d__renderable_indices");
static ShaderVariableInfo variants("dagdp_grid3d__variants");
static ShaderVariableInfo tiles("dagdp_grid3d__tiles");
static ShaderVariableInfo volumes("dagdp_grid3d__volumes");
static ShaderVariableInfo placers_data("dagdp_grid3d__placers");

static ShaderVariableInfo num_renderables("dagdp_grid3d__num_renderables");
static ShaderVariableInfo num_placeables("dagdp_grid3d__num_placeables");

static ShaderVariableInfo max_placeable_bounding_radius("dagdp_grid3d__max_placeable_bounding_radius");
static ShaderVariableInfo debug_frustum_culling_bias("dagdp_grid3d__debug_frustum_culling_bias");
} // namespace var

namespace dagdp
{

struct Grid3dConstants
{
  ViewInfo viewInfo;
  uint32_t maxTiles;
  uint32_t maxVolumes;
  uint32_t numRenderables;
  uint32_t numPlaceables;
  float maxPlaceableBoundingRadius;

  dag::Vector<Grid3dPlacer> placers;
  Grid3dMapping mapping;
  refined_block::PassBlockHandle passBlock;
};

struct Grid3dPersistentData
{
  CommonPlacerBuffers commonBuffers;
  UniqueBuf tileBuffer;
  UniqueBuf volumesBuffer;
  UniqueBuf placerDataBuffer;
  UniqueBuf viewportDataBuffer;

  Grid3dConstants constants{};
};

struct Grid3dGatheredInfo
{
  uint32_t totalTiles = 0;
  bool isValid = false;
};

struct ViewportEnd
{
  uint32_t volumes;
  uint32_t tiles;
};
using ViewportEnds = dag::RelocatableFixedVector<ViewportEnd, DAGDP_MAX_VIEWPORTS>;

static void truncate_gathered_info(
  Grid3dVolumes &volumes, Grid3dTiles &tiles, const ViewportEnds &viewport_ends, const Grid3dConstants &constants)
{
  const uint32_t requiredTiles = tiles.size();
  const uint32_t requiredVolumes = volumes.size();
  if (requiredTiles <= constants.maxTiles && requiredVolumes <= constants.maxVolumes)
    return;

  uint32_t fittingViewportsEnd = 0;
  for (const auto &end : viewport_ends)
  {
    if (end.tiles > constants.maxTiles || end.volumes > constants.maxVolumes)
      break;
    ++fittingViewportsEnd;
  }

  const bool any = fittingViewportsEnd > 0;
  const uint32_t volumeEnd = any ? viewport_ends[fittingViewportsEnd - 1].volumes : 0;
  const uint32_t tileEnd = any ? viewport_ends[fittingViewportsEnd - 1].tiles : 0;

  volumes.resize(volumeEnd);
  tiles.resize(tileEnd);

  RequiredLimits required;
  required.max3dTiles = requiredTiles;
  required.maxVolumes = requiredVolumes;
  GlobalManager::updateRequiredLimits(required);
}

static Grid3dPlacerGpuData make_grid3d_placer_gpu_data(const Grid3dPlacer &placer)
{
  Grid3dPlacerGpuData result{};
  result.prngSeed = static_cast<uint32_t>(placer.prngSeed);
  result.tilePosDelta = placer.worldStep;
  result.gridJitter = placer.gridJitter;
  return result;
}

static Grid3dViewportGpuData make_grid3d_viewport_gpu_data(const Viewport &viewport, const Grid3dConstants &constants)
{
  Grid3dViewportGpuData result{};
  fill_viewport_gpu_data(result, viewport, constants.viewInfo.maxDrawDistance);
  return result;
}

static dafg::NodeHandle create_place_node(
  const dafg::NameSpace &ns, const eastl::shared_ptr<Grid3dPersistentData> &persistentData, bool is_optimistic)
{
  TmpName name(TmpName::CtorSprintf(), "place_stage%d", is_optimistic ? 0 : 1);

  return ns.registerNode(name.c_str(), DAFG_PP_NODE_SRC, [persistentData, is_optimistic](dafg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);
    request_place_stage_buffers(registry, constants.viewInfo, is_optimistic);

    ComputeShader placeShader = ComputeShader(is_optimistic ? "dagdp_grid3d_place_stage0" : "dagdp_grid3d_place_stage1");

    registry.modify("grid3d_tiles").buffer().atStage(dafg::Stage::COMPUTE).bindToShaderVar("dagdp_grid3d__tiles");
    registry.modify("grid3d_volumes").buffer().atStage(dafg::Stage::COMPUTE).bindToShaderVar("dagdp_grid3d__volumes");
    registry.read("grid3d_viewports").buffer().atStage(dafg::Stage::COMPUTE).bindToShaderVar("dagdp_grid3d__viewports");

    const auto gatheredInfoHandle = registry.readBlob<Grid3dGatheredInfo>("gatheredInfo").handle();

    return [persistentData, gatheredInfoHandle, shader = eastl::move(placeShader)] {
      const auto &constants = persistentData->constants;
      const auto &gatheredInfo = gatheredInfoHandle.ref();
      if (!gatheredInfo.isValid)
        return;

      constants.passBlock.setState();

      ShaderGlobal::set_buffer(var::draw_ranges, persistentData->commonBuffers.drawRangesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::placeables, persistentData->commonBuffers.placeablesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::placeable_weights, persistentData->commonBuffers.placeableWeightsBuffer.getBufId());
      ShaderGlobal::set_buffer(var::renderable_indices, persistentData->commonBuffers.renderableIndicesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::variants, persistentData->commonBuffers.variantsBuffer.getBufId());
      ShaderGlobal::set_buffer(var::placers_data, persistentData->placerDataBuffer.getBufId());

      ShaderGlobal::set_int(var::num_renderables, constants.numRenderables);
      ShaderGlobal::set_int(var::num_placeables, constants.numPlaceables);

      ShaderGlobal::set_float(var::max_placeable_bounding_radius, constants.maxPlaceableBoundingRadius);
      ShaderGlobal::set_float(var::debug_frustum_culling_bias, get_frustum_culling_bias());

      if (gatheredInfo.totalTiles > 0)
      {
        const bool res = shader.dispatchGroups(gatheredInfo.totalTiles, 1, 1);
        G_ASSERT(res);
        G_UNUSED(res);
      }

      ShaderGlobal::set_buffer(var::draw_ranges, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::placeables, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::placeable_weights, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::renderable_indices, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::variants, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::placers_data, BAD_D3DRESID);
    };
  });
}

void create_grid3d_nodes(const ViewInfo &view_info,
  const ViewBuilder &view_builder,
  const RulesBuilder &rules_builder,
  const Grid3dManager &grid3d_manager,
  NodeInserter node_inserter)
{
  if (view_builder.totalMaxInstances == 0)
    return; // Nothing to do, early exit.

  FRAMEMEM_REGION;
  auto &builder = grid3d_manager.currentBuilder;

  if (builder.placers.empty())
    return;

  CommonPlacerBufferInit commonBufferInit;

  for (const auto &placer : builder.placers)
    add_variant(commonBufferInit, placer.objectGroups, placer.objectGroups.size(), 0);
  G_ASSERT(commonBufferInit.variantsFmem.size() == builder.placers.size());

  if (commonBufferInit.numPlaceables == 0)
    return;

  TmpName bufferNamePrefix(TmpName::CtorSprintf(), "dagdp_%s_grid3d", view_info.uniqueName.c_str());
  const dafg::NameSpace ns = dafg::root() / "dagdp" / view_info.uniqueName.c_str() / "grid3d";
  auto persistentData = eastl::make_shared<Grid3dPersistentData>();

  Grid3dConstants &constants = persistentData->constants;
  constants.viewInfo = view_info;
  constants.maxTiles = rules_builder.max3dTiles;
  constants.maxVolumes = rules_builder.maxVolumes;
  constants.numPlaceables = commonBufferInit.numPlaceables;
  constants.numRenderables = view_builder.numRenderables;

  constants.maxPlaceableBoundingRadius = 0.0f;
  for (const auto &placer : builder.placers)
    for (const auto &objectGroup : placer.objectGroups)
      constants.maxPlaceableBoundingRadius = max(constants.maxPlaceableBoundingRadius, objectGroup.info->maxPlaceableBoundingRadius);

  constants.placers = eastl::move(builder.placers);
  constants.mapping = eastl::move(builder.mapping);

  bool commonSuccess = init_common_placer_buffers(commonBufferInit, bufferNamePrefix, persistentData->commonBuffers);
  if (!commonSuccess)
    return;

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_volumes", bufferNamePrefix.c_str());
    persistentData->volumesBuffer =
      dag::buffers::create_one_frame_sr_structured(sizeof(VolumeGpuData), constants.maxVolumes, bufferName.c_str());
    debug("[FRAMEMEM] - For volumes '%s': %d", bufferName.c_str(), constants.maxVolumes);
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_tiles", bufferNamePrefix.c_str());
    persistentData->tileBuffer =
      dag::buffers::create_one_frame_sr_structured(sizeof(Grid3dTile), constants.maxTiles, bufferName.c_str());
    debug("[FRAMEMEM] - For tiles '%s': %d", bufferName.c_str(), constants.maxTiles);
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_viewports", bufferNamePrefix.c_str());
    persistentData->viewportDataBuffer = dag::buffers::create_one_frame_cb(
      dag::buffers::cb_array_reg_count<Grid3dViewportGpuData>(DAGDP_MAX_VIEWPORTS), bufferName.c_str());
    debug("[FRAMEMEM] - For viewports '%s': %d", bufferName.c_str(), constants.viewInfo.maxViewports);
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_placers", bufferNamePrefix.c_str());
    dag::RelocatableFixedVector<Grid3dPlacerGpuData, 16, true, framemem_allocator> placersGpuData;
    for (const auto &placer : constants.placers)
      placersGpuData.push_back(make_grid3d_placer_gpu_data(placer));

    persistentData->placerDataBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(Grid3dPlacerGpuData), constants.placers.size(), bufferName.c_str());

    if (!persistentData->placerDataBuffer->updateData(0, data_size(placersGpuData), placersGpuData.data(), VBLOCK_WRITEONLY))
    {
      logerr("daGdp: could not update buffer %s", bufferName.c_str());
      return;
    }
  }

  constants.passBlock =
    get_dagdp_view_block(constants.viewInfo.uniqueName.c_str())
      .refineBlock(TmpName(TmpName::CtorSprintf(), "dagdp_grid3d_pass@%s", constants.viewInfo.uniqueName.c_str()).c_str());

  dafg::NodeHandle gatherNode = ns.registerNode("gather", DAFG_PP_NODE_SRC, [persistentData](dafg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);

    eastl::fixed_string<char, 32> viewResourceName;
    viewResourceName.append_sprintf("view@%s", constants.viewInfo.uniqueName.c_str());
    const auto viewHandle = registry.readBlob<ViewPerFrameData>(viewResourceName.c_str()).handle();
    const auto gatheredInfoHandle = registry.createBlob<Grid3dGatheredInfo>("gatheredInfo").handle();

    registry.registerBuffer("grid3d_tiles", [persistentData](auto) { return ManagedBufView(persistentData->tileBuffer); })
      .atStage(dafg::Stage::TRANSFER);
    registry.registerBuffer("grid3d_volumes", [persistentData](auto) { return ManagedBufView(persistentData->volumesBuffer); })
      .atStage(dafg::Stage::TRANSFER);
    registry.registerBuffer("grid3d_viewports", [persistentData](auto) { return ManagedBufView(persistentData->viewportDataBuffer); })
      .atStage(dafg::Stage::TRANSFER);

    return [persistentData, viewHandle, gatheredInfoHandle] {
      const auto &constants = persistentData->constants;
      auto &gatheredInfo = gatheredInfoHandle.ref();
      const auto &view = viewHandle.ref();
      G_ASSERT(view.viewports.size() <= constants.viewInfo.maxViewports);

      Grid3dVolumes volumes;
      Grid3dBoxes boxes;
      Grid3dTiles tiles;

      ViewportEnds viewportEnds;

      for (uint32_t viewportIndex = 0; viewportIndex < view.viewports.size(); ++viewportIndex)
      {
        const uint32_t volumeStartIndex = volumes.size();
        const auto &viewport = view.viewports[viewportIndex];

        // gather active volumes
        gather(constants.placers, constants.mapping, constants.viewInfo, viewportIndex, viewport, constants.maxPlaceableBoundingRadius,
          volumes, boxes);

        // sort volumes by placer
        stlsort::sort(volumes.begin() + volumeStartIndex, volumes.end(),
          [](const auto &a, const auto &b) { return a.variantIndex < b.variantIndex; });

        // process each placer's volumes
        auto packVolumeRange = [](uint32_t vs, uint32_t ve) -> uint32_t { return vs | ve << 16; };
        auto unpackVolumeRange = [](uint32_t r) -> eastl::pair<uint32_t, uint32_t> { return {r & 0xffff, r >> 16}; };

        for (uint32_t volumeIndex = volumeStartIndex, volumeEndIndex = volumes.size(); volumeIndex < volumeEndIndex;)
        {
          const uint32_t placerIndex = volumes[volumeIndex].variantIndex;
          const auto &placer = constants.placers[placerIndex];
          const uint32_t tileStartIndex = tiles.size();

          auto findTile = [&tiles, tileStartIndex](int x, int y, int z) -> int {
            for (int i = tileStartIndex, end = tiles.size(); i < end; i++)
              if (tiles[i].pos.x == x and tiles[i].pos.y == y and tiles[i].pos.z == z)
                return i;
            return -1;
          };

          // gather affected tiles
          for (; volumeIndex < volumeEndIndex and volumes[volumeIndex].variantIndex == placerIndex; volumeIndex++)
          {
            const auto &box = boxes[volumeIndex];
            const vec3f step = v_splats(placer.worldStep * GRID3D_TILE_SIZE);
            vec4i bmin = v_cvt_floori(v_div(box.bmin, step));
            vec4i bmax = v_cvt_floori(v_div(box.bmax, step));
            IPoint4 imin, imax;
            v_stui(&imin.x, bmin);
            v_stui(&imax.x, bmax);
            for (int z = imin.z; z <= imax.z; z++)
              for (int y = imin.y; y <= imax.y; y++)
                for (int x = imin.x; x <= imax.x; x++)
                {
                  //== TODO: cull tile by volume?
                  int ti = findTile(x, y, z);
                  if (ti >= 0)
                  {
                    auto [vs, ve] = unpackVolumeRange(tiles[ti].volumeRange);
                    vs = min(vs, volumeIndex);
                    ve = max(ve, volumeIndex + 1u);
                    tiles[ti].volumeRange = packVolumeRange(vs, ve);
                  }
                  else
                    tiles.emplace_back(
                      Grid3dTile{int3{x, y, z}, packVolumeRange(volumeIndex, volumeIndex + 1u), placerIndex | (viewportIndex << 16)});
                }
          }
        }

        viewportEnds.emplace_back(ViewportEnd{volumes.size(), tiles.size()});
      }

      truncate_gathered_info(volumes, tiles, viewportEnds, constants);
      gatheredInfo.totalTiles = tiles.size();

      dag::RelocatableFixedVector<Grid3dViewportGpuData, DAGDP_MAX_VIEWPORTS, true, framemem_allocator> viewportGpuData;
      for (const auto &viewport : view.viewports)
        viewportGpuData.push_back(make_grid3d_viewport_gpu_data(viewport, constants));

      if (!update_frame_mem(make_span_const(viewportGpuData), persistentData->viewportDataBuffer.getBuf(),
            constants.viewInfo.maxViewports, "3d viewports"))
        return;

      if (!update_frame_mem(make_span_const(tiles), persistentData->tileBuffer.getBuf(), constants.maxTiles, "3d tiles"))
        return;

      if (!update_frame_mem(make_span_const(volumes), persistentData->volumesBuffer.getBuf(), constants.maxVolumes, "3d volumes"))
        return;

      gatheredInfo.isValid = true;
    };
  });

  node_inserter(eastl::move(gatherNode));

  node_inserter(create_place_node(ns, persistentData, true));
  node_inserter(create_place_node(ns, persistentData, false));
}

} // namespace dagdp
