//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/event.h>

struct ParallelUpdateFrameDelayed : public ecs::Event
{
  float curTime = 0.;
  float dt = 0.;
  ECS_INSIDE_EVENT_DECL(ParallelUpdateFrameDelayed, ::ecs::EVCAST_BROADCAST | ::ecs::EVFLG_PROFILE)
  ParallelUpdateFrameDelayed(float _dt, float _curTime) : ECS_EVENT_CONSTRUCTOR(ParallelUpdateFrameDelayed), dt(_dt), curTime(_curTime)
  {}
};
