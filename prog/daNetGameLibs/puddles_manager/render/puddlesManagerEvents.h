// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>
#include <math/dag_Point3.h>
#include <memory/dag_memBase.h>


struct RemovePuddlesInRadius : public ecs::Event
{
  Point3 position;
  float radius;
  ECS_BROADCAST_EVENT_DECL(RemovePuddlesInRadius)
  RemovePuddlesInRadius(const Point3 &pos, float rad) : ECS_EVENT_CONSTRUCTOR(RemovePuddlesInRadius), position(pos), radius(rad) {}
};

struct PreparePuddles : public ecs::Event
{
  Point3 camPos;
  ECS_BROADCAST_EVENT_DECL(PreparePuddles)
  PreparePuddles(const Point3 &cam_pos) : ECS_EVENT_CONSTRUCTOR(PreparePuddles), camPos(cam_pos) {}
};