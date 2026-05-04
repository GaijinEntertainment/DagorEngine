// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <daECS/core/event.h>
#include <EASTL/array.h>

struct FireOnGroundEvent : public ecs::Event
{
  Point3 position;
  float radius;
  float adjustedRadius;
  float groundHeight;
  float animated;
  eastl::array<int, 2> biomeGroupsIds;
  eastl::array<float, 2> biomeGroupWeights;

  ECS_BROADCAST_EVENT_DECL(FireOnGroundEvent)
  FireOnGroundEvent(const Point3 &position,
    float radius,
    float adjusted_radius,
    float ground_height,
    float animated,
    const eastl::array<int, 2> &biome_groups_ids,
    const eastl::array<float, 2> &biome_group_weights) :
    ECS_EVENT_CONSTRUCTOR(FireOnGroundEvent),
    position(position),
    radius(radius),
    adjustedRadius(adjusted_radius),
    groundHeight(ground_height),
    animated(animated),
    biomeGroupsIds(biome_groups_ids),
    biomeGroupWeights(biome_group_weights)
  {}
};