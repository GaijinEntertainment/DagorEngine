// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/event.h>

ECS_UNICAST_EVENT_TYPE(CmdBodyCleanup, float /*ttl*/);              // [0]float (ttl). if ttl < 0 - resurrects */
ECS_UNICAST_EVENT_TYPE(CmdBodyCleanupUpdateTtl, float /*new ttl*/); // [0]float (new ttl)*/