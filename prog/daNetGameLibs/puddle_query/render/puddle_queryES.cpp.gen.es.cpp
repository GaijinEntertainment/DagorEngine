// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "puddle_queryES.cpp.inl"
ECS_DEF_PULL_VAR(puddle_query);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc puddle_query_on_level_loaded_es_comps[] ={};
static void puddle_query_on_level_loaded_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  puddle_query_on_level_loaded_es(evt
        );
}
static ecs::EntitySystemDesc puddle_query_on_level_loaded_es_es_desc
(
  "puddle_query_on_level_loaded_es",
  "prog/daNetGameLibs/puddle_query/render/puddle_queryES.cpp.inl",
  ecs::EntitySystemOps(nullptr, puddle_query_on_level_loaded_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc puddle_query_on_level_unloaded_es_comps[] ={};
static void puddle_query_on_level_unloaded_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  puddle_query_on_level_unloaded_es(evt
        );
}
static ecs::EntitySystemDesc puddle_query_on_level_unloaded_es_es_desc
(
  "puddle_query_on_level_unloaded_es",
  "prog/daNetGameLibs/puddle_query/render/puddle_queryES.cpp.inl",
  ecs::EntitySystemOps(nullptr, puddle_query_on_level_unloaded_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UnloadLevel>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc puddle_query_update_es_comps[] ={};
static void puddle_query_update_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  puddle_query_update_es(evt
        );
}
static ecs::EntitySystemDesc puddle_query_update_es_es_desc
(
  "puddle_query_update_es",
  "prog/daNetGameLibs/puddle_query/render/puddle_queryES.cpp.inl",
  ecs::EntitySystemOps(nullptr, puddle_query_update_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
