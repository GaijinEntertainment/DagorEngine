// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <ecs/core/entitySystem.h>
#include <generic/dag_enumerate.h>
#include <math/dag_hlsl_floatx.h>
#include "../../shaders/dagdp_heightmap.hlsli"
#include "../globalManager.h"
#include "heightmap.h"

ECS_REGISTER_BOXED_TYPE(dagdp::HeightmapManager, nullptr);

namespace dagdp
{

static inline float pow2f(float x) { return x * x; }
static inline uint32_t divUp(uint32_t size, uint32_t stride) { return (size + stride - 1) / stride; }

static constexpr float TILE_LIMIT_MULTIPLIER = 2.0f;

template <typename Callable>
static inline void heightmap_placers_ecs_query(Callable);

template <typename Callable>
static inline void heightmap_density_masks_ecs_query(Callable);

template <typename Callable>
static inline void manager_ecs_query(Callable);

struct MergeEntry
{
  int seed;
  float jitter;
  bool lowerLevel;
  int drawRangeLogFloor;

  int biomeIndex;

  const ObjectGroupInfo *objectGroupInfo;
  float effectiveDensity;
};

#define COMPARE_GRID_FIELDS \
  CMP(seed)                 \
  CMP(jitter)               \
  CMP(lowerLevel)           \
  CMP(drawRangeLogFloor)

#define COMPARE_OTHER_FIELDS \
  CMP(biomeIndex)            \
  CMP(objectGroupInfo)       \
  CMP(effectiveDensity)

static bool operator<(const MergeEntry &a, const MergeEntry &b)
{
#define CMP(field)        \
  if (a.field != b.field) \
    return a.field < b.field;

  COMPARE_GRID_FIELDS
  COMPARE_OTHER_FIELDS
  return false;

#undef CMP
}

#define CMP(field)        \
  if (a.field != b.field) \
    return false;

static bool same_grid(const MergeEntry &a, const MergeEntry &b)
{
  COMPARE_GRID_FIELDS
  return true;
}

#undef CMP

ECS_NO_ORDER
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(ecs::Tag dagdp_density_mask)
static inline void heightmap_density_mask_disappeared_es(const ecs::Event &)
{
  manager_ecs_query([](dagdp::GlobalManager &dagdp__global_manager) { dagdp__global_manager.invalidateRules(); });
}

ECS_NO_ORDER
static inline void heightmap_initialize_density_mask_es(const dagdp::EventInitialize &,
  dagdp::HeightmapManager &dagdp__heightmap_manager)
{
  auto &currentBuilder = dagdp__heightmap_manager.currentBuilder;

  heightmap_density_masks_ecs_query([&](ECS_REQUIRE(ecs::Tag dagdp_density_mask) const ecs::string &dagdp__density_mask_res) {
    currentBuilder.densityMask = dag::get_tex_gameres(dagdp__density_mask_res.c_str());
    if (currentBuilder.densityMask.getTexId() == BAD_TEXTUREID)
      logerr("daGdp: density mask texture %s not found", dagdp__density_mask_res.c_str());
  });
}

ECS_NO_ORDER
static inline void heightmap_view_process_es(const dagdp::EventViewProcess &evt, dagdp::HeightmapManager &dagdp__heightmap_manager)
{
  FRAMEMEM_REGION;

  const auto &rulesBuilder = evt.get<0>();
  const auto &viewInfo = evt.get<1>();
  auto &viewBuilder = *evt.get<2>();
  auto &builder = dagdp__heightmap_manager.currentBuilder;

  dag::Vector<MergeEntry, framemem_allocator> entries;

  heightmap_placers_ecs_query([&](ECS_REQUIRE(ecs::Tag dagdp_placer_heightmap) ecs::EntityId eid, const ecs::List<int> &dagdp__biomes,
                                float dagdp__density, int dagdp__seed, float dagdp__jitter, bool dagdp__heightmap_lower_level,
                                bool dagdp__heightmap_allow_unoptimal_grids, float dagdp__heightmap_cell_size) {
    auto iter = rulesBuilder.placers.find(eid);
    if (iter == rulesBuilder.placers.end())
      return;

    auto &placer = iter->second;

    if (dagdp__biomes.size() == 0)
    {
      logerr("daGdp: placer with EID %u should have at least one biome index in dagdp__biomes. Skipping it.",
        static_cast<unsigned int>(eid));
      return;
    }

    if (dagdp__density <= 0.0f)
    {
      logerr("daGdp: placer with EID %u has invalid density", static_cast<unsigned int>(eid));
      return;
    }

    if (dagdp__heightmap_cell_size <= 0.0f)
    {
      logerr("daGdp: placer with EID %u has invalid heightmap cell size", static_cast<unsigned int>(eid));
      return;
    }

    if (dagdp__jitter < 0.0f || dagdp__jitter > 1.0f)
    {
      logerr("daGdp: placer with EID %u has invalid jitter", static_cast<unsigned int>(eid));
      return;
    }

    if (!dagdp__heightmap_allow_unoptimal_grids && dagdp__heightmap_lower_level)
    {
      logerr("daGdp: placer with EID %u has non-default dagdp__heightmap_lower_level, which may prevent using optimal grids, but "
             "allow_unoptimal_grids is not set.",
        static_cast<unsigned int>(eid));
      return;
    }

    if (!dagdp__heightmap_allow_unoptimal_grids && dagdp__seed != 0)
    {
      logerr(
        "daGdp: placer with EID %u has non-default seed, which may prevent using optimal grids, but allow_unoptimal_grids is not set.",
        static_cast<unsigned int>(eid));
      return;
    }

    if (!dagdp__heightmap_allow_unoptimal_grids && dagdp__jitter != 1.0)
    {
      logerr("daGdp: placer with EID %u has non-default jitter, which may prevent using optimal grids, but allow_unoptimal_grids is "
             "not set.",
        static_cast<unsigned int>(eid));
      return;
    }

    // This is shared between object groups to make sure they are forced into the same grid, and thus there is
    // no inter-grid collisions (accidental close placements) between objects. This is not really optimal,
    // because with large draw distance disparity between objects, it's better to split them into separate grids.
    // Maybe this should be configurable.
    float viewIndependentMaxDrawDistance = 0.0f;
    for (const auto objectGroupEid : placer.objectGroupEids)
    {
      auto iter = rulesBuilder.objectGroups.find(objectGroupEid);
      if (iter == rulesBuilder.objectGroups.end())
        continue;

      // Note: don't use the view maxDrawDistance here, because the grids must be consistent between views.
      viewIndependentMaxDrawDistance = eastl::max(viewIndependentMaxDrawDistance, iter->second.maxDrawDistance);
    }

    MergeEntry entry;
    entry.jitter = dagdp__jitter;
    entry.seed = dagdp__seed;
    entry.lowerLevel = dagdp__heightmap_lower_level;
    entry.drawRangeLogFloor = floor(log(viewIndependentMaxDrawDistance) / log(dagdp__heightmap_manager.config.drawRangeLogBase));
    entry.effectiveDensity = dagdp__density / pow2f(dagdp__heightmap_cell_size);
    for (const auto biomeIndex : dagdp__biomes)
    {
      entry.biomeIndex = biomeIndex;
      for (const auto objectGroupEid : placer.objectGroupEids)
      {
        auto iter = rulesBuilder.objectGroups.find(objectGroupEid);
        if (iter == rulesBuilder.objectGroups.end())
          continue;
        entry.objectGroupInfo = &iter->second;
        entries.push_back(entry);
      }
    }
  });

  stlsort::sort(entries.begin(), entries.end());

  HeightmapGrid *lastGrid = nullptr;
  HeightmapGridVariant *lastVariant = nullptr;
  PlacerObjectGroup *lastObjectGroup = nullptr;
  const MergeEntry *lastEntry = nullptr;

  // Note: calling this inside the loop below is worst-case O(N^2). In practice, this should not be a problem, because
  // the number of variants themselves should not be that high.
  auto maybeMergeLastVariant = [&]() {
    const uint32_t numVariants = lastGrid ? lastGrid->variants.size() : 0;
    if (numVariants >= 2)
    {
      const auto *v = &lastGrid->variants[numVariants - 1];
      for (uint32_t i = 0; i < numVariants - 1; ++i)
      {
        auto *vI = &lastGrid->variants[i];
        if (vI->objectGroups.size() == v->objectGroups.size())
        {
          bool sameGroups = true;
          for (uint32_t j = 0; j < vI->objectGroups.size(); ++j)
            if ((vI->objectGroups[j].effectiveDensity != v->objectGroups[j].effectiveDensity) ||
                (vI->objectGroups[j].info != v->objectGroups[j].info))
            {
              sameGroups = false;
              break;
            }

          if (sameGroups)
          {
            // Merge variants.
            vI->biomes.insert(vI->biomes.end(), v->biomes.begin(), v->biomes.end());
            vI->effectiveDensity = max(vI->effectiveDensity, v->effectiveDensity);
            lastGrid->variants.pop_back();
          }
        }
      }
    }
  };

  for (const auto &entry : entries)
  {
    if (!lastGrid || !same_grid(*lastEntry, entry))
    {
      maybeMergeLastVariant();

      lastGrid = &builder.grids.push_back();
      lastGrid->prngSeed = entry.seed;
      lastGrid->gridJitter = entry.jitter;
      lastGrid->lowerLevel = entry.lowerLevel;
      lastVariant = nullptr;
      lastObjectGroup = nullptr;
    }

    if (!lastVariant || lastEntry->biomeIndex != entry.biomeIndex)
    {
      maybeMergeLastVariant();

      lastVariant = &lastGrid->variants.push_back();
      lastVariant->biomes.push_back(entry.biomeIndex);
      lastObjectGroup = nullptr;
    }

    if (!lastObjectGroup || lastEntry->objectGroupInfo != entry.objectGroupInfo)
    {
      lastObjectGroup = &lastVariant->objectGroups.push_back();
      lastObjectGroup->info = entry.objectGroupInfo;
    }

    lastObjectGroup->effectiveDensity += entry.effectiveDensity;
    lastVariant->effectiveDensity += entry.effectiveDensity;

    lastEntry = &entry;
  }

  maybeMergeLastVariant();

  for (auto &grid : builder.grids)
  {
    for (const auto &variant : grid.variants)
      grid.density = max(grid.density, variant.effectiveDensity);

    grid.tileWorldSize = TILE_INSTANCE_COUNT_1D / sqrtf(grid.density);
  }

  for (auto &grid : builder.grids)
  {
    uint32_t totalPlaceableCount = 0;
    for (const auto &variant : grid.variants)
      for (const auto &objectGroup : variant.objectGroups)
        totalPlaceableCount += objectGroup.info->placeables.size();

    grid.placeablesTileLimits.resize(totalPlaceableCount);

    uint32_t pIdStart = 0;
    for (const auto &variant : grid.variants)
    {
      for (const auto &objectGroup : variant.objectGroups)
      {
        for (const auto [pIdOffset, placeable] : enumerate(objectGroup.info->placeables))
        {
          uint32_t pId = pIdStart + pIdOffset;

          // TODO: this "tile limit" logic is generalizable to other placers and could be shared.
          //
          // TODO: placeables for the same object are now distinct for different variants. So, the per-tile limit
          // will be calculated independently for each such placeable, leading to more memory usage than optimal.
          //
          // Note: placeable weight must already have been group-normalized at this point.
          float expectedInstancesPerTile = TILE_INSTANCE_COUNT * objectGroup.effectiveDensity / grid.density * placeable.params.weight;

          // Approximates a reasonable upper bound of number of instances per tile.
          uint32_t tileLimit = ceilf(TILE_LIMIT_MULTIPLIER * expectedInstancesPerTile);

          tileLimit = eastl::min(static_cast<uint32_t>(TILE_INSTANCE_COUNT), eastl::max(1u, tileLimit));
          grid.placeablesTileLimits[pId] = tileLimit;

          float minScale = md_min(placeable.params.scaleMidDev);
          float maxScale = md_max(placeable.params.scaleMidDev);
          G_ASSERT(minScale > 0.0f);

          float minDrawDistance = 0.0f;
          for (const auto &range : placeable.ranges)
          {
            // We extend the min/max ranges by min/maxScale respectively, since the scaled objects will appear in a larger range.
            float maxDrawDistance = eastl::min(range.baseDrawDistance * maxScale, viewInfo.maxDrawDistance);

            int32_t maxTileOffset = ceilf(maxDrawDistance / grid.tileWorldSize);
            for (int32_t x = -maxTileOffset; x <= maxTileOffset; ++x)
            {
              for (int32_t z = -maxTileOffset; z <= maxTileOffset; ++z)
              {
                // Tile fully inside min. draw distance?
                if ((pow2f(x) + pow2f(z)) * pow2f(grid.tileWorldSize) < pow2f(minDrawDistance))
                  continue;

                int32_t x1 = x > 0 ? x - 1 : (x < 0 ? x + 1 : 0);
                int32_t z1 = z > 0 ? z - 1 : (z < 0 ? z + 1 : 0);
                // Tile fully outside max. draw distance?
                if ((pow2f(x1) + pow2f(z1)) * pow2f(grid.tileWorldSize) > pow2f(maxDrawDistance))
                  continue;

                // TODO: the distribution of scales is ignored here. When scale deviation is large,
                // this uses significantly more memory. Looks like this can be improved with more nuanced calculations.

                // TODO: when the view draw distance does not cover a tile, this obviously wastes some memory.
                viewBuilder.renderablesMaxInstances[range.rId] += grid.placeablesTileLimits[pId];

                grid.tiles.push_back_unsorted(HeightmapTileCoord{x, z});
              }
            }

            // The min. draw distance for the next range is based on max. draw distance of current range.
            minDrawDistance = range.baseDrawDistance * minScale;

            if (minDrawDistance > viewInfo.maxDrawDistance)
              break;
          }
        }

        pIdStart += objectGroup.info->placeables.size();
      }
    }

    // TODO: when reusing the view for lesser draw distances (for example, for shadows), we can
    // only use the tiles corresponding to that draw distance. For that, the tiles must be placed in the
    // array in a pattern of enlarging area, and a lookup table for (draw distance -> num. tiles) computed.
    auto &tiles = grid.tiles;
    stlsort::sort(tiles.begin(), tiles.end());
    tiles.erase(eastl::unique(tiles.begin(), tiles.end()), tiles.end());
  }
}

ECS_NO_ORDER static inline void heightmap_view_finalize_es(const dagdp::EventViewFinalize &evt,
  dagdp::HeightmapManager &dagdp__heightmap_manager)
{
  const auto &viewInfo = evt.get<0>();
  const auto &viewBuilder = evt.get<1>();
  auto nodes = evt.get<2>();

  create_heightmap_nodes(viewInfo, viewBuilder, dagdp__heightmap_manager, nodes);

  // Cleanup.
#if DAGOR_DBGLEVEL == 0
  dagdp__heightmap_manager.currentBuilder = {}; // Reset the state.
#else
  dagdp__heightmap_manager.debug.builders.emplace_back(eastl::move(dagdp__heightmap_manager.currentBuilder));
#endif
}

template <typename Callable>
static inline void manager_ecs_query(Callable);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_ON_EVENT(on_disappear)
ECS_TRACK(dagdp__name,
  dagdp__biomes,
  dagdp__density,
  dagdp__seed,
  dagdp__jitter,
  dagdp__heightmap_allow_unoptimal_grids,
  dagdp__heightmap_lower_level,
  dagdp__heightmap_cell_size)
ECS_REQUIRE(ecs::Tag dagdp_placer_heightmap,
  const ecs::string &dagdp__name,
  const ecs::List<int> &dagdp__biomes,
  float dagdp__density,
  int dagdp__seed,
  float dagdp__jitter,
  bool dagdp__heightmap_allow_unoptimal_grids,
  bool dagdp__heightmap_lower_level,
  float dagdp__heightmap_cell_size)
static void dagdp_placer_heightmap_changed_es(const ecs::Event &)
{
  manager_ecs_query([](dagdp::GlobalManager &dagdp__global_manager) { dagdp__global_manager.invalidateRules(); });
}

} // namespace dagdp
