// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/fixed_string.h>
#include <EASTL/shared_ptr.h>
#include <generic/dag_enumerate.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_lockSbuffer.h>
#include <drv/3d/dag_rwResource.h>
#include <shaders/dag_computeShaders.h>
#include <render/daBfg/bfg.h>
#include <math/dag_hlsl_floatx.h>
#include <frustumCulling/frustumPlanes.h>
#include <render/world/frameGraphHelpers.h>
#include "../../shaders/dagdp_common.hlsli"
#include "../../shaders/dagdp_common_placer.hlsli"
#include "../../shaders/dagdp_heightmap.hlsli"
#include "heightmap.h"

namespace var
{
static ShaderVariableInfo draw_ranges("dagdp_heightmap__draw_ranges");
static ShaderVariableInfo placeables("dagdp_heightmap__placeables");
static ShaderVariableInfo placeable_weights("dagdp_heightmap__placeable_weights");
static ShaderVariableInfo placeable_tile_limits("dagdp_heightmap__placeable_tile_limits");
static ShaderVariableInfo renderable_indices("dagdp_heightmap__renderable_indices");
static ShaderVariableInfo instance_regions("dagdp_heightmap__instance_regions");
static ShaderVariableInfo tile_positions("dagdp_heightmap__tile_positions");
static ShaderVariableInfo biomes("dagdp_heightmap__biomes");
static ShaderVariableInfo variants("dagdp_heightmap__variants");
static ShaderVariableInfo indirect_args("dagdp_heightmap__indirect_args");

static ShaderVariableInfo density_mask("dagdp_heightmap__density_mask");
static ShaderVariableInfo density_mask_scale_offset("dagdp_heightmap__density_mask_scale_offset");

static ShaderVariableInfo num_renderables("dagdp_heightmap__num_renderables");
static ShaderVariableInfo num_placeables("dagdp_heightmap__num_placeables");
static ShaderVariableInfo num_biomes("dagdp_heightmap__num_biomes");
static ShaderVariableInfo num_tiles("dagdp_heightmap__num_tiles");

static ShaderVariableInfo max_placeable_bounding_radius("dagdp_heightmap__max_placeable_bounding_radius");
static ShaderVariableInfo tile_pos_delta("dagdp_heightmap__tile_pos_delta");
static ShaderVariableInfo instance_pos_delta("dagdp_heightmap__instance_pos_delta");
static ShaderVariableInfo debug_frustum_culling_bias("dagdp_heightmap__debug_frustum_culling_bias");

static ShaderVariableInfo prng_seed_jitter_x("dagdp_heightmap__prng_seed_jitter_x");
static ShaderVariableInfo prng_seed_jitter_z("dagdp_heightmap__prng_seed_jitter_z");
static ShaderVariableInfo prng_seed_placeable("dagdp_heightmap__prng_seed_placeable");
static ShaderVariableInfo prng_seed_slope("dagdp_heightmap__prng_seed_slope");
static ShaderVariableInfo prng_seed_scale("dagdp_heightmap__prng_seed_scale");
static ShaderVariableInfo prng_seed_yaw("dagdp_heightmap__prng_seed_yaw");
static ShaderVariableInfo prng_seed_pitch("dagdp_heightmap__prng_seed_pitch");
static ShaderVariableInfo prng_seed_roll("dagdp_heightmap__prng_seed_roll");
static ShaderVariableInfo prng_seed_density("dagdp_heightmap__prng_seed_density");
static ShaderVariableInfo grid_jitter("dagdp_heightmap__grid_jitter");
static ShaderVariableInfo lower_level("dagdp_heightmap__lower_level");

static ShaderVariableInfo base_tile_pos_xz("dagdp_heightmap__base_tile_pos_xz");
static ShaderVariableInfo base_tile_int_pos_xz("dagdp_heightmap__base_tile_int_pos_xz");
static ShaderVariableInfo viewport_pos("dagdp_heightmap__viewport_pos");
static ShaderVariableInfo viewport_max_distance("dagdp_heightmap__viewport_max_distance");
static ShaderVariableInfo viewport_index("dagdp_heightmap__viewport_index");
static ShaderVariableInfo viewport_instance_offset("dagdp_heightmap__viewport_instance_offset");
} // namespace var

