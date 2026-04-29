//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/event.h>

struct ParallelUpdateFrameDelayed : public ecs::Event
{
  float curTime; // TODO: make it double and remove `curTimeD`
  float dt;
  double curTimeD;
  ECS_INSIDE_EVENT_DECL(ParallelUpdateFrameDelayed, ::ecs::EVCAST_BROADCAST | ::ecs::EVFLG_PROFILE)
  ParallelUpdateFrameDelayed(float d, double ct) : ECS_EVENT_CONSTRUCTOR(ParallelUpdateFrameDelayed), dt(d), curTime(ct), curTimeD(ct)
  {}
};
