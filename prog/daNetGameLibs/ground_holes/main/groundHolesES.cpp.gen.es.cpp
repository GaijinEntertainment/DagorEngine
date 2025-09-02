// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "groundHolesES.cpp.inl"
ECS_DEF_PULL_VAR(groundHoles);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc ground_holes_on_level_loaded_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("ground_holes_gen"), ecs::ComponentTypeInfo<uint8_t>()}
};
static void ground_holes_on_level_loaded_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ground_holes_on_level_loaded_es(evt
        , ECS_RW_COMP(ground_holes_on_level_loaded_es_comps, "ground_holes_gen", uint8_t)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ground_holes_on_level_loaded_es_es_desc
(
  "ground_holes_on_level_loaded_es",
  "prog/daNetGameLibs/ground_holes/main/groundHolesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ground_holes_on_level_loaded_es_all_events),
  make_span(ground_holes_on_level_loaded_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded>::build(),
  0
);
static constexpr ecs::ComponentDesc ground_holes_changed_es_comps[] =
{
//start of 3 rq components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ground_hole_shape_intersection"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ground_hole_sphere_shape"), ecs::ComponentTypeInfo<bool>()}
};
static void ground_holes_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  ground_holes_changed_es(evt
        );
}
static ecs::EntitySystemDesc ground_holes_changed_es_es_desc
(
  "ground_holes_changed_es",
  "prog/daNetGameLibs/ground_holes/main/groundHolesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ground_holes_changed_es_all_events),
  empty_span(),
  empty_span(),
  make_span(ground_holes_changed_es_comps+0, 3)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,nullptr,"ground_hole_shape_intersection,ground_hole_sphere_shape,transform");
static constexpr ecs::ComponentDesc ground_holes_update_coll_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("ground_holes_gen"), ecs::ComponentTypeInfo<uint8_t>()},
  {ECS_HASH("should_render_ground_holes"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static void ground_holes_update_coll_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ground_holes_update_coll_es(evt
        , ECS_RW_COMP(ground_holes_update_coll_es_comps, "ground_holes_gen", uint8_t)
    , ECS_RW_COMP_PTR(ground_holes_update_coll_es_comps, "should_render_ground_holes", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ground_holes_update_coll_es_es_desc
(
  "ground_holes_update_coll_es",
  "prog/daNetGameLibs/ground_holes/main/groundHolesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ground_holes_update_coll_es_all_events),
  make_span(ground_holes_update_coll_es_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"ground_holes_gen");
static constexpr ecs::ComponentDesc set_holes_changed_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("ground_holes_gen"), ecs::ComponentTypeInfo<uint8_t>()},
  {ECS_HASH("should_render_ground_holes"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc set_holes_changed_ecs_query_desc
(
  "set_holes_changed_ecs_query",
  make_span(set_holes_changed_ecs_query_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_holes_changed_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, set_holes_changed_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(set_holes_changed_ecs_query_comps, "ground_holes_gen", uint8_t)
            , ECS_RW_COMP_PTR(set_holes_changed_ecs_query_comps, "should_render_ground_holes", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gather_holes_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ground_hole_sphere_shape"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ground_hole_shape_intersection"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc gather_holes_ecs_query_desc
(
  "gather_holes_ecs_query",
  empty_span(),
  make_span(gather_holes_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void gather_holes_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_holes_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_holes_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(gather_holes_ecs_query_comps, "ground_hole_sphere_shape", bool)
            , ECS_RO_COMP(gather_holes_ecs_query_comps, "ground_hole_shape_intersection", bool)
            );

        }while (++comp != compE);
    }
  );
}
