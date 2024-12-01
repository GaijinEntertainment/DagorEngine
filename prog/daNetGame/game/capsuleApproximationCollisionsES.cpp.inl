// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <ecs/anim/anim.h>
#include <animChar/dag_animCharacter2.h>
#include <ecs/render/updateStageRender.h>
#include <shaders/animchar_additional_data_types.hlsli>
#include <render/world/animCharRenderAdditionalData.h>
#include <EASTL/vector_set.h>
#include <EASTL/algorithm.h>

#include "game/capsuleApproximation.h"
#include <ecs/phys/collRes.h>

template <typename Callable>
static void get_attached_to_capsules_preprocess_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
static void get_attached_to_capsules_ecs_query(ecs::EntityId eid, Callable c);

ECS_TAG(render)
ECS_TRACK(slot_attach__attachedTo)
ECS_ON_EVENT(on_appear)
void capsules_collision_on_appear_es(const ecs::Event &,
  ecs::StringList &capsule_approximation_collisions_names,
  ecs::EntityId &slot_attach__attachedTo,
  ecs::IntList &capsule_approximation_collisions_ids)
{
  eastl::vector_set<uint32_t, eastl::less<uint32_t>, framemem_allocator> capsuleNodes;
  capsuleNodes.reserve(capsule_approximation_collisions_names.size());
  for (auto &name : capsule_approximation_collisions_names)
    capsuleNodes.insert(str_hash_fnv1(name.c_str()));
  get_attached_to_capsules_preprocess_ecs_query(slot_attach__attachedTo,
    [&](CollisionResource &collres, ECS_SHARED(CapsuleApproximation) capsule_approximation) {
      for (int i = 0; i < capsule_approximation.capsuleDatas.size(); i++)
      {
        auto approx = capsule_approximation.capsuleDatas[i];
        const char *collNodeName = collres.getNode(approx.collNodeId)->name.c_str();
        if (capsuleNodes.find(str_hash_fnv1(collNodeName)) != capsuleNodes.end())
          capsule_approximation_collisions_ids.push_back(i);
      }
    });
}

ECS_TAG(render)
ECS_NO_ORDER
void capsules_collisions_es(const UpdateStageInfoBeforeRender &,
  ecs::EntityId &slot_attach__attachedTo,
  ecs::IntList &capsule_approximation_collisions_ids,
  ecs::Point4List &additional_data)
{
  int count = 2 * capsule_approximation_collisions_ids.size();
  int offset = animchar_additional_data::request_space<CAPSULE_APPROX>(additional_data, count);
  offset += count;
  get_attached_to_capsules_ecs_query(slot_attach__attachedTo,
    [&](AnimV20::AnimcharBaseComponent &animchar, ECS_SHARED(CapsuleApproximation) capsule_approximation) {
      for (auto approxId : capsule_approximation_collisions_ids)
      {
        auto approx = capsule_approximation.capsuleDatas[approxId];
        mat44f wtm;
        animchar.getNodeTree().getNodeWtm(approx.nodeIndex, wtm);
        vec4f a = v_ldu(&approx.a.x), b = v_ldu(&approx.b.x);
        vec3f rad = v_mul(v_splat_w(a), v_length3(wtm.col0));
        a = v_mat44_mul_vec3p(wtm, a);
        b = v_mat44_mul_vec3p(wtm, b);
        a = v_perm_xyzd(a, rad);

        v_stu(&additional_data[--offset].x, a);
        v_stu(&additional_data[--offset].x, b);
      }
    });
}
