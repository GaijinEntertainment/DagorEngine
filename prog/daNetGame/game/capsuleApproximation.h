// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_staticTab.h>
#include <daECS/core/sharedComponent.h>
#include <render/capsulesAO.h>

class CapsuleApproximation
{
public:
  enum
  {
    CAPSULES_NUM = 10,
  };
  template <typename T>
  using StaticData = StaticTab<T, CAPSULES_NUM>;
  bool inited = false;
  StaticData<CapsuleData> capsuleDatas;
  bool onLoaded(const ecs::EntityManager &mgr, ecs::EntityId eid);
  void addCapsuleData(dag::Index16 node_idx, uint16_t coll_node_id, const BBox3 &box);
};

ECS_DECLARE_SHARED_TYPE(CapsuleApproximation);
