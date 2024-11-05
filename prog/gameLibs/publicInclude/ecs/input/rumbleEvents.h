//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/event.h>
#include <util/dag_simpleString.h>

namespace rumble
{

#define RUMBLE_ECS_EVENTS                                                    \
  RUMBLE_ECS_EVENT(CmdRumble, SimpleString /*event_name*/, int /*duration*/) \
  RUMBLE_ECS_EVENT(CmdScaledRumble, SimpleString /*event_min_name*/, SimpleString /*event_max_name*/, float /*scale*/)

#define RUMBLE_ECS_EVENT ECS_UNICAST_EVENT_TYPE
RUMBLE_ECS_EVENTS
#undef RUMBLE_ECS_EVENT

}; // namespace rumble
