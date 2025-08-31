// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/event.h>

struct MotionMatchingStartSearchJob : public ecs::Event
{
  float dt = 0;
  ECS_INSIDE_EVENT_DECL(MotionMatchingStartSearchJob, ::ecs::EVCAST_BROADCAST | ::ecs::EVFLG_PROFILE)
  MotionMatchingStartSearchJob(float _dt) : ECS_EVENT_CONSTRUCTOR(MotionMatchingStartSearchJob), dt(_dt) {}
};

ECS_BROADCAST_EVENT_TYPE(InvalidateAnimationDataBase);
ECS_UNICAST_EVENT_TYPE(AnimationDataBaseAssigned);
