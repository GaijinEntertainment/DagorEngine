// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "invalidateHeightmapDisplacementES.cpp.inl"
ECS_DEF_PULL_VAR(invalidateHeightmapDisplacement);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc create_hmap_displacement_invalidators_manager_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("hmap_displacement_invalidation__enabled"), ecs::ComponentTypeInfo<bool>()}
};
static void create_hmap_displacement_invalidators_manager_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    create_hmap_displacement_invalidators_manager_es(evt
        , components.manager()
    , ECS_RO_COMP(create_hmap_displacement_invalidators_manager_es_comps, "hmap_displacement_invalidation__enabled", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_hmap_displacement_invalidators_manager_es_es_desc
(
  "create_hmap_displacement_invalidators_manager_es",
  "prog/daNetGame/render/world/invalidateHeightmapDisplacementES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_hmap_displacement_invalidators_manager_es_all_events),
  empty_span(),
  make_span(create_hmap_displacement_invalidators_manager_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","hmap_displacement_invalidation__enabled");
static constexpr ecs::ComponentDesc init_hmap_displacement_invalidation_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("hmap_displacement_invalidators__buffer"), ecs::ComponentTypeInfo<UniqueBufHolder>()}
};
static void init_hmap_displacement_invalidation_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_hmap_displacement_invalidation_es(evt
        , ECS_RW_COMP(init_hmap_displacement_invalidation_es_comps, "hmap_displacement_invalidators__buffer", UniqueBufHolder)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_hmap_displacement_invalidation_es_es_desc
(
  "init_hmap_displacement_invalidation_es",
  "prog/daNetGame/render/world/invalidateHeightmapDisplacementES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_hmap_displacement_invalidation_es_all_events),
  make_span(init_hmap_displacement_invalidation_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc add_hmap_displacement_invalidator_es_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("hmap_displacement_invalidator__inner_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("hmap_displacement_invalidator__outer_radius"), ecs::ComponentTypeInfo<float>()}
};
static void add_hmap_displacement_invalidator_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    add_hmap_displacement_invalidator_es(evt
        , ECS_RO_COMP(add_hmap_displacement_invalidator_es_comps, "eid", ecs::EntityId)
    , components.manager()
    , ECS_RO_COMP(add_hmap_displacement_invalidator_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(add_hmap_displacement_invalidator_es_comps, "hmap_displacement_invalidator__inner_radius", float)
    , ECS_RO_COMP(add_hmap_displacement_invalidator_es_comps, "hmap_displacement_invalidator__outer_radius", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc add_hmap_displacement_invalidator_es_es_desc
(
  "add_hmap_displacement_invalidator_es",
  "prog/daNetGame/render/world/invalidateHeightmapDisplacementES.cpp.inl",
  ecs::EntitySystemOps(nullptr, add_hmap_displacement_invalidator_es_all_events),
  empty_span(),
  make_span(add_hmap_displacement_invalidator_es_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc move_hmap_displacement_invalidator_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 2 rq components at [2]
  {ECS_HASH("hmap_displacement_invalidator__inner_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("hmap_displacement_invalidator__outer_radius"), ecs::ComponentTypeInfo<float>()}
};
static void move_hmap_displacement_invalidator_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    move_hmap_displacement_invalidator_es(evt
        , ECS_RO_COMP(move_hmap_displacement_invalidator_es_comps, "eid", ecs::EntityId)
    , components.manager()
    , ECS_RO_COMP(move_hmap_displacement_invalidator_es_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc move_hmap_displacement_invalidator_es_es_desc
(
  "move_hmap_displacement_invalidator_es",
  "prog/daNetGame/render/world/invalidateHeightmapDisplacementES.cpp.inl",
  ecs::EntitySystemOps(nullptr, move_hmap_displacement_invalidator_es_all_events),
  empty_span(),
  make_span(move_hmap_displacement_invalidator_es_comps+0, 2)/*ro*/,
  make_span(move_hmap_displacement_invalidator_es_comps+2, 2)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","transform");
static constexpr ecs::ComponentDesc remove_hmap_displacement_invalidator_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 2 rq components at [1]
  {ECS_HASH("hmap_displacement_invalidator__inner_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("hmap_displacement_invalidator__outer_radius"), ecs::ComponentTypeInfo<float>()}
};
static void remove_hmap_displacement_invalidator_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    remove_hmap_displacement_invalidator_es(evt
        , ECS_RO_COMP(remove_hmap_displacement_invalidator_es_comps, "eid", ecs::EntityId)
    , components.manager()
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc remove_hmap_displacement_invalidator_es_es_desc
(
  "remove_hmap_displacement_invalidator_es",
  "prog/daNetGame/render/world/invalidateHeightmapDisplacementES.cpp.inl",
  ecs::EntitySystemOps(nullptr, remove_hmap_displacement_invalidator_es_all_events),
  empty_span(),
  make_span(remove_hmap_displacement_invalidator_es_comps+0, 1)/*ro*/,
  make_span(remove_hmap_displacement_invalidator_es_comps+1, 2)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc hmap_displacement_invalidators_count_over_limit_update_buffer_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("hmap_displacement_invalidators__data"), ecs::ComponentTypeInfo<HmapDisplacementInvalidators>()},
  {ECS_HASH("hmap_displacement_invalidators__buffer"), ecs::ComponentTypeInfo<UniqueBufHolder>()},
//start of 1 ro components at [2]
  {ECS_HASH("hmap_displacement_invalidators__count_over_limit"), ecs::ComponentTypeInfo<bool>()}
};
static void hmap_displacement_invalidators_count_over_limit_update_buffer_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(hmap_displacement_invalidators_count_over_limit_update_buffer_es_comps, "hmap_displacement_invalidators__count_over_limit", bool)) )
      continue;
    hmap_displacement_invalidators_count_over_limit_update_buffer_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
          , ECS_RW_COMP(hmap_displacement_invalidators_count_over_limit_update_buffer_es_comps, "hmap_displacement_invalidators__data", HmapDisplacementInvalidators)
      , ECS_RW_COMP(hmap_displacement_invalidators_count_over_limit_update_buffer_es_comps, "hmap_displacement_invalidators__buffer", UniqueBufHolder)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc hmap_displacement_invalidators_count_over_limit_update_buffer_es_es_desc
(
  "hmap_displacement_invalidators_count_over_limit_update_buffer_es",
  "prog/daNetGame/render/world/invalidateHeightmapDisplacementES.cpp.inl",
  ecs::EntitySystemOps(nullptr, hmap_displacement_invalidators_count_over_limit_update_buffer_es_all_events),
  make_span(hmap_displacement_invalidators_count_over_limit_update_buffer_es_comps+0, 2)/*rw*/,
  make_span(hmap_displacement_invalidators_count_over_limit_update_buffer_es_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc add_hmap_displacement_invalidator_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("hmap_displacement_invalidators__count_over_limit"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("hmap_displacement_invalidators__buffer"), ecs::ComponentTypeInfo<UniqueBufHolder>()},
  {ECS_HASH("hmap_displacement_invalidators__data"), ecs::ComponentTypeInfo<HmapDisplacementInvalidators>()}
};
static ecs::CompileTimeQueryDesc add_hmap_displacement_invalidator_ecs_query_desc
(
  "add_hmap_displacement_invalidator_ecs_query",
  make_span(add_hmap_displacement_invalidator_ecs_query_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void add_hmap_displacement_invalidator_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, add_hmap_displacement_invalidator_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(add_hmap_displacement_invalidator_ecs_query_comps, "hmap_displacement_invalidators__count_over_limit", bool)
            , ECS_RW_COMP(add_hmap_displacement_invalidator_ecs_query_comps, "hmap_displacement_invalidators__buffer", UniqueBufHolder)
            , ECS_RW_COMP(add_hmap_displacement_invalidator_ecs_query_comps, "hmap_displacement_invalidators__data", HmapDisplacementInvalidators)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc move_hmap_displacement_invalidator_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("hmap_displacement_invalidators__buffer"), ecs::ComponentTypeInfo<UniqueBufHolder>()},
  {ECS_HASH("hmap_displacement_invalidators__data"), ecs::ComponentTypeInfo<HmapDisplacementInvalidators>()}
};
static ecs::CompileTimeQueryDesc move_hmap_displacement_invalidator_ecs_query_desc
(
  "move_hmap_displacement_invalidator_ecs_query",
  make_span(move_hmap_displacement_invalidator_ecs_query_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void move_hmap_displacement_invalidator_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, move_hmap_displacement_invalidator_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(move_hmap_displacement_invalidator_ecs_query_comps, "hmap_displacement_invalidators__buffer", UniqueBufHolder)
            , ECS_RW_COMP(move_hmap_displacement_invalidator_ecs_query_comps, "hmap_displacement_invalidators__data", HmapDisplacementInvalidators)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc remove_hmap_displacement_invalidator_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("hmap_displacement_invalidators__count_over_limit"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("hmap_displacement_invalidators__buffer"), ecs::ComponentTypeInfo<UniqueBufHolder>()},
  {ECS_HASH("hmap_displacement_invalidators__data"), ecs::ComponentTypeInfo<HmapDisplacementInvalidators>()}
};
static ecs::CompileTimeQueryDesc remove_hmap_displacement_invalidator_ecs_query_desc
(
  "remove_hmap_displacement_invalidator_ecs_query",
  make_span(remove_hmap_displacement_invalidator_ecs_query_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void remove_hmap_displacement_invalidator_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, remove_hmap_displacement_invalidator_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(remove_hmap_displacement_invalidator_ecs_query_comps, "hmap_displacement_invalidators__count_over_limit", bool)
            , ECS_RW_COMP(remove_hmap_displacement_invalidator_ecs_query_comps, "hmap_displacement_invalidators__buffer", UniqueBufHolder)
            , ECS_RW_COMP(remove_hmap_displacement_invalidator_ecs_query_comps, "hmap_displacement_invalidators__data", HmapDisplacementInvalidators)
            );

        }
    }
  );
}
