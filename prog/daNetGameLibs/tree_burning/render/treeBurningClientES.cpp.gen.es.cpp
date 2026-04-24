// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "treeBurningClientES.cpp.inl"
ECS_DEF_PULL_VAR(treeBurningClient);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc tree_burning_add_fire_source_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("tree_burning__ignite_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void tree_burning_add_fire_source_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    tree_burning_add_fire_source_es(evt
        , components.manager()
    , ECS_RO_COMP(tree_burning_add_fire_source_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(tree_burning_add_fire_source_es_comps, "tree_burning__ignite_radius", float)
    , ECS_RO_COMP(tree_burning_add_fire_source_es_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc tree_burning_add_fire_source_es_es_desc
(
  "tree_burning_add_fire_source_es",
  "prog/daNetGameLibs/tree_burning/render/treeBurningClientES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tree_burning_add_fire_source_es_all_events),
  empty_span(),
  make_span(tree_burning_add_fire_source_es_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc tree_burning_update_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("tree_burning_manager"), ecs::ComponentTypeInfo<TreeBurningManager>()},
//start of 2 ro components at [1]
  {ECS_HASH("tree_burning__fire_spread_speed"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("tree_burning__time_to_invalidate_shadows"), ecs::ComponentTypeInfo<float>()}
};
static void tree_burning_update_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    tree_burning_update_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(tree_burning_update_es_comps, "tree_burning_manager", TreeBurningManager)
    , ECS_RO_COMP(tree_burning_update_es_comps, "tree_burning__fire_spread_speed", float)
    , ECS_RO_COMP(tree_burning_update_es_comps, "tree_burning__time_to_invalidate_shadows", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc tree_burning_update_es_es_desc
(
  "tree_burning_update_es",
  "prog/daNetGameLibs/tree_burning/render/treeBurningClientES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tree_burning_update_es_all_events),
  make_span(tree_burning_update_es_comps+0, 1)/*rw*/,
  make_span(tree_burning_update_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc register_tree_burning_callbacks_on_level_loaded_es_comps[] ={};
static void register_tree_burning_callbacks_on_level_loaded_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OnLevelLoaded>());
  register_tree_burning_callbacks_on_level_loaded_es(static_cast<const OnLevelLoaded&>(evt)
        );
}
static ecs::EntitySystemDesc register_tree_burning_callbacks_on_level_loaded_es_es_desc
(
  "register_tree_burning_callbacks_on_level_loaded_es",
  "prog/daNetGameLibs/tree_burning/render/treeBurningClientES.cpp.inl",
  ecs::EntitySystemOps(nullptr, register_tree_burning_callbacks_on_level_loaded_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc init_burnable_ri_extra_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("tree_burning_manager"), ecs::ComponentTypeInfo<TreeBurningManager>()},
//start of 2 ro components at [1]
  {ECS_HASH("tree_burning__burnableRiExtra"), ecs::ComponentTypeInfo<ecs::StringList>()},
  {ECS_HASH("tree_burning__leavesShaders"), ecs::ComponentTypeInfo<ecs::StringList>()}
};
static void init_burnable_ri_extra_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_burnable_ri_extra_es(evt
        , ECS_RO_COMP(init_burnable_ri_extra_es_comps, "tree_burning__burnableRiExtra", ecs::StringList)
    , ECS_RO_COMP(init_burnable_ri_extra_es_comps, "tree_burning__leavesShaders", ecs::StringList)
    , ECS_RW_COMP(init_burnable_ri_extra_es_comps, "tree_burning_manager", TreeBurningManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_burnable_ri_extra_es_es_desc
(
  "init_burnable_ri_extra_es",
  "prog/daNetGameLibs/tree_burning/render/treeBurningClientES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_burnable_ri_extra_es_all_events),
  make_span(init_burnable_ri_extra_es_comps+0, 1)/*rw*/,
  make_span(init_burnable_ri_extra_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventRendinstsLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc add_fire_source_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("tree_burning_manager"), ecs::ComponentTypeInfo<TreeBurningManager>()},
//start of 1 ro components at [1]
  {ECS_HASH("tree_burning__additional_source_min_distance"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc add_fire_source_ecs_query_desc
(
  "add_fire_source_ecs_query",
  make_span(add_fire_source_ecs_query_comps+0, 1)/*rw*/,
  make_span(add_fire_source_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void add_fire_source_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, add_fire_source_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(add_fire_source_ecs_query_comps, "tree_burning_manager", TreeBurningManager)
            , ECS_RO_COMP(add_fire_source_ecs_query_comps, "tree_burning__additional_source_min_distance", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc add_tree_chain_burn_source_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("tree_burning_chain__treeHandle"), ecs::ComponentTypeInfo<rendinst::riex_handle_t>()}
};
static ecs::CompileTimeQueryDesc add_tree_chain_burn_source_ecs_query_desc
(
  "add_tree_chain_burn_source_ecs_query",
  empty_span(),
  make_span(add_tree_chain_burn_source_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void add_tree_chain_burn_source_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, add_tree_chain_burn_source_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(add_tree_chain_burn_source_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(add_tree_chain_burn_source_ecs_query_comps, "tree_burning_chain__treeHandle", rendinst::riex_handle_t)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_tree_burning_manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("tree_burning_manager"), ecs::ComponentTypeInfo<TreeBurningManager>()}
};
static ecs::CompileTimeQueryDesc get_tree_burning_manager_ecs_query_desc
(
  "get_tree_burning_manager_ecs_query",
  make_span(get_tree_burning_manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_tree_burning_manager_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_tree_burning_manager_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_tree_burning_manager_ecs_query_comps, "tree_burning_manager", TreeBurningManager)
            );

        }while (++comp != compE);
    }
  );
}
