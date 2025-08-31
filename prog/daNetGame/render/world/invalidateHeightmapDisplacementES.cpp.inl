// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <camera/sceneCam.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entitySystem.h>
#include <ecs/render/resPtr.h>
#include <ecs/render/updateStageRender.h>

#include <math/dag_hlsl_floatx.h>
#include <shaders/invalidate_hmap_displacement.hlsli>

using HmapDisplacementInvalidators = dag::Vector<HmapDisplacementInvalidator>;

ECS_DECLARE_RELOCATABLE_TYPE(HmapDisplacementInvalidators);
ECS_REGISTER_RELOCATABLE_TYPE(HmapDisplacementInvalidators, nullptr);

namespace var
{
static ShaderVariableInfo hmap_displacement_invalidators_count("hmap_displacement_invalidators_count", true);
} // namespace var

ECS_TAG(render)
ECS_TRACK(hmap_displacement_invalidation__enabled)
static void create_hmap_displacement_invalidators_manager_es(
  const ecs::Event &, ecs::EntityManager &manager, bool hmap_displacement_invalidation__enabled)
{
  ecs::EntityId invalidatorsManagerEid = manager.getSingletonEntity(ECS_HASH("hmap_displacement_invalidators_manager"));
  if (!hmap_displacement_invalidation__enabled)
    manager.destroyEntity(invalidatorsManagerEid);
  else if (!invalidatorsManagerEid)
    manager.getOrCreateSingletonEntity(ECS_HASH("hmap_displacement_invalidators_manager"));
}

