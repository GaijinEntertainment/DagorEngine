// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/fixed_string.h>
#include <EASTL/shared_ptr.h>
#include <generic/dag_enumerate.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_lockSbuffer.h>
#include <drv/3d/dag_rwResource.h>
#include <shaders/dag_computeShaders.h>
#include <render/daFrameGraph/daFG.h>
#include <math/dag_hlsl_floatx.h>
#include <frustumCulling/frustumPlanes.h>
#include <render/world/frameGraphHelpers.h>
#include "../../shaders/dagdp_common.hlsli"
#include "../../shaders/dagdp_common_placer.hlsli"
#include "grid3d.h"

using TmpName = eastl::fixed_string<char, 256>;

namespace var
{
static ShaderVariableInfo draw_ranges("dagdp_grid3d__draw_ranges");
static ShaderVariableInfo placeables("dagdp_grid3d__placeables");
static ShaderVariableInfo placeable_weights("dagdp_grid3d__placeable_weights");
static ShaderVariableInfo renderable_indices("dagdp_grid3d__renderable_indices");
static ShaderVariableInfo variants("dagdp_grid3d__variants");
static ShaderVariableInfo tiles("dagdp_grid3d__tiles");
static ShaderVariableInfo volumes("dagdp_grid3d__volumes");

static ShaderVariableInfo num_renderables("dagdp_grid3d__num_renderables");
static ShaderVariableInfo num_placeables("dagdp_grid3d__num_placeables");
static ShaderVariableInfo tiles_start_offset("dagdp_grid3d__tiles_start_offset");

static ShaderVariableInfo max_placeable_bounding_radius("dagdp_grid3d__max_placeable_bounding_radius");
static ShaderVariableInfo tile_pos_delta("dagdp_grid3d__tile_pos_delta");
static ShaderVariableInfo instance_pos_delta("dagdp_grid3d__instance_pos_delta");
static ShaderVariableInfo debug_frustum_culling_bias("dagdp_grid3d__debug_frustum_culling_bias");

static ShaderVariableInfo prng_seed_jitter_x("dagdp_grid3d__prng_seed_jitter_x");
static ShaderVariableInfo prng_seed_jitter_y("dagdp_grid3d__prng_seed_jitter_y");
static ShaderVariableInfo prng_seed_jitter_z("dagdp_grid3d__prng_seed_jitter_z");
static ShaderVariableInfo prng_seed_placeable("dagdp_grid3d__prng_seed_placeable");
static ShaderVariableInfo prng_seed_slope("dagdp_grid3d__prng_seed_slope");
static ShaderVariableInfo prng_seed_occlusion("dagdp_grid3d__prng_seed_occlusion");
static ShaderVariableInfo prng_seed_scale("dagdp_grid3d__prng_seed_scale");
static ShaderVariableInfo prng_seed_yaw("dagdp_grid3d__prng_seed_yaw");
static ShaderVariableInfo prng_seed_pitch("dagdp_grid3d__prng_seed_pitch");
static ShaderVariableInfo prng_seed_roll("dagdp_grid3d__prng_seed_roll");

static ShaderVariableInfo variant_index("dagdp_grid3d__variant_index");
static ShaderVariableInfo grid_jitter("dagdp_grid3d__grid_jitter");
static ShaderVariableInfo dist_based_scale_range("dagdp_grid3d__dist_based_scale_range");
static ShaderVariableInfo dist_based_center("dagdp_grid3d__dist_based_center");

static ShaderVariableInfo viewport_pos("dagdp_grid3d__viewport_pos");
static ShaderVariableInfo viewport_max_distance("dagdp_grid3d__viewport_max_distance");
static ShaderVariableInfo viewport_index("dagdp_grid3d__viewport_index");
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
};

struct Grid3dPersistentData
{
  CommonPlacerBuffers commonBuffers;
  UniqueBuf tileBuffer;
  UniqueBuf volumesBuffer;

  Grid3dConstants constants{};
};

struct Grid3dGatheredInfo
{
  struct PerPlacer
  {
    uint32_t placerIndex;
    uint32_t tileStartIndex;
    uint32_t tileEndIndex;
  };

