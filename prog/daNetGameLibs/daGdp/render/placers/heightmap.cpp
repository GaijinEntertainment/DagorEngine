// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/fixed_string.h>
#include <EASTL/shared_ptr.h>
#include <generic/dag_enumerate.h>
#include <util/dag_convar.h>
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

CONSOLE_FLOAT_VAL("dagdp", heightmap_frustum_culling_bias, 0.0f);

struct Variant
{
  uint32_t drawRangeStartIndex;
  uint32_t renderableIndicesStartIndex; // Not a typo.
  uint32_t placeableStartIndex;
};

struct Biome
{
  uint32_t variantIndex;
};

struct HeightmapConstants
{
  ViewInfo viewInfo;
  uint32_t maxInstancesPerViewport;
  uint32_t numDrawRanges;
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
  UniqueBuf drawRangesBuffer;
  UniqueBuf placeablesBuffer;
  UniqueBuf placeableWeightsBuffer;
  UniqueBuf placeableTileLimitsBuffer;
  UniqueBuf renderableIndicesBuffer;
  UniqueBuf instanceRegionsBuffer;
  UniqueBuf tilePositionsBuffer;
  UniqueBuf biomesBuffer;
  UniqueBuf variantsBuffer;
  SharedTex densityMask;

  HeightmapConstants constants{};
};

static dabfg::NodeHandle create_cull_tiles_node(const dabfg::NameSpace &ns,
  const eastl::shared_ptr<HeightmapPersistentData> &persistentData)
{
  return ns.registerNode("cull_tiles", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);

    const eastl::fixed_string<char, 32> viewResourceName({}, "view@%s", constants.viewInfo.uniqueName.c_str());
    const auto viewHandle = registry.readBlob<ViewPerFrameData>(viewResourceName.c_str()).handle();

    const eastl::fixed_string<char, 64> indirectBufferName({}, "dagdp_heightmap_indirect_dispatch_%s", viewResourceName.c_str());
    const auto indirectArgsHandle = registry.create(indirectBufferName.c_str(), dabfg::History::No)
                                      .indirectBufferUa(d3d::buffers::Indirect::Dispatch, ViewPerFrameData::MAX_VIEWPORTS)
                                      .atStage(dabfg::Stage::COMPUTE)
                                      .bindToShaderVar("dagdp_heightmap__indirect_args")
                                      .handle();

    const eastl::fixed_string<char, 64> visibleTilePosBufferName({}, "dagdp_heightmap_visible_tile_positions_%s",
      viewResourceName.c_str());
    registry.create(visibleTilePosBufferName.c_str(), dabfg::History::No)
      .byteAddressBufferUaSr(constants.numTiles * 2) // int2
      .atStage(dabfg::Stage::COMPUTE)
      .bindToShaderVar("dagdp_heightmap__visible_tile_positions");

    return [persistentData, viewHandle, indirectArgsHandle, shader = ComputeShader("dagdp_heightmap_cull_tiles")] {
      const auto &constants = persistentData->constants;
      const auto &view = viewHandle.ref();

      ShaderGlobal::set_int(var::num_tiles, constants.numTiles);
      ShaderGlobal::set_real(var::tile_pos_delta, constants.tileWorldSize);
      ShaderGlobal::set_real(var::max_placeable_bounding_radius, constants.maxPlaceableBoundingRadius);

      ShaderGlobal::set_buffer(var::tile_positions, persistentData->tilePositionsBuffer.getBufId());

      const uint32_t val[4] = {1, 1, 1, 1};
      d3d::clear_rwbufi(indirectArgsHandle.get(), val);

      for (uint32_t viewportIndex = 0; viewportIndex < view.viewports.size(); ++viewportIndex)
      {
        const auto &viewport = view.viewports[viewportIndex];
        ScopeFrustumPlanesShaderVars scopedFrustumVars(viewport.frustum);

        IPoint4 baseTileIntPos;
        baseTileIntPos.x = static_cast<int>(floorf(viewport.worldPos.x / constants.tileWorldSize));
        baseTileIntPos.y = static_cast<int>(floorf(viewport.worldPos.z / constants.tileWorldSize));

        ShaderGlobal::set_color4(var::base_tile_pos_xz, point4(baseTileIntPos) * constants.tileWorldSize);
        ShaderGlobal::set_int(var::viewport_index, viewportIndex);

        const bool res = shader.dispatchThreads(constants.numTiles, 1, 1);
        G_ASSERT(res);
        G_UNUSED(res);
      }

      ShaderGlobal::set_buffer(var::tile_positions, BAD_D3DRESID);
    };
  });
}

