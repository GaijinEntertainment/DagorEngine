// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentsMap.h>
#include <daECS/core/entityId.h>
#include <EASTL/fixed_function.h>

typedef eastl::fixed_function<sizeof(void *) * 4, void(ecs::EntityId /*created_entity*/, ecs::ComponentsInitializer & /*cInit*/)>
  remote_recreate_entity_async_cb_t;

// Server only - re-create & broadcast re-create to new template on all remote machines. No-op on client.
// Callback is called afer re-creation and could mutate passed `cInit` is required.
ecs::EntityId remote_recreate_entity_from(ecs::EntityId eid,
  const char *new_templ_name,
  ecs::ComponentsInitializer &&inst_comps = {},
  remote_recreate_entity_async_cb_t &&cb = {});

// Server only - add and/or remove sub-template and broadcast it on remote machines. No-op on client.
// Callback is called afer re-creation and could mutate passed `cInit` is required.
// Do nothing if requested subtemplates already (not)exist in entity (unless not empty `inst_comps` or `force` pass)
ecs::EntityId remote_change_sub_template(ecs::EntityId eid,
  const char *remove_templ_name,
  const char *add_templ_name,
  ecs::ComponentsInitializer &&inst_comps = {},
  remote_recreate_entity_async_cb_t &&cb = {},
  bool force = false);