  struct PerViewport
  {
    uint32_t placerStartIndex;
    uint32_t placerEndIndex;
  };

  dag::RelocatableFixedVector<PerPlacer, 16> perPlacer;
  dag::RelocatableFixedVector<PerViewport, DAGDP_MAX_VIEWPORTS> perViewport;
  bool isValid = false;
};

static dafg::NodeHandle create_place_node(
  const dafg::NameSpace &ns, const eastl::shared_ptr<Grid3dPersistentData> &persistentData, bool is_optimistic)
{
  TmpName name(TmpName::CtorSprintf(), "place_stage%d", is_optimistic ? 0 : 1);

  return ns.registerNode(name.c_str(), DAFG_PP_NODE_SRC, [persistentData, is_optimistic](dafg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);

    (registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str())
      .modify("instance_data")
      .buffer()
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("dagdp__instance_data");

    ComputeShader placeShader = ComputeShader(is_optimistic ? "dagdp_grid3d_place_stage0" : "dagdp_grid3d_place_stage1");

    (registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str())
      .read(is_optimistic ? "dyn_allocs_stage0" : "dyn_allocs_stage1")
      .buffer()
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("dagdp__dyn_allocs");

    (registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str())
      .modify(is_optimistic ? "dyn_counters_stage0" : "dyn_counters_stage1")
      .buffer()
      .atStage(dafg::Stage::COMPUTE)
      .bindToShaderVar("dagdp__dyn_counters");

    registry.modify("grid3d_tiles").buffer().atStage(dafg::Stage::COMPUTE).bindToShaderVar("dagdp_grid3d__tiles");
    registry.modify("grid3d_volumes").buffer().atStage(dafg::Stage::COMPUTE).bindToShaderVar("dagdp_grid3d__volumes");

    eastl::fixed_string<char, 32> viewResourceName;
    viewResourceName.append_sprintf("view@%s", constants.viewInfo.uniqueName.c_str());
    const auto viewHandle = registry.readBlob<ViewPerFrameData>(viewResourceName.c_str()).handle();
    const auto gatheredInfoHandle = registry.readBlob<Grid3dGatheredInfo>("gatheredInfo").handle();

    return [persistentData, viewHandle, gatheredInfoHandle, shader = eastl::move(placeShader)] {
      const auto &constants = persistentData->constants;
      const auto &view = viewHandle.ref();
      G_ASSERT(view.viewports.size() <= constants.viewInfo.maxViewports);
      const auto &gatheredInfo = gatheredInfoHandle.ref();
      if (!gatheredInfo.isValid)
        return;

      ShaderGlobal::set_buffer(var::draw_ranges, persistentData->commonBuffers.drawRangesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::placeables, persistentData->commonBuffers.placeablesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::placeable_weights, persistentData->commonBuffers.placeableWeightsBuffer.getBufId());
      ShaderGlobal::set_buffer(var::renderable_indices, persistentData->commonBuffers.renderableIndicesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::variants, persistentData->commonBuffers.variantsBuffer.getBufId());

      ShaderGlobal::set_int(var::num_renderables, constants.numRenderables);
      ShaderGlobal::set_int(var::num_placeables, constants.numPlaceables);

      ShaderGlobal::set_float(var::max_placeable_bounding_radius, constants.maxPlaceableBoundingRadius);
      ShaderGlobal::set_float(var::debug_frustum_culling_bias, get_frustum_culling_bias());

      // TODO: the way culling variables are bound right now (see frustum.dshl), we need a dispatch per viewport.
      // Otherwise they could be merged into a single dispatch.
      for (uint32_t viewportIndex = 0; viewportIndex < view.viewports.size(); ++viewportIndex)
      {
        const auto &viewport = view.viewports[viewportIndex];
        const auto &perViewport = gatheredInfo.perViewport[viewportIndex];
        ScopeFrustumPlanesShaderVars scopedFrustumVars(viewport.frustum);

        for (uint32_t pp = perViewport.placerStartIndex, ppe = perViewport.placerEndIndex; pp < ppe; pp++)
        {
          const auto &perPlacer = gatheredInfo.perPlacer[pp];
          const auto &placer = constants.placers[perPlacer.placerIndex];

          const float tileWorldSize = placer.worldStep * GRID3D_TILE_SIZE;
          ShaderGlobal::set_float(var::tile_pos_delta, tileWorldSize);
          ShaderGlobal::set_float(var::instance_pos_delta, placer.worldStep);

          ShaderGlobal::set_int(var::prng_seed_jitter_x, placer.prngSeed + 0x4272ECD4u);
          ShaderGlobal::set_int(var::prng_seed_jitter_y, placer.prngSeed + 0x54F2A367u);
          ShaderGlobal::set_int(var::prng_seed_jitter_z, placer.prngSeed + 0x86E5A4D2u);
          ShaderGlobal::set_int(var::prng_seed_placeable, placer.prngSeed + 0x08C2592Cu);
          ShaderGlobal::set_int(var::prng_seed_scale, placer.prngSeed + 0xDF3069FFu);
          ShaderGlobal::set_int(var::prng_seed_slope, placer.prngSeed + 0x3C1385DBu);
          ShaderGlobal::set_int(var::prng_seed_occlusion, placer.prngSeed + 0x93C0FB91u);
          ShaderGlobal::set_int(var::prng_seed_yaw, placer.prngSeed + 0x71F23960u);
          ShaderGlobal::set_int(var::prng_seed_pitch, placer.prngSeed + 0xDEB40CF0u);
          ShaderGlobal::set_int(var::prng_seed_roll, placer.prngSeed + 0xF6A38A81u);

          ShaderGlobal::set_int(var::variant_index, perPlacer.placerIndex);
          ShaderGlobal::set_float4(var::dist_based_scale_range, placer.distBasedScale, placer.distBasedRange, 0);
          ShaderGlobal::set_float4(var::dist_based_center, placer.distBasedCenter);
          ShaderGlobal::set_float(var::grid_jitter, placer.gridJitter);

          ShaderGlobal::set_float4(var::viewport_pos, viewport.worldPos);
          ShaderGlobal::set_float(var::viewport_max_distance,
            min(viewport.maxDrawDistance, constants.viewInfo.maxDrawDistance) * get_global_range_scale());
          ShaderGlobal::set_int(var::viewport_index, viewportIndex);
          ShaderGlobal::set_int(var::tiles_start_offset, perPlacer.tileStartIndex);

          bool res = shader.dispatchGroups(perPlacer.tileEndIndex - perPlacer.tileStartIndex, 1, 1);
          G_ASSERT(res);
          G_UNUSED(res);
        }
      }

      ShaderGlobal::set_buffer(var::draw_ranges, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::placeables, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::placeable_weights, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::renderable_indices, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::variants, BAD_D3DRESID);
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

    return [persistentData, viewHandle, gatheredInfoHandle] {
      const auto &constants = persistentData->constants;
      auto &gatheredInfo = gatheredInfoHandle.ref();
      const auto &view = viewHandle.ref();
      G_ASSERT(view.viewports.size() <= constants.viewInfo.maxViewports);

      Grid3dVolumes volumes;
      Grid3dBoxes boxes;
      Grid3dTiles tiles;

      for (uint32_t viewportIndex = 0; viewportIndex < view.viewports.size(); ++viewportIndex)
      {
        const uint32_t volumeStartIndex = volumes.size();
        const uint32_t placerStartIndex = gatheredInfo.perPlacer.size();
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
                    tiles.emplace_back(Grid3dTile{int3{x, y, z}, packVolumeRange(volumeIndex, volumeIndex + 1u)});
                }
          }

          if (tiles.size() > tileStartIndex)
            gatheredInfo.perPlacer.emplace_back(Grid3dGatheredInfo::PerPlacer{placerIndex, tileStartIndex, tiles.size()});
        }

        gatheredInfo.perViewport.emplace_back(Grid3dGatheredInfo::PerViewport{placerStartIndex, gatheredInfo.perPlacer.size()});
      }

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
