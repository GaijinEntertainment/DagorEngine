// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "globalManager.h"
#include "bvh_dagdp.h"
#include <render/world/bvh.h>
#include <render/renderEvent.h>
#include <bvh/bvh_instanceMapper.h>
#include <shaders/dag_rendInstRes.h>
#include <util/dag_convar.h>
#include <ecs/render/updateStageRender.h>

CONSOLE_FLOAT_VAL_MINMAX("raytracing", max_dagdp_draw_distance, 32, 0, dagdp::GLOBAL_MAX_DRAW_DISTANCE);

ECS_REGISTER_BOXED_TYPE(dagdp::BvhManager, nullptr);

namespace dagdp
{
ECS_NO_ORDER
static void bvh_recreate_views_es(const dagdp::EventRecreateViews &evt, dagdp::BvhManager &dagdp__bvh_manager)
{
  G_UNUSED(dagdp__bvh_manager);

  // Check for both it being enabled, and the feature flag correctly being passed during context creation
  if (!is_bvh_dagdp_enabled() || !get_bvh_dagdp_instance_mapper())
    return;

  const auto &viewInserter = evt.get<1>();

  ViewInfo &info = viewInserter();
  info.uniqueName = "bvh";
  info.kind = ViewKind::BVH;
  info.maxDrawDistance = max_dagdp_draw_distance;
  info.maxViewports = 1;
}

ECS_TAG(render)
ECS_REQUIRE(dagdp::BvhManager &dagdp__bvh_manager)
static void bvh_changed_es(const BVHDagdpChanged &, dagdp::GlobalManager &dagdp__global_manager)
{
  dagdp__global_manager.destroyViews();
}

ECS_TAG(render)
ECS_BEFORE(dagdp_update_es)
static void dagdp_bvh_range_update_es(const UpdateStageInfoBeforeRender &, dagdp::GlobalManager &dagdp__global_manager)
{
  if (max_dagdp_draw_distance.pullValueChange() && is_bvh_dagdp_enabled() && get_bvh_dagdp_instance_mapper())
    dagdp__global_manager.destroyViews();
}

template <typename Callable>
static void get_bvh_manager_ecs_query(ecs::EntityManager &manager, Callable c);

BvhManager *get_bvh_manager()
{
  BvhManager *result = nullptr;
  get_bvh_manager_ecs_query(*g_entity_mgr, [&](dagdp::BvhManager &dagdp__bvh_manager) { result = &dagdp__bvh_manager; });
  return result;
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void bvh_manager_appear_es(ecs::Event &, dagdp::BvhManager &dagdp__bvh_manager)
{
  dagdp__bvh_manager.makeRTHWInstances = new_compute_shader("dagdp_bvh_riex_instantiate");
  dagdp__bvh_manager.makeIndirectArgs = new_compute_shader("dagdp_bvh_make_indirect_args");
}

DagdpBvhMapping BvhManager::getMappingData(const DagdpBvhPreMapping &preMapping) const
{
  DagdpBvhMapping result{};
  IPoint2 blas = IPoint2::ZERO;
  get_bvh_dagdp_instance_mapper()->mapRendinst(preMapping.riRes, preMapping.lodIdx, preMapping.elemIdx, blas, result.metaIndex);
  result.blas.x = blas.x;
  result.blas.y = blas.y;
  result.isFoliage = preMapping.riRes->hasTreeOrFlag();
  result.wind_channel_strength = Point3::rgb(preMapping.wind_channel_strength);
  G_ASSERTF(preMapping.elemIdx == 0 || blas == IPoint2::ZERO || !preMapping.riRes->isUsedAsDagdp() ||
              get_bvh_dagdp_instance_mapper()->isInStaticBLASQueue(preMapping.riRes),
    "dagdp rendinsts are expected to all be in one object, but elem=%d has a BLAS!", preMapping.elemIdx);
  return result;
}

void BvhManager::requestStaticBLAS(const RenderableInstanceLodsResource *ri) const
{
  get_bvh_dagdp_instance_mapper()->requestStaticBLAS(ri);
}

size_t BvhManager::hw_instance_size() { return BVHInstanceMapper::getHWInstanceSize(); }

} // namespace dagdp