// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <daECS/core/event.h>

namespace dedicated
{
void statsd_act_time(dag::ConstSpan<float> frame_hist, float mean_ms, const char *scene_fn, int num_players); // time in ms
}

#define DED_SERVER_ECS_EVENTS                      \
  DED_SERVER_ECS_EVENT(DedicatedServerEventOnInit) \
  DED_SERVER_ECS_EVENT(DedicatedServerEventOnTerm)

#define DED_SERVER_ECS_EVENT ECS_BROADCAST_EVENT_TYPE
DED_SERVER_ECS_EVENTS
#undef DED_SERVER_ECS_EVENT
