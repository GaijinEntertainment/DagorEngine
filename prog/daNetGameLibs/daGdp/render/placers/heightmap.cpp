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
#include "heightmap.h"

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

static ShaderVariableInfo density_mask("dagdp_heightmap__density_mask");
static ShaderVariableInfo density_mask_sampler("dagdp_heightmap__density_mask_samplerstate");
static ShaderVariableInfo density_mask_scale_offset("dagdp_heightmap__density_mask_scale_offset");
static ShaderVariableInfo density_mask_channel_weights("dagdp_heightmap__density_mask_channel_weights");

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
static ShaderVariableInfo prng_seed_occlusion("dagdp_heightmap__prng_seed_occlusion");
static ShaderVariableInfo prng_seed_scale("dagdp_heightmap__prng_seed_scale");
static ShaderVariableInfo prng_seed_yaw("dagdp_heightmap__prng_seed_yaw");
static ShaderVariableInfo prng_seed_pitch("dagdp_heightmap__prng_seed_pitch");
static ShaderVariableInfo prng_seed_roll("dagdp_heightmap__prng_seed_roll");
static ShaderVariableInfo prng_seed_density("dagdp_heightmap__prng_seed_density");
static ShaderVariableInfo grid_jitter("dagdp_heightmap__grid_jitter");
static ShaderVariableInfo displacement_noise_scale("dagdp_heightmap__displacement_noise_scale");
static ShaderVariableInfo displacement_strength("dagdp_heightmap__displacement_strength");
static ShaderVariableInfo placement_noise_scale("dagdp_heightmap__placement_noise_scale");
static ShaderVariableInfo sample_range("dagdp_heightmap__sample_range");
static ShaderVariableInfo lower_level("dagdp_heightmap__lower_level");
static ShaderVariableInfo use_decals("dagdp_heightmap__use_decals");
static ShaderVariableInfo discard_on_grass_erasure("dagdp_heightmap__discard_on_grass_erasure");

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
};

struct HeightmapConstants
{
  ViewInfo viewInfo;
  uint32_t numTiles;
  uint32_t numRenderables;
  uint32_t numPlaceables;
  uint32_t numBiomes;
  float tileWorldSize;
  float maxPlaceableBoundingRadius;
  uint32_t prngSeed;
  float gridJitter;
  float displacementNoiseScale;
  float displacementStrength;
  float placementNoiseScale;
  float sampleRange;
  bool lowerLevel;
  bool useDecals;
  bool discardOnGrassErasure;
};

struct HeightmapPersistentData
{
  CommonPlacerBuffers commonBuffers;

  UniqueBuf placeableTileLimitsBuffer;
  UniqueBuf tilePositionsBuffer;
  UniqueBuf biomesBuffer;
  UniqueBuf densityMaskChannelWeightsBuffer;
  SharedTex densityMask;
  Point4 densityMaskScaleOffset;

  HeightmapConstants constants{};
};


#define IS_GRASS 0
#include "placer_common.h"

static void create_grid_nodes(const ViewInfo &view_info,
  const ViewBuilder &view_builder,
  NodeInserter node_inserter,
  const HeightmapGrid &grid,
  size_t grid_index,
  const SharedTex &density_mask,
  const Point4 &mask_scale_offset)
{
  var::density_mask_sampler.set_sampler(d3d::request_sampler({}));
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
  const dafg::NameSpace ns = dafg::root() / "dagdp" / view_info.uniqueName.c_str() / nameSpaceName.c_str();
  auto persistentData = eastl::make_shared<HeightmapPersistentData>();
  persistentData->densityMask = density_mask;
  persistentData->densityMaskScaleOffset = mask_scale_offset;

  HeightmapConstants &constants = persistentData->constants;
  constants.viewInfo = view_info;
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
  constants.displacementNoiseScale = grid.displacementNoiseScale;
  constants.displacementStrength = grid.displacementStrength;
  constants.placementNoiseScale = grid.placementNoiseScale;
  constants.sampleRange = grid.sampleRange;
  constants.lowerLevel = grid.lowerLevel;
  constants.useDecals = grid.useDecals;
  constants.discardOnGrassErasure = grid.discardOnGrassErasure;

  bool commonSuccess = init_common_placer_buffers(commonBufferInit, bufferNamePrefix, persistentData->commonBuffers);
  if (!commonSuccess)
    return;

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

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_density_mask_channel_weights", bufferNamePrefix.c_str());
    persistentData->densityMaskChannelWeightsBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(Point4), commonBufferInit.numPlaceables, bufferName.c_str());

    auto lockedBuffer = lock_sbuffer<Point4>(persistentData->densityMaskChannelWeightsBuffer.getBuf(), 0,
      commonBufferInit.numPlaceables, VBLOCK_WRITEONLY);
    if (!lockedBuffer)
    {
      logerr("daGdp: could not lock buffer %s", bufferName.c_str());
      return;
    }

    uint32_t i = 0;
    for (const auto &objectGroup : commonBufferInit.objectGroupsFmem)
      for (const auto &placeable : objectGroup.info->placeables)
      {
        lockedBuffer[i++] = objectGroup.densityMaskChannelWeights;
        G_UNREFERENCED(placeable);
      }

    G_ASSERT(i == commonBufferInit.numPlaceables);
  }

  node_inserter(create_indirect_args_node(ns, persistentData));
  node_inserter(create_cull_tiles_node(ns, persistentData));

  node_inserter(create_place_node(ns, persistentData, true));
  node_inserter(create_place_node(ns, persistentData, false));
}

void create_heightmap_nodes(
  const ViewInfo &view_info, const ViewBuilder &view_builder, const HeightmapManager &heightmap_manager, NodeInserter node_inserter)
{
  if (view_builder.totalMaxInstances == 0)
    return; // Nothing to do, early exit.

  // TODO: merge these separate grid nodes into a single node/dispatch.
  for (const auto [i, grid] : enumerate(heightmap_manager.currentBuilder.grids))
    create_grid_nodes(view_info, view_builder, node_inserter, grid, i, heightmap_manager.currentBuilder.densityMask,
      heightmap_manager.currentBuilder.maskScaleOffset);
}

} // namespace dagdp