using TmpName = eastl::fixed_string<char, 256>;

namespace dagdp
{

struct Biome
{
  uint32_t variantIndex;
};

struct HeightmapConstants
{
  ViewInfo viewInfo;
  uint32_t maxStaticInstancesPerViewport;
  uint32_t numTiles;
  uint32_t numRenderables;
  uint32_t numPlaceables;
  uint32_t numBiomes;
  float tileWorldSize;
  float maxPlaceableBoundingRadius;
  uint32_t prngSeed;
  float gridJitter;
  bool lowerLevel;
};

struct HeightmapPersistentData
{
  CommonPlacerBuffers commonBuffers;

  UniqueBuf placeableTileLimitsBuffer;
  UniqueBuf instanceRegionsBuffer;
  UniqueBuf tilePositionsBuffer;
  UniqueBuf biomesBuffer;
  SharedTex densityMask;
  Point4 densityMaskScaleOffset;

  HeightmapConstants constants{};
};

static dabfg::NodeHandle create_indirect_args_node(const dabfg::NameSpace &ns,
  const eastl::shared_ptr<HeightmapPersistentData> &persistentData)
{
  return ns.registerNode("indirect_args", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);

    const auto indirectArgsHandle = registry.create("indirect_args", dabfg::History::No)
                                      .indirectBufferUa(d3d::buffers::Indirect::Dispatch, DAGDP_MAX_VIEWPORTS)
                                      .atStage(dabfg::Stage::UNKNOWN) // TODO: d3d::clear_rwbufi semantics is not defined.
                                      .useAs(dabfg::Usage::UNKNOWN)
                                      .handle();

    return [indirectArgsHandle] {
      const uint32_t val[4] = {0, 0, 0, 0};
      d3d::clear_rwbufi(indirectArgsHandle.get(), val);
    };
  });
}
static dabfg::NodeHandle create_cull_tiles_node(const dabfg::NameSpace &ns,
  const eastl::shared_ptr<HeightmapPersistentData> &persistentData)
{
  return ns.registerNode("cull_tiles", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);

    const eastl::fixed_string<char, 32> viewResourceName({}, "view@%s", constants.viewInfo.uniqueName.c_str());
    const auto viewHandle = registry.readBlob<ViewPerFrameData>(viewResourceName.c_str()).handle();

    registry.modify("indirect_args").buffer().atStage(dabfg::Stage::COMPUTE).bindToShaderVar("dagdp_heightmap__indirect_args");

    registry.create("visible_tile_positions", dabfg::History::No)
      .byteAddressBufferUaSr(constants.numTiles * 2) // int2
      .atStage(dabfg::Stage::COMPUTE)
      .bindToShaderVar("dagdp_heightmap__visible_tile_positions");

    return [persistentData, viewHandle, shader = ComputeShader("dagdp_heightmap_cull_tiles")] {
      const auto &constants = persistentData->constants;
      const auto &view = viewHandle.ref();

      ShaderGlobal::set_int(var::num_tiles, constants.numTiles);
      ShaderGlobal::set_real(var::tile_pos_delta, constants.tileWorldSize);
      ShaderGlobal::set_real(var::max_placeable_bounding_radius, constants.maxPlaceableBoundingRadius);

      ShaderGlobal::set_buffer(var::tile_positions, persistentData->tilePositionsBuffer.getBufId());

      for (uint32_t viewportIndex = 0; viewportIndex < view.viewports.size(); ++viewportIndex)
      {
        const auto &viewport = view.viewports[viewportIndex];
        ScopeFrustumPlanesShaderVars scopedFrustumVars(viewport.frustum);

        IPoint4 baseTileIntPos;
        baseTileIntPos.x = static_cast<int>(floorf(viewport.worldPos.x / constants.tileWorldSize));
        baseTileIntPos.y = static_cast<int>(floorf(viewport.worldPos.z / constants.tileWorldSize));

        ShaderGlobal::set_color4(var::base_tile_pos_xz, point4(baseTileIntPos) * constants.tileWorldSize);
        ShaderGlobal::set_color4(var::viewport_pos, viewport.worldPos);
        ShaderGlobal::set_int(var::viewport_index, viewportIndex);
        ShaderGlobal::set_real(var::viewport_max_distance, min(viewport.maxDrawDistance, constants.viewInfo.maxDrawDistance));

        const bool res = shader.dispatchThreads(constants.numTiles, 1, 1);
        G_ASSERT(res);
        G_UNUSED(res);
      }

      ShaderGlobal::set_buffer(var::tile_positions, BAD_D3DRESID);
    };
  });
}

