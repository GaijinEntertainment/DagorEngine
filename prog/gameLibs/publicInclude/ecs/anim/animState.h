//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/event.h>
#include <daECS/core/ecsHash.h>
#include <util/dag_simpleString.h>

ECS_UNICAST_EVENT_TYPE(EventChangeAnimState, ecs::HashedConstString /* stateName */, int /* stateIdx */);
