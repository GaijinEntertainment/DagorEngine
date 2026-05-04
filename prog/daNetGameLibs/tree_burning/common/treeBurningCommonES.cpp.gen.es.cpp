// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "treeBurningCommonES.cpp.inl"
ECS_DEF_PULL_VAR(treeBurningCommon);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc tree_burning_destroy_burned_tree_es_comps[] ={};
static void tree_burning_destroy_burned_tree_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventDestroyBurnedTree>());
  tree_burning_destroy_burned_tree_es(static_cast<const EventDestroyBurnedTree&>(evt)
        );
}
static ecs::EntitySystemDesc tree_burning_destroy_burned_tree_es_es_desc
(
  "tree_burning_destroy_burned_tree_es",
  "prog/daNetGameLibs/tree_burning/common/treeBurningCommonES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tree_burning_destroy_burned_tree_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventDestroyBurnedTree>::build(),
  0
,"server");
//static constexpr ecs::ComponentDesc tree_burning_destroy_burned_trees_es_comps[] ={};
static void tree_burning_destroy_burned_trees_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventDestroyBurnedTreesInRadius>());
  tree_burning_destroy_burned_trees_es(static_cast<const EventDestroyBurnedTreesInRadius&>(evt)
        );
}
static ecs::EntitySystemDesc tree_burning_destroy_burned_trees_es_es_desc
(
  "tree_burning_destroy_burned_trees_es",
  "prog/daNetGameLibs/tree_burning/common/treeBurningCommonES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tree_burning_destroy_burned_trees_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventDestroyBurnedTreesInRadius>::build(),
  0
);
//static constexpr ecs::ComponentDesc deserialize_pending_tree_burning_data_es_comps[] ={};
static void deserialize_pending_tree_burning_data_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventRendinstsLoaded>());
  deserialize_pending_tree_burning_data_es(static_cast<const EventRendinstsLoaded&>(evt)
        );
}
static ecs::EntitySystemDesc deserialize_pending_tree_burning_data_es_es_desc
(
  "deserialize_pending_tree_burning_data_es",
  "prog/daNetGameLibs/tree_burning/common/treeBurningCommonES.cpp.inl",
  ecs::EntitySystemOps(nullptr, deserialize_pending_tree_burning_data_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventRendinstsLoaded>::build(),
  0
);
//static constexpr ecs::ComponentDesc clear_tree_burning_data_on_game_term_es_comps[] ={};
static void clear_tree_burning_data_on_game_term_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventOnGameTerm>());
  clear_tree_burning_data_on_game_term_es(static_cast<const EventOnGameTerm&>(evt)
        );
}
static ecs::EntitySystemDesc clear_tree_burning_data_on_game_term_es_es_desc
(
  "clear_tree_burning_data_on_game_term_es",
  "prog/daNetGameLibs/tree_burning/common/treeBurningCommonES.cpp.inl",
  ecs::EntitySystemOps(nullptr, clear_tree_burning_data_on_game_term_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnGameTerm>::build(),
  0
);