static dabfg::NodeHandle create_place_node(
  const dabfg::NameSpace &ns, const eastl::shared_ptr<HeightmapPersistentData> &persistentData, HeightmapPlacementType type)
{
  eastl::fixed_string<char, 32> name("place");
  if (type != HeightmapPlacementType::STATIC)
    name.append_sprintf("_stage%d", eastl::to_underlying(type) - 1);
  G_ASSERTF_RETURN(type == HeightmapPlacementType::STATIC || type == HeightmapPlacementType::DYNAMIC_OPTIMISTIC ||
                     type == HeightmapPlacementType::DYNAMIC_PESSIMISTIC,
    {}, "Unknown placement type %d", eastl::to_underlying(type));

  return ns.registerNode(name.c_str(), DABFG_PP_NODE_SRC, [persistentData, type](dabfg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);

    (registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str())
      .modify("instance_data")
      .buffer()
      .atStage(dabfg::Stage::COMPUTE)
      .bindToShaderVar("dagdp__instance_data");

    ComputeShader placeShader;
    if (type == HeightmapPlacementType::STATIC)
    {
      placeShader = ComputeShader("dagdp_heightmap_place");
      (registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str())
        .modify("counters")
        .buffer()
        .atStage(dabfg::Stage::COMPUTE)
        .bindToShaderVar("dagdp__counters");
    }
    else
    {
      const bool is_optimistic = type == HeightmapPlacementType::DYNAMIC_OPTIMISTIC;
      placeShader = ComputeShader(is_optimistic ? "dagdp_heightmap_place_stage0" : "dagdp_heightmap_place_stage1");
      (registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str())
        .read(is_optimistic ? "dyn_allocs_stage0" : "dyn_allocs_stage1")
        .buffer()
        .atStage(dabfg::Stage::COMPUTE)
        .bindToShaderVar("dagdp__dyn_allocs");

      (registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str())
        .modify(is_optimistic ? "dyn_counters_stage0" : "dyn_counters_stage1")
        .buffer()
        .atStage(dabfg::Stage::COMPUTE)
        .bindToShaderVar("dagdp__dyn_counters");
    }

    eastl::fixed_string<char, 32> viewResourceName;
    viewResourceName.append_sprintf("view@%s", constants.viewInfo.uniqueName.c_str());
    const auto viewHandle = registry.readBlob<ViewPerFrameData>(viewResourceName.c_str()).handle();

    const auto indirectArgsHandle =
      registry.read("indirect_args").buffer().atStage(dabfg::Stage::COMPUTE).useAs(dabfg::Usage::INDIRECTION_BUFFER).handle();

    registry.read("visible_tile_positions")
      .buffer()
      .atStage(dabfg::Stage::COMPUTE)
      .bindToShaderVar("dagdp_heightmap__visible_tile_positions");

    return [persistentData, viewHandle, indirectArgsHandle, shader = eastl::move(placeShader)] {
      const auto &constants = persistentData->constants;
      const auto &view = viewHandle.ref();
      G_ASSERT(view.viewports.size() <= constants.viewInfo.maxViewports);

      ShaderGlobal::set_buffer(var::draw_ranges, persistentData->commonBuffers.drawRangesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::placeables, persistentData->commonBuffers.placeablesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::placeable_weights, persistentData->commonBuffers.placeableWeightsBuffer.getBufId());
      ShaderGlobal::set_buffer(var::placeable_tile_limits, persistentData->placeableTileLimitsBuffer.getBufId());
      ShaderGlobal::set_buffer(var::renderable_indices, persistentData->commonBuffers.renderableIndicesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::instance_regions, persistentData->instanceRegionsBuffer.getBufId());
      ShaderGlobal::set_buffer(var::biomes, persistentData->biomesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::variants, persistentData->commonBuffers.variantsBuffer.getBufId());

      ShaderGlobal::set_texture(var::density_mask, persistentData->densityMask);
      ShaderGlobal::set_color4(var::density_mask_scale_offset, persistentData->densityMaskScaleOffset);

      ShaderGlobal::set_int(var::num_renderables, constants.numRenderables);
      ShaderGlobal::set_int(var::num_placeables, constants.numPlaceables);
      ShaderGlobal::set_int(var::num_biomes, constants.numBiomes);
      ShaderGlobal::set_int(var::num_tiles, constants.numTiles);

      ShaderGlobal::set_real(var::max_placeable_bounding_radius, constants.maxPlaceableBoundingRadius);
      ShaderGlobal::set_real(var::tile_pos_delta, constants.tileWorldSize);
      ShaderGlobal::set_real(var::instance_pos_delta, constants.tileWorldSize / TILE_INSTANCE_COUNT_1D);
      ShaderGlobal::set_real(var::debug_frustum_culling_bias, get_frustum_culling_bias());

      ShaderGlobal::set_int(var::prng_seed_jitter_x, constants.prngSeed + 0x4272ECD4u);
      ShaderGlobal::set_int(var::prng_seed_jitter_z, constants.prngSeed + 0x86E5A4D2u);
      ShaderGlobal::set_int(var::prng_seed_placeable, constants.prngSeed + 0x08C2592Cu);
      ShaderGlobal::set_int(var::prng_seed_scale, constants.prngSeed + 0xDF3069FFu);
      ShaderGlobal::set_int(var::prng_seed_slope, constants.prngSeed + 0x3C1385DBu);
      ShaderGlobal::set_int(var::prng_seed_yaw, constants.prngSeed + 0x71F23960u);
      ShaderGlobal::set_int(var::prng_seed_pitch, constants.prngSeed + 0xDEB40CF0u);
      ShaderGlobal::set_int(var::prng_seed_roll, constants.prngSeed + 0xF6A38A81u);
      ShaderGlobal::set_real(var::grid_jitter, constants.gridJitter);
      ShaderGlobal::set_int(var::lower_level, constants.lowerLevel);

      // TODO: the way culling variables are bound right now (see frustum.dshl), we need a dispatch per viewport.
      // Otherwise they could be merged into a single dispatch.
      for (uint32_t viewportIndex = 0; viewportIndex < view.viewports.size(); ++viewportIndex)
      {
        const auto &viewport = view.viewports[viewportIndex];
        ScopeFrustumPlanesShaderVars scopedFrustumVars(viewport.frustum);

        IPoint4 baseTileIntPos;
        baseTileIntPos.x = static_cast<int>(floorf(viewport.worldPos.x / constants.tileWorldSize));
        baseTileIntPos.y = static_cast<int>(floorf(viewport.worldPos.z / constants.tileWorldSize));

        ShaderGlobal::set_color4(var::base_tile_pos_xz, point4(baseTileIntPos) * constants.tileWorldSize);
        ShaderGlobal::set_int4(var::base_tile_int_pos_xz, baseTileIntPos);
        ShaderGlobal::set_color4(var::viewport_pos, viewport.worldPos);
        ShaderGlobal::set_real(var::viewport_max_distance, min(viewport.maxDrawDistance, constants.viewInfo.maxDrawDistance));
        ShaderGlobal::set_int(var::viewport_index, viewportIndex);
        ShaderGlobal::set_int(var::viewport_instance_offset, constants.maxStaticInstancesPerViewport * viewportIndex);

        bool res = shader.dispatchIndirect(indirectArgsHandle.view().getBuf(), viewportIndex * DISPATCH_INDIRECT_BUFFER_SIZE);
        G_ASSERT(res);
        G_UNUSED(res);
      }

      ShaderGlobal::set_buffer(var::draw_ranges, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::placeables, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::placeable_weights, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::placeable_tile_limits, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::renderable_indices, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::instance_regions, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::tile_positions, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::biomes, BAD_D3DRESID);
      ShaderGlobal::set_buffer(var::variants, BAD_D3DRESID);
    };
  });
}