static void hmap_displacement_invalidators_count_under_limit_update_buffer(const HmapDisplacementInvalidators &data,
  UniqueBufHolder &buffer)
{
  if (!data.empty())
  {
    // We need to fill the rest of the buffer so that extra iterations of loop unrolling are guaranteed to not create any
    // fake heightmap displacement invalidation areas:
    HmapDisplacementInvalidators dataCopy(data);
    dataCopy.resize(HMAP_DISPLACEMENT_INVALIDATORS_MAX_COUNT, {Point3(1e6f, 1e6f, 1e6f), 0, 0});
    buffer.getBuf()->updateData(0, HMAP_DISPLACEMENT_INVALIDATORS_MAX_COUNT * sizeof(HmapDisplacementInvalidator), dataCopy.data(),
      VBLOCK_WRITEONLY);
  }
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void init_hmap_displacement_invalidation_es(const ecs::Event &, UniqueBufHolder &hmap_displacement_invalidators__buffer)
{
  hmap_displacement_invalidators__buffer = dag::buffers::create_persistent_sr_structured(sizeof(HmapDisplacementInvalidator),
    HMAP_DISPLACEMENT_INVALIDATORS_MAX_COUNT, "hmap_displacement_invalidators_buffer");
}

template <typename Callable>
static void add_hmap_displacement_invalidator_ecs_query(ecs::EntityId eid, Callable c);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void add_hmap_displacement_invalidator_es(const ecs::Event &,
  ecs::EntityId eid,
  ecs::EntityManager &manager,
  const TMatrix &transform,
  float hmap_displacement_invalidator__inner_radius,
  float hmap_displacement_invalidator__outer_radius)
{
  ecs::EntityId invalidatorsManagerEid = manager.getSingletonEntity(ECS_HASH("hmap_displacement_invalidators_manager"));
  if (!invalidatorsManagerEid)
    return;

  const Point3 &objectPos = transform.getcol(3);
  add_hmap_displacement_invalidator_ecs_query(invalidatorsManagerEid,
    [&objectPos, hmap_displacement_invalidator__inner_radius, hmap_displacement_invalidator__outer_radius, eid](
      bool &hmap_displacement_invalidators__count_over_limit, UniqueBufHolder &hmap_displacement_invalidators__buffer,
      HmapDisplacementInvalidators &hmap_displacement_invalidators__data) {
      HmapDisplacementInvalidator objectToAdd = {
        objectPos, hmap_displacement_invalidator__inner_radius, hmap_displacement_invalidator__outer_radius, ecs::entity_id_t(eid)};
      hmap_displacement_invalidators__data.push_back(objectToAdd);
      ShaderGlobal::set_int(var::hmap_displacement_invalidators_count,
        eastl::min((int)hmap_displacement_invalidators__data.size(), HMAP_DISPLACEMENT_INVALIDATORS_MAX_COUNT));
      hmap_displacement_invalidators__count_over_limit =
        hmap_displacement_invalidators__data.size() > HMAP_DISPLACEMENT_INVALIDATORS_MAX_COUNT;
      if (!hmap_displacement_invalidators__count_over_limit)
        hmap_displacement_invalidators_count_under_limit_update_buffer(hmap_displacement_invalidators__data,
          hmap_displacement_invalidators__buffer);
    });
}

template <typename Callable>
static void move_hmap_displacement_invalidator_ecs_query(ecs::EntityId eid, Callable c);

ECS_TAG(render)
ECS_TRACK(transform)
ECS_REQUIRE(float hmap_displacement_invalidator__inner_radius, float hmap_displacement_invalidator__outer_radius)
static void move_hmap_displacement_invalidator_es(
  const ecs::Event &, ecs::EntityId eid, ecs::EntityManager &manager, const TMatrix &transform)
{
  ecs::EntityId invalidatorsManagerEid = manager.getSingletonEntity(ECS_HASH("hmap_displacement_invalidators_manager"));
  if (!invalidatorsManagerEid)
    return;

  const Point3 &newOjectPos = transform.getcol(3);
  move_hmap_displacement_invalidator_ecs_query(invalidatorsManagerEid,
    [&newOjectPos, eid](UniqueBufHolder &hmap_displacement_invalidators__buffer,
      HmapDisplacementInvalidators &hmap_displacement_invalidators__data) {
      if (auto it = eastl::find_if(hmap_displacement_invalidators__data.begin(), hmap_displacement_invalidators__data.end(),
            [&](const HmapDisplacementInvalidator &object) { return object.eid == ecs::entity_id_t(eid); });
          it != hmap_displacement_invalidators__data.end())
      {
        int indexToMove = it - hmap_displacement_invalidators__data.begin();
        HmapDisplacementInvalidator &objectToMove = hmap_displacement_invalidators__data[indexToMove];
        objectToMove.worldPos = newOjectPos;
        hmap_displacement_invalidators_count_under_limit_update_buffer(hmap_displacement_invalidators__data,
          hmap_displacement_invalidators__buffer);
      }
      else
        logerr("Couldn't find hmap displacement invalidator to move!");
    });
}

template <typename Callable>
static void remove_hmap_displacement_invalidator_ecs_query(ecs::EntityId eid, Callable c);

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(float hmap_displacement_invalidator__inner_radius, float hmap_displacement_invalidator__outer_radius)
static void remove_hmap_displacement_invalidator_es(const ecs::Event &, ecs::EntityId eid, ecs::EntityManager &manager)
{
  ecs::EntityId invalidatorsManagerEid = manager.getSingletonEntity(ECS_HASH("hmap_displacement_invalidators_manager"));
  if (!invalidatorsManagerEid)
    return;

  remove_hmap_displacement_invalidator_ecs_query(invalidatorsManagerEid,
    [eid](bool &hmap_displacement_invalidators__count_over_limit, UniqueBufHolder &hmap_displacement_invalidators__buffer,
      HmapDisplacementInvalidators &hmap_displacement_invalidators__data) {
      if (auto it = eastl::find_if(hmap_displacement_invalidators__data.begin(), hmap_displacement_invalidators__data.end(),
            [&](const HmapDisplacementInvalidator &object) { return object.eid == ecs::entity_id_t(eid); });
          it != hmap_displacement_invalidators__data.end())
      {
        int indexToErase = it - hmap_displacement_invalidators__data.begin();
        hmap_displacement_invalidators__data.erase(hmap_displacement_invalidators__data.begin() + indexToErase);
        ShaderGlobal::set_int(var::hmap_displacement_invalidators_count,
          eastl::min((int)hmap_displacement_invalidators__data.size(), HMAP_DISPLACEMENT_INVALIDATORS_MAX_COUNT));
        hmap_displacement_invalidators__count_over_limit =
          hmap_displacement_invalidators__data.size() > HMAP_DISPLACEMENT_INVALIDATORS_MAX_COUNT;
        if (!hmap_displacement_invalidators__count_over_limit)
          hmap_displacement_invalidators_count_under_limit_update_buffer(hmap_displacement_invalidators__data,
            hmap_displacement_invalidators__buffer);
      }
      else
        logerr("Couldn't find hmap displacement invalidator to erase!");
    });
}

ECS_TAG(render)
ECS_REQUIRE(eastl::true_type hmap_displacement_invalidators__count_over_limit)
static void hmap_displacement_invalidators_count_over_limit_update_buffer_es(const UpdateStageInfoBeforeRender &,
  HmapDisplacementInvalidators &hmap_displacement_invalidators__data,
  UniqueBufHolder &hmap_displacement_invalidators__buffer)
{
  // When we go above the limit of allowed concurrent objects, we update the objects vector every frame since the decision which
  // objects disable heightmap displacement around them is driven by their closeness to the camera (the effect is applied only to
  // HMAP_DISPLACEMENT_INVALIDATORS_MAX_COUNT objects closest to camera):
  HmapDisplacementInvalidators oldData(hmap_displacement_invalidators__data);
  const Point3 &camPos = get_cam_itm().getcol(3);
  eastl::sort(hmap_displacement_invalidators__data.data(),
    hmap_displacement_invalidators__data.data() + hmap_displacement_invalidators__data.size(),
    [&camPos](const HmapDisplacementInvalidator &first, const HmapDisplacementInvalidator &second) {
      return lengthSq(first.worldPos - camPos) < lengthSq(second.worldPos - camPos);
    });
  // We only update the buffer when first HMAP_DISPLACEMENT_INVALIDATORS_MAX_COUNT objects in the data vector are not the
  // same as those before sorting (irrespective of order):
  if (!eastl::is_permutation(oldData.begin(), oldData.begin() + HMAP_DISPLACEMENT_INVALIDATORS_MAX_COUNT,
        hmap_displacement_invalidators__data.begin(),
        [](const HmapDisplacementInvalidator &first, const HmapDisplacementInvalidator &second) { return first.eid == second.eid; }))
    hmap_displacement_invalidators__buffer.getBuf()->updateData(0,
      HMAP_DISPLACEMENT_INVALIDATORS_MAX_COUNT * sizeof(HmapDisplacementInvalidator), hmap_displacement_invalidators__data.data(),
      VBLOCK_WRITEONLY);
}