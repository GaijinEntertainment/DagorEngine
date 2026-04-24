// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daEditorE/editorCommon/entityEditor.h>
#include <daEditorE/editorCommon/inGameEditor.h>
#include <daEditorE/de_objCreator.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/scene/scene.h>

template <typename Callable>
inline void editable_entities_ecs_query(ecs::EntityManager &manager, Callable c);

template <typename Callable>
inline void singleton_mutex_ecs_query(ecs::EntityManager &manager, Callable c);

EntityObjEditor::EntityObjEditor()
{
  editable_entities_ecs_query(*g_entity_mgr,
    [this](ecs::EntityId eid ECS_REQUIRE(eastl::true_type editableObj = true)) { this->addEntity(eid); });
  initSceneObjects();
  add_con_proc(this);
}

void EntityObjEditor::refreshEids()
{
  editable_entities_ecs_query(*g_entity_mgr, [this](ecs::EntityId eid ECS_REQUIRE(eastl::true_type editableObj = true)) {
    if (!eidsCreated.count(eid))
      this->addEntity(eid);
  });
}

bool EntityObjEditor::checkSingletonMutexExists(const char *mutex_name)
{
  bool found = false;
  singleton_mutex_ecs_query(*g_entity_mgr, [&](ecs::EntityId eid, const ecs::string *singleton_mutex) {
    G_UNUSED(eid);
    if (singleton_mutex && strcmp(singleton_mutex->c_str(), mutex_name) == 0)
      found = true;
  });
  return found;
}

ECS_TAG(dev, gameClient)
void on_scene_created_es_event_handler(const ecs::EventOnSceneCreated &event)
{
  if (EntityObjEditor *editor = get_entity_obj_editor())
  {
    editor->addScene(eastl::get<0>(event));
  }
}

ECS_TAG(dev, gameClient)
void on_scene_destroyed_es_event_handler(const ecs::EventOnSceneDestroyed &event)
{
  if (EntityObjEditor *editor = get_entity_obj_editor())
  {
    editor->removeScene(eastl::get<0>(event));
  }
}

ECS_TAG(dev, gameClient)
ECS_ON_EVENT(on_appear)
void entity_added_to_editor_es_event_handler(const ecs::Event &, ecs::EntityId eid, bool editableObj = true)
{
  if (!editableObj || !is_editor_activated())
    return;
  if (EntityObjEditor *editor = get_entity_obj_editor())
    editor->addEntity(eid);
}
