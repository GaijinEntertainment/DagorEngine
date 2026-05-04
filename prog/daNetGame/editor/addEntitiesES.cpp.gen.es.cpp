// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "addEntitiesES.cpp.inl"
ECS_DEF_PULL_VAR(addEntities);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc on_scene_created_es_event_handler_comps[] ={};
static void on_scene_created_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ecs::EventOnSceneCreated>());
  on_scene_created_es_event_handler(static_cast<const ecs::EventOnSceneCreated&>(evt)
        );
}
static ecs::EntitySystemDesc on_scene_created_es_event_handler_es_desc
(
  "on_scene_created_es",
  "prog/daNetGame/editor/addEntitiesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, on_scene_created_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventOnSceneCreated>::build(),
  0
,"dev,gameClient");
//static constexpr ecs::ComponentDesc on_scene_destroyed_es_event_handler_comps[] ={};
static void on_scene_destroyed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ecs::EventOnSceneDestroyed>());
  on_scene_destroyed_es_event_handler(static_cast<const ecs::EventOnSceneDestroyed&>(evt)
        );
}
static ecs::EntitySystemDesc on_scene_destroyed_es_event_handler_es_desc
(
  "on_scene_destroyed_es",
  "prog/daNetGame/editor/addEntitiesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, on_scene_destroyed_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventOnSceneDestroyed>::build(),
  0
,"dev,gameClient");
static constexpr ecs::ComponentDesc entity_added_to_editor_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("editableObj"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static void entity_added_to_editor_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    entity_added_to_editor_es_event_handler(evt
        , ECS_RO_COMP(entity_added_to_editor_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP_OR(entity_added_to_editor_es_event_handler_comps, "editableObj", bool(true))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc entity_added_to_editor_es_event_handler_es_desc
(
  "entity_added_to_editor_es",
  "prog/daNetGame/editor/addEntitiesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, entity_added_to_editor_es_event_handler_all_events),
  empty_span(),
  make_span(entity_added_to_editor_es_event_handler_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"dev,gameClient");
static constexpr ecs::ComponentDesc editable_entities_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("editableObj"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc editable_entities_ecs_query_desc
(
  "editable_entities_ecs_query",
  empty_span(),
  make_span(editable_entities_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void editable_entities_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, editable_entities_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP_OR(editable_entities_ecs_query_comps, "editableObj", bool( true))) )
            continue;
          function(
              ECS_RO_COMP(editable_entities_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc singleton_mutex_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("singleton_mutex"), ecs::ComponentTypeInfo<ecs::string>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc singleton_mutex_ecs_query_desc
(
  "singleton_mutex_ecs_query",
  empty_span(),
  make_span(singleton_mutex_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void singleton_mutex_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, singleton_mutex_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(singleton_mutex_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP_PTR(singleton_mutex_ecs_query_comps, "singleton_mutex", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