static void create_grid_nodes(const ViewInfo &view_info,
  const ViewBuilder &view_builder,
  NodeInserter node_inserter,
  const HeightmapGrid &grid,
  size_t grid_index,
  const SharedTex &density_mask,
  const Point4 &mask_scale_offset)
{
  FRAMEMEM_REGION;

  if (grid.tiles.empty())
    return;

  dag::RelocatableFixedVector<Biome, 128, true, framemem_allocator> perBiomeFmem;
  CommonPlacerBufferInit commonBufferInit;

  for (const auto &variant : grid.variants)
    add_variant(commonBufferInit, variant.objectGroups, variant.effectiveDensity, 1.0f - variant.effectiveDensity / grid.density);

  if (commonBufferInit.numPlaceables == 0)
    return;

  uint32_t maxBiomeIndex = 0;
  for (const auto &variant : grid.variants)
    for (const auto ix : variant.biomes)
      maxBiomeIndex = max(maxBiomeIndex, static_cast<uint32_t>(ix));

  perBiomeFmem.assign(maxBiomeIndex + 1, Biome{~0u});

  for (const auto [variantIndex, variant] : enumerate(grid.variants))
    for (const auto biomeIndex : variant.biomes)
      perBiomeFmem[biomeIndex].variantIndex = variantIndex;

  TmpName nameSpaceName(TmpName::CtorSprintf(), "heightmap_%zu", grid_index);
  TmpName bufferNamePrefix(TmpName::CtorSprintf(), "dagdp_%s_heightmap_%zu", view_info.uniqueName.c_str(), grid_index);
  const dabfg::NameSpace ns = dabfg::root() / "dagdp" / view_info.uniqueName.c_str() / nameSpaceName.c_str();
  auto persistentData = eastl::make_shared<HeightmapPersistentData>();
  persistentData->densityMask = density_mask;
  persistentData->densityMaskScaleOffset = mask_scale_offset;

  HeightmapConstants &constants = persistentData->constants;
  constants.viewInfo = view_info;
  constants.maxStaticInstancesPerViewport = view_builder.maxStaticInstancesPerViewport;
  constants.numTiles = grid.tiles.size();
  constants.numPlaceables = commonBufferInit.numPlaceables;
  constants.numBiomes = perBiomeFmem.size();
  constants.numRenderables = view_builder.numRenderables;
  constants.tileWorldSize = grid.tileWorldSize;

  constants.maxPlaceableBoundingRadius = 0.0f;
  for (const auto &variant : grid.variants)
    for (const auto &objectGroup : variant.objectGroups)
      constants.maxPlaceableBoundingRadius = max(constants.maxPlaceableBoundingRadius, objectGroup.info->maxPlaceableBoundingRadius);

  constants.prngSeed = static_cast<uint32_t>(grid.prngSeed);
  constants.gridJitter = grid.gridJitter;
  constants.lowerLevel = grid.lowerLevel;

  bool commonSuccess = init_common_placer_buffers(commonBufferInit, bufferNamePrefix, persistentData->commonBuffers);
  if (!commonSuccess)
    return;

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_placeable_tile_limits", bufferNamePrefix.c_str());
    persistentData->placeableTileLimitsBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(uint32_t), grid.placeablesTileLimits.size(), bufferName.c_str());

    bool updated = persistentData->placeableTileLimitsBuffer->updateData(0, data_size(grid.placeablesTileLimits),
      grid.placeablesTileLimits.data(), VBLOCK_WRITEONLY);

    if (!updated)
    {
      logerr("daGdp: could not update buffer %s", bufferName.c_str());
      return;
    }
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_instance_regions", bufferNamePrefix.c_str());
    persistentData->instanceRegionsBuffer = dag::buffers::create_persistent_sr_structured(sizeof(InstanceRegion),
      view_builder.renderablesInstanceRegions.size(), bufferName.c_str());

    bool updated = persistentData->instanceRegionsBuffer->updateData(0, data_size(view_builder.renderablesInstanceRegions),
      view_builder.renderablesInstanceRegions.data(), VBLOCK_WRITEONLY);

    if (!updated)
    {
      logerr("daGdp: could not update buffer %s", bufferName.c_str());
      return;
    }
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_tile_positions", bufferNamePrefix.c_str());
    persistentData->tilePositionsBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(HeightmapTileCoord), grid.tiles.size(), bufferName.c_str());

    bool updated = persistentData->tilePositionsBuffer->updateData(0, data_size(grid.tiles), grid.tiles.data(), VBLOCK_WRITEONLY);

    if (!updated)
    {
      logerr("daGdp: could not update buffer %s", bufferName.c_str());
      return;
    }
  }

  {
    static_assert(sizeof(perBiomeFmem[0]) == sizeof(uint32_t));

    TmpName bufferName(TmpName::CtorSprintf(), "%s_biomes", bufferNamePrefix.c_str());
    persistentData->biomesBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(uint32_t), constants.numBiomes, bufferName.c_str());

    bool updated = persistentData->biomesBuffer->updateData(0, data_size(perBiomeFmem), perBiomeFmem.data(), VBLOCK_WRITEONLY);

    if (!updated)
    {
      logerr("daGdp: could not update buffer %s", bufferName.c_str());
      return;
    }
  }

  node_inserter(create_indirect_args_node(ns, persistentData));
  node_inserter(create_cull_tiles_node(ns, persistentData));

  if (grid.useDynamicAllocation)
  {
    node_inserter(create_place_node(ns, persistentData, HeightmapPlacementType::DYNAMIC_OPTIMISTIC));
    node_inserter(create_place_node(ns, persistentData, HeightmapPlacementType::DYNAMIC_PESSIMISTIC));
  }
  else
  {
    node_inserter(create_place_node(ns, persistentData, HeightmapPlacementType::STATIC));
  }
}

void create_heightmap_nodes(
  const ViewInfo &view_info, const ViewBuilder &view_builder, const HeightmapManager &heightmap_manager, NodeInserter node_inserter)
{
  if (view_builder.totalMaxInstances == 0)
    return; // Nothing to do, early exit.

  // TODO: merge these separate grid nodes into a single node/dispatch.
  // https://youtrack.gaijin.team/issue/RE-790/daGDP-merge-dispatches-of-heightmap-placers
  for (const auto [i, grid] : enumerate(heightmap_manager.currentBuilder.grids))
    create_grid_nodes(view_info, view_builder, node_inserter, grid, i, heightmap_manager.currentBuilder.densityMask,
      heightmap_manager.currentBuilder.maskScaleOffset);
}

} // namespace dagdp