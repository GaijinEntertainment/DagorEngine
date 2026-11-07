// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "collimatorTubeRivalryES.cpp.inl"
ECS_DEF_PULL_VAR(collimatorTubeRivalry);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc create_collimator_tube_rivalry_nodes_es_comps[] ={};
static void create_collimator_tube_rivalry_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventLevelLoaded>());
  create_collimator_tube_rivalry_nodes_es(static_cast<const EventLevelLoaded&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc create_collimator_tube_rivalry_nodes_es_es_desc
(
  "create_collimator_tube_rivalry_nodes_es",
  "prog/daNetGameLibs/collimator_moa/render/collimatorTubeRivalryES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_collimator_tube_rivalry_nodes_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc collimator_tube_rivalry_view_nodes_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("collimator_tube_rivalry_registrator"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void collimator_tube_rivalry_view_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OnCameraMainViewNodeConstruction>());
  collimator_tube_rivalry_view_nodes_es(static_cast<const OnCameraMainViewNodeConstruction&>(evt)
        );
}
static ecs::EntitySystemDesc collimator_tube_rivalry_view_nodes_es_es_desc
(
  "collimator_tube_rivalry_view_nodes_es",
  "prog/daNetGameLibs/collimator_moa/render/collimatorTubeRivalryES.cpp.inl",
  ecs::EntitySystemOps(nullptr, collimator_tube_rivalry_view_nodes_es_all_events),
  empty_span(),
  empty_span(),
  make_span(collimator_tube_rivalry_view_nodes_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<OnCameraMainViewNodeConstruction>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc get_collimator_tube_rivalry_aim_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("aim_data__aimPosition"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc get_collimator_tube_rivalry_aim_ecs_query_desc
(
  "get_collimator_tube_rivalry_aim_ecs_query",
  empty_span(),
  make_span(get_collimator_tube_rivalry_aim_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_collimator_tube_rivalry_aim_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_collimator_tube_rivalry_aim_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(get_collimator_tube_rivalry_aim_ecs_query_comps, "camera__active", bool)) )
            continue;
          function(
              ECS_RO_COMP(get_collimator_tube_rivalry_aim_ecs_query_comps, "aim_data__aimPosition", float)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc get_collimator_tube_rivalry_lens_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("collimator_moa_render__gun_mod_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("collimator_moa_render__rigid_id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("collimator_moa_render__relem_id"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc get_collimator_tube_rivalry_lens_ecs_query_desc
(
  "get_collimator_tube_rivalry_lens_ecs_query",
  empty_span(),
  make_span(get_collimator_tube_rivalry_lens_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_collimator_tube_rivalry_lens_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_collimator_tube_rivalry_lens_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_collimator_tube_rivalry_lens_ecs_query_comps, "collimator_moa_render__gun_mod_eid", ecs::EntityId)
            , ECS_RO_COMP(get_collimator_tube_rivalry_lens_ecs_query_comps, "collimator_moa_render__rigid_id", int)
            , ECS_RO_COMP(get_collimator_tube_rivalry_lens_ecs_query_comps, "collimator_moa_render__relem_id", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_collimator_tube_rivalry_lens_animchar_ecs_query_comps[] =
{
//start of 5 ro components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("gunmod__collimator_moa_lens_axis"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
  {ECS_HASH("gunmod__collimator_moa_tube_rivalry_darkening"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("gunmod__collimator_moa_tube_rivalry_visible_angle"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc get_collimator_tube_rivalry_lens_animchar_ecs_query_desc
(
  "get_collimator_tube_rivalry_lens_animchar_ecs_query",
  empty_span(),
  make_span(get_collimator_tube_rivalry_lens_animchar_ecs_query_comps+0, 5)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_collimator_tube_rivalry_lens_animchar_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  perform_query(&manager, eid, get_collimator_tube_rivalry_lens_animchar_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(get_collimator_tube_rivalry_lens_animchar_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(get_collimator_tube_rivalry_lens_animchar_ecs_query_comps, "gunmod__collimator_moa_lens_axis", Point3)
            , ECS_RO_COMP(get_collimator_tube_rivalry_lens_animchar_ecs_query_comps, "collres", CollisionResource)
            , ECS_RO_COMP(get_collimator_tube_rivalry_lens_animchar_ecs_query_comps, "gunmod__collimator_moa_tube_rivalry_darkening", float)
            , ECS_RO_COMP(get_collimator_tube_rivalry_lens_animchar_ecs_query_comps, "gunmod__collimator_moa_tube_rivalry_visible_angle", float)
            );

        }
    }
  );
}
