//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gamePhys/phys/walker/humanSegPhys.h>
#include <daECS/core/sharedComponent.h>

struct SharedSegmentedHumanPhysics : public SegmentedHumanPhysics
{
  SharedSegmentedHumanPhysics() {} // non default ctor to avoid zeroing on creation
  bool onLoaded(ecs::EntityManager &mgr, ecs::EntityId eid);
};

void reloadSharedSegmentedHumanPhysics();

ECS_DECLARE_SHARED_TYPE(SharedSegmentedHumanPhysics);
