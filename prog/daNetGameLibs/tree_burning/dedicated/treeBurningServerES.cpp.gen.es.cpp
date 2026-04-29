// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "treeBurningServerES.cpp.inl"
ECS_DEF_PULL_VAR(treeBurningServer);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc tree_burning_replace_tree_on_server_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("tree_burning__ignite_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void tree_burning_replace_tree_on_server_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    tree_burning_replace_tree_on_server_es(evt
        , ECS_RO_COMP(tree_burning_replace_tree_on_server_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(tree_burning_replace_tree_on_server_es_comps, "tree_burning__ignite_radius", float)
    , ECS_RO_COMP(tree_burning_replace_tree_on_server_es_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc tree_burning_replace_tree_on_server_es_es_desc
(
  "tree_burning_replace_tree_on_server_es",
  "prog/daNetGameLibs/tree_burning/dedicated/treeBurningServerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tree_burning_replace_tree_on_server_es_all_events),
  empty_span(),
  make_span(tree_burning_replace_tree_on_server_es_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
//static constexpr ecs::ComponentDesc send_burnt_trees_to_new_client_es_comps[] ={};
static void send_burnt_trees_to_new_client_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventOnClientConnected>());
  send_burnt_trees_to_new_client_es(static_cast<const EventOnClientConnected&>(evt)
        );
}
static ecs::EntitySystemDesc send_burnt_trees_to_new_client_es_es_desc
(
  "send_burnt_trees_to_new_client_es",
  "prog/daNetGameLibs/tree_burning/dedicated/treeBurningServerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, send_burnt_trees_to_new_client_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnClientConnected>::build(),
  0
);
//static constexpr ecs::ComponentDesc tree_burning_register_server_callback_es_comps[] ={};
static void tree_burning_register_server_callback_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventOnGameAppStarted>());
  tree_burning_register_server_callback_es(static_cast<const EventOnGameAppStarted&>(evt)
        );
}
static ecs::EntitySystemDesc tree_burning_register_server_callback_es_es_desc
(
  "tree_burning_register_server_callback_es",
  "prog/daNetGameLibs/tree_burning/dedicated/treeBurningServerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tree_burning_register_server_callback_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnGameAppStarted>::build(),
  0
);
//static constexpr ecs::ComponentDesc tree_burning_clear_server_burnt_trees_es_comps[] ={};
static void tree_burning_clear_server_burnt_trees_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventOnGameTerm>());
  tree_burning_clear_server_burnt_trees_es(static_cast<const EventOnGameTerm&>(evt)
        );
}
static ecs::EntitySystemDesc tree_burning_clear_server_burnt_trees_es_es_desc
(
  "tree_burning_clear_server_burnt_trees_es",
  "prog/daNetGameLibs/tree_burning/dedicated/treeBurningServerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tree_burning_clear_server_burnt_trees_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnGameTerm>::build(),
  0
);