static dabfg::NodeHandle create_place_node(const dabfg::NameSpace &ns,
  const eastl::shared_ptr<HeightmapPersistentData> &persistentData)
{
  return ns.registerNode("place", DABFG_PP_NODE_SRC, [persistentData](dabfg::Registry registry) {
    const auto &constants = persistentData->constants;
    view_multiplex(registry, constants.viewInfo.kind);

    (registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str())
      .modify("instance_data")
      .buffer()
      .atStage(dabfg::Stage::COMPUTE)
      .bindToShaderVar("dagdp__instance_data");

    (registry.root() / "dagdp" / constants.viewInfo.uniqueName.c_str())
      .modify("counters")
      .buffer()
      .atStage(dabfg::Stage::COMPUTE)
      .bindToShaderVar("dagdp__counters");

    eastl::fixed_string<char, 32> viewResourceName;
    viewResourceName.append_sprintf("view@%s", constants.viewInfo.uniqueName.c_str());
    const auto viewHandle = registry.readBlob<ViewPerFrameData>(viewResourceName.c_str()).handle();

    eastl::fixed_string<char, 64> indirectBufferName({}, "dagdp_heightmap_indirect_dispatch_%s", viewResourceName.c_str());
    const auto indirectArgsHandle = registry.read(indirectBufferName.c_str())
                                      .buffer()
                                      .atStage(dabfg::Stage::COMPUTE)
                                      .useAs(dabfg::Usage::INDIRECTION_BUFFER)
                                      .handle();

    eastl::fixed_string<char, 64> visibleTilePosBufferName({}, "dagdp_heightmap_visible_tile_positions_%s", viewResourceName.c_str());
    registry.read(visibleTilePosBufferName.c_str())
      .buffer()
      .atStage(dabfg::Stage::COMPUTE)
      .bindToShaderVar("dagdp_heightmap__visible_tile_positions");

    return [persistentData, viewHandle, indirectArgsHandle, shader = ComputeShader("dagdp_heightmap_place")] {
      const auto &constants = persistentData->constants;
      const auto &view = viewHandle.ref();
      G_ASSERT(view.viewports.size() <= constants.viewInfo.maxViewports);

      ShaderGlobal::set_buffer(var::draw_ranges, persistentData->drawRangesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::placeables, persistentData->placeablesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::placeable_weights, persistentData->placeableWeightsBuffer.getBufId());
      ShaderGlobal::set_buffer(var::placeable_tile_limits, persistentData->placeableTileLimitsBuffer.getBufId());
      ShaderGlobal::set_buffer(var::renderable_indices, persistentData->renderableIndicesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::instance_regions, persistentData->instanceRegionsBuffer.getBufId());
      ShaderGlobal::set_buffer(var::biomes, persistentData->biomesBuffer.getBufId());
      ShaderGlobal::set_buffer(var::variants, persistentData->variantsBuffer.getBufId());

      ShaderGlobal::set_texture(var::density_mask, persistentData->densityMask);

      ShaderGlobal::set_int(var::num_renderables, constants.numRenderables);
      ShaderGlobal::set_int(var::num_placeables, constants.numPlaceables);
      ShaderGlobal::set_int(var::num_biomes, constants.numBiomes);
      ShaderGlobal::set_int(var::num_tiles, constants.numTiles);

      ShaderGlobal::set_real(var::max_placeable_bounding_radius, constants.maxPlaceableBoundingRadius);
      ShaderGlobal::set_real(var::tile_pos_delta, constants.tileWorldSize);
      ShaderGlobal::set_real(var::instance_pos_delta, constants.tileWorldSize / TILE_INSTANCE_COUNT_1D);
      ShaderGlobal::set_real(var::debug_frustum_culling_bias, heightmap_frustum_culling_bias.get());

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
        ShaderGlobal::set_int(var::viewport_instance_offset, constants.maxInstancesPerViewport * viewportIndex);

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
  const SharedTex &density_mask)
{
  FRAMEMEM_REGION;

  // Draw ranges are concatenated across variants, with `drawRangeStartIndex` marking the start of entries for the variant.
  // For a single variant, (P * R) float distances are stored, where:
  // - P = number of placeables in the variant.
  // - R = number of ranges in the variant.
  DrawRangesFmem drawRangesFmem;

  // Renderable indices are concatenated across variants, with `renderableIndicesStartIndex` marking the start of entries for the
  // variant. For a single variant, (P * R) renderable indices (IDs) are stored, where:
  // - P = number of placeables in the variant.
  // - R = number of ranges in the variant.
  RenderableIndicesFmem renderableIndicesFmem;

  dag::RelocatableFixedVector<Variant, 16, true, framemem_allocator> perVariantFmem;
  dag::RelocatableFixedVector<Biome, 128, true, framemem_allocator> perBiomeFmem;

  perVariantFmem.resize(grid.variants.size());

  uint32_t pIdStart = 0;
  for (const auto [variantIndex, variant] : enumerate(grid.variants))
  {
    perVariantFmem[variantIndex].drawRangeStartIndex = drawRangesFmem.size();
    perVariantFmem[variantIndex].renderableIndicesStartIndex = renderableIndicesFmem.size();
    perVariantFmem[variantIndex].placeableStartIndex = pIdStart;

    calculate_draw_ranges(pIdStart, variant.objectGroups, drawRangesFmem, renderableIndicesFmem);

    uint32_t numVariantPlaceables = 0;
    for (const auto &objectGroup : variant.objectGroups)
      numVariantPlaceables += objectGroup.info->placeables.size();

    pIdStart += numVariantPlaceables;
  }

  uint32_t maxBiomeIndex = 0;
  for (const auto &variant : grid.variants)
    for (const auto ix : variant.biomes)
      maxBiomeIndex = max(maxBiomeIndex, static_cast<uint32_t>(ix));

  perBiomeFmem.assign(maxBiomeIndex + 1, Biome{~0u});

  for (const auto [variantIndex, variant] : enumerate(grid.variants))
    for (const auto biomeIndex : variant.biomes)
      perBiomeFmem[biomeIndex].variantIndex = variantIndex;

  if (grid.tiles.empty())
    return;

  TmpName nameSpaceName(TmpName::CtorSprintf(), "heightmap_%zu", grid_index);
  const dabfg::NameSpace ns = dabfg::root() / "dagdp" / view_info.uniqueName.c_str() / nameSpaceName.c_str();
  auto persistentData = eastl::make_shared<HeightmapPersistentData>();
  persistentData->densityMask = density_mask;

  HeightmapConstants &constants = persistentData->constants;
  constants.viewInfo = view_info;
  constants.maxInstancesPerViewport = view_builder.maxInstancesPerViewport;
  constants.numDrawRanges = drawRangesFmem.size();
  constants.numTiles = grid.tiles.size();
  constants.numPlaceables = pIdStart;
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

  TmpName bufferNamePrefix(TmpName::CtorSprintf(), "dagdp_%s_heightmap_%zu", view_info.uniqueName.c_str(), grid_index);
  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_draw_ranges", bufferNamePrefix.c_str());
    persistentData->drawRangesBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(float), drawRangesFmem.size(), bufferName.c_str());

    bool updated = persistentData->drawRangesBuffer->updateData(0, data_size(drawRangesFmem), drawRangesFmem.data(), VBLOCK_WRITEONLY);

    if (!updated)
    {
      logerr("daGdp: could not update buffer %s", bufferName.c_str());
      return;
    }
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_placeables", bufferNamePrefix.c_str());
    persistentData->placeablesBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(PlaceableGpuData), constants.numPlaceables, bufferName.c_str());

    auto lockedBuffer =
      lock_sbuffer<PlaceableGpuData>(persistentData->placeablesBuffer.getBuf(), 0, constants.numPlaceables, VBLOCK_WRITEONLY);
    if (!lockedBuffer)
    {
      logerr("daGdp: could not lock buffer %s", bufferName.c_str());
      return;
    }

    uint32_t i = 0;
    for (const auto &variant : grid.variants)
      for (const auto &objectGroup : variant.objectGroups)
        for (const auto &placeable : objectGroup.info->placeables)
        {
          auto &item = lockedBuffer[i++];
          const auto &params = placeable.params;

          item.yawRadiansMin = md_min(params.yawRadiansMidDev);
          item.yawRadiansMax = md_max(params.yawRadiansMidDev);
          item.pitchRadiansMin = md_min(params.pitchRadiansMidDev);
          item.pitchRadiansMax = md_max(params.pitchRadiansMidDev);
          item.rollRadiansMin = md_min(params.rollRadiansMidDev);
          item.rollRadiansMax = md_max(params.rollRadiansMidDev);
          item.scaleMin = md_min(params.scaleMidDev);
          item.scaleMax = md_max(params.scaleMidDev);
          item.maxBaseDrawDistance = placeable.ranges.back().baseDrawDistance;
          item.slopeFactor = params.slopeFactor;
          item.flags = params.flags; // TODO: little-endian assumed.
          item.riPoolOffset = params.riPoolOffset;
        }
  }

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_placeable_weights", bufferNamePrefix.c_str());
    persistentData->placeableWeightsBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(float), constants.numPlaceables, bufferName.c_str());

    auto lockedBuffer =
      lock_sbuffer<float>(persistentData->placeableWeightsBuffer.getBuf(), 0, constants.numPlaceables, VBLOCK_WRITEONLY);
    if (!lockedBuffer)
    {
      logerr("daGdp: could not lock buffer %s", bufferName.c_str());
      return;
    }

    uint32_t i = 0;
    for (const auto &variant : grid.variants)
      for (const auto &objectGroup : variant.objectGroups)
        for (const auto &placeable : objectGroup.info->placeables)
          lockedBuffer[i++] = placeable.params.weight * (objectGroup.effectiveDensity / grid.density);
  }

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
    TmpName bufferName(TmpName::CtorSprintf(), "%s_renderable_indices", bufferNamePrefix.c_str());
    persistentData->renderableIndicesBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(uint32_t), renderableIndicesFmem.size(), bufferName.c_str());

    bool updated = persistentData->renderableIndicesBuffer->updateData(0, data_size(renderableIndicesFmem),
      renderableIndicesFmem.data(), VBLOCK_WRITEONLY);

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

  {
    TmpName bufferName(TmpName::CtorSprintf(), "%s_variants", bufferNamePrefix.c_str());
    persistentData->variantsBuffer =
      dag::buffers::create_persistent_sr_structured(sizeof(VariantGpuData), grid.variants.size(), bufferName.c_str());

    auto lockedBuffer =
      lock_sbuffer<VariantGpuData>(persistentData->variantsBuffer.getBuf(), 0, grid.variants.size(), VBLOCK_WRITEONLY);
    if (!lockedBuffer)
    {
      logerr("daGdp: could not lock buffer %s", bufferName.c_str());
      return;
    }

    uint32_t i = 0;
    for (const auto &variant : grid.variants)
    {
      auto &item = lockedBuffer[i];
      const bool isLastVariant = i == grid.variants.size() - 1;

      item.placeableCount = 0;
      for (const auto &objectGroup : variant.objectGroups)
        item.placeableCount += objectGroup.info->placeables.size();

      item.placeableWeightEmpty = 1.0f - variant.effectiveDensity / grid.density;
      item.placeableStartIndex = perVariantFmem[i].placeableStartIndex;
      item.placeableEndIndex = item.placeableStartIndex + item.placeableCount;
      item.drawRangeStartIndex = perVariantFmem[i].drawRangeStartIndex;
      item.drawRangeEndIndex = isLastVariant ? drawRangesFmem.size() : perVariantFmem[i + 1].drawRangeStartIndex;
      item.renderableIndicesStartIndex = perVariantFmem[i].renderableIndicesStartIndex;

      ++i;
    }
  }

  node_inserter(create_cull_tiles_node(ns, persistentData));
  node_inserter(create_place_node(ns, persistentData));
}

void create_heightmap_nodes(
  const ViewInfo &view_info, const ViewBuilder &view_builder, const HeightmapManager &heightmap_manager, NodeInserter node_inserter)
{
  if (view_builder.maxInstancesPerViewport == 0)
    return; // Nothing to do, early exit.

  // TODO: merge these separate grid nodes into a single node/dispatch.
  // https://youtrack.gaijin.team/issue/RE-790/daGDP-merge-dispatches-of-heightmap-placers
  for (const auto [i, grid] : enumerate(heightmap_manager.currentBuilder.grids))
    create_grid_nodes(view_info, view_builder, node_inserter, grid, i, heightmap_manager.currentBuilder.densityMask);
}

} // namespace dagdp