// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_adjpow2.h>
#include <ecs/anim/anim.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <ecs/render/updateStageRender.h>
#include <math/dag_hlsl_floatx.h>
#include <shaders/capsuledAOOcclusion.hlsli>

#include <render/capsulesAO.cpp.inl>
ECS_DECLARE_RELOCATABLE_TYPE(CapsulesAOHolder);
ECS_REGISTER_RELOCATABLE_TYPE(CapsulesAOHolder, nullptr);
ECS_AUTO_REGISTER_COMPONENT(CapsulesAOHolder, "capsules_ao", nullptr, 0);

#include "game/capsuleApproximation.h"
template <typename T>
void get_capsules_ecs_query(T cb);

struct GetNodeTreeWtmCB
{
  GetNodeTreeWtmCB(const GeomNodeTree &in_node_tree) : nodeTree(in_node_tree) {}

  __forceinline mat44f operator()(dag::Index16 node_index) const
  {
    G_ASSERT(node_index);
    mat44f wtm;
    nodeTree.getNodeWtm(node_index, wtm);
    return wtm;
  }

  const GeomNodeTree &nodeTree;
};

ECS_TAG(render)
void capsules_ao_es(const UpdateStageInfoBeforeRender &stg, CapsulesAOHolder &capsules_ao, int capsules_ao__max_units_per_cell = 32)
{
  set_capsules_ao<GetNodeTreeWtmCB>(
    stg.camPos, stg.mainCullingFrustum, capsules_ao,
    [&](const GetCapsuleCB<GetNodeTreeWtmCB> &cb) {
      get_capsules_ecs_query([&](ECS_REQUIRE_NOT(ecs::Tag disable_capsule_ao) ECS_SHARED(CapsuleApproximation) capsule_approximation,
                               const AnimV20::AnimcharBaseComponent &animchar, const TMatrix &transform) {
        cb(capsule_approximation.capsuleDatas, GetNodeTreeWtmCB(animchar.getNodeTree()), transform);
      });
    },
    capsules_ao__max_units_per_cell);
}