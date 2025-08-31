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
#include "../../shaders/dagdp_heightmap.hlsli"
#include "grass.h"

namespace var
{
static ShaderVariableInfo draw_ranges("dagdp_heightmap__draw_ranges");
static ShaderVariableInfo placeables("dagdp_heightmap__placeables");
static ShaderVariableInfo placeable_weights("dagdp_heightmap__placeable_weights");
static ShaderVariableInfo renderable_indices("dagdp_heightmap__renderable_indices");
static ShaderVariableInfo tile_positions("dagdp_heightmap__tile_positions");
static ShaderVariableInfo biomes("dagdp_heightmap__biomes");
static ShaderVariableInfo variants("dagdp_heightmap__variants");
static ShaderVariableInfo indirect_args("dagdp_heightmap__indirect_args");

static ShaderVariableInfo num_renderables("dagdp_heightmap__num_renderables");
static ShaderVariableInfo num_placeables("dagdp_heightmap__num_placeables");
static ShaderVariableInfo num_biomes("dagdp_heightmap__num_biomes");
static ShaderVariableInfo num_tiles("dagdp_heightmap__num_tiles");

static ShaderVariableInfo max_placeable_bounding_radius("dagdp_heightmap__max_placeable_bounding_radius");
static ShaderVariableInfo tile_pos_delta("dagdp_heightmap__tile_pos_delta");
static ShaderVariableInfo instance_pos_delta("dagdp_heightmap__instance_pos_delta");
static ShaderVariableInfo debug_frustum_culling_bias("dagdp_heightmap__debug_frustum_culling_bias");
static ShaderVariableInfo grass_max_range("dagdp_heightmap__grass_max_range");

static ShaderVariableInfo prng_seed_jitter_x("dagdp_heightmap__prng_seed_jitter_x");
static ShaderVariableInfo prng_seed_jitter_z("dagdp_heightmap__prng_seed_jitter_z");
static ShaderVariableInfo prng_seed_placeable("dagdp_heightmap__prng_seed_placeable");
static ShaderVariableInfo prng_seed_slope("dagdp_heightmap__prng_seed_slope");
static ShaderVariableInfo prng_seed_occlusion("dagdp_heightmap__prng_seed_occlusion");
static ShaderVariableInfo prng_seed_scale("dagdp_heightmap__prng_seed_scale");
static ShaderVariableInfo prng_seed_yaw("dagdp_heightmap__prng_seed_yaw");
static ShaderVariableInfo prng_seed_pitch("dagdp_heightmap__prng_seed_pitch");
static ShaderVariableInfo prng_seed_roll("dagdp_heightmap__prng_seed_roll");
static ShaderVariableInfo prng_seed_density("dagdp_heightmap__prng_seed_density");
static ShaderVariableInfo prng_seed_decal("dagdp_heightmap__prng_seed_decal");
static ShaderVariableInfo prng_seed_height("dagdp_grass__prng_seed_height");

static ShaderVariableInfo base_tile_pos_xz("dagdp_heightmap__base_tile_pos_xz");
static ShaderVariableInfo base_tile_int_pos_xz("dagdp_heightmap__base_tile_int_pos_xz");
static ShaderVariableInfo viewport_pos("dagdp_heightmap__viewport_pos");
static ShaderVariableInfo viewport_max_distance("dagdp_heightmap__viewport_max_distance");
static ShaderVariableInfo viewport_index("dagdp_heightmap__viewport_index");
} // namespace var

namespace dagdp
{

struct Biome
{
  uint32_t variantIndex;
  float height, heightVariance;
};

struct GrassConstants
{
  ViewInfo viewInfo;
  uint32_t numTiles;
  uint32_t numRenderables;
  uint32_t numPlaceables;
  uint32_t numBiomes;
  float tileWorldSize;
  float maxPlaceableBoundingRadius;
  float grassMaxRange;
  uint32_t prngSeed;
};

struct GrassPersistentData
{
  CommonPlacerBuffers commonBuffers;

  UniqueBuf placeableTileLimitsBuffer;
  UniqueBuf tilePositionsBuffer;
  UniqueBuf biomesBuffer;

