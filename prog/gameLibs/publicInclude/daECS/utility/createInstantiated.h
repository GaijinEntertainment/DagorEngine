//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/entityManager.h>

namespace ecs
{

class ComponentsInitializer;

EntityId createInstantiatedEntitySync(EntityManager &mgr, const char *name, ComponentsInitializer &&initializer);

} // namespace ecs
