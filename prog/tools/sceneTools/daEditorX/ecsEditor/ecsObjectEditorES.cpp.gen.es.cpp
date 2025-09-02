// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "ecsObjectEditorES.cpp.inl"
ECS_DEF_PULL_VAR(ecsObjectEditor);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc riextra_spawn_ri_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void riextra_spawn_ri_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    riextra_spawn_ri_es(evt
        , components.manager()
    , ECS_RO_COMP(riextra_spawn_ri_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(riextra_spawn_ri_es_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc riextra_spawn_ri_es_es_desc
(
  "riextra_spawn_ri_es",
  "prog/tools/sceneTools/daEditorX/ecsEditor/ecsObjectEditorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, riextra_spawn_ri_es_all_events),
  empty_span(),
  make_span(riextra_spawn_ri_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc riextra_destroyed_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void riextra_destroyed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    riextra_destroyed_es_event_handler(evt
        , ECS_RO_COMP(riextra_destroyed_es_event_handler_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc riextra_destroyed_es_event_handler_es_desc
(
  "riextra_destroyed_es",
  "prog/tools/sceneTools/daEditorX/ecsEditor/ecsObjectEditorES.cpp.inl",
  ecs::EntitySystemOps(nullptr, riextra_destroyed_es_event_handler_all_events),
  empty_span(),
  make_span(riextra_destroyed_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
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
inline void editable_entities_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, editable_entities_ecs_query_desc.getHandle(),
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
inline void singleton_mutex_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, singleton_mutex_ecs_query_desc.getHandle(),
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