  GrassConstants constants{};
};

#define IS_GRASS 1
#include "placer_common.h"

static void create_grid_nodes(const ViewInfo &view_info,
  const ViewBuilder &view_builder,
  NodeInserter node_inserter,
  const GrassGrid &grid,
  size_t grid_index,
  float grass_max_range)
{
  FRAMEMEM_REGION;

  if (grid.tiles.empty())
    return;

  dag::RelocatableFixedVector<Biome, 64, true, framemem_allocator> perBiomeFmem;
  CommonPlacerBufferInit commonBufferInit;

  for (const auto &variant : grid.variants)
    add_variant(commonBufferInit, variant.objectGroups, variant.objectGroups.size(), 0);

  if (commonBufferInit.numPlaceables == 0)
    return;

  uint32_t maxBiomeIndex = 0;
  for (const auto &variant : grid.variants)
    maxBiomeIndex = max(maxBiomeIndex, static_cast<uint32_t>(variant.biome));

  perBiomeFmem.assign(maxBiomeIndex + 1, Biome{~0u, 1.0f, 0.0f});

  for (const auto [i, variant] : enumerate(grid.variants))
  {
    auto &biome = perBiomeFmem[variant.biome];
    biome.variantIndex = i;
    biome.height = variant.height;
    biome.heightVariance = variant.heightVariance;
  }

  TmpName nameSpaceName(TmpName::CtorSprintf(), "grass_%zu", grid_index);
  TmpName bufferNamePrefix(TmpName::CtorSprintf(), "dagdp_%s_grass_%zu", view_info.uniqueName.c_str(), grid_index);
  const dafg::NameSpace ns = dafg::root() / "dagdp" / view_info.uniqueName.c_str() / nameSpaceName.c_str();
  auto persistentData = eastl::make_shared<GrassPersistentData>();

  GrassConstants &constants = persistentData->constants;
  constants.viewInfo = view_info;
  constants.numTiles = grid.tiles.size();
  constants.numPlaceables = commonBufferInit.numPlaceables;
  constants.numBiomes = perBiomeFmem.size();
  constants.numRenderables = view_builder.numRenderables;
  constants.tileWorldSize = grid.tileWorldSize;
  constants.grassMaxRange = grass_max_range;
  constants.prngSeed = 0u;

  constants.maxPlaceableBoundingRadius = 0.0f;
  for (const auto &variant : grid.variants)
    for (const auto &objectGroup : variant.objectGroups)
      constants.maxPlaceableBoundingRadius = max(constants.maxPlaceableBoundingRadius, objectGroup.info->maxPlaceableBoundingRadius);

  bool commonSuccess = init_common_placer_buffers(commonBufferInit, bufferNamePrefix, persistentData->commonBuffers);
  if (!commonSuccess)
    return;

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_tile_positions", bufferNamePrefix.c_str());
    persistentData->tilePositionsBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(GrassTileCoord), grid.tiles.size(), bufferName.c_str());

    bool updated = persistentData->tilePositionsBuffer->updateData(0, data_size(grid.tiles), grid.tiles.data(), VBLOCK_WRITEONLY);

    if (!updated)
    {
      logerr("daGdp: could not update buffer %s", bufferName.c_str());
      return;
    }
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_biomes", bufferNamePrefix.c_str());
    persistentData->biomesBuffer = dag::buffers::create_persistent_sr_structured(sizeof(uint32_t),
      constants.numBiomes * (sizeof(perBiomeFmem[0]) / sizeof(uint32_t)), bufferName.c_str());

    bool updated = persistentData->biomesBuffer->updateData(0, data_size(perBiomeFmem), perBiomeFmem.data(), VBLOCK_WRITEONLY);

    if (!updated)
    {
      logerr("daGdp: could not update buffer %s", bufferName.c_str());
      return;
    }
  }

  node_inserter(create_indirect_args_node(ns, persistentData));
  node_inserter(create_cull_tiles_node(ns, persistentData));

  node_inserter(create_place_node(ns, persistentData, true));
  node_inserter(create_place_node(ns, persistentData, false));
}

void create_grass_nodes(
  const ViewInfo &view_info, const ViewBuilder &view_builder, const GrassManager &grass_manager, NodeInserter node_inserter)
{
  if (view_builder.totalMaxInstances == 0)
    return; // Nothing to do, early exit.

  // TODO: merge these separate grid nodes into a single node/dispatch.
  for (const auto [i, grid] : enumerate(grass_manager.currentBuilder.grids))
    create_grid_nodes(view_info, view_builder, node_inserter, grid, i, grass_manager.currentBuilder.grassMaxRange);
}

} // namespace dagdp