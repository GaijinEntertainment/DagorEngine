// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "tiledMapGenES.cpp.inl"
ECS_DEF_PULL_VAR(tiledMapGen);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc gen_tiled_map_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("level__loaded"), ecs::ComponentTypeInfo<bool>()}
};
static void gen_tiled_map_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventLevelLoaded>());
  gen_tiled_map_es(static_cast<const EventLevelLoaded&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc gen_tiled_map_es_es_desc
(
  "gen_tiled_map_es",
  "prog/daNetGameLibs/tiled_map/ui/tiledMapGenES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gen_tiled_map_es_all_events),
  empty_span(),
  empty_span(),
  make_span(gen_tiled_map_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded>::build(),
  0
,"gameClient,render");
static constexpr ecs::ComponentDesc tiled_map_ecs_query_comps[] =
{
//start of 7 ro components at [0]
  {ECS_HASH("tiled_map__leftTop"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("tiled_map__rightBottom"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("tiled_map__tileWidth"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("tiled_map__zlevels"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("tiled_map__genExposure"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("tiled_map__genHeight"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("tiled_map__genHeightOffset"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc tiled_map_ecs_query_desc
(
  "tiled_map_ecs_query",
  empty_span(),
  make_span(tiled_map_ecs_query_comps+0, 7)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void tiled_map_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, tiled_map_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(tiled_map_ecs_query_comps, "tiled_map__leftTop", Point2)
            , ECS_RO_COMP(tiled_map_ecs_query_comps, "tiled_map__rightBottom", Point2)
            , ECS_RO_COMP(tiled_map_ecs_query_comps, "tiled_map__tileWidth", int)
            , ECS_RO_COMP(tiled_map_ecs_query_comps, "tiled_map__zlevels", int)
            , ECS_RO_COMP_OR(tiled_map_ecs_query_comps, "tiled_map__genExposure", float(1.0f))
            , ECS_RO_COMP_OR(tiled_map_ecs_query_comps, "tiled_map__genHeight", float(-1.0f))
            , ECS_RO_COMP_OR(tiled_map_ecs_query_comps, "tiled_map__genHeightOffset", float(-1.0f))
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc set_adaptation_override_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("adaptation_override_settings"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static ecs::CompileTimeQueryDesc set_adaptation_override_ecs_query_desc
(
  "set_adaptation_override_ecs_query",
  make_span(set_adaptation_override_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_adaptation_override_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, set_adaptation_override_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(set_adaptation_override_ecs_query_comps, "adaptation_override_settings", ecs::Object)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc set_active_camera_down_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("fov"), ecs::ComponentTypeInfo<float>()},
//start of 1 ro components at [2]
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc set_active_camera_down_ecs_query_desc
(
  "set_active_camera_down_ecs_query",
  make_span(set_active_camera_down_ecs_query_comps+0, 2)/*rw*/,
  make_span(set_active_camera_down_ecs_query_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_active_camera_down_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, set_active_camera_down_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(set_active_camera_down_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(set_active_camera_down_ecs_query_comps, "camera__active", bool)
            , ECS_RW_COMP(set_active_camera_down_ecs_query_comps, "fov", float)
            );

        }while (++comp != compE);
    }
  );
}
