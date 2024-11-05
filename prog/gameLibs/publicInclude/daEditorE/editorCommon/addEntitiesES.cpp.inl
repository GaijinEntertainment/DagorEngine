// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daEditorE/editorCommon/entityEditor.h>
#include <daEditorE/de_objCreator.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>

template <typename Callable>
static __forceinline void editable_entities_ecs_query(Callable c);

template <typename Callable>
inline void singleton_mutex_ecs_query(Callable c);

EntityObjEditor::EntityObjEditor()
{
  editable_entities_ecs_query([this](ecs::EntityId eid ECS_REQUIRE(eastl::true_type editableObj = true)) { this->addEntity(eid); });
  add_con_proc(this);
}

void EntityObjEditor::refreshEids()
{
  editable_entities_ecs_query([this](ecs::EntityId eid ECS_REQUIRE(eastl::true_type editableObj = true)) {
    if (!eidsCreated.count(eid))
      this->addEntity(eid);
  });
}

bool EntityObjEditor::checkSingletonMutexExists(const char *mutex_name)
{
  bool found = false;
  singleton_mutex_ecs_query([&](ecs::EntityId eid, const ecs::string *singleton_mutex) {
    G_UNUSED(eid);
    if (singleton_mutex && strcmp(singleton_mutex->c_str(), mutex_name) == 0)
      found = true;
  });
  return found;
}
