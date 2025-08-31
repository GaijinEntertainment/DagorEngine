// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <generic/dag_span.h>
#include <generic/dag_tab.h>
#include <daECS/core/entityManager.h>

class TMatrix;

void load_ecs_templates(ecs::EntityManager &mgr, const char *user_game_mode_import_fn = nullptr);
ecs::EntityId create_simple_entity(
  ecs::EntityManager &mgr, const char *templ_name, ecs::ComponentsInitializer &&amap = ecs::ComponentsInitializer());
void ecs_set_global_tags_context(ecs::EntityManager &mgr, const char *user_game_mode_es_order_fn = nullptr);
Tab<const char *> ecs_get_global_tags_context();


bool reload_ecs_templates(ecs::EntityManager &mgr, eastl::function<void(const char *, ecs::EntityManager::UpdateTemplateResult)> cb);