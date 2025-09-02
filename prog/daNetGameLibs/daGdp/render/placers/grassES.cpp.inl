// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <ecs/core/entitySystem.h>
#include <generic/dag_enumerate.h>
#include <math/dag_hlsl_floatx.h>
#include "../../shaders/dagdp_heightmap.hlsli"
#include "../globalManager.h"
#include "grass.h"

ECS_REGISTER_BOXED_TYPE(dagdp::GrassManager, nullptr);

namespace dagdp
{

static inline float pow2f(float x) { return x * x; }

template <typename Callable>
static inline void grass_settings_ecs_query(Callable);

template <typename Callable>
static inline void grass_placers_ecs_query(Callable);

template <typename Callable>
static inline void manager_ecs_query(Callable);


ECS_NO_ORDER
static inline void grass_view_process_es(const dagdp::EventViewProcess &evt, dagdp::GrassManager &dagdp__grass_manager)
{
  FRAMEMEM_REGION;

  const auto &rulesBuilder = evt.get<0>();
  const auto &viewInfo = evt.get<1>();
  auto &builder = dagdp__grass_manager.currentBuilder;
  auto &grassMaxRange = builder.grassMaxRange;
  grassMaxRange = viewInfo.maxDrawDistance;

  grass_settings_ecs_query([&](ECS_REQUIRE(ecs::Tag dagdp_level_settings) float dagdp__grass_max_range) {
    grassMaxRange = eastl::min(grassMaxRange, dagdp__grass_max_range);
  });

  grass_placers_ecs_query(
    [&](ECS_REQUIRE(ecs::Tag dagdp_placer_grass) ecs::EntityId eid, const ecs::List<int> &dagdp__biomes, float dagdp__density,
      const ecs::string &dagdp__name, const float *fast_grass__height, const float *fast_grass__height_variance) {
      auto iter = rulesBuilder.placers.find(eid);
      if (iter == rulesBuilder.placers.end())
        return;

      auto &placer = iter->second;

      if (rulesBuilder.maxObjects == 0)
      {
        logerr("daGdp: %s is disabled, as the max. objects setting is 0", dagdp__name);
        return;
      }

      if (dagdp__biomes.size() == 0)
      {
        logerr("daGdp: %s should have at least one biome index in dagdp__biomes. Skipping it.", dagdp__name);
        return;
      }

      if (dagdp__density <= 0.0f)
      {
        logerr("daGdp: %s has invalid density", dagdp__name);
        return;
      }

      GrassGrid *matchingGrid = nullptr;
      for (auto &grid : builder.grids)
        if (grid.density == dagdp__density)
        {
          matchingGrid = &grid;
          break;
        }

      if (!matchingGrid)
      {
        matchingGrid = &builder.grids.push_back();
        matchingGrid->density = dagdp__density;
      }

      for (const auto biomeIndex : dagdp__biomes)
      {
        GrassGridVariant *matchingVariant = nullptr;
        for (auto &variant : matchingGrid->variants)
          if (variant.biome == biomeIndex)
          {
            matchingVariant = &variant;
            break;
          }

        if (!matchingVariant)
        {
          matchingVariant = &matchingGrid->variants.push_back();
          matchingVariant->biome = biomeIndex;
          matchingVariant->height = fast_grass__height ? *fast_grass__height : 1.0f;
          matchingVariant->heightVariance = fast_grass__height_variance ? *fast_grass__height_variance : 0.0f;
        }
        else
        {
          if (fast_grass__height)
            matchingVariant->height = eastl::max(matchingVariant->height, *fast_grass__height);
          if (fast_grass__height_variance)
            matchingVariant->heightVariance = eastl::max(matchingVariant->heightVariance, *fast_grass__height_variance);
        }

        for (const auto objectGroupEid : placer.objectGroupEids)
        {
          auto iter = rulesBuilder.objectGroups.find(objectGroupEid);
          if (iter == rulesBuilder.objectGroups.end())
            continue;
          auto &group = matchingVariant->objectGroups.push_back();
          group.info = &iter->second;
          group.effectiveDensity = 1;
        }
      }
    });

  for (auto &grid : builder.grids)
  {
    grid.tileWorldSize = TILE_INSTANCE_COUNT_1D / sqrtf(grid.density);

    float maxDrawDistance = 0;
    for (const auto &variant : grid.variants)
      for (const auto &objectGroup : variant.objectGroups)
        maxDrawDistance = eastl::max(maxDrawDistance, eastl::min(objectGroup.info->maxDrawDistance, grassMaxRange));

    int32_t maxTileOffset = ceilf(maxDrawDistance / grid.tileWorldSize);
    for (int32_t x = -maxTileOffset; x <= maxTileOffset; ++x)
    {
      for (int32_t z = -maxTileOffset; z <= maxTileOffset; ++z)
      {
        int32_t x1 = x > 0 ? x - 1 : (x < 0 ? x + 1 : 0);
        int32_t z1 = z > 0 ? z - 1 : (z < 0 ? z + 1 : 0);
        // Tile fully outside max. draw distance?
        if ((pow2f(x1) + pow2f(z1)) * pow2f(grid.tileWorldSize) > pow2f(maxDrawDistance))
          continue;

        grid.tiles.push_back(GrassTileCoord{x, z});
      }
    }

    stlsort::sort(grid.tiles.begin(), grid.tiles.end());
  }
}

ECS_NO_ORDER static inline void grass_view_finalize_es(const dagdp::EventViewFinalize &evt, dagdp::GrassManager &dagdp__grass_manager)
{
  const auto &viewInfo = evt.get<0>();
  const auto &viewBuilder = evt.get<1>();
  auto nodes = evt.get<2>();

  create_grass_nodes(viewInfo, viewBuilder, dagdp__grass_manager, nodes);

  // Cleanup.
#if DAGDP_DEBUG
  dagdp__grass_manager.debug.builders.emplace_back(eastl::move(dagdp__grass_manager.currentBuilder));
#else
  dagdp__grass_manager.currentBuilder = {}; // Reset the state.
#endif
}

template <typename Callable>
static inline void manager_ecs_query(Callable);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_ON_EVENT(on_disappear)
ECS_TRACK(dagdp__name, dagdp__biomes, dagdp__density)
ECS_REQUIRE(ecs::Tag dagdp_placer_grass, const ecs::string &dagdp__name, const ecs::List<int> &dagdp__biomes, float dagdp__density)
static void dagdp_placer_grass_changed_es(const ecs::Event &)
{
  manager_ecs_query([](dagdp::GlobalManager &dagdp__global_manager) { dagdp__global_manager.invalidateRules(); });
}

} // namespace dagdp
