// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/event.h>
#include <daECS/net/connid.h>
#include <math/dag_Point3.h>

void add_entity_in_net_scope(ecs::EntityId eid, int connid);

ECS_BROADCAST_EVENT_TYPE(CmdAddInitialEntitiesInNetScope, int)
ECS_BROADCAST_EVENT_TYPE(CmdAddDefaultEntitiesInNetScope, /*connid*/ int, /*viewPos*/ Point3, /*viewFwd*/ Point3)