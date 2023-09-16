//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/entityId.h>
#include <dasModules/dasManagedTab.h>
#include <ecs/game/actions/action.h>

MAKE_TYPE_FACTORY(EntityAction, EntityAction);
MAKE_TYPE_FACTORY(EntityActions, EntityActions);

using EntityActionsTab = Tab<EntityAction>;

DAS_BIND_VECTOR(EntityActionsTab, EntityActionsTab, EntityAction, " EntityActionsTab");